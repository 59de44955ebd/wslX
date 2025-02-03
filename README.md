# wslX - an enhanced Windows X-server for WSL

wslX is a customized and repacked version of [Cygwin](https://www.cygwin.com/)'s [Cygwin/X](https://x.cygwin.com/) X.Org server `XWin.exe`, explicitely meant for integrating local WSL Linux GUI applications into the Windows workflow. It supports any Linux distro, but it works best and was mainly tested in combination with a Xfce Desktop environment, that's the recommended environment to use in your distro(s).

It's an alternative for [VcXsrv](https://sourceforge.net/projects/vcxsrv/), [Xming](http://www.straightrunning.com/XmingNotes/) and [WSLg](https://github.com/microsoft/wslg), with a couple of extra features.

wslX is solely installed in user space, it doesn't touch system registry (HKLM) and never requires elevated privileges.

## Features
* Drag and drop support - this is wslX's core feature. If a Linux GUI app - e.g. a text editor, image viewer/editor, media player etc. - natively supports dropping files into its window, drag and drop actions from Windows explorer (including the desktop) are automatically converted into X drop events, and the dropped file paths are converted into WSL Linux paths (e.g. /mnt/c/...). Windows shortcut files (.LNK) are resolved on the fly. So you can quickly open files in WSL Linux GUI apps by dragging and dropping them from explorer.
* Menu icons in the configurable system tray menu (see screenshot below)
* Automatic creation of "Linux start menus" - including icons - for GUI apps detected in the currently installed WSL distro(s).
* "Open with Linux..." context menu in Explorer that allows to open currently selected files directly in some Linux GUI app. This is not implemented as a shell extension, but just as single item added to HKEY_CURRENT_USER\Software\Classes\*\shell, and therefor lightweight regarding Explorer. The actual Linux menu is displayed by an independant process.
* Dark window title bars of Linux GUI app windows, if either Windows currently uses a dark theme or dark mode is explitely activated in system.XWinrc (see below). This is missing in other Windows X-servers like VcXsrv or Xming.
* Dark system tray menu (again if either Windows currently uses a dark theme or dark mode is explitely activated in system.XWinrc)
* Listening for TCP/IP connections is by default activated and listing for unix domain sockets by default deactivated, which is the opposite of Cygwin/X's default server configuration (unix domain sockets don't work in between Cygwin's pseudo-Linux layer and WSL Linux).
* PulseAudio for Windows is included and can be started/stopped via the system tray. Start it before using a Linux media player like e.g. Parole or VLC, otherwise audio wouldn't work.

## Screenshots
*wslX in system tray of Windows 11*  
![wslX in Windows 11](screenshots/wslx_win11.png)

*"Open with Linux..." context menu in Windows 11 Explorer*  
![wslX in Windows 11](screenshots/open-with-linux.png)

## Usage

### a) Installer
The provided installer will automatically try to detect Linux GUI apps in your current WSL Linux Distro(s) after the installation finished, those are then shown in the Linux start menus inside the wslX menu (in the system tray) as well as in the "Open with Linux..." Explorer context menu. The latter will only show those applications that are able to open something (as indicated by having a %f or %u entry in the application's `.desktop` file inside Linux).

You can recreate those menus anytime by selecting `wslX -> (Re)Create application menus` in the wslX menu. 

### b) Portable version (.7z)
If you use the portable version (.7z), you have to select `wslX -> (Re)Create application menus` in the wslX menu after starting wslX for the first time. 

Run `register_open_with_linux.cmd` to add the "Open with Linux..." context menu item to Explorer. Note that the menu won't work anymore if you moved the folder after running the register script, in this case you have to run it again. 

Run `unregister_open_with_linux.cmd` to remove the context menu item from Explorer.

## Notes
* The Linux GUI app detection depends on Python 3 and python3-gi resp. python-gobject being available inside the Linux distro. In Debian-based distros both are usually preinstalled, if app detection fails, run `sudo apt install python3-gi` and then try again. In Arch-based distros use `sudo pacman -S python-gobject` instead.
* "Open with Linux..." is added to the *real* Explorer context menu, not to this weird trailer menu that came with Windows 11. You can deactivate the latter by running  
`
reg.exe add "HKCU\Software\Classes\CLSID\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\InprocServer32" /f /ve
`  
in an elevated CMD shell.
* When searching for Linux GUI apps, for each found Linux distro 2 links are added to folder "wslX" in the Windows startmenu:
  * "Shell\\WSL-[distro-name]" for directly opening a bash shell for this distro.
  * "Desktop\\WSL-[distro-name]" for starting a fullscreen desktop session for this distro.

  Desktop session links are by default configured for Xfce and will only work if you have a Xfce desktop environment (DE) installed. You can manually edit file `data\DesktopSession.ini` inside the wslX program folder if you want to use a different lightweight DE like e.g. LXDE or LXQt. 
  Other DEs like KDE Plasma, GNOME or Cinnamon are generally much harder to run inside WSL.
  
  wslX, when run normally from the startmenu, is configured for multi-window mode, adds a system tray icon and always uses display port :0. Those desktop session links instead use fullscreen mode without tray icon and always use display port :1.

  Use Alt+TAB to switch between the fullscreen Linux desktop and the Windows desktop. When logging out of the Linux desktop session you should automatically return to the Windows desktop, in case this fails use task manager to kill the "XWin.exe" process.

## To-dos
* Language localisation (currently menu items are english only)
* Port utilities make-menus.exe and wsl-open-with.exe from Python to C
