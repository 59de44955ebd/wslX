#include <wchar.h>
#include "X11/Xdefs.h" // for Bool type

Bool isLnkFileW(const LPWSTR lpszFile);
Bool resolveLnkW(const LPWSTR lpszLinkFile, const LPWSTR lpszPath);
