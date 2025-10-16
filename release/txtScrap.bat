@ECHO OFF
CHCP 65001 >nul
SET SAVE_DIR=G:\マイドライブ\txtScrap\
IF NOT EXIST "%SAVE_DIR%" MKDIR "%SAVE_DIR%"
FOR /F "tokens=1-3 delims=/ " %%a IN ("%DATE%") DO SET YYYYMMDD=%%a%%b%%c
ECHO ## %DATE% %time:~0,8% >> "%SAVE_DIR%%YYYYMMDD%.md"
ECHO. >> "%SAVE_DIR%%YYYYMMDD%.md"
.\cget >> "%SAVE_DIR%%YYYYMMDD%.md"
ECHO. >> "%SAVE_DIR%%YYYYMMDD%.md"
ECHO *** >> "%SAVE_DIR%%YYYYMMDD%.md"
ECHO. >> "%SAVE_DIR%%YYYYMMDD%.md"
IF %ERRORLEVEL% NEQ 0 msg "%USERNAME%" "保存に失敗しました"
