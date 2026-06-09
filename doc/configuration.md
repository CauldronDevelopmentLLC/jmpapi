# Configuration

A JmpAPI config is one or more YAML files merged at startup. Pass them as
positional args:

```sh
jmpapi main.yaml secrets.yaml
```

Later files override earlier files. Use this to keep secrets in a
root-only file separate from the public API definition.

## Top-level keys

| Key      | Purpose                                                       |
|----------|---------------------------------------------------------------|
| `jmpapi` | Required. Minimum config version. Must be `>= 1.2.0`.         |
| `info`   | OpenAPI `info` block (`title`, `version`, `description`, …).  |
| `apis`   | Map of named sub-APIs, usually included from other files.     |
| `options`| Runtime options (addresses, DB, OAuth2, paths, …).            |
| `endpoints` | Endpoints when not using `apis`. See [endpoints.md](endpoints.md). |
| `args`   | Shared arg definitions. See [args.md](args.md).               |
| `queries`| Named reusable SQL queries. See [sql.md](sql.md).             |
| `timeseries` | Named timeseries. See [timeseries.md](timeseries.md).     |

## Sub-APIs and `!include`

Split a config across files with the YAML `!include` directive:

```yaml
apis:
  files: !include jmpapi-files.yaml
  auth:  !include jmpapi-auth.yaml
```

Each sub-API has its own namespace for `args`, `queries`, and
`timeseries`. Reference another namespace by prefixing with its name:

```yaml
args:
  page: {inherit: global.timeseries}
```

A sub-API file usually has its own `title`, `help`, and `endpoints`:

```yaml
title: Auth API
help: User and session management.
endpoints:
  /login: {get: {handler: login}}
```

Set `hide: true` on a sub-API or endpoint to keep it out of the
generated OpenAPI spec.

## Options

Set via the top-level `options:` block. Common ones:

```yaml
options:
  http-addresses:  [0.0.0.0:80]
  https-addresses: [0.0.0.0:443]
  certificate-file: /etc/ssl/certs/site.pem
  private-key-file: /etc/ssl/private/site.key

  db-user: jmpapi
  db-pass: "..."
  db-name: jmpapi

  google-client-id:      "..."
  google-client-secret:  "..."
  google-redirect-base:  https://example.com

  http-root: /usr/share/jmpapi/http
  timeseries-db: /var/lib/jmpapi/timeseries
  session-timeout:  3600
  session-lifetime: 2592000
```

Options are referenced from configs as `{options.<name>}` — see
[sql.md](sql.md).

Run `jmpapi --help` for the full list.
