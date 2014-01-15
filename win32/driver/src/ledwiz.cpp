/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
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

#include <string.h>

#include <windows.h>
#include <crtdbg.h>
#include <Setupapi.h>

extern "C" {
#include <Hidsdi.h>
}

#include <Dbt.h>

#define LWZ_DLL_EXPORT
#include "../include/ledwiz.h"


#define USE_SEPERATE_IO_THREAD
// #define USE_OVERLAPPED_WRITEFILE


const GUID HIDguid = { 0x4d1e55b2, 0xf16f, 0x11Cf, { 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

USHORT const VendorID_LEDWiz       = 0xFAFA;
USHORT const ProductID_LEDWiz_min  = 0x00F0;
USHORT const ProductID_LEDWiz_max  = ProductID_LEDWiz_min + LWZ_MAX_DEVICES - 1;

static const char * lwz_process_sync_mutex_name = "lwz_process_sync_mutex";

typedef struct {
	HANDLE hdev;
	LONG refcount;
} devobj;

typedef struct {
	devobj* pdo;
	DWORD dat[256];
} lwz_device_t;

typedef void * HQUEUE;

typedef struct
{
	lwz_device_t devices[LWZ_MAX_DEVICES];
	LWZDEVICELIST *plist;
	HWND hwnd;
	HANDLE hDevNotify;
	WNDPROC WndProc;

	#if defined(USE_SEPERATE_IO_THREAD)
	HQUEUE hqueue;
	#endif

	struct {
		void * puser;
		LWZNOTIFYPROC notify;
		LWZNOTIFYPROC_EX notify_ex;
	} cb;

} lwz_context_t;


// 'g_hmutex' gives us atomic access to the hardware in case the dll is used in different processes
HANDLE g_hmutex;

// 'g_cs' protects our state if there is more than on thread in the process using the API.
// Do not synchronize with other threads from within the callback routine because than it can deadlock!
// Calling the API within the callback from the same thread is fine because the critical section does not block for that.
CRITICAL_SECTION g_cs;

// global context
lwz_context_t * g_plwz = NULL;


static LRESULT CALLBACK lwz_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static lwz_context_t * lwz_open(HINSTANCE hinstDLL);
static void lwz_close(lwz_context_t *h);

static void lwz_register(lwz_context_t *h, int indx_user, HWND hwnd);
static devobj * lwz_get_hdev(lwz_context_t *h, int indx_user);
static void lwz_notify_callback(lwz_context_t *h, int reason, LWZHANDLE hlwz);

static void lwz_refreshlist_attached(lwz_context_t *h);
static void lwz_refreshlist_detached(lwz_context_t *h);
static void lwz_freelist(lwz_context_t *h);
static void lwz_add(lwz_context_t *h, int indx);
static void lwz_remove(lwz_context_t *h, int indx);

static devobj * devobj_create(HANDLE hdev);
void devobj_addref(devobj *pdo);
static void devobj_release(devobj *pdo);

static void queue_close(HQUEUE hqueue, bool unload);
static HQUEUE queue_open(void);
static size_t queue_push(HQUEUE hqueue, devobj *pdo, uint8_t const *pdata, size_t ndata);
static size_t queue_pop(HQUEUE hqueue, devobj **ppdo, uint8_t *pbuffer, size_t nsize);
static void queue_wait_empty(HQUEUE hqueue);


struct CAutoLockCS  // helper class to lock a critical section, and unlock it automatically
{
    CRITICAL_SECTION *m_pcs;
	CAutoLockCS(CRITICAL_SECTION *pcs) { m_pcs = pcs; EnterCriticalSection(m_pcs); };
	~CAutoLockCS() { LeaveCriticalSection(m_pcs); };
};

#define AUTOLOCK(cs) CAutoLockCS lock_##__LINE__##__(&cs)   // helper macro for using the helper class


//**********************************************************************************************************************
// Top Level API functions
//**********************************************************************************************************************

void LWZ_SBA(
	LWZHANDLE hlwz, 
	BYTE bank0, 
	BYTE bank1,
	BYTE bank2,
	BYTE bank3,
	BYTE globalPulseSpeed)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	devobj * pdo = lwz_get_hdev(g_plwz, indx);

	if (pdo == NULL) {
		return;
	}

	BYTE data[9];

	data[0] = 0; // report id
	data[1] = 0x40; // LWZ_SBA command identifier
	data[2] = bank0;
	data[3] = bank1;
	data[4] = bank2;
	data[5] = bank3;
	data[6] = globalPulseSpeed;
	data[7] = 0;
	data[8] = 0;

	#if defined(USE_SEPERATE_IO_THREAD)

	queue_push(g_plwz->hqueue, pdo, &data[1], 8);

	#else

	DWORD nwritten = 0;

	WaitForSingleObject(g_hmutex, INFINITE);
	{
		WriteFile(hdev, data, 9, &nwritten, NULL);
	}
	ReleaseMutex(g_hmutex);

	#endif

}

void LWZ_PBA(LWZHANDLE hlwz, BYTE const *pbrightness_32bytes)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pbrightness_32bytes == NULL)
		return;

	devobj * pdo = lwz_get_hdev(g_plwz, indx);

	if (pdo == NULL) {
		return;
	}

	#if defined(USE_SEPERATE_IO_THREAD)

	queue_push(g_plwz->hqueue, pdo, pbrightness_32bytes, 32);

	#else

	BYTE data[1 + 32]; // one additional byte in front of the data for the report id
	memcpy(&data[1], pbrightness_32bytes, 32);

	BYTE * b = &data[0];
	DWORD nwritten = 0;

	b[0] = 0; // report id

	WaitForSingleObject(g_hmutex, INFINITE);
	{
		WriteFile(hdev, b, 9, &nwritten, NULL); b += 8; b[0] = 0;
		WriteFile(hdev, b, 9, &nwritten, NULL); b += 8; b[0] = 0;
		WriteFile(hdev, b, 9, &nwritten, NULL); b += 8; b[0] = 0;
		WriteFile(hdev, b, 9, &nwritten, NULL); b += 8; b[0] = 0;
	}
	ReleaseMutex(g_hmutex);

	#endif
}

