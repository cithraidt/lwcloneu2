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
#include "queue.h"


static uint8_t fifo_getlevel(fifo_t const *f) { return f->wpos - f->rpos; }
static uint8_t fifo_getfree(fifo_t const *f) { return (f->mask + 1) - fifo_getlevel(f); }


int8_t queue_push(fifo_t *f, uint8_t x)
{
	uint8_t const nfree = fifo_getfree(f);

	if (nfree == 0)
		return -1;

	uint8_t const index = f->wpos & f->mask;
	f->buf[index] = x;
	f->wpos += 1;

	return 0;
}


int8_t queue_pop(fifo_t *f, uint8_t *px)
{
	uint8_t const ndata = fifo_getlevel(f);

	if (ndata == 0 || px == NULL)
		return -1;

	uint8_t const index = f->rpos & f->mask;
	*px = f->buf[index];
	f->rpos += 1;

	return 0;
}


uint8_t* chunk_pop(fifo_t *f)
{
	uint8_t const ndata = fifo_getlevel(f);

	if (ndata == 0)
		return NULL;

	uint8_t index = f->rpos & f->mask;

	return &f->buf[index];
}


void chunk_release(fifo_t *f)
{
	f->rpos += (1 << CHUNK_MAXSIZE_LOG2);
}


uint8_t* chunk_prepare(fifo_t *f)
{
	uint8_t const nfree = fifo_getfree(f);

	if (nfree == 0)
		return NULL;

	uint8_t index = f->wpos & f->mask;
	
	return &f->buf[index];
}


void chunk_push(fifo_t* f)
{
	f->wpos += (1 << CHUNK_MAXSIZE_LOG2);
}
