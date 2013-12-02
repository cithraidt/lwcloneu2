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

#if (MCU != atmega8u2) || (MCU != atmega16u2) || (MCU != atmega32u2) 
#error "invalid CPU type ==> should be atmega8u2/16u2/32u2"
#endif

#include "../usbconfig.h"


/****************************************
 Data UART config
****************************************/

#define DATA_TX_UART_vect   USART1_UDRE_vect
#define DATA_RX_UART_vect   USART1_RX_vect

#include "../../data_uart1.h"

static void inline data_uart_init(void)
{
	UBRR1 = 3; // 250 kBit/s @ 16 MHz CPU
	UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10) | (1 << UPM11) | (1 << UPM10); // asynchron uart, odd parity, 9-bit-character, 1 stop bit
	UCSR1B |= (1 << UCSZ12) | (1 << TXEN1) | (1 << RXEN1) | (1 << RXCIE1); // enable Receiver and Transmitter
}


/****************************************
 Bootloader shutdown config
****************************************/

#define exit_bootloader(x) exit_bootloader_stk500v2(x)
#include <util/delay.h>

static void inline exit_bootloader_stk500v2(void)
{
	UBRR1 = 16;  // 115200 bit/s @ 16MHz (==> 2.1% error)
	UCSR1A |= (1 << U2X1);
	UCSR1B |= (1 << TXEN1);

	_delay_ms(70); // wait until the uC is active (worst case: 65ms + 14CK with LFUSE=0xFF) and waits for data

	#define SEND(_b_) do { \
		loop_until_bit_is_set(UCSR1A, UDRE1); \
		UDR1 = _b_; } while(0)

	SEND(0x1B); // message start
	SEND(0); // sequence number
	SEND(0); // message size high
	SEND(3); // message size low
	SEND(0x0E); // token
	SEND(0x11); // command id "leave progmode"
	SEND(0); // pre delay
	SEND(0); // post delay
	SEND(0x1B ^ 3 ^ 0x0e ^ 0x11); // checksum

	_delay_ms(60); // wait until the bootloader has replied and exited (50 ms minimum for shutting down the LEDs --> see stk500boot.c)
	uint8_t x = UDR1; // clear data reg
	UCSR1A &= ~(1 << U2X1); // restore previous state of the uart
	UCSR1B &= ~(1 << TXEN1);

	#undef SEND
}



#endif
