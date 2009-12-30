
#include <windows.h>

#include "../../Core/Src/Core.h"
#include "VideoConfig.h"
#include "main.h"
#include "EmuWindow.h"
#include "Fifo.h"

namespace EmuWindow
{
HWND m_hWnd = NULL;
HWND m_hMain = NULL;
HWND m_hParent = NULL;
HINSTANCE m_hInstance = NULL;
WNDCLASSEX wndClass;
const TCHAR m_szClassName[] = _T("DolphinEmuWnd");
int g_winstyle;
static volatile bool s_sizing;

bool IsSizing()
{
	return s_sizing;
}

HWND GetWnd()
{
	return m_hWnd;
}

HWND GetParentWnd()
{
	return m_hParent;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	switch( iMsg )
	{
	case WM_CREATE:
		PostMessage( m_hMain, WM_USER, WM_USER_CREATE, g_Config.RenderToMainframe );
		break;
	case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		s_sizing = true;
		break;

	case WM_EXITSIZEMOVE:
		s_sizing = false;
		break;

	case WM_KEYDOWN:
		switch (LOWORD(wParam))
		{
			case VK_ESCAPE:
				// Toggle full screen doesn't work yet
				/*
				if (g_ActiveConfig.bFullscreen)
				{
					// Pressing Esc switch to Windowed in Fullscreen mode
					ToggleFullscreen(hWnd);
					return 0;
				}
				else
				{
					// And stops the emulation when already in Windowed mode
					PostMessage(m_hMain, WM_USER, WM_USER_STOP, 0);
					return 0;
				}
				*/
				if (g_ActiveConfig.bFullscreen)
				{
					DestroyWindow(hWnd);
					PostQuitMessage(0);
					ExitProcess(0);
				}
				break;
		}
		// Tell the hotkey function that this key was pressed
		g_VideoInitialize.pKeyPress(LOWORD(wParam), GetAsyncKeyState(VK_SHIFT) != 0, GetAsyncKeyState(VK_CONTROL) != 0);
		break;
/*
	case WM_SYSKEYDOWN:
		switch( LOWORD( wParam ))
		{
			case VK_RETURN:   // Pressing Alt+Enter switch FullScreen/Windowed
				if (g_ActiveConfig.bFullscreen)
				{
					DestroyWindow(hWnd);
					PostQuitMessage(0);
					ExitProcess(0);
				}
				break;
		}
		//g_VideoInitialize.pKeyPress(LOWORD(wParam), GetAsyncKeyState(VK_SHIFT) != 0, GetAsyncKeyState(VK_CONTROL) != 0);
		break;
*/

	/* Post thes mouse events to the main window, it's nessesary because in difference to the
	   keyboard inputs these events only appear here, not in the parent window or any other WndProc()*/
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		PostMessage(GetParentWnd(), iMsg, wParam, lParam);
	break;

	case WM_CLOSE:
		// When the user closes the window, we post an event to the main window to call Stop()
		// Which then handles all the necessary steps to Shutdown the core + the plugins
		PostMessage( m_hMain, WM_USER, WM_USER_STOP, 0 );
		return 0;

	case WM_USER:
		// if (wParam == TOGGLE_FULLSCREEN)
		// TODO : Insert some toggle fullscreen code here, kthx :d
		// see TODO ^ upper
		break;

	case WM_SYSCOMMAND:
		switch (wParam) 
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;
	}

	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}


HWND OpenWindow(HWND parent, HINSTANCE hInstance, int width, int height, const TCHAR *title)
{
	wndClass.cbSize = sizeof( wndClass );
	wndClass.style  = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	wndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = m_szClassName;
	wndClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

	m_hInstance = hInstance;
	RegisterClassEx( &wndClass );

	if (g_Config.RenderToMainframe)
	{
		m_hParent = m_hMain = parent;

		m_hWnd = CreateWindowEx(0, m_szClassName, title, WS_CHILD,
			0, 0, width, height,
			m_hParent, NULL, hInstance, NULL);

		/*if( !g_Config.bFullscreen )
			SetWindowPos( GetParent(m_hParent), NULL, 0, 0, width, height, SWP_NOMOVE|SWP_NOZORDER );*/
	}
	else
	{
		m_hMain = parent;

		DWORD style = g_Config.bFullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;

		RECT rc = {0, 0, width, height};
		AdjustWindowRect(&rc, style, false);

		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;

		rc.left = (1280 - w)/2;
		rc.right = rc.left + w;
		rc.top = (1024 - h)/2;
		rc.bottom = rc.top + h;

		m_hWnd = CreateWindowEx(0, m_szClassName, title, style,
			rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
			NULL, NULL, hInstance, NULL);
	}

	return m_hWnd;
}

void Show()
{
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	BringWindowToTop(m_hWnd);
	UpdateWindow(m_hWnd);
}

HWND Create(HWND hParent, HINSTANCE hInstance, const TCHAR *title)
{
	int width=640, height=480;
	sscanf( g_Config.bFullscreen ? g_Config.cFSResolution : g_Config.cInternalRes, "%dx%d", &width, &height );
	return OpenWindow(hParent, hInstance, width, height, title);
}

void Close()
{
	DestroyWindow(m_hWnd);
	UnregisterClass(m_szClassName, m_hInstance);
}

void SetSize(int width, int height)
{
	RECT rc = {0, 0, width, height};
	DWORD style = GetWindowLong(m_hWnd, GWL_STYLE);
	AdjustWindowRect(&rc, style, false);

	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;

	rc.left = (1280 - w)/2;
	rc.right = rc.left + w;
	rc.top = (1024 - h)/2;
	rc.bottom = rc.top + h;
	::MoveWindow(m_hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
}

void ToggleFullscreen(HWND hParent)
{
	if (m_hParent == NULL)
	{ 
		int	w_fs = 640, h_fs = 480;
		if (g_Config.bFullscreen)
		{
			//Get out of fullscreen
			g_ActiveConfig.bFullscreen = false;

			// FullScreen - > Desktop
			ChangeDisplaySettings(NULL, 0);

			// Re-Enable the cursor
			ShowCursor(TRUE);

			RECT rcdesktop;
			RECT rc = {0, 0, 640, 480};
			GetWindowRect(GetDesktopWindow(), &rcdesktop);

			int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
			int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;
			// SetWindowPos to the center of the screen
			SetWindowPos(hParent, NULL, X, Y, 640, 480, SWP_NOREPOSITION | SWP_NOZORDER);

			// Set new window style FS -> Windowed
			SetWindowLong(hParent, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			// Eventually show the window!
			EmuWindow::Show();
		}
		else
		{
			// Get into fullscreen
			g_ActiveConfig.bFullscreen = true;
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			RECT rcdesktop;
			GetWindowRect(GetDesktopWindow(), &rcdesktop);

			// Desktop -> FullScreen
			dmScreenSettings.dmSize			= sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth	= rcdesktop.right;
			dmScreenSettings.dmPelsHeight	= rcdesktop.bottom;
			dmScreenSettings.dmBitsPerPel	= 32;
			dmScreenSettings.dmFields = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
				return ;
			// Disable the cursor
			ShowCursor(FALSE);

			// SetWindowPos to the upper-left corner of the screen
			SetWindowPos(hParent, NULL, 0, 0, rcdesktop.right, rcdesktop.bottom, SWP_NOREPOSITION | SWP_NOZORDER);

			// Set new window style -> PopUp
			SetWindowLong(hParent, GWL_STYLE, WS_POPUP);

			// Eventually show the window!
			EmuWindow::Show();
		}
	}
}

}
