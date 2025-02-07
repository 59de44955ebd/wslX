#include "windrop.h"
#include <stdio.h>

#include "lnk.h"

extern Bool g_fClipboardStarted; // set to TRUE in winClipboardThreadProc (winclipboardinit.c)

extern xcb_window_t g_ClipWindow; // set in winClipboardProc (winclipboard\thread.c)

// set in winClipboardProc (winclipboard/thread.c)
xcb_atom_t
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


char g_uriList[MAX_LEN_URILIST];		// filed in WM_DROPFILES (winmultiwindowwndproc.c)
int g_dropX, g_dropY;  					// filed in WM_DROPFILES (winmultiwindowwndproc.c)

// local, set in dndSendDropMessage
static HWND g_DropWindowHwnd;

//xcb_window_t g_root_window_id; 	// set in winMultiWindowXMsgProc (winmultiwindowwm.c)
//xcb_connection_t * g_conn; 		// set in winMultiWindowXMsgProc (winmultiwindowwm.c)

xcb_window_t g_root_window_id; 	// set in winClipboardProc (winclipboard\thread.c)
xcb_connection_t * g_conn; 		// set in winClipboardProc (winclipboard\thread.c)

xcb_atom_t g_atmPrivMap;		// set in winInitMultiWindowWM (winmultiwindowwm.c)


// Finds top level window with given HWND
static Bool dndWindowFromHWND(
	xcb_connection_t * conn,
	xcb_atom_t atmPrivMap,
	xcb_window_t parent_window,
	HWND hWnd,
	xcb_window_t * target_window
)
{
	xcb_query_tree_cookie_t cookie;
	xcb_query_tree_reply_t * reply;

    xcb_get_property_cookie_t cookie_child;
    xcb_get_property_reply_t * reply_child;

	cookie = xcb_query_tree(conn, parent_window);

	if ((reply = xcb_query_tree_reply(conn, cookie, NULL)))
	{
		xcb_window_t *children = xcb_query_tree_children(reply);
		for (int i = 0; i < xcb_query_tree_children_length(reply); i++)
		{
		    cookie_child = xcb_get_property(conn, FALSE, children[i], atmPrivMap,
		                              XCB_ATOM_INTEGER, 0L, sizeof(HWND)/4L);
		    reply_child = xcb_get_property_reply(conn, cookie_child, NULL);
		    if (reply_child)
		    {
		        int length = xcb_get_property_value_length(reply_child);
		        HWND *value = xcb_get_property_value(reply_child);
		        if (value && (length == sizeof(HWND)) && *value == hWnd)
		        {
		            *target_window = children[i];
		            free(reply_child);
		            free(reply);
		            return TRUE;
		        }
		        free(reply_child);
		    }
		}
		free(reply);
	}
    return FALSE;
}

// This checks if the supplied window has the XdndAware property
static uint32_t dndGetAwareProperty(
	xcb_connection_t * conn,
	xcb_window_t win
)
{
	uint32_t protocol_version = 0;
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *reply;
    cookie = xcb_get_property(
    	conn,
    	FALSE,
    	win,
		g_atmDndAware,
		XCB_GET_PROPERTY_TYPE_ANY,
		0,
		INT_MAX
	);
    reply = xcb_get_property_reply(conn, cookie, NULL);
    if (reply && (reply->type != XCB_NONE))
    {
        protocol_version = *((uint32_t *)xcb_get_property_value(reply));
        free(reply);
    }
    return protocol_version;
}

// This sends the XdndEnter message which initiates the XDND protocol exchange
static void dndSendEnter(
	xcb_connection_t * conn,
	int xdndVersion,
	xcb_window_t source,
	xcb_window_t target
)
{
    xcb_client_message_event_t e;
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = target;
    e.type = g_atmDndEnter;
    e.format = 32;
    e.data.data32[0] = source;
    e.data.data32[1] = xdndVersion << 24;
	e.data.data32[2] = g_atmUriListType;
    xcb_send_event(conn, FALSE, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&e);
}

// This sends the XdndPosition message
static void dndSendPosition(
	xcb_connection_t * conn,
	xcb_window_t source,
	xcb_window_t target,
	int time,
	int p_rootX,
	int p_rootY
)
{
    xcb_client_message_event_t e;
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = target;
    e.type = g_atmDndPosition;
    e.format = 32;
    e.data.data32[0] = source;
//    e.data.data32[1] reserved
	e.data.data32[2] = p_rootX << 16 | p_rootY;
	e.data.data32[3] = time;
	e.data.data32[4] = g_atmDndActionCopy;
    xcb_send_event(conn, FALSE, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&e);
}

