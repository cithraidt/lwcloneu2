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

#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <util/atomic.h>
#include <util/delay.h>

#include <LUFA/Drivers/USB/USB.h>
#include "descriptors.h"

#include <hwconfig.h>
#include "comm.h"
#include "led.h"
#include "panel.h"


#define LWCCONFIG_CMD_SETID 65
#define LWCCONFIG_CMD_DFU   66

#define LWC_CONFIG_IDENTIFIER 0xA62817B2   // some magic number(s)
#define RESETSTATE_BOOTLOADER 0x42B8217C

#define OFFSET_OF(_struct_, _member_) (((uint8_t*)&(((_struct_*)NULL)->_member_)) - (uint8_t*)NULL)


uint32_t g_resetstate __attribute__ ((section (".noinit")));

typedef struct {
	uint32_t id; 
	uint8_t ledwiz_id;
	uint8_t reserved[7];
} lwc_config_t;

static struct {
	uint8_t reserved[16];
	uint8_t configdata[sizeof(lwc_config_t)];
} g_eeprom_table EEMEM;


static void hardware_init(void);
static void main_task(void);
static uint8_t* buffer_lock(void);
static void buffer_unlock(void);
static void hardware_restart(bool enter_bootloader);
static void configure_device(void);


// Main program entry point. This routine configures the hardware required by the application, then
// enters a loop to run the application tasks in sequence.

int main(void)
{
	hardware_init();
	USB_Init();

	sei();

	DbgOut(DBGINFO, "enter main loop");

	do {
		USB_USBTask();
	} while (USB_DeviceState != DEVICE_STATE_Configured);

	for (;;)
	{
		USB_USBTask();
		main_task();
		sleep_ms(0);
	}
}


static void hardware_init(void)
{
	uint8_t const mcusr = MCUSR; // save status register

	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	#if defined(BOOTLOADER_START_ADDR)
	// if that was a reset due to watchdog timeout that we issued, call the bootloader
	if ((mcusr & (1 << WDRF)) && 
	    (g_resetstate == RESETSTATE_BOOTLOADER))
	{
		g_resetstate = 0;
		void (*bootloader)(void) = (void*)(BOOTLOADER_START_ADDR);
		bootloader();
	}
	#endif

	// Disable clock division
	clock_prescale_set(clock_div_1);

	set_sleep_mode(SLEEP_MODE_IDLE);

	// initialize LED driver and UART and panel
	clock_init();
	comm_init();
	led_init();
	panel_init();

	// config
	configure_device();
}


// load current configuration from eeprom and set USB product ID

static void configure_device(void)
{
	lwc_config_t cfg;
	eeprom_read_block((void*)&cfg, g_eeprom_table.configdata, sizeof(cfg));

	if (cfg.id != LWC_CONFIG_IDENTIFIER)
	{
		memset(&cfg, 0x00, sizeof(cfg));
		cfg.id = LWC_CONFIG_IDENTIFIER;
		cfg.ledwiz_id = USB_PRODUCT_ID & 0x0F;

		eeprom_write_block((void*)&cfg, g_eeprom_table.configdata, sizeof(cfg));
	}

	// set USB product ID
	uint16_t const product_id = (USB_PRODUCT_ID & ~0x000F) | (cfg.ledwiz_id & 0x0F);
	SetProductID(product_id);
}


