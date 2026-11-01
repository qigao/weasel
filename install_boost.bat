@echo off
setlocal

if not defined RIME_ROOT set "RIME_ROOT=%CD%"
if not defined boost_version set "boost_version=1.84.0"
set "boost_x_y_z=%boost_version:.=_%"

if not defined BOOST_ROOT set "BOOST_ROOT=%RIME_ROOT%\deps\boost_%boost_x_y_z%"
if exist "%BOOST_ROOT%\boost" goto boost_found
rem The 7z archive contains a boost_x_y_z top-level directory.
rem Extract into its parent, not into BOOST_ROOT itself.
for %%I in ("%BOOST_ROOT%") do set "src_dir=%%~dpI"
if not exist "%src_dir%" mkdir "%src_dir%"
if errorlevel 1 exit /b 1

rem Keep source acquisition separate from compilation in native CI.
aria2c "https://archives.boost.io/release/%boost_version%/source/boost_%boost_x_y_z%.7z" -d "%src_dir%"
if errorlevel 1 exit /b 1
pushd "%src_dir%"
if errorlevel 1 exit /b 1
7z x -y "boost_%boost_x_y_z%.7z"
if errorlevel 1 (
  popd
  exit /b 1
)
popd

:boost_found
if /I "%~1"=="source-only" exit /b 0
rem Legacy invocation remains source+build for existing release scripts.
call .\build.bat boost
exit /b %ERRORLEVEL%