// This is sent by the source to the target to say it can call XConvertSelection
static void dndSendDrop(
	xcb_connection_t * conn,
	xcb_window_t source,
	xcb_window_t target,
	time_t xdndLastPositionTimestamp
)
{
    xcb_client_message_event_t e;
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = target;
    e.type = g_atmDndDrop;
    e.format = 32;
    e.data.data32[0] = source;
//    e.data.data32[1] reserved
	e.data.data32[2] = xdndLastPositionTimestamp;
    xcb_send_event(conn, FALSE, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&e);
}

//void get_name(xcb_connection_t* c, xcb_window_t window, char** name, int* length)
//{
//    *length = 0;
//    *name = NULL;
//
//    auto cookie = xcb_get_property(c, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 0);
//    auto reply = xcb_get_property_reply(c, cookie, NULL);
//    if (reply)
//    {
//        *length = xcb_get_property_value_length(reply);
//        *name = (char*)xcb_get_property_value(reply);
//
//        free(reply);
//    }
//}

// called from XCB_CLIENT_MESSAGE in xevents.c
void dndDoDrop(void)
{
	//xcb_window_t drop_src_window = g_root_window_id;
	xcb_window_t drop_src_window = g_ClipWindow;

    // Find target_window
	xcb_window_t target_window;

	if (!dndWindowFromHWND(g_conn, g_atmPrivMap, g_root_window_id, g_DropWindowHwnd, &target_window))
	{
		//ErrorF("do_drop: target_window with HWND %ld not found", (long)g_DropWindowHwnd);
		return;
	}

	// Check target_window supports XDND
	uint32_t xdndVersion = dndGetAwareProperty(g_conn, target_window);

	if (xdndVersion == 0)
		return;

	// Claim ownership of Xdnd selection
	xcb_set_selection_owner(
		g_conn,
		drop_src_window,
		g_atmDndSelection,
		XCB_CURRENT_TIME
	);

	// Send XdndEnter message
	dndSendEnter(
		g_conn,
		xdndVersion,
		drop_src_window,
		target_window
	);

	// Send XdndPosition message
	dndSendPosition(
		g_conn,
		drop_src_window,
		target_window,
		XCB_CURRENT_TIME,
		g_dropX, g_dropY
	);

	// Send XdndDrop message
	dndSendDrop(
		g_conn,
		drop_src_window,
		target_window,
		XCB_CURRENT_TIME //Time xdndLastPositionTimestamp
	);

	xcb_flush(g_conn);
}

// called by winClipboardFlushXEvents (winclipboard\xevents.c)
void dndDoDropFullScreen(void)
{
	xcb_window_t target_window = dndFindWindow();
	if (!target_window)
		return;

	//xcb_window_t drop_src_window = g_root_window_id;
	xcb_window_t drop_src_window = g_ClipWindow;

	// Check target_window supports XDND
	uint32_t xdndVersion = dndGetAwareProperty(g_conn, target_window);

//	if (xdndVersion == 0)
//	{
//		OutputDebugString("FAILURE: target not drop aware");
//		return;
//	}

	// Claim ownership of Xdnd selection
	xcb_set_selection_owner(
		g_conn,
		drop_src_window,
		g_atmDndSelection,
		XCB_CURRENT_TIME
	);

	// Send XdndEnter message
	dndSendEnter(
		g_conn,
		xdndVersion,
		drop_src_window,
		target_window
	);

	// Send XdndPosition message
	dndSendPosition(
		g_conn,
		drop_src_window,
		target_window,
		XCB_CURRENT_TIME,
		g_dropX, g_dropY
	);

	// Send XdndDrop message
	dndSendDrop(
		g_conn,
		drop_src_window,
		target_window,
		XCB_CURRENT_TIME //Time xdndLastPositionTimestamp
	);

	xcb_flush(g_conn);
}

// called from WM_DROPFILES in winmultiwindowwndproc.c
void dndSendDropMessage(
	HWND hWnd
)
{
	if (!g_fClipboardStarted)
	{
		//winDebug("send_drop_message - g_fClipboardStarted is False.\n");
		return;
	}

	g_DropWindowHwnd = hWnd;

	xcb_window_t source = g_root_window_id;
	xcb_window_t destination = g_ClipWindow;

    xcb_client_message_event_t e;
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = source;
    e.type = g_atmDropEvent;
    e.format = 32;
    e.data.data32[0] = 0;
    e.data.data32[1] = XCB_CURRENT_TIME;
    xcb_send_event(
    	g_conn,
    	FALSE,
    	destination,
    	XCB_EVENT_MASK_NO_EVENT,
    	(const char *)&e
    );
	xcb_flush(g_conn);
}

