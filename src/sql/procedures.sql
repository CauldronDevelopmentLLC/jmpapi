-- Delete all stored procedures on this DB
DELETE FROM mysql.proc WHERE db = "jmpapi";


CREATE PROCEDURE Login(IN _sid VARCHAR(48), IN _provider VARCHAR(16),
  IN _provider_id VARCHAR(64), IN _user VARCHAR(256), IN _name VARCHAR(256),
  IN _avatar VARCHAR(256))
BEGIN
  -- Create or update user
  INSERT INTO users (provider, provider_id, user, name, avatar)
    VALUES (_provider, _provider_id, _user, _name, _avatar)
    ON DUPLICATE KEY UPDATE id = LAST_INSERT_ID(id),
      user = _user, name = _name, avatar = _avatar, last_used = NOW();

  -- Save user ID
  SET @uid = LAST_INSERT_ID();

  -- Create or update session
  INSERT INTO sessions (id, uid) VALUES (_sid, @uid)
    ON DUPLICATE KEY UPDATE last_used = NOW();

  -- List user's groups
  SELECT g.name 'group' FROM user_groups ug
    JOIN groups g ON ug.gid = g.id WHERE ug.uid = @uid;
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


CREATE PROCEDURE GetSessions()
BEGIN
    SET @@session.time_zone = "+00:00";

    DELETE FROM sessions WHERE last_used + INTERVAL 1 DAY < NOW();

    SELECT s.id, u.provider, u.provider_id, u.user, u.name, u.avatar,
      DATE_FORMAT(s.created, '%Y-%m-%dT%TZ') created,
      DATE_FORMAT(s.last_used, '%Y-%m-%dT%TZ') last_used
      FROM sessions s JOIN users u ON s.uid = u.id;
END;
