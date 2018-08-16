-- Delete all stored procedures on this DB
DELETE FROM mysql.proc WHERE db = "jmpapi";


CREATE PROCEDURE Login(IN _sid VARCHAR(48), IN _provider VARCHAR(16),
  IN _provider_id VARCHAR(64), IN _email VARCHAR(256), IN _name VARCHAR(256),
  IN _avatar VARCHAR(256))
BEGIN
  -- Create or update user
  INSERT INTO users (provider, provider_id, email, name, avatar)
    VALUES (_provider, _provider_id, _email, _name, _avatar)
    ON DUPLICATE KEY UPDATE id = LAST_INSERT_ID(id),
      email = _email, name = _name, avatar = _avatar, last_used = NOW();

  -- Save user ID
  SET @uid = LAST_INSERT_ID();

  -- Create or update session
  INSERT INTO sessions (id, uid) VALUES (_sid, @uid)
    ON DUPLICATE KEY UPDATE last_used = NOW();

  -- List user's groups
  SELECT g.name 'group' FROM user_groups ug
    JOIN groups g ON ug.gid = g.id AND ug.uid = @uid;
END;


CREATE PROCEDURE Logout(IN _sid VARCHAR(48))
BEGIN
    UPDATE users u
      INNER JOIN sessions s ON u.id = s.uid SET u.last_used = NOW();
    DELETE FROM sessions WHERE id = _sid;
END;


CREATE PROCEDURE UpdateSession(IN _sid VARCHAR(48), IN _ts TIMESTAMP)
BEGIN
    UPDATE users u INNER JOIN sessions s ON u.id = s.uid SET u.last_used = _ts;
END;


CREATE PROCEDURE CleanSessions()
BEGIN
    DELETE FROM sessions WHERE last_used + INTERVAL 1 DAY < NOW();
END;


CREATE PROCEDURE GetSession(IN _sid VARCHAR(48))
BEGIN
    SET @@session.time_zone = "+00:00";

    -- Get session, if it exists
    SELECT u.provider, u.provider_id, u.email user, u.name, u.avatar,
      DATE_FORMAT(s.created, '%Y-%m-%dT%TZ') created,
      DATE_FORMAT(s.last_used, '%Y-%m-%dT%TZ') last_used
      FROM sessions s JOIN users u ON s.id = _sid AND s.uid = u.id;

    -- Update session last_used
    UPDATE sessions SET last_used = NOW() WHERE id = _sid;

    -- List user's groups
    SELECT g.name 'group' FROM groups g
      JOIN user_groups ug ON ug.gid = g.id
      JOIN users u ON u.id = ug.uid
      JOIN sessions s ON s.id = _sid AND s.uid = u.id;
END;