int LWZ_RAWWRITE(LWZHANDLE hlwz, BYTE const *pdata, DWORD ndata)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pdata == NULL)
		return -1;

	if (ndata >  32 || ndata == 0)
		return -2;

	devobj * pdo = lwz_get_hdev(g_plwz, indx);

	if (pdo == NULL) {
		return -3;
	}

	int res = 0;

	#if defined(USE_SEPERATE_IO_THREAD)

	size_t const nwritten = queue_push(g_plwz->hqueue, pdo, pdata, ndata);

	res = (nwritten == ndata) ? 0 : 100 + (int)nwritten;

	#else

	WaitForSingleObject(g_hmutex, INFINITE);
	{
		BYTE buf[9]; 
		buf[0] = 0; // report id

		while (ndata > 0)
		{
			int const ncopy = (ndata > 8) ? 8 : ndata;

			memset(&buf[1], 0x00, 8);
			memcpy(&buf[1], pdata, ncopy);
			pdata += ncopy;
			ndata -= ncopy;

			DWORD nwritten = 0;
			DWORD const nwrite = 9;
			WriteFile(hdev, buf, nwrite, &nwritten, NULL);

			if (nwritten != nwrite) {
				res = 100 + nwritten;
				break;
			}
		}
	}
	ReleaseMutex(g_hmutex);

	#endif

	return res;
}

DWORD LWZ_RAWREAD(LWZHANDLE hlwz, BYTE *pdata, DWORD ndata)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pdata == NULL)
		return 0;

	if (ndata > 64)
	    ndata = 64;

	devobj * pdo = lwz_get_hdev(g_plwz, indx);

	if (pdo == NULL) {
		return 0;
	}

	#if defined(USE_SEPERATE_IO_THREAD)
	queue_wait_empty(g_plwz->hqueue);
	#endif

	int res = 0;
	BYTE buffer[65];
	DWORD nread = 0;
	BOOL bres = FALSE;

	WaitForSingleObject(g_hmutex, INFINITE);
	{
		bres = ReadFile(pdo->hdev, buffer, sizeof(buffer), &nread, NULL);
	}
	ReleaseMutex(g_hmutex);

	if (ndata > nread)
	    ndata = nread;

	memcpy(pdata, buffer, ndata);

	return ndata;
}

