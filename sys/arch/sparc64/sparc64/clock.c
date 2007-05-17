/*	$NetBSD: clock.c,v 1.85 2007/05/17 14:51:31 yamt Exp $ */

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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.85 2007/05/17 14:51:31 yamt Exp $");

#include "opt_multiprocessor.h"

/*
 * Clock driver.  This is the id prom and eeprom driver as well
 * and includes the timer register functions too.
 */

/* Define this for a 1/4s clock to ease debugging */
/* #define INTR_DEBUG */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/cpu.h>
#include <machine/cpu_counter.h>

#include <sparc64/sparc64/intreg.h>
#include <sparc64/sparc64/timerreg.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>
#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>


/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int statvar = 8192;
int statmin;			/* statclock interval - 1/2*variance */
int timerok;

static long tick_increment;
int schedintr(void *);

static struct intrhand level10 = { .ih_fun = clockintr };
static struct intrhand level0 = { .ih_fun = tickintr };
static struct intrhand level14 = { .ih_fun = statintr };
static struct intrhand schedint = { .ih_fun = schedintr };

static int	timermatch(struct device *, struct cfdata *, void *);
static void	timerattach(struct device *, struct device *, void *);

struct timerreg_4u	timerreg_4u;	/* XXX - need more cleanup */

CFATTACH_DECL(timer, sizeof(struct device),
    timermatch, timerattach, NULL, NULL);

struct chiptime;
void stopcounter(struct timer_4u *);

int timerblurb = 10; /* Guess a value; used before clock is attached */

static u_int timer_get_timecount(struct timecounter *);

/*
 * define timecounter
 */

static struct timecounter counter_timecounter = {
	timer_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,                      /* frequency - set at initialisation */
	"tick-counter",		/* name */
	100,			/* quality */
	0,			/* private reference - UNUSED */
	NULL			/* next timecounter */
};

/*
 * timer_get_timecount provide current counter value
 */
static u_int
timer_get_timecount(struct timecounter *tc)
{
	return cpu_counter();
}

/*
 * The sun4u OPENPROMs call the timer the "counter-timer", except for
 * the lame UltraSPARC IIi PCI machines that don't have them.
 */
static int
timermatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("counter-timer", ma->ma_name) == 0);
}

static void
timerattach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	u_int *va = ma->ma_address;
#if 0
	volatile int64_t *cnt = NULL, *lim = NULL;
#endif
	
	/*
	 * What we should have are 3 sets of registers that reside on
	 * different parts of SYSIO or PSYCHO.  We'll use the prom
	 * mappings cause we can't get rid of them and set up appropriate
	 * pointers on the timerreg_4u structure.
	 */
	timerreg_4u.t_timer = (struct timer_4u *)(u_long)va[0];
	timerreg_4u.t_clrintr = (int64_t *)(u_long)va[1];
	timerreg_4u.t_mapintr = (int64_t *)(u_long)va[2];

	/* Install the appropriate interrupt vector here */
	level10.ih_number = ma->ma_interrupts[0];
	level10.ih_clr = &timerreg_4u.t_clrintr[0];
	intr_establish(10, &level10);
	level14.ih_number = ma->ma_interrupts[1];
	level14.ih_clr = &timerreg_4u.t_clrintr[1];

	intr_establish(14, &level14);
	printf(" irq vectors %lx and %lx", 
	       (u_long)level10.ih_number, 
	       (u_long)level14.ih_number);

#if 0
	cnt = &(timerreg_4u.t_timer[0].t_count);
	lim = &(timerreg_4u.t_timer[0].t_limit);

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us 
	 * on the clock.  Since we're using the %tick register 
	 * which should be running at exactly the CPU clock rate, it
	 * has a period of somewhere between 7ns and 3ns.
	 */

#ifdef DEBUG
	printf("Delay calibrarion....\n");
#endif
	for (timerblurb = 1; timerblurb > 0; timerblurb++) {
		volatile int discard;
		register int t0, t1;

		/* Reset counter register by writing some large limit value */
		discard = *lim;
		*lim = tmr_ustolim(TMR_MASK-1);

		t0 = *cnt;
		delay(100);
		t1 = *cnt;

		if (t1 & TMR_LIMIT)
			panic("delay calibration");

		t0 = (t0 >> TMR_SHIFT) & TMR_MASK;
		t1 = (t1 >> TMR_SHIFT) & TMR_MASK;

		if (t1 >= t0 + 100)
			break;
	}

	printf(" delay constant %d\n", timerblurb);