// called from WM_DROPFILES in winWindowProc (winwndproc.c)
void dndSendDropMessageFullScreen(void)
{
	if (!g_fClipboardStarted)
	{
		//winDebug("send_drop_message - g_fClipboardStarted is False.\n");
		return;
	}

	xcb_window_t source = g_root_window_id;
	xcb_window_t destination = g_ClipWindow;

    xcb_client_message_event_t e;
    memset(&e, 0, sizeof(e));
    e.response_type = XCB_CLIENT_MESSAGE;
    e.window = source;
    e.type = g_atmDropEventFullscreen;
    e.format = 32;
    e.data.data32[0] = 0;
    e.data.data32[1] = XCB_CURRENT_TIME;
    xcb_send_event(
    	g_conn,
    	FALSE,
    	destination,
    	XCB_EVENT_MASK_NO_EVENT,
    	(const char *)&e
    );
	xcb_flush(g_conn);
}

// This is sent by the source to the target to say the data is ready
// called by XCB_SELECTION_REQUEST in winMultiWindowXMsgProc (winmultiwindowwm.c)
void dndSendSelectionNotify(
	xcb_selection_request_event_t *selection_request
)
{
	xcb_change_property_checked(
		g_conn,
		XCB_PROP_MODE_REPLACE,
		selection_request->requestor,
		selection_request->property,
		g_atmUriListType,
		8,
		strlen(g_uriList),
		(unsigned char *) g_uriList
	);

	// Setup selection notify xevent
    xcb_selection_notify_event_t eventSelection;
    eventSelection.response_type = XCB_SELECTION_NOTIFY;
    eventSelection.requestor = selection_request->requestor;
    eventSelection.selection = selection_request->selection;
    eventSelection.target = selection_request->target;
    eventSelection.property = selection_request->property;
    eventSelection.time = selection_request->time;

    xcb_send_event(
    	g_conn,
    	FALSE,
    	eventSelection.requestor,
    	XCB_EVENT_MASK_NO_EVENT,
    	(const char *)&eventSelection
    );

    xcb_flush(g_conn);
}

static xcb_atom_t
intern_atom(
	xcb_connection_t *conn,
	const char *atomName
)
{
	xcb_intern_atom_reply_t *atom_reply;
	xcb_intern_atom_cookie_t atom_cookie;
	xcb_atom_t atom = XCB_ATOM_NONE;

	atom_cookie = xcb_intern_atom(conn, 0, strlen(atomName), atomName);
	atom_reply = xcb_intern_atom_reply(conn, atom_cookie, NULL);
	if (atom_reply)
	{
		atom = atom_reply->atom;
		free(atom_reply);
	}
	return atom;
}

// Returns currently active window (or 0)
xcb_window_t dndGetActiveWindow(void)
{
	xcb_atom_t atm_active_window = intern_atom(g_conn,"_NET_ACTIVE_WINDOW");

	xcb_get_property_cookie_t prop_cookie;
	xcb_get_property_reply_t *prop_reply;

	xcb_window_t result = (xcb_window_t)0;

	prop_cookie = xcb_get_property(
		g_conn,
		0, //uint8_t _delete,
      	g_root_window_id,
      	atm_active_window,
      	XCB_GET_PROPERTY_TYPE_ANY,
      	0, 32
    );

	prop_reply = xcb_get_property_reply(g_conn, prop_cookie, NULL); //&e);
	if (prop_reply)
	{
		if (xcb_get_property_value_length(prop_reply))
		{
			result = *(xcb_window_t*)xcb_get_property_value(prop_reply);
		}

		free(prop_reply);
	}

	return result;
}

// Returns currently active window IF the drop coords are within its (client) rect, otherwise 0
xcb_window_t dndFindWindow(void)
{
	xcb_window_t result = (xcb_window_t)0;  // xcb_window_t = unsigned int

	xcb_get_geometry_cookie_t geometry_cookie;
	xcb_get_geometry_reply_t *geometry_reply;

	xcb_translate_coordinates_cookie_t translate_cookie;
	xcb_translate_coordinates_reply_t *translate_reply;

	xcb_window_t win_active = dndGetActiveWindow();
	if (win_active)
	{
		translate_cookie = xcb_translate_coordinates(
			g_conn,
			win_active,
			g_root_window_id,
			0, //int16_t src_x,
			0  //int16_t src_y
		);

		translate_reply = xcb_translate_coordinates_reply(
			g_conn,
			translate_cookie,
			NULL
		);

		if (translate_reply)
		{
			geometry_cookie = xcb_get_geometry(g_conn, win_active);
			geometry_reply = xcb_get_geometry_reply(
				g_conn,
          		geometry_cookie,
          		NULL
          	);

			if (geometry_reply)
			{
				if (
					g_dropX >= translate_reply->dst_x && g_dropX < translate_reply->dst_x + geometry_reply->width &&
					g_dropY >= translate_reply->dst_y && g_dropY < translate_reply->dst_y + geometry_reply->height
				)
				{
					result = win_active;
				}
				free(geometry_reply);
			}
			free(translate_reply);
		}
	}

	return result;
}

