from ctypes import windll
from ctypes.wintypes import LPCWSTR, HWND, UINT, LPVOID

kernel32 = windll.Kernel32
kernel32.SetConsoleTitleW.argtypes = (LPCWSTR,)

kernel32.SetConsoleTitleW('Searching for WSL Linux desktop applications...')

user32 = windll.user32
user32.FindWindowW.argtypes = (LPCWSTR, LPCWSTR)
user32.FindWindowW.restype = HWND
user32.SendMessageW.argtypes = (HWND, UINT, LPVOID, LPVOID)

import io
import os
from PIL_min import Image
from cairosvg_min import svg2png
import configparser
import re
import shutil
import subprocess
import sys

# for freezing only
from PIL_min.BmpImagePlugin import BmpImageFile
from PIL_min.IcoImagePlugin import IcoImageFile
from PIL_min.PngImagePlugin import PngImageFile

WM_RELOAD_PREFS = 1124

CATEGORIES = {
    'TextEditor': 'accessories-text-editor',
    'Graphics': 'applications-graphics',
    'Settings': 'preferences-system',  #'package_settings',
    'System': 'applications-system',
    'AudioVideo': 'applications-multimedia',
    'Games': 'applications-games',
    'Network': 'applications-internet',
    'Development': 'applications-development',
    'Science': 'applications-science',
    'Office': 'applications-office',
    'Utility': 'applications-utilities',
    'Other': 'applications-other',
}

ICO_SIZE = (16, 16)

APP_DIR = os.path.dirname(os.path.realpath(__file__))

BLACK_BG = Image.new("RGBA", ICO_SIZE, (0, 0, 0))

IS_FROZEN = getattr(sys, "frozen", False)
if IS_FROZEN:
    DATA_DIR = os.path.join(APP_DIR, '..', '..')  # APP_DIR is "_internal"
else:
    DATA_DIR = os.path.join(APP_DIR, '..', '..', 'dist', 'wslX', 'data')

RC_FILE = os.path.join(DATA_DIR, 'xwin', 'etc', 'X11', 'system.XWinrc')
ICO_DIR = os.path.join(DATA_DIR, 'icons')

DEFAULT_ICO_DIR = os.path.join(DATA_DIR, 'default-icons')
DISTRO_ICO_DIR = os.path.join(DEFAULT_ICO_DIR, 'distros')

SHORTCUT_DIR = os.environ['APPDATA'] + r'\Microsoft\Windows\Start Menu\Programs\wslX'
if not os.path.isdir(SHORTCUT_DIR):
    os.mkdir(SHORTCUT_DIR)

SHELL_SHORTCUT_DIR = os.path.join(SHORTCUT_DIR, 'Shell')
if not os.path.isdir(SHELL_SHORTCUT_DIR):
    os.mkdir(SHELL_SHORTCUT_DIR)

DESKTOP_SHORTCUT_DIR = os.path.join(SHORTCUT_DIR, 'Desktop')
if not os.path.isdir(DESKTOP_SHORTCUT_DIR):
    os.mkdir(DESKTOP_SHORTCUT_DIR)

DESKTOP_EXE = os.path.realpath(os.path.join(DATA_DIR, 'DesktopSession.exe'))

DISTRO_ICONS = [f[:-4] for f in os.listdir(DISTRO_ICO_DIR)]

########################################
#
########################################
def make_ico(app_name, src_file):
    src_file = f'\\\\wsl.localhost\\{distro}' + src_file.replace('/', '\\')

    if src_file.lower().endswith('.svg'):
        svg_code = open(src_file, 'rt').read()
        image_data = svg2png(bytestring=svg_code, output_width=ICO_SIZE[0], output_height=ICO_SIZE[1])
        im_rgba = Image.open(io.BytesIO(image_data))
    else:
        im_rgba = Image.open(src_file).convert('RGBA')
        if im_rgba.size != ICO_SIZE:
            im_rgba = im_rgba.resize(ICO_SIZE)

    # To make Windows render the .ico decently, we have to force a black background
    mask = im_rgba.split()[-1]
    alpha_composite = Image.alpha_composite(BLACK_BG, im_rgba)
    alpha_composite.putalpha(mask)
    alpha_composite.save(os.path.join(ICO_DIR, app_name) + '.ico', bitmap_format='bmp')

