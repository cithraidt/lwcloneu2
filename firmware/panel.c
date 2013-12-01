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

 /* Name: hid_input.c
 * Project: V-USB Mame Panel
 * Author: Andreas Oberdorfer
 * Creation Date: 2009-09-19
 * Copyright 2009 - 2011 Andreas Oberdorfer
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 */

#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include <hwconfig.h>
#include "panel.h"
#include "keydefs.h"


#if !defined(PANEL_TIMER_vect)
	void panel_init(void) {}
	void panel_ScanInput(void) {}
	uint16_t panel_gettime_ms(void) { return 0; }
	uint8_t * panel_GetNextReport(uint8_t *psize) { return NULL; }
#else


const int DEBOUNCE = 5;

// derive the number of inputs from the table, let the compiler check that no pin is used twice
enum { 
	#define MAP(X, pin, normal_id, shift_id) X##pin##_index,
	PANEL_MAPPING_TABLE(MAP)
	NUMBER_OF_INPUTS,
	#undef MAP
	#define MAP(X, pin, inv) X##pin##_index,
	LED_MAPPING_TABLE(MAP)
	#undef MAP
};

static uint8_t ReportBuffer[8];
static uint8_t InputState[NUMBER_OF_INPUTS];
static uint8_t shift_key = 0;
static uint8_t shift_key_cleanup = 0;
static uint8_t need_key_update = 0;
static uint8_t need_consumer_update = 0;
static uint16_t global_time_ms = 0;

#if (NR_OF_JOYSTICKS >= 1)
static uint8_t need_joystick_update[NR_OF_JOYSTICKS];
static uint8_t last_joystick_update = ID_Joystick1;
#endif

#if defined(MOUSE_DEVICE)
static uint8_t need_mouse_update = 0;
static uint8_t mouse_x_last_clk_state = 0;
static uint8_t mouse_x_last_dir_state = 0;
static int8_t mouse_x_count = 0;
static uint8_t mouse_y_last_clk_state = 0;
static uint8_t mouse_y_last_dir_state = 0;
static int8_t mouse_y_count = 0;
#define MOUSE_X_DELTA 1
#define MOUSE_Y_DELTA 1
#endif


// Shift switch off
PROGMEM const uint8_t NormalMapping[NUMBER_OF_INPUTS] =
{ 
	#define MAP(port, pin, normal_id, shift_id) normal_id,
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP
};

// Shift switch on
PROGMEM const uint8_t ShiftMapping[NUMBER_OF_INPUTS] =
{
	#define MAP(port, pin, normal_id, shift_id) shift_id,
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP
};


#define IsKeyDown(index) (InputState[index] & 0x80)

static uint8_t GetKeyNormalMap(unsigned char index) { return (index < NUMBER_OF_INPUTS) ? pgm_read_byte(NormalMapping + index) : 0; }
static uint8_t GetKeyShiftMap(unsigned char index) { return (index < NUMBER_OF_INPUTS) ? pgm_read_byte(ShiftMapping + index): 0; }
static uint8_t IsKeyboardCode(uint8_t key) { return (key >= KEY_A) && (key <= MOD_RightGUI); }
static uint8_t IsModifierCode(uint8_t key) { return (key >= MOD_LeftControl) && (key <= MOD_RightGUI); }
static uint8_t IsConsumerCode(uint8_t key) { return (key >= AC_VolumeUp) && (key <= AC_Mute); }
static uint8_t GetKey(uint8_t index) { return (shift_key != 0) ? GetKeyShiftMap(index) : GetKeyNormalMap(index); }


#if (NR_OF_JOYSTICKS >= 1)

static uint8_t IsJoystickCode(uint8_t key, uint8_t joy)
{
	key -= (joy * NR_OF_EVENTS_PER_JOY);

	return (key >= J1_Left) && (key < (J1_Left + NR_OF_EVENTS_PER_JOY));
}

static uint8_t NeedJoystickUpdate(void)
{
	uint8_t i;

	for (i = 0; i < NR_OF_JOYSTICKS; i++)
	{
		if (need_joystick_update[i]) {
			return 1;
		}
	}

	return 0;
}

#endif


#if defined(MOUSE_DEVICE)

static uint8_t IsMouseButtonCode(uint8_t key)
{
	return (key >= MB_Left) && (key <= MB_Middle);
}

