@echo off

cd /d %~dp0

:: config
set APP_NAME=wsl-open-with
set APP_ICON=app.ico
set DIR=%CD%
set APP_DIR=%CD%\dist\%APP_NAME%\

set PYQTDESIGNERPATH=
set PYTHONPATH=

:: cleanup
rmdir /s /q "dist\%APP_NAME%" 2>nul

echo.
echo ****************************************
echo Running pyinstaller...
echo ****************************************

pyinstaller --noupx -w -i "%APP_ICON%" -n "%APP_NAME%" --version-file=version_res.txt --exclude PyQt5 --exclude PyQt6 -D main.py

echo.
echo ****************************************
echo Optimizing dist folder...
echo ****************************************

del "dist\%APP_NAME%\_internal\api-ms-win-*.dll"
del "dist\%APP_NAME%\_internal\libcrypto-3.dll"
del "dist\%APP_NAME%\_internal\ucrtbase.dll"
del "dist\%APP_NAME%\_internal\unicodedata.pyd"
del "dist\%APP_NAME%\_internal\_bz2.pyd"
del "dist\%APP_NAME%\_internal\_lzma.pyd"

:done
echo.
echo ****************************************
echo Done.
echo ****************************************
echo.
pause
