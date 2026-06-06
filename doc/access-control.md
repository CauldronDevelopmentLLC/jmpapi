# Access Control

JmpAPI gates endpoints with `allow:` / `deny:` lists and individual
args with the same keys.

## Endpoint-level

```yaml
/users:
  allow: [$admin]              # only admins
  get:
    sql: CALL UserList()

  /{id}:
    allow: [$admin, $manager]  # admins or managers
    get:
      sql: CALL UserGet({args.id})
```

Each entry in the list is one of:

| Form        | Matches                                                  |
|-------------|----------------------------------------------------------|
| `$name` or `@name` | The session belongs to group `name`.              |
| `*`         | Everyone (use to override an inherited deny).            |
| `name`      | The session's user equals `name`.                        |

`deny:` is checked the same way; if both match, deny wins. The default
is allow-all unless `allow:` is set.

## Built-in groups

These are set automatically by the session handler:

  - `authenticated` — session has a logged-in user.
  - `unauthenticated` — no session user. Anonymous requests get
    `user = "anonymous"`.

Custom groups (e.g. `admin`, `manager`, `moderator`) are populated by
your session SQL. Each row column named `group` adds the user to that
group; or set group booleans directly via `CALL AuthSession(...)`.

Inside SQL, test a group with `{group.<name>}`:

```yaml
sql: >
  CALL TeamUpdate({args.team}, {args.name},
    {group.admin} OR {group.manager})
```

## Inheritance

`allow:` / `deny:` defined on a parent path or on the method block
apply to all children. Re-declare to override.

```yaml
/admin:
  allow: [$admin]      # all of /admin/* requires admin
  /reports:
    get: {sql: ...}
```

## Per-arg access

`allow:` / `deny:` on an arg restrict who can supply or read that
value. In addition to `$group`, args support `=sessionVar` to require
that the arg value equals a session field.

```yaml
/users/{id}:
  args:
    id:
      type: u32
      allow: [$admin, =uid]   # admin can pass any id; user can only pass their own uid
```

When the check fails the request gets 401 Unauthorized.

## Sessions

A session is established by the `session` handler (see
[handlers.md](handlers.md)). Session fields are exposed as
`{session.*}` in SQL and other interpolated strings.

Common fields populated by an OAuth2 login flow:

  - `session.id` — opaque session id (cookie value).
  - `session.user` — username from the login provider.
  - `session.uid` — internal user id from your DB.
  - `session.provider`, `session.provider_id`.
  - `session.name`, `session.avatar`.

Session cookies are HTTP-only and `SameSite=None`. Lifetime is
controlled by the `session-timeout` and `session-lifetime` options.
