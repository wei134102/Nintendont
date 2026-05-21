@echo off
setlocal EnableExtensions

REM -----------------------------------------------------------------------------
REM Nintendont Windows build script
REM Uses a dedicated devkitPro tree (default: C:\devkitPro.Nintendont).
REM
REM Override the root before running, e.g.:
REM   set NINTENDONT_DEVKITPRO=D:\tools\devkitPro.Nintendont
REM   Build.bat
REM
REM Extra make goals are forwarded, e.g.:
REM   Build.bat clean
REM   Build.bat loader
REM -----------------------------------------------------------------------------

if not defined NINTENDONT_DEVKITPRO (
	set "NINTENDONT_DEVKITPRO=C:\devkitPro.Nintendont"
)

set "DEVKITPRO=%NINTENDONT_DEVKITPRO%"
set "DEVKITPPC=%DEVKITPRO%\devkitPPC"
set "DEVKITARM=%DEVKITPRO%\devkitARM"

REM Prefer this devkit's MSYS2 make and compilers over any system install.
set "PATH=%DEVKITPRO%\msys2\usr\bin;%DEVKITPPC%\bin;%DEVKITARM%\bin;%PATH%"

echo.
echo === Nintendont build (Windows) ===
echo DEVKITPRO  = %DEVKITPRO%
echo DEVKITPPC  = %DEVKITPPC%
echo DEVKITARM  = %DEVKITARM%
echo.

if not exist "%DEVKITPRO%\" (
	echo ERROR: devkitPro root not found: %DEVKITPRO%
	echo        Install devkitPro to that path or set NINTENDONT_DEVKITPRO.
	goto :fail
)
if not exist "%DEVKITPPC%\bin\powerpc-eabi-gcc.exe" (
	echo ERROR: devkitPPC not found under %DEVKITPPC%
	goto :fail
)
if not exist "%DEVKITARM%\bin\arm-none-eabi-gcc.exe" (
	echo ERROR: devkitARM not found under %DEVKITARM%
	goto :fail
)
if not exist "%DEVKITPRO%\msys2\usr\bin\make.exe" (
	echo ERROR: MSYS2 make not found under %DEVKITPRO%\msys2\usr\bin
	goto :fail
)

cd /d "%~dp0"

if "%~1"=="" (
	make forced windows=1
) else (
	make windows=1 %*
)

if errorlevel 1 goto :fail

echo.
echo Build finished successfully.
goto :done

:fail
echo.
echo Build failed.
pause
exit /b 1

:done
pause
exit /b 0