#endif
	printf("\n");
	timerok = 1;
}

void
stopcounter(struct timer_4u *creg)
{
	/* Stop the clock */
	volatile int discard;
	discard = creg->t_limit;
	creg->t_limit = 0;
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only if
 * no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks()
{
	int statint, minint;
	uint64_t start_time = 0;
#ifdef DEBUG
	extern int intrdebug;
#endif

#ifdef DEBUG
	/* Set a 1s clock */
	if (intrdebug) {
		hz = 1;
		tick = 1000000 / hz;
		printf("intrdebug set: 1Hz clock\n");
	}
#endif

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}

	/* Make sure we have a sane cpu_clockrate -- we'll need it */
	if (!cpu_clockrate[0]) {
		/* Default to 200MHz clock XXXXX */
		cpu_clockrate[0] = 200000000;
		cpu_clockrate[1] = 200000000 / 1000000;
	}
	
	/* Initialize the %tick register */
#ifdef __arch64__
	__asm volatile("wrpr %0, 0, %%tick" : : "r" (start_time));
#else
	{
		int start_hi = (start_time>>32), start_lo = start_time;
		__asm volatile("sllx %1,32,%0; or %0,%2,%0; wrpr %0, 0, %%tick" 
				 : "=&r" (start_hi) /* scratch register */
				 : "r" ((int)(start_hi)), "r" ((int)(start_lo)));
	}
#endif

	counter_timecounter.tc_frequency = cpu_clockrate[0];
	tc_init(&counter_timecounter);

	/*
	 * Now handle machines w/o counter-timers.
	 */

	if (!timerreg_4u.t_timer || !timerreg_4u.t_clrintr) {

		printf("No counter-timer -- using %%tick at %ldMHz as system clock.\n",
			(long)cpu_clockrate[1]);
		/* We don't have a counter-timer -- use %tick */
		level0.ih_clr = 0;
		/* 
		 * Establish a level 10 interrupt handler 
		 *
		 * We will have a conflict with the softint handler,
		 * so we set the ih_number to 1.
		 */
		level0.ih_number = 1;
		intr_establish(10, &level0);
		/* We only have one timer so we have no statclock */
		stathz = 0;	

		/* set the next interrupt time */
		tick_increment = cpu_clockrate[0] / hz;
#ifdef DEBUG
		printf("Using %%tick -- intr in %ld cycles...", tick_increment);
#endif
		next_tick(tick_increment);
#ifdef DEBUG
		printf("done.\n");
#endif
		return;
	}

	if (stathz == 0)
		stathz = hz;
	if (1000000 % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}

	profhz = stathz;		/* always */

	statint = 1000000 / stathz;
	minint = statint / 2 + 100;
	while (statvar > minint)
		statvar >>= 1;

	/* 
	 * Establish scheduler softint.
	 */
	schedint.ih_pil = PIL_SCHED;
	schedint.ih_clr = NULL;
	schedint.ih_arg = 0;
	schedint.ih_pending = 0;
	schedhz = stathz/4;

	/* 
	 * Enable timers 
	 *
	 * Also need to map the interrupts cause we're not a child of the sbus.
	 * N.B. By default timer[0] is disabled and timer[1] is enabled.
	 */
	stxa((vaddr_t)&timerreg_4u.t_timer[0].t_limit, ASI_NUCLEUS,
	     tmr_ustolim(tick)|TMR_LIM_IEN|TMR_LIM_PERIODIC|TMR_LIM_RELOAD); 
	stxa((vaddr_t)&timerreg_4u.t_mapintr[0], ASI_NUCLEUS, 
	     timerreg_4u.t_mapintr[0]|INTMAP_V|(CPU_UPAID << INTMAP_TID_SHIFT)); 

#ifdef DEBUG
	if (intrdebug)
		/* Neglect to enable timer */
		stxa((vaddr_t)&timerreg_4u.t_timer[1].t_limit, ASI_NUCLEUS, 
		     tmr_ustolim(statint)|TMR_LIM_RELOAD); 
	else
#endif
		stxa((vaddr_t)&timerreg_4u.t_timer[1].t_limit, ASI_NUCLEUS, 
		     tmr_ustolim(statint)|TMR_LIM_IEN|TMR_LIM_RELOAD); 
	stxa((vaddr_t)&timerreg_4u.t_mapintr[1], ASI_NUCLEUS, 
	     timerreg_4u.t_mapintr[1]|INTMAP_V|(CPU_UPAID << INTMAP_TID_SHIFT));

	statmin = statint - (statvar >> 1);
}

/*
 * Dummy setstatclockrate(), since we know profhz==hz.
 */
/* ARGSUSED */
void
setstatclockrate(newhz)
	int newhz;
{
	/* nothing */
}

/*
 * Level 10 (clock) interrupts.  If we are using the FORTH PROM for
 * console input, we need to check for that here as well, and generate
 * a software interrupt to read it.
 */
#ifdef	DEBUG
static int clockcheck = 0;
#endif
int
clockintr(void *cap)
{
#ifdef DEBUG
	static int64_t tick_base = 0;
	struct timeval ctime;
	int64_t t = (uint64_t)tick();

	microtime(&ctime);
	if (!tick_base) {
		tick_base = (ctime.tv_sec * 1000000LL + ctime.tv_usec) 
			/ cpu_clockrate[1];
		tick_base -= t;
	} else if (clockcheck) {
		int64_t tk = t;
		int64_t clk = (ctime.tv_sec * 1000000LL + ctime.tv_usec);
		t -= tick_base;
		t = t / cpu_clockrate[1];
		if (t - clk > hz) {
			printf("Clock lost an interrupt!\n");
			printf("Actual: %llx Expected: %llx tick %llx tick_base %llx\n",
			       (long long)t, (long long)clk, (long long)tk, (long long)tick_base);
			tick_base = 0;
		}
	}	
#endif
	/* Let locore.s clear the interrupt for us. */
	hardclock((struct clockframe *)cap);
	return (1);
}

int poll_console = 0;

/*
 * Level 10 (clock) interrupts.  If we are using the FORTH PROM for
 * console input, we need to check for that here as well, and generate
 * a software interrupt to read it.
 *
 * %tick is really a level-14 interrupt.  We need to remap this in 
 * locore.s to a level 10.
 */
int
tickintr(void *cap)
{
	int s;

#if	NKBD	> 0
	extern int cnrom(void);
	extern int rom_console_input;
#endif

	hardclock((struct clockframe *)cap);
	if (poll_console)
		setsoftint();

	s = splhigh();
	/* Reset the interrupt */
	next_tick(tick_increment);
	splx(s);

	return (1);
}

/*
 * Level 14 (stat clock) interrupt handler.
 */
int
statintr(cap)
	void *cap;
{
	register u_long newint, r, var;
	struct cpu_info *ci = curcpu();

#ifdef NOT_DEBUG
	printf("statclock: count %x:%x, limit %x:%x\n", 
	       timerreg_4u.t_timer[1].t_count, timerreg_4u.t_timer[1].t_limit);
#endif
#ifdef NOT_DEBUG
	prom_printf("!");
#endif
	statclock((struct clockframe *)cap);
#ifdef NOTDEF_DEBUG
	/* Don't re-schedule the IRQ */
	return 1;
#endif
	/*
	 * Compute new randomized interval.  The intervals are uniformly
	 * distributed on [statint - statvar / 2, statint + statvar / 2],
	 * and therefore have mean statint, giving a stathz frequency clock.
	 */
	var = statvar;
	do {
		r = random() & (var - 1);
	} while (r == 0);
	newint = statmin + r;

	if (schedhz)
		if ((++ci->ci_schedstate.spc_schedticks & 3) == 0)
			send_softint(-1, PIL_SCHED, &schedint);
	stxa((vaddr_t)&timerreg_4u.t_timer[1].t_limit, ASI_NUCLEUS, 
	     tmr_ustolim(newint)|TMR_LIM_IEN|TMR_LIM_RELOAD);
	return (1);
}

int
schedintr(void *arg)
{

	schedclock(curlwp);
	return (1);
}
