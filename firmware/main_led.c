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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <hwconfig.h>
#include "comm.h"
#include "led.h"
#include "panel.h"


int main(void)
{
	clock_init();
	comm_init();
	led_init();
	panel_init();

	set_sleep_mode(SLEEP_MODE_IDLE);

	sei();

	DbgOut(DBGINFO, "main_led, enter main loop");

	for (;;)
	{
		// process LED messages

		#if defined(LED_TIMER_vect)

		msg_t * const prxmsg = msg_recv();

		if (prxmsg != NULL)
		{
			DbgOut(DBGINFO, "main_led, message received");

			// is the message valid?

			if (prxmsg->nlen != 8)
			{
				DbgOut(DBGERROR, "main_led, invalid framesize");
			}
			else
			{
				// process the data
				led_update(&prxmsg->data[0]);
			}

			msg_release();

			continue;
		}

		#endif

		// process panel changes

		#if defined(PANEL_TASK)

		uint8_t * pdata = NULL;
		uint8_t const ndata = panel_get_report(&pdata);

		if (ndata > 0)
		{
			msg_t * const ptxmsg = msg_prepare();

			if (ptxmsg != NULL)
			{
				memcpy(&ptxmsg->data[0], pdata, ndata);
				ptxmsg->nlen = ndata;
				msg_send();
			}
			else
			{
				DbgOut(DBGERROR, "main_led, tx buffer overflow");
			}

			continue;
		}

		#endif

		// if we are here, there was no new message and no new panel report
		// ==> enter idle mode

		#if defined(ENABLE_PROFILER)
		profile_stop();
		#endif

		sleep_mode();
	}

	return 0;
}
