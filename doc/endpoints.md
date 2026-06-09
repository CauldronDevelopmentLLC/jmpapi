# Endpoints

`endpoints:` is a tree of URL paths. Keys starting with `/` are child
paths; keys naming HTTP methods are method handlers at the current path.

```yaml
endpoints:
  /users:
    get:  {sql: CALL UserList(), return: list}
    post: {sql: CALL UserAdd({args.name})}

    /{id}:
      args: {id: {type: u32}}
      get:    {sql: CALL UserGet({args.id}), return: dict}
      delete: {sql: CALL UserDelete({args.id})}
```

## Path syntax

  - `/literal` — exact match.
  - `/{name}` — capture into `args.name`.
  - `/regex.*` — paths are regexes (RE2). Capture groups with `{name}`
    become args.

## Methods

Lower-case method names: `get`, `put`, `post`, `delete`, `options`,
`head`, `patch`. Combine with `|` to share a handler:

```yaml
/.*:
  get|put|post|delete:
    handler: session
```

Use `any` for all methods.

## Endpoint keys

Apply on any method or path block. Child keys (paths and methods)
inherit from parent keys for `args` and `allow`/`deny`.

| Key          | Purpose                                                |
|--------------|--------------------------------------------------------|
| `handler`    | Handler name. See [handlers.md](handlers.md).          |
| `handlers`   | List of handlers run in order (e.g. CORS + query).     |
| `sql`        | SQL query. Implies `handler: query`.                   |
| `query`      | Name of a query defined in `queries:`. See [sql.md](sql.md). |
| `args`       | Arg definitions. See [args.md](args.md).               |
| `allow` / `deny` | Access rules. See [access-control.md](access-control.md). |
| `headers`    | Response headers, as a map.                            |
| `return`     | Query return shape. See [sql.md](sql.md).              |
| `fields`     | Field layout when `return: fields`. See [sql.md](sql.md). |
| `body`       | Raw binary request body. See [binary.md](binary.md).   |
| `files`      | Multipart file-part declarations. See [binary.md](binary.md). |
| `content-type` | Response `Content-Type` for `return: binary`. See [binary.md](binary.md). |
| `help`       | Description used in the OpenAPI spec.                  |
| `hide`       | If true, omit from the OpenAPI spec.                   |
| `exec` | Run an external program as a step. See [exec.md](exec.md). |
| `if` / `then` / `else` | Conditional execution. See [conditions.md](conditions.md). |
| `path`       | File path. Implies `handler: file`.                    |
| `index`      | Index filename for `handler: file` directories.        |
| `resource`   | Compiled-in resource name. Implies `handler: resource`. |
| `bind`       | Name of a C++-bound callback. Implies `handler: bind`. |
| `url`        | Upstream URL. Implies `handler: proxy`.                |

## Inheritance

`args` and `allow`/`deny` blocks defined on a parent apply to all
children:

```yaml
/users:
  allow: [$admin]            # applies to all child endpoints
  /{id}:
    args: {id: {type: u32}}  # applies to all methods under /{id}
    get:    {sql: ...}
    delete: {sql: ...}
```

## Multiple handlers

Use `handlers:` to chain handlers (e.g. set headers then run a query):

```yaml
/data:
  get:
    handlers:
      - {handler: cors, patterns: ["https://example.com"]}
      - {sql: CALL DataList(), return: list}
```
