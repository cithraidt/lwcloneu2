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
#include <avr/sleep.h>
#include <util/atomic.h>

#include "clock.h"
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

	#if defined(DEBUG_TX_UART_vect) || defined(DEBUG_TX_SOFT_UART_vect)
	debug_uart_init();
	stdout = &g_stdout_uart;
	#endif
}


#if defined(DEBUG_TX_UART_vect) || defined(DEBUG_TX_SOFT_UART_vect)

CREATE_FIFO(g_dbgfifo, 7, 0)

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

#if defined(DEBUG_TX_UART_vect)

ISR(DEBUG_TX_UART_vect)
{
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	uint8_t x;

	int8_t res = queue_pop(g_dbgfifo, &x);

	if (res != 0)
	{
		debug_uart_setUDRIE(0);
		return;
	}

	debug_uart_writeUDR(x);
}

#elif defined(DEBUG_TX_SOFT_UART_vect)

#define BAUDRATE 9600
#define DURATION_TXBIT ((F_CPU/8 + (BAUDRATE/2)) / BAUDRATE)

ISR(DEBUG_TX_SOFT_UART_vect)
{
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	OCR0A += DURATION_TXBIT;

	static uint8_t count = 0;
	static uint8_t x = 0;

	if (count == 0)
	{
		int8_t res = queue_pop(g_dbgfifo, &x);

		if (res != 0)
		{
			debug_uart_setUDRIE(0);
			return;
		}

		x = ~x;        // invert data for Stop bit generation
		count = 9;     // 10 bits: Start + data + Stop

		TCCR0A = 1 << COM0A1; // clear on next compare

		return;
	}

	if (count)
	{
		count--;
		TCCR0A = 1 << COM0A1; // clear on next compare

		// no start bit
		if (!(x & 0x01)) // test inverted data
			TCCR0A = (1 << COM0A1) | (1 << COM0A0); // set on next compare

		x >>= 1; // shift zero in from left
	}
}

#endif
#endif

#if defined(ENABLE_PROFILING)

static volatile uint8_t s_profiling = 0;
static volatile uint32_t s_t_start = 0;

void profile_stop(void)
{
	if (!s_profiling) {
		return;
	}

	uint32_t t_now = clock();
	uint32_t duration;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		duration = t_now - s_t_start;
		s_profiling = 0;
	}

	static uint32_t duration_total = 0;
	static uint32_t t_start_total = 0;

	if (t_start_total == 0)
	{
		t_start_total = t_now;
		duration_total = 0;
		return;
	}

	duration_total += duration;

	if ((t_now - t_start_total) > (((uint32_t)1 << 18) * 100)) {
		MsgOut("\rCPU usage: %2d%%", (uint16_t)(duration_total >> 18));
		t_start_total = t_now;
		duration_total = 0;
	}
}

void profile_start(void)
{
	if (!s_profiling) {
		s_profiling = 1;
		s_t_start = clock();
	}
}

#endif


void sleep_ms(uint16_t ms)
{
	if (ms > 0) {
		ms = clock_ms() + ms;
	}

	for (;;)
	{
		#if defined(ENABLE_PROFILING)
		profile_stop();
		#endif

		sleep_mode();

		if (ms == 0) {
			break;
		}

		uint16_t const t_now = clock_ms();

		if (((int16_t)t_now - (int16_t)ms) >= 0) {
			break;
		}
	}
}


#if defined(DATA_TX_UART_vect)

CREATE_FIFO(g_txfifo, 0, 6)

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
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	static uint8_t nbytes = 0;
	static uint8_t * pdata = NULL;

	// start new data frame?

	if (nbytes == 0)
	{
		pdata = chunk_peek(g_txfifo);

		if (pdata == NULL)
		{
			//clear UDRE interrupt
			uart_setUDRIE(0);
			return; // end of transmission
		}

		uint8_t const nlen = pdata[0];

		if (nlen >= g_txfifo->chunksize)
		{
			DbgOut(DBGERROR, "ISR(TX), invalid argument nlen");
			chunk_release(g_txfifo);
			return;
		}

		uart_setBIT8TX(1);  // set 9nth bit

		nbytes = nlen + 1;
	}
	else
	{
		uart_setBIT8TX(0);  // clear 9nth bit
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

CREATE_FIFO(g_rxfifo, 0, 6)

msg_t* msg_recv(void)
{
	uint8_t * const pdata = chunk_peek(g_rxfifo);

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
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	static uint8_t nbytes = 0;
	static uint8_t * pdata = NULL;

	uint8_t e = uart_getError();
	uint8_t s = uart_getBIT8RX();
	uint8_t b = uart_readUDR();

	// check error condition

	if (e)
	{
		#if 0
		if (e & (1 << FE0))
			DbgOut(DBGERROR, "ISR(rx), FE0");

		if (e & (1 << DOR0))
			DbgOut(DBGERROR, "ISR(rx), DOR0");

		if (e & (1 << UPE0))
			DbgOut(DBGERROR, "ISR(rx), UPE0");
		#endif

		nbytes = 0;
		return;
	}

	// sync to start of frame, i.e. bit 8 is set

	if (s) 
	{
		if (nbytes > 0)
		{
			DbgOut(DBGERROR, "ISR(rx), nbytes > 0");
		}

		nbytes = 0;
	}

	if (nbytes == 0)
	{
		if (!s) {
			DbgOut(DBGERROR, "ISR(rx), !s");
			return;
		}

		if (b >= g_rxfifo->chunksize) {
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
