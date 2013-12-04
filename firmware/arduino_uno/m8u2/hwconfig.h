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

#include "../devconfig.h"


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

#define exit_bootloader(x) exit_bootloader_optiboot(x)
#include <util/delay.h>

static void inline exit_bootloader_optiboot(void)
{
	// todo
}



#endif
