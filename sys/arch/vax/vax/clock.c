/*	$NetBSD: clock.c,v 1.56.2.1 2017/12/03 11:36:48 jdolecek Exp $	 */
/*
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.56.2.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/clock.h>

#include "opt_cputype.h"

struct evcnt clock_misscnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "clock", "intr miss");

EVCNT_ATTACH_STATIC(clock_misscnt);

struct evcnt clock_intrcnt =
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "clock", "intr");

EVCNT_ATTACH_STATIC(clock_intrcnt);

static int vax_gettime(todr_chip_handle_t, struct timeval *);
static int vax_settime(todr_chip_handle_t, struct timeval *);

static struct todr_chip_handle todr_handle = {
	.todr_gettime = vax_gettime,
	.todr_settime = vax_settime,
};

#if VAX46 || VAXANY
static u_int
vax_diag_get_counter(struct timecounter *tc)
{
	extern struct vs_cpu *ka46_cpu;
	int cur_hardclock;
	u_int counter;

	do {
		cur_hardclock = hardclock_ticks;
		counter = *(volatile u_int *)&ka46_cpu->vc_diagtimu;
	} while (cur_hardclock != hardclock_ticks);

	counter = (counter & 0x3ff) + (counter >> 16) * 1024;

	return counter + hardclock_ticks * tick;
}
#endif

static u_int
vax_mfpr_get_counter(struct timecounter *tc)
{
	int cur_hardclock;
	u_int counter;
	static int prev_count, prev_hardclock;

	do {
		cur_hardclock = hardclock_ticks;
		counter = mfpr(PR_ICR) + tick;
	} while (cur_hardclock != hardclock_ticks);

	/*
	 * Handle interval counter wrapping with interrupts blocked.
	 * If the current hardclock_ticks is less than what we saw
	 *   previously, use the previous value.
	 * If the interval counter is smaller, assume it has wrapped,
	 *   and if the [adjusted] current hardclock ticks is the same
	 *   as what we saw previously, increment the local copy of
	 *   the hardclock ticks.
	 */
	if (cur_hardclock < prev_hardclock)
		cur_hardclock = prev_hardclock;
	if (counter < prev_count && cur_hardclock == prev_hardclock)
		cur_hardclock++;

	prev_count = counter;
	prev_hardclock=cur_hardclock;

	return counter + cur_hardclock * tick;
}

#if VAX46 || VAXANY
static struct timecounter vax_diag_tc = {
	vax_diag_get_counter,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	1000000,		/* frequency */
	"diagtimer",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};
#endif

