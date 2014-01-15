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

#include <crtdbg.h>
#include <windows.h>
#include "usbdev.h"


static void usbdev_close_internal(HUDEV hudev);

#define USB_READ_TIMOUT_MS 500



struct CAutoLockCS  // helper class to lock a critical section, and unlock it automatically
{
    CRITICAL_SECTION *m_pcs;
	CAutoLockCS(CRITICAL_SECTION *pcs) { m_pcs = pcs; EnterCriticalSection(m_pcs); };
	~CAutoLockCS() { LeaveCriticalSection(m_pcs); };
};

#define AUTOLOCK(cs) CAutoLockCS lock_##__LINE__##__(&cs)   // helper macro for using the helper class



typedef struct {
	CRITICAL_SECTION cslock;
	HANDLE hrevent;
	HANDLE hwevent;
	HANDLE hdev;
	LONG refcount;
} usbdev_context_t;


HUDEV usbdev_create(LPCSTR devicepath)
{
	// create context

	usbdev_context_t * const h = (usbdev_context_t*)malloc(sizeof(usbdev_context_t));

	if (h == NULL)
		return NULL;

	memset(h, 0x00, sizeof(*h));
	h->hdev = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&h->cslock);

	h->hrevent = CreateEvent(NULL, TRUE, FALSE, NULL);
	h->hwevent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (h->hrevent == NULL ||
		h->hwevent == NULL)
	{
		goto Failed;
	}

	// open device

	HANDLE hdev  = CreateFileA(
		devicepath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (hdev != INVALID_HANDLE_VALUE)
	{
		h->hdev = hdev;
		h->refcount = 1;
		return h;
	}

Failed:
	usbdev_close_internal(h);
	return NULL;
}

static void usbdev_close_internal(HUDEV hudev)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;

	if (h == NULL)
		return;

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

	if (h->hdev != INVALID_HANDLE_VALUE)
	{
		CloseHandle(h->hdev);
		h->hdev = INVALID_HANDLE_VALUE;
	}

	DeleteCriticalSection(&h->cslock);

	free(h);
}

void usbdev_release(HUDEV hudev)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;

	if (h != NULL)
	{
		LONG refcount_new = InterlockedDecrement(&h->refcount);

		if (refcount_new <= 0)
		{
			usbdev_close_internal(h);
		}
	}
}

void usbdev_addref(HUDEV hudev)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;

	if (h != NULL)
	{
		InterlockedIncrement(&h->refcount);
	}
}

HANDLE usbdev_handle(HUDEV hudev)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;
	
	if (h == NULL)
		return INVALID_HANDLE_VALUE;

	return h->hdev;
}

size_t usbdev_read(HUDEV hudev, void *psrc, size_t ndata)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;

	if (h == NULL)
		return NULL;

	BYTE * pdata = (BYTE*)psrc;

	if (pdata == NULL)
		return 0;

	if (ndata > 64)
	    ndata = 64;

	AUTOLOCK(h->cslock);

	int res = 0;
	BYTE buffer[65];
	DWORD nread = 0;
	BOOL bres = FALSE;

	OVERLAPPED ol = {};
	ol.hEvent = h->hrevent;

	bres = ReadFile(h->hdev, buffer, sizeof(buffer), NULL, &ol);

	if (bres != TRUE)
	{
		DWORD dwerror = GetLastError();

		if (dwerror == ERROR_IO_PENDING)
		{
			if (WaitForSingleObject(h->hrevent, USB_READ_TIMOUT_MS) != WAIT_OBJECT_0)
			{
				CancelIo(h->hdev);
			}

			bres = TRUE;
		}
	}

	if (bres == TRUE)
	{
		bres = GetOverlappedResult(
			h->hdev,
			&ol,
			&nread,
			TRUE);
	}

	if (bres != TRUE)
	{
		DWORD dwerror = GetLastError();
		_ASSERT(0);
	}

	if (nread <= 1 || bres != TRUE)
		return 0;

	nread -= 1; // skip report id

	if (ndata > nread)
	    ndata = nread;

	memcpy(pdata, &buffer[1], ndata);

	return ndata;
}

size_t usbdev_write(HUDEV hudev, void const *pdst, size_t ndata)
{
	usbdev_context_t * const h = (usbdev_context_t*)hudev;
	
	if (h == NULL)
		return NULL;

	BYTE const * pdata = (BYTE const *)pdst;

	if (pdata == NULL || ndata == 0)
		return 0;

	if (ndata > 32)
	    ndata = 32;

	AUTOLOCK(h->cslock);

	int res = 0;
	DWORD nbyteswritten = 0;

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

		OVERLAPPED ol = {};
		ol.hEvent = h->hwevent;

		BOOL bres = WriteFile(h->hdev, buf, nwrite, NULL, &ol);

		if (bres != TRUE)
		{
			DWORD dwerror = GetLastError();

			if (dwerror == ERROR_IO_PENDING)
			{
				if (WaitForSingleObject(h->hwevent, USB_READ_TIMOUT_MS) != WAIT_OBJECT_0)
				{
					CancelIo(h->hdev);
				}

				bres = TRUE;
			}
		}

		if (bres == TRUE)
		{
			bres = GetOverlappedResult(
				h->hdev,
				&ol,
				&nwritten,
				TRUE);
		}

		if (bres != TRUE)
		{
			DWORD dwerror = GetLastError();
			_ASSERT(0);
		}

		if (nwritten != nwrite || bres != TRUE) {
			break;
		}

		nbyteswritten += ncopy;
	}

	return nbyteswritten;
}

