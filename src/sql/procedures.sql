DELIMITER //

DROP PROCEDURE IF EXISTS AddUser;
CREATE PROCEDURE AddUser(
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


DROP PROCEDURE IF EXISTS Login;
CREATE PROCEDURE Login(IN _sid VARCHAR(48), IN _provider VARCHAR(16),
  IN _email VARCHAR(128), IN _name VARCHAR(128), IN _avatar VARCHAR(256))
BEGIN
  -- Update user
  UPDATE users SET name = _name, avatar = _avatar, last_used = NOW()
      WHERE provider = _provider AND email = _email;

  IF ROW_COUNT() != 1 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'User not found.';
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


DROP PROCEDURE IF EXISTS Logout;
CREATE PROCEDURE Logout(IN _sid VARCHAR(48))
BEGIN
    UPDATE users u
      INNER JOIN sessions s ON u.id = s.uid SET u.last_used = NOW();
    DELETE FROM sessions WHERE id = _sid;
END //


DROP PROCEDURE IF EXISTS UpdateSession;
CREATE PROCEDURE UpdateSession(IN _sid VARCHAR(48), IN _ts TIMESTAMP)
BEGIN
    UPDATE users u
      INNER JOIN sessions s ON u.id = s.uid SET u.last_used = _ts;
END //


DROP PROCEDURE IF EXISTS CleanSessions;
CREATE PROCEDURE CleanSessions()
BEGIN
    DELETE FROM sessions WHERE last_used + INTERVAL 1 DAY < NOW();
END //


DROP PROCEDURE IF EXISTS GetSession;
CREATE PROCEDURE GetSession(IN _sid VARCHAR(48))
BEGIN
    SET @@session.time_zone = "+00:00";

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

DELIMITER ;
