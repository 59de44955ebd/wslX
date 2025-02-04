#include "winlog.h"

void WinLog(const char *buf)
{
	OutputDebugString(buf);
}
