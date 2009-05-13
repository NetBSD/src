/*	$NetBSD: clock.c,v 1.6.14.1 2009/05/13 17:18:12 jym Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.6.14.1 2009/05/13 17:18:12 jym Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* time */

#include <mips/locore.h>

#include <dev/clock_subr.h>
#include <machine/bootinfo.h>

#include <playstation2/ee/timervar.h>

static int get_bootinfo_tod(todr_chip_handle_t, struct clock_ymdhms *);

void
cpu_initclocks(void)
{
	struct todr_chip_handle	todr = {
		.todr_gettime_ymdhms = get_bootinfo_tod;
	};

	/*
	 *  PS2 R5900 CPU clock is 294.912 MHz = (1 << 15) * 9 * 1000
	 */
	curcpu()->ci_cpu_freq = 294912000;

	hz = 100;

	/* Install clock interrupt */
	timer_clock_init();

	todr_attach(&todr);

	mips3_init_tc();
}

void
setstatclockrate(int arg)
{
	/* not yet */
}

static int
get_bootinfo_tod(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	time_t utc;
	struct bootinfo_rtc *rtc = 
	    (void *)MIPS_PHYS_TO_KSEG1(BOOTINFO_BLOCK_BASE + BOOTINFO_RTC);

	/* PS2 RTC is JST */
	dt->dt_year = FROMBCD(rtc->year) + 2000;
	dt->dt_mon = FROMBCD(rtc->mon);
	dt->dt_day = FROMBCD(rtc->day);
	dt->dt_hour = FROMBCD(rtc->hour);
	dt->dt_min = FROMBCD(rtc->min);
	dt->dt_sec = FROMBCD(rtc->sec);

	/* convert to UTC */
	utc = clock_ymdhms_to_secs(dt) - 9*60*60;
	clock_secs_to_ymdhms(utc, dt);
#ifdef DEBUG
        printf("bootinfo: %d/%d/%d/%d/%d/%d rtc_offset %d\n", dt->dt_year,
	    dt->dt_mon, dt->dt_day, dt->dt_hour, dt->dt_min, dt->dt_sec,
	    rtc_offset);
#endif
	return 0;
}
