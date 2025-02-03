#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>

#include "lnk.h"

const GUID IID_IShellLinkW =  {0x000214F9, 0x0000, 0x0000,{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID IID_IPERSISTFILE = {0x0000010B, 0x0000, 0x0000,{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

Bool isLnkFileW(const LPWSTR lpszFile)
{
	LPWSTR str = wcsrchr(lpszFile, L'.');
	if (str != NULL)
		return wcscmp(str, L".LNK") == 0 || wcscmp(str, L".lnk") == 0;
	return FALSE;
}

Bool resolveLnkW(const LPWSTR lpszLinkFile, const LPWSTR lpszPath)
{
    BOOL ok = FALSE;
    IShellLinkW * psl;
    WCHAR szGotPath[MAX_PATH];
    WCHAR szDescription[MAX_PATH];
    WIN32_FIND_DATAW wfd;
    *lpszPath = 0;

	if (SUCCEEDED(CoInitialize(NULL)))
	{
	    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize has already been called.
	    if (SUCCEEDED(CoCreateInstance((const CLSID *)&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, (const IID *)&IID_IShellLinkW, (LPVOID*)&psl)))
	    {
	        IPersistFile* ppf;
	        if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, (const IID *)&IID_IPersistFile, (void**)&ppf)))
	        {
	            // Load the shortcut.
	            if (SUCCEEDED(ppf->lpVtbl->Load(ppf, lpszLinkFile, STGM_READ)))
	            {
	                // Resolve the link.
	                if (SUCCEEDED(psl->lpVtbl->Resolve(psl, NULL, 0)))  // HWND
	                {
	                    // Get the path to the link target.
	                    if (SUCCEEDED(psl->lpVtbl->GetPath(psl, szGotPath, MAX_PATH, (WIN32_FIND_DATAW*)&wfd, 0)))
	                    {
	                        // Get the description of the target.
	                        if (SUCCEEDED(psl->lpVtbl->GetDescription(psl, szDescription, MAX_PATH)))
	                        {
	                            ok = wcscpy(lpszPath, szGotPath) != NULL;
	                        }
	                    }
	                }
	            }
	            // Release the pointer to the IPersistFile interface.
	            ppf->lpVtbl->Release(ppf);
	        }
	        // Release the pointer to the IShellLink interface.
	        psl->lpVtbl->Release(psl);
	    }
	    CoUninitialize();
	}
    return ok;
}
