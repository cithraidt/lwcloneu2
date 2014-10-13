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

#include "descriptors.h"
#include "panel.h"

#define USB_STRING_TABLE(_map_) \
	_map_(ManufacturerString_id,  "n/a") \
	_map_(ProductString_id,       "LWCloneU2 v1.0")


typedef enum {
	LanguageString_id = 0,
	#define MAP(x, y) x,
	USB_STRING_TABLE(MAP)
	#undef MAP
	SerialNumberString_id
} UsbStringsEnum;

static const USB_Descriptor_String_t PROGMEM LanguageString =
{
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},
	.UnicodeString          = {LANGUAGE_ID_ENG}
};

#define MAP(name, str) \
static const USB_Descriptor_String_t PROGMEM name##__str_ = \
{ \
	.Header                 = {.Size = USB_STRING_LEN(sizeof(str) - 1), .Type = DTYPE_String}, \
	.UnicodeString          = L##str \
};
USB_STRING_TABLE(MAP)
#undef MAP


/** HID class report descriptor. This is a special descriptor constructed with values from the
 *  USBIF HID class specification to describe the reports and capabilities of the HID device. This
 *  descriptor is parsed by the host and its contents used to determine what data (and in what encoding)
 *  the device will send, and what it may be sent back from the host. Refer to the HID specification for
 *  more details on HID report descriptors.
 */

static const USB_Descriptor_HIDReport_Datatype_t PROGMEM MiscReport[] =
{
	HID_RI_USAGE_PAGE(16, 0xFF00), /* Vendor Page 0 */
	HID_RI_USAGE(8, 0x01), /* Vendor Usage 1 */
	HID_RI_COLLECTION(8, 0x01), /* Vendor Usage 1 */
		HID_RI_LOGICAL_MINIMUM(8, 0x00),
		HID_RI_LOGICAL_MAXIMUM(8, 0xFF),
		HID_RI_REPORT_SIZE(8, 0x08),
		HID_RI_REPORT_COUNT(8, 64),
		HID_RI_USAGE(8, 0x01), /* Vendor Usage 1 */
		HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
		HID_RI_REPORT_COUNT(8, 8),
		HID_RI_USAGE(8, 0x01), /* Vendor Usage 1 */
		HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NON_VOLATILE),
	HID_RI_END_COLLECTION(0),
};


#if defined(ENABLE_PANEL_DEVICE)

#if !defined(USE_KEYBOARD)
#define USE_KEYBOARD 0
#endif

#if !defined(USE_CONSUMER)
#define USE_CONSUMER 0
#endif

#if !defined(NUM_JOYSTICKS)
#define NUM_JOYSTICKS 0
#endif

#if !defined(USE_ACCELGYRO)
#define USE_ACCELGYRO 0
#endif

#if !defined(USE_MOUSE)
#define USE_MOUSE 0
#endif

enum HID_KeyModifierCodes
{
	KEY_LeftControl      = 0xE0,
	KEY_LeftShift,
	KEY_LeftAlt,
	KEY_LeftGUI,
	KEY_RightControl,
	KEY_RightShift,
	KEY_RightAlt,
	KEY_RightGUI
};

enum HID_ConsumerCodes
{
	CKEY_Mute       = 0xE2,
	CKEY_VolumeUp   = 0xE9,
	CKEY_VolumeDown = 0xEA,
};


