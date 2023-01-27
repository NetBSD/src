/*	$NetBSD: clock.c,v 1.13 2023/01/27 15:21:52 tsutsui Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.13 2023/01/27 15:21:52 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <next68k/dev/clockreg.h>
#include <next68k/dev/intiovar.h>

#include <next68k/next68k/rtc.h>
#include <next68k/next68k/isr.h>

/* @@@ This is pretty bogus and will need fixing once
 * things are working better.
 * -- jewell@mit.edu
 */

int clock_intr(void *);

int
clock_intr(void *arg)
{
	volatile struct timer_reg *timer;
	int whilecount = 0;

	if (!INTR_OCCURRED(NEXT_I_TIMER)) {
		return(0);
	}

	do {
		static int in_hardclock = 0;
		int s;
		
		timer = (volatile struct timer_reg *)IIOV(NEXT_P_TIMER);
		timer->csr |= TIMER_REG_UPDATE;

		if (! in_hardclock) {
			in_hardclock = 1;
			s = splclock ();
			hardclock(arg);
			splx(s);
			in_hardclock = 0;
		}
		if (whilecount++ > 10)
			panic ("whilecount");
	} while (INTR_OCCURRED(NEXT_I_TIMER));
	return(1);
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only
 * if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks(void)
{
	int s, cnt;
	volatile struct timer_reg *timer;

	rtc_init();
	hz = 100;
	s = splclock();
	timer = (volatile struct timer_reg *)IIOV(NEXT_P_TIMER);
	cnt = 1000000/hz;          /* usec timer */
	timer->csr = 0;
	timer->msb = (cnt >> 8);
	timer->lsb = cnt;
	timer->csr = TIMER_REG_ENABLE|TIMER_REG_UPDATE;
	isrlink_autovec(clock_intr, NULL, NEXT_I_IPL(NEXT_I_TIMER), 0, NULL);
	INTR_ENABLE(NEXT_I_TIMER);
	splx(s);
}


void
setstatclockrate(int newhz)
{

	/* XXX should we do something here? XXX */
}

/* @@@ update this to use the usec timer 
 * Darrin B Jewell <jewell@mit.edu>  Sun Feb  8 05:01:02 1998
 * XXX: Well, there is no more microtime.  But if there is a hardware
 * timer of any sort, it could be added.   ENODOCS.  gdamore.
 */

