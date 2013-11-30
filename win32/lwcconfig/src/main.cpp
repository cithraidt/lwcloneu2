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
		int (LWZCALL * LWZ_RAWWRITE) (LWZHANDLE hlwz, uint8_t const *pdata, uint32_t ndata);
		void (LWZCALL * LWZ_REGISTER)  (LWZHANDLE hlwz, void * hwnd);
		void (LWZCALL * LWZ_SET_NOTIFY) (LWZNOTIFYPROC notify_callback, LWZDEVICELIST *plist);
	} fn;

	HMODULE hdll;
	HWND hwnd;
	HANDLE hthread;

	LWZDEVICELIST devlist;

} g_main = {0};


static void CALLBACK notify_cb(int32_t reason, LWZHANDLE hlwz)
{
	if (reason == LWZ_REASON_ADD)
	{
		printf("ledwiz device found, id: %d\n", hlwz);

		g_main.fn.LWZ_REGISTER(hlwz, g_main.hwnd);
	}
	else if (reason == LWZ_REASON_DELETE)
	{
		printf("ledwiz device removed, id: %d\n", hlwz);
	}
}


void usage()
{
	printf("\n");
	printf("Usage:\n\n");
	printf("lwcconfig [-p <new id>] [<current id>]\n");
	printf("    -h .................... help\n");
	printf("    -p <new id> ........... program new id\n");
	printf("\n");
}


int main(int argc, char* argv[])
{
	// parse arguments

	if (argc <= 1) {
		usage();
		return 0;
	}

	const char * p_arg = NULL;
	const char * id_arg = NULL;
	int err = 0;

	for (int i = 1; i < argc && err == 0; i++) 
	{
		if (argv[i][0] == '-') 
		{
			switch (argv[i][1]) {
			case 'h':
			{
				err = 1;
				break;
			}
			case 'p':
			{
				p_arg = &argv[i][2];

				if (p_arg[0] == '\0' && (i+1) < argc) {
				    p_arg = argv[++i];
				}

				break;
			}
			default:
			{
				err = -4;
			}
			}
		}
		else
		{
			if (id_arg != NULL)	{
				err = -5;
			} else {
				id_arg = argv[i];
			}
		}
	}

	if (err < 0)
	{
		printf("invalid argument(s), %d", err);
		usage();
		goto Failed;
	}
	else if (err != 0)
	{
		usage();
		goto Failed;
	}

	// load the ledwiz.dll

	g_main.hdll = LoadLibraryA(LEDWIZ_DLL_NAME);

	if (g_main.hdll == NULL)
	{
		printf("loading " LEDWIZ_DLL_NAME " failed!\n");
		return -1;
	}

	((void**)&g_main.fn.LWZ_SBA)[0]         = GetProcAddress(g_main.hdll, "LWZ_SBA");
	((void**)&g_main.fn.LWZ_PBA)[0]         = GetProcAddress(g_main.hdll, "LWZ_PBA");
	((void**)&g_main.fn.LWZ_RAWWRITE)[0]    = GetProcAddress(g_main.hdll, "LWZ_RAWWRITE");
	((void**)&g_main.fn.LWZ_REGISTER)[0]    = GetProcAddress(g_main.hdll, "LWZ_REGISTER");
	((void**)&g_main.fn.LWZ_SET_NOTIFY)[0]  = GetProcAddress(g_main.hdll, "LWZ_SET_NOTIFY");

	if (g_main.fn.LWZ_SBA == NULL ||
		g_main.fn.LWZ_PBA == NULL ||
		g_main.fn.LWZ_RAWWRITE == NULL ||
		g_main.fn.LWZ_REGISTER == NULL ||
		g_main.fn.LWZ_SET_NOTIFY == NULL)
	{
		printf("getting the function addresses failed!\n");
		goto Failed;
	}

	// enumerate LED wiz devices

	g_main.fn.LWZ_SET_NOTIFY(notify_cb, &g_main.devlist);

	if (g_main.devlist.numdevices <= 0)
	{
		printf("no ledwiz devices detected!\n");
		goto Failed;
	}

	// reprogram new id

	if (p_arg && g_main.devlist.numdevices > 0)
	{
		int const id_old = (id_arg != NULL) ? atoi(id_arg) : g_main.devlist.handles[0];
		int const id_new = atoi(p_arg);

		if (id_new < 1 || id_new >= 32 || id_new == id_old)
		{
			printf("invalid id specified!\n");
			goto Failed;
		}

		int index = -1;
		for (int i = 0; i < g_main.devlist.numdevices; i++)
		{
			if (g_main.devlist.handles[i] == id_old)
			{
				index = i;
				break;
			}
		}

		if (index < 0)
		{
			printf("devide with id=%d not found!\n", id_old);
		}
		else
		{
			// send change ID command
			printf("sending change-id-command to device %d, new id: %d\n", id_old, id_new);

			uint8_t const LWCCONFIG_CMD_SETID = 65;
			uint8_t const id = id_new - 1;
			uint8_t const check = ~id;

			uint8_t buf[8] = {LWCCONFIG_CMD_SETID, id, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, check};

			g_main.fn.LWZ_RAWWRITE(g_main.devlist.handles[index], buf, sizeof(buf));
		}
	}

Failed:
	if (g_main.hdll) 
	{
		FreeLibrary(g_main.hdll);
		g_main.hdll = NULL;
	}

	return 0;
}
