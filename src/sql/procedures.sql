-- Remove all stored procedures & functions
DELETE FROM mysql.proc WHERE db = DATABASE();

DELIMITER //


-- Functions
CREATE FUNCTION EmailIsValid(addr VARCHAR(100))
  RETURNS BOOLEAN
  DETERMINISTIC
BEGIN
  SET @chars = 'a-zA-Z0-9!#$%&*+/=?^_`{|}~-';
  SET @pattern =
    CONCAT('^[', @chars, '][.', @chars, ']*@[.', @chars, ']*[', @chars, ']$');

  RETURN NOT addr IS NULL AND addr REGEXP @pattern;
END //


CREATE FUNCTION ConfigGetFunc(_name VARCHAR(16), _default INT)
  RETURNS INT
  READS SQL DATA
BEGIN
  SET @value = _default;
  SELECT value INTO @value FROM config WHERE name = _name;
  RETURN @value;
END //


CREATE PROCEDURE ConfigGet(IN _name VARCHAR(16))
BEGIN
  SELECT value FROM config WHERE name = _name;
END //


CREATE PROCEDURE ConfigSet(IN _name VARCHAR(16), IN _value INT)
BEGIN
  SELECT COUNT(*) INTO @count FROM config WHERE name = _name;
  IF @count = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'Config variable not found.';
  END IF;

  UPDATE config SET value = _value WHERE name = _name;
END //


CREATE PROCEDURE ConfigList()
BEGIN
  SELECT name, value, type, help, writable FROM config;
END //


-- Auth
CREATE PROCEDURE AuthLogin(
  IN _sid         VARCHAR(48),
  IN _provider    VARCHAR(16),
  IN _provider_id VARCHAR(128),
  IN _email       VARCHAR(128),
  IN _name        VARCHAR(128),
  IN _avatar      VARCHAR(256))
BEGIN
  SET @@session.time_zone = '+00:00';

  IF ISNULL(_name) OR LENGTH(_name) = 0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'User name cannot be empty';
  END IF;

  START TRANSACTION;

  -- Lookup association by ID or email if ID is NULL
  SET @uid = NULL;
  SELECT uid, email, avatar INTO @uid, @email, @avatar FROM associations
    WHERE provider = _provider AND
      (id = _provider_id OR (ISNULL(id) AND email = _email));

  IF ISNULL(@uid) THEN
    IF ConfigGetFunc('auto-register', FALSE) THEN
      CALL UserAdd(_provider, _provider_id, _email, _name, _avatar, TRUE);

      -- Get new user ID
      SELECT uid INTO @uid FROM associations
        WHERE provider = _provider AND id = _provider_id;

    ELSE
      SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'User not found.';
    END IF;

  ELSE
    -- Check if user is enabled
    SET @enabled = FALSE;
    SELECT enabled INTO @enabled FROM users WHERE id = @uid;
    IF NOT @enabled THEN
      SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'User disabled';
    END IF;

    -- Update association
    UPDATE associations
      SET id = _provider_id, email = _email, name = _name, avatar = _avatar
      WHERE provider = _provider AND
        (id = _provider_id OR (ISNULL(id) AND email = _email));

    -- Update user
    -- NOTE, avatar updated if NULL or if equal to old association avatar
    UPDATE users SET
      name = IFNULL(name, _name),
      avatar = IF(avatar = @avatar, _avatar, IFNULL(avatar, _avatar)),
      last_used = NOW()
      WHERE id = @uid;
  END IF;

  -- Create or update session
  INSERT INTO sessions (id, uid) VALUES (_sid, @uid)
    ON DUPLICATE KEY UPDATE last_used = NOW();

  COMMIT;

  -- List session variables
  SELECT 'uid' name, @uid value UNION ALL
    SELECT 'lifetime', ConfigGetFunc('session-lifetime', 0) UNION ALL
    SELECT 'timeout',  ConfigGetFunc('session-timeout',  0);

  -- List user's groups
  SELECT g.name 'group' FROM user_groups ug
    JOIN groups g ON ug.gid = g.id AND ug.uid = @uid;
END //


CREATE PROCEDURE AuthLogout(IN _sid VARCHAR(48))
BEGIN
  SET @@session.time_zone = '+00:00';

  UPDATE users u
    INNER JOIN sessions s ON u.id = s.uid SET u.last_used = NOW();
  DELETE FROM sessions WHERE id = _sid;
END //


-- Sessions
CREATE PROCEDURE AuthSession(IN _sid VARCHAR(48))
BEGIN
  SET @@session.time_zone = '+00:00';

  -- Delete old sessions
  SET @timeout  = ConfigGetFunc('session-timeout',  0);
  SET @lifetime = ConfigGetFunc('session-lifetime', 0);
  DELETE FROM sessions
    WHERE (@timeout AND last_used + INTERVAL @timeout SECOND < NOW()) OR
      (@lifetime AND created + INTERVAL @lifetime SECOND < NOW());

  -- Get session, if it exists
  SELECT u.id uid, u.email user, u.name, u.avatar,
    DATE_FORMAT(s.created, '%Y-%m-%dT%TZ') created,
    DATE_FORMAT(s.last_used, '%Y-%m-%dT%TZ') last_used,
    @timeout timeout, @lifetime lifetime
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
  SELECT id, name, avatar, email, enabled, created, last_used FROM users;
