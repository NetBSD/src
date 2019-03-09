/*	$NetBSD: clock.c,v 1.33.56.1 2019/03/09 17:10:20 martin Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */
/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * Primitive clock interrupt routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.33.56.1 2019/03/09 17:10:20 martin Exp $");

/* #define CLOCKDEBUG */
/* #define CLOCK_PARANOIA */

#include "opt_multiprocessor.h"
#include "opt_ntp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/pio.h>
#include <machine/cpufunc.h>
#include <machine/lock.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/i8253reg.h>
#include <i386/isa/nvram.h>
#include <x86/x86/tsc.h>
#include <x86/lock.h>
#include <machine/specialreg.h> 
#include <x86/rtc.h>

#ifndef __x86_64__
#include "mca.h"
#endif
#if NMCA > 0
#include <machine/mca_machdep.h>	/* for MCA_system */
#endif

#include "pcppi.h"
#if (NPCPPI > 0)
#include <dev/isa/pcppivar.h>

int sysbeepmatch(device_t, cfdata_t, void *);
void sysbeepattach(device_t, device_t, void *);
int sysbeepdetach(device_t, int);

CFATTACH_DECL3_NEW(sysbeep, 0,
    sysbeepmatch, sysbeepattach, sysbeepdetach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

static int ppi_attached;
static pcppi_tag_t ppicookie;
#endif /* PCPPI */

#ifdef CLOCKDEBUG
int clock_debug = 0;
#define DPRINTF(arg) if (clock_debug) printf arg
#else
#define DPRINTF(arg)
#endif

void (*x86_delay)(unsigned int) = i8254_delay;

void		sysbeep(int, int);
static void     tickle_tc(void);

static int	clockintr(void *, struct intrframe *);

int 		sysbeepdetach(device_t, int);

static unsigned int	gettick_broken_latch(void);

static volatile uint32_t i8254_lastcount;
static volatile uint32_t i8254_offset;
static volatile int i8254_ticked;

/* to protect TC timer variables */
static __cpu_simple_lock_t tmr_lock = __SIMPLELOCK_UNLOCKED;

u_int i8254_get_timecount(struct timecounter *);

static struct timecounter i8254_timecounter = {
	i8254_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	TIMER_FREQ,		/* frequency */
	"i8254",		/* name */
	100,			/* quality */
	NULL,			/* private data */
	NULL,			/* next */
};

u_long rtclock_tval;		/* i8254 reload value for countdown */
int    rtclock_init = 0;

int clock_broken_latch = 0;

#ifdef CLOCK_PARANOIA
static int ticks[6];
#endif
/*
 * i8254 latch check routine:
 *     National Geode (formerly Cyrix MediaGX) has a serious bug in
 *     its built-in i8254-compatible clock module.
 *     machdep sets the variable 'clock_broken_latch' to indicate it.
 */

static unsigned int
gettick_broken_latch(void)
{
	int v1, v2, v3;
	int w1, w2, w3;
	int s;

	/* Don't want someone screwing with the counter while we're here. */
	s = splhigh();
	__cpu_simple_lock(&tmr_lock);
	v1 = inb(IO_TIMER1+TIMER_CNTR0);
	v1 |= inb(IO_TIMER1+TIMER_CNTR0) << 8;
	v2 = inb(IO_TIMER1+TIMER_CNTR0);
	v2 |= inb(IO_TIMER1+TIMER_CNTR0) << 8;
	v3 = inb(IO_TIMER1+TIMER_CNTR0);
	v3 |= inb(IO_TIMER1+TIMER_CNTR0) << 8;
	__cpu_simple_unlock(&tmr_lock);
	splx(s);

#ifdef CLOCK_PARANOIA
	if (clock_debug) {
		ticks[0] = ticks[3];
		ticks[1] = ticks[4];
		ticks[2] = ticks[5];
		ticks[3] = v1;
		ticks[4] = v2;
		ticks[5] = v3;
	}
#endif

	if (v1 >= v2 && v2 >= v3 && v1 - v3 < 0x200)
		return (v2);

#define _swap_val(a, b) do { \
	int c = a; \
	a = b; \
	b = c; \
} while (0)

	/*
	 * sort v1 v2 v3
	 */
	if (v1 < v2)
		_swap_val(v1, v2);
	if (v2 < v3)
		_swap_val(v2, v3);
	if (v1 < v2)
		_swap_val(v1, v2);

	/*
	 * compute the middle value
	 */

	if (v1 - v3 < 0x200)
		return (v2);

	w1 = v2 - v3;
	w2 = v3 - v1 + rtclock_tval;
	w3 = v1 - v2;
	if (w1 >= w2) {
		if (w1 >= w3)
		        return (v1);
	} else {
		if (w2 >= w3)
			return (v2);
	}
	return (v3);
}