static struct timecounter vax_mfpr_tc = {
	vax_mfpr_get_counter,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	1000000,		/* frequency */
	"mfpr",			/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

/*
 * A delayloop that delays about the number of milliseconds that is
 * given as argument.
 */
void
delay(int i)
{
	__asm ("1: sobgtr %0, 1b" : : "r" (dep_call->cpu_vups * i));
}

/*
 * On all VAXen there are a microsecond clock that should
 * be used for interval interrupts. Some CPUs don't use the ICR interval
 * register but it doesn't hurt to load it anyway.
 */
void
cpu_initclocks(void)
{
	mtpr(-10000, PR_NICR); /* Load in count register */
	mtpr(0x800000d1, PR_ICCS); /* Start clock and enable interrupt */

	todr_attach(&todr_handle);

#if VAX46 || VAXANY
	if (vax_boardtype == VAX_BTYP_46)
		tc_init(&vax_diag_tc);
#endif
	if (vax_boardtype != VAX_BTYP_46 && vax_boardtype != VAX_BTYP_48)
		tc_init(&vax_mfpr_tc);
}

int
vax_gettime(todr_chip_handle_t handle, struct timeval *tvp)
{
	tvp->tv_sec = handle->base_time;
	return (*dep_call->cpu_gettime)(tvp);
}

int
vax_settime(todr_chip_handle_t handle, struct timeval *tvp)
{
	(*dep_call->cpu_settime)(tvp);
	return 0;
}

/*
 * There are two types of real-time battery-backed up clocks on
 * VAX computers, one with a register that counts up every 1/100 second,
 * one with a clock chip that delivers time. For the register clock
 * we have a generic version, and for the chip clock there are 
 * support routines for time conversion.
 */
/*
 * Converts a year to corresponding number of ticks.
 */
int
yeartonum(int y)
{
	int n;

	for (n = 0, y -= 1; y > 1969; y--)
 		n += days_per_year(y) * SECS_PER_DAY;
	return n;
}

/* 
 * Converts tick number to a year 1970 ->
 */
int
numtoyear(int num)
{
	int y = 1970, j;
	while(num >= (j = days_per_year(y) * SECS_PER_DAY)) {
		y++;
		num -= j;
	}
	return y;
}

#if VAX750 || VAX780 || VAX8600 || VAX650 || \
    VAX660 || VAX670 || VAX680 || VAX53 || VAXANY
/*
 * Reads the TODR register; returns a (probably) true tick value, and 0 is
 * success or EINVAL if failed.  The year is based on the argument
 * year; the TODR doesn't hold years.
 */
int
generic_gettime(struct timeval *tvp)
{
	unsigned klocka = mfpr(PR_TODR);

	/*
	 * Sanity check.
	 */
	if (klocka < TODRBASE) {
		if (klocka == 0)
			printf("TODR stopped");
		else
			printf("TODR too small");
		return EINVAL;
	}

	tvp->tv_sec = yeartonum(numtoyear(tvp->tv_sec)) + (klocka - TODRBASE) / 100;
	return 0;
}

/*
 * Takes the current system time and writes it to the TODR.
 */
void
generic_settime(struct timeval *tvp)
{
	unsigned tid = tvp->tv_sec, bastid;

	bastid = tid - yeartonum(numtoyear(tid));
	mtpr((bastid * 100) + TODRBASE, PR_TODR);
}
#endif

#if VAX630 || VAX410 || VAX43 || VAX8200 || VAX46 || VAX48 || VAX49 || VAXANY

volatile short *clk_page;	/* where the chip is mapped in virtual memory */
int	clk_adrshift;	/* how much to multiply the in-page address with */
int	clk_tweak;	/* Offset of time into word. */

#define	REGPEEK(off)	(clk_page[off << clk_adrshift] >> clk_tweak)
#define	REGPOKE(off, v)	(clk_page[off << clk_adrshift] = ((v) << clk_tweak))

int
chip_gettime(struct timeval *tvp)
{
	struct clock_ymdhms c;
	int timeout = 1<<15, s;

#ifdef DIAGNOSTIC
	if (clk_page == 0)
		panic("trying to use unset chip clock page");
#endif

	if ((REGPEEK(CSRD_OFF) & CSRD_VRT) == 0) {
		printf("WARNING: TOY clock not marked valid\n");
		return EINVAL;
	}
	while (REGPEEK(CSRA_OFF) & CSRA_UIP) {
		if (--timeout == 0) {
			printf ("TOY clock timed out");
			return ETIMEDOUT;
		}
	}

	s = splhigh();
	c.dt_year = ((u_char)REGPEEK(YR_OFF)) + 1970;
	c.dt_mon = REGPEEK(MON_OFF);
	c.dt_day = REGPEEK(DAY_OFF);
	c.dt_wday = REGPEEK(WDAY_OFF);
	c.dt_hour = REGPEEK(HR_OFF);
	c.dt_min = REGPEEK(MIN_OFF);
	c.dt_sec = REGPEEK(SEC_OFF);
	splx(s);

	tvp->tv_sec = clock_ymdhms_to_secs(&c);
	tvp->tv_usec = 0;
	return 0;
}

void
chip_settime(struct timeval *tvp)
{
	struct clock_ymdhms c;

#ifdef DIAGNOSTIC
	if (clk_page == 0)
		panic("trying to use unset chip clock page");
#endif

	REGPOKE(CSRB_OFF, CSRB_SET);

	clock_secs_to_ymdhms(tvp->tv_sec, &c);

	REGPOKE(YR_OFF, ((u_char)(c.dt_year - 1970)));
	REGPOKE(MON_OFF, c.dt_mon);
	REGPOKE(DAY_OFF, c.dt_day);
	REGPOKE(WDAY_OFF, c.dt_wday);
	REGPOKE(HR_OFF, c.dt_hour);
	REGPOKE(MIN_OFF, c.dt_min);
	REGPOKE(SEC_OFF, c.dt_sec);

	REGPOKE(CSRB_OFF, CSRB_DM|CSRB_24);
};
#endif