static void main_task(void)
{
#if defined(ENABLE_PANEL_DEVICE)

	/* Select the Joystick Report Endpoint */
	Endpoint_SelectEndpoint(PANEL_EPADDR);

	/* Check to see if the host is ready for another packet */
	if (!Endpoint_IsINReady())
		return;

	#if defined(DATA_RX_UART_vect)

	msg_t * const pmsg = msg_recv();

	if (pmsg != NULL)
	{
		DbgOut(DBGINFO, "main_usb, message received");

		// is the message valid?

		if (pmsg->nlen < 2 || pmsg->nlen > 8)
		{
			DbgOut(DBGERROR, "main_usb, invalid framesize");
		}
		else
		{
			/* Write Joystick Report Data */
			Endpoint_Write_Stream_LE(&pmsg->data[0], pmsg->nlen, NULL);

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();
		}

		msg_release();
	}

	#elif defined(PANEL_TASK)

	uint8_t * pdata;
	uint8_t const ndata = panel_get_report(&pdata);

	if (ndata > 0)
	{
		/* Write Joystick Report Data */
		Endpoint_Write_Stream_LE(pdata, ndata, NULL);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
	}

	#else

	#error "invalid configuration, panel is enabled but no uart-rx-ISR or panel-timer-ISR"

	#endif

#endif
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
	// Setup HID Report Endpoint

	Endpoint_ConfigureEndpoint(MISC_EPADDR, EP_TYPE_INTERRUPT, MISC_EPSIZE, 1);
	#if defined(ENABLE_LED_DEVICE)
	Endpoint_ConfigureEndpoint(LED_EPADDR, EP_TYPE_INTERRUPT, LED_EPSIZE, 1);
	#endif
	#if defined(ENABLE_PANEL_DEVICE)
	Endpoint_ConfigureEndpoint(PANEL_EPADDR, EP_TYPE_INTERRUPT, PANEL_EPSIZE, 1);
	#endif
}

// Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
// the device from the USB host before passing along unhandled control requests to the library for processing
// internally.
 
void EVENT_USB_Device_ControlRequest(void)
{
	// Handle HID Class specific requests
	switch (USB_ControlRequest.bRequest)
	{
	case HID_REQ_GetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			Endpoint_ClearSETUP();

			uint8_t zero = 0;

			// Write one 'zero' byte report data to the control endpoint
			Endpoint_Write_Control_Stream_LE(&zero, 1);
			Endpoint_ClearOUT();
		}
		break;

	#if defined(ENABLE_LED_DEVICE)
	case HID_REQ_SetReport:
		if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
		{
			Endpoint_ClearSETUP();

			DbgOut(DBGINFO, "HID_REQ_SetReport, bRequest: 0x%02X, wIndex: %d, wLength: %d, wValue: %d",
				USB_ControlRequest.bRequest, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, USB_ControlRequest.wValue);

			uint8_t * const pdata = buffer_lock();

			if (pdata != NULL)
			{
				// Read the report data from the control endpoint
				Endpoint_Read_Control_Stream_LE(pdata, 8);

				DbgOut(DBGINFO, "HID_REQ_SetReport: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", 
					pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);

				// if this is a special command to set the ledwiz ID, execute it
				if (pdata[0] == LWCCONFIG_CMD_SETID)
				{
					const uint8_t id = pdata[1];
					const uint8_t check = ~id;

					if (pdata[2] == 0xFF &&
					    pdata[3] == 0xFF &&
					    pdata[4] == 0xFF &&
					    pdata[5] == 0xFF &&
					    pdata[6] == 0xFF &&
					    pdata[7] == check)
					{
						eeprom_update_byte(
							&g_eeprom_table.configdata[0] + OFFSET_OF(lwc_config_t, ledwiz_id),
							id & 0x0F);

						hardware_restart(false);
					}
				}

				buffer_unlock();
			}
			else
			{
				uint8_t temp[8];
				Endpoint_Read_Control_Stream_LE(temp, 8); // drop data

				DbgOut(DBGERROR, "HID_REQ_SetReport: buffer overflow");
			}

			Endpoint_ClearIN();
		}
		break;
	#endif
	}
}


static void hardware_restart(bool enter_bootloader)
{
	// detach from the bus
	USB_Disable();
	USB_Detach();

	// Disable all interrupts
	cli();

	// Wait some time for the USB detachment to register on the host
	_delay_ms(250);

	// force a reset via watchdog timeout
	wdt_enable(WDTO_15MS);

	// set persistant variable, so we know that we issued the reset
	g_resetstate = enter_bootloader ? RESETSTATE_BOOTLOADER : 0;

	for (;;) {;}
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
