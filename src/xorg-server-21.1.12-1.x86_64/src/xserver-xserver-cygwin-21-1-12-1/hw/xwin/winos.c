/*
 * Copyright (c) 2010-2014 Colin Harrison All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 *
 * Author: Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

static USHORT WowNativeMachine (void)
{
  typedef WINBOOL (WINAPI *PFNISWOW64PROCESS2)(HANDLE, USHORT *, USHORT *);
  PFNISWOW64PROCESS2 pfnIsWow64Process2 = (PFNISWOW64PROCESS2)GetProcAddress(GetModuleHandle("kernel32"), "IsWow64Process2");

  typedef WINBOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE, PBOOL);
  PFNISWOW64PROCESS pfnIsWow64Process = (PFNISWOW64PROCESS)GetProcAddress(GetModuleHandle("kernel32"), "IsWow64Process");

  USHORT processMachine, nativeMachine;
  if ((pfnIsWow64Process2) &&
      (pfnIsWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine)))
    return nativeMachine;
  else if (pfnIsWow64Process) {
#ifdef _X86_
    WINBOOL bIsWow64 = FALSE;
    if (pfnIsWow64Process(GetCurrentProcess(), &bIsWow64))
      return bIsWow64 ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386;
#endif
  }

#ifdef __x86_64__
  return IMAGE_FILE_MACHINE_AMD64;
#else
  return IMAGE_FILE_MACHINE_I386;
#endif
}

static char *machine_name(USHORT machine)
{
  const char *result;

  switch (machine)
      {
      case IMAGE_FILE_MACHINE_I386:
          result = "x86";
          break;
      case IMAGE_FILE_MACHINE_AMD64:
          result = "x64";
          break;
      case IMAGE_FILE_MACHINE_ARM64:
          result = "ARM64";
          break;
      default:
          {
              static char buf[8];
              snprintf(buf, sizeof(buf), "%x", machine);
              result = buf;
          }
      }
  return strdup(result);
}

static const char* arch_wow(void)
{
  static char arch[32];

#ifdef __x86_64__
  USHORT processMachine = IMAGE_FILE_MACHINE_AMD64;
#else
  USHORT processMachine = IMAGE_FILE_MACHINE_I386;
#endif

  USHORT nativeMachine = WowNativeMachine();

  char *processMachineName = machine_name(processMachine);
  char *nativeMachineName = machine_name(nativeMachine);

  if (processMachine != nativeMachine)
      sprintf(arch, "%s-on-%s", processMachineName, nativeMachineName);
  else
      sprintf(arch, "%s", processMachineName);

  free(nativeMachineName);
  free(processMachineName);

  return arch;
}

/*
 * Report the OS version
 */

void
winOS(void)
{
    OSVERSIONINFOEX osvi = {0};
    const char *windowstype = "Unknown";
    const char *prodName = "Unknown";

    /* Get operating system version information */
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((LPOSVERSIONINFO)&osvi);

    /* Branch on platform ID */
    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
        windowstype = "Windows NT";

        if (osvi.dwMajorVersion <= 4)
            prodName = "Windows NT";
        else if (osvi.dwMajorVersion == 10) {
            if (osvi.dwMinorVersion == 0) {
                if (osvi.dwMinorVersion < 22000)
                    {
                        if (osvi.wProductType == VER_NT_WORKSTATION)
                            prodName = "Windows 10";
                        else
                            prodName = "Windows 10 Server";
                    }
                else
                    {
                        if (osvi.wProductType == VER_NT_WORKSTATION)
                            prodName = "Windows 11";
                        else
                            prodName = "Windows 11 Server";
                    }
            }
        }
        else if (osvi.dwMajorVersion == 6) {
            if (osvi.dwMinorVersion == 4) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 10";
                else
                    prodName = "Windows Server 2016";
            }
            if (osvi.dwMinorVersion == 3) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 8.1";
                else
                    prodName = "Windows Server 2012 R2";
            }
            if (osvi.dwMinorVersion == 2) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 8";
                else
                    prodName = "Windows Server 2012";
            }
            else if (osvi.dwMinorVersion == 1) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows 7";
                else
                    prodName = "Windows Server 2008 R2";
            }
            else if (osvi.dwMinorVersion == 0) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows Vista";
                else
                    prodName = "Windows Server 2008";
            }
        }
        else if (osvi.dwMajorVersion == 5) {
            if (osvi.dwMinorVersion == 2) {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                    prodName = "Windows XP x64 Edition";
                else if (GetSystemMetrics(SM_SERVERR2))
                    prodName = "Windows Server 2003 R2";
                else
                    prodName = "Windows Server 2003";
            }
            else if (osvi.dwMinorVersion == 1)
                prodName = "Windows XP";
            else if (osvi.dwMinorVersion == 0) {
                prodName = "Windows 2000";
                break;
            }
        }

        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        windowstype = "Windows";
        break;
    }

    ErrorF("OS: %s %s [%s %d.%d build %d] %s\n",
           prodName, osvi.szCSDVersion,
           windowstype, (int)osvi.dwMajorVersion, (int)osvi.dwMinorVersion,
           (int)osvi.dwBuildNumber, arch_wow());
}
