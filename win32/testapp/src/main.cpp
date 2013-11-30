/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <dittrich@cithraidt.de>
 *
 * This program is free software; you can redistribute it and/or modifyit under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <ledwiz.h>
#include <windows.h>


#define LEDWIZ_DLL_NAME "ledwiz.dll"


struct {

	struct {
		void (LWZCALL * LWZ_SBA) (LWZHANDLE hlwz, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t gps);
		void (LWZCALL * LWZ_PBA) (LWZHANDLE hlwz, uint8_t const *pmode32bytes);
		void (LWZCALL * LWZ_REGISTER)  (LWZHANDLE hlwz, void * hwnd);
		void (LWZCALL * LWZ_SET_NOTIFY) (LWZNOTIFYPROC notify_callback, LWZDEVICELIST *plist);
	} fn;

	HMODULE hdll;
	HWND hwnd;
	HANDLE hthread;

	LWZDEVICELIST devlist;

} g_main;


static void CALLBACK notify_cb(int32_t reason, LWZHANDLE hlwz)
{
	if (reason == LWZ_REASON_ADD)
	{
		printf("ledwiz device found, id: %d\n", hlwz);

		g_main.fn.LWZ_REGISTER(hlwz, g_main.hwnd);

		// switch on all LEDs and set brightness to '30'

		g_main.fn.LWZ_SBA(hlwz, 0xFF, 0xFF, 0xFF, 0xFF, 2);

		uint8_t mode[32];
		memset(&mode, 30, sizeof(mode));
		g_main.fn.LWZ_PBA(hlwz, mode);
	}
	else if (reason == LWZ_REASON_DELETE)
	{
		printf("ledwiz device removed, id: %d\n", hlwz);
	}
}

static LRESULT CALLBACK MyWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_CREATE:
	{
		// save windows handle

		g_main.hwnd = hwnd;

		// enumerate LED wiz devices

		g_main.fn.LWZ_SET_NOTIFY(notify_cb, &g_main.devlist);

		if (g_main.devlist.numdevices <= 0)
		{
			printf("no ledwiz devices detected!\n");
			DestroyWindow(hwnd);
			return 0;
		}

		break;
	}

	case WM_CLOSE:
	{
		// switch off all LEDs for all devices

		for (int i = 0; i < g_main.devlist.numdevices; i++)
		{
			g_main.fn.LWZ_SBA(g_main.devlist.handles[i], 0x00, 0x00, 0x00, 0x00, 2);
		}

		DestroyWindow(hwnd);
		return 0;
	}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

static BOOL ctrlhandler(DWORD fdwCtrlType)
{ 
	switch(fdwCtrlType)
	{ 
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		PostMessage(g_main.hwnd, WM_CLOSE, 0, 0);
		WaitForSingleObject(g_main.hthread, INFINITE);
		return TRUE;

	default:
		return FALSE;
	}
}

static DWORD WINAPI MyThreadProc(LPVOID lpParameter)
{
	// load the ledwiz.dll

	g_main.hdll = LoadLibraryA(LEDWIZ_DLL_NAME);

	if (g_main.hdll == NULL)
	{
		printf("loading " LEDWIZ_DLL_NAME " failed!\n");
		return 0;
	}

	((void**)&g_main.fn.LWZ_SBA)[0]         = GetProcAddress(g_main.hdll, "LWZ_SBA");
	((void**)&g_main.fn.LWZ_PBA)[0]         = GetProcAddress(g_main.hdll, "LWZ_PBA");
	((void**)&g_main.fn.LWZ_REGISTER)[0]    = GetProcAddress(g_main.hdll, "LWZ_REGISTER");
	((void**)&g_main.fn.LWZ_SET_NOTIFY)[0]  = GetProcAddress(g_main.hdll, "LWZ_SET_NOTIFY");

	if (g_main.fn.LWZ_SBA == NULL ||
		g_main.fn.LWZ_PBA == NULL ||
		g_main.fn.LWZ_REGISTER == NULL ||
		g_main.fn.LWZ_SET_NOTIFY == NULL)
	{
		printf("getting the function addresses failed!\n");
		goto Failed;
	}

	// create a hidden window, so we get a 'HWND' for the ledwiz driver and 
	// we can do the other stuff from the window proc
	{
		WNDCLASSEXA wx = {};
		wx.cbSize = sizeof(WNDCLASSEXA);
		wx.lpfnWndProc = MyWndProc; 
		wx.hInstance = GetModuleHandle(NULL);
		wx.lpszClassName = "LWCloneU2";

		if (!RegisterClassExA(&wx))
			goto Failed;

		HWND hwnd = CreateWindowExA(0, wx.lpszClassName, "LWCloneU2 TestApp Hidden Window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

		if (hwnd == NULL)
			goto Failed;
	}

	// set the console handler to handle the crtl-c and exit clicks

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlhandler, TRUE);

	// window message loop

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

Failed:
	if (g_main.hdll) 
	{
		FreeLibrary(g_main.hdll);
		g_main.hdll = NULL;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	// create a worker thread (for creating a hidden window) and wait until it exits

	g_main.hthread = CreateThread(NULL, 0, MyThreadProc, NULL, 0, NULL);

	if (g_main.hthread == NULL)
		return -1;

	WaitForSingleObject(g_main.hthread, INFINITE);
	CloseHandle(g_main.hthread);
}
