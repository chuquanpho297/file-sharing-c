DROP DATABASE IF EXISTS file_sharing;
CREATE DATABASE file_sharing;
USE file_sharing;

CREATE TABLE `Users` (
    `userName` VARCHAR(255) NOT NULL,
    `passwordHash` VARCHAR(255) NOT NULL,
    PRIMARY KEY (`userName`)
);

ENUM('private', 'view', 'download',)

-- CREATE TABLE `FolderRoot` (
--     `folderID` VARCHAR(255) NOT NULL,
--     `folderName` VARCHAR(255) NOT NULL,
--     `createBy` VARCHAR(255) NOT NULL,
--     `createAt` TIMESTAMP NOT NULL,
--     PRIMARY KEY (`folderID`),
--     CONSTRAINT `createBy` FOREIGN KEY (`createBy`) REFERENCES `Users`(`userName`)
-- );

CREATE TABLE `Folder` (
	`folderID` VARCHAR(255) NOT NULL,
    `folderName` VARCHAR(255) NOT NULL,
    `parentFolderID` VARCHAR(255),
    `createBy` VARCHAR(255) NOT NULL,
    `createAt` TIMESTAMP NOT NULL,
    `access` ENUM('private', 'view', 'download') DEFAULT 'private',
    PRIMARY KEY (`folderID`),
    CONSTRAINT `createBy` FOREIGN KEY (`createBy`) REFERENCES `Users`(`userName`),
    CONSTRAINT `parentFolderID` FOREIGN KEY (`parentFolderID`) REFERENCES `Folder`(`folderID`)
);

CREATE TABLE `File` (
	`fileID` VARCHAR(255) NOT NULL,
    `folderID` VARCHAR(255) NOT NULL,
    `fName` VARCHAR(255) NOT NULL,
    `fileSize` BIGINT NOT NULL,
    `access` ENUM('private', 'view', 'download') DEFAULT 'private',
    PRIMARY KEY (`fileID`),
    CONSTRAINT `folderID` FOREIGN KEY (`folderID`) REFERENCES `Folder`(`folderID`)
);

-- CREATE TABLE `MemberOfGroup` (
--     `folderID` VARCHAR(255) NOT NULL,
--     `userName` VARCHAR (255) NOT NULL,
--     `mRole` ENUM('admin', 'member') NOT NULL,
--     PRIMARY KEY (`folderID`, `userName`),
--     FOREIGN KEY (`userName`) REFERENCES `Users`(`userName`),
--     FOREIGN KEY (`folderID`) REFERENCES `Folder`(`folderID`)
-- );

-- CREATE TABLE `JoinGroup` (
--     `userName` VARCHAR(255) NOT NULL,
--     `requestType` ENUM('join','invite') NOT NULL,
--     `status` ENUM('pending','accepted','denied') NOT NULL,
--     `createAt` TIMESTAMP NOT NULL,
--     `folderID` VARCHAR(255) NOT NULL,
--     PRIMARY KEY (`folderID`, `userName`),
--     FOREIGN KEY (`userName`) REFERENCES `Users`(`userName`),
--     FOREIGN KEY (`folderID`) REFERENCES `Folder`(`folderID`)
-- );

DELIMITER //

CREATE FUNCTION HashPasswordWithUserName(userName VARCHAR(255), userPassword VARCHAR(255))
RETURNS VARBINARY(64) DETERMINISTIC
BEGIN
    RETURN UNHEX(SHA2(CONCAT(userPassword, HEX(SHA2(userName, 256))), 256));
