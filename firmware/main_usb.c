/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
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

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <string.h>
 
#include <LUFA/Drivers/USB/USB.h>
#include "descriptors.h"

#include <hwconfig.h>
#include "comm.h"
#include "led.h"


static void hardware_init(void);
static void main_task(void);
static uint8_t* buffer_lock(void);
static void buffer_unlock(void);


// Main program entry point. This routine configures the hardware required by the application, then
// enters a loop to run the application tasks in sequence.

int main(void)
{
	hardware_init();
	USB_Init();

	sei();

	DbgOut(DBGINFO, "enter main loop");

	for (;;)
	{
		main_task();
		USB_USBTask();
	}
}


static void hardware_init(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	set_sleep_mode(SLEEP_MODE_IDLE);

	// initialize LED driver and UART
	comm_init();
	led_init();
}


static void main_task(void)
{
	// for now nothing to do here
}


// Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
// starts the library USB task to begin the enumeration and USB management process.
 
void EVENT_USB_Device_Connect(void)
{
}

// Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
// the status LEDs and stops the USB management and joystick reporting tasks.
 
void EVENT_USB_Device_Disconnect(void)
{
}

// Event handler for the library USB Configuration Changed event.

void EVENT_USB_Device_ConfigurationChanged(void)
{
	/* Setup HID Report Endpoint */

	Endpoint_ConfigureEndpoint(
		GENERIC_IN_EPADDR,
		EP_TYPE_INTERRUPT,
		GENERIC_EPSIZE,
		1);
}

// Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
// the device from the USB host before passing along unhandled control requests to the library for processing
// internally.
 
void EVENT_USB_Device_ControlRequest(void)
{
	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
	case HID_REQ_GetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			Endpoint_ClearSETUP();

			uint8_t zero = 0;

			/* Write one 'zero' byte report data to the control endpoint */
			Endpoint_Write_Control_Stream_LE(&zero, 1);
			Endpoint_ClearOUT();
		}
		break;

	case HID_REQ_SetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			Endpoint_ClearSETUP();

			uint8_t * const pdata = buffer_lock();

			if (pdata != NULL)
			{
				/* Read the report data from the control endpoint */
				Endpoint_Read_Control_Stream_LE(pdata, 8);

				DbgOut(DBGINFO, "HID_REQ_SetReport: %02x%02x%02x%02x%02x%02x%02x%02x", 
					pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);

				buffer_unlock();
			}
			else
			{
				DbgOut(DBGERROR, "HID_REQ_SetReport: buffer overflow");
			}

			Endpoint_ClearIN();
		}
		break;
	}
}


#if defined(LED_TIMER_vect)

static uint8_t g_databuffer[8];

static uint8_t * buffer_lock(void)
{
	return &g_databuffer[0];
}

void buffer_unlock(void)
{
	led_update(&g_databuffer[0]);
}

#endif


#if defined(DATA_TX_UART_vect)

static uint8_t * buffer_lock(void)
{
	msg_t * const pmsg = msg_prepare();

	if (pmsg == NULL) {
		return NULL;
	}

	pmsg->nlen = 8;

	return &pmsg->data[0];
}

static void buffer_unlock(void)
{
	msg_send();
}

#endif
