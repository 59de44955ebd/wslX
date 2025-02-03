//#include <windows.h>
#include "dark.h"

BOOL g_isDark = FALSE;

BOOL regCheckDark(void)
{
	BOOL isDark = FALSE;
    HKEY hkey;
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize" , &hkey) == ERROR_SUCCESS)
    {
        DWORD dwData;
        DWORD dwBufSize = sizeof(DWORD);
        if (RegQueryValueExW(hkey, L"AppsUseLightTheme", 0, 0, (LPBYTE)&dwData, &dwBufSize) == ERROR_SUCCESS)
        	isDark = dwData == 0;
        RegCloseKey(hkey);
	}
	return isDark;
}

void setAppModeDark(void)
{
	fnSetPreferredAppMode SetPreferredAppMode;
	HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
	SetPreferredAppMode(ForceDark);
	//FlushMenuThemes();
	FreeLibrary(hUxtheme);
}
