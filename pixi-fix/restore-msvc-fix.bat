@echo off
:: ============================================================================
:: One-click restore of the VS 2026 MSVC fallback activation script.
::
:: Background: this machine only has Visual Studio 2026 (v18) installed. The
:: conda package "vs2022_win-64" used by the pixi environment cannot find it
:: (its vswhere queries and hardcoded VS2022 paths all miss), so its
:: activation script fails and leaves CC=cl.exe dangling -> CMake aborts with
:: "Could not find compiler set in environment variable CC: cl.exe".
::
:: The fix is an extra activation script, kept next to this file, that gets
:: copied into the pixi environment's activate.d. Whenever the environment is
:: recreated (e.g. "pixi clean", a lock file change, or deleting .pixi), that
:: copy is lost - just run this script again.
:: ============================================================================

setlocal
set "SCRIPT_DIR=%~dp0"
for %%i in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fi"

set "SRC=%SCRIPT_DIR%vs2026_fallback_activate.bat"
set "TARGET_DIR=%REPO_ROOT%\.pixi\envs\default\etc\conda\activate.d"

if not exist "%SRC%" (
    echo [ERROR] Missing "%SRC%" - cannot restore.
    goto :fail
)

if not exist "%REPO_ROOT%\.pixi\envs\default" (
    echo [WARN]  Pixi environment not found yet. Run "pixi install" in
    echo        %REPO_ROOT%
    echo        first, then run this script again.
    goto :fail
)

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
copy /Y "%SRC%" "%TARGET_DIR%\" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy to "%TARGET_DIR%".
    goto :fail
)
echo [OK]    Restored: %TARGET_DIR%\vs2026_fallback_activate.bat

:: Verify that MSVC now resolves inside the pixi environment.
set "PIXI_EXE="
where pixi >nul 2>&1
if not errorlevel 1 set "PIXI_EXE=pixi"
if not defined PIXI_EXE if exist "%LOCALAPPDATA%\pixi\bin\pixi.exe" set "PIXI_EXE=%LOCALAPPDATA%\pixi\bin\pixi.exe"

if not defined PIXI_EXE (
    echo [SKIP]  pixi not found - start any pixi task once to verify, e.g.:
    echo        pixi run where cl.exe
) else (
    echo [INFO]  Verifying MSVC activation ...
    pushd "%REPO_ROOT%"
    "%PIXI_EXE%" run --force-activate where cl.exe
    popd
    if errorlevel 1 (
        echo [ERROR] cl.exe still not found - see messages above.
        goto :fail
    )
    echo [OK]    MSVC toolchain resolves. You are ready to build:
    echo        pixi run configure-debug ^&^& pixi run build-debug
)

echo.
echo Done.
endlocal
pause
exit /b 0

:fail
echo.
endlocal
pause
exit /b 1