static void MouseMoveX(uint8_t direction)
{
	if (direction)
	{
		if (mouse_x_count > -(127 - MOUSE_X_DELTA)) {
		    mouse_x_count -= MOUSE_X_DELTA;
		}
	}
	else
	{
		if (mouse_x_count < (127 - MOUSE_X_DELTA)) {
		    mouse_x_count += MOUSE_X_DELTA;
		}
	}

	need_mouse_update = 1;
}

static void MouseMoveY(uint8_t direction)
{
	if (direction)
	{
		if (mouse_y_count > -(127 - MOUSE_Y_DELTA)) {
		    mouse_y_count -= MOUSE_Y_DELTA;
		}
	}
	else
	{
		if (mouse_y_count < (127 - MOUSE_Y_DELTA)) {
		    mouse_y_count += MOUSE_Y_DELTA;
		}
	}

	need_mouse_update = 1;
}

static void CheckMouseUpdate(void)
{
	uint8_t mouse_clk_state = InputState[MOUSE_X_CLK_INDEX];
	uint8_t mouse_dir_state = InputState[MOUSE_X_DIR_INDEX];

	if (mouse_clk_state != mouse_x_last_clk_state)
	{
		if (mouse_dir_state == mouse_x_last_dir_state) {
			MouseMoveX(mouse_clk_state ^ mouse_dir_state);
		}

		mouse_x_last_clk_state = mouse_clk_state;
	}

	if (mouse_dir_state != mouse_x_last_dir_state)
	{
		if (mouse_clk_state == mouse_x_last_clk_state) {
			MouseMoveX(!(mouse_clk_state ^ mouse_dir_state));
		}

		mouse_x_last_dir_state = mouse_dir_state;
	}

	mouse_clk_state = InputState[MOUSE_Y_CLK_INDEX];
	mouse_dir_state = InputState[MOUSE_Y_DIR_INDEX];

	if (mouse_clk_state != mouse_y_last_clk_state)
	{
		if (mouse_dir_state == mouse_y_last_dir_state) {
			MouseMoveY(mouse_clk_state ^ mouse_dir_state);
		}

		mouse_y_last_clk_state = mouse_clk_state;
	}

	if (mouse_dir_state != mouse_y_last_dir_state)
	{
		if (mouse_clk_state == mouse_y_last_clk_state) {
			MouseMoveY(!(mouse_clk_state ^ mouse_dir_state));
		}

		mouse_y_last_dir_state = mouse_dir_state;
	}
}

static uint8_t NeedMouseUpdate(void) { return need_mouse_update; }

#endif

void panel_init(void)
{
	#if (NR_OF_JOYSTICKS >= 1)
	memset(&need_joystick_update[0], 0x00, sizeof(need_joystick_update));
	#endif

	#define MAP(port, pin, normal_id, shift_id) \
		PORT##port |= (1 << pin); \
		DDR##port &= ~(1 << pin);
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP

	panel_timer_init();
}

static void SetNeedUpdate(uint8_t index)
{
	uint8_t key = GetKey(index);
	if (IsConsumerCode(key))
	{
		need_consumer_update = 1;
	}
	else if (IsKeyboardCode(key))
	{
		need_key_update = 1;
	}
	#if defined(MOUSE_DEVICE)
	else if (IsMouseButtonCode(key))
	{
		need_mouse_update = 1;
	}
	#endif
	#if (NR_OF_JOYSTICKS >= 1)
	else
	{
		uint8_t i;

		for (i = 0; i < NR_OF_JOYSTICKS; i++)
		{
			if (IsJoystickCode(key, i))
			{
				need_joystick_update[i] = 1;
				break;
			}
		}
	}
	#endif
}

