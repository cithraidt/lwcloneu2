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

#ifndef KEYDEFS_H__INCLUDED
#define KEYDEFS_H__INCLUDED


enum KeyboardCodes
{
	KEY_reserved = 0,
	KEY_errorRollOver,
	KEY_POSTfail,
	KEY_errorUndefined,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,          // 0x10
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_1,
	KEY_2,
	KEY_3,          // 0x20
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_Enter,
	KEY_Esc,
	KEY_BackSpace,
	KEY_Tab,
	KEY_Space,
	KEY_Minus,      // - (and _)
	KEY_Equal,      // = (and +)
	KEY_LeftBr,     // [
	KEY_RightBr,    // ]  -- 0x30
	KEY_BackSlash,  // \ (and |)
	KEY_Hash,       // Non-US # and ~
	KEY_Semicolon,  // ; (and :)
	KEY_Quotation,  // ' and "
	KEY_Tilde,      // Grave accent and tilde
	KEY_Comma,      // , (and <)
	KEY_Dot,        // . (and >)
	KEY_Slash,      // / (and ?)
	KEY_CapsLock,   // capslock
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,         // 0x40
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_PrtScr,
	KEY_ScrLck,
	KEY_Pause,
	KEY_Ins,
	KEY_Home,
	KEY_PageUp,
	KEY_Del,
	KEY_End,
	KEY_PageDown,
	KEY_RightArrow,
	KEY_LeftArrow,  // 0x50
	KEY_DownArrow,
	KEY_UpArrow,
	KEY_NumLock,
	KEY_KP_Slash,
	KEY_KP_Ast,
	KEY_KP_Minus,
	KEY_KP_Plus,
	KEY_KP_Enter,
	KEY_KP_1,
	KEY_KP_2,
	KEY_KP_3,
	KEY_KP_4,
	KEY_KP_5,
	KEY_KP_6,
	KEY_KP_7,
	KEY_KP_8,       // 0x60
	KEY_KP_9,
	KEY_KP_0,
	KEY_KP_Comma,
	KEY_Euro,       // <|> Typically near the Left-Shift key in AT-102 implementations
	KEY_Application
};

enum KeyModCodes
{
	KM_ALT_F4       = 0x70,
	KM_SHIFT_F7
};

enum ModifiersCodes
{
	MOD_LeftControl = 0x80,
	MOD_LeftShift,
	MOD_LeftAlt,
	MOD_LeftGUI,
	MOD_RightControl,
	MOD_RightShift,
	MOD_RightAlt,
	MOD_RightGUI,
};
#define ModifierBit(key) (1 << (key - MOD_LeftControl))

enum ConsumerCodes
{
	AC_VolumeUp     = 0x90,
	AC_VolumeDown,
	AC_Mute
};
#define ConsumerBit(key) (1 << (key - AC_VolumeUp))

enum MouseButtonCodes
{
    MB_Left         = 0x98,
    MB_Right,
    MB_Middle
};
#define MouseButtonBit(key) (1 << (key - MB_Left))

enum JoystickCodes
{
	NR_OF_EVENTS_PER_JOY = 12,

	J1_Left         = 0xA0,
	J1_Right,
	J1_Up,
	J1_Down,
	J1_Button1,
	J1_Button2,
	J1_Button3,
	J1_Button4,
	J1_Button5,
	J1_Button6,
	J1_Button7,
	J1_Button8,

	J2_Left,
	J2_Right,
	J2_Up,
	J2_Down,
	J2_Button1,
	J2_Button2,
	J2_Button3,
	J2_Button4,
	J2_Button5,
	J2_Button6,
	J2_Button7,
	J2_Button8,

	J3_Left,
	J3_Right,
	J3_Up,
	J3_Down,
	J3_Button1,
	J3_Button2,
	J3_Button3,
	J3_Button4,
	J3_Button5,
	J3_Button6,
	J3_Button7,
	J3_Button8,

	J4_Left,
	J4_Right,
	J4_Up,
	J4_Down,
	J4_Button1,
	J4_Button2,
	J4_Button3,
	J4_Button4,
	J4_Button5,
	J4_Button6,
	J4_Button7,
	J4_Button8,

	AG_Left,
	AG_Right,
	AG_Up,
	AG_Down,
	AG_Button1,
	AG_Button2,
	AG_Button3,
	AG_Button4,
	AG_Button5,
	AG_Button6,
	AG_Button7,
	AG_Button8
};
#define JoyButtonBit(key) (0x01 << (key - J1_Button1))



#endif  // KEYDEFS_H__INCLUDED