/* minimal initialization, enough for delay() */
void
initrtclock(u_long freq)
{
	u_long tval;

	/*
	 * Compute timer_count, the count-down count the timer will be
	 * set to.  Also, correctly round
	 * this by carrying an extra bit through the division.
	 */
	tval = (freq * 2) / (u_long) hz;
	tval = (tval / 2) + (tval & 0x1);

	/* initialize 8254 clock */
	outb(IO_TIMER1+TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);

	/* Correct rounding will buy us a better precision in timekeeping */
	outb(IO_TIMER1+TIMER_CNTR0, tval % 256);
	outb(IO_TIMER1+TIMER_CNTR0, tval / 256);

	rtclock_tval = tval ? tval : 0xFFFF;
	rtclock_init = 1;
}

void
startrtclock(void)
{
	int s;

	if (!rtclock_init)
		initrtclock(TIMER_FREQ);

	/* Check diagnostic status */
	if ((s = mc146818_read(NULL, NVRAM_DIAG)) != 0) { /* XXX softc */
		char bits[128];
		snprintb(bits, sizeof(bits), NVRAM_DIAG_BITS, s);
		printf("RTC BIOS diagnostic error %s\n", bits);
	}

	tc_init(&i8254_timecounter);
	rtc_register();
}

/*
 * Must be called at splsched().
 */
static void
tickle_tc(void) 
{
#if defined(MULTIPROCESSOR)
	struct cpu_info *ci = curcpu();
	/*
	 * If we are not the primary CPU, we're not allowed to do
	 * any more work.
	 */
	if (CPU_IS_PRIMARY(ci) == 0)
		return;
#endif
	if (rtclock_tval && timecounter->tc_get_timecount == i8254_get_timecount) {
		__cpu_simple_lock(&tmr_lock);
		if (i8254_ticked)
			i8254_ticked    = 0;
		else {
			i8254_offset   += rtclock_tval;
			i8254_lastcount = 0;
		}
		__cpu_simple_unlock(&tmr_lock);
	}

}

static int
clockintr(void *arg, struct intrframe *frame)
{
	tickle_tc();

	hardclock((struct clockframe *)frame);

#if NMCA > 0
	if (MCA_system) {
		/* Reset PS/2 clock interrupt by asserting bit 7 of port 0x61 */
		outb(0x61, inb(0x61) | 0x80);
	}
#endif
	return -1;
}

