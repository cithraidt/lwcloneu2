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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include <hwconfig.h>
#include "clock.h"


static volatile uint16_t g_tsc_lo = 0;
static volatile uint16_t g_tsc_hi = 0;
static volatile uint16_t g_time_ms = 0;


ISR(CLOCK_COMPARE_MATCH_vect)
{
	#if defined(ENABLE_PROFILING)
	profile_start();
	#endif

	uint16_t const t = CLOCK_TCNT;

	if (g_tsc_lo > t) {
	    g_tsc_hi += 1;
	}

	g_tsc_lo = t;
	g_time_ms += 1;

	CLOCK_OCR += (F_CPU / 1000);
}

uint32_t clock(void)
{
	uint16_t t_lo;
	uint16_t t_hi;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		t_lo = g_tsc_lo;
		t_hi = g_tsc_hi;
	}

	uint16_t const t = CLOCK_TCNT;

	if (t_lo > t) {
	    t_hi += 1;
	}

	return ((uint32_t)t_hi << 16) | t;
}

uint16_t clock_ms(void)
{
	uint16_t t0;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		t0 = g_time_ms;
	}

	return t0;
}