// Create and save g_uriList
Bool dndHdropToUriList(
	HDROP pData
)
{
	wchar_t wfilename[MAX_PATH];
	char filename[MAX_PATH];

	g_uriList[0] = 0;
	char *ptr;

	int cnt = DragQueryFileW((HDROP)pData, 0xFFFFFFFF, NULL, 0);
	if (cnt > MAX_FILES_PER_DROP)
		cnt = MAX_FILES_PER_DROP;

	for (int i = 0; i < cnt; i++)
	{
		DragQueryFileW((HDROP)pData, i, wfilename, MAX_PATH);

		if (isLnkFileW(wfilename))
		{
			WCHAR wresovled[MAX_PATH];
			resolveLnkW(wfilename, wresovled);
			wcscpy(wfilename, wresovled);
		}

		// convert wide char path to UTF-8
		WideCharToMultiByte(
			CP_UTF8,
			0,
			wfilename,
			MAX_PATH,
			filename,
			MAX_PATH,
			NULL,
			NULL
		);

		// replace backslashes with slashes
	    ptr = (char *)filename;
	    while ((ptr = strchr(ptr, '\\')) != NULL)
	    {
	        *ptr++ = '/';
	    }

		ptr = (char *)filename;
		if (strncasecmp(filename, "//wsl.localhost/", 16) == 0)
		{
			strcat(g_uriList, "file://");
			ptr += 16;
			strcat(g_uriList, ptr);
			strcat(g_uriList, "\r\n");
		}
		else if (strncmp(filename, "//", 2) == 0)
		{
			// \\192.168.2.12\test\test.txt  =>  /mnt/192.168.2.12/test/test.txt
			// This will ONLY work if the share was previously mounted this way.
			//   $ sudo mkdir -p /mnt/192.168.2.12/test
			//   $ sudo mount -t drvfs '\\192.168.2.12\test' /mnt/192.168.2.12/test
			strcat(g_uriList, "file:///mnt/");
			ptr += 2;
			strcat(g_uriList, ptr);
			strcat(g_uriList, "\r\n");
		}
		else if (strlen(filename) > 1 && filename[1] == ':')
		{
			strcat(g_uriList, "file:///mnt/");
			char drive[] = " ";
			drive[0] = tolower(filename[0]);
			strcat(g_uriList, drive);
			ptr += 2;
			strcat(g_uriList, ptr);
			strcat(g_uriList, "\r\n");
		}
	}

	//DragFinish((HDROP)wParam);

	return TRUE;
}

// Stolen from cURL

static const unsigned char hextable[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,       /* 0x30 - 0x3f */
  0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 - 0x4f */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* 0x50 - 0x5f */
  0, 10, 11, 12, 13, 14, 15                             /* 0x60 - 0x66 */
};

#define onehex2dec(x) hextable[x - '0']

#define ISLOWHEXALHA(x) (((x) >= 'a') && ((x) <= 'f'))
#define ISUPHEXALHA(x) (((x) >= 'A') && ((x) <= 'F'))
#define ISDIGIT(x)  (((x) >= '0') && ((x) <= '9'))
#define ISXDIGIT(x) (ISDIGIT(x) || ISLOWHEXALHA(x) || ISUPHEXALHA(x))

Bool
urldecode(
	const char *string,
	int length,
	char **ostring,
	int *olen
)
{
	size_t alloc;
	char *ns;
	alloc = (length ? length : strlen(string));
	ns = malloc(alloc + 1);
	if(!ns)
		return FALSE;
	*ostring = ns;
	while(alloc)
	{
		unsigned char in = (unsigned char)*string;
		if (('%' == in) && (alloc > 2) && ISXDIGIT(string[1]) && ISXDIGIT(string[2]))
		{
			/* this is two hexadecimal digits following a '%' */
			in = (unsigned char)(onehex2dec(string[1]) << 4) | onehex2dec(string[2]);
			string += 3;
			alloc -= 3;
		}
		else
		{
			string++;
			alloc--;
		}
		*ns++ = (char)in;
	}
	*ns = 0; /* terminate it */
	if (olen)
		/* store output size */
		*olen = ns - *ostring;
	return TRUE;
}
