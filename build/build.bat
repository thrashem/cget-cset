@echo off
echo Building cget and cset...
echo.

REM Check if GCC is available
gcc --version >nul 2>&1
if errorlevel 1 (
    echo Error: GCC not found. Please install MinGW-w64:
    echo   winget install BrechtSanders.WinLibs.POSIX.UCRT
    pause
    exit /b 1
)

REM Create directories
if not exist "releases" mkdir releases

REM Build executables
echo Building cget.exe...
gcc -O2 -s -static src/cget.c -o releases/cget.exe
if errorlevel 1 (
    echo Error building cget.exe
    pause
    exit /b 1
)

echo Building cset.exe...
gcc -O2 -s -static src/cset.c -o releases/cset.exe
if errorlevel 1 (
    echo Error building cset.exe
    pause
    exit /b 1
)

echo.
echo Build complete!
echo.
echo File sizes:
dir releases\*.exe

echo.
echo Test the executables:
echo   releases\cget --help
echo   releases\cset --help
pause