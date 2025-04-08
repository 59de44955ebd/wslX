from ctypes import *
from ctypes.wintypes import *
import os
import sys

########################################
# Constants
########################################
APP_NAME = 'WslOpenWith'
APP_CLASS = 'WslOpenWith'

APP_DIR = os.path.dirname(os.path.realpath(__file__))
IS_FROZEN = getattr(sys, 'frozen', False) and hasattr(sys, '_MEIPASS')
if IS_FROZEN:
    DATA_DIR = os.path.join(APP_DIR, '..', '..')
else:
    DATA_DIR = os.path.join(APP_DIR, '..', '..', 'dist', 'wslX', 'data')
ICO_DIR = os.path.join(DATA_DIR, 'icons')

ERROR_SUCCESS       = 0
HKEY_CURRENT_USER   = -2147483647
IMAGE_BITMAP        = 0
IMAGE_ICON          = 1
LR_CREATEDIBSECTION = 8192
LR_LOADFROMFILE     = 16
MF_POPUP            = 16
MF_STRING           = 0
MIIM_BITMAP         = 128
TPM_LEFTBUTTON      = 0
TPM_RETURNCMD       = 256
WM_NULL             = 0
WS_OVERLAPPEDWINDOW = 13565952

########################################
# Additional Types
########################################
is_64_bit = sys.maxsize > 2**32
LONG_PTR = c_longlong if is_64_bit else c_long
UINT_PTR = WPARAM
WNDPROC = WINFUNCTYPE(LONG_PTR, HWND, UINT, WPARAM, LPARAM)
LSTATUS = LONG
PHKEY = POINTER(HKEY)

########################################
# Structures
########################################

class ICONINFO(Structure):
    _fields_ = [
        ("fIcon", BOOL),
        ("xHotspot", DWORD),
        ("yHotspot", DWORD),
        ("hbmMask", HBITMAP),
        ("hbmColor", HBITMAP)
    ]

class MENUITEMINFOW(Structure):
    def __init__(self, *args, **kwargs):
        super(MENUITEMINFOW, self).__init__(*args, **kwargs)
        self.cbSize = sizeof(self)
    _fields_ = [
        ('cbSize', UINT),
        ('fMask', UINT),
        ('fType', UINT),
        ('fState', UINT),
        ('wID', UINT),
        ('hSubMenu', HMENU),
        ('hbmpChecked', HBITMAP),
        ('hbmpUnchecked', HBITMAP),
        ('dwItemData', HANDLE), #ULONG_PTR
        ('dwTypeData', LPWSTR),
        ('cch', UINT),
        ('hbmpItem', HANDLE),
    ]

class WNDCLASSEX(Structure):
    def __init__(self, *args, **kwargs):
        super(WNDCLASSEX, self).__init__(*args, **kwargs)
        self.cbSize = sizeof(self)
    _fields_ = [
        ("cbSize", c_uint),
        ("style", c_uint),
        ("lpfnWndProc", WNDPROC),
        ("cbClsExtra", c_int),
        ("cbWndExtra", c_int),
        ("hInstance", HANDLE),
        ("hIcon", HANDLE),
        ("hCursor", HANDLE),
        ("hBrush", HANDLE),
        ("lpszMenuName", LPCWSTR),
        ("lpszClassName", LPCWSTR),
        ("hIconSm", HANDLE)
    ]

########################################
# Used Win32 API Functions
########################################
advapi32 = windll.Advapi32
gdi32 = windll.Gdi32
shell32 = windll.shell32
user32 = windll.user32
uxtheme = windll.UxTheme

advapi32.RegOpenKeyW.argtypes = (HKEY, LPCWSTR, PHKEY)
advapi32.RegOpenKeyW.restype = LSTATUS
advapi32.RegCloseKey.argtypes = (HKEY, )
advapi32.RegCloseKey.restype = LSTATUS
advapi32.RegQueryValueExW.argtypes = (HKEY, LPCWSTR, POINTER(DWORD), POINTER(DWORD), c_void_p, POINTER(DWORD))
advapi32.RegQueryValueExW.restype = LSTATUS
gdi32.DeleteObject.argtypes = (HANDLE, )
shell32.ShellExecuteW.argtypes = (HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT)
user32.AppendMenuW.argtypes = (HMENU, UINT, UINT_PTR, LPCWSTR)
user32.CopyImage.argtypes = (HBITMAP, UINT, INT, INT, UINT)
user32.CopyImage.restype = HBITMAP
user32.CreateMenu.restype = HMENU
user32.CreatePopupMenu.restype = HMENU
user32.CreateWindowExW.argtypes = (DWORD, LPCWSTR, LPCWSTR, DWORD, INT, INT, INT, INT, HWND, HMENU, HINSTANCE, LPVOID)
user32.CreateWindowExW.restype = HWND
user32.DefWindowProcW.argtypes = (HWND, c_uint, WPARAM, LPARAM)
user32.DestroyWindow.argtypes = (HWND,)
#user32.DispatchMessageW.argtypes = (POINTER(MSG),)
user32.GetCursorPos.argtypes = (POINTER(POINT),)
user32.GetIconInfo.argtypes = (HICON, LPVOID)
#user32.GetMessageW.argtypes = (POINTER(MSG), HWND, UINT, UINT)
user32.LoadImageW.argtypes = (HINSTANCE, LPCWSTR, UINT, INT, INT, UINT)
user32.LoadImageW.restype = HANDLE
#user32.MessageBoxW.argtypes = (HWND, LPCWSTR, LPCWSTR, UINT)
user32.PostMessageW.argtypes = (HWND, UINT, LPVOID, LPVOID)
user32.PostMessageW.restype = LONG_PTR
user32.RegisterClassExW.argtypes = (POINTER(WNDCLASSEX),)
user32.SetForegroundWindow.argtypes = (HWND,)
user32.SetMenuItemInfoW.argtypes = (HMENU, UINT, BOOL, POINTER(MENUITEMINFOW))
user32.TrackPopupMenuEx.argtypes = (HANDLE, UINT, INT, INT, HANDLE, c_void_p)
#user32.TranslateMessage.argtypes = (POINTER(MSG),)