const USB_Descriptor_HIDReport_Datatype_t PROGMEM PanelReport[] =
{
	#if (USE_KEYBOARD != 0)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,             // USAGE (Keyboard)
	0xa1, 0x01,             // COLLECTION (Application)
	0x85, ID_Keyboard,      //   REPORT_ID (ID_Keyboard)
	0x05, 0x07,             //   USAGE_PAGE (Keyboard)
	0x19, KEY_LeftControl,  //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, KEY_RightGUI,     //   USAGE_MAXIMUM (Keyboard RightGUI)
	0x15, 0x00,             //   LOGICAL_MINIMUM (0)
	0x25, 0x01,             //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,             //   REPORT_SIZE (1)
	0x95, 0x08,             //   REPORT_COUNT (8)
	0x81, 0x02,             //   INPUT (Data,Var,Abs)
	0x95, 0x06,             //   REPORT_COUNT (6)
	0x75, 0x08,             //   REPORT_SIZE (8)
	0x25, 0x65,             //   LOGICAL_MAXIMUM (101)
	0x19, 0x00,             //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,             //   USAGE_MAXIMUM (101)
	0x81, 0x00,             //   INPUT (Data,Ary,Abs)
	0xc0,                   // END_COLLECTION
	#endif

	#if (USE_CONSUMER != 0)
	0x05, 0x0C,             // USAGE_PAGE (Consumer Devices)
	0x09, 0x01,             // USAGE (Consumer Control)
	0xa1, 0x01,             // COLLECTION (Application)
	0x85, ID_Consumer,      //   REPORT_ID (ID_Consumer)
	0x15, 0x00,             //   LOGICAL_MINIMUM (0)
	0x25, 0x01,             //   LOGICAL_MAXIMUM (1)
	0x09, CKEY_VolumeUp,    //   USAGE (Volume Up)
	0x09, CKEY_VolumeDown,  //   USAGE (Volume Down)
	0x75, 0x01,             //   REPORT_SIZE (1)
	0x95, 0x02,             //   REPORT_COUNT (2)
	0x81, 0x02,             //   INPUT (Data,Var,Abs)
	0x09, CKEY_Mute,        //   USAGE (Mute)
	0x95, 0x01,             //   REPORT_COUNT (1)
	0x81, 0x06,             //   INPUT (Data,Var,Rel)
	0x95, 0x05,             //   REPORT_COUNT (5)
	0x81, 0x03,             //   INPUT (Cnst,Var,Abs)
	0xc0,                   // END_COLLECTION
	#endif

	#if (NUM_JOYSTICKS >= 1)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,             // USAGE (Gamepad)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_Joystick1,     //     REPORT_ID (ID_Joystick1)
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	HID_RI_LOGICAL_MINIMUM(16, -2047),
	HID_RI_LOGICAL_MAXIMUM(16, 2047),
	0x75, 0x0C,             //     REPORT_SIZE (12)
	0x95, 0x02,             //     REPORT_COUNT (2)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	HID_RI_LOGICAL_MINIMUM(8, 0),
	HID_RI_LOGICAL_MAXIMUM(8, 1),
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x95, 0x08,             //     REPORT_COUNT (8)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,             //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif

	#if (NUM_JOYSTICKS >= 2)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,             // USAGE (Gamepad)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_Joystick2,     //     REPORT_ID (ID_Joystick2)
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	HID_RI_LOGICAL_MINIMUM(16, -2047),
	HID_RI_LOGICAL_MAXIMUM(16, 2047),
	0x75, 0x0C,             //     REPORT_SIZE (12)
	0x95, 0x02,             //     REPORT_COUNT (2)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	HID_RI_LOGICAL_MINIMUM(8, 0),
	HID_RI_LOGICAL_MAXIMUM(8, 1),
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x95, 0x08,             //     REPORT_COUNT (8)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,             //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif

	#if (NUM_JOYSTICKS >= 3)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,             // USAGE (Gamepad)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_Joystick3,     //     REPORT_ID (ID_Joystick3)
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	HID_RI_LOGICAL_MINIMUM(16, -2047),
	HID_RI_LOGICAL_MAXIMUM(16, 2047),
	0x75, 0x0C,             //     REPORT_SIZE (12)
	0x95, 0x02,             //     REPORT_COUNT (2)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	HID_RI_LOGICAL_MINIMUM(8, 0),
	HID_RI_LOGICAL_MAXIMUM(8, 1),
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x95, 0x08,             //     REPORT_COUNT (8)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,             //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif

	#if (NUM_JOYSTICKS >= 4)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,             // USAGE (Gamepad)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_Joystick4,     //     REPORT_ID (ID_Joystick4)
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	HID_RI_LOGICAL_MINIMUM(16, -2047),
	HID_RI_LOGICAL_MAXIMUM(16, 2047),
	0x75, 0x0C,             //     REPORT_SIZE (12)
	0x95, 0x02,             //     REPORT_COUNT (2)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	HID_RI_LOGICAL_MINIMUM(8, 0),
	HID_RI_LOGICAL_MAXIMUM(8, 1),
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x95, 0x08,             //     REPORT_COUNT (8)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,             //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif // NUM_JOYSTICKS >= 4

	#if (USE_ACCELGYRO)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,             // USAGE (Gamepad)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_AccelGyro,     //     REPORT_ID
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	0x09, 0x32,             //     USAGE (Z)
	0x09, 0x33,             //     USAGE (Rx)
	0x09, 0x34,             //     USAGE (Ry)
	0x09, 0x35,             //     USAGE (Rz)
	HID_RI_LOGICAL_MINIMUM(8, -127),
	HID_RI_LOGICAL_MAXIMUM(8, 127),
	0x75, 0x08,             //     REPORT_SIZE (8)
	0x95, 0x06,             //     REPORT_COUNT (6)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	HID_RI_LOGICAL_MINIMUM(8, 0),
	HID_RI_LOGICAL_MAXIMUM(8, 1),
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x95, 0x08,             //     REPORT_COUNT (8)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,             //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif

	#if (USE_MOUSE != 0)
	0x05, 0x01,             // USAGE_PAGE (Generic Desktop)
	0x09, 0x02,             // USAGE (Mouse)
	0xa1, 0x01,             // COLLECTION (Application)
	0x09, 0x01,             //   USAGE (Pointer)
	0xa1, 0x00,             //   COLLECTION (Physical)
	0x85, ID_Mouse,         //     REPORT_ID (ID_Mouse)
	0x05, 0x09,             //     USAGE_PAGE (Button)
	0x19, 0x01,             //     USAGE_MINIMUM (Button 1)
	0x29, 0x03,             //     USAGE_MAXIMUM (Button 3)
	0x15, 0x00,             //     LOGICAL_MINIMUM (0)
	0x25, 0x01,             //     LOGICAL_MAXIMUM (1)
	0x95, 0x03,             //     REPORT_COUNT (3)
	0x75, 0x01,             //     REPORT_SIZE (1)
	0x81, 0x02,             //     INPUT (Data,Var,Abs)
	0x95, 0x01,             //     REPORT_COUNT (1)
	0x75, 0x05,             //     REPORT_SIZE (5)
	0x81, 0x03,             //     INPUT (Cnst,Var,Abs)
	0x05, 0x01,             //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,             //     USAGE (X)
	0x09, 0x31,             //     USAGE (Y)
	0x15, 0x81,             //     LOGICAL_MINIMUM (-127)
	0x25, 0x7f,             //     LOGICAL_MAXIMUM (127)
	0x75, 0x08,             //     REPORT_SIZE (8)
	0x95, 0x02,             //     REPORT_COUNT (2)
	0x81, 0x06,             //     INPUT (Data,Var,Rel)
	0xc0,                   //   END_COLLECTION
	0xc0,                   // END_COLLECTION
	#endif
};