static void ShiftKeyCleanUp(void)
{
	if (shift_key_cleanup == 1)
	{
		uint8_t i;
		shift_key_cleanup = 2;

		for (i = 0; i < NUMBER_OF_INPUTS; i++)
		{
			if (i != SHIFT_SWITCH_INDEX)
			{
				if (InputState[i] != 0)
				{
					if (GetKeyNormalMap(i) != GetKeyShiftMap(i))
					{
						SetNeedUpdate(i);
						InputState[i] = 0;
					}
				}
			}
		}
	}

	if (shift_key_cleanup == 2)
	{
		bool no_update = !need_consumer_update && !need_key_update;

		#if defined(MOUSE_DEVICE)
		no_update = no_uptdate && !NeedMouseUpdate();
		#endif

		#if (NR_OF_JOYSTICKS >= 1)
		no_update = no_update && !NeedJoystickUpdate();
		#endif

		if (no_update)
		{
			shift_key_cleanup = 0;
			shift_key = IsKeyDown(SHIFT_SWITCH_INDEX);
		}
	}
}

static uint8_t NeedUpdate(void)
{
	ShiftKeyCleanUp();

	#if (NR_OF_JOYSTICKS >= 1)
	if (NeedJoystickUpdate())
	{
		uint8_t i;
		uint8_t x = last_joystick_update - ID_Joystick1;

		for (i = 0; i < NR_OF_JOYSTICKS; i++)
		{
			x++;

			if (x >= NR_OF_JOYSTICKS) {
				x = 0;
			}

			if (need_joystick_update[x])
			{
				last_joystick_update = ID_Joystick1 + x;
				need_joystick_update[x] = 0;

				return last_joystick_update;
			}
		}
	}
	#endif

	#if defined(MOUSE_DEVICE)
	if (need_mouse_update)
	{
		need_mouse_update = 0;
		return ID_Mouse;
	}
	#endif

	if (need_key_update)
	{
		need_key_update = 0;
		return ID_Keyboard;
	}

	if (need_consumer_update)
	{
		need_consumer_update = 0;
		return ID_Consumer;
	}

	return ID_Unknown;
}

static void SetInputCount(uint8_t index, uint8_t condition)
{
	#if defined(MOUSE_DEVICE)
	if ((index == MOUSE_X_CLK_INDEX) || (index == MOUSE_X_DIR_INDEX) ||
		(index == MOUSE_Y_CLK_INDEX) || (index == MOUSE_Y_DIR_INDEX))
	{
		InputState[index] = condition;
		return;
	}
	#endif

	if (index >= NUMBER_OF_INPUTS)
	{
		return;
	}

	#if defined(MULTIFIRE_INDEX)
	if (index == MULTIFIRE_INDEX)
	{
		static uint16_t ncycle = 0;
		static uint8_t ncount = 0;
		static uint8_t ndelay = 0;

		// simple debounce
		if (ndelay == 0 && condition)
		{
			ndelay = 100;
			ncount += MULTIFIRE_COUNT;
		}
		else if (ndelay > 1)
		{
			ndelay--;
		}
		else if (ndelay == 1)
		{
			if (!condition)
				ndelay = 0;
		}

		condition = 0;

		// state machine to generate multiple events
		if (ncount > 0)
		{
			condition = (ncycle < 100) ? 1 : 0;

			if (ncycle >= 600)
			{
				ncycle = 0;
				ncount -= 1;
			}
			else
			{
				ncycle += 1;
			}
		}
	}
	#endif

	uint8_t changed = 0;
	uint8_t count = InputState[index];
	uint8_t state = count & 0x80;
	count &= 0x7f;

	if (condition)
	{
		if (count <= DEBOUNCE)
		{
			if ((count == DEBOUNCE) && !state)
			{
				changed = 1;
				state = 0x80;
			}

			count++;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (count > 0)
		{
			if ((count == 1) && state)
			{
				changed = 1;
				state = 0;
			}

			count--;
		}
		else
		{
			return;
		}
	}

	InputState[index] = state | count;

	if (changed)
	{
		if (index == SHIFT_SWITCH_INDEX)
		{
			shift_key_cleanup = 1;
		}
		else
		{
			SetNeedUpdate(index);
		}
	}
}

void panel_ScanInput(void)
{
	if (shift_key_cleanup) {
		return;
	}

	uint8_t i = 0;

	#define MAP(port, pin, normal_id, shift_id) SetInputCount(i++, 0 == (PIN##port & (1 << pin)));
	PANEL_MAPPING_TABLE(MAP)
	#undef MAP

	#if defined(MOUSE_DEVICE)
	CheckMouseUpdate();
	#endif
}

static uint8_t ReportConsumer(void)
{
	uint8_t i;
	uint8_t consumer = 0;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsConsumerCode(key)) {
				consumer |= ConsumerBit(key);
			}
		}
	}

	ReportBuffer[0] = ID_Consumer;
	ReportBuffer[1] = consumer;

	return 2;
}

