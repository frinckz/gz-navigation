@echo off
setlocal

rem Uninstalls the mod: moves the mod DLL back into gz_nav\ and restores the backed-up xinput9_1_0.dll
set "MOD_DIR=%~dp0"
for %%I in ("%MOD_DIR%..") do set "GAME_DIR=%%~fI"
set "DLL=xinput9_1_0.dll"
set "BACKUP=xinput9_1_0.dll.gz_nav_backup"

if exist "%MOD_DIR%%DLL%" (
    echo The mod is not installed.
    goto :done
)

if not exist "%GAME_DIR%\%DLL%" (
    echo %DLL% not found in "%GAME_DIR%". Nothing to uninstall.
    goto :fail
)

tasklist /FI "IMAGENAME eq GenerationZero_F.exe" 2>nul | find /I "GenerationZero_F.exe" >nul
if not errorlevel 1 (
    echo Generation Zero is running. Close the game and run uninstall.bat again.
    goto :fail
)

move /Y "%GAME_DIR%\%DLL%" "%MOD_DIR%%DLL%" >nul || goto :denied

if exist "%GAME_DIR%\%BACKUP%" (
    echo Restoring the original %DLL% from %BACKUP%
    move /Y "%GAME_DIR%\%BACKUP%" "%GAME_DIR%\%DLL%" >nul || goto :denied
)

echo Uninstalled.
goto :done

:denied
echo Moving the files failed. Run uninstall.bat as administrator.

:fail
pause
exit /b 1

:done
pause
exit /b 0