#endif


#if defined(ENABLE_LED_DEVICE)

#define LED_REPORT_SIZE 8

static const USB_Descriptor_HIDReport_Datatype_t PROGMEM LEDReport[] =
{
	HID_RI_USAGE_PAGE(16, 0xFF00), /* Vendor Page 0 */
	HID_RI_USAGE(8, 0x01), /* Vendor Usage 1 */
	HID_RI_COLLECTION(8, 0x01), /* Vendor Usage 1 */
		HID_RI_LOGICAL_MINIMUM(8, 0x00),
		HID_RI_LOGICAL_MAXIMUM(8, 0xFF),
		HID_RI_REPORT_SIZE(8, 0x08),
		HID_RI_REPORT_COUNT(8, 64),
		HID_RI_USAGE(8, 0x02), /* Vendor Usage 2 */
		HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
		HID_RI_REPORT_COUNT(8, LED_REPORT_SIZE),
		HID_RI_USAGE(8, 0x03), /* Vendor Usage 3 */
		HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NON_VOLATILE),
	HID_RI_END_COLLECTION(0),
};

#endif

/** Device descriptor structure. This descriptor describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
 
static USB_Descriptor_Device_t DeviceDescriptor =
{
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
	.USBSpecification       = XYZ_TO_BCD(1,1,0),
	.Class                  = USB_CSCP_NoDeviceClass,
	.SubClass               = USB_CSCP_NoDeviceSubclass,
	.Protocol               = USB_CSCP_NoDeviceProtocol,
	.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,
	.VendorID               = USB_VENDOR_ID,
	.ProductID              = USB_PRODUCT_ID,
	.ReleaseNumber          = 0,
	.ManufacturerStrIndex   = ManufacturerString_id,
	.ProductStrIndex        = ProductString_id,
	.SerialNumStrIndex      = SerialNumberString_id,
	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

static char toHex(uint8_t x)
{
	if (x < 10)
		return '0' + x;

	return x + 'A' - 10;
}

typedef enum {
	config_mask_joysticks  = 3 << 0,
	config_flag_keyboard   = 1 << 2,
	config_flag_mouse      = 1 << 3,
	config_flag_consumer   = 1 << 4,
	config_mask_panel      = config_mask_joysticks | config_flag_keyboard | config_flag_mouse | config_flag_consumer,
	config_flag_led        = 1 << 5,
} usb_config_flags;

static struct {
	USB_Descriptor_Header_t Header;
	uint16_t UnicodeString[15];
} SerialNumberString = {
	.Header = {
		.Size = USB_STRING_LEN(15), 
		.Type = DTYPE_String
	} 
};

void SetProductID(uint16_t id)
{
	uint8_t const ledwiz_id_minus1 = id & 0xFF;
	uint16_t const flags = 
		#if defined(ENABLE_PANEL_DEVICE)
		(config_mask_joysticks & NUM_JOYSTICKS) |
		(USE_KEYBOARD != 0 ? config_flag_keyboard : 0) |
		(USE_MOUSE != 0 ? config_flag_mouse : 0) |
		(USE_CONSUMER != 0 ? config_flag_consumer : 0) |
		#endif
		#if defined(ENABLE_LED_DEVICE)
		config_flag_led |
		#endif
		0;
	uint8_t const ver = LWCLONEU2_VERSION;
	uint16_t const rel = ((uint16_t)(ver & 0x1F)  << 8) | flags; // up to 13 bits (maximum is 9999)

	DeviceDescriptor.ProductID = id;
	DeviceDescriptor.ReleaseNumber = NUMBER_TO_BCD(rel);

	uint16_t * const c = &SerialNumberString.UnicodeString[0];
	c[0]  = 'L';
	c[1]  = 'W';
	c[2]  = 'C';
	c[3]  = '-';
	c[4]  = '0' + (((ver >>  8) & 0xFF) / 10);
	c[5]  = '0' + (((ver >>  8) & 0xFF) % 10);
	c[6]  = '0' + ((ver >>  4) & 0x0F);
	c[7]  = '0' + ((ver >>  0) & 0x0F);
	c[8]  = '-';
	c[9]  = toHex((ledwiz_id_minus1 >> 4) & 0x0F);
	c[10] = toHex((ledwiz_id_minus1 >> 0) & 0x0F);
	c[11] = '-';
	c[12] = toHex((flags >> 8) & 0x0F);
	c[13] = toHex((flags >> 4) & 0x0F);
	c[14] = toHex((flags >> 0) & 0x0F);
}

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */

typedef enum {
	IFACENUMBER_MISC,
	#if defined(ENABLE_PANEL_DEVICE)
	IFACENUMBER_PANEL,
	#endif
	#if defined(ENABLE_LED_DEVICE) 
	IFACENUMBER_LED,
	#endif
	NUM_TOTAL_INTERFACES,
} iface_numbers;
 
static const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
	.Config =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},
		.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces        = NUM_TOTAL_INTERFACES,
		.ConfigurationNumber    = 1,
		.ConfigurationStrIndex  = NO_DESCRIPTOR,
		.ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),
		.MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
	},

	.HID_MiscInterface =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},
		.InterfaceNumber        = IFACENUMBER_MISC,
		.AlternateSetting       = 0x00,
		.TotalEndpoints         = 1,
		.Class                  = HID_CSCP_HIDClass,
		.SubClass               = HID_CSCP_NonBootSubclass,
		.Protocol               = HID_CSCP_NonBootProtocol,
		.InterfaceStrIndex      = NO_DESCRIPTOR
	},

	.HID_MiscHID =
	{
		.Header                 = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},
		.HIDSpec                = XYZ_TO_BCD(1,1,1),
		.CountryCode            = 0x00,
		.TotalReportDescriptors = 1,
		.HIDReportType          = HID_DTYPE_Report,
		.HIDReportLength        = sizeof(LEDReport)
	},

	.HID_MiscReportINEndpoint =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
		.EndpointAddress        = MISC_EPADDR,
		.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = MISC_EPSIZE,
		.PollingIntervalMS      = MISC_INTERVAL_MS
	},

	#if defined(ENABLE_PANEL_DEVICE)
	.HID_PanelInterface =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},
		.InterfaceNumber        = IFACENUMBER_PANEL,
		.AlternateSetting       = 0x00,
		.TotalEndpoints         = 1,
		.Class                  = HID_CSCP_HIDClass,
		.SubClass               = HID_CSCP_NonBootSubclass,
		.Protocol               = HID_CSCP_NonBootProtocol,
		.InterfaceStrIndex      = NO_DESCRIPTOR
	},

	.HID_PanelHID =
	{
		.Header                 = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},
		.HIDSpec                = XYZ_TO_BCD(1,1,1),
		.CountryCode            = 0x00,
		.TotalReportDescriptors = 1,
		.HIDReportType          = HID_DTYPE_Report,
		.HIDReportLength        = sizeof(PanelReport)
	},

	.HID_PanelReportINEndpoint =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
		.EndpointAddress        = PANEL_EPADDR,
		.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = PANEL_EPSIZE,
		.PollingIntervalMS      = PANEL_INTERVAL_MS
	},
	#endif

	#if defined(ENABLE_LED_DEVICE)
	.HID_LEDInterface =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},
		.InterfaceNumber        = IFACENUMBER_LED,
		.AlternateSetting       = 0x00,
		.TotalEndpoints         = 1,
		.Class                  = HID_CSCP_HIDClass,
		.SubClass               = HID_CSCP_NonBootSubclass,
		.Protocol               = HID_CSCP_NonBootProtocol,
		.InterfaceStrIndex      = NO_DESCRIPTOR
	},

	.HID_LEDHID =
	{
		.Header                 = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},
		.HIDSpec                = XYZ_TO_BCD(1,1,1),
		.CountryCode            = 0x00,
		.TotalReportDescriptors = 1,
		.HIDReportType          = HID_DTYPE_Report,
		.HIDReportLength        = sizeof(LEDReport)
	},

	.HID_LEDReportINEndpoint =
	{
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},
		.EndpointAddress        = LED_EPADDR,
		.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = LED_EPSIZE,
		.PollingIntervalMS      = LED_INTERVAL_MS
	},
	#endif
};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
 
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t wValue,
	const uint8_t wIndex,
	const void** const DescriptorAddress,
	uint8_t *const DescriptorMemorySpace)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	uint8_t memspace;
	uint8_t * const pmemspace = (DescriptorMemorySpace != NULL) ? DescriptorMemorySpace : &memspace;

	*pmemspace = MEMSPACE_FLASH;

	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			*pmemspace = MEMSPACE_RAM;
			Address = &DeviceDescriptor;
			Size    = sizeof(USB_Descriptor_Device_t);
			break;

		case DTYPE_Configuration:
			Address = &ConfigurationDescriptor;
			Size    = sizeof(USB_Descriptor_Configuration_t);
			break;

		case DTYPE_String:
			switch (DescriptorNumber)
			{
				case LanguageString_id:
					Address = &LanguageString;
					Size    = pgm_read_byte(&LanguageString.Header.Size);
					break;

				case SerialNumberString_id:
					*DescriptorMemorySpace = MEMSPACE_RAM;
					Address = &SerialNumberString;
					Size = SerialNumberString.Header.Size;
					break;

				#define MAP(name, str) \
				case name: \
					Address = &name##__str_; \
					Size    = pgm_read_byte(&name##__str_.Header.Size); \
					break;
				USB_STRING_TABLE(MAP)
				#undef MAP
			}
			break;

		case DTYPE_HID:
			if (wIndex == IFACENUMBER_MISC)
			{
				Address = &ConfigurationDescriptor.HID_MiscHID;
				Size    = sizeof(USB_HID_Descriptor_HID_t);
				break;
			}
			#if defined(ENABLE_LED_DEVICE)
			if (wIndex == IFACENUMBER_LED)
			{
				Address = &ConfigurationDescriptor.HID_LEDHID;
				Size    = sizeof(USB_HID_Descriptor_HID_t);
				break;
			}
			#endif
			#if defined(ENABLE_PANEL_DEVICE)
			if (wIndex == IFACENUMBER_PANEL) 
			{
				Address = &ConfigurationDescriptor.HID_PanelHID;
				Size    = sizeof(USB_HID_Descriptor_HID_t);
				break;
			}
			#endif
			break;

		case DTYPE_Report:
			if (wIndex == IFACENUMBER_MISC)
			{
				Address = &MiscReport;
				Size    = sizeof(MiscReport);
				break;
			}
			#if defined(ENABLE_LED_DEVICE)
			if (wIndex == IFACENUMBER_LED)
			{
				Address = &LEDReport;
				Size    = sizeof(LEDReport);
				break;
			}
			#endif
			#if defined(ENABLE_PANEL_DEVICE)
			if (wIndex == IFACENUMBER_PANEL) 
			{
				Address = &PanelReport;
				Size    = sizeof(PanelReport);
				break;
			}
			#endif
		break;
	}

	*DescriptorAddress = Address;

	return Size;
}

