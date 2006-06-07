/*	$NetBSD: timer_sun4.c,v 1.14 2006/06/07 22:38:50 kardel Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
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
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Sun4/Sun4c timer support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer_sun4.c,v 1.14 2006/06/07 22:38:50 kardel Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/timerreg.h>
#include <sparc/sparc/timervar.h>

#define	timerreg4	((struct timerreg_4 *)TIMERREG_VA)

/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
timer_init_4(void)
{

	timerreg4->t_c10.t_limit = tmr_ustolim(tick);
	timerreg4->t_c14.t_limit = tmr_ustolim(statint);
	ienab_bis(IE_L14 | IE_L10);
}

/*
 * Level 10 (clock) interrupts from system counter.
 */
int
clockintr_4(void *cap)
{

	/* read the limit register to clear the interrupt */
	*((volatile int *)&timerreg4->t_c10.t_limit);
	tickle_tc();
	hardclock((struct clockframe *)cap);
	return (1);
}

/*
 * Level 14 (stat clock) interrupts from processor counter.
 */
int
statintr_4(void *cap)
{
	struct clockframe *frame = cap;
	u_long newint;

	/* read the limit register to clear the interrupt */
	*((volatile int *)&timerreg4->t_c14.t_limit);

	statclock(frame);

	/*
	 * Compute new randomized interval.
	 */
	newint = new_interval();

	/*
	 * The sun4/4c timer has no `non-resetting' register;
	 * use the current counter value to compensate the new
	 * limit value for the number of counter ticks elapsed.
	 */
	newint -= tmr_cnttous(timerreg4->t_c14.t_counter);
	timerreg4->t_c14.t_limit = tmr_ustolim(newint);

	/*
	 * The factor 8 is only valid for stathz==100.
	 * See also clock.c
	 */
	if (curlwp && (++cpuinfo.ci_schedstate.spc_schedticks & 7) == 0) {
		if (CLKF_LOPRI(frame, IPL_SCHED)) {
			/* No need to schedule a soft interrupt */
			spllowerschedclock();
			schedintr(cap);
		} else {
			/*
			 * We're interrupting a thread that may have the
			 * scheduler lock; run schedintr() later.
			 */
			softintr_schedule(sched_cookie);
		}
	}

	return (1);
}

#if defined(SUN4)
void
timerattach_obio_4(struct device *parent, struct device *self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_handle_t bh;

	if (bus_space_map2(oba->oba_bustag,
			   oba->oba_paddr,
			   sizeof(struct timerreg_4),
			   BUS_SPACE_MAP_LINEAR,
			   TIMERREG_VA,
			   &bh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	timerattach(&timerreg4->t_c14.t_counter, &timerreg4->t_c14.t_limit);
}
#endif /* SUN4 */

#if defined(SUN4C)
void
timerattach_mainbus_4c(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	/*
	 * This time we ignore any existing virtual address because
	 * we have a fixed virtual address for the timer, to make
	 * microtime() faster.
	 */
	if (bus_space_map2(ma->ma_bustag,
			   ma->ma_paddr,
			   sizeof(struct timerreg_4),
			   BUS_SPACE_MAP_LINEAR,
			   TIMERREG_VA, &bh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	timerattach(&timerreg4->t_c14.t_counter, &timerreg4->t_c14.t_limit);
}
#endif /* SUN4C */
