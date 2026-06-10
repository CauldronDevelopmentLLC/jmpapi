# Exec

jmpapi pushes dynamic logic to the DB in stored procedures. `exec` is the
deliberate exception: when work doesn't belong in SQL — resizing an image,
shelling out to a tool, touching the filesystem — an endpoint can run an
external program as a step in its pipeline.

An `exec` step exchanges a metadata envelope with jmpapi and may modify args,
set the response, or short-circuit with a status code. Downstream handlers
consume `args`, so modifying `args` is how `exec` feeds data forward.

To run a program only under some condition (a missing file, a DB result), wrap
it in a conditional — see [conditions.md](conditions.md).

## Pipeline model

A method endpoint runs a sequence of statements ending in one terminal
**handler** — the thing that replies (`path`, `resource`, a replying `query`,
…, per [handlers.md](handlers.md)). An `exec` is one such statement. A
sequence is a `steps:` list: statements run in order, and each either
replies (ending the request) or passes to the next. A method body that is a
bare YAML list is shorthand for `{steps: [...]}` — use the explicit form
when the method also declares keys like `args` or `help`:

```yaml
get:
  args: {q: {min: 2, optional: true}}
  steps:
    - sql: "CALL LogSearch({session.user}, {args.q})"
      return: pass
    - sql: "CALL Search({args.q})"
      return: list
```

`steps` cannot be combined with a handler-implying key (`sql`, `path`, …)
in the same statement — a statement is either a pipeline or a handler.

`exec` is async: like a DB query it interrupts synchronous processing, runs off
the event loop, and resumes when it completes. On completion it either

  - **short-circuits** — sends the reply itself and stops, or
  - **feeds forward** — applies its result to the request and passes to the
    next statement (and ultimately the terminal handler).

```yaml
/images/{size}/{path}:
  get:
    if:   {exists: '{options.cache-root}/{args.size}/{args.path}'}
    then: {path: '{options.cache-root}'}            # cache hit: serve it
    else:                                           # miss: resize, then serve
      - exec:
          cmd:   resize-image
          input: {src:  '{options.src-root}/{args.path}',
                  dst:  '{options.cache-root}/{args.size}/{args.path}',
                  size: '{args.size}'}
      - path: '{options.cache-root}'
```

An `exec` can also run *after* a query: a query statement with
[`into:`](sql.md#capturing-results) captures its result and continues
(`return: pass` discards and continues), so an exec can consume — or
transform — what SQL produced, and a later statement replies:

```yaml
/image/{id}/{size}:
  args: {id: {type: u64}, size: {type: u32}}
  get:
    - sql: "CALL ImageGet({args.id})"     # fetch the original blob
      return: binary
      into: original
    - exec:
        cmd:   'resize --size {args.size}'
        input: {src: '{original}'}        # bytes land in a temp file
      # returns {"files": {"out": {"path": ..., "type": "image/png"}}}
    - sql: "CALL ImageSetSize({args.id}, {args.size}, {files.out})"
      return: pass                        # cache it, keep going
    - reply: '{files.out}'                # serve the resized image
```

## The exec protocol

The program reads a JSON **request envelope** on stdin and writes a JSON
**result** on stdout. stderr is logged (prefixed with the PID).

### Request (stdin)

By default the envelope is:

```json
{"args": { ... }}
```

`args` holds the validated request args. Set `input:` to send something else;
it is a JSON template resolved against the request (the usual `{namespace.key}`
syntax). A lone `'{ref}'` keeps its native type — `'{args}'` sends the args
object, `'{args.size}'` a number (see [Typed values](sql.md#typed-values)).
`input` replaces the whole envelope, so include `{args}` explicitly if the
program still needs it.

### Temporary files and binary data

Each `exec` step gets a private temporary directory, exported to the process
as `TMPDIR` and removed when the step completes. Standard temp-file tooling
(`mktemp`, Python's `tempfile`) lands in it automatically.

A *binary* ref in `input:` — `{body}`, `{files.<name>}`, or a captured
binary result — is written to a file in that directory and its **path** is
substituted into the envelope as a string:

```yaml
exec:
  cmd:   resize-image
  input: {src: '{files.photo}', type: '{files.photo.type}'}
```

```json
{"src": "/tmp/jmpapi-1a2b/src", "type": "image/png"}
```

Metadata refs (`.size`, `.type`, `.filename`) resolve as plain values, as
everywhere else.

### Result (stdout)

A JSON dict. Every field is optional:

| Field              | Effect                                                       |
|--------------------|--------------------------------------------------------------|
| `code`             | HTTP status (default `200`). Non-2xx → reply and stop.       |
| `error`            | Logged; included in an error reply.                          |
| `args` (or `data`) | Merged into the request args, visible to later `{args.*}`.   |
| `headers`          | Response headers to set.                                     |
| `files`            | Files the program wrote, returned as binary values.  See below. |
| `response`         | Reply body. If present, `exec` replies and stops.            |

Continuation rule: if `code` is 2xx and there is no `response`, `exec`
**continues** to the next statement; otherwise it **replies and stops**.

### Returning files

`files` maps names to files the program wrote — typically under `$TMPDIR`:

```json
{"files": {"out": {"path": "/tmp/jmpapi-1a2b/resized.png",
                   "type": "image/png"}}}
```

A bare path string is shorthand for `{"path": ...}`. Each file is read and
becomes a binary value under `{files.<name>}` — exactly like an uploaded
multipart part (a returned name shadows an uploaded part) — so later
statements can bind it into SQL, pass it to another `exec`, or serve it with
[`reply:`](handlers.md#reply). Optional `type` and `filename` fill the
`.type` / `.filename` metadata.

### Errors

A non-zero exit, empty or unparseable stdout, or a thrown exception replies
`500`. A non-2xx `code` replies that status (logging `error` if given).

## Configuration

An `exec` statement:

```yaml
exec: resize-image                  # string: shell-parsed command
```
```yaml
exec:
  cmd:   [resize-image, --fast]     # string or argv list
  input: { ... }                    # optional; default {args: {args}}
```

At the method level it runs as a pre-step before the terminal handler. Inside
an [`if`](conditions.md) it runs only when the branch is taken.

Variables (`{args.*}`, `{session.*}`, `{options.*}`) are resolved in `cmd` and
`input` before the process runs.
