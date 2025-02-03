/*

[run]
file=
parameters=
verb=
directory=
show=1
priority=0

tray=0
trayexitstring=Exit
traytooltip=Run
trayaboutstring=About
trayaboutmessage=

[env]
...

*/

#include "resource.h"

#include <windows.h>
#include <shlwapi.h>
#include <wchar.h>

//#include <string>
//#include <stdlib.h>

//#include <cstring>
//#include <cstdlib>



#define MSVCRTLIGHT
#define DARK_MENU

#ifdef DARK_MENU
enum mode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};
typedef enum mode PreferredAppMode;
typedef PreferredAppMode(*fnSetPreferredAppMode)(PreferredAppMode appMode);
#endif

//-------------------------------------------------------------------- CONSTANTS

#define MAX_KEY_LEN		64
#define MAX_VAL_LEN 	2048
#define MAX_VERB_LEN	64

#define TRAYICONID 1 // ID number for the Notify Icon
#define WM_SWM_TRAYMSG	WM_APP  // the message ID sent to our window
#define WM_SWM_EXIT WM_APP + 1 // close the window
#define WM_SWM_INFO WM_APP + 2 // show info dialog

//-------------------------------------------------------------------- GLOBALS

static wchar_t szTrayAboutString[MAX_VAL_LEN] = L"";
static wchar_t szTrayAboutMsg[MAX_VAL_LEN] = L"";
static wchar_t szTrayExitString[MAX_VAL_LEN] = L"";

static SHELLEXECUTEINFO ExecInfo;
static HMENU hMenu = NULL;

//-------------------------------------------------------------------- FUNCTIONS
void ExpandVars(wchar_t *src)
{
	wchar_t dest[MAX_VAL_LEN];
	ExpandEnvironmentStrings(src, dest, MAX_VAL_LEN);
	wcscpy_s(src, MAX_VAL_LEN, dest);
}

void WinPathToLinux(wchar_t *win_path_input, wchar_t *linux_path)
{
	wchar_t * ptr;

	// If path starts with "\\wsl.localhost\", we remove "\\wsl.localhost\<distro>" and
	// replace all backslashes with slashes.
	// This will NOT work if two different distros are involved.
	if (wcsncmp(win_path_input, L"\\\\wsl.localhost\\", 16) == 0)
	{
		// omit "\\wsl.localhost\<distro>\"
		ptr = (wchar_t *)win_path_input + 16;
		ptr = wcschr(ptr, L'\\');
		wcscpy_s(linux_path, MAX_PATH, ptr);
		// replace \ with /
		ptr = (wchar_t *)linux_path;
		while ((ptr = wcschr(ptr, L'\\')) != NULL)
		{
			*ptr++ = L'/';
		}
		return;
	}

	// If path starts with "\\", we convert it like this:
	// \\192.168.2.12\test\test.txt  =>  /mnt/192.168.2.12/test/test.txt
	// This will ONLY work if the share was previously mounted this way.
	else if (wcsncmp(win_path_input, L"\\\\", 2) == 0)
	{
		wcscpy_s(linux_path, MAX_PATH, L"/mnt/");
		ptr = (wchar_t *)win_path_input + 2;
		wcscat_s(linux_path, MAX_PATH, ptr);

		ptr = (wchar_t *)linux_path + 5;
		while ((ptr = wcschr(ptr, L'\\')) != NULL)
		{
			*ptr++ = L'/';
		}
		return;
	}

	wchar_t win_path[MAX_PATH];

	// If it's not an absolute local path, try to convert it to an absolute path.
	if (wcslen(win_path_input) < 2 || win_path_input[1] != L':')
		_wfullpath(win_path, win_path_input, MAX_PATH);
	else
		wcscpy_s(win_path, MAX_PATH, win_path_input);

	BOOL has_spaces = wcschr(win_path, L' ') != NULL;

	wcscpy_s(linux_path, MAX_PATH, has_spaces ? L"\"" : L"");

	wcscat_s(linux_path, MAX_PATH, L"/mnt/");

	// append drive letter in lower case
	wchar_t drive[] = L" ";
	drive[0] = towlower(win_path[0]);
	wcscat_s(linux_path, MAX_PATH, drive);

	// append remaining path after ":"
	ptr = (wchar_t *)win_path + 2;
	wcscat_s(linux_path, MAX_PATH, ptr);

	// replace backslashes with slashes
	ptr = (wchar_t *)linux_path; // +6; // "/mnt/c"
	while ((ptr = wcschr(ptr, L'\\')) != NULL)
	{
		*ptr++ = L'/';
	}

	if (has_spaces)
		wcscat_s(linux_path, MAX_PATH, L"\"");
}

void ShowContextMenu(HWND hWnd)
{
	if (hMenu)
	{
		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	}
}

