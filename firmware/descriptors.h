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

/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for Descriptors.c.
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

/* Includes: */
#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>


#define USB_VENDOR_ID      0xFAFA
#define USB_PRODUCT_ID     0x00F3        // this is used as the device identifier, 0x00F0 is '1' up to 0x00FF is '16'
#define USB_VERSION_BCD    VERSION_BCD(01.00)


/* Type Defines: */
/** Type define for the device configuration descriptor structure. This must be defined in the
 *  application code, as the configuration descriptor contains several sub-descriptors which
 *  vary between devices, and which describe the device's usage to the host.
 */
typedef struct
{
	USB_Descriptor_Configuration_Header_t  Config;
	USB_Descriptor_Interface_t             HID_Interface;
	USB_HID_Descriptor_HID_t               HID_GenericHID;
	USB_Descriptor_Endpoint_t              HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;

/* Macros: */
/** Endpoint address of the Generic HID reporting IN endpoint. */
#define GENERIC_IN_EPADDR         (ENDPOINT_DIR_IN  | 1)

/** Size in bytes of the HID reporting IN endpoint. */
#define GENERIC_EPSIZE           8

/** Descriptor header type value, to indicate a HID class HID descriptor. */
#define DTYPE_HID                 0x21

/** Descriptor header type value, to indicate a HID class HID report descriptor. */
#define DTYPE_Report              0x22

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t wValue,
	const uint8_t wIndex,
	const void** const DescriptorAddress,
	uint8_t *const DescriptorMemorySpace)
	ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

void SetProductID(uint16_t id);



#endif

