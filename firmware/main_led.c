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
				msg_release();
			}
			else
			{
				// process the data

				led_update(&prxmsg->data[0]);
				msg_release();
			}
		}

		#endif

		// process panel changes

		#if defined(PANEL_TIMER_vect)

		static uint16_t time_next_ms = 0;
		uint16_t time_curr_ms = panel_gettime_ms();
		const uint16_t DELTA_TIME_PANEL_REPORT_MS = 5;

		if (((int16_t)time_curr_ms - (int16_t)time_next_ms) >= 0)
		{
			time_next_ms = time_curr_ms + DELTA_TIME_PANEL_REPORT_MS;

			panel_ScanInput();

			msg_t * const ptxmsg = msg_prepare();

			if (ptxmsg != NULL)
			{
				uint8_t nlen = 0;
				uint8_t * pdata = panel_GetNextReport(&nlen);
			
				if (pdata != NULL && nlen <= sizeof(ptxmsg->data))
				{
					memcpy(&ptxmsg->data[0], pdata, nlen);
					ptxmsg->nlen = nlen;
					msg_send();
				}
			}
		}

		#endif

		sleep_mode();
	}

	return 0;
}