void LWZ_REGISTER(LWZHANDLE hlwz, void * hwin)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	lwz_register(g_plwz, indx, (HWND)hwin);
}

void LWZ_SET_NOTIFY_EX(LWZNOTIFYPROC_EX notify_ex_cb, void * puser, LWZDEVICELIST *plist)
{
	AUTOLOCK(g_cs);

	lwz_context_t * const h = g_plwz;

	h->plist = plist;
	h->cb.notify_ex = notify_ex_cb;
	h->cb.puser = puser;

	if (h->plist)
	{
		memset(h->plist, 0x00, sizeof(*plist));
	}

	lwz_refreshlist_attached(h);
}

void LWZ_SET_NOTIFY(LWZNOTIFYPROC notify_cb, LWZDEVICELIST *plist)
{
	AUTOLOCK(g_cs);

	lwz_context_t * const h = g_plwz;

	// clean up (if there was a LWZ_SET_NOTIFY before)

	lwz_freelist(h);
	lwz_register(h, 0, NULL);

	// set new list pointer and callbacks

	h->plist = plist;
	h->cb.notify = notify_cb;

	if (h->plist)
	{
		memset(h->plist, 0x00, sizeof(*plist));
	}

	// create a new internal list of available devices

	lwz_refreshlist_attached(h);
}


//**********************************************************************************************************************
// Low level implementation 
//**********************************************************************************************************************

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hmutex = CreateMutexA(
			NULL,
			FALSE,
			lwz_process_sync_mutex_name);

		if (g_hmutex == NULL)
			return FALSE;

		InitializeCriticalSection(&g_cs);

		g_plwz = lwz_open(hinstDLL);

		if (g_plwz == NULL)
		{
			CloseHandle(g_hmutex);
			DeleteCriticalSection(&g_cs);
			return FALSE;
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		{
			AUTOLOCK(g_cs);

			lwz_close(g_plwz);
			CloseHandle(g_hmutex);
		}

		DeleteCriticalSection(&g_cs);
	}

	return TRUE;
}

static LRESULT CALLBACK lwz_wndproc(
	HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
	AUTOLOCK(g_cs);

	lwz_context_t * const h = g_plwz;

	// get the original WndProc

	WNDPROC OriginalWndProc = h->WndProc;

	// check for device attach/remove messages

	if (uMsg == WM_DEVICECHANGE)
	{
		switch (wParam) {
		case DBT_DEVICEARRIVAL:
			lwz_refreshlist_attached(h);
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			lwz_refreshlist_detached(h);
			break;
		}
	}
	else 

	// check if the window is going to be destroyed

	if (uMsg == WM_DESTROY)
	{
		lwz_freelist(h);
		lwz_register(h, 0, NULL); // this will restore the original windows proc (and clear h->WndProc)
	}

	// forward message to original windows procedure

	if (OriginalWndProc != NULL)
	{
		return CallWindowProc(
			OriginalWndProc,
			hwnd,
			uMsg,
			wParam,
			lParam);
	}

	return 0;
}

static lwz_context_t * lwz_open(HINSTANCE hinstDLL)
{
	lwz_context_t * const h = (lwz_context_t *)malloc(sizeof(lwz_context_t));
	if (h == NULL)
		return NULL;

	memset(h, 0x00, sizeof(*h));

	#if defined(USE_SEPERATE_IO_THREAD)
	h->hqueue = queue_open();

	if (h->hqueue == NULL)
	{
		free(h);
		return NULL;
	}
	#endif

	return h;
}

static void lwz_close(lwz_context_t *h)
{
	if (h == NULL)
		return;

	// close all open device handles and
	// unhook our window proc (and unregister the device change notifications)

	lwz_freelist(h);
	lwz_register(h, 0, NULL);

	#if defined(USE_SEPERATE_IO_THREAD)
	if (h->hqueue != NULL)
	{
		queue_close(h->hqueue, true);
		h->hqueue = NULL;
	}
	#endif

	// free resources

	free(h);
}
	
