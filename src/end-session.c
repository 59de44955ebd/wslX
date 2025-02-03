#include <windows.h>

#define WINDOW_CLASS_X_MSG      "wslx X msg"

int CALLBACK WinMainCRTStartup()
{
	HWND hwnd_msg = FindWindowA(WINDOW_CLASS_X_MSG, NULL);
	if (hwnd_msg)
		SendMessageA(hwnd_msg, WM_ENDSESSION, 1, 0);
	return 0;
}