########################################
#
########################################
def find_distro_icon(distro):
    for d in DISTRO_ICONS:
        if distro.lower().startswith(d):
            return d + '.ico'
    return 'linux.ico'

# Make sure wslx is running, otherwise find_linux_apps.py would fail
subprocess.run(os.path.join(DATA_DIR, '..', 'wslX.exe'))

if not os.path.isfile(RC_FILE):
    shutil.copyfile(os.path.join(DATA_DIR, 'XWinrc.default'), RC_FILE)

with open(RC_FILE, 'r') as f:
    rc_text = f.read()

########################################
# Remove autogen section
########################################
m = re.search(r'#<AUTOGEN>(.*)#</AUTOGEN>[\s]*\n', rc_text, re.IGNORECASE | re.MULTILINE | re.DOTALL)
if m:
    rc_text = rc_text[:m.start()] + rc_text[m.end():]

########################################
# Clear icon dir
########################################
if os.path.isdir(ICO_DIR):
    shutil.rmtree(ICO_DIR)  #, ignore_errors=True
os.mkdir(ICO_DIR)

shutil.copyfile(os.path.join(DEFAULT_ICO_DIR, 'pulseaudio.ico'), os.path.join(ICO_DIR, 'Start Pulseaudio.ico'))

menus = []
menu_data_open = {}

distros = subprocess.run('wsl --list -q', stdout=subprocess.PIPE, encoding='utf-16-le').stdout.rstrip().split('\n')
for distro in sorted(distros, key=str.casefold):
    print('Distro found:', distro)

    menu_data_open[distro] = {}

    distro_ico = os.path.join(DISTRO_ICO_DIR, find_distro_icon(distro))

    ########################################
    #
    ########################################
    print('Creating startmenu shortcuts...')

    lnk_file = os.path.join(SHELL_SHORTCUT_DIR, f'WSL-{distro}.lnk')
    command = f"$Shortcut=(New-Object -comObject WScript.Shell).CreateShortcut('{lnk_file}');$Shortcut.IconLocation='{distro_ico}';$Shortcut.TargetPath='C:\\Windows\\System32\\wsl.exe';$Shortcut.Arguments='-d {distro} --cd ~';$Shortcut.Save()"
    res = subprocess.run(f'powershell -NoProfile -Command "{command}"', stdout=subprocess.PIPE, stderr=subprocess.PIPE).stderr
    if res:
        print(res)

    lnk_file = os.path.join(DESKTOP_SHORTCUT_DIR, f'Desktop-{distro}.lnk')
    command = f"$Shortcut=(New-Object -comObject WScript.Shell).CreateShortcut('{lnk_file}');$Shortcut.IconLocation='{distro_ico}';$Shortcut.TargetPath='{DESKTOP_EXE}';$Shortcut.Arguments='{distro}';$Shortcut.Save()"
    res = subprocess.run(f'powershell -NoProfile -Command "{command}"', stdout=subprocess.PIPE, stderr=subprocess.PIPE).stderr
    if res:
        print(res)

    ########################################
    # Run find_linux_apps.py in Linux system (X server must be running)
    ########################################
    print('Searching desktop appplications...')

    tmp_file = f'\\\\wsl.localhost\\{distro}\\tmp\\data.py'
    if os.path.isfile(tmp_file):
        os.unlink(tmp_file)

    try:
        py = os.path.join(APP_DIR, 'find_linux_apps.py')
        py = '/mnt/' + py[0].lower() + py[2:].replace('\\', '/')

        os.environ['DISPLAY'] = ':0'
        os.environ['WSLENV'] = 'DISPLAY'

        res = subprocess.run(f'wsl.exe -d {distro} python3 "{py}"', stdout=subprocess.PIPE, stderr=subprocess.PIPE).stderr
        if res:
            print(res)

        # name -> command
        with open(tmp_file, 'r') as f:
            data = eval(f.read())

        ########################################
        # Copy distro ICO file
        ########################################
        shutil.copyfile(distro_ico, os.path.join(ICO_DIR, f'menu-{distro}.ico'))

        ########################################
        # Create category and app ICO files
        ########################################
        print('Creating icons...')

        # category icons
        for cat_name, icon in data['cat_icons'].items():
            try:
                make_ico('menu-' + cat_name, icon)
            except: pass

        # app icons
        for cat_name, app_dict in data['apps'].items():
            for app_name, row in app_dict.items():
                try:
                    app_name = app_name.replace('/', '_')  # "Add/Remove Software"
                    make_ico(app_name, row['icon'])
                except: pass

        ########################################
        # Create MENU entries for this distro
        ########################################
        print('Creating wsl/X menus...')

        for cat_name in sorted(CATEGORIES.keys()):
            if len(data['apps'][cat_name].keys()) == 0:
                continue
            menus.append(f'MENU {distro}_{cat_name} {{')

            menu_data_open[distro][cat_name] = {}

            for app_name in sorted(data['apps'][cat_name].keys(), key=str.casefold):
                exec_cmd = data['apps'][cat_name][app_name]['exec']

                if '%' in exec_cmd:
                    menu_data_open[distro][cat_name][app_name] = exec_cmd

                exec_cmd = exec_cmd.split(' ')[0]
                if exec_cmd.startswith('"'):
                    exec_cmd = exec_cmd[1:-1]
                app_name = app_name.replace('/', '_')
                menus.append(f'\t"{app_name}" EXEC "wsl.exe -d {distro} --exec {exec_cmd}"')

            if len(menu_data_open[distro][cat_name].keys()) == 0:
                del menu_data_open[distro][cat_name]

            menus.append('}')

        menus.append(f'MENU {distro} {{')
        for cat_name in sorted(CATEGORIES.keys()):
            if len(data['apps'][cat_name].keys()) == 0:
                continue
            menus.append(f'\t{cat_name} MENU {distro}_{cat_name}')
        menus.append('}')

    except Exception as e:
        print(e)

    print()