static void lwz_register(lwz_context_t *h, int indx, HWND hwnd)
{
	// register *or* unregister

	if (hwnd == NULL)
	{
		if (h->hDevNotify)
		{
			UnregisterDeviceNotification(h->hDevNotify);
			h->hDevNotify = NULL;
		}

		if (h->hwnd &&
		    h->WndProc)
		{
			SetWindowLongPtrA(
				h->hwnd,
				GWLP_WNDPROC,
				(LONG_PTR)h->WndProc);

			h->hwnd = NULL;
			h->WndProc = NULL;
		}
	}
	else
	{
		// do not allow to register to multiple windows

		if (h->hwnd &&
			h->hwnd != hwnd)
		{
			return;
		}

		// check if we got a user callback
		
		if (h->cb.notify == NULL && 
			h->cb.notify_ex == NULL)
		{
			return;
		}

		// verify that this index is valid

		if (indx < 0 ||
			indx >= LWZ_MAX_DEVICES)
		{
			return;
		}

		if (h->devices[indx].pdo == NULL) {
			return;
		}

		// "subclass" the window

		WNDPROC PrevWndProc = (WNDPROC)SetWindowLongPtrA(
			hwnd,
			GWLP_WNDPROC,
			(LONG_PTR)lwz_wndproc);

		if (PrevWndProc == NULL ||
			PrevWndProc == lwz_wndproc)
		{
			return;
		}

		h->WndProc = PrevWndProc;
		h->hwnd = hwnd;

		if (h->hDevNotify == NULL)
		{
			DEV_BROADCAST_DEVICEINTERFACE_A dbch = {};
			dbch.dbcc_size = sizeof(dbch); 
			dbch.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE; 
			dbch.dbcc_classguid = HIDguid; 
			dbch.dbcc_name[0] = '\0'; 
			
			h->hDevNotify = RegisterDeviceNotificationA(hwnd, &dbch, DEVICE_NOTIFY_WINDOW_HANDLE);
		}
	}
}

static devobj * lwz_get_hdev(lwz_context_t *h, int indx)
{
	if (indx < 0 ||
	    indx >= LWZ_MAX_DEVICES)
	{
		return NULL;
	}

	return h->devices[indx].pdo;
}

static void lwz_notify_callback(lwz_context_t *h, int reason, LWZHANDLE hlwz)
{
	if (h->cb.notify)
		h->cb.notify(reason, hlwz);

	if (h->cb.notify_ex)
		h->cb.notify_ex(h->cb.puser, reason, hlwz);
}

static void lwz_add(lwz_context_t *h, int indx)
{
	LWZHANDLE hlwz = indx + 1;

	// update user list (if one was provided)

	if (h->plist)
	{
		if (h->plist->numdevices < LWZ_MAX_DEVICES)
		{
			h->plist->handles[h->plist->numdevices] = hlwz;
			h->plist->numdevices += 1;
		}
	}

	// notify callback

	lwz_notify_callback(h, LWZ_REASON_ADD, hlwz);
}

static void lwz_remove(lwz_context_t *h, int indx)
{
	LWZHANDLE hlwz = indx + 1;

	// update user list (if one waw provided)

	if (h->plist)
	{
		for (int i = 0; i < h->plist->numdevices; i++)
		{
			LWZHANDLE hlwz = indx + 1;

			if (h->plist->handles[i] != hlwz)
				continue;

			h->plist->handles[i] = h->plist->handles[h->plist->numdevices - 1];
			h->plist->handles[h->plist->numdevices - 1] = 0;

			h->plist->numdevices -= 1;
		}
	}

	// notify callback

	lwz_notify_callback(h, LWZ_REASON_DELETE, hlwz);
}

static void lwz_refreshlist_detached(lwz_context_t *h)
{
	// check for removed devices
	// i.e. try to re-open all registered devices in our internal list

	for (int i = 0; i < LWZ_MAX_DEVICES; i++)
	{
		if (h->devices[i].pdo != NULL)
		{
			SP_DEVICE_INTERFACE_DETAIL_DATA_A * pdiddat = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)&h->devices[i].dat[0];

			HANDLE hdev = CreateFileA(
				pdiddat->DevicePath,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if (hdev == NULL)
			{
				devobj_release(h->devices[i].pdo);
				h->devices[i].pdo = NULL;

				lwz_remove(h, i);
			}
			else
			{
				CloseHandle(hdev);
			}
		}
	}
}

