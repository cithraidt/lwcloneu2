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
#include "usbdev.h"

#define USE_SEPERATE_IO_THREAD


const GUID HIDguid = { 0x4d1e55b2, 0xf16f, 0x11Cf, { 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

USHORT const VendorID_LEDWiz       = 0xFAFA;
USHORT const ProductID_LEDWiz_min  = 0x00F0;
USHORT const ProductID_LEDWiz_max  = ProductID_LEDWiz_min + LWZ_MAX_DEVICES - 1;

static const char * lwz_process_sync_mutex_name = "lwz_process_sync_mutex";

typedef struct {
	HUDEV hudev;
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
static HUDEV lwz_get_hdev(lwz_context_t *h, int indx_user);
static void lwz_notify_callback(lwz_context_t *h, int reason, LWZHANDLE hlwz);

static void lwz_refreshlist_attached(lwz_context_t *h);
static void lwz_refreshlist_detached(lwz_context_t *h);
static void lwz_freelist(lwz_context_t *h);
static void lwz_add(lwz_context_t *h, int indx);
static void lwz_remove(lwz_context_t *h, int indx);

static void queue_close(HQUEUE hqueue, bool unload);
static HQUEUE queue_open(void);
static size_t queue_push(HQUEUE hqueue, HUDEV hudev, uint8_t const *pdata, size_t ndata);
static size_t queue_pop(HQUEUE hqueue, HUDEV *phudev, uint8_t *pbuffer, size_t nsize);
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

	HUDEV hudev = lwz_get_hdev(g_plwz, indx);

	if (hudev == NULL) {
		return;
	}

	BYTE data[8];
	data[0] = 0x40; // LWZ_SBA command identifier
	data[1] = bank0;
	data[2] = bank1;
	data[3] = bank2;
	data[4] = bank3;
	data[5] = globalPulseSpeed;
	data[6] = 0;
	data[7] = 0;

	#if defined(USE_SEPERATE_IO_THREAD)

	queue_push(g_plwz->hqueue, hudev, &data[0], 8);

	#else

	usbdev_write(hudev, &data[0], 8);

	#endif

}

void LWZ_PBA(LWZHANDLE hlwz, BYTE const *pbrightness_32bytes)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pbrightness_32bytes == NULL)
		return;

	HUDEV hudev = lwz_get_hdev(g_plwz, indx);

	if (hudev == NULL) {
		return;
	}

	#if defined(USE_SEPERATE_IO_THREAD)

	queue_push(g_plwz->hqueue, hudev, pbrightness_32bytes, 32);

	#else

	usbdev_write(hudev, pbrightness_32bytes, 32);

	#endif
}

DWORD LWZ_RAWWRITE(LWZHANDLE hlwz, BYTE const *pdata, DWORD ndata)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pdata == NULL || ndata == 0)
		return 0;

	if (ndata > 32)
	    ndata = 32;

	HUDEV hudev = lwz_get_hdev(g_plwz, indx);

	if (hudev == NULL) {
		return 0;
	}

	int res = 0;
	DWORD nbyteswritten = 0;

	#if defined(USE_SEPERATE_IO_THREAD)

	nbyteswritten = queue_push(g_plwz->hqueue, hudev, pdata, ndata);

	#else

	usbdev_write(hudev, pdata, ndata);

	#endif

	return nbyteswritten;
}

