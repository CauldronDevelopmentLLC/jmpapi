JmpAPI
===========
A high performance JSON over HTTP API server with MariaDB backend and pug.js
frontend.

# Prerequisites
  - [C!](https://github.com/CauldronDevelopmentLLC/cbang)
  - [libre2](https://code.google.com/p/re2/)
  - [mariadb](https://mariadb.org/)
  - [node.js](https://nodejs.org/)

In Debian Linux, after installing C!, you can install the prerequsites as
follows:

    curl -sL https://deb.nodesource.com/setup_8.x | sudo -E bash -
    sudo apt-get update
    sudo apt-get install -y libre2-dev libmariadbclient-dev mariadb-server \
      python3-mysql.connector ssl-cert nodejs build-essential

# Build

    export CBANG_HOME=/path/to/cbang
    scons

# Create the DB and user

    mysql -u root -p
    CREATE DATABASE jmpapi;
    CREATE USER 'jmpapi'@'localhost' IDENTIFIED BY '<password>';
    GRANT EXECUTE, SELECT, UPDATE, INSERT, DELETE ON jmpapi.* TO
      'jmpapi'@'localhost';
    exit
    ./scripts/update_db.py