END;
//
CREATE FUNCTION InsertNewUser(pUserName VARCHAR(255), userPassword VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE userExists INT;

    -- Check if the username already exists
    SELECT COUNT(*) INTO userExists
    FROM Users
    WHERE userName = pUserName;

    IF userExists > 0 THEN
        -- Username already exists, return NULL to indicate failure
        RETURN FALSE;
    ELSE
        -- Username doesn't exist, proceed to insert the new user
        INSERT INTO Users (userName, passwordHash)
        VALUES (pUserName, HEX(HashPasswordWithUserName(pUserName, userPassword)));

        -- Return TRUE to indicate successful insertion
        RETURN TRUE;
    END IF;
END //
CREATE FUNCTION Login(userName VARCHAR(255), userPassword VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE foundUserName VARCHAR(255);

    -- Check if the username and hashed password match
    SELECT userName INTO foundUserName
    FROM Users
    WHERE userName = userName AND passwordHash = HEX(HashPasswordWithUserName(userName, userPassword))
    LIMIT 1;

    IF foundUserName IS NOT NULL THEN
        RETURN TRUE; -- Valid credentials, return TRUE
    ELSE
        RETURN FALSE; -- Invalid credentials, return FALSE
    END IF;
END //

-- CREATE FUNCTION CreateNewGroup(username VARCHAR(255), groupNameInput VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE creatorName VARCHAR(255);
--     DECLARE groupExists VARCHAR(255);

--     -- Retrieve the user ID based on the username
--     SELECT userName INTO creatorName
--     FROM Users
--     WHERE userName = username
--     LIMIT 1;

--     -- If no user found, return FALSE
--     IF creatorName IS NULL THEN
--         RETURN FALSE;
--     END IF;

--     -- Check if the group name already exists
--     SELECT groupName INTO groupExists
--     FROM `Groups`
--     WHERE groupName = groupNameInput;

--     -- If the group name already exists, return NULL
--     IF groupExists IS NOT NULL THEN
--         RETURN FALSE;
--     END IF;

--     -- Create a new group
--     INSERT INTO `Groups` (groupName, createBy, createAt)
--     VALUES (groupNameInput, creatorName, NOW());

--     -- Add the creator to the MemberOfGroup table as an admin
--     INSERT INTO MemberOfGroup (groupName, userName, mRole)
--     VALUES (groupNameInput, username, 'admin');

--     RETURN TRUE;
-- END //

-- CREATE FUNCTION CheckIsMember(user_name VARCHAR(255), folder_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isMember INT;

--     -- Check if the user is a member of the specified group
--     SELECT COUNT(*) INTO isMember
--     FROM MemberOfGroup
--     WHERE userName = user_name AND groupName = group_name;

--     IF isMember > 0 THEN
--         RETURN TRUE; -- User is a member of the group
--     ELSE
--         RETURN FALSE; -- User is not a member of the group
--     END IF;
-- END //

-- CREATE FUNCTION RequestToJoinGroup(user_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isMem INT;
--     DECLARE isValidUser INT;
--     DECLARE isValidGroup INT;
--     DECLARE inJoinGroup INT;

--     -- Check if the username exists in the Users table
--     SELECT COUNT(*)
--     INTO isValidUser
--     FROM Users
--     WHERE userName = user_name;

--     -- Check if the group exists in the Groups table
--     SELECT COUNT(*)
--     INTO isValidGroup
--     FROM `Groups`
--     WHERE groupName = group_name;

-- 	SELECT COUNT(*)
--     INTO inJoinGroup
--     FROM JoinGroup
--     WHERE userName = user_name AND groupName = group_name and status = 'pending';
    
--     IF isValidUser > 0 AND isValidGroup > 0 AND inJoinGroup = 0 THEN
--         -- User exists in Users table and group exists in Groups table
--         -- Check if the user is already a member of the group using CheckIsMember function
--         SET isMem = CheckIsMember(user_name, group_name);

--         IF isMem > 0 THEN
--             -- User is already a member, return FALSE indicating no need for a join request
--             RETURN FALSE;
--         ELSE
--             -- User is not a member, proceed to create a join request
--             INSERT INTO JoinGroup (userName, requestType, status, createAt, groupName)
--             VALUES (user_name, 'join', 'pending', NOW(), group_name);

--             RETURN TRUE; -- Return TRUE for successful request initiation
--         END IF;
--     ELSE
--         -- Invalid username or group name, return FALSE indicating invalid entry
--         RETURN FALSE;
--     END IF;
-- END //

-- CREATE FUNCTION InviteToGroup(invitedUsername VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isValidInvited INT;
--     DECLARE isValidGroup INT;
--     DECLARE isUserInGroup INT;
-- 	DECLARE inJoinGroup INT;
--     -- Check if the invited user exists in the Users table
--     SELECT COUNT(*)
--     INTO isValidInvited
--     FROM Users
--     WHERE userName = invitedUsername;

--     -- Check if the group exists in the Groups table
--     SELECT COUNT(*)
--     INTO isValidGroup
--     FROM `Groups`
--     WHERE groupName = group_name;

--     -- Check if the invited user is already in the JoinGroup table for the specified group
--     SELECT COUNT(*)
--     INTO inJoinGroup
--     FROM JoinGroup
--     WHERE userName = invitedUsername AND groupName = group_name and status = 'pending';
-- 	 -- Check if the invited user is already a member of the group
--     SET isUserInGroup = CheckIsMember(invitedUsername, group_name);
    
--     IF isValidInvited > 0 AND isValidGroup > 0 AND isUserInGroup = 0 AND inJoinGroup = 0 THEN
--         -- Invited user exists, group exists, and user is not already in the JoinGroup for the specified group
--         -- Proceed to create an invitation
--         INSERT INTO JoinGroup (userName, requestType, status, createAt, groupName)
--         VALUES (invitedUsername, 'invite', 'pending', NOW(), group_name);

--         -- Additional logic for further processing can be added here

--         RETURN TRUE; -- Return TRUE for successful invitation initiation
--     ELSE
--         -- Invalid invited user, group name, or user already in the JoinGroup, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //
-- CREATE FUNCTION CheckIsAdmin(user_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE is_admin INT;

--     -- Check if the user is an admin in the group
--     SELECT COUNT(*)
--     INTO is_admin
--     FROM MemberOfGroup
--     WHERE userName = user_name AND groupName = group_name AND mRole = 'admin';

--     IF is_admin > 0 THEN
--         RETURN TRUE; -- User is an admin in the group
--     ELSE
--         RETURN FALSE; -- User is not an admin in the group
--     END IF;
-- END;
-- CREATE FUNCTION AcceptUsertoGroup(user_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isAdmin BOOLEAN;
--     DECLARE isPendingMember INT;
--         -- Check if the user is already in JoinGroup with 'pending' status
--         SELECT COUNT(*)
--         INTO isPendingMember
--         FROM JoinGroup
--         WHERE userName = user_name AND groupName = group_name AND status = 'pending';

--         IF isPendingMember > 0 THEN
--             -- User is not in JoinGroup with 'pending' status, proceed with normal acceptance process
--             INSERT INTO MemberOfGroup (groupName, userName, mRole)
--             VALUES (group_name, user_name, 'member');

--             UPDATE JoinGroup
--             SET status = 'accepted'
--             WHERE userName = user_name AND groupName = group_name;
--             RETURN TRUE; -- Return TRUE for successful acceptance
-- 		ELSE
--             RETURN FALSE;
--         END IF;
-- END //

-- CREATE FUNCTION DenyUsertoGroup(user_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isPendingMember INT;

--     -- Check if the user is already in JoinGroup with 'pending' status
--     SELECT COUNT(*)
--     INTO isPendingMember
--     FROM JoinGroup
--     WHERE userName = user_name AND groupName = group_name AND status = 'pending';

--     IF isPendingMember > 0 THEN
--         -- User is in JoinGroup with 'pending' status, deny their request
--         UPDATE JoinGroup
--         SET status = 'denied'
--         WHERE userName = user_name AND groupName = group_name;

--         RETURN TRUE; -- Return TRUE for successful denial
--     ELSE
--         -- User is not in JoinGroup with 'pending' status, or the group does not exist, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

-- CREATE FUNCTION LeaveGroup(user_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE isMember INT;

--     -- Check if the user is a member of the specified group
--     SELECT COUNT(*)
--     INTO isMember
--     FROM MemberOfGroup
--     WHERE userName = user_name AND groupName = group_name;

--     IF isMember > 0 THEN
--         -- User is a member of the group, proceed to remove them from the group
--         DELETE FROM MemberOfGroup
--         WHERE userName = user_name AND groupName = group_name;

--         -- Additional logic (if needed) can be added here
        
--         RETURN TRUE; -- Return TRUE for successful leaving
--     ELSE
--         -- User is not a member of the group, or the group does not exist, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

CREATE FUNCTION CreateFolder(folder_name VARCHAR(255), parent_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE folder_id VARCHAR(255);
    DECLARE folder_exists INT;

    -- Check if the folder name already exists in the specified group
    SELECT COUNT(*)
    INTO folder_exists
    FROM Folder
    WHERE folderName = folder_name AND parentFolderID = parent_folder_id;

    IF folder_exists > 0 THEN
        -- Folder with the same name already exists in the group, return FALSE
        RETURN FALSE;
    ELSE
        -- Generate a unique folder ID
        SET folder_id = UUID();

        -- Insert the new folder into the Folder table
        INSERT INTO Folder (folderID, folderName, parentFolderID, createBy, createAt)
        VALUES (folder_id, folder_name, parent_folder_id, user_name, NOW());

        RETURN TRUE; -- Return TRUE for successful folder creation
    END IF;
END //

-- CREATE FUNCTION CopyFolder(fromgroup_name VARCHAR(255), folder_name VARCHAR(255), toGroup_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE from_folder_id VARCHAR(255);
--     DECLARE to_folder_id VARCHAR(255);
--     DECLARE to_group_exists INT;

--     SELECT folderID INTO from_folder_id
--     FROM Folder
--     WHERE folderName = folder_name AND groupName = fromgroup_name;

--     IF from_folder_id IS NULL THEN
--         RETURN FALSE;
--     END IF;

--     SELECT COUNT(*) INTO to_group_exists
--     FROM `Groups`
--     WHERE groupName = toGroup_name;

--     IF to_group_exists = 0 THEN
--         RETURN FALSE;
--     END IF;

--     SELECT folderID INTO to_folder_id
--     FROM Folder
--     WHERE folderName = folder_name AND groupName = toGroup_name;

--     IF to_folder_id IS NOT NULL THEN
--         RETURN FALSE;
--     ELSE
--         SET to_folder_id = UUID();
--         INSERT INTO Folder (folderID, folderName, groupName, createAt)
--         VALUES (to_folder_id, folder_name, toGroup_name, NOW());

--         INSERT INTO File (fileID, folderID, fName, fileSize, folderName, groupName)
--         SELECT UUID(), to_folder_id, fName, fileSize, folderName, toGroup_name
--         FROM File
--         WHERE folderID = from_folder_id;

--         RETURN TRUE;
--     END IF;
-- END //

-- CREATE FUNCTION RemoveFolder(folder_name VARCHAR(255), group_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE folder_id VARCHAR(255);
--     DECLARE folder_exists INT;

--     -- Get the folder ID
--     SELECT folderID INTO folder_id
--     FROM Folder
--     WHERE folderName = folder_name AND groupName = group_name;

--     -- Check if the folder exists
--     SELECT COUNT(*) INTO folder_exists
--     FROM Folder
--     WHERE folderName = folder_name AND groupName = group_name;

--     IF folder_exists > 0 THEN
--         -- Delete files in the folder
--         DELETE FROM File
--         WHERE folderID = folder_id;

--         -- Remove the folder
--         DELETE FROM Folder
--         WHERE folderID = folder_id;

--         RETURN TRUE; -- Return TRUE for successful folder removal
--     ELSE
--         -- Folder does not exist, or the folder does not belong to the specified group, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

-- CREATE FUNCTION RenameFolder(group_name VARCHAR(255), folder_name VARCHAR(255), new_folder_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE folder_exists INT;

--     -- Check if the folder exists within the specified group
--     SELECT COUNT(*)
--     INTO folder_exists
--     FROM Folder
--     WHERE folderName = folder_name AND groupName = group_name;

--     IF folder_exists > 0 THEN
--         -- Folder exists in the specified group, proceed to rename it
--         UPDATE Folder
--         SET folderName = new_folder_name
--         WHERE folderName = folder_name AND groupName = group_name;

--         -- Update corresponding folder name in the File table
--         UPDATE `File`
--         SET folderName = new_folder_name
--         WHERE folderName = folder_name AND groupName = group_name;

--         RETURN TRUE; -- Return TRUE for successful folder rename
--     ELSE
--         -- Folder does not exist in the specified group, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

-- CREATE FUNCTION CreateFile(file_name VARCHAR(255), file_size BIGINT, group_name VARCHAR(255), folder_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE file_id VARCHAR(255);
--     DECLARE folder_id VARCHAR(255);
--     DECLARE group_exists INT;
--     DECLARE folder_exists INT;
--     DECLARE file_exists INT;
    
--     -- Check if the group exists
--     SELECT COUNT(*)
--     INTO group_exists
--     FROM `Groups`
--     WHERE groupName = group_name;

--     IF group_exists > 0 THEN
--         -- Group exists, check if the folder exists within the group
--         SELECT folderID
--         INTO folder_id
--         FROM Folder
--         WHERE folderName = folder_name AND groupName = group_name;

--         IF folder_id IS NOT NULL THEN
--             -- Check if the file already exists in the folder
--             SELECT COUNT(*)
--             INTO file_exists
--             FROM File
--             WHERE fName = file_name AND folderName = folder_name AND groupName = group_name;

--             IF file_exists > 0 THEN
--                 -- File already exists in the folder within the group, return FALSE
--                 RETURN FALSE;
--             ELSE
--                 -- Generate a fileID using UUID()
--                 SET file_id = UUID();

--                 -- Insert the file into the File table
--                 INSERT INTO `File` (fileID, fName, fileSize, folderID, folderName, groupName)
--                 VALUES (file_id, file_name, file_size, folder_id, folder_name, group_name);

--                 RETURN TRUE; -- Return TRUE for successful file creation
--             END IF;
--         ELSE
--             -- Folder does not exist within the group, return FALSE
--             RETURN FALSE;
--         END IF;
--     ELSE
--         -- Group does not exist, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //



-- CREATE FUNCTION RemoveFile(file_name VARCHAR(255), group_name VARCHAR(255), folder_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE file_exists INT;

--     -- Check if the file exists in the specified folder within the group
--     SELECT COUNT(*)
--     INTO file_exists
--     FROM `File`
--     WHERE fName = file_name AND groupName = group_name AND folderName = folder_name;

--     IF file_exists > 0 THEN
--         -- File exists, proceed to remove it
--         DELETE FROM `File`
--         WHERE fName = file_name AND groupName = group_name AND folderName = folder_name;

--         RETURN TRUE; -- Return TRUE for successful file removal
--     ELSE
--         -- File does not exist, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

-- CREATE FUNCTION RenameFile(file_name VARCHAR(255), group_name VARCHAR(255), folder_name VARCHAR(255), new_file_name VARCHAR(255))
-- RETURNS BOOLEAN DETERMINISTIC
-- BEGIN
--     DECLARE file_exists INT;

--     -- Check if the file exists in the specified folder within the group
--     SELECT COUNT(*)
--     INTO file_exists
--     FROM `File`
--     WHERE fName = file_name AND groupName = group_name AND folderName = folder_name;

--     IF file_exists > 0 THEN
--         -- File exists, and the new file name does not exist; proceed to rename it
--         UPDATE `File`
--         SET fName = new_file_name
--         WHERE fName = file_name AND groupName = group_name AND folderName = folder_name;

--         RETURN TRUE; -- Return TRUE for successful file renaming
--     ELSE
--         -- File does not exist or new file name already exists, return FALSE
--         RETURN FALSE;
--     END IF;
-- END //

CREATE FUNCTION GetFolderID(folder_name VARCHAR(255), user_name VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    SELECT folderID INTO folder_id
    FROM Folder
    WHERE folderName = folder_name AND createBy = user_name;
    RETURN folder_id;
END //

CREATE FUNCTION GetParentFolderID(folder_name VARCHAR(255), user_name VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    SELECT parentFolderID INTO parent_folder_id
    FROM Folder
    WHERE folderName = folder_name AND createBy = user_name;
    RETURN parent_folder_id;
END //


CREATE FUNCTION CopyFolder(from_folder_id VARCHAR(255), to_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE new_folder_id VARCHAR(255);
    DECLARE done INT DEFAULT FALSE;
    DECLARE cur_file_name VARCHAR(255);
    DECLARE cur_file_size BIGINT;
    
    -- Copy files from source folder to destination folder
    DECLARE file_cursor CURSOR FOR 
        SELECT fName, fileSize 
        FROM File 
        WHERE folderID = from_folder_id;
    
    DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
    
    -- Start transaction
    START TRANSACTION;
    
    -- Copy files
    OPEN file_cursor;
    read_loop: LOOP
        FETCH file_cursor INTO cur_file_name, cur_file_size;
        IF done THEN
            LEAVE read_loop;
        END IF;
        
        -- Create new file entry
        INSERT INTO File (fileID, folderID, fName, fileSize)
        VALUES (UUID(), to_folder_id, cur_file_name, cur_file_size);
    END LOOP;
    CLOSE file_cursor;
    
    -- Copy subfolders recursively
    SET done = FALSE;
    BEGIN
        DECLARE subfolder_cursor CURSOR FOR
            SELECT folderID, folderName
            FROM Folder
            WHERE parentFolderID = from_folder_id;
            
        DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
        
        OPEN subfolder_cursor;
        subfolder_loop: LOOP
            FETCH subfolder_cursor INTO new_folder_id, cur_file_name;
            IF done THEN
                LEAVE subfolder_loop;
            END IF;
            
            -- Create new subfolder
            SET new_folder_id = UUID();
            INSERT INTO Folder (folderID, folderName, parentFolderID, createBy, createAt)
            SELECT new_folder_id, folderName, to_folder_id, createBy, NOW()
            FROM Folder
            WHERE folderID = from_folder_id;
            
            -- Recursively copy contents of subfolder
            IF NOT CopyFolder(from_folder_id, new_folder_id) THEN
                ROLLBACK;
                RETURN FALSE;
            END IF;
        END LOOP;
        CLOSE subfolder_cursor;
    END;
    
    COMMIT;
    RETURN TRUE;
END //  

CREATE FUNCTION MoveFolder(from_folder_id VARCHAR(255), to_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    -- Check if folders exist
    IF NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = from_folder_id) OR
       NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = to_folder_id) THEN
        RETURN FALSE;
    END IF;
    
    START TRANSACTION;
    
    -- Update parent folder ID
    UPDATE Folder SET parentFolderID = to_folder_id 
    WHERE folderID = from_folder_id;
    
    -- If successful, commit and return true
    IF ROW_COUNT() > 0 THEN
        COMMIT;
        RETURN TRUE;
    ELSE
        ROLLBACK;
        RETURN FALSE;
    END IF;
END //

CREATE FUNCTION RenameFolder(folder_id VARCHAR(255), new_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    -- Check if folder exists
    IF NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = folder_id) THEN
        RETURN FALSE;
    END IF;
    
    -- Check if new name already exists in same parent folder
    IF EXISTS (
        SELECT 1 FROM Folder f1, Folder f2 
        WHERE f1.folderID = folder_id 
        AND f1.parentFolderID = f2.parentFolderID 
        AND f2.folderName = new_name
    ) THEN
        RETURN FALSE;
    END IF;
    
    -- Update folder name
    UPDATE Folder SET folderName = new_name 
    WHERE folderID = folder_id;
    
    RETURN TRUE;
END //

CREATE FUNCTION DeleteFolder(folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE done INT DEFAULT FALSE;
    DECLARE sub_folder_id VARCHAR(255);
    
    -- Cursor for subfolders
    DECLARE folder_cursor CURSOR FOR 
        SELECT folderID FROM Folder 
        WHERE parentFolderID = folder_id;
        
    DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
    
    START TRANSACTION;
    
    -- Delete all files in the folder
    DELETE FROM File WHERE folderID = folder_id;
    
    -- Recursively delete subfolders
    OPEN folder_cursor;
    read_loop: LOOP
        FETCH folder_cursor INTO sub_folder_id;
        IF done THEN
            LEAVE read_loop;
        END IF;
        
        IF NOT DeleteFolder(sub_folder_id) THEN
            ROLLBACK;
            CLOSE folder_cursor;
            RETURN FALSE;
        END IF;
    END LOOP;
    
    CLOSE folder_cursor;
    
    -- Delete the folder itself
    DELETE FROM Folder WHERE folderID = folder_id;
    
    COMMIT;
    RETURN TRUE;
END //

-- File operations
CREATE FUNCTION CreateFile(file_name VARCHAR(255), file_size BIGINT, folder_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE new_file_id VARCHAR(255);
    
    -- Check if folder exists
    IF NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = folder_id) THEN
        RETURN NULL;
    END IF;
    
    -- Check if file already exists in folder
    IF EXISTS (SELECT 1 FROM File WHERE folderID = folder_id AND fName = file_name) THEN
        RETURN NULL;
    END IF;
    
    -- Create new file
    SET new_file_id = UUID();
    INSERT INTO File (fileID, folderID, fName, fileSize)
    VALUES (new_file_id, folder_id, file_name, file_size);
    
    RETURN new_file_id;
END //

CREATE FUNCTION CopyFile(file_id VARCHAR(255), to_folder_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE new_file_id VARCHAR(255);
    
    -- Check if source file and destination folder exist
    IF NOT EXISTS (SELECT 1 FROM File WHERE fileID = file_id) OR
       NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = to_folder_id) THEN
        RETURN NULL;
    END IF;
    
    -- Create copy of file
    SET new_file_id = UUID();
    INSERT INTO File (fileID, folderID, fName, fileSize)
    SELECT new_file_id, to_folder_id, fName, fileSize
    FROM File
    WHERE fileID = file_id;
    
    RETURN new_file_id;
END //

CREATE FUNCTION MoveFile(file_id VARCHAR(255), to_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    -- Check if file and destination folder exist
    IF NOT EXISTS (SELECT 1 FROM File WHERE fileID = file_id) OR
       NOT EXISTS (SELECT 1 FROM Folder WHERE folderID = to_folder_id) THEN
        RETURN FALSE;
    END IF;
    
    -- Move file to new folder
    UPDATE File SET folderID = to_folder_id
    WHERE fileID = file_id;
    
    RETURN TRUE;
END //

CREATE FUNCTION RenameFile(file_id VARCHAR(255), new_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    -- Check if file exists
    IF NOT EXISTS (SELECT 1 FROM File WHERE fileID = file_id) THEN
        RETURN FALSE;
    END IF;
    
    -- Check if new name already exists in same folder
    IF EXISTS (
        SELECT 1 FROM File f1, File f2
        WHERE f1.fileID = file_id
        AND f1.folderID = f2.folderID
        AND f2.fName = new_name
    ) THEN
        RETURN FALSE;
    END IF;
    
    -- Update file name
    UPDATE File SET fName = new_name
    WHERE fileID = file_id;
    
    RETURN TRUE;
END //

CREATE FUNCTION DeleteFile(file_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DELETE FROM File WHERE fileID = file_id;
    RETURN TRUE;
END //

CREATE FUNCTION GetFileID(file_name VARCHAR(255), folder_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    SELECT fileID INTO file_id
    FROM File
    WHERE fName = file_name AND folderID = folder_id;
    RETURN file_id;
END //  

CREATE FUNCTION GetFileAccess(file_id VARCHAR(255))
RETURNS ENUM('private', 'view', 'download') DETERMINISTIC
BEGIN
    SELECT access INTO access
    FROM File
    WHERE fileID = file_id;
    RETURN access;
END //

CREATE FUNCTION SetFileAccess(file_id VARCHAR(255), access ENUM('private', 'view', 'download'))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    UPDATE File SET access = access WHERE fileID = file_id;
    RETURN TRUE;
END //

CREATE FUNCTION SetFolderAccess(folder_id VARCHAR(255), access ENUM('private', 'view', 'download'))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    UPDATE Folder SET access = access WHERE folderID = folder_id;
    UPDATE File SET access = access WHERE folderID = folder_id;
    RETURN TRUE;
END //

CREATE FUNCTION GetFolderAccess(folder_id VARCHAR(255))
RETURNS ENUM('private', 'view', 'download') DETERMINISTIC
BEGIN
    SELECT access INTO access
    FROM Folder
    WHERE folderID = folder_id;
    RETURN access;
END //

CREATE FUNCTION SearchFile(file_name VARCHAR(255)) 
RETURNS TABLE (
    fileID VARCHAR(255),
    folderID VARCHAR(255),
    fileName VARCHAR(255),
    fileSize BIGINT,
    folderName VARCHAR(255),
    createBy VARCHAR(255),
    access ENUM('private', 'view', 'download')
) DETERMINISTIC
BEGIN
    RETURN SELECT 
        f.fileID,
        f.folderID,
        f.fName as fileName,
        f.fileSize,
        fo.folderName,
        fo.createBy,
        f.access
    FROM File f
    JOIN Folder fo ON f.folderID = fo.folderID
    WHERE f.fName LIKE CONCAT('%', file_name, '%') AND (f.access = 'view' OR f.access = 'download');
END //

CREATE FUNCTION SearchFolder(folder_name VARCHAR(255)) 
RETURNS TABLE (
    folderID VARCHAR(255),
    folderName VARCHAR(255),
    createBy VARCHAR(255),
    access ENUM('private', 'view', 'download')
) DETERMINISTIC
BEGIN
    RETURN SELECT * FROM Folder WHERE folderName LIKE CONCAT('%', folder_name, '%') AND (access = 'view' OR access = 'download');
END //

DELIMITER ;
