@echo off

cd /d %~dp0

:: config
set APP_NAME=make-menus
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

pyinstaller --noupx -n "%APP_NAME%" -i "%APP_ICON%" --version-file=version_res.txt --exclude PyQt5 --exclude PyQt6 --exclude packaging --exclude pkg_resources --exclude setuptools -D main.py
::pyinstaller %APP_NAME%_windows.spec

echo.
echo ****************************************
echo Copying resources...
echo ****************************************

copy cairo.dll "dist\%APP_NAME%\_internal\"
copy find_linux_apps.py "dist\%APP_NAME%\_internal\"

echo.
echo ****************************************
echo Optimizing dist folder...
echo ****************************************

del "dist\%APP_NAME%\_internal\api-ms-win-*.dll"
del "dist\%APP_NAME%\_internal\libcrypto-3.dll"
del "dist\%APP_NAME%\_internal\libssl-3.dll"
del "dist\%APP_NAME%\_internal\ucrtbase.dll"
del "dist\%APP_NAME%\_internal\unicodedata.pyd"
del "dist\%APP_NAME%\_internal\_bz2.pyd"
del "dist\%APP_NAME%\_internal\_elementtree.pyd"
del "dist\%APP_NAME%\_internal\_lzma.pyd"
del "dist\%APP_NAME%\_internal\_ssl.pyd"
del "dist\%APP_NAME%\_internal\_wmi.pyd"

:done
echo.
echo ****************************************
echo Done.
echo ****************************************
echo.
pause
