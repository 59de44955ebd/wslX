@echo off
setlocal EnableDelayedExpansion
cd /d %~dp0

:: config
set APP_NAME=wslX
set APP_VERSION=1.0.0

::set APP_ICON=app.ico

set DIR=%CD%
set APP_DIR=%CD%\dist\%APP_NAME%\

set DIST_EXE=%APP_NAME%-windows-x64-setup.exe
set DIST_ZIP=%APP_NAME%-windows-x64-portable.7z


:: cleanup
del "dist\%DIST_EXE%" 2>nul
del "dist\%DIST_ZIP%" 2>nul

del "%APP_DIR%\data\menu_open.pson" 2>nul

rmdir /s /q "%APP_DIR%\data\icons" 2>nul
mkdir "%APP_DIR%\data\icons"

copy /y "%APP_DIR%\data\XWinrc.default" "%APP_DIR%\data\xwin\etc\X11\system.XWinrc" >nul


rmdir /s /q "%APP_DIR%\data\menus" 2>nul
xcopy /e /q src\make-menus\dist\make-menus "%APP_DIR%\data\menus\"
copy src\wsl-open-with\dist\wsl-open-with\wsl-open-with.exe "%APP_DIR%\data\menus\"


call :create_7z
call :create_installer

:done
echo.
echo ****************************************
echo Done.
echo ****************************************
echo.
pause

endlocal
goto :eof


:create_7z
if not exist "C:\Program Files\7-Zip\" (
	echo.
	echo ****************************************
	echo 7z.exe not found at default location, omitting .7z creation...
	echo ****************************************
	exit /B
)
echo.
echo ****************************************
echo Creating .7z archive...
echo ****************************************

copy /y register_open_with_linux.cmd "%APP_DIR%\register_open_with_linux.cmd" >nul
copy /y unregister_open_with_linux.cmd "%APP_DIR%\unregister_open_with_linux.cmd" >nul

cd dist
set PATH=C:\Program Files\7-Zip;%PATH%
7z a "%DIST_ZIP%" "%APP_NAME%\*"
cd ..

del "%APP_DIR%\register_open_with_linux.cmd" 2>nul
del "%APP_DIR%\unregister_open_with_linux.cmd" 2>nul

exit /B


:create_installer
if not exist "C:\Program Files (x86)\NSIS\" (
	echo.
	echo ****************************************
	echo NSIS not found at default location, omitting installer creation...
	echo ****************************************
	exit /B
)
echo.
echo ****************************************
echo Creating installer...
echo ****************************************

:: get length of APP_DIR
set TF=%TMP%\x
echo %APP_DIR%> %TF%
for %%? in (%TF%) do set /a LEN=%%~z? - 2
del %TF%

call :make_abs_nsh nsis\uninstall_list.nsh

del "%NSH%" 2>nul

cd "%APP_DIR%"

for /F %%f in ('dir /b /a-d') do (
	echo Delete "$INSTDIR\%%f" >> "%NSH%"
)

for /F %%d in ('dir /s /b /aD') do (
	cd "%%d"
	set DIR_REL=%%d
	for /F %%f IN ('dir /b /a-d 2^>nul') do (
		echo Delete "$INSTDIR\!DIR_REL:~%LEN%!\%%f" >> "%NSH%"
	)
)

cd "%APP_DIR%"

for /F %%d in ('dir /s /b /ad^|sort /r') do (
	set DIR_REL=%%d
	echo RMDir "$INSTDIR\!DIR_REL:~%LEN%!" >> "%NSH%"
)

cd "%DIR%"
set PATH=C:\Program Files (x86)\NSIS;%PATH%
makensis nsis\make-installer.nsi
exit /B


:make_abs_nsh
set NSH=%~dpnx1%
exit /B