INT_PTR CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (msg)
	{
	case WM_INITDIALOG:
#ifdef DARK_MENU
	{
		fnSetPreferredAppMode SetPreferredAppMode;
		HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
		SetPreferredAppMode(ForceDark);
		FreeLibrary(hUxtheme);
	}
#endif
	break;

	case WM_SWM_TRAYMSG:
		switch (lParam)
		{
		case WM_RBUTTONUP: //WM_RBUTTONDOWN
		case WM_CONTEXTMENU:
			ShowContextMenu(hwnd);
		}
		break;

	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case WM_SWM_EXIT:
			PostQuitMessage(0);
			break;

		case WM_SWM_INFO:
			ShowWindow(hwnd, SW_SHOW);
			break;

		case IDC_BUTTON1:
			ShowWindow(hwnd, SW_HIDE);
			break;
		}
		return 1;

	case WM_CLOSE:
		ShowWindow(hwnd, SW_HIDE);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

//###########################################################################
// WinMain
//###########################################################################

#ifdef MSVCRTLIGHT
int CALLBACK WinMainCRTStartup()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	//wchar_t * lpCmdLine = GetCommandLine();
	//wchar_t * ptr;

	//if (lpCmdLine[0] == '"')
	//{
	//	ptr = wcschr(lpCmdLine + 1, L'"');
	//	if (ptr)
	//	{
	//		ptr++;
	//		while (iswspace(*ptr))
	//			ptr++;
	//		lpCmdLine = ptr;
	//	}
	//	else
	//		lpCmdLine = L"";
	//}
	//else
	//{
	//	ptr = wcschr(lpCmdLine, L' ');
	//	if (ptr)
	//	{
	//		while (iswspace(*ptr))
	//			ptr++;
	//		lpCmdLine = ptr;
	//	}
	//	else
	//		lpCmdLine = L"";
	//}