with open(os.path.join(DATA_DIR, 'menu_open.pson'), 'w') as f:
    f.write(str(menu_data_open))

########################################
# Update <DISTROS> section in root menu
########################################
m = re.search(r'#<DISTROS>(.*)#</DISTROS>[\s]*\n', rc_text, re.IGNORECASE | re.MULTILINE | re.DOTALL)
if m:
    rc_text = rc_text[:m.start()] + '#<DISTROS>\n' + '\n'.join([f'\t"{distro}" MENU {distro}' for distro in sorted(distros, key=str.casefold)]) + '\n#</DISTROS>\n' + rc_text[m.end():]
else:
    # Find name of rootmenu
    m = re.search(r'\nROOTMENU[\s]+([^\s]+)[\s]*\n', rc_text, re.IGNORECASE)
    if m:
        rootmenu_name = m.group(1)
        # Find "MENU<ws>root<ws>{"
        m = re.search(r'MENU[\s]+' + rootmenu_name + r'[\s]+{[\s]*\n', rc_text, re.IGNORECASE)
        if m:
            root_menu_start = m.end()
            rc_text = rc_text[:root_menu_start] + '#<DISTROS>\n' + '\n'.join([f'\t"{distro}" MENU {distro}\n' for distro in distros]) + '#</DISTROS>\n' + rc_text[root_menu_start:]

with open(RC_FILE, 'w') as f:
    f.write('#<AUTOGEN>\n')
    f.write('\n'.join(menus) + '\n')
    f.write('#</AUTOGEN>\n')
    f.write(rc_text)

print('Reloading .XWinrc file...\n')

# Tell xwin to reload RC_FILE
user32.SendMessageW(user32.FindWindowW('wslx', None), WM_RELOAD_PREFS, 0, 0)

print('Done.')
