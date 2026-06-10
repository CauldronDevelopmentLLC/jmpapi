# Handlers

A handler decides what a method endpoint does. Set explicitly with
`handler: <name>`, or imply one by including a triggering key.

| Handler      | Triggered by   | Purpose                                  |
|--------------|----------------|------------------------------------------|
| `query`      | `sql:` or `query:` | Run a SQL query. See [sql.md](sql.md). |
| `timeseries` | `timeseries:` or under `timeseries:` block | Read or stream periodic SQL data. See [timeseries.md](timeseries.md). |
| `session`    | explicit       | Load or create the session for the request. |
| `login`      | explicit       | OAuth2 login flow.                       |
| `logout`     | explicit       | Close the current session.               |
| `pass`       | default        | Do nothing; let the next handler / route match. |
| `file`       | `path:`        | Serve a file or directory.               |
| `resource`   | `resource:`    | Serve a compiled-in resource.            |
| `redirect`   | explicit       | HTTP redirect.                           |
| `status`     | explicit       | Return a fixed status code + body.       |
| `reply`      | `reply:`       | Reply with a resolved value (JSON or binary). |
| `cors`       | explicit       | Apply CORS headers; reply to OPTIONS.    |
| `spec`       | explicit       | Serve the generated OpenAPI spec.        |
| `websocket`  | explicit       | Upgrade to websocket; route messages.    |
| `proxy`      | `url:`         | Forward to an upstream HTTP service.     |
| `bind`       | `bind:`        | Call a C++ callback registered with `bind`. |

## query

See [sql.md](sql.md).

## session

Loads the session from a cookie (or creates one). Usually attached at
the root of an auth API so every request has a session:

```yaml
/.*:
  get|put|post|delete:
    handler: session
    sql: CALL AuthSession({session.id})
```

The `sql:` lookup is optional — if set, it populates session fields
from the DB. A session with a `user` column becomes "authenticated".

## login

OAuth2 login. Specify the provider as a path arg or via `provider:`:

```yaml
/login/{provider}:
  args:
    provider: {enum: [google, facebook, github, providers, '']}
  get:
    handler: login
    sql: >
      CALL AuthLogin({session.id}, {session.provider},
        {session.provider_id}, {session.user}, {session.name},
        {session.avatar})
```

  - `provider: providers` — list configured providers.
  - empty provider — return the current session.
  - `redirect:` — URL to redirect to on success (otherwise reply JSON).
  - `redirect_uri` arg — page to return the user to.

`provider: none` enables password login via `email`/`password` args.

## logout

```yaml
/logout:
  put:
    handler: logout
    sql: CALL AuthLogout({session.id})
```

## file

Serve static files:

```yaml
/favicon.ico:
  get:
    path: '{options.favicon}'

/.*:
  get:
    path: '{options.http-root}'
    index: index.html
```

## cors

```yaml
/.*:
  any:
    handler: cors
    credentials: true
    patterns:
      - "https://[^.]+\\.example\\.com"
      - "https?://localhost(:\\d+)?"
```

Keys: `origins` (literal list), `patterns` (regex list), `methods`,
`headers`, `credentials`, `max-age`.

## redirect / status

```yaml
/old:
  get: {handler: redirect, location: /new, code: 301}

/down:
  get: {handler: status, code: 503, text: Maintenance}
```

## reply

Replies with a value resolved against the request — typically a result
captured with [`into:`](sql.md#capturing-results) or a file returned by an
[exec](exec.md). A binary value is sent as the raw response body with its
`Content-Type`; anything else as JSON.

```yaml
- reply: '{files.out}'                        # binary
- reply: {user: '{user}', teams: '{teams}'}   # shaped JSON
- {reply: '{user}', code: 201}                # explicit status code
```

The `reply:` value is always the response body; a sibling `code:` sets the
status (default `200`). Statuses are never carried between statements — the
statement that replies chooses the code.

## spec

```yaml
/openapi-spec:
  get: {handler: spec}
```

Returns the auto-generated OpenAPI 3.1 document built from all
endpoints and args.

## websocket

```yaml
/websocket:
  get:
    handler: websocket
    on-message:
      - map: msg.type
        args:
          timeseries: {enum: [team.score, user.score]}
          action:     {source: type}
          since:      {type: time, optional: true}
        to:
          subscribe:   {timeseries: '{args.timeseries}'}
          unsubscribe: {timeseries: '{args.timeseries}'}
```

`on-message` is a list of sub-handlers. Each one is either `map`
(dispatch on a field), or a direct `query`/`timeseries` handler. The
incoming message is bound to `{msg.*}`.

## proxy

```yaml
/upstream/{path}:
  get:
    url: https://api.example.com/v1/{args.path}
    method: GET
    request-headers: {Authorization: 'Bearer {options.api-key}'}
```

The proxy can also template the response — see the `proxy` source
files for `with`, `each`, `if`, `and`, `or`, `not`, `equal`, `literal`,
`dict`, `on`, `debug`, `status`.

## exec

Not a handler but a statement: runs an external program as a step in the
pipeline, exchanging a JSON metadata envelope. It can modify args, set the
response, or short-circuit. See [exec.md](exec.md). To run one
conditionally, see [conditions.md](conditions.md).
