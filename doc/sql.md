# SQL & Interpolation

A `sql:` key on a method makes it a SQL query handler. The string is
run against MariaDB after variables are interpolated.

```yaml
/user/{id}:
  args: {id: {type: u32}}
  get:
    sql: CALL UserGet({args.id})
    return: dict
```

The server expects MariaDB stored procedures for non-trivial queries,
but any SQL works.

## Interpolation

`{path.to.value}` is replaced before the SQL is sent. Available roots:

| Root        | Source                                                  |
|-------------|---------------------------------------------------------|
| `args.*`    | Validated request args. See [args.md](args.md).         |
| `session.*` | Logged-in session fields (`id`, `user`, `uid`, `provider`, `name`, `avatar`, …). |
| `group.*`   | Booleans for each group on the session (e.g. `{group.admin}`). |
| `options.*` | Server options. See [configuration.md](configuration.md). |
| `msg.*`     | Incoming websocket message (in websocket handlers).     |

Modifiers:

  - `{~x.y}` — return JSON `null` if missing instead of erroring.
  - `{x.y:S}` — force SQL-string quoting (default for strings).
  - `{x.y:fmt}` — pass `fmt` to the value's formatter.

Inside SQL, strings are auto-quoted with MariaDB escaping; numbers and
booleans are inserted raw; missing values become `NULL`.

## Typed values

Inside SQL, values are always formatted into the statement (above). Elsewhere —
in JSON value positions such as a [subprocess](subprocess.md) `input` template,
a [condition](conditions.md) operand, or a response — a *lone* reference (a
quoted string that is exactly one `{ref}` with no `:fmt`) resolves to the
value's native JSON type. An embedded reference, or one with a `:fmt`, stays a
string.

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
