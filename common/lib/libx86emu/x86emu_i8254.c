/* $NetBSD: x86emu_i8254.c,v 1.1.34.1 2014/08/19 23:45:15 tls Exp $ */

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

#include <x86emu/x86emu_i8254.h>

#ifndef _KERNEL
#include <assert.h>
#define	KASSERT(x) assert(x)
#endif

#define	I8254_FREQ	1193182 /* Hz */

static uint16_t
bcd2bin(uint16_t bcd_val)
{
	return bcd_val % 0x10 + (bcd_val / 0x10 % 0x10 * 10) +
	    (bcd_val / 0x100 % 0x10 * 100) + (bcd_val / 0x1000 % 0x10 * 1000);
}

static uint16_t
bin2bcd(uint16_t bin_val)
{
	return (bin_val % 10) + (bin_val / 10 % 10 * 0x10) +
	    (bin_val / 100 % 10 * 0x100) + (bin_val / 1000 % 10 * 0x1000);
}

/*
 * Compute tick of the virtual timer based on start time and
 * current time.
 */
static uint64_t
x86emu_i8254_gettick(struct x86emu_i8254 *sc)
{
	struct timespec curtime;
	uint64_t tick;

	(*sc->gettime)(&curtime);

	tick = (curtime.tv_sec - sc->base_time.tv_sec) * I8254_FREQ;
	tick += (uint64_t)(curtime.tv_nsec - sc->base_time.tv_nsec) * I8254_FREQ / 1000000000;

	return tick;
}

/* Compute current counter value. */
static uint16_t
x86emu_i8254_counter(struct x86emu_i8254_timer *timer, uint64_t curtick)
{
	uint16_t maxtick;

	/* Initial value if timer is disabled or not yet started */
	if (timer->gate_high || timer->start_tick > curtick)
		return timer->active_counter;

	/* Compute maximum value based on BCD/binary mode */
	if (timer->active_is_bcd)
		maxtick = 9999;
	else
		maxtick = 0xffff;

	curtick -= timer->start_tick;

	/* Check if first run over the time counter is over. */
	if (curtick <= timer->active_counter)
		return timer->active_counter - curtick;
	/* Now curtick > 0 as both values above are unsigned. */

	/* Special case of active_counter == maxtick + 1 */
	if (timer->active_counter == 0 && curtick - 1 <= maxtick)
		return maxtick + 1 - curtick;

	/* For periodic timers, compute current periode. */
	if (timer->active_mode & 2)
		return timer->active_counter - curtick % timer->active_counter;

	/* For one-shot timers, compute overflow. */
	curtick -= maxtick + 1;
	return maxtick - curtick % maxtick + 1;
}

static bool
x86emu_i8254_out(struct x86emu_i8254_timer *timer, uint64_t curtick)
{
	/*
	 * TODO:
	 * Mode 0:
	 * After the write of the LSB and before the write of the MSB,
	 * this should return LOW.
	 */

	/* 
	 * If the timer was not started yet or is disabled,
	 * only Mode 0 is LOW
	 */
	if (timer->gate_high || timer->start_tick > curtick)
		return (timer->active_mode != 0);

	curtick -= timer->start_tick;

	/* Return LOW until counter is 0, afterwards HIGH until reload. */
	if (timer->active_mode == 0 || timer->active_mode == 1)
		return curtick >= timer->start_tick;

	/* Return LOW until the counter is 0, raise to HIGH and go LOW again. */
	if (timer->active_mode == 5 || timer->active_mode == 7)
		return curtick != timer->start_tick;

	/*
	 * Return LOW until the counter is 1, raise to HIGH and go LOW
	 * again. Afterwards reload the counter.
	 */
	if (timer->active_mode == 2 || timer->active_mode == 3) {
		curtick %= timer->active_counter;
		return curtick + 1 != timer->active_counter;
	}

	/*
	 * If the initial counter is even, return HIGH for the first half
	 * and LOW for the second. If it is even, bias the first half.
	 */
	curtick %= timer->active_counter;
	return curtick < (timer->active_counter + 1) / 2;
}

static void
x86emu_i8254_latch_status(struct x86emu_i8254_timer *timer, uint64_t curtick)
{
	if (timer->status_is_latched)
		return;
	timer->latched_status = timer->active_is_bcd ? 1 : 0;
	timer->latched_status |= timer->active_mode << 1;
	timer->latched_status |= timer->rw_status;
	timer->latched_status |= timer->null_count ? 0x40 : 0;
}

static void
x86emu_i8254_latch_counter(struct x86emu_i8254_timer *timer, uint64_t curtick)
{
	if (!timer->counter_is_latched)
		return; /* Already latched. */
	timer->latched_counter = x86emu_i8254_counter(timer, curtick);
	timer->counter_is_latched = true;
}

static void
x86emu_i8254_write_command(struct x86emu_i8254 *sc, uint8_t val)
{
	struct x86emu_i8254_timer *timer;
	int i;

	if ((val >> 6) == 3) {
		/* Read Back Command */
		uint64_t curtick;

		curtick = x86emu_i8254_gettick(sc);
		for (i = 0; i < 3; ++i) {
			timer = &sc->timer[i];

			if ((val & (2 << i)) == 0)
				continue;
			if ((val & 0x10) != 0)
				x86emu_i8254_latch_status(timer, curtick);
			if ((val & 0x20) != 0)
				x86emu_i8254_latch_counter(timer, curtick);
		}
		return;
	}

	timer = &sc->timer[val >> 6];

	switch (val & 0x30) {
	case 0:
		x86emu_i8254_latch_counter(timer, x86emu_i8254_gettick(sc));
		return;
	case 1:
		timer->write_lsb = timer->read_lsb = true;
		timer->write_msb = timer->read_msb = false;
		break;
	case 2:
		timer->write_lsb = timer->read_lsb = false;
		timer->write_msb = timer->read_msb = true;
		break;
	case 3:
		timer->write_lsb = timer->read_lsb = true;
		timer->write_msb = timer->read_msb = true;
		break;
	}
	timer->rw_status = val & 0x30;
	timer->null_count = true;
	timer->new_mode = (val >> 1) & 0x7;
	timer->new_is_bcd = (val & 1) == 1;
}

