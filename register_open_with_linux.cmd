@echo off
reg add "HKCU\Software\Classes\*\shell\open-with-linux" /v MUIVerb /t REG_SZ /d "Open with Linux..." /f >nul
reg add "HKCU\Software\Classes\*\shell\open-with-linux" /v NeverDefault /t REG_SZ /d "" /f >nul
reg add "HKCU\Software\Classes\*\shell\open-with-linux" /v Icon /t REG_SZ /d "%~dp0data\menus\wsl-open-with.exe" /f >nul
reg add "HKCU\Software\Classes\*\shell\open-with-linux\command" /t REG_SZ /d """%~dp0data\menus\wsl-open-with.exe"" ""%%1""" /f >nul