static void lwz_refreshlist_attached(lwz_context_t *h)
{
	// check for new devices

	HDEVINFO hDevInfo = SetupDiGetClassDevsA(
		&HIDguid, 
		NULL, 
		NULL, 
		DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	if (hDevInfo == INVALID_HANDLE_VALUE)
		return;

	// go through all available devices and look for the proper VID/PID

	int num_devices = 0;

	for (DWORD dwindex = 0;; dwindex++)
	{
		if (num_devices >= LWZ_MAX_DEVICES)
			break;

		SP_DEVICE_INTERFACE_DATA didat = {};
		didat.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		BOOL bres = FALSE;

		bres = SetupDiEnumDeviceInterfaces(
			hDevInfo,
			NULL,
			&HIDguid,
			dwindex,
			&didat);

		if (bres == FALSE)
			break;

		lwz_device_t device_tmp = {};

		SP_DEVICE_INTERFACE_DETAIL_DATA_A * pdiddat = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)&device_tmp.dat[0];
		pdiddat->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		bres = SetupDiGetDeviceInterfaceDetailA(
			hDevInfo,
			&didat,
			pdiddat,
			sizeof(device_tmp.dat),
			NULL,
			NULL);

		if (bres == FALSE) {
			continue;
		}

		HANDLE hdev_tmp  = CreateFileA(
			pdiddat->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		device_tmp.pdo = devobj_create(hdev_tmp);

		if (device_tmp.pdo != NULL)
		{
			HIDD_ATTRIBUTES attrib = {};
			attrib.Size = sizeof(HIDD_ATTRIBUTES);

			BOOLEAN bSuccess = HidD_GetAttributes(
				device_tmp.pdo->hdev,
				&attrib);

			// if this is the VID/PID we are interested in
			// check some additional properties

			int indx = (int)attrib.ProductID - (int)ProductID_LEDWiz_min;

			if (bSuccess && 
			    attrib.VendorID == VendorID_LEDWiz &&
			    indx >= 0 && indx < LWZ_MAX_DEVICES)
			{
				PHIDP_PREPARSED_DATA p_prepdata = NULL;

				if (HidD_GetPreparsedData(device_tmp.pdo->hdev, &p_prepdata) == TRUE)
				{
					HIDP_CAPS caps = {};

					if (HIDP_STATUS_SUCCESS == HidP_GetCaps(p_prepdata, &caps))
					{
						// LED-wiz has an interface with a eight byte report 
						// (report-id is zero and is not transmitted, but counts here
						// for the total length)

						if (caps.NumberLinkCollectionNodes == 1 &&
							caps.OutputReportByteLength == 9)
						{
							// if this index is already in use, check the UsagePage lower Byte 
							// and try to register as an additional device

							if (h->devices[indx].pdo != NULL)
							{
								USHORT UsagePage = caps.UsagePage;
								int indx_new = UsagePage & 0xFF;

								if (indx_new >= 0 && indx_new < LWZ_MAX_DEVICES) {
									indx = indx_new;
								}
							}

							if (h->devices[indx].pdo == NULL)
							{
								memcpy(&h->devices[indx], &device_tmp, sizeof(device_tmp));
								device_tmp.pdo = NULL;

								lwz_add(h, indx);

								num_devices++;
							}
						}
					}

					HidD_FreePreparsedData(p_prepdata);
				}
			}

			if (device_tmp.pdo != NULL)
			{
				devobj_release(device_tmp.pdo);
				device_tmp.pdo = NULL;
			}
		}
	}

	if (hDevInfo != NULL)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
}

static void lwz_freelist(lwz_context_t *h)
{
	for (int i = 0; i < LWZ_MAX_DEVICES; i++)
	{
		if (h->devices[i].pdo != NULL)
		{
			devobj_release(h->devices[i].pdo);
			h->devices[i].pdo = NULL;
		}
	}
}


// simple fifo to move the WriteFile() calls to a seperate thread

typedef struct {
	devobj* pdo;
	size_t ndata;
	uint8_t data[32];
} chunk_t;

#define QUEUE_LENGTH   64   // the maximum bandwidth of the device is around 2 kByte/s so a lenght of 64 corresponds to one second

typedef struct {
	int rpos;
	int wpos;
	int level;
	int state;
	HANDLE hthread;
	CRITICAL_SECTION cs;
	HANDLE hrevent;
	HANDLE hwevent;
	HANDLE heevent;
	HANDLE hqevent;
	bool rblocked;
	bool wblocked;
	bool eblocked;
	chunk_t buf[QUEUE_LENGTH];
} queue_t;


static DWORD WINAPI QueueThreadProc(LPVOID lpParameter)
{
	queue_t * const h = (queue_t*)lpParameter;

	for (;;)
	{
		uint8_t buffer[64];
		memset(&buffer[0], 0x00, sizeof(buffer));

		// reserve the first byte for setting the report id later

		devobj * pdo = NULL;
		size_t ndata = queue_pop(h, &pdo, &buffer[1], sizeof(buffer) - 1);

		// exit thread if required

		if (ndata == 0 || pdo == NULL) {
			break;
		}

		WaitForSingleObject(g_hmutex, INFINITE);
		{
			uint8_t * b = &buffer[0];

			while (ndata > 0)
			{
				b[0] = 0; // report id

				size_t const npayload = (ndata > 8) ? 8 : ndata;
				DWORD nwritten = 0;
				DWORD nwrite = 1 + (DWORD)npayload;

				BOOL bresult = WriteFile(pdo->hdev, b, nwrite, &nwritten, NULL);

				if (bresult != TRUE || nwritten != nwrite)
				{
					DWORD dwerror = GetLastError();
					_ASSERT(0);
				}

				b += npayload;
				ndata -= npayload;
			}
		}
		ReleaseMutex(g_hmutex);

		devobj_release(pdo);
	}

	SetEvent(h->hqevent);

	return 0;
}

static void queue_close(HQUEUE hqueue, bool unload)
{
	queue_t * const h = (queue_t*)hqueue;

	if (h == NULL) {
		return;
	}

	if (h->hthread)
	{
		queue_push(h, NULL, NULL, 0);

		if (unload)
		{
			// we can *not* wait for the thread itself
			// if we are closed within the DLL unload.
			// this would result in a deadlock
			// instead we sync with the 'hqevent' that is set at the end of the thread routine

			WaitForSingleObject(h->hqevent, INFINITE);
			CloseHandle(h->hthread);
			h->hthread = NULL;
		}
		else
		{
			WaitForSingleObject(h->hthread, INFINITE);
			CloseHandle(h->hthread);
			h->hthread = NULL;
		}
	}

	if (h->hrevent)
	{
		CloseHandle(h->hrevent);
		h->hrevent = NULL;
	}

	if (h->hwevent)
	{
		CloseHandle(h->hwevent);
		h->hwevent = NULL;
	}

	if (h->hqevent)
	{
		CloseHandle(h->hqevent);
		h->hqevent = NULL;
	}

	DeleteCriticalSection(&h->cs);

	free(h);
}

static HQUEUE queue_open(void)
{
	queue_t * const h = (queue_t*)malloc(sizeof(queue_t));

	if (h == NULL) {
		return NULL;
	}

	memset(h, 0x00, sizeof(queue_t));

	InitializeCriticalSection(&h->cs);

	h->hrevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	h->hwevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	h->heevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	h->hqevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (h->hrevent == NULL ||
		h->hwevent == NULL ||
		h->heevent == NULL ||
		h->hqevent == NULL )
	{
		goto Failed;
	}

	h->hthread = CreateThread(NULL, 0, QueueThreadProc, (void*)h, 0, NULL);

	if (h->hthread == NULL) {
		goto Failed;
	}

	return h;

Failed:
	queue_close(h, false);
	return NULL;
}

static void queue_wait_empty(HQUEUE hqueue)
{
	queue_t * const h = (queue_t*)hqueue;

	for (;;)
	{
		{
			AUTOLOCK(h->cs);

			if (h->state != 0) {
				return;
			}

			if (h->level == 0)
			{
				h->eblocked = false;
				return;
			}

			h->eblocked = true;
		}

		WaitForSingleObject(h->heevent, INFINITE);
	}
}

static size_t queue_push(HQUEUE hqueue, devobj* pdo, uint8_t const *pdata, size_t ndata)
{
	queue_t * const h = (queue_t*)hqueue;

	if (pdata == NULL || ndata == 0 || ndata > sizeof(h->buf[0].data)) 
	{
		// push empty chunk to signal shutdown

		pdata = NULL;
		ndata = 0;
		pdo = NULL;
	}

	for (;;)
	{
		bool do_wait = false;
		bool do_unblock = false;

		// check if there is some space to write into the queue

		{
			AUTOLOCK(h->cs);

			if (h->state != 0) {
				return 0;
			}

			int const nfree = QUEUE_LENGTH - h->level;

			if (nfree <= 0)
			{
				h->wblocked = true;
				do_wait = true;
			}
			else
			{
				chunk_t * const pc = &h->buf[h->wpos];

				if (pdo != NULL) {
					devobj_addref(pdo);
				}

				pc->pdo = pdo;
				pc->ndata = ndata;

				if (pdata != NULL) {
					memcpy(&pc->data[0], pdata, ndata);
				}

				h->wpos += 1;

				if (h->wpos >= QUEUE_LENGTH) {
					h->wpos -= QUEUE_LENGTH;
				}

				h->level += 1;

				h->wblocked = false;
				do_unblock = h->rblocked;
			}
		}

		// if the reader is blocked (because the queue was empty), signal that there is now data available

		if (do_unblock) {
			SetEvent(h->hwevent);
		}

		if (!do_wait) {
			return ndata;
		}

		// if we are here, the queue is full and we have to wait until the consumer reads something

		WaitForSingleObject(h->hrevent, INFINITE);
	}
}

static size_t queue_pop(HQUEUE hqueue, devobj **ppdo, uint8_t *pbuffer, size_t nsize)
{
	queue_t * const h = (queue_t*)hqueue;

	if (ppdo == NULL || pbuffer == NULL || nsize == 0 || nsize < sizeof(h->buf[0].data)) {
		return 0;
	}

	for (;;)
	{
		bool do_wait = false;
		bool do_unblock = false;
		size_t nread = 0;

		// check if there is some data to read from the queue

		{
			AUTOLOCK(h->cs);

			if (h->state != 0) {
				return 0;
			}

			if (h->level <= 0)
			{
				h->rblocked = true;
				do_wait = true;
			}
			else
			{
				chunk_t * const pc = &h->buf[h->rpos];

				*ppdo = pc->pdo;
				pc->pdo = NULL;

				if (pc->ndata > 0) 
				{
					memcpy(pbuffer, &pc->data[0], pc->ndata);
				}
				else 
				{
					h->state = 1; 
				}

				nread = pc->ndata;

				h->rpos += 1;

				if (h->rpos >= QUEUE_LENGTH) {
					h->rpos -= QUEUE_LENGTH;
				}

				h->level -= 1;

				if (h->level == 0 && h->eblocked) {
					SetEvent(h->heevent);
				}

				h->rblocked = false;
				do_unblock = h->wblocked;
			}
		}

		// if the writer is blocked (because the queue was full), signal that there is now some free space

		if (do_unblock) {
			SetEvent(h->hrevent);
		}

		if (!do_wait) {
			return nread;
		}

		// if we are here, the queue is empty and we have to wait until the producer writes something

		WaitForSingleObject(h->hwevent, INFINITE);
	}
}


static devobj * devobj_create(HANDLE hdev)
{
	if (hdev == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	devobj * const pdo = (devobj*)malloc(sizeof(devobj));

	if (pdo == NULL)
	{
		CloseHandle(hdev);
		return NULL;
	}

	memset(pdo, 0x00, sizeof(devobj));
	pdo->hdev = hdev;
	pdo->refcount = 1;

	return pdo;
}

static void devobj_release(devobj *pdo)
{
	LONG refcount_new = InterlockedDecrement(&pdo->refcount);

	if (refcount_new <= 0)
	{
		CloseHandle(pdo->hdev);
		free(pdo);
	}
}

static void devobj_addref(devobj *pdo)
{
	InterlockedIncrement(&pdo->refcount);
}