static uint8_t
x86emu_i8254_read_counter(struct x86emu_i8254 *sc,
    struct x86emu_i8254_timer *timer)
{
	uint16_t val;
	uint8_t output;

	/* If status was latched by Read Back Command, return it. */
	if (timer->status_is_latched) {
		timer->status_is_latched = false;
		return timer->latched_status;
	}

	/*
	 * The value of the counter is either the latched value
	 * or the current counter.
	 */
	if (timer->counter_is_latched)
		val = timer->latched_counter;
	else
		val = x86emu_i8254_counter(&sc->timer[2],
		    x86emu_i8254_gettick(sc));

	if (timer->active_is_bcd)
		val = bin2bcd(val);

	/* Extract requested byte. */
	if (timer->read_lsb) {
		output = val & 0xff;
		timer->read_lsb = false;
	} else if (timer->read_msb) {
		output = val >> 8;
		timer->read_msb = false;
	} else
		output = 0; /* Undefined value. */

	/* Clean latched status if all requested bytes have been read. */
	if (!timer->read_lsb && !timer->read_msb)
		timer->counter_is_latched = false;

	return output;
}

static void
x86emu_i8254_write_counter(struct x86emu_i8254 *sc,
    struct x86emu_i8254_timer *timer, uint8_t val)
{
	/* Nothing to write, undefined. */
	if (!timer->write_lsb && !timer->write_msb)
		return;

	/* Update requested bytes. */
	if (timer->write_lsb) {
		timer->new_counter &= ~0xff;
		timer->new_counter |= val;
		timer->write_lsb = false;
	} else {
		KASSERT(timer->write_msb);
		timer->new_counter &= ~0xff00;
		timer->new_counter |= val << 8;
		timer->write_msb = false;
	}

	/* If all requested bytes have been written, update counter. */
	if (!timer->write_lsb && !timer->write_msb) {
		timer->null_count = false;
		timer->counter_is_latched = false;
		timer->status_is_latched = false;
		timer->active_is_bcd = timer->new_is_bcd;
		timer->active_mode = timer->new_mode;
		timer->start_tick = x86emu_i8254_gettick(sc) + 1;
		if (timer->new_is_bcd)
			timer->active_counter = bcd2bin(timer->new_counter);
	}
}

static uint8_t
x86emu_i8254_read_nmi(struct x86emu_i8254 *sc)
{
	uint8_t val;

	val = (sc->timer[2].gate_high) ? 1 : 0;
	if (x86emu_i8254_out(&sc->timer[2], x86emu_i8254_gettick(sc)))
		val |= 0x20;

	return val;
}

static void
x86emu_i8254_write_nmi(struct x86emu_i8254 *sc, uint8_t val)
{
	bool old_gate;

	old_gate = sc->timer[2].gate_high;
	sc->timer[2].gate_high = (val & 1) == 1;
	if (!old_gate && sc->timer[2].gate_high)
		sc->timer[2].start_tick = x86emu_i8254_gettick(sc) + 1;
}

void
x86emu_i8254_init(struct x86emu_i8254 *sc, void (*gettime)(struct timespec *))
{
	struct x86emu_i8254_timer *timer;
	int i;

	sc->gettime = gettime;
	(*sc->gettime)(&sc->base_time);

	for (i = 0; i < 3; ++i) {
		timer = &sc->timer[i];
		timer->gate_high = false;
		timer->start_tick = 0;
		timer->active_counter = 0;
		timer->active_mode = 0;
		timer->active_is_bcd = false;
		timer->counter_is_latched = false;
		timer->read_lsb = false;
		timer->read_msb = false;
		timer->status_is_latched = false;
		timer->null_count = false;
	}
}

uint8_t
x86emu_i8254_inb(struct x86emu_i8254 *sc, uint16_t port)
{
	KASSERT(x86emu_i8254_claim_port(sc, port));
	if (port == 0x40)
		return x86emu_i8254_read_counter(sc, &sc->timer[0]);
	if (port == 0x41)
		return x86emu_i8254_read_counter(sc, &sc->timer[1]);
	if (port == 0x42)
		return x86emu_i8254_read_counter(sc, &sc->timer[2]);
	if (port == 0x43)
		return 0xff; /* unsupported */
	return	x86emu_i8254_read_nmi(sc);
}

void
x86emu_i8254_outb(struct x86emu_i8254 *sc, uint16_t port, uint8_t val)
{
	KASSERT(x86emu_i8254_claim_port(sc, port));
	if (port == 0x40)
		x86emu_i8254_write_counter(sc, &sc->timer[0], val);
	else if (port == 0x41)
		x86emu_i8254_write_counter(sc, &sc->timer[1], val);
	else if (port == 0x42)
		x86emu_i8254_write_counter(sc, &sc->timer[2], val);
	else if (port == 0x43)
		x86emu_i8254_write_command(sc, val);
	else
		x86emu_i8254_write_nmi(sc, val);
}

/* ARGSUSED */
bool
x86emu_i8254_claim_port(struct x86emu_i8254 *sc, uint16_t port)
{
	/* i8254 registers */
	if (port >= 0x40 && port < 0x44)
		return true;
	/* NMI register, used to control timer 2 and the output of it */
	if (port == 0x61)
		return true;
	return false;
}
