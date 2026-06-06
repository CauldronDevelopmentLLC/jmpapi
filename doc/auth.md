# Authentication

JmpAPI authenticates users via OAuth2 (Google, Facebook, GitHub) or via
a stored-procedure-driven password login. Both paths populate the same
session, so the rest of the API treats them identically.

A request goes through three steps:

  1. The `session` handler attaches (or creates) a session cookie. See
     [handlers.md](handlers.md).
  2. A login endpoint (OAuth2 or password) authenticates the user and
     fills session variables and group memberships.
  3. Subsequent requests carry the session cookie; `{session.*}` and
     `{group.*}` are available in SQL. See [access-control.md](access-control.md).

## OAuth2

### Configuration

For each provider, set three options in the top-level `options:` block:

```yaml
options:
  google-client-id:     "..."
  google-client-secret: "..."
  google-redirect-base: https://example.com
```

  - `<provider>-client-id` — public client ID from the provider.
  - `<provider>-client-secret` — secret. **Must** be in a file readable
    only by `root` (typically `/etc/jmpapi/local.yaml`).
  - `<provider>-redirect-base` — base URL the provider redirects back
    to. Append the login path to get the full redirect URI that you
    register with the provider.

A provider is only enabled when all three options are set. `GET
/login/providers` lists configured providers.

### Login endpoint

Wire one endpoint to handle all providers:

```yaml
/login/{provider}:
  args:
    provider:
      enum: [google, facebook, github, providers, '']
    redirect_uri: {type: uri, optional: true}
  get:
    handler: login
    sql: >
      CALL AuthLogin({session.id}, {session.provider},
        {session.provider_id}, {session.user}, {session.name},
        {session.avatar})
```

  - Empty `{provider}` returns the current session (or 401).
  - `providers` returns the list of configured providers.
  - A real provider name starts the OAuth2 dance and, on success, runs
    the `sql:` to record the login and populate the session. See
    [Session SQL output](#session-sql-output).

`redirect_uri` (optional query arg) is where the client wants to land
after login.

### Registering with each provider

#### Google

  1. Go to <https://console.cloud.google.com/apis/credentials>.
  2. Create OAuth client ID → Web application.
  3. Set the authorized redirect URI to
     `<google-redirect-base>/login/google`.
  4. Copy the client ID and secret into `google-client-id` and
     `google-client-secret`.

#### GitHub

  1. From your personal or organization settings, go to **Developer
     Settings → OAuth Apps → New OAuth App**.
  2. Set the authorization callback URL to
     `<github-redirect-base>/login/github`.
  3. Copy the client ID and secret into `github-client-id` and
     `github-client-secret`.

#### Facebook

  1. Go to <https://developers.facebook.com/apps> and create an app.
  2. Add the **Facebook Login** product.
  3. Under Facebook Login → Settings, add
     `<facebook-redirect-base>/login/facebook` to **Valid OAuth
     Redirect URIs**.
  4. Copy the App ID and App Secret into `facebook-client-id` and
     `facebook-client-secret`.

## Password login

JmpAPI does not hash or verify passwords itself. The login handler
forwards the plaintext password to a SQL stored procedure that does
the verification.

### Endpoint

```yaml
/login:
  get:
    handler: login
    provider: none          # skip OAuth2; run the SQL directly
    args:
      email:    {type: email}
      password: {min: 8, max: 64}
    sql: CALL AuthPasswordLogin({session.id}, {args.email}, {args.password})
```

  - `provider: none` disables OAuth2 — `sql:` is called immediately
    with the validated args.
  - Always serve `/login` over HTTPS. The password is sent in clear
    text to the server.

### Session SQL output

The login `sql:` is expected to return multiple result sets:

  1. **Session variables** — a result set of `(name, value)` rows. Each
     row sets `session.<name>` to that value (numbers preserved,
     everything else as string). Typical names: `uid`, `user`, `name`,
     `avatar`, `provider`, `provider_id`.
  2. **Groups** — one or more additional result sets whose first
     column is a group name. Each row adds the user to that group.

On any error (bad credentials, disabled account), raise a SQL signal —
e.g. `SIGNAL SQLSTATE '45000'` for 400, or rely on `ER_ACCESS_DENIED`
for 401. See [sql.md](sql.md) for the full error mapping.

### Storing passwords

Store a per-user random salt and a hash in the `users` table. A
reasonable schema:

```sql
ALTER TABLE users
  ADD COLUMN password_hash CHAR(64),     -- hex SHA-256
  ADD COLUMN password_salt CHAR(32);     -- hex random
```

Use a strong CSPRNG to fill `password_salt` at registration. Then
hash:

```sql
SET hash = SHA2(CONCAT(salt, plaintext), 256);
```

SHA-256 with a per-user salt is fast — fine when paired with rate
limiting on `/login` and a strong password policy. For higher-value
applications, do a slow hash (bcrypt/argon2) in a small external
service called from your procedure, or in an app-side wrapper before
the password reaches MariaDB.

### Example procedure

```sql
CREATE PROCEDURE AuthPasswordLogin(
  IN sid VARCHAR(48), IN _email VARCHAR(128), IN _password VARCHAR(64))
BEGIN
  DECLARE _uid INT;
  DECLARE _name VARCHAR(128);
  DECLARE _avatar VARCHAR(256);

  SELECT u.id, u.name, u.avatar INTO _uid, _name, _avatar
    FROM users u
    WHERE u.email = _email
      AND u.enabled
      AND u.password_hash = SHA2(CONCAT(u.password_salt, _password), 256);

  IF _uid IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Invalid login';
  END IF;

  -- Bind session to user
  REPLACE INTO sessions (id, uid) VALUES (sid, _uid);

  -- Result 1: session variables
  SELECT 'uid'    AS name, _uid    AS value
  UNION ALL SELECT 'user',   _email
  UNION ALL SELECT 'name',   _name
  UNION ALL SELECT 'avatar', _avatar;

  -- Result 2+: groups
  SELECT g.name FROM groups g
    JOIN user_groups ug ON ug.gid = g.id
    WHERE ug.uid = _uid;
END
```

### Registration and password change

Registration and "change password" endpoints are normal `query`
endpoints — nothing special from JmpAPI's side. They should:

  1. Generate a fresh random salt server-side (in the SQL procedure or
     before).
  2. Store `SHA2(CONCAT(salt, plaintext), 256)` along with the salt.
  3. Never log the plaintext password.

The `auto-register` row in the `config` table (created by
`scripts/update_db.py`) is a convenient feature flag for whether
self-registration is open.

## Logout

```yaml
/logout:
  put:
    handler: logout
    sql: CALL AuthLogout({session.id})
```

Both OAuth2 and password sessions are closed the same way.

## Mixing providers per user

The default schema's `users` + `associations` tables let one user link
multiple OAuth2 providers, but the `users` table holds the password
hash. A user can therefore log in via any linked OAuth2 provider
**or** with their email + password — the login handler doesn't care
which path filled the session.
