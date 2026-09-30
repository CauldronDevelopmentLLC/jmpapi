# Conditions

General control flow for an endpoint pipeline. Conditions are independent of
any handler: they wrap a **statement** — a step like
[`exec`](exec.md) or `query`, a terminal handler such as `path`, a
nested conditional, or a sequence of these.

```yaml
/images/{size}/{path}:
  args: {size: {type: u32}, path: {}}
  get:
    if:   {exists: '{options.cache-root}/{args.size}/{args.path}'}
    then: {path: '{options.cache-root}'}            # cache hit: serve it
    else:                                           # miss: resize, then serve
      - exec: {cmd: resize-image, input: {...}}
      - path: '{options.cache-root}'
```

On a cache hit the file is served directly; on a miss the `else` sequence
resizes, then serves. An `if` coexists only with its own `then`/`else` — it
never sits beside other statement keys, so there is no execution-order
ambiguity.

## Structure

```yaml
if:   <condition>   # required
then: <body>        # required
else: <body>        # optional
```

If the condition is true, `then` runs; otherwise `else` (if present) runs. `if`
always holds the condition and `then` always holds the body, so the two never
collide — there is no `cond:` or `do:` keyword. Chain alternatives by nesting
an `if` in the `else` body:

```yaml
if:   {exists: '{a}'}
then: {...}
else:
  if:   {exists: '{b}'}
  then: {...}
  else:
    if:   {sql: 'CALL Ready({args.id})'}
    then: {...}
    else: {...}
```

## Conditions

A condition is a dict with one of:

| Condition          | True when                                        |
|--------------------|--------------------------------------------------|
| `exists: '<path>'` | the file or directory exists                     |
| `=:  [a, b]`       | `a` equals `b`                                   |
| `!=: [a, b]`       | `a` does not equal `b`                           |
| `<:  [a, b]`       | `a` is less than `b`                             |
| `<=: [a, b]`       | `a` is less than or equal to `b`                 |
| `not: <condition>` | the inner condition is false                     |
| `and: [c, ...]`    | every listed condition is true                   |
| `or:  [c, ...]`    | any listed condition is true                     |
| `sql: <query>`     | the query returns a truthy scalar (**async**)    |
| `cmd: <command>`   | the process exits 0 (**async**)                  |

A condition may also be a **bare scalar** — a ref or literal rather than a dict
— which is a *truthiness test*: true when the value is set and not `false`, `0`,
`''`, or `null`.

```yaml
if:   '{~session.uid}'          # true when logged in (uid set and non-zero)
then: {...}

if:   {not: '{~session.uid}'}   # true when anonymous
then: {...}
```

Use `{~ref}` (not `{ref}`) so a missing value resolves to `null` (falsy) instead
of erroring. A lone `'{ref}'` keeps its native type, so an integer `0` is falsy
and a missing id is `null`. To test specifically for null — distinct from `0` —
compare against it: `{=: ['{~session.uid}', null]}` (which `0` does *not*
satisfy, since the compare is type-aware).

Each comparison operator takes a list of exactly two values; comparison follows
C!'s JSON value compare rules. There is no `>` or `>=` — swap the operands.
`and`/`or` short-circuit (stop at the first false / first true), which matters
when their operands are async `sql`/`cmd` conditions — those run in order.

`{namespace.key}` variables are resolved first ([args.md](args.md),
[sql.md](sql.md)). A lone `'{ref}'` keeps its native type, so `<`/`<=` compare
numbers as numbers (see [Typed values](sql.md#typed-values)). Truthiness
follows the usual rule: set and not false/empty/zero/null.

`sql` and `cmd` are asynchronous — like any query or `exec` they interrupt
synchronous processing and resume when complete; branch evaluation continues on
completion. This is why a DB- or process-driven condition needs no separate
"run a query, store a flag, then test it" plumbing.

`cmd` judges only the exit status (0 → true); its stdout is ignored and stderr
is logged. An `exec` that must *return data* is a body (`exec`), not a
condition.

## Bodies

A body is any statement:

  - a step — `exec`, `query`, ...
  - a terminal handler — `path`, `resource`, `redirect`, ...
  - a nested `if`
  - a list of statements, run in order

A list runs its statements in order; an async statement (an `exec`, a
`query`) passes to the next when it completes.