static uint8_t ReportKeyboard(void)
{
	uint8_t i;
	uint8_t r = 2;

	memset(&ReportBuffer[0], 0x00, sizeof(ReportBuffer));

	ReportBuffer[0] = ID_Keyboard;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsKeyboardCode(key))
			{
				if (IsModifierCode(key))
				{
					ReportBuffer[1] |= ModifierBit(key);
				}
				else
				{
					if (r < sizeof(ReportBuffer))
					{
						switch (key)
						{
						case KM_ALT_F4:
							ReportBuffer[1] |= ModifierBit(MOD_LeftAlt);
							ReportBuffer[r] = KEY_F4;
							break;
						case KM_SHIFT_F7:
							ReportBuffer[1] |= ModifierBit(MOD_LeftShift);
							ReportBuffer[r] = KEY_F7;
							break;
						default:
							ReportBuffer[r] = key;
							break;
						}

						r++;
					}
				}
			}
		}
	}

	return sizeof(ReportBuffer);
}

#if (NR_OF_JOYSTICKS >= 1)

static uint8_t ReportJoystick(uint8_t id)
{
	uint8_t i;
	uint8_t joy_x = 0x00;
	uint8_t joy_y = 0x00;
	uint8_t joy_b = 0;

	ReportBuffer[0] = id;
	id = (id - ID_Joystick1) * NR_OF_EVENTS_PER_JOY;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i) - id;
			switch (key)
			{
			case J1_Left:
				joy_x = 0x0F;
				break;
			case J1_Right:
				joy_x = 0x01;
				break;

			case J1_Up:
				joy_y = 0xF0;
				break;
			case J1_Down:
				joy_y = 0x10;
				break;

			case J1_Button1:
			case J1_Button2:
			case J1_Button3:
			case J1_Button4:
			case J1_Button5:
			case J1_Button6:
			case J1_Button7:
			case J1_Button8:
				joy_b |= JoyButtonBit(key);
				break;

			default:
				break;
			}
		}
	}

	ReportBuffer[1] = joy_x | joy_y;
	ReportBuffer[2] = joy_b;

	return 3;
}

#endif


#if defined(MOUSE_DEVICE)

static uint8_t ReportMouse(void)
{
	uint8_t i;
	uint8_t buttons = 0;

	for (i = 0; i < NUMBER_OF_INPUTS; i++)
	{
		if (IsKeyDown(i))
		{
			uint8_t key = GetKey(i);

			if (IsMouseButtonCode(key)) {
				buttons |= MouseButtonBit(key);
			}
		}
	}

	ReportBuffer[0] = ID_Mouse;
	ReportBuffer[1] = buttons;
	ReportBuffer[2] = mouse_x_count;
	ReportBuffer[3] = mouse_y_count;
	mouse_x_count = 0;
	mouse_y_count = 0;

	return 4;
}

#endif


static uint8_t BuildReport(uint8_t id)
{
	switch (id)
	{
	case ID_Keyboard:
		return ReportKeyboard();

	case ID_Consumer:
		return ReportConsumer();

	#if (NR_OF_JOYSTICKS >= 1)
	case ID_Joystick4:
	case ID_Joystick3:
	case ID_Joystick2:
	case ID_Joystick1:
		return ReportJoystick(id);
	#endif

	#if defined(MOUSE_DEVICE)
	case ID_Mouse:
		return ReportMouse();
	#endif

	default:
		break;
	}

	return 0;
}


uint8_t * panel_GetNextReport(uint8_t *psize)
{
	uint8_t id = NeedUpdate();

	if (id == ID_Unknown) {
		return NULL;
	}

	uint8_t ndata = BuildReport(id);

	if (ndata <= 0) {
		return NULL;
	}

	if (psize != NULL)
	    psize[0] = ndata;
	
	return ReportBuffer;
}


ISR(PANEL_TIMER_vect)
{
	global_time_ms += 1;
}


uint16_t panel_gettime_ms(void)
{
	uint16_t t0;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		t0 = global_time_ms;
	}

	return t0;
}

#endif