u_int
i8254_get_timecount(struct timecounter *tc)
{
	u_int count;
	uint16_t rdval;
	u_long psl;

	/* Don't want someone screwing with the counter while we're here. */
	psl = x86_read_psl();
	x86_disable_intr();
	__cpu_simple_lock(&tmr_lock);
	/* Select timer0 and latch counter value. */ 
	outb(IO_TIMER1 + TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	/* insb to make the read atomic */
	rdval = inb(IO_TIMER1+TIMER_CNTR0);
	rdval |= (inb(IO_TIMER1+TIMER_CNTR0) << 8);
	count = rtclock_tval - rdval;
	if (rtclock_tval && (count < i8254_lastcount &&
			     (!i8254_ticked || rtclock_tval == 0xFFFF))) {
		i8254_ticked = 1;
		i8254_offset += rtclock_tval;
	}
	i8254_lastcount = count;
	count += i8254_offset;
	__cpu_simple_unlock(&tmr_lock);
	x86_write_psl(psl);

	return (count);
}

unsigned int
gettick(void)
{
	uint16_t rdval;
	u_long psl;
	
	if (clock_broken_latch)
		return (gettick_broken_latch());

	/* Don't want someone screwing with the counter while we're here. */
	psl = x86_read_psl();
	x86_disable_intr();
	__cpu_simple_lock(&tmr_lock);
	/* Select counter 0 and latch it. */
	outb(IO_TIMER1+TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	rdval = inb(IO_TIMER1+TIMER_CNTR0);
	rdval |= (inb(IO_TIMER1+TIMER_CNTR0) << 8);
	__cpu_simple_unlock(&tmr_lock);
	x86_write_psl(psl);

	return rdval;
}

/*
 * Wait approximately `n' microseconds.
 * Relies on timer 1 counting down from (TIMER_FREQ / hz) at TIMER_FREQ Hz.
 * Note: timer had better have been programmed before this is first used!
 * (Note that we use `rate generator' mode, which counts at 1:1; `square
 * wave' mode counts at 2:1).
 * Don't rely on this being particularly accurate.
 */
void
i8254_delay(unsigned int n)
{
	unsigned int cur_tick, initial_tick;
	int remaining;

	/* allow DELAY() to be used before startrtclock() */
	if (!rtclock_init)
		initrtclock(TIMER_FREQ);

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	initial_tick = gettick();

	if (n <= UINT_MAX / TIMER_FREQ) {
		/*
		 * For unsigned arithmetic, division can be replaced with
		 * multiplication with the inverse and a shift.
		 */
		remaining = n * TIMER_FREQ / 1000000;
	} else {
		/* This is a very long delay.
		 * Being slow here doesn't matter.
		 */
		remaining = (unsigned long long) n * TIMER_FREQ / 1000000;
	}

	while (remaining > 1) {
#ifdef CLOCK_PARANOIA
		int delta;
		cur_tick = gettick();
		if (cur_tick > initial_tick)
			delta = rtclock_tval - (cur_tick - initial_tick);
		else
			delta = initial_tick - cur_tick;
		if (delta < 0 || delta >= rtclock_tval / 2) {
			DPRINTF(("delay: ignore ticks %.4x-%.4x",
				 initial_tick, cur_tick));
			if (clock_broken_latch) {
				DPRINTF(("  (%.4x %.4x %.4x %.4x %.4x %.4x)\n",
				         ticks[0], ticks[1], ticks[2],
				         ticks[3], ticks[4], ticks[5]));
			} else {
				DPRINTF(("\n"));
			}
		} else
			remaining -= delta;
#else
		cur_tick = gettick();
		if (cur_tick > initial_tick)
			remaining -= rtclock_tval - (cur_tick - initial_tick);
		else
			remaining -= initial_tick - cur_tick;
#endif
		initial_tick = cur_tick;
	}
}

#if (NPCPPI > 0)
int
sysbeepmatch(device_t parent, cfdata_t match, void *aux)
{
	return (!ppi_attached);
}

void
sysbeepattach(device_t parent, device_t self, void *aux)
{
	aprint_naive("\n");
	aprint_normal("\n");

	ppicookie = ((struct pcppi_attach_args *)aux)->pa_cookie;
	ppi_attached = 1;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

int
sysbeepdetach(device_t self, int flags)
{
	pmf_device_deregister(self);
	ppi_attached = 0;
	return 0;
}
#endif

void
sysbeep(int pitch, int period)
{
#if (NPCPPI > 0)
	if (ppi_attached)
		pcppi_bell(ppicookie, pitch, period, 0);
#endif
}

void
i8254_initclocks(void)
{

	/*
	 * XXX If you're doing strange things with multiple clocks, you might
	 * want to keep track of clock handlers.
	 */
	(void)isa_intr_establish(NULL, 0, IST_PULSE, IPL_CLOCK,
	    (int (*)(void *))clockintr, 0);
}

void
setstatclockrate(int arg)
{
}
