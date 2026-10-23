@echo off
setlocal

rem Installs the mod: backs up an existing xinput9_1_0.dll in the game folder and moves the mod DLL from gz_nav\ next to the exe
set "MOD_DIR=%~dp0"
for %%I in ("%MOD_DIR%..") do set "GAME_DIR=%%~fI"
set "DLL=xinput9_1_0.dll"
set "BACKUP=xinput9_1_0.dll.gz_nav_backup"

if not exist "%GAME_DIR%\GenerationZero_F.exe" (
    echo GenerationZero_F.exe not found in "%GAME_DIR%".
    echo Move the gz_nav folder next to GenerationZero_F.exe and run install.bat again.
    goto :fail
)

if not exist "%MOD_DIR%%DLL%" (
    if exist "%GAME_DIR%\%DLL%" (
        echo The mod is already installed.
        goto :done
    )
    echo %DLL% is missing from "%MOD_DIR%". Download the mod again.
    goto :fail
)

tasklist /FI "IMAGENAME eq GenerationZero_F.exe" 2>nul | find /I "GenerationZero_F.exe" >nul
if not errorlevel 1 (
    echo Generation Zero is running. Close the game and run install.bat again.
    goto :fail
)

if exist "%GAME_DIR%\%DLL%" (
    if exist "%GAME_DIR%\%BACKUP%" (
        echo A backup already exists, replacing the installed %DLL% without a new backup.
        del /F "%GAME_DIR%\%DLL%" || goto :denied
    ) else (
        echo Backing up the existing %DLL% to %BACKUP%
        move /Y "%GAME_DIR%\%DLL%" "%GAME_DIR%\%BACKUP%" >nul || goto :denied
    )
)

move /Y "%MOD_DIR%%DLL%" "%GAME_DIR%\%DLL%" >nul || goto :denied

echo Installed. Edit gz_nav\settings.ini to change the game mode.
goto :done

:denied
echo Moving the files failed. Run install.bat as administrator.

:fail
pause
exit /b 1

:done
pause
exit /b 0
