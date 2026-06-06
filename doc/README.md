# JmpAPI Configuration

JmpAPI builds a JSON-over-HTTP API from one or more YAML files. Each file
declares URL endpoints that map HTTP methods to SQL queries, login flows,
file serving, websockets, or HTTP proxies.

A minimal config:

```yaml
jmpapi: 1.1.0

info:
  title: Example
  version: 1.0.0

endpoints:
  /hello:
    get:
      sql: SELECT 'world' AS greeting
      return: dict
```

Run with `jmpapi config.yaml`.

## Reference

  - [configuration.md](configuration.md) — top-level keys, includes, options
  - [endpoints.md](endpoints.md) — paths, methods, nesting, common keys
  - [handlers.md](handlers.md) — handler types and their config
  - [args.md](args.md) — argument validation, types, inheritance
  - [sql.md](sql.md) — SQL queries, interpolation, return types, fields
  - [access-control.md](access-control.md) — `allow`/`deny`, groups, sessions
  - [auth.md](auth.md) — OAuth2 setup and password-based login
  - [timeseries.md](timeseries.md) — periodic SQL data and websocket streams

## Conventions

  - `{name}` in a URL is a path parameter, available as `{args.name}`.
  - `{x.y}` inside a string interpolates from the resolver
    (see [sql.md](sql.md)).
  - Keys starting with `/` are nested child paths.
  - HTTP methods are lower-case (`get`, `put`, `post`, `delete`); combine
    with `|` (e.g. `get|put`); use `any` to match all methods.

## Example configs

See `api/jmpapi-example.yaml`, `api/jmpapi-auth.yaml`, and
`api/jmpapi-files.yaml` in this repo for working examples.
