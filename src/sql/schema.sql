ALTER DATABASE jmpapi charset=utf8;


CREATE TABLE IF NOT EXISTS jmpapi_config (
  name VARCHAR(16) PRIMARY KEY,
  value VARCHAR(255)
);


CREATE TABLE IF NOT EXISTS jmpapi_providers (
  name VARCHAR(16) PRIMARY KEY
);


INSERT INTO jmpapi_providers
  VALUES ('google'), ('github'), ('facebook'), ('twitter')
  ON DUPLICATE KEY UPDATE name = name;



CREATE TABLE IF NOT EXISTS jmpapi_groups (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  name         VARCHAR(64) NOT NULL UNIQUE
);


INSERT INTO jmpapi_groups (name) VALUES ('admin')
  ON DUPLICATE KEY UPDATE name = name;


CREATE TABLE IF NOT EXISTS jmpapi_users (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  provider     VARCHAR(16) NOT NULL,
  provider_id  VARCHAR(64) NOT NULL,
  email        VARCHAR(256) NOT NULL,
  name         VARCHAR(256) NOT NULL,
  avatar       VARCHAR(256) NOT NULL,
  created      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_used    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

  UNIQUE KEY (provider, provider_id),
  FOREIGN KEY (provider) REFERENCES jmpapi_providers(name) ON DELETE CASCADE
);


CREATE TABLE IF NOT EXISTS jmpapi_sessions (
  id           VARCHAR(48) NOT NULL PRIMARY KEY,
  uid          INT,
  created      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_used    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

  FOREIGN KEY (uid) REFERENCES jmpapi_users(id) ON DELETE CASCADE
);


CREATE TABLE IF NOT EXISTS jmpapi_user_groups (
  uid          INT NOT NULL,
  gid          INT NOT NULL,

  FOREIGN KEY (uid) REFERENCES jmpapi_users(id) ON DELETE CASCADE,
  FOREIGN KEY (gid) REFERENCES jmpapi_groups(id) ON DELETE CASCADE,
  PRIMARY KEY (uid, gid)
);
