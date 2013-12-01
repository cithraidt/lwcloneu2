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
	_map_( F, 7, 0 ) /* Analog Pin 0 */ \
	_map_( F, 6, 0 ) /* Analog Pin 1 */ \
	_map_( F, 5, 0 ) /* Analog Pin 2 */ \
	_map_( F, 4, 0 ) /* Analog Pin 3 */ \
	_map_( F, 1, 0 ) /* Analog Pin 4 */ \
	_map_( F, 0, 0 ) /* Analog Pin 5 */ \
	\
	_map_( D, 2, 0 ) /* Digital Pin 0 */ \
	_map_( D, 3, 0 ) /* Digital Pin 1 */ \
	_map_( D, 1, 0 ) /* Digital Pin 2 */ \
	_map_( D, 0, 0 ) /* Digital Pin 3 */ \
	_map_( D, 4, 0 ) /* Digital Pin 4 */ \
	_map_( C, 6, 0 ) /* Digital Pin 5 */ \
	_map_( D, 7, 0 ) /* Digital Pin 6 */ \
	_map_( E, 6, 0 ) /* Digital Pin 7 */ \
	_map_( B, 4, 0 ) /* Digital Pin 8 */ \
	_map_( B, 5, 0 ) /* Digital Pin 9 */ \
	_map_( B, 6, 0 ) /* Digital Pin 10 */ \
	_map_( B, 7, 0 ) /* Digital Pin 11 */ \
	_map_( D, 6, 0 ) /* Digital Pin 12 */ \
	_map_( C, 7, 0 ) /* Digital Pin 13 (L LED) */ \
	\
	_map_( B, 0, 1 ) /* RX LED */ \
	_map_( D, 5, 1 ) /* TX LED */ \
	\
	_map_( B, 1, 0 ) /* SCK */ \
	_map_( B, 2, 0 ) /* MOSI */ \
	_map_( B, 3, 0 ) /* MISO */ \
	\
	/* end */
