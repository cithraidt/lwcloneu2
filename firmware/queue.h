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

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>


#define CHUNK_MAXSIZE_LOG2  4
#define CHUNK_MAXSIZE  (1 << CHUNK_MAXSIZE_LOG2)
#define FIFO_MAXSIZE_LOG2  3

typedef struct {
	uint8_t volatile rpos;
	uint8_t volatile wpos;
	uint8_t mask;
	uint8_t buf[1];
} fifo_t;


// helper to allocate and initialize a fifo buffer 
#define CREATE_FIFO(_name_) CREATE_FIFO_(_name_, FIFO_MAXSIZE_LOG2)
#define CREATE_FIFO_(_name_, _queuesize_log_2_) \
	union { \
		uint8_t _name_##_buffer__[sizeof(fifo_t) - 1 + (1 << (_queuesize_log_2_ + CHUNK_MAXSIZE_LOG2))]; \
		fifo_t fifo; \
	} _name_##_fifo__ = { .fifo.mask = ((1 << (_queuesize_log_2_ + CHUNK_MAXSIZE_LOG2)) - 1) }; \
	fifo_t * const _name_ = &_name_##_fifo__.fifo;

	
int8_t queue_push(fifo_t *f, uint8_t x);
int8_t queue_pop(fifo_t *f, uint8_t *px);

uint8_t* chunk_prepare(fifo_t *f);
void chunk_push(fifo_t *f);
uint8_t* chunk_pop(fifo_t *f);
void chunk_release(fifo_t *f);



#endif




