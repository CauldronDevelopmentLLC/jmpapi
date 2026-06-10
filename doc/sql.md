# SQL & Variables

A `sql:` key on a method makes it a SQL query handler. Variable references
in the string are bound as statement parameters and the query is run
against MariaDB as a prepared statement.

```yaml
/user/{id}:
  args: {id: {type: u32}}
  get:
    sql: CALL UserGet({args.id})
    return: dict
```

The server expects MariaDB stored procedures for non-trivial queries,
but any SQL works.

## Variables

Each `{path.to.value}` in the SQL becomes a bound `?` parameter — the value
is never spliced into the SQL text, so quoting, escaping and SQL injection
are structurally impossible, and binary data is safe at any size. Available
roots:

| Root        | Source                                                  |
|-------------|---------------------------------------------------------|
| `args.*`    | Validated request args. See [args.md](args.md).         |
| `session.*` | Logged-in session fields (`id`, `user`, `uid`, `provider`, `name`, `avatar`, …). |
| `group.*`   | Booleans for each group on the session (e.g. `{group.admin}`). |
| `options.*` | Server options. See [configuration.md](configuration.md). |
| `msg.*`     | Incoming websocket message (in websocket handlers).     |
| `body`, `files.*` | Binary request data. See [binary.md](binary.md).  |
| *`<name>`*  | A result captured by an earlier statement's `into:`. See [Capturing results](#capturing-results). |

A missing ref is a request-time error. Mark a ref optional with `~`:
`{~x.y}` binds SQL `NULL` (elsewhere JSON `null`) when missing. This is
how optional args reach the database:

```yaml
args: {alt_text: {optional: true, max: 512}}
sql:  CALL AssetSave({args.id}, {~args.alt_text})
```

Strings, numbers and booleans bind with their native values (booleans as
`1`/`0`); binary values ([binary.md](binary.md)) bind their bytes. A
`{ref:fmt}` format spec formats the value first and binds the resulting
string (e.g. `{args.pi:d}` binds `'3'`). Because
a parameter is a whole value, a ref cannot sit inside a string literal —
`LIKE '%{args.q}%'` is an error. Assemble such strings in SQL instead:

```sql
... WHERE name LIKE CONCAT('%', {args.q}, '%')
```

Trusted SQL *fragments* (table names, etc.) can come only from
`{options.*}` refs, which resolve into the config at server start, before
any request.

## Typed values

Outside SQL — in JSON value positions such as an [exec](exec.md) `input`
template, a [condition](conditions.md) operand, or a response — a *lone*
reference (a quoted string that is exactly one `{ref}` with no `:fmt`)
resolves to the value's native JSON type. An embedded reference, or one
with a `:fmt` format spec, stays a string.

```yaml
size: '{args.size}'      # → 800       (number)
on:   '{args.enabled}'   # → true      (boolean)
who:  '{session}'        # → { ... }   (object)
text: 'hi {args.name}'   # → "hi Bob"  (embedded → string)
```

The quotes are needed only because YAML reads a leading `{` as a mapping; the
resolver sees the post-parse string, so a lone `'{ref}'` is written exactly like
any other interpolation today.

## Return types

Set `return:` on the method.

| `return` | Response shape                                           |
|----------|----------------------------------------------------------|
| `ok`     | Empty body (default when nothing else implies a shape).  |
| `one`    | A single scalar from row 1, column 1.                    |
| `bool`   | Boolean from row 1, column 1.                            |
| `u64`    | Unsigned 64-bit int from row 1, column 1.                |
| `s64`    | Signed 64-bit int from row 1, column 1.                  |
| `dict`   | One row as a JSON object. 404 if no row.                 |
| `list`   | All rows as a list (of values if 1 column, else objects). |
| `hlist`  | List with a header row of column names first.            |
| `fields` | Multi-result-set merge into a dict. See below.           |
| `binary` | First column of row 1 as the raw response body. See [binary.md](binary.md). |
| `pass`   | Run the query, discard the results, and continue to the next statement. |

## `fields`

When a procedure returns multiple result sets (e.g. user + teams),
`fields` names each set:

```yaml
get:
  fields: ["*", teams, projects]
  sql: CALL UserGet({args.id})
```

  - `"*"` — merge the result set into the top-level dict.
  - `"*name"` — insert as a nested dict under `name`.
  - `"name"` — insert as a list under `name`.

`return:` defaults to `fields` whenever `fields` is set.

## Capturing results

By default a query statement replies and ends the request. `into: <name>`
stores the result instead — shaped by `return:` as usual — and continues to
the next statement in the sequence. The captured result is available to
later statements as the interpolation root `{<name>}`:

```yaml
/user/{id}/notify:
  args: {id: {type: u32}}
  put:
    - sql: "CALL UserGet({args.id})"
      return: dict
      into: user
    - exec:
        cmd:   notify-user
        input: {user: '{user}', email: '{user.email}'}
    - reply: '{user}'
```

A lone `'{<name>}'` passes the whole result with its native type; dotted
paths select into it. A captured `return: binary` value is binary — usable
anywhere `{body}` is (bound into SQL, passed to an [exec](exec.md) as a
file, or replied with [`reply:`](handlers.md#reply)).

Query errors are unaffected by `into:` — they reply immediately and the
rest of the chain never runs. There is no carried status: the statement
that finally replies chooses the response code.

Use `return: pass` instead of `into:` when the result doesn't matter
(e.g. an audit log call).

## Reusable queries

Define named queries at the top level (or in a sub-API) and reference
them by name:

```yaml
queries:
  user_get:
    sql: CALL UserGet({args.id})
    return: dict

endpoints:
  /user/{id}:
    args: {id: {type: u32}}
    get:  {query: user_get}
```

## Errors

The handler maps common MariaDB errors to HTTP status codes:

| MariaDB                                   | HTTP                       |
|-------------------------------------------|----------------------------|
| `SIGNAL NOT FOUND`, `FILE NOT FOUND`      | 404 Not Found              |
| `ER_DUP_ENTRY`                            | 409 Conflict               |
| `SIGNAL EXCEPTION`                        | 400 Bad Request            |
| `ER_ACCESS_DENIED`, `ER_DBACCESS_DENIED`  | 401 Unauthorized           |
| other                                     | 500 Internal Server Error  |

Use `SIGNAL SQLSTATE '45000'` from a stored procedure to return a
custom 400.
