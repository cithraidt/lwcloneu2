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

#ifndef HWCONFIG_H__INCLUDED
#define HWCONFIG_H__INCLUDED

#if (F_CPU != 16000000)
#error "invalid CPU clock frequency ==> should be 16 MHZ"
#endif

#if (MCU != atmega32u4)
#error "invalid CPU type ==> should be ATMega32u4"
#endif

#include "devconfig.h"
#include "pinmap.h"


/****************************************
 LED driver config
****************************************/

#if defined(ENABLE_LED_DEVICE)

#define LED_TIMER_vect TIMER0_COMPA_vect

static void inline led_timer_init(void)
{
	const int T0_CYCLE_US = 200;
	OCR0A = (((T0_CYCLE_US * (F_CPU / 1000L)) / (64 * 1000L)) - 1);
	TCCR0A = _BV(WGM01); // clear timer/counter on compare0 match
	TCCR0B = _BV(CS01) |_BV(CS00); // prescale 64
	TIMSK0 = _BV(OCIE0A); // enable Output Compare 0 overflow interrupt
	TCNT0 = 0x00;
}

#endif


/****************************************
 Panel config
****************************************/

#if defined(ENABLE_PANEL_DEVICE)
#define PANEL_TASK
#endif


/****************************************
 Clock config
****************************************/

#define CLOCK_COMPARE_MATCH_vect TIMER1_COMPA_vect
#define CLOCK_TCNT TCNT1
#define CLOCK_OCR OCR1A

static void inline clock_init(void)
{
	OCR1A = TCNT1 + (F_CPU / 1000);
	TCCR1B = _BV(CS10); //  normal mode, no prescale
	TIMSK1 = _BV(OCIE1A); // enable Output Compare 1 interrupt
}


/****************************************
 Debug UART config
****************************************/

#if defined(DEBUGLEVEL)

#define DEBUG_TX_UART_vect   USART1_UDRE_vect

static void inline debug_uart_setUDRIE(uint8_t x) { if (x) { UCSR1B |= (1 << UDRIE1); } else { UCSR1B &= ~(1 << UDRIE1); } }
static void inline debug_uart_writeUDR(uint8_t x) { UDR1 = x; }

static void inline debug_uart_init(void)
{
	UBRR1 = 16;  // 115200 bit/s @ 16MHz (==> 2.1% error)
	UCSR1A |= (1 << U2X1);
	UCSR1C = (3 << UCSZ10); // 1 stop, 8 data
	UCSR1B |= (1 << TXEN1);
}

#endif



#endif