DWORD LWZ_RAWREAD(LWZHANDLE hlwz, BYTE *pdata, DWORD ndata)
{
	AUTOLOCK(g_cs);

	int indx = hlwz - 1;

	if (pdata == NULL)
		return 0;

	if (ndata > 64)
	    ndata = 64;

	HUDEV hudev = lwz_get_hdev(g_plwz, indx);

	if (hudev == NULL) {
		return 0;
	}

	#if defined(USE_SEPERATE_IO_THREAD)
	queue_wait_empty(g_plwz->hqueue);
	#endif

	return usbdev_read(hudev, pdata, ndata);
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
		InitializeCriticalSection(&g_cs);

		g_plwz = lwz_open(hinstDLL);

		if (g_plwz == NULL)
		{
			DeleteCriticalSection(&g_cs);
			return FALSE;
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		{
			AUTOLOCK(g_cs);

			lwz_close(g_plwz);
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

		if (h->devices[indx].hudev == NULL) {
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

static HUDEV lwz_get_hdev(lwz_context_t *h, int indx)
{
	if (indx < 0 ||
	    indx >= LWZ_MAX_DEVICES)
	{
		return NULL;
	}

	return h->devices[indx].hudev;
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
		if (h->devices[i].hudev != NULL)
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

			if (hdev == INVALID_HANDLE_VALUE)
			{
				usbdev_release(h->devices[i].hudev);
				h->devices[i].hudev = NULL;

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

		device_tmp.hudev = usbdev_create(pdiddat->DevicePath);

		if (device_tmp.hudev != NULL)
		{
			HIDD_ATTRIBUTES attrib = {};
			attrib.Size = sizeof(HIDD_ATTRIBUTES);

			BOOLEAN bSuccess = HidD_GetAttributes(
				usbdev_handle(device_tmp.hudev),
				&attrib);

			// if this is the VID/PID we are interested in
			// check some additional properties

			int indx = (int)attrib.ProductID - (int)ProductID_LEDWiz_min;

			if (bSuccess && 
			    attrib.VendorID == VendorID_LEDWiz &&
			    indx >= 0 && indx < LWZ_MAX_DEVICES)
			{
				PHIDP_PREPARSED_DATA p_prepdata = NULL;

				if (HidD_GetPreparsedData(usbdev_handle(device_tmp.hudev), &p_prepdata) == TRUE)
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
							if (h->devices[indx].hudev == NULL)
							{
								memcpy(&h->devices[indx], &device_tmp, sizeof(device_tmp));
								device_tmp.hudev = NULL;

								lwz_add(h, indx);

								num_devices++;
							}
						}
					}

					HidD_FreePreparsedData(p_prepdata);
				}
			}

			if (device_tmp.hudev != NULL)
			{
				usbdev_release(device_tmp.hudev);
				device_tmp.hudev = NULL;
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
		if (h->devices[i].hudev != NULL)
		{
			usbdev_release(h->devices[i].hudev);
			h->devices[i].hudev = NULL;
		}
	}
}


// simple fifo to move the WriteFile() calls to a seperate thread

typedef struct {
	HUDEV hudev;
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

		HUDEV hudev = NULL;
		size_t ndata = queue_pop(h, &hudev, &buffer[0], sizeof(buffer));

		// exit thread if required

		if (ndata == 0 || hudev == NULL) {
			break;
		}

		usbdev_write(hudev, &buffer[0], ndata);
		usbdev_release(hudev);
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

			if (h->level == 0 && h->rblocked)
			{
				h->eblocked = false;
				return;
			}

			h->eblocked = true;
		}

		WaitForSingleObject(h->heevent, INFINITE);
	}
}

static size_t queue_push(HQUEUE hqueue, HUDEV hudev, uint8_t const *pdata, size_t ndata)
{
	queue_t * const h = (queue_t*)hqueue;

	if (pdata == NULL || ndata == 0 || ndata > sizeof(h->buf[0].data)) 
	{
		// push empty chunk to signal shutdown

		pdata = NULL;
		ndata = 0;
		hudev = NULL;
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

				if (hudev != NULL) {
					usbdev_addref(hudev);
				}

				pc->hudev = hudev;
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

static size_t queue_pop(HQUEUE hqueue, HUDEV *phudev, uint8_t *pbuffer, size_t nsize)
{
	queue_t * const h = (queue_t*)hqueue;

	if (phudev == NULL || pbuffer == NULL || nsize == 0 || nsize < sizeof(h->buf[0].data)) {
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

				if (h->eblocked) {
					SetEvent(h->heevent);
				}
			}
			else
			{
				chunk_t * const pc = &h->buf[h->rpos];

				*phudev = pc->hudev;
				pc->hudev = NULL;

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
