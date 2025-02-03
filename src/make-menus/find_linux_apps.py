#!/usr/bin/env python3

import configparser
import gi
import os
from pathlib import Path

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

CATEGORIES = {
    'TextEditor': 'accessories-text-editor',
    'Graphics': 'applications-graphics',
    'Settings': 'preferences-system',
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

ICON_SIZE = 16

APPS_DIR = '/usr/share/applications'

# Only works if X server is running! Otherwise None
ICON_THEME = Gtk.IconTheme.get_default()

def resolve_icon_path(icon_name):
    "This takes a freedesktop.org icon name and prints the GTK 3.0 resolved file location."

    for resolution in [16, 20, 22, 24, 28, 32, 36, 48, 64, 72, 96, 128, 192, 256]:
        if resolution < ICON_SIZE:
            continue
        icon_file = ICON_THEME.lookup_icon(icon_name, resolution, 0)
        if icon_file:
            file_path = str(icon_file.get_filename())
            return str(Path(file_path).resolve())

app_data = {'cat_icons': {}, 'apps': {}}

for cat, icon in CATEGORIES.items():
    icon_file = resolve_icon_path(icon)
    if icon_file:
        app_data['cat_icons'][cat] = icon_file

for cat in CATEGORIES.keys():
    app_data['apps'][cat] = {}

icons = {}

for filename in os.listdir(APPS_DIR):
    if not filename.lower().endswith('.desktop'):
        continue
    try:
        config = configparser.ConfigParser(interpolation=None)
        config.read(os.path.join(APPS_DIR, filename), encoding='utf-8')
        if not 'Desktop Entry' in config:
            continue
        data = config['Desktop Entry']
        if 'NoDisplay' in data and data['NoDisplay'] == 'true':
            continue
        if 'Terminal' in data and data['Terminal'] == 'true':
            continue
        if not 'Icon' in data or not 'Categories' in data:
            continue

        categories = data['Categories'].split(';')
        cat_found = 'Other'  # None
        for cat in CATEGORIES.keys():
            if cat in categories:
                cat_found = cat
                break

        # No need to resolve same icon twice
        if data['Icon'] not in icons:
            icon_file = resolve_icon_path(data['Icon'])
            if icon_file is None:
                continue
            icons[data['Icon']] = icon_file

        app_data['apps'][cat_found][data['Name']] = {
            'exec': data['Exec'],  #.split(' ')[0],
            'icon': icons[data['Icon']]
        }
    except:
        pass

with open('/tmp/data.py', 'w') as f:
    f.write(str(app_data))
