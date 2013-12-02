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

#ifndef USBCONFIG_H__INCLUDED
#define USBCONFIG_H__INCLUDED


/****************************************
 USB device config
****************************************/

#define USB_VENDOR_ID      0xFAFA
#define USB_PRODUCT_ID     0x00F3        // this is used as the device identifier, 0x00F0 is '1' up to 0x00FF is '16'
#define USB_VERSION_BCD    VERSION_BCD(01.00)

#define ENABLE_LED_DEVICE

#define ENABLE_PANEL_DEVICE
#define NUM_JOYSTICKS 2
#define USE_MOUSE 0
#define USE_CONSUMER 1
#define USE_KEYBOARD 1



#endif
