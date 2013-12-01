/*
 * LWCloneU2
 * Copyright (C) 2013 Andreas Dittrich <lwcloneu2@cithraidt.de>
 *
 * This program is free software; you can redistribute it and/or modifyit under the terms of the
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

#include <stdint.h>
#include <avr/io.h>

#if (F_CPU != 16000000)
#error "invalid CPU clock frequency ==> should be 16 MHZ"
#endif

#if (MCU != atmega2560)
#error "invalid CPU type ==> should be ATMega2560"
#endif


/****************************************
 LED driver config
****************************************/

#include "pinmap.h"

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


/****************************************
 Panel config
****************************************/

#define PANEL_TIMER_vect TIMER1_COMPA_vect

static void inline panel_timer_init(void)
{
	const int T1_CYCLE_US = 1000;
	OCR1A = (((T1_CYCLE_US * (F_CPU / 1000L)) / (64 * 1000L)) - 1);
	TCCR1B = _BV(WGM12) | _BV(CS11) |_BV(CS10); //  clear timer/counter on compare1 match, prescale 64
	TIMSK1 = _BV(OCIE1A); // enable Output Compare 1 overflow interrupt
	TCNT1 = 0x00;
}


/****************************************
 Data UART config
****************************************/

#define DATA_TX_UART_vect   USART0_UDRE_vect
#define DATA_RX_UART_vect   USART0_RX_vect

#include "../../data_uart0.h"

static void inline data_uart_init(void)
{
	UBRR0 = 3; // 250 kBit/s @ 16 MHz CPU
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01) | (1 << UPM00); // asynchron uart, odd parity, 9-bit-character, 1 stop bit
	UCSR0B |= (1 << UCSZ02) | (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0); // enable Receiver and Transmitter
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
