DELIMITER //

DROP DATABASE IF EXISTS file_sharing;
CREATE DATABASE file_sharing;
USE file_sharing;

CREATE TABLE `Users` (
    `userName` VARCHAR(255) NOT NULL,
    `passwordHash` VARCHAR(255) NOT NULL,
    PRIMARY KEY (`userName`)
);

CREATE TABLE `Folder` (
	`folderID` VARCHAR(255) NOT NULL,
    `folderName` VARCHAR(255) NOT NULL,
    `parentFolderID` VARCHAR(255),
    `createBy` VARCHAR(255) NOT NULL,
    `createdAt` TIMESTAMP NOT NULL,
    `access` ENUM('private', 'view', 'download') DEFAULT 'view',
    `isRoot` BOOLEAN DEFAULT FALSE,
    PRIMARY KEY (`folderID`),
    CONSTRAINT `createBy` FOREIGN KEY (`createBy`) REFERENCES `Users`(`userName`),
    CONSTRAINT `parentFolderID` FOREIGN KEY (`parentFolderID`) REFERENCES `Folder`(`folderID`)
);

CREATE TABLE `File` (
	`fileID` VARCHAR(255) NOT NULL,
    `folderID` VARCHAR(255) NOT NULL,
    `fName` VARCHAR(255) NOT NULL,
    `fileSize` BIGINT NOT NULL,
    `createBy` VARCHAR(255) NOT NULL,
    `createdAt` TIMESTAMP NOT NULL,
    `access` ENUM('private', 'view', 'download') DEFAULT 'view',
    PRIMARY KEY (`fileID`),
    CONSTRAINT `folderID` FOREIGN KEY (`folderID`) REFERENCES `Folder`(`folderID`)
);

CREATE FUNCTION HashPasswordWithUserName(userName VARCHAR(255), userPassword VARCHAR(255))
RETURNS VARBINARY(64) DETERMINISTIC
BEGIN
    RETURN UNHEX(SHA2(CONCAT(userPassword, HEX(SHA2(userName, 256))), 256));
END //

