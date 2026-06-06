JmpAPI
======
A high performance JSON-over-HTTP(S) API server. JmpAPI builds an API
from YAML config files: URL endpoints map to MariaDB stored procedures,
OAuth2 logins, file serving, websockets, or upstream HTTP proxies.

Features:

  - YAML-driven endpoints — no code required for a typical CRUD API.
  - MariaDB-backed sessions and OAuth2 federated login (Google, GitHub,
    Facebook).
  - Optional password login via a stored procedure.
  - Periodic timeseries with websocket subscription.
  - Auto-generated OpenAPI 3.1 spec.
  - HTTPS with ACME2 (Let's Encrypt) certificate acquisition.

# Documentation

Configuration reference is in [doc/](doc/):

  - [doc/README.md](doc/README.md) — overview and links
  - [doc/configuration.md](doc/configuration.md) — top-level config
  - [doc/endpoints.md](doc/endpoints.md) — endpoint structure
  - [doc/handlers.md](doc/handlers.md) — handler types
  - [doc/args.md](doc/args.md) — argument validation
  - [doc/sql.md](doc/sql.md) — SQL queries and interpolation
  - [doc/access-control.md](doc/access-control.md) — allow/deny rules
  - [doc/auth.md](doc/auth.md) — OAuth2 and password login
  - [doc/timeseries.md](doc/timeseries.md) — periodic data + websockets

Working example configs live in [api/](api/).

# Prerequisites

  - [C!](https://github.com/CauldronDevelopmentLLC/cbang)

On Debian/Ubuntu, install the rest with:

```sh
sudo apt-get update
sudo apt-get install -y scons libmariadb-dev libmariadb-dev-compat \
  mariadb-server python3-pymysql libssl-dev ssl-cert npm build-essential
```

# Build

```sh
export CBANG_HOME=/path/to/cbang
scons
```

# Install

```sh
scons package
sudo dpkg -i jmpapi_*_amd64.deb
```

The package creates the `jmpapi` system user, installs the systemd
units, and drops example configs into `/etc/jmpapi/`:

  - `jmpapi.yaml` — top-level config (copy of `api/jmpapi-example.yaml`).
  - `local.yaml` — local secrets and addresses (copy of
    `api/local-example.yaml`, **root-readable only**).
  - `jmpapi-auth.yaml` — auth API (symlink to the packaged file).

# Create the database

```sh
mysql -u root -p
```

```sql
CREATE DATABASE jmpapi;
CREATE USER 'jmpapi'@'localhost' IDENTIFIED BY '<password>';
GRANT EXECUTE, SELECT, UPDATE, INSERT, DELETE ON jmpapi.* TO
  'jmpapi'@'localhost';
```

Then load the schema and stored procedures:

```sh
./scripts/update_db.py
```

`update_db.py` reads `src/sql/schema.sql` and `src/sql/procedures.sql`
on a fresh DB, or applies `update-*.sql` migrations on an existing one.

# Configure

Edit `/etc/jmpapi/local.yaml` with your DB credentials, listen
addresses, SSL certificate, and any OAuth2 client IDs and secrets:

```yaml
options:
  https-addresses: [0.0.0.0:443]
  db-user: jmpapi
  db-pass: "..."
  google-client-id:     "..."
  google-client-secret: "..."
  google-redirect-base: https://example.com
```

See [doc/configuration.md](doc/configuration.md) for the full options
list and [doc/auth.md](doc/auth.md) for OAuth2 provider setup.

# Run

```sh
sudo systemctl start jmpapi
sudo systemctl enable jmpapi   # start on boot
```

For multiple independent API instances on one host, use the templated
unit:

```sh
sudo systemctl start jmpapi@<instance>
```

Logs are written to `journalctl -u jmpapi`.

# License

GPL-3.0-or-later. See [LICENSE](LICENSE).
