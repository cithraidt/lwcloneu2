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
	\
	_map_( B, 3, 0 ) \
	_map_( B, 2, 0 ) \
	_map_( B, 1, 0 ) \
	_map_( D, 6, 0 ) \
	_map_( D, 7, 0 ) \
	_map_( B, 0, 0 ) \
	\
	/* end */



#define SHIFT_SWITCH_INDEX  3

#define MULTIFIRE_INDEX  7     // if this key is pressed once, it will result in 'MULTIFIRE_COUNT' presses
#define MULTIFIRE_COUNT  5     // usefull for coin trigger that should give more than one credit

#define PANEL_MAPPING_TABLE(_map_) \
	\
	_map_( D, 0, KEY_1,       KEY_Tab          ) \
	_map_( D, 1, J1_Button1,  KEY_Esc          ) \
	_map_( D, 2, KEY_2,       KEY_P            ) \
	_map_( D, 3, 0,           0                ) \
	_map_( D, 4, 0,           0                ) \
	_map_( D, 5, 0,           0                ) \
	_map_( B, 6, KEY_5,       KEY_5            ) \
	_map_( B, 7, KEY_5,       KEY_5            ) \
	_map_( C, 5, J1_Button2,  J1_Button2       ) \
	_map_( C, 4, J1_Right,    KEY_Equal        ) \
	_map_( C, 7, J1_Left,     KEY_Minus        ) \
	_map_( C, 2, J1_Down,     AC_VolumeDown    ) \
	_map_( C, 6, 0,           0                ) \
	_map_( B, 5, J1_Up,       AC_VolumeUp      ) \
	_map_( B, 4, 0,           0                )
	\
	/* end */
	
