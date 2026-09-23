@echo off
:: Fallback MSVC activation for hosts that only have Visual Studio 2026 (v18)
:: installed. The vs2022_win-64 activation script (which runs before this one,
:: alphabetical order) can only locate VS2022 (17.x) installations or the
:: hardcoded VS2022 paths under Program Files (x86). On such hosts it fails to
:: run vcvars and leaves CC=cl.exe dangling without cl.exe on PATH, which
:: makes CMake abort with "Could not find compiler set in environment
:: variable CC: cl.exe".
::
:: This script locates the newest VS installation via "vswhere -latest" and
:: initializes the x64 native toolchain, preferring the exact MSVC 14.44
:: toolset that the conda packages in this environment were built with
:: (falling back to the installation's default toolset otherwise).

where cl.exe >nul 2>&1
if not errorlevel 1 goto :done

set "FC_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%FC_VSWHERE%" goto :done

set "FC_VSINSTALLDIR="
for /f "usebackq tokens=*" %%i in (`"%FC_VSWHERE%" -nologo -latest -products * -property installationPath`) do (
    set "FC_VSINSTALLDIR=%%i\"
)

if not exist "%FC_VSINSTALLDIR%VC\Auxiliary\Build\vcvars64.bat" goto :done

:: The failed vs2022 script leaves VSINSTALLDIR (and friends) pointing at a
:: non-existent VS2022 path; VsDevCmd would trust those and abort. Clear them
:: so VsDevCmd re-derives everything from the vcvars64.bat invoked below.
set "VSINSTALLDIR="
set "VCINSTALLDIR="
set "VCToolsInstallDir="
set "VSCMD_VER="
set "VSCMD_ARG_HOST_ARCH="
set "VSCMD_ARG_TGT_ARCH="

:: Prefer the toolset version matching the conda packages (14.44). VC 2026's
:: vcvars only accepts the SxS-registered name (e.g. 14.44.17.14) found in
:: VC\Auxiliary\Build, not the plain toolset version (14.44.35207), so scan
:: for that marker. Fall back to the default toolset if it is missing (MSVC
:: v14x ABI is compatible anyway).
set "FC_VCTOOLSVER="
for /d %%d in ("%FC_VSINSTALLDIR%VC\Auxiliary\Build\14.44.*") do (
    if exist "%FC_VSINSTALLDIR%VC\Auxiliary\Build\%%~nxd\Microsoft.VCToolsVersion.%%~nxd.txt" set "FC_VCTOOLSVER=%%~nxd"
)

if defined FC_VCTOOLSVER (
    call "%FC_VSINSTALLDIR%VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=%FC_VCTOOLSVER%
) else (
    call "%FC_VSINSTALLDIR%VC\Auxiliary\Build\vcvars64.bat"
)

:done
exit /b 0
