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

#ifndef DATA_UART0_H__INCLUDED
#define DATA_UART0_H__INCLUDED

#include <stdint.h>
#include <avr/io.h>

static void inline uart_setUDRIE(uint8_t x) { if (x) { UCSR0B |= (1 << UDRIE0); } else { UCSR0B &= ~(1 << UDRIE0); } }
static void inline uart_writeUDR(uint8_t x) { UDR0 = x; }
static uint8_t inline uart_readUDR(void) { return UDR0; }
static void inline uart_setBIT8TX(uint8_t x) { if (x) { UCSR0B |= (1 << TXB80); } else { UCSR0B &= ~(1 << TXB80); } }
static uint8_t inline uart_getBIT8RX(void) { return UCSR0B & (1 << RXB80); }
static uint8_t inline uart_getSRA(void) { return UCSR0A; }
static uint8_t inline uart_getError(void) { return UCSR0A & ((1 << FE0) | (1 << DOR0) | (1 << UPE0)); }


#endif