#else
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR lpCmdLine, int nCmdShow)
{
#endif

	wchar_t szIniFile[MAX_PATH];

	wchar_t szFile[MAX_PATH] = L"";				// Command to run
	wchar_t szParameters[MAX_VAL_LEN] = L"";	// Parameters
	wchar_t szVerb[MAX_VERB_LEN] = L""; 		// Verb (open, explore, print, runas, ...)
	wchar_t szDirectory[MAX_PATH];				// CWD
	int nShowCmd = SW_SHOWNORMAL;
	int nPriority = 0;
	wchar_t szEnvVars[MAX_VAL_LEN] = L"";

	BOOL bTrayMode = FALSE;
	wchar_t szTrayToolTip[MAX_PATH] = L"";
	NOTIFYICONDATA niData;

	// Build the name of the INI file
	GetModuleFileName(NULL, szIniFile, MAX_PATH);
	PathRemoveExtension(szIniFile);
	wcscat_s(szIniFile, MAX_PATH, L".ini");

	// Read data from the INI file
	nShowCmd = GetPrivateProfileInt(L"run", L"show", SW_SHOWNORMAL, szIniFile);
	nPriority = GetPrivateProfileInt(L"run", L"priority", 0, szIniFile);

	// Set env var %_CD_% to directory of exe
	GetModuleFileName(NULL, szEnvVars, MAX_PATH);
	if (wcsrchr(szEnvVars, L'\\'))
		*(wcsrchr(szEnvVars, L'\\')) = L'\0';
	SetEnvironmentVariable(L"_CD_", szEnvVars);

	// Set env var %_CL_% to commandline passed to exe
	//SetEnvironmentVariable(L"_CL_", lpCmdLine);

	//BOOL bConvertToLinux = GetPrivateProfileInt(L"run", L"linux", FALSE, szIniFile);
	int nConvertToLinux = GetPrivateProfileInt(L"run", L"linux", -1, szIniFile);

	LPWSTR *szArglist;
	wchar_t varname[2];
	int nArgs;
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (szArglist)
	{
		for (int i = 1; i < nArgs; i++)
		{
			_itow_s(i, varname, 2, 10);
			//if (bConvertToLinux)
			if (i == nConvertToLinux)
			{

				wchar_t linux_path[MAX_PATH + 4]; // C:\... 0 => /mnt/c/...
				WinPathToLinux(szArglist[i], linux_path);
				SetEnvironmentVariable(varname, linux_path);
			}
			else
				SetEnvironmentVariable(varname, szArglist[i]);
		}
		LocalFree(szArglist);
	}
	else
		nArgs = 0;
	for (int i = nArgs; i < 10; i++)
	{
		_itow_s(i, varname, 2, 10);
		SetEnvironmentVariable(varname, L"");
	}

	// env
	wchar_t szEnvAllVars[MAX_VAL_LEN] = L"";
	if (GetPrivateProfileSection(L"env", szEnvAllVars, MAX_VAL_LEN, szIniFile))
	{
		wchar_t szKey[MAX_KEY_LEN] = L"";
		wchar_t szValue[MAX_VAL_LEN] = L"";
		size_t lineLen;

		wchar_t * pt;
		wchar_t * line = wcstok_s(szEnvAllVars, L"\0", &pt);
		do
		{
			lineLen = wcslen(line);
			const wchar_t * pos = wcschr(line, L'=');
			wcsncpy_s(szKey, _countof(szKey), line, pos - line);
			wcsncpy_s(szValue, _countof(szValue), pos + 1, lineLen - (pos - line));
			ExpandVars(szValue);
			SetEnvironmentVariable(szKey, szValue);
			line = wcstok_s(line + wcslen(line) + 1, L"\0", &pt);
		}
		while (line != NULL);
	}

	// file
	GetPrivateProfileString(L"run", L"file", L"", szFile, MAX_PATH, szIniFile);
	if (szFile[0])
		ExpandVars(szFile);

	// parameters
	GetPrivateProfileString(L"run", L"parameters", L"", szParameters, MAX_VAL_LEN, szIniFile);
	if (szParameters[0])
		ExpandVars(szParameters);

	// verb
	GetPrivateProfileString(L"run", L"verb", L"", szVerb, MAX_VERB_LEN, szIniFile);

	// directory
	GetPrivateProfileString(L"run", L"directory", L"", szDirectory, MAX_VAL_LEN, szIniFile);
	if (szDirectory[0])
		ExpandVars(szDirectory);

	// tray
	bTrayMode = GetPrivateProfileInt(L"run", L"tray", FALSE, szIniFile);
	if (bTrayMode)
	{
		// trayexitstring
		GetPrivateProfileString(L"run", L"trayexitstring", L"Exit", szTrayExitString, MAX_VAL_LEN, szIniFile);

		// traytooltip
		GetPrivateProfileString(L"run", L"traytooltip", L"", szTrayToolTip, MAX_PATH, szIniFile);
		if (szTrayToolTip[0] == 0)
		{
			GetModuleFileName(NULL, szTrayToolTip, MAX_PATH);
			PathStripPath(szTrayToolTip);
			PathRemoveExtension(szTrayToolTip);
		}

		// trayaboutstring
		GetPrivateProfileString(L"run", L"trayaboutstring", L"About", szTrayAboutString, MAX_VAL_LEN, szIniFile);

		// trayaboutmessage
		GetPrivateProfileString(L"run", L"trayaboutmessage", L"", szTrayAboutMsg, MAX_VAL_LEN, szIniFile);

		hMenu = CreatePopupMenu();
		if (szTrayAboutMsg[0])
		{
			// Replace "\n" with CRLF
			wchar_t * ptr = szTrayAboutMsg;
			while (*ptr != 0)
			{
				if (*ptr == L'\\' && *(ptr + 1) == L'n')
				{
					*ptr = 13;
					ptr++;
					*ptr = 10;
				}
				ptr++;
			}

			InsertMenu(hMenu, -1, MF_BYPOSITION, WM_SWM_INFO, szTrayAboutString);
			InsertMenu(hMenu, -1, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
		}

		InsertMenu(hMenu, -1, MF_BYPOSITION, WM_SWM_EXIT, szTrayExitString);
	}

	ExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ExecInfo.hwnd = NULL;
	ExecInfo.lpFile = szFile;
	ExecInfo.lpParameters = szParameters;
	ExecInfo.lpVerb = szVerb[0] ? szVerb : NULL;
	ExecInfo.nShow = nShowCmd;
	if (szDirectory[0])
		ExecInfo.lpDirectory = szDirectory;

	// SEE_MASK_NOCLOSEPROCESS: Use to indicate that the hProcess member receives the process handle.
	ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (!ShellExecuteEx(&ExecInfo))
		return 0;

	if (ExecInfo.hProcess && nPriority)
	{
		SetPriorityClass(ExecInfo.hProcess, nPriority);
	}

	if (bTrayMode)
	{
		ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

		niData.cbSize = sizeof(NOTIFYICONDATA);

		// The ID number can be anything you choose
		niData.uID = TRAYICONID;

		// State which structure members are valid
		niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

		// Load the icon
		niData.hIcon = (HICON)LoadImage(
			hInstance,
			MAKEINTRESOURCE(IDI_ICON1),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR
		);

		// Create a dialog
		niData.hWnd = CreateDialog(
			hInstance,
			MAKEINTRESOURCE(IDD_DLG_DIALOG),
			NULL,
			WndProc
		);
		SetDlgItemText(niData.hWnd, IDC_STATIC1, szTrayAboutMsg);

		niData.uCallbackMessage = WM_SWM_TRAYMSG;

		lstrcpyn(niData.szTip, szTrayToolTip, sizeof(niData.szTip));

		Shell_NotifyIcon(NIM_ADD, &niData);

		// Free icon handle
		if (niData.hIcon && DestroyIcon(niData.hIcon))
			niData.hIcon = NULL;

		// The Message Loop
		MSG Msg;
		while (GetMessage(&Msg, NULL, 0, 0) > 0)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}

		// Kill the process
		TerminateProcess(ExecInfo.hProcess, 0);
		CloseHandle(ExecInfo.hProcess);

		// Clean up
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		DestroyWindow(niData.hWnd);
		if (hMenu)
			DestroyMenu(hMenu);
	}
	else
	{
		CloseHandle(ExecInfo.hProcess);
	}
	ExitProcess(0);
	return 0;
}
