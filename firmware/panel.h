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

/* Name: hid_input.h
 * Project: V-USB Mame Panel
 * Author: Andreas Oberdorfer
 * Creation Date: 2009-09-19
 * Copyright 2009 - 2011 Andreas Oberdorfer
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 */

#ifndef PANEL_H__INCLUDED
#define PANEL_H__INCLUDED

#include <stdint.h>


enum ReportIds
{
	ID_Unknown = 0,
	ID_Keyboard,
	ID_Consumer,
	ID_Joystick1,
	ID_Joystick2,
	ID_Joystick3,
	ID_Joystick4,
	ID_Mouse
};

static const uint16_t DELTA_TIME_PANEL_REPORT_MS = 5;

void panel_init(void);
uint8_t panel_get_report(uint8_t **ppdata);



#endif  // PANEL_H__INCLUDED
