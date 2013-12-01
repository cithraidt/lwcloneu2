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
#include <avr/interrupt.h>
#include <avr/io.h>

#include <hwconfig.h>
#include "led.h"


#if !defined(LED_TIMER_vect)
	void led_init(void) {}
	void led_update(uint8_t *p8bytes) {}
#else


#define MAP(X, pin, inv) X##pin##_index,
enum { LED_MAPPING_TABLE(MAP) NUMBER_OF_LEDS };
#undef MAP

#define NUMBER_OF_BANKS   ((NUMBER_OF_LEDS + 7) / 8)
#define MAX_PWM 49


#if (NUMBER_OF_LEDS > 32)
	#error "number of led pins is bigger than 32!"
#endif


struct {
	volatile uint8_t enable;
	volatile uint8_t mode;
} g_LED[NUMBER_OF_BANKS * 8];

volatile uint16_t g_dt = 256;  // access is not atomic, but the read in the pwm loop is not critical


static void update_state(uint8_t * p5bytes);
static void update_profile(int8_t k, uint8_t * p8bytes);
static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t);
static void led_ports_init(void);



void led_init(void)
{
	/* LED driver */
	led_ports_init();

	// Timer for soft-PWM
	led_timer_init();
}


void led_update(uint8_t *p8bytes)
{
	static uint8_t nbank = 0;

	if (p8bytes[0] == 64)
	{
		update_state(p8bytes + 1);
		nbank = 0;
	}
	else
	{
		update_profile(nbank, p8bytes);
		nbank = (nbank + 1) & 0x03;
	}
}


static void update_state(uint8_t * p5bytes)
{
	for (int8_t k = 0; k < NUMBER_OF_BANKS; k++)
	{
		uint8_t b = p5bytes[k];

		for (int8_t i = 0; i < 8; i++)
		{
			g_LED[k * 8 + i].enable = b & 0x01;
			b >>= 1;
		}
	}

	uint8_t pulse_speed = p5bytes[4];

	if (pulse_speed > 7)
	    pulse_speed = 7;

	if (pulse_speed == 0)
	    pulse_speed = 1;

	g_dt = pulse_speed * 128;
}


static void update_profile(int8_t k, uint8_t * p8bytes)
{
	if (k >= NUMBER_OF_BANKS)
		return;

	for (int8_t i = 0; i < 8; i++)
	{
		g_LED[k * 8 + i].mode = p8bytes[i];
	}
}


static void update_pwm(uint8_t *pwm, int8_t n, uint16_t t)
{
	for (int8_t i = 0; i < n; i++) 
	{
		if (g_LED[i].enable == 0)
		{
			pwm[i] = 0;
		}
		else
		{
			uint8_t b = g_LED[i].mode;

			if ((b >= 0) && (b <= MAX_PWM))
			{
				// constant brightness

				pwm[i] = b;
			}
			else if (b == 129)
			{
				// triangle

				uint16_t x = t >> 8;
				if (x & 0x80) // 128..255
					x = 255 - x;

				pwm[i] = (MAX_PWM * x) >> 7;

			}
			else if (b == 130)
			{
				// rect
				
				pwm[i] = (t & 0x8000) ? MAX_PWM : 0;
			}
			else if (b == 131)
			{
				// fall

				uint16_t x = 255 - (t >> 8);

				pwm[i] = (MAX_PWM * x) >> 8;
			}
			else if (b == 132)
			{
				// rise

				uint16_t x = t >> 8;

				pwm[i] = (MAX_PWM * x) >> 8;
			}
			else
			{
				// unexpected!

				pwm[i] = 0;
			}
		}
	}
}


ISR(LED_TIMER_vect)
{
	static int8_t counter = 0;
	static uint16_t t = 0;
	static uint8_t pwm[NUMBER_OF_LEDS];

	counter--;

	if (counter < 0)
	{
		// reset counter
		counter = MAX_PWM - 1; // pwm value of MAX_PWM should be allways 'on', 0 should be allways 'off'

		// increment time counter
		t += g_dt;

		// update pwm values
		update_pwm(pwm, sizeof(pwm) / sizeof(pwm[0]), t);
	}

	// set or clear all defined pins

	#define MAP(X, pin, inv) if ((pwm[X##pin##_index] > counter) == (!inv)) { PORT##X |= (1 << pin); } else { PORT##X &= ~(1 << pin); }
	LED_MAPPING_TABLE(MAP)
	#undef MAP
}


static void led_ports_init(void)
{
	#define MAP(X, pin, inv) \
		PORT##X &= ~(1 << pin); \
		DDR##X  |= (1 << pin);
	LED_MAPPING_TABLE(MAP)
	#undef MAP
}

#endif
