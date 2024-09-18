@echo off
if not exist build mkdir build
set compile= g++ -O2 src/win32_handmade.cpp -DHANDMADE_WIN32=1 -o build/main.exe -lgdi32 -lXinput
REM Compiling with g++:
echo %compile%
%compile%
set result=%errorlevel%

REM Result:
if %result%==0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    exit /b %result%
)

REM Making a symbolic link:
mklink Game build\main.exe > nul 2>&1 