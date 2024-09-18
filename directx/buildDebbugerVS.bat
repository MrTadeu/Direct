@echo off
setlocal enabledelayedexpansion

REM for /f "delims=" %%a in ('cd') do set "currentDir=%%a"
if not exist build mkdir build
set currentDir= %cd%
set compile= cl -Fe:main.exe -DHANDMADE_WIN32=1 -DVISUAL_STUDIO_OUTPUTCONSOLE=1 /EHsc -FC -Zi C:/Users/benno/Desktop/Direct/directx/src/win32_handmade.cpp user32.lib gdi32.lib 
cd C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\
call VsDevCmd.bat && (
    cd %currentDir%
    mkdir build
    pushd build
    %compile%
    set /p a="Devenv? [Y/N]: "
    echo Valor: !a!
    if /I "!a!"=="Y" (
        devenv main.exe
    ) else if /I "!a!"=="Yes" (
        devenv main.exe
    ) else (
        main.exe
    )
    popd
)

REM Verifica e remove o link ou arquivo existente:
if exist Game del Game

REM Making a symbolic link:
mklink Game build\main.exe > nul 2>&1