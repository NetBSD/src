/* $NetBSD: x86emu_i8254.h,v 1.1 2007/12/21 17:45:50 joerg Exp $ */

/*-
 * Copyright (c) 2007 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _X86EMU_X86EMU_I8254_H
#define _X86EMU_X86EMU_I8254_H

#include <sys/time.h>

#ifndef _KERNEL
#  include <stdbool.h>
#else
#  include <sys/types.h>
#endif

struct x86emu_i8254_timer {
	/* Tick when the counter was started, 0 if it is inactive */
	uint64_t start_tick;

	/* GATE is raised */
	bool gate_high;

	/* The initial value of the counter */
	uint16_t active_counter;

	/* The current mode as bit pattern */
	uint8_t active_mode;

	/* The current setting of the BCD flag */
	bool active_is_bcd;

	/* Latched counter */
	uint16_t latched_counter;
	bool counter_is_latched;

	/* Latched status */
	uint8_t latched_status;
	bool status_is_latched;

	/* Read counter */
	bool read_lsb, read_msb;

	/* Write counter */
	uint8_t rw_status;
	uint16_t new_counter;
	uint8_t new_mode;
	bool new_is_bcd;
	bool null_count;

	bool write_lsb, write_msb;
};

struct x86emu_i8254 {
	struct x86emu_i8254_timer timer[3];
	struct timespec base_time;

	void (*gettime)(struct timespec *);
};

__BEGIN_DECLS

void	x86emu_i8254_init(struct x86emu_i8254 *, void (*)(struct timespec *));
uint8_t	x86emu_i8254_inb(struct x86emu_i8254 *, uint16_t);
void	x86emu_i8254_outb(struct x86emu_i8254 *, uint16_t, uint8_t);
bool	x86emu_i8254_claim_port(struct x86emu_i8254 *, uint16_t);

__END_DECLS

#endif
