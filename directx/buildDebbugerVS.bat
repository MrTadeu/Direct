@echo off
setlocal enabledelayedexpansion

rem for /f "delims=" %%a in ('cd') do set "currentDir=%%a"
set currentDir= %cd%
cd C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\
call VsDevCmd.bat && (
    cd %currentDir%
    mkdir build
    pushd build
    cl -FC -Zi C:/Users/benno/Desktop/Direct/directx/main.cpp user32.lib gdi32.lib
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