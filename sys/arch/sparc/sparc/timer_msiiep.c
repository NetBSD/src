/*	$NetBSD: timer_msiiep.c,v 1.26.2.1 2012/10/30 17:20:22 yamt Exp $	*/

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
 * MicroSPARC-IIep timer support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer_msiiep.c,v 1.26.2.1 2012/10/30 17:20:22 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>

#include <sparc/sparc/timervar.h>


static int	timermatch_msiiep(device_t, cfdata_t, void *);
static void	timerattach_msiiep(device_t, device_t, void *);

CFATTACH_DECL_NEW(timer_msiiep, 0,
    timermatch_msiiep, timerattach_msiiep, NULL, NULL);


static void	timer_init_msiiep(void);
static int	clockintr_msiiep(void *);
static int	statintr_msiiep(void *);
static u_int	timer_get_timecount(struct timecounter *);

void*	sched_cookie;

static struct intrhand level10 = { .ih_fun = clockintr_msiiep };
static struct intrhand level14 = { .ih_fun = statintr_msiiep  };

/*
 * timecounter local state
 */
static struct counter {
	u_int limit;		/* limit we count up to */
	u_int offset;		/* accumulated offet due to wraps */
} cntr;

/*
 * define timecounter
 */
static struct timecounter counter_timecounter = {
	.tc_get_timecount	= timer_get_timecount,
	.tc_poll_pps		= NULL,
	.tc_counter_mask	= ~0u,
	.tc_frequency		= 25000000, /* Hz */
	.tc_name		= "timer-counter",
	.tc_quality		= 100,
	.tc_priv		= &cntr,
};


/*
 * ms-IIep counters tick every 4 CPU clocks @100MHz.
 * counter is reset to 1 when new limit is written.
 */
#define	tmr_ustolimIIep(n)	((n) * 25 + 1)


static int
timermatch_msiiep(device_t parent, cfdata_t cf, void *aux)
{
	struct msiiep_attach_args *msa = aux;

	return (strcmp(msa->msa_name, "timer") == 0);
}


/*
 * Attach system and processor counters (kernel hard and stat clocks)
 * for ms-IIep.  Counters are part of the PCIC and there's no PROM
 * node for them.
 */
static void
timerattach_msiiep(device_t parent, device_t self, void *aux)
{

	/* Put processor counter in "counter" mode */
	mspcic_write_1(pcic_pc_ctl, 0); /* stop user timer (just in case) */
	mspcic_write_1(pcic_pc_cfg, 0); /* timer mode disabled */

	/*
	 * Calibrate delay() by tweaking the magic constant until
	 * delay(100) actually reads (at least) 100 us on the clock.
	 */
	for (timerblurb = 1; ; ++timerblurb) {
		int t;
		volatile uint32_t junk;

		/* we need 'junk' to keep the read from getting eliminated */
		junk = mspcic_read_4(pcic_pclr); /* clear the limit bit */
		mspcic_write_4(pcic_pclr, 0); /* reset to 1, free run */
		delay(100);
		t = mspcic_read_4(pcic_pccr);
		if (t < 0)	/* limit bit set, cannot happen */
			panic("delay calibration");

		if (t >= 2501) /* (t - 1) / 25 >= 100 us */
			break;
	}

	printf(": delay constant %d\n", timerblurb);

	timer_init = timer_init_msiiep;

	/*
	 * Set counter interrupt priority assignment:
	 * upper 4 bits are for system counter: level 10
	 * lower 4 bits are for processor counter: level 14
	 *
	 * XXX: ensure that interrupt target mask doesn't mask them?
	 */
	mspcic_write_1(pcic_cipar, 0xae);

	/* link interrupt handlers */
	intr_establish(10, 0, &level10, NULL, false);
	intr_establish(14, 0, &level14, NULL, false);

	/* Establish a soft interrupt at a lower level for schedclock */
	sched_cookie = sparc_softintr_establish(IPL_SCHED, schedintr, NULL);
	if (sched_cookie == NULL)
		panic("timerattach: cannot establish schedintr");
}


/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
static void
timer_init_msiiep(void)
{

	mspcic_write_4(pcic_sclr, tmr_ustolimIIep(tick));
	mspcic_write_4(pcic_pclr, tmr_ustolimIIep(statint));

	cntr.limit = tmr_ustolimIIep(tick);
	tc_init(&counter_timecounter);
}


/*
 * timer_get_timecount provide current counter value
 */
static u_int
timer_get_timecount(struct timecounter *tc)
{
	struct counter *ctr = (struct counter *)tc->tc_priv;
	u_int c, res;
	int s;

	s = splhigh();

	/* XXX: consider re-reading until increment is detected */
	c = mspcic_read_4(pcic_sccr);

	res = c & ~MSIIEP_COUNTER_LIMIT;
	if (res != c)
		res += ctr->limit;

	res += ctr->offset;

	splx(s);

	return res;
}


/*
 * Level 10 (clock) interrupts from system counter.
 */
static int
clockintr_msiiep(void *cap)
{
	volatile uint32_t junk;

	junk = mspcic_read_4(pcic_sclr); /* clear the interrupt */

	/*
	 * XXX this needs to be fixed in a more general way
	 * problem is that the kernel enables interrupts and THEN
	 * sets up clocks. In between there's an opportunity to catch
	 * a timer interrupt - if we call hardclock() at that point we'll
	 * panic
	 * so for now just bail when cold
	 */
	if (__predict_false(cold))
		return 0;

	if (timecounter->tc_get_timecount == timer_get_timecount)
		cntr.offset += cntr.limit;

	hardclock((struct clockframe *)cap);
	return 1;
}


/*
 * Level 14 (stat clock) interrupts from processor counter.
 */
static int
statintr_msiiep(void *cap)
{
	struct clockframe *frame = cap;
	u_long newint;
	volatile uint32_t junk;

	junk = mspcic_read_4(pcic_pclr); /* clear the interrupt */

	statclock(frame);

	/*
	 * Compute new randomized interval.
	 */
	newint = new_interval();

	/*
	 * Use the `non-resetting' limit register, so that we don't
	 * lose the counter ticks that happened since this interrupt
	 * has been raised.
	 */
	mspcic_write_4(pcic_pclr_nr, tmr_ustolimIIep(newint));

	/*
	 * The factor 8 is only valid for stathz==100.
	 * See also clock.c
	 */
	if ((++cpuinfo.ci_schedstate.spc_schedticks & 7) == 0) {
		if (CLKF_LOPRI(frame, IPL_SCHED)) {
			/* No need to schedule a soft interrupt */
			spllowerschedclock();
			schedintr(cap);
		} else {
			/*
			 * We're interrupting a thread that may have the
			 * scheduler lock; run schedintr() later.
			 */
			sparc_softintr_schedule(sched_cookie);
		}
	}

	return 1;
}