CREATE FUNCTION InsertNewUser(pUserName VARCHAR(255), userPassword VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE userExists INT;
    SELECT COUNT(*) INTO userExists FROM Users WHERE userName = pUserName;
    
    IF userExists > 0 THEN
        RETURN FALSE;
    ELSE
        INSERT INTO Users (userName, passwordHash)
        VALUES (pUserName, HEX(HashPasswordWithUserName(pUserName, userPassword)));
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

CREATE FUNCTION CreateFolder(folder_name VARCHAR(255), parent_folder_id VARCHAR(255), user_name VARCHAR(255))
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
        INSERT INTO Folder (folderID, folderName, parentFolderID, createBy, createdAt)
        VALUES (folder_id, folder_name, parent_folder_id, user_name, NOW());

        RETURN TRUE; -- Return TRUE for successful folder creation
    END IF;
END //

CREATE FUNCTION CreateRootFolder(folder_name VARCHAR(255), user_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    INSERT INTO Folder (folderID, folderName, parentFolderID, createBy, createdAt, isRoot)
    VALUES (UUID(), folder_name, NULL, user_name, NOW(), TRUE);
    RETURN TRUE;
END //

CREATE FUNCTION GetRootFolderID(user_name VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE root_folder_id VARCHAR(255);
    SELECT folderID INTO root_folder_id
    FROM Folder
    WHERE createBy = user_name AND isRoot = TRUE;
    RETURN root_folder_id;
END //

CREATE FUNCTION GetFolderID(folder_name VARCHAR(255), user_name VARCHAR(255), parent_folder_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE folder_id VARCHAR(255);
    SELECT folderID INTO folder_id
    FROM Folder
    WHERE folderName = folder_name AND createBy = user_name AND parentFolderID = parent_folder_id;
    RETURN folder_id;
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

-- File operations
CREATE FUNCTION CreateFile(file_name VARCHAR(255), file_size BIGINT, folder_id VARCHAR(255), user_name VARCHAR(255))
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
    INSERT INTO File (fileID, folderID, fName, fileSize, createBy, createdAt)
    VALUES (new_file_id, folder_id, file_name, file_size, user_name, NOW());
    
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
    DECLARE file_id VARCHAR(255);
    SELECT fileID INTO file_id
    FROM File
    WHERE fName = file_name AND folderID = folder_id;
    RETURN file_id;
END //

CREATE FUNCTION GetFileAccess(file_id VARCHAR(255))
RETURNS ENUM('private', 'view', 'download') DETERMINISTIC
BEGIN
    DECLARE tmpAccess ENUM('private', 'view', 'download');
    SELECT access INTO tmpAccess
    FROM File
    WHERE fileID = file_id;
    RETURN tmpAccess;
END //

CREATE FUNCTION SetFileAccess(file_id VARCHAR(255), new_access ENUM('private', 'view', 'download'), user_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    IF EXISTS (SELECT 1 FROM File WHERE fileID = file_id AND createBy = user_name) THEN
        UPDATE File SET access = new_access WHERE fileID = file_id AND createBy = user_name;
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END //

CREATE FUNCTION SetFolderAccess(folder_id VARCHAR(255), new_access ENUM('private', 'view', 'download'), user_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    IF EXISTS (SELECT 1 FROM Folder WHERE folderID = folder_id AND createBy = user_name) THEN
        UPDATE Folder SET access = new_access WHERE folderID = folder_id AND createBy = user_name;
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END //

CREATE FUNCTION GetFolderAccess(folder_id VARCHAR(255))
RETURNS ENUM('private', 'view', 'download') DETERMINISTIC
BEGIN
    DECLARE tmp ENUM('private', 'view', 'download');
    SELECT access INTO tmp
    FROM Folder
    WHERE folderID = folder_id;
    RETURN tmp;
END //

CREATE PROCEDURE SearchFile(IN file_name VARCHAR(255))
BEGIN
    SELECT 
        f.fileID,
        f.folderID,
        f.fName as fileName,
        f.fileSize,
        fo.folderName,
        fo.createBy,
        f.createdAt,
        f.access,
        GetFilePath(f.fileID) as filePath
    FROM File f
    JOIN Folder fo ON f.folderID = fo.folderID
    WHERE f.fName LIKE CONCAT('%', file_name, '%') AND (f.access = 'view' OR f.access = 'download');
END //

CREATE PROCEDURE SearchFolder(IN folder_name VARCHAR(255))
BEGIN
    SELECT 
        folderID,
        folderName,
        createBy,
        access,
        GetFolderPath(folderID) as folderPath
    FROM Folder
    WHERE folderName LIKE CONCAT('%', folder_name, '%') AND (access = 'view' OR access = 'download');
END //


CREATE FUNCTION CheckFolderExist(folder_name VARCHAR(255), user_name VARCHAR(255), parent_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE folder_exists INT;
    SELECT COUNT(*) INTO folder_exists
    FROM Folder
    WHERE folderName = folder_name AND createBy = user_name AND parentFolderID = parent_folder_id;
    IF folder_exists > 0 THEN
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END //

CREATE FUNCTION CheckFileExists(file_name VARCHAR(255), user_name VARCHAR(255), parent_folder_id VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    DECLARE file_exists INT;
    SELECT COUNT(*) INTO file_exists
    FROM File f
    WHERE f.fName = file_name 
    AND f.createBy = user_name 
    AND f.folderID = parent_folder_id;
    IF file_exists > 0 THEN
        RETURN TRUE;
    ELSE
        RETURN FALSE;
    END IF;
END //

CREATE PROCEDURE GetAllFileInFolder(folder_id VARCHAR(255))
BEGIN
    SELECT * FROM File WHERE folderID = folder_id;
END //

CREATE PROCEDURE GetAllFolderInFolder(folder_id VARCHAR(255))
BEGIN
    SELECT * FROM Folder WHERE parentFolderID = folder_id;
END //

CREATE FUNCTION MoveFolder(folder_id VARCHAR(255), parent_folder_id VARCHAR(255), user_name VARCHAR(255))
RETURNS BOOLEAN DETERMINISTIC
BEGIN
    UPDATE Folder SET parentFolderID = parent_folder_id WHERE folderID = folder_id;
    RETURN TRUE;
END //

CREATE FUNCTION GetFilePath(file_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE file_path VARCHAR(255);
    DECLARE folder_id VARCHAR(255);
    DECLARE folder_name VARCHAR(255);
    DECLARE parent_folder_id VARCHAR(255);
    DECLARE done BOOLEAN DEFAULT FALSE;
    
    -- Get initial file name and folder ID
    SELECT f.fName, f.folderID INTO file_path, folder_id 
    FROM File f
    WHERE f.fileID = file_id;
    
    -- Get folder path by traversing up the folder hierarchy
    WHILE folder_id IS NOT NULL DO
        SELECT folderName, parentFolderID 
        INTO folder_name, parent_folder_id
        FROM Folder 
        WHERE folderID = folder_id;
        
        IF folder_name IS NOT NULL THEN
            SET file_path = CONCAT(folder_name, '/', file_path);
        END IF;
        
        SET folder_id = parent_folder_id;
    END WHILE;
    
    RETURN file_path;
END //

CREATE FUNCTION GetFolderPath(folder_id VARCHAR(255))
RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
    DECLARE folder_path VARCHAR(255);
    DECLARE folder_name VARCHAR(255);
    DECLARE parent_folder_id VARCHAR(255);
    DECLARE done BOOLEAN DEFAULT FALSE;

    SELECT folderName, parentFolderID INTO folder_name, parent_folder_id
    FROM Folder
    WHERE folderID = folder_id;

    SET folder_path = folder_name;
    SET folder_id = parent_folder_id;

    WHILE folder_id IS NOT NULL DO
        SELECT folderName, parentFolderID INTO folder_name, parent_folder_id
        FROM Folder
        WHERE folderID = folder_id;

        IF folder_name IS NOT NULL THEN
            SET folder_path = CONCAT(folder_name, '/', folder_path);
        END IF;

        SET folder_id = parent_folder_id;
    END WHILE;

    RETURN folder_path;
END //

DELIMITER ;
