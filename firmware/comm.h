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

#ifndef UART_H__INCLUDED
#define UART_H__INCLUDED

#include <avr/pgmspace.h>
#include <hwconfig.h>
#include "queue.h"


typedef struct {
	uint8_t nlen;
	uint8_t data[1];
} msg_t;


void comm_init(void);

#if defined(DATA_TX_UART_vect)
msg_t* msg_prepare(void);
void msg_send(void);
#endif

#if defined(DATA_RX_UART_vect)
msg_t* msg_recv(void);
void msg_release(void);
#endif


typedef enum {
	DBGERROR = 0,
	DBGLOG,
	DBGINFO,
	DBGTRACE,
} debuglevel;

#if defined(DEBUG_TX_UART_vect) || defined(DEBUG_TX_SOFT_UART_vect)

#define DbgOut(_level_, _msg_, ...) do { \
	if ((_level_) > DEBUGLEVEL) break; \
	printf_P( \
		((_level_) == DBGERROR)  ? PSTR("[Error] ") : \
		((_level_) == DBGLOG)    ? PSTR("[Log] ") : \
		((_level_) == DBGINFO)   ? PSTR("[Info] ") : \
		((_level_) == DBGTRACE)  ? PSTR("[Trace] ") : PSTR("[] ")); \
	printf_P(PSTR(_msg_), ##__VA_ARGS__); \
	printf_P(PSTR("\n")); \
} while (0)

#define MsgOut(_msg_, ...) printf_P(PSTR(_msg_), ##__VA_ARGS__)

#else

#define DbgOut(_level_, _msg_, ...)
#define MsgOut(_msg_, ...)

#endif


#if defined(ENABLE_PROFILING)
void profile_start(void);
void profile_stop(void);
#endif

void sleep_ms(uint16_t ms);



#endif