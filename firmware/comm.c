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

#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "comm.h"

extern FILE g_stdout_uart;


void comm_init(void)
{
	#if defined(exit_bootloader)
	exit_bootloader();
	#endif

	#if defined(DATA_TX_UART_vect) || defined(DATA_RX_UART_vect)
	data_uart_init();
	#endif

	#if defined(DEBUG_TX_UART_vect)
	debug_uart_init();
	stdout = &g_stdout_uart;
	#endif
}


#if defined(DEBUG_TX_UART_vect)

CREATE_FIFO(g_dbgfifo)

static int putchar_uart_txt(char c, FILE *stream);
static int putchar_uart_raw(char c, FILE *stream);

FILE g_stdout_uart = FDEV_SETUP_STREAM(putchar_uart_txt, NULL, _FDEV_SETUP_WRITE);

static int putchar_uart_raw(char c, FILE *stream)
{
	for (;;)
	{
		if (0 == queue_push(g_dbgfifo, c)) {
			break;
		}
	}

	debug_uart_setUDRIE(1);
	return 0;
}

static int putchar_uart_txt(char c, FILE *stream)
{
	if (c == '\n') {
		putchar_uart_raw('\r', stream);
	}

	putchar_uart_raw(c, stream);

	return 0;
}


ISR(DEBUG_TX_UART_vect)
{
	uint8_t x;

	int8_t res = queue_pop(g_dbgfifo, &x);

	if (res != 0) {
		debug_uart_setUDRIE(0);
		return;
	}

	debug_uart_writeUDR(x);
}

#endif


#if defined(DATA_TX_UART_vect)

CREATE_FIFO(g_txfifo)

msg_t* msg_prepare(void)
{
	uint8_t * const pdata = chunk_prepare(g_txfifo);
	if (pdata == NULL) {
		return NULL;
	}

	return (msg_t*)pdata;
}

void msg_send(void)
{
	chunk_push(g_txfifo);
	uart_setUDRIE(1);
}

ISR(DATA_TX_UART_vect)
{
	static uint8_t nbytes = 0;
	static uint8_t * pdata = NULL;

	// start new data frame?

	if (nbytes == 0)
	{
		pdata = chunk_pop(g_txfifo);

		if (pdata == NULL)
		{
			//clear UDRE interrupt
			uart_setUDRIE(0);
			return; // end of transmission
		}

		uint8_t const nlen = pdata[0];

		if (nlen >= CHUNK_MAXSIZE)
		{
			chunk_release(g_txfifo);
			DbgOut(DBGERROR, "ISR(TX), invalid argument nlen");
			return;
		}

		uart_setBIT8TX(1);  // set 9nth bit

		nbytes = nlen + 1;
	}

	// transmit byte

	uart_writeUDR(*pdata++);
	nbytes--;

	// advance fifo

	if (nbytes == 0)
		chunk_release(g_txfifo);
}

#endif


#if defined(DATA_RX_UART_vect)

CREATE_FIFO(g_rxfifo)

msg_t* msg_recv(void)
{
	uint8_t * const pdata = chunk_pop(g_rxfifo);
	if (pdata == NULL) {
		return NULL;
	}

	return (msg_t*)pdata;
}

void msg_release(void)
{
	chunk_release(g_rxfifo);
}

ISR(DATA_RX_UART_vect)
{
	static uint8_t nbytes = 0;
	static uint8_t * pdata = NULL;

	uint8_t e = uart_getError();
	uint8_t s = uart_getBIT8RX();
	uint8_t b = uart_readUDR();

	// check error condition

	if (e)
	{
		DbgOut(DBGERROR, "ISR(rx), usart error");
		nbytes = 0;
		return;
	}

	// sync to start of frame, i.e. bit 8 is set

	if (nbytes == 0)
	{
		if (!s) {
			DbgOut(DBGERROR, "ISR(rx), frame sync");
			return;
		}

		if (b >= CHUNK_MAXSIZE) {
			DbgOut(DBGERROR, "ISR(rx), message size to big");
			return;
		}

		pdata = chunk_prepare(g_rxfifo);

		if (pdata == NULL)
		{
			DbgOut(DBGERROR, "ISR(rx), buffer full");
			return;
		}

		nbytes = b + 1;
	}

	// store byte

	*pdata++ = b;
	nbytes--;

	// commit the message

	if (nbytes == 0)
		chunk_push(g_rxfifo);
}

#endif