# Those require Windows 10+, but WSL does anyway
uxtheme.SetPreferredAppMode = uxtheme[135]
uxtheme.FlushMenuThemes = uxtheme[136]

########################################
# Custom Functions
########################################

def reg_should_use_dark_mode():
    use_dark_mode = False
    hkey = HKEY()
    if advapi32.RegOpenKeyW(HKEY_CURRENT_USER, 'Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize' , byref(hkey)) == ERROR_SUCCESS:
        data = DWORD()
        cbData = DWORD(sizeof(data))
        # 'SystemUsesLightTheme'
        if advapi32.RegQueryValueExW(hkey, 'AppsUseLightTheme', None, None, byref(data), byref(cbData)) == ERROR_SUCCESS:
            use_dark_mode = not bool(data.value)
        advapi32.RegCloseKey(hkey)
    return use_dark_mode

def load_icon_as_bitmap(ico_file):
    h_icon = user32.LoadImageW(None, ico_file, IMAGE_ICON, 16, 16, LR_LOADFROMFILE)
    if h_icon:
        icon_info = ICONINFO()
        user32.GetIconInfo(h_icon, byref(icon_info))
        h_bitmap = user32.CopyImage(icon_info.hbmColor, IMAGE_BITMAP, 16, 16, LR_CREATEDIBSECTION)
        gdi32.DeleteObject(icon_info.hbmColor)
        gdi32.DeleteObject(icon_info.hbmMask)
        return h_bitmap


########################################
#
########################################
if __name__ == '__main__':

    pson_file = os.path.join(DATA_DIR, 'menu_open.pson')
    if not os.path.isfile(pson_file):
        #user32.MessageBoxW(None, 'No applications found', 'Warning', 0x000000030)
        sys.exit(0)

    try:
        with open(pson_file, 'r') as f:
            menu_config = eval(f.read())
    except:
        #user32.MessageBoxW(None, 'No applications found', 'Warning', 0x000000030)
        sys.exit(0)

    newclass = WNDCLASSEX()
    newclass.lpfnWndProc = WNDPROC(user32.DefWindowProcW)
    newclass.lpszClassName = APP_CLASS
    user32.RegisterClassExW(byref(newclass))

    hwnd = user32.CreateWindowExW(
        0,
        APP_CLASS,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0,
        None, None, None, None
    )

    if reg_should_use_dark_mode():
        uxtheme.SetPreferredAppMode(2)  # ForceDark = 2
        uxtheme.FlushMenuThemes()

    idm = 0
    exec_dict = {}
    hmenu = user32.CreatePopupMenu()

    for distro_name, distro_row in menu_config.items():

        hmenu_distro = user32.CreateMenu()
        user32.AppendMenuW(hmenu, MF_POPUP, hmenu_distro, distro_name)
        info = MENUITEMINFOW()
        info.fMask = MIIM_BITMAP
        info.hbmpItem = load_icon_as_bitmap(os.path.join(ICO_DIR, 'menu-' + distro_name + '.ico'))
        user32.SetMenuItemInfoW(hmenu, hmenu_distro, 0, byref(info))

        for cat_name, cat_row in distro_row.items():
            if len(cat_row.keys()) == 0:
                continue

            hmenu_cat = user32.CreateMenu()
            user32.AppendMenuW(hmenu_distro, MF_POPUP, hmenu_cat, cat_name)
            info = MENUITEMINFOW()
            info.fMask = MIIM_BITMAP
            info.hbmpItem = load_icon_as_bitmap(os.path.join(ICO_DIR, 'menu-' + cat_name + '.ico'))
            user32.SetMenuItemInfoW(hmenu_distro, hmenu_cat, 0, byref(info))

            for app_name, app_row in cat_row.items():
                idm += 1
                exec_dict[idm] = f"-d {distro_name} {app_row}"
                user32.AppendMenuW(hmenu_cat, MF_STRING, idm, app_name)
                info = MENUITEMINFOW()
                info.fMask = MIIM_BITMAP
                info.hbmpItem = load_icon_as_bitmap(os.path.join(ICO_DIR, app_name + '.ico'))
                user32.SetMenuItemInfoW(hmenu_cat, idm, 0, byref(info))

    pt = POINT()
    user32.GetCursorPos(byref(pt))
    user32.SetForegroundWindow(hwnd)
    idm = user32.TrackPopupMenuEx(hmenu, TPM_LEFTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd, None)
    if idm:
        os.environ['DISPLAY'] = ':0'
        os.environ['QT_QPA_PLATFORMTHEME'] = 'qt5ct'
        os.environ['WSLENV'] = 'DISPLAY:QT_QPA_PLATFORMTHEME'

        # Make sure that wslx is running
        shell32.ShellExecuteW(0, None, os.path.join(DATA_DIR, 'xwin', 'bin', 'xwin.exe'),
                ':0 -silent-dup-error -multiwindow -wgl', os.path.join(DATA_DIR, '..'), 0)

        # Run the selected Linux app
        arg = '"' + f'`wslpath "{sys.argv[1]}"`' + '"'
        # Using a regexp would be nicer, but this way we don't have to import re just for a single task
        command = exec_dict[idm].replace('%U', arg).replace('%u', arg).replace('%F', arg).replace('%f', arg)
        shell32.ShellExecuteW(0, None, 'wslg.exe', command, None, 0)

    user32.DestroyWindow(hwnd)