END //


CREATE PROCEDURE UserAssociationAdd(
  IN _uid         INT,
  IN _provider    VARCHAR(16),
  IN _provider_id VARCHAR(128),
  IN _email       VARCHAR(256),
  IN _name        VARCHAR(128),
  IN _avatar      VARCHAR(256))
BEGIN
  SET @@session.time_zone = '+00:00';

  IF NOT EmailIsValid(_email) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Invalid email';
  END IF;

  INSERT INTO associations (provider, id, email, name, avatar, uid)
    VALUES (_provider, _provider_id, _email, _name, _avatar, _uid)
    ON DUPLICATE KEY UPDATE
      email  = _email,
      name   = _name,
      avatar = IFNULL(_avatar, avatar),
      uid    = _uid;
END //


CREATE PROCEDURE UserAssociationDelete(
  IN _uid INT,
  IN _provider VARCHAR(16))
BEGIN
  DELETE FROM associations WHERE uid = _uid AND provider = _provider;

  IF ROW_COUNT() = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'Association not found.';
  END IF;
END //


CREATE PROCEDURE UserAdd(
  IN _provider    VARCHAR(16),
  IN _provider_id VARCHAR(128),
  IN _email       VARCHAR(256),
  IN _name        VARCHAR(128),
  IN _avatar      VARCHAR(256),
  IN _enabled     BOOLEAN)
BEGIN
  SET @@session.time_zone = '+00:00';

  IF NOT EmailIsValid(_email) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Invalid email';
  END IF;

  -- Create user
  INSERT INTO users (email, name, avatar, enabled) VALUES
    (_email, _name, _avatar, _enabled);
  SET @uid = LAST_INSERT_ID();

  -- Create association
  CALL
    UserAssociationAdd(@uid, _provider, _provider_id, _email, _name, _avatar);

  -- Automatically make the first user an admin
  IF @uid = 1 THEN
    INSERT INTO user_groups SELECT 1, id FROM groups WHERE name = 'admin'
      ON DUPLICATE KEY UPDATE uid = 1;
  END IF;
END //


CREATE PROCEDURE UserDelete(IN _id INT)
BEGIN
  DELETE FROM users WHERE id = _id;

  IF ROW_COUNT() = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'User not found.';
  END IF;
END //


CREATE PROCEDURE UserGet(IN _id INT)
BEGIN
  -- Return user settings
  SELECT email, name, avatar, enabled, created, last_used FROM users
    WHERE id = _id;

  -- List user's associations
  SELECT provider, id, email, name, avatar, created, last_used FROM associations
    WHERE uid = _id;

  -- List user's groups
  SELECT g.name 'group' FROM user_groups ug
    JOIN groups g ON ug.gid = g.id AND ug.uid = _id;
END //


CREATE PROCEDURE UserSet(
  IN _id      INT,
  IN _email   VARCHAR(128),
  IN _name    VARCHAR(128),
  IN _avatar  VARCHAR(256),
  IN _enabled BOOLEAN)
BEGIN
  IF NOT ISNULL(_email) AND NOT EmailIsValid(_email) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Invalid email';
  END IF;

  UPDATE users SET
      email   = IFNULL(_email,   email),
      name    = IFNULL(_name,    name),
      avatar  = IFNULL(_avatar,  avatar),
      enabled = IFNULL(_enabled, enabled)
    WHERE id = _id;

  SELECT COUNT(*) INTO @count FROM users WHERE id = _id;
  IF @count = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'User not found.';
  END IF;
END //


CREATE PROCEDURE UserEnable(IN _id INT, IN _enabled BOOLEAN)
BEGIN
  UPDATE users SET enabled = _enabled WHERE id = _id;
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

  IF ROW_COUNT() = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'User not found in group.';
  END IF;
END //


-- Groups
CREATE PROCEDURE GroupList()
BEGIN
  SELECT name FROM groups;
END //


CREATE PROCEDURE GroupAdd(IN _group VARCHAR(64))
BEGIN
  IF ISNULL(_group) OR LENGTH(_group) = 0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Group name cannot be empty';
  END IF;

  INSERT INTO groups (name) VALUES (_group);
END //


CREATE PROCEDURE GroupDelete(IN _group VARCHAR(64))
BEGIN
  DELETE FROM groups WHERE name = _group AND name != 'admin';

  IF ROW_COUNT() = 0 THEN
    SIGNAL SQLSTATE '02000' SET MESSAGE_TEXT = 'Group not found.';
  END IF;
END //


CREATE PROCEDURE GroupMemberList(IN _group VARCHAR(64))
BEGIN
  SELECT uid id, u.name, avatar, created, last_used
    FROM user_groups ug
    JOIN groups g ON g.id = ug.gid AND g.name = _group
    JOIN users u ON u.id = ug.uid;
END //


CREATE PROCEDURE GroupNonmemberList(IN _group VARCHAR(64))
BEGIN
  SELECT u.id, u.name, u.avatar, u.created, u.last_used FROM users u
    JOIN groups g ON g.name = _group
    LEFT JOIN user_groups ug ON ug.uid = u.id AND ug.gid = g.id
    WHERE ug.uid IS NULL;
END //

DELIMITER ;
