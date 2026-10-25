@echo off
setlocal

rem source directory without a trailing backslash, for cmake
for %%I in ("%~dp0") do set "SRCDIR=%%~fI"
set "SRCDIR=%SRCDIR:~0,-1%"
cd /d "%SRCDIR%"

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
call %VCVARS% >nul
if errorlevel 1 (
    echo Failed to init MSVC x86 environment
    exit /b 1
)

set BUILD_TYPE=Release
set STANDALONE=OFF
set METAHHOOK=OFF
set NOBROWSER=OFF
set "HL_DIR="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /I "%~1"=="release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /I "%~1"=="standalone" (
    set STANDALONE=ON
    shift
    goto parse_args
)
if /I "%~1"=="metahook" (
    set METAHHOOK=ON
    shift
    goto parse_args
)
if /I "%~1"=="nometahook" (
    set METAHHOOK=OFF
    shift
    goto parse_args
)
if /I "%~1"=="nobrowser" (
    set NOBROWSER=ON
    shift
    goto parse_args
)
if "%STANDALONE%"=="ON" if "%HL_DIR%"=="" (
    set "HL_DIR=%~1"
    shift
    goto parse_args
)
echo Unknown argument: %~1
exit /b 1
:args_done
if "%STANDALONE%"=="ON" if "%HL_DIR%"=="" (
    if exist "%SRCDIR%\..\raspad-hl\src\launcher.cpp" (
        set "HL_DIR=%~dp0..\raspad-hl"
    ) else (
        set "HL_DIR=%~dp0..\hl"
    )
)

rem One wrapper per game: <name> <game dir> <steam appid>.
rem The first one keeps the historical build\ directory.
call :build_edition cstrike cstrike 10
if errorlevel 1 exit /b 1
call :build_edition svencoop svencoop 225840
if errorlevel 1 exit /b 1

echo.
echo Build OK:
call :report cstrike
call :report svencoop
endlocal
exit /b 0

:build_edition
set NAME=%~1
set GAME=%~2
set APPID=%~3
call :builddir
if not exist "%BUILDDIR%" mkdir "%BUILDDIR%"
pushd "%BUILDDIR%"
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DVELLUM_STANDALONE=%STANDALONE% -DHL_METAHHOOK=%METAHHOOK% -DVELLUM_NO_SERVER_BROWSER=%NOBROWSER% -DHL_DIR="%HL_DIR%" -DVELLUM_NAME="%NAME%" -DVELLUM_GAME="%GAME%" -DVELLUM_APPID="%APPID%" "%SRCDIR%"
if errorlevel 1 (
    popd
    exit /b 1
)
cmake --build .
if errorlevel 1 (
    popd
    exit /b 1
)
popd
exit /b 0

rem build directory of an edition: the wrapper and the standalone build keep
rem separate caches, they configure the same target differently
:builddir
if /I "%NAME%"=="cstrike" (
    if "%STANDALONE%"=="ON" (set BUILDDIR=build-standalone) else (set BUILDDIR=build)
) else (
    if "%STANDALONE%"=="ON" (set BUILDDIR=build-%NAME%-standalone) else (set BUILDDIR=build-%NAME%)
)
exit /b 0

:report
if /I "%~1"=="cstrike" (
    if "%STANDALONE%"=="ON" (
        echo   build-standalone\%~1.exe build-standalone\steamclient.dll ^(%BUILD_TYPE%^)
    ) else (
        echo   build\%~1.exe build\steamclient.dll ^(%BUILD_TYPE%^)
    )
) else (
    if "%STANDALONE%"=="ON" (
        echo   build-%~1-standalone\%~1.exe build-%~1-standalone\steamclient.dll ^(%BUILD_TYPE%^)
    ) else (
        echo   build-%~1\%~1.exe build-%~1\steamclient.dll ^(%BUILD_TYPE%^)
    )
)
exit /b 0
