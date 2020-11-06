JmpAPI
===========
A high performance JSON over HTTP API server with MariaDB built-in federated
login.

# Prerequisites
  - [C!](https://github.com/CauldronDevelopmentLLC/cbang)

In Debian Linux, after installing C!, you can install the packaged prerequsites
as follows:

    sudo apt-get update
    sudo apt-get install -y libre2-dev libmariadbclient-dev mariadb-server \
      python3-pymysql ssl-cert nodejs build-essential

# Build

    export CBANG_HOME=/path/to/cbang
    scons

# Install

    scons package
    sudo dpkg -i jmpapi_0.2.4_amd64.deb

Next edit or install the API configuration files in ``/etc/jmpapi/``.

# Create the DB and user

    mysql -u root -p
    CREATE DATABASE jmpapi;
    CREATE USER 'jmpapi'@'localhost' IDENTIFIED BY '<password>';
    GRANT EXECUTE, SELECT, UPDATE, INSERT, DELETE ON jmpapi.* TO
      'jmpapi'@'localhost';
    exit
    ./scripts/update_db.py


# Start the API server

    sudo service jmpapi start

# OAuth2 Setup

JmpAPI can use OAuth2 logins provided other sites such as Google, Facebook or
GitHub.  OAuth2 configuration requires three configuration options per provider:

  1. ``<provider>-client-secret``.  The OAuth2 client secret.
  2. ``<provider>-client-id``.  The OAuth2 client ID.
  3. ``<provider>-redirect-base``.  The base URL login redirects.

You must obtain the OAuth2 client secret and id from the provider in a provider
specfic way.  Most providers require that you also configure the allowed
redirect URL.  This is the URL they will redirect logins to get user back to
your site.

The OAuth2 secret must be protected so make sure that the JmpAPI configuration
file which contains the secrets is readable only by ``root``.

## GitHub

To obtain OAuth2 credentials from GitHub.  Login in and either go to your
personal settings or organization settings.  Then under ``Developer Settings``
select ``OAuth Apps`` and click ``New OAuth App``.  Set the client ID and
secret from this page as ``github-client-id`` and ``github-client-secret``.

## Facebook
TODO

## Google
TODO
