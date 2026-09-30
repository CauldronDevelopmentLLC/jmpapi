# Design / implementation notes

How the [exec](exec.md) and [conditions](conditions.md) design maps onto
existing cbang code. Internal — not user documentation.

## Execution model

Originally handlers were synchronous first-true-wins: `bool operator()(ctx)`
returned `false` to **pass** (the `HandlerGroup` loop advanced) or `true` to
**stop**. An async handler must return `true` to yield to the event loop, after
which the loop has returned and the downstream handlers are unreachable — so
async breaks "pass". The old `ArgFilterHandler` only worked around this by
hard-wiring one `child`.

The return value is replaced with an explicit continuation:

```cpp
using Cont = std::function<void (const CtxPtr &)>;
virtual void operator()(const CtxPtr &ctx, const Cont &next) = 0;
```

  - a terminal handler replies and ignores `next`
  - a pass / matcher / wrapper calls `next(ctx)`
  - an async handler captures `next` and `ctx`, returns to the loop, and on
    completion either replies or calls `next(ctx)`. It stays alive across the
    wait — `Query` via a `self` SmartPointer, an `ExecProcess` via the proc
    pool, a `cmd` condition via its `CmdConditionProc`.

`HandlerGroup` folds its children right-to-left into one `Cont` (each child's
continuation runs the rest; the last child's is the group's own `next`), then
invokes it. Nested groups pass the outer `next` through, so "pass" crosses
group boundaries. The top-level driver supplies a terminal `next` that replies
`404`. A YAML-list sequence folds the same way.

## Statements

A method body, a `then`/`else` body, or a sequence item is a *statement*.
`API::createStatementHandler(cfg)` dispatches on shape:

  - a **list** → a `HandlerGroup` folded into the chain (a sequence)
  - a dict with **`if`** → a `ConditionalHandler`
  - otherwise a single statement: `addSteps` (headers + the `exec` pre-step)
    then the endpoint handler

Method-level validation (`args`/`allow`/`deny`) wraps the whole statement once,
applied by `createMethodsHandler` via `Config::addValidation` — not re-applied
per sequence item.

## Exec handler

  - **`ExecHandler`/`ExecProcess`** (`api/handler/`) generalize the old
    `ArgFilterHandler`/`ArgFilterProcess`. `exec()` writes the resolved `input`
    envelope to stdin. `done()` parses the result and applies it: merge `args`,
    set `headers`, then either `next(ctx)` (continue) or `ctx->reply(...)`
    (short-circuit).
  - Added as a pre-step in `addSteps`, before the endpoint handler.

## Merged args downstream

Merged `args` must be visible to later `{args.*}` resolution. `ArgsHandler`
sets the resolver `args` namespace from `ctx->getArgs()`; after an `exec`
merges, that binding has to be refreshed (or share the args object) so the
next handler's SQL and templates see the new values.

## Conditions

Conditions live in `api/condition/`, one class per file: a `Condition` base
with a `parse` factory, the sync `exists`/compare/`not`/`and`/`or` evaluators,
and the async `sql`/`cmd` evaluators. Evaluation is continuation-based
(`BoolCallback`) so `and`/`or` short-circuit correctly even when an operand is
an async `sql`/`cmd`. Those async conditions reuse the same plumbing as a query
or an `exec` — which is why a DB- or process-driven condition needs no separate
"run a query, store a flag, then test it" step.

