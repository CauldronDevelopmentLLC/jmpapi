-- Remove all stored procedures & functions
DELETE FROM mysql.proc WHERE db = DATABASE();

DELIMITER //


-- Auth
CREATE PROCEDURE AuthLogin(IN _sid VARCHAR(48), IN _provider VARCHAR(16),
  IN _email VARCHAR(128), IN _name VARCHAR(128), IN _avatar VARCHAR(256))
BEGIN
  IF ISNULL(_name) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'User name cannot be NULL';
  END IF;

  -- Update user
  UPDATE users SET name = _name, avatar = _avatar, last_used = NOW()
      WHERE provider = _provider AND email = _email;

  IF ROW_COUNT() != 1 THEN
    SET @msg = CONCAT('User "', _name, '" not found.');
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = @msg;
  END IF;

  -- Save user ID
  SELECT id INTO @uid FROM users WHERE provider = _provider AND email = _email;

  -- Automatically make the first user an admin
  INSERT INTO user_groups SELECT @uid, id FROM groups
    WHERE name = 'admin' AND @uid = 1
    ON DUPLICATE KEY UPDATE uid = 1;

  -- Create or update session
  INSERT INTO sessions (id, uid) VALUES (_sid, @uid)
    ON DUPLICATE KEY UPDATE last_used = NOW();

  -- List user's groups
  SELECT g.name 'group' FROM user_groups ug
    JOIN groups g ON ug.gid = g.id AND ug.uid = @uid;
END //


CREATE PROCEDURE AuthLogout(IN _sid VARCHAR(48))
BEGIN
    UPDATE users u
      INNER JOIN sessions s ON u.id = s.uid SET u.last_used = NOW();
    DELETE FROM sessions WHERE id = _sid;
END //


-- Sessions
CREATE PROCEDURE AuthSession(IN _sid VARCHAR(48))
BEGIN
    SET @@session.time_zone = "+00:00";

    -- Delete old sessions
    DELETE FROM sessions WHERE last_used + INTERVAL 1 DAY < NOW();

    -- Get session, if it exists
    SELECT u.provider, u.email user, u.name, u.avatar,
      DATE_FORMAT(s.created, '%Y-%m-%dT%TZ') created,
      DATE_FORMAT(s.last_used, '%Y-%m-%dT%TZ') last_used
      FROM sessions s
      JOIN users u ON s.id = _sid AND s.uid = u.id;

    -- Update session last_used
    UPDATE sessions SET last_used = NOW() WHERE id = _sid;

    -- List user's groups
    SELECT g.name 'group' FROM groups g
      JOIN user_groups ug ON ug.gid = g.id
      JOIN users u ON u.id = ug.uid
      JOIN sessions s ON s.id = _sid AND s.uid = u.id;
END //


-- Users
CREATE PROCEDURE UserList()
BEGIN
  SELECT id, provider, name, avatar, email, created, last_used FROM users;
END //


CREATE PROCEDURE UserAdd(
  IN _provider VARCHAR(16),
  IN _email VARCHAR(256),
  IN _name VARCHAR(128))
BEGIN
  -- Insert user
  INSERT INTO users (provider, email, name, last_used)
    VALUES (_provider, _email, _name, 0);

  -- Save user ID
  SET @uid = LAST_INSERT_ID();

  -- Automatically make the first user an admin
  INSERT INTO user_groups SELECT @uid, id FROM groups
    WHERE name = 'admin' AND @uid = 1
    ON DUPLICATE KEY UPDATE uid = 1;

  SELECT @uid id;
END //


CREATE PROCEDURE UserDelete(IN _id INT)
BEGIN
  DELETE FROM users WHERE id = _id;
END //


-- User Groups
CREATE PROCEDURE UserGroupList(IN _id INT)
BEGIN
  SELECT g.name, COUNT(ug.uid) member FROM jmpapi.groups g
    LEFT JOIN user_groups ug ON ug.gid = g.id AND ug.uid = _id
    GROUP BY g.name;
END //


CREATE PROCEDURE UserGroupAdd(IN _id INT, IN _group VARCHAR(64))
BEGIN
  INSERT INTO user_groups (uid, gid)
    SELECT _id, g.id FROM groups g WHERE g.name = _group;
END //


CREATE PROCEDURE UserGroupDelete(IN _id INT, IN _group VARCHAR(64))
BEGIN
  DELETE FROM user_groups WHERE uid = _id AND gid IN
    (SELECT id FROM groups WHERE name = _group);
END //


-- Groups
CREATE PROCEDURE GroupList()
BEGIN
  SELECT name FROM groups;
END //


CREATE PROCEDURE GroupAdd(IN _group VARCHAR(64))
BEGIN
  INSERT INTO groups (name) VALUES (_group);
END //


CREATE PROCEDURE GroupDelete(IN _group VARCHAR(64))
BEGIN
  DELETE FROM groups WHERE name = _group AND name != 'admin';
END //


CREATE PROCEDURE GroupMemberList(IN _group VARCHAR(64))
BEGIN
  SELECT uid id, provider, u.name, avatar, created, last_used
    FROM user_groups ug
    JOIN groups g ON g.id = ug.gid AND g.name = _group
    JOIN users u ON u.id = ug.uid;
END //


CREATE PROCEDURE GroupNonmemberList(IN _group VARCHAR(64))
BEGIN
  SELECT u.id, u.provider, u.name, u.avatar, u.created, u.last_used FROM users u
    JOIN groups g ON g.name = _group
    LEFT JOIN user_groups ug ON ug.uid = u.id AND ug.gid = g.id
    WHERE ug.uid IS NULL;
END //

DELIMITER ;
