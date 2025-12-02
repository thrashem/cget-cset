@echo off
CHCP 65001 >nul
setlocal enabledelayedexpansion

:: 保存先とファイル名の準備
set "SAVE_DIR=G:\マイドライブ\txtScrap\"
if not exist "%SAVE_DIR%" mkdir "%SAVE_DIR%"

for /f "tokens=1-3 delims=/ " %%a in ("%DATE%") do set "YYYYMMDD=%%a%%b%%c"
set "OUTFILE=%SAVE_DIR%%YYYYMMDD%.md"

:: 一時ファイルにMarkdownを出力
ECHO ## %DATE% %time:~0,8% > temp.txt
ECHO. >> temp.txt
.\cget >> temp.txt
if %ERRORLEVEL% EQU 1 (
    msg "%USERNAME%" "Cannot open clipboard (code 1)"
    goto :EOF
)
if %ERRORLEVEL% EQU 2 (
    msg "%USERNAME%" "No text data (code 2)"
    goto :EOF
)
ECHO. >> temp.txt
ECHO *** >> temp.txt
ECHO. >> temp.txt

:: Markdownファイルに追記
TYPE .\temp.txt >> "%OUTFILE%"
endlocal
