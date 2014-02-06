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

#define LED_MAPPING_TABLE(_map_) \
	/* the following corresponds to the Arduino Leonardo mapping */ \
	\
	_map_( B, 0, 1 ) /* RX LED */ \
	_map_( D, 5, 1 ) /* TX LED */ \
	\
	/* end */


#define MOUSE_X_CLK_INDEX    9
#define MOUSE_X_DIR_INDEX   10
#define MOUSE_Y_CLK_INDEX   11
#define MOUSE_Y_DIR_INDEX   12
#define MOUSE_X_DELTA 2
#define MOUSE_Y_DELTA 2

#define SHIFT_SWITCH_INDEX   13

#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( D, 2,    KEY_Esc,         0                 ) /* Digital Pin 0 */ \
	_map_( D, 3,    KEY_1,           KEY_P             ) /* Digital Pin 1 */ \
	_map_( D, 1,    KEY_2,           AC_Mute           ) /* Digital Pin 2 */ \
	_map_( D, 0,    KEY_5,           KEY_5             ) /* Digital Pin 3 */ \
	_map_( D, 4,    J1_Up,           KEY_UpArrow       ) /* Digital Pin 4 */ \
	_map_( C, 6,    J1_Left,         KEY_LeftArrow     ) /* Digital Pin 5 */ \
	_map_( D, 7,    J1_Right,        KEY_RightArrow    ) /* Digital Pin 6 */ \
	_map_( E, 6,    J1_Down,         KEY_DownArrow     ) /* Digital Pin 7 */ \
	_map_( B, 4,    J1_Button1,      KEY_Enter         ) /* Digital Pin 8 */ \
	_map_( B, 5,    0,               0                 ) /* Digital Pin 9 */ \
	_map_( B, 6,    0,               0                 ) /* Digital Pin 10 */ \
	_map_( B, 3,    0,               0                 ) /* Digital Pin 14 / MISO */ \
	_map_( B, 1,    0,               0                 ) /* Digital Pin 15 / SCK */ \
	_map_( B, 2,    0,               0                 ) /* Digital Pin 16 / MOSI */ \
	\
	/* end */


	// (port, pin, mux, value_min, value_max, joyid, axis
#define ADC_MAPPING_TABLE(_map_) \
	\
	_map_( F, 7, 0x07, 0.000, 1.000, ID_Joystick1, 0 ) /* Analog Pin 0 */ \
	_map_( F, 6, 0x06, 1.000, 0.000, ID_Joystick1, 1 ) /* Analog Pin 1 */ \
	_map_( F, 5, 0x05, 0.100, 0.500, ID_Joystick2, 0 ) /* Analog Pin 2 */ \
	_map_( F, 4, 0x04, 0.000, 0.500, ID_Joystick2, 1 ) /* Analog Pin 3 */ \
	\
	/* end */

