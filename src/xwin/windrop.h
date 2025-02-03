//#include <xcb/xcb.h>

#include <limits.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_aux.h>
#include <xcb/composite.h>

#include <X11/Xwindows.h>

#include "X11/Xdefs.h" // for Bool type

#include <wchar.h>

//#include <stdio.h>
//#define DBGI(x) {char dbg[32];sprintf(dbg,"%ld",(size_t)x);OutputDebugStringA(dbg);}

////#include "window.h"
//
//#include "winglobals.h"
//#include "winmsg.h"

#define XDND_PROTOCOL_VERSION 5
#define MAX_FILES_PER_DROP 10
// "file:///mnt/" + "\r\n" => 14
#define MAX_LEN_URILIST MAX_FILES_PER_DROP * (MAX_PATH + 14) + 1
//#define MAX_LEN_TITLE 260


extern xcb_atom_t
	g_atmDropEvent,
	g_atmDropEventFullscreen, // new

	g_atmDndAware,
	g_atmDndEnter,
	g_atmDndPosition,
	g_atmDndActionCopy,
	g_atmDndDrop,
	g_atmDndSelection,
	//g_atmDndLeave,

	g_atmUriListType,
	g_atmGnomeCopiedFilesType;

//extern char g_uriList[MAX_LEN_URILIST];
//extern int g_dropX, g_dropY;

extern char g_uriList[MAX_LEN_URILIST];
extern int g_dropX, g_dropY;

void dndSendSelectionNotify(
	xcb_selection_request_event_t *selection_request
);

//void dndSendDropMessage(HWND hWnd);
void dndSendDropMessage(
	//xcb_connection_t * g_conn,
	HWND hWnd
);

void dndSendDropMessageFullScreen(void);

void dndDoDrop(void);
void dndDoDropFullScreen(void);

//void dndDoDrop(
//	xcb_connection_t * conn,
//	xcb_atom_t atmPrivMap,
//	xcb_window_t root_window_id,
//	HWND DropWindowHwnd
//);

xcb_window_t dndGetActiveWindow(void);
xcb_window_t dndFindWindow(void);

Bool dndHdropToUriList(HDROP pData);

Bool urldecode(
	const char *string,
	int length,
	char **ostring,
	int *olen
);
