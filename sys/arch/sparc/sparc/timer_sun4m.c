/*	$NetBSD: timer_sun4m.c,v 1.22 2010/01/13 02:17:12 mrg Exp $	*/

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
 * Sun4m timer support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer_sun4m.c,v 1.22 2010/01/13 02:17:12 mrg Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/timerreg.h>
#include <sparc/sparc/timervar.h>

struct timer_4m		*timerreg4m;
#define	counterreg4m	cpuinfo.counterreg_4m

/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
timer_init_4m(void)
{
	struct cpu_info *cpi;
	int n;

	timerreg4m->t_limit = tmr_ustolim4m(tick);
	for (CPU_INFO_FOREACH(n, cpi)) {
		cpi->counterreg_4m->t_limit = tmr_ustolim4m(statint);
	}
	icr_si_bic(SINTR_T);
}

void
schedintr_4m(void *v)
{

#ifdef MULTIPROCESSOR
	/*
	 * We call hardclock() here so that we make sure it is called on
	 * all CPUs.  This function ends up being called on sun4m systems
	 * every tick.
	 */
	hardclock(v);

	/*
	 * The factor 8 is only valid for stathz==100.
	 * See also clock.c
	 */
	if ((++cpuinfo.ci_schedstate.spc_schedticks & 7) == 0 && schedhz != 0)
#endif
		schedclock(curlwp);
}


/*
 * Level 10 (clock) interrupts from system counter.
 */
int
clockintr_4m(void *cap)
{

	/*
	 * XXX this needs to be fixed in a more general way
	 * problem is that the kernel enables interrupts and THEN
	 * sets up clocks. In between there's an opportunity to catch
	 * a timer interrupt - if we call hardclock() at that point we'll
	 * panic
	 * so for now just bail when cold
	 *
	 * For MP, we defer calling hardclock() to the schedintr so
	 * that we call it on all cpus.
	 */
	cpuinfo.ci_lev10.ev_count++;
	if (cold)
		return 0;
	/* read the limit register to clear the interrupt */
	*((volatile int *)&timerreg4m->t_limit);
	tickle_tc();
#if !defined(MULTIPROCESSOR)
	hardclock((struct clockframe *)cap);
#endif
	return (1);
}

/*
 * Level 14 (stat clock) interrupts from processor counter.
 */
int
statintr_4m(void *cap)
{
	struct clockframe *frame = cap;
	u_long newint;

	cpuinfo.ci_lev14.ev_count++;

	/* read the limit register to clear the interrupt */
	*((volatile int *)&counterreg4m->t_limit);

	statclock(frame);

	/*
	 * Compute new randomized interval.
	 */
	newint = new_interval();

	/*
	 * Use the `non-resetting' limit register, so we don't
	 * loose the counter ticks that happened since this
	 * interrupt was raised.
	 */
	counterreg4m->t_limit_nr = tmr_ustolim4m(newint);

	/*
	 * The factor 8 is only valid for stathz==100.
	 * See also clock.c
	 */
#if !defined(MULTIPROCESSOR)
	if ((++cpuinfo.ci_schedstate.spc_schedticks & 7) == 0 && schedhz != 0) {
#endif
		if (CLKF_LOPRI(frame, IPL_SCHED)) {
			/* No need to schedule a soft interrupt */
			spllowerschedclock();
			schedintr_4m(cap);
		} else {
			/*
			 * We're interrupting a thread that may have the
			 * scheduler lock; run schedintr_4m() on this CPU later.
			 */
			raise_ipi(&cpuinfo, IPL_SCHED); /* sched_cookie->pil */
		}
#if !defined(MULTIPROCESSOR)
	}
#endif

	return (1);
}

void
timerattach_obio_4m(struct device *parent, struct device *self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	struct cpu_info *cpi;
	bus_space_handle_t bh;
	int i, n;

	if (sa->sa_nreg < 2) {
		printf(": only %d register sets\n", sa->sa_nreg);
		return;
	}

	/* Map the system timer */
	i = sa->sa_nreg - 1;
	if (bus_space_map2(sa->sa_bustag,
			   BUS_ADDR(sa->sa_reg[i].oa_space,
				    sa->sa_reg[i].oa_base),
			   sizeof(struct timer_4m),
			   BUS_SPACE_MAP_LINEAR,
			   TIMERREG_VA, &bh) != 0) {
		printf(": can't map registers\n");
		return;
	}
	timerreg4m = (struct timer_4m *)TIMERREG_VA;

	/* Map each CPU's counter */
	for (i = 0; i < sa->sa_nreg - 1; i++) {
		/*
		 * Check whether the CPU corresponding to this timer
		 * register is installed.
		 */
		for (CPU_INFO_FOREACH(n, cpi)) {
			if ((i == 0 && sparc_ncpus == 1) || cpi->mid == i + 8) {
				/* We got a corresponding MID. */
				break;
			}
			cpi = NULL;
		}
		if (cpi == NULL)
			continue;

		if (sbus_bus_map(sa->sa_bustag,
				 sa->sa_reg[i].oa_space,
				 sa->sa_reg[i].oa_base,
				 sizeof(struct timer_4m),
				 BUS_SPACE_MAP_LINEAR,
				 &bh) != 0) {
			printf(": can't map CPU counter %d\n", i);
			return;
		}
		cpi->counterreg_4m = (struct counter_4m *)bh;
	}

	/* Install timer/statclock event counters, per cpu */
	for (CPU_INFO_FOREACH(n, cpi)) {
		evcnt_attach_dynamic(&cpi->ci_lev10, EVCNT_TYPE_INTR, NULL, cpu_name(cpi), "lev10");
		evcnt_attach_dynamic(&cpi->ci_lev14, EVCNT_TYPE_INTR, NULL, cpu_name(cpi), "lev14");
	}

	/* Put processor counter in "timer" mode */
	timerreg4m->t_cfg = 0;

	timerattach(&timerreg4m->t_counter, &timerreg4m->t_limit);
}