Typed value substitution (a lone `'{ref}'` resolving to its native JSON type)
is what lets numeric `<`/`<=` compares work and `exec` `input` embed
non-strings — see [Typed values](sql.md#typed-values).

## Process pool

The `Event::SubprocessPool` is `maxActive = CPU count`, no timeout.
Long-running resizes will queue; consider a per-step timeout and/or a separate
pool. `exec` and `cmd` are blocking-async only (the request waits for the
process); there is no fire-and-forget mode.

## Binary data

User docs: [binary.md](binary.md). Decisions: side-channel refs for input,
prepared-statement binding for blobs, raw response body for output. Maps onto
cbang as follows; all of it is net-new work.

### Plan

Ordered so the build stays green at each step. Purely additive — no config is
removed — so `minVer` stays `1.2.0`.  `[x]` done, `[~]` partial, `[ ]` to do.

1. `[x]` **Multipart parser** (cbang HTTP; independent, do first). Parse
   `multipart/form-data`: split by boundary, read each part's
   `Content-Disposition` (`name`, optional `filename`) and `Content-Type` into
   (name, filename, type, bytes). A standalone util, unit-tested offline
   against fixture bodies — no API wiring yet. See [Multipart
   parsing](#multipart-parsing).
   *Done: `cbang/http/MultipartParser.{h,cpp}` (`getBoundary` + `parse`,
   binary-safe, quoted-string-aware); 11 offline cases in
   `cbang/tests/multipartTests`. Hardened via adversarial review (line-anchored
   first delimiter; quoted-value `;`/escape spoofing fixed).*

2. `[x]` **Context binary store + resolver roots** (see [Side channel on
   Context](#side-channel-on-context)). Add the body/file-parts store to
   `Context`; teach `Resolver` the `{body}`/`{files.*}` roots — metadata fields
   resolve as strings/numbers, bytes refs are flagged binary and error in a
   string context. Populate it from `Request::parseArgs` / an API pre-step:
   plain multipart fields → `args`, file parts → store, raw body → `{body}`.
   *Done: `cbang/api/Blob` — a `JSON::Dict` subclass holding the bytes in a
   member with `size`/`type`/`filename` as entries, so metadata resolves via
   the normal dotted-path select (no `Resolver` changes) and `write()` throws
   on any string/JSON use of the bytes. `Context::parseBody()` (called from
   `API::operator()`) sets the `body`/`files` resolver roots and folds plain
   multipart fields into args. Tests: `apiTests` Body*/Upload cases.*

3. `[x]` **Prepared-statement binding** (the big one; see [Prepared-statement
   parameter binding](#prepared-statement-parameter-binding)). Add
   `mysql_stmt_*` prepare/bind/exec to `EventDB` (blob + existing scalar
   types), results streamed as now. Change the query build from "resolve to a
   string" to "resolve to a string **+ ordered bind list**": a binary ref in
   SQL mode emits `?` and appends its bytes; non-binary refs interpolate as
   before. Thread the list through `QueryDef`/`Query`. Binary-only to start.
   *Done: `DB`/`EventDB` fully converted to the `mysql_stmt_*` path,
   regression-tested DB-free via `cbang/tests/dbTests`. Bind list:
   `Resolver::resolveSQL` emits `?` per binary ref and collects the bytes;
   threaded `QueryDef` → `Query` → `EventDB::query` → `QueryCallback` →
   `DB::queryNB`, bound as `MYSQL_TYPE_BLOB` in `DB::startExecute`.
   `QueryDef` checks placeholder vs param count quote-aware, so a binary ref
   inside a SQL string literal fails with a clear error; `startExecute`
   re-checks via `mysql_stmt_param_count` as a backstop. Tested: dbTests
   Blob/Upload/BlobEmbed (exact bytes verified by captured binds).*

4. `[x]` **Binary response** (see [Binary response](#binary-response)). Add a raw
   `Context` reply (bytes + `Content-Type`) over `Request::reply(Event::Buffer)`;
   implement `return: binary` (row 1 / col 1 binary-safe, `Content-Type` from
   the `content-type` key → 2nd column → `application/octet-stream`, no row →
   `404`).
   *Done: `Query::returnBinary` packages the bytes as a `Blob` through the
   existing JSON reply callback; `Context::reply` detects a Blob and replies
   raw (`Request::reply(Event::Buffer)` + `Content-Type`). The `content-type`
   key is read by `QueryDef` and resolved per request. Tested: dbTests
   Binary/BinaryTypeCol/BinaryCTKey/BinaryNotFound.*

5. `[x]` **Validation + config + spec.** `body:`/`files:` declaration blocks
   (`required`/`max-size`/`type` → `400`/`413`/`415`) and the `content-type`
   key; add the request-body and `multipart/form-data` entries to the generated
   OpenAPI spec.
   *Done: `api/handler/BodyHandler` validates declared `body:`/`files:`
   (size suffixes K/M/G, `type` string/list/`*` glob matched against the
   media type sans parameters); collected by `Config` and wrapped via
   `addValidation` like `args`. Spec: `requestBody` with binary schema
   (`application/octet-stream` or the declared type) or a
   `multipart/form-data` object schema with per-file properties. Tests:
   apiTests BodyRequired/BodyTooBig/BodyType/UploadBadType + SpecTest. The
   offline drivers now dispatch through `HTTP::RequestErrorHandler` as the
   server does, so thrown validation errors reply with real status codes.*

6. `[x]` **Tests.** Offline C++ drivers (no server / no curl): multipart-parser
   units; resolver `{body}`/`{files.*}` (metadata resolves, bytes-in-string
   errors). The store + `return: binary` round-trip needs a DB-backed harness —
   the current `apiTests` fixture is DB-free, so that path needs either a test
   DB or a faked `EventDB`.
   *Done: multipart-parser units (`multipartTests`); resolver `{body}`/
   `{files.*}` units (`apiTests`: metadata + typed compare + bytes-in-string
   error + multipart fold); binding units (`dbTests`: raw-body and multipart
   blob → bound `?` param byte-exact, embedded-ref error); `return: binary`
   units (`dbTests`: bytes + Content-Type chain + 404); validation units
   (`apiTests`: 400/413/415 + spec). Both halves of the store/fetch
   round-trip are covered against the faked `EventDB`.*

7. `[ ]` **Acceptance.** Upload via multipart → store the blob → fetch via
   `return: binary` with the right `Content-Type`, end to end.

### Side channel on Context

`JSON::Value` has no byte type, so binary cannot ride in the `args` dict. The
`Context` gains a binary store — the raw body and the parsed multipart file
parts (name → {bytes, filename, type, size}). The `Resolver` learns two roots:

  - `{body}` / `{body.type}` / `{body.size}`
  - `{files.<name>}` (+ `.filename`/`.type`/`.size`)

The metadata fields (`.type`, `.size`, `.filename`) are ordinary strings and
numbers and resolve normally. The *bytes* refs (`{body}`, `{files.<name>}`) are valid
only as a whole value bound into SQL or written as the response; using one in a
string context is an error.

### Multipart parsing

cbang HTTP has no multipart parser (only raw-body access via
`Request::getInputBuffer`). Add one: split `multipart/form-data` by boundary,
read each part's `Content-Disposition` (`name`, optional `filename`) and
`Content-Type`. Plain fields (no filename) merge into the args dict before
validation; file parts go to the Context binary store. Likely a parser util
under `http/`, driven from `Request::parseArgs` or an API pre-step.

### Prepared-statement parameter binding

The biggest change. Today `QueryDef::query` resolves the SQL to one finished
string (`Resolver::resolve(sql, true)`) and `EventDB` runs it as text — there
is no parameter binding. To bind blobs:

  - `EventDB` gains prepared-statement support (`mysql_stmt_*`): prepare, bind
    an ordered list of params (at least blob + the existing scalar types),
    execute, and stream results as now.
  - The query build changes from "resolve to a string" to "resolve to a string
    **plus an ordered list of bound values**." When the resolver hits a binary
    ref in SQL mode it emits a `?` placeholder and appends the bytes to the
    bind list; non-binary refs interpolate as before. `Query`/`QueryDef` thread
    the bind list through to the prepared statement.
  - Scope question: bind only binary params (mixed text-interpolation + `?`
    placeholders in one statement) or move all params to binding over time. The
    docs commit only to binary being bound; start there.

### Binary response

`Context` is JSON-only; add a raw reply (bytes + `Content-Type`) that drops to
`Request::reply(Event::Buffer)` — the path `FileHandler`/`ResourceHandler`
already use. `return: binary` reads row 1 / column 1 binary-safe
(`DB::getString` keeps length, no JSON escaping) and replies with it; the
`Content-Type` comes from the `content-type` key, else a second column, else
`application/octet-stream`; no row → `404`.

### Open

  - Large uploads are buffered in memory (no streaming to disk or to the DB).
  - The binary path is verified against the faked `EventDB` only; the opt-in
    real-MariaDB acceptance test (plan step 7) has not been run.

## Exec files & pipelines

User docs: [exec.md](exec.md) (temp files, `files` result),
[sql.md](sql.md#capturing-results) (`into:`, `return: pass`),
[handlers.md](handlers.md#reply) (`reply:`). Enables e.g. image storage
with resize-on-request — no DB queue, no external worker.

Decisions:

  - **No carried status.** The statement that replies chooses the code;
    `reply:` takes a sibling `code:` key. `status` stays terminal.
  - **Temp files via `TMPDIR`.** A private directory per exec step, exported
    to the child as `TMPDIR`, removed when the step completes (after output
    files are read). Binary input refs are written there; the envelope
    carries path strings.
  - **Returned files merge into the `files` root**, shadowing same-name
    uploads, so binding/validation/reply work identically.
  - Resolved-result capture (`into:`) lives on the *query* statement; exec
    feeds forward via `args` and `files` as before.

### Plan

1. `[x]` **`steps:` keyword.** `createStatementHandler`: a dict with
   `steps` folds the list (after its `headers`/`exec` pre-steps); a bare
   list body is shorthand for `{steps: [...]}`. A handler-implying key
   alongside `steps` is a config error. Lets a method keep `args`/`help`
   beside its pipeline. *Tested: apiTests Steps/StepsZero; spec picks up
   `help` + args beside `steps`.*
2. `[x]` **Query capture.** `into: <name>` on a query statement: the
   formatted result (incl. a binary Blob) is set on the request resolver and
   the chain continues; errors still reply. `return: pass` runs and
   discards. `QueryDef` reads both keys (combining them is a config error);
   `QueryHandler` switches reply → capture on success. *Tested: dbTests
   Pipe (pass + dict capture + shaped reply) and BlobPipe (binary capture →
   re-bound `?` param → raw reply with carried Content-Type).*
3. `[x]` **`reply:` statement.** `ReplyHandler` deep-copies its template,
   resolves it inside a wrapper list (so a lone `'{ref}'` keeps its native
   type, binary included), and replies — `Context::reply` handles Blob →
   raw body. Sibling `code:` sets the status. *Tested: apiTests
   Reply/ReplyCode.* Spec generation now tolerates bare-list method bodies.
4. `[x]` **Exec temp-file input.** `ExecHandler` walks the resolved `input`
   envelope; each Blob is written to a per-step `TemporaryDirectory`
   (named by its envelope key) and replaced by its path string. The
   directory is owned by `ExecProcess` (auto-removed after `done`) and
   exported as `TMPDIR`.
5. `[x]` **Exec file output.** `ExecProcess::done` reads the result `files`
   dict (path or {path,type,filename}), loads each into a Blob, and merges
   into the resolver's `files` root before continuing.
6. `[x]` **Tests.** apiTests: Steps/StepsZero, Reply/ReplyCode, Upcase
   (body → temp file → transformed file → binary reply, temp dir verified
   removed); dbTests: Pipe, BlobPipe.
7. `[x]` **Acceptance.** dbTests Resize: PUT body with NUL bytes →
   `ImageSet(?)` bound byte-exact → fetch via `into` → exec transform via
   temp files → `ImageSetSize(?)` re-bound transformed bytes → raw
   `image/png` reply. The full exec.md pipeline against the fakes.

## Bind everything & strict refs

User docs: [sql.md](sql.md#variables). Triggered by the `{~ref}`-eaten-at-
load-time bug: `API::load` resolved the whole config once and a tilde ref
could never be "missing", so every `{~x}` became literal `null` before the
first request.

Decisions:

  - **One resolve mode parameter, no instance state.** `resolve(s, partial)`;
    the load-time pass is *partial* (missing refs, tilde included, stay
    literal for request time). `~` handling moved out of `select()` (pure
    lookup) into the resolve layer (`selectRef`).
  - **No SQL text mode.** `resolveSQL` binds *every* ref as a prepared-
    statement parameter (`?`); quoting/escaping code deleted, injection
    structurally impossible. Params are JSON values end-to-end
    (`DB::queryNB`): null → SQL `NULL`, bool → 1/0, else string value.
  - **Strict refs.** At request time a missing `{ref}` is an error
    naming the fix (`use {~ref}`); `{~ref}` resolves null/NULL. Optional
    args must be referenced as `{~args.x}`.
  - A ref inside a SQL string literal is caught by the quote-aware
    placeholder/param count check → "use CONCAT()". `:S` (force SQL quote)
    is gone; a `:fmt` spec on a SQL ref formats the value, then binds the
    string — uniform with non-SQL templates.
  - SQL *fragments* can only come from `{options.*}`, resolved at load.

Config migration: `{args.x}` where `x` is optional → `{~args.x}`;
`LIKE '%{q:s}%'` → `LIKE CONCAT('%', {q}, '%')`.

Tested: resolverTests (bind/partial/strict/typed), dbTests Opt*/Strict*/
AssetUpload (multipart + exec renditions + `{~files.*}` NULL binds), all
goldens regenerated and reviewed.
