ALTER DATABASE jmpapi charset=utf8;


CREATE TABLE IF NOT EXISTS config (
  name      VARCHAR(16) PRIMARY KEY,
  value     INT NOT NULL,
  type      VARCHAR(8) DEFAULT 'number',
  help      VARCHAR(255),
  writable  BOOLEAN DEFAULT TRUE
);


INSERT INTO config (name, value, type, help, writable) VALUES
  ('version', 0, 'number', 'Current database version', FALSE),
  ('auto-register', TRUE, 'bool', 'Allow automatic user registration', TRUE),
  ('session-timeout', 2592000, 'number',
   'Seconds since last session use or zero for no limit', TRUE),
  ('session-lifetime', 0, 'number',
   'Max session lifetime in seconds, zero for no limit', TRUE)
  ON DUPLICATE KEY UPDATE name = name;


CREATE TABLE IF NOT EXISTS providers (
  name VARCHAR(16) PRIMARY KEY
);


INSERT INTO providers
  VALUES ('google'), ('github'), ('facebook'), ('twitter')
  ON DUPLICATE KEY UPDATE name = name;



CREATE TABLE IF NOT EXISTS groups (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  name         VARCHAR(64) NOT NULL UNIQUE
);


INSERT INTO groups (name) VALUES ('admin')
  ON DUPLICATE KEY UPDATE name = name;


CREATE TABLE IF NOT EXISTS users (
  id           INT AUTO_INCREMENT PRIMARY KEY,
  email        VARCHAR(128) NOT NULL,
  name         VARCHAR(128),
  avatar       VARCHAR(256),
  enabled      BOOLEAN DEFAULT TRUE,
  created      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_used    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

  UNIQUE KEY (email)
);


CREATE TABLE IF NOT EXISTS associations (
  provider     VARCHAR(16) NOT NULL,
  id           VARCHAR(128),
  email        VARCHAR(128) NOT NULL,
  name         VARCHAR(128),
  avatar       VARCHAR(256),
  uid          INT,
  created      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_used    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

  UNIQUE KEY (uid, provider),
  UNIQUE KEY (id, provider),
  KEY (email),
  FOREIGN KEY (provider) REFERENCES providers(name) ON DELETE CASCADE,
  FOREIGN KEY (uid) REFERENCES users(id) ON DELETE CASCADE
);


CREATE TABLE IF NOT EXISTS sessions (
  id           VARCHAR(48) NOT NULL PRIMARY KEY,
  uid          INT,
  created      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_used    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

  FOREIGN KEY (uid) REFERENCES users(id) ON DELETE CASCADE
);


CREATE TABLE IF NOT EXISTS user_groups (
  uid          INT NOT NULL,
  gid          INT NOT NULL,

  FOREIGN KEY (uid) REFERENCES users(id) ON DELETE CASCADE,
  FOREIGN KEY (gid) REFERENCES groups(id) ON DELETE CASCADE,
  PRIMARY KEY (uid, gid)
);
