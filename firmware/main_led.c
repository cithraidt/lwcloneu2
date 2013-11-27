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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "led.h"
#include "comm.h"


int main(void)
{
	comm_init();
	led_init();

	set_sleep_mode(SLEEP_MODE_IDLE);

	sei();

	DbgOut(DBGINFO, "main_led, enter main loop");

	for (;;)
	{
		// wait for data

		msg_t * pmsg;

		for (;;) 
		{
			pmsg = msg_recv();

			if (pmsg != NULL)
				break;

			sleep_mode();
		}

		DbgOut(DBGINFO, "main_led, message received");

		// is the message valid?

		if (pmsg->nlen != 8)
		{
			DbgOut(DBGERROR, "main_led, invalid framesize");
			msg_release();
			continue;
		}

		// process the data

		led_update(&pmsg->data[0]);

		msg_release();
	}

	return 0;
}
