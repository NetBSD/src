/*	$NetBSD: timer.c,v 1.2 2002/03/28 19:50:21 uwe Exp $ */

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

/*
 * Kernel clocks provided by "timer" device.  The
 * hardclock is provided by the timer register (aka system counter).
 * The statclock is provided by per cpu counter register(s) (aka
 * processor counter(s)).
 *
 */
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/idprom.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/timerreg.h>

#if defined(MSIIEP)
#include <sparc/sparc/msiiepreg.h>
#include <sparc/sparc/msiiepvar.h>
#endif

static struct intrhand level10;
static struct intrhand level14;

#if defined(SUN4) || defined(SUN4C)
static int	clockintr_4(void *);
static int	statintr_4(void *);
static void	timer_init_4(void);
#endif

#if defined(SUN4M)
static int	clockintr_4m(void *);
static int	statintr_4m(void *);
static void	timer_init_4m(void);
#endif

#if defined(MSIIEP)
static int	clockintr_msiiep(void *);
static int	statintr_msiiep(void *);
static void	timer_init_msiiep(void);
#endif

static int	timermatch_mainbus(struct device *, struct cfdata *, void *);
static int	timermatch_obio(struct device *, struct cfdata *, void *);
static int	timermatch_msiiep(struct device *, struct cfdata *, void *);
static void	timerattach_mainbus(struct device *, struct device *, void *);
static void	timerattach_obio(struct device *, struct device *, void *);
static void	timerattach_msiiep(struct device *, struct device *, void *);

static void	timerattach(volatile int *, volatile int *);

static int timerok;

/* Imported from clock.c: */
extern int statvar, statmin, statint;
extern int timerblurb;
extern void (*timer_init)(void);


#if defined(SUN4) || defined(SUN4C)
#define	timerreg4	((struct timerreg_4 *)TIMERREG_VA)
#endif

#if defined(SUN4M)
struct timer_4m		*timerreg4m;
#define counterreg4m	cpuinfo.counterreg_4m
#endif

#if defined(MSIIEP)
/* XXX: move this stuff to msiiepreg.h? */

/* ms-IIep PCIC registers mapped at fixed VA (see vaddrs.h) */
#define msiiep ((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)

/* 
 * ms-IIep counters tick every 4 cpu clock @100MHz.
 * counter is reset to 1 when new limit is written.
 */
#define tmr_ustolimIIep(n) ((n) * 25 + 1)
#endif /* MSIIEP */


struct cfattach timer_mainbus_ca = {
	sizeof(struct device), timermatch_mainbus, timerattach_mainbus
};

struct cfattach timer_obio_ca = {
	sizeof(struct device), timermatch_obio, timerattach_obio
};

struct cfattach timer_msiiep_ca = {
	sizeof(struct device), timermatch_msiiep, timerattach_msiiep
};

/*
 * The sun4c OPENPROM calls the timer the "counter-timer".
 */
static int
timermatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("counter-timer", ma->ma_name) == 0);
}

/*
 * The sun4m OPENPROM calls the timer the "counter".
 * The sun4 timer must be probed.
 */
static int
timermatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("counter", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("timermatch_obio: attach args mixed up\n");
		return (0);
	}

	/* Only these sun4s have "timer" (others have "oclock") */
	if (cpuinfo.cpu_type != CPUTYP_4_300 &&
	    cpuinfo.cpu_type != CPUTYP_4_400)
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

static int
timermatch_msiiep(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct msiiep_attach_args *msa = aux;

	return (strcmp(msa->msa_name, "timer") == 0);
}

/* ARGSUSED */
static void
timerattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4) || defined(SUN4C)
	struct mainbus_attach_args *ma = aux;
	bus_space_handle_t bh;

	/*
	 * This time, we ignore any existing virtual address because
	 * we have a fixed virtual address for the timer, to make
	 * microtime() faster.
	 */
	if (bus_space_map2(ma->ma_bustag,
			   ma->ma_paddr,
			   sizeof(struct timerreg_4),
			   BUS_SPACE_MAP_LINEAR,
			   TIMERREG_VA, &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	timerattach(&timerreg4->t_c14.t_counter, &timerreg4->t_c14.t_limit);
#endif /* SUN4/SUN4C */
}

static void
timerattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0) {
		/* sun4m timer at obio */
#if defined(SUN4M)
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		bus_space_handle_t bh;
		int i;

		if (sa->sa_nreg < 2) {
			printf("%s: only %d register sets\n", self->dv_xname,
				sa->sa_nreg);
			return;
		}

		/* Map the system timer */
		i = sa->sa_nreg - 1;
		if (bus_space_map2(sa->sa_bustag,
				 BUS_ADDR(
					sa->sa_reg[i].sbr_slot,
					sa->sa_reg[i].sbr_offset),
				 sizeof(struct timer_4m),
				 BUS_SPACE_MAP_LINEAR,
				 TIMERREG_VA, &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
		timerreg4m = (struct timer_4m *)TIMERREG_VA;

		/* Map each CPU's counter */
		for (i = 0; i < sa->sa_nreg - 1; i++) {
			struct cpu_info *cpi = NULL;
			int n;

			/*
			 * Check whether the CPU corresponding to this
			 * timer register is installed.
			 */
			for (n = 0; n < ncpu; n++) {
				if ((cpi = cpus[n]) == NULL)
					continue;
				if ((i == 0 && ncpu == 1) || cpi->mid == i + 8)
					/* We got a corresponding MID */
					break;
				cpi = NULL;
			}
			if (cpi == NULL)
				continue;
			if (sbus_bus_map(sa->sa_bustag,
					 sa->sa_reg[i].sbr_slot,
					 sa->sa_reg[i].sbr_offset,
					 sizeof(struct timer_4m),
					 BUS_SPACE_MAP_LINEAR,
					 &bh) != 0) {
				printf("%s: can't map register\n",
					self->dv_xname);
				return;
			}
			cpi->counterreg_4m = (struct counter_4m *)bh;
		}

		/* Put processor counter in "timer" mode */
		timerreg4m->t_cfg = 0;

		timerattach(&counterreg4m->t_counter, &counterreg4m->t_limit);
#endif /* SUN4M */
		return;
	} else {
#if defined(SUN4)
		/* sun4 timer at obio */
		struct obio4_attach_args *oba = &uoba->uoba_oba4;
		bus_space_handle_t bh;

		if (bus_space_map2(oba->oba_bustag,
				  oba->oba_paddr,
				  sizeof(struct timerreg_4),
				  BUS_SPACE_MAP_LINEAR,
				  TIMERREG_VA,
				  &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
		timerattach(&timerreg4->t_c14.t_counter,
			    &timerreg4->t_c14.t_limit);
#endif /* SUN4 */
	}
}

/*
 * sun4/sun4c/sun4m common timer attach code
 */
static void
timerattach(cntreg, limreg)
	volatile int *cntreg, *limreg;
{

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us on the clock.
	 * Note: sun4m clocks tick with 500ns periods.
	 */
	for (timerblurb = 1; ; timerblurb++) {
		volatile int discard;
		int t0, t1;

		/* Reset counter register by writing some large limit value */
		discard = *limreg;
		*limreg = tmr_ustolim(TMR_MASK-1);

		t0 = *cntreg;
		delay(100);
		t1 = *cntreg;

		if (t1 & TMR_LIMIT)
			panic("delay calibration");

		t0 = (t0 >> TMR_SHIFT) & TMR_MASK;
		t1 = (t1 >> TMR_SHIFT) & TMR_MASK;

		if (t1 >= t0 + 100)
			break;
	}

	printf(" delay constant %d\n", timerblurb);

#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4OR4C) {
		timer_init = timer_init_4;
		level10.ih_fun = clockintr_4;
		level14.ih_fun = statintr_4;
	}
#endif
#if defined(SUN4M)
	if (CPU_ISSUN4M) {
		timer_init = timer_init_4m;
		level10.ih_fun = clockintr_4m;
		level14.ih_fun = statintr_4m;
	}
#endif
	/* link interrupt handlers */
	intr_establish(10, &level10);
	intr_establish(14, &level14);

	timerok = 1;
}


/* ARGSUSED */
static void
timerattach_msiiep(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(MSIIEP)
	/*
	 * Attach system and cpu counters (kernel hard and stat clocks)
	 * for ms-IIep. Counters are part of the PCIC and there's no
	 * PROM node for them.
	 */
	/* Put processor counter in "counter" mode */
	msiiep->pcic_pc_ctl = 0; /* stop user timer (just in case) */
	msiiep->pcic_pc_cfg = 0; /* timer mode disabled (processor counter) */

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us on the clock.
	 * Note: ms-IIep clocks ticks every 4 processor cycles.
	 */
	for (timerblurb = 1; ; ++timerblurb) {
		volatile int discard;
		int t;

		discard = msiiep->pcic_pclr; /* clear the limit bit */
		msiiep->pcic_pclr = 0; /* reset counter to 1, free run */
		delay(100);
		t = msiiep->pcic_pccr;

		if (t & TMR_LIMIT) /* cannot happen */
			panic("delay calibration");

		/* counter ticks -> usec, inverse of tmr_ustolimIIep */
		t = (t - 1) / 25;
		if (t >= 100)
			break;
	}
	printf(" delay constant %d\n", timerblurb);

	/*
	 * Set counter interrupt priority assignment:
	 * upper 4 bits are for system counter: level 10
	 * lower 4 bits are for processor counter: level 14
	 */
	msiiep->pcic_cipar = 0xae;

	timer_init = timer_init_msiiep;
	level10.ih_fun = clockintr_msiiep;
	level14.ih_fun = statintr_msiiep;

	/* link interrupt handlers */
	intr_establish(10, &level10);
	intr_establish(14, &level14);

	timerok = 1;
#endif /* MSIIEP */
}

/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
#if defined(SUN4) || defined(SUN4C)
void
timer_init_4()
{
	timerreg4->t_c10.t_limit = tmr_ustolim(tick);
	timerreg4->t_c14.t_limit = tmr_ustolim(statint);
	ienab_bis(IE_L14 | IE_L10);
}
#endif

#if defined(SUN4M)
void
timer_init_4m()
{
	int n;
	timerreg4m->t_limit = tmr_ustolim4m(tick);
	for (n = 0; n < ncpu; n++) {
		struct cpu_info *cpi;
		if ((cpi = cpus[n]) == NULL)
			continue;
		cpi->counterreg_4m->t_limit = tmr_ustolim4m(statint);
	}
	ienab_bic(SINTR_T);
}
#endif

#if defined(MSIIEP)
void
timer_init_msiiep()
{
	/* ms-IIep kernels support *only* IIep */
	msiiep->pcic_sclr = tmr_ustolimIIep(tick);
	msiiep->pcic_pclr = tmr_ustolimIIep(statint);
	/* XXX: ensure interrupt target mask doesn't masks them? */
}
#endif /* MSIIEP */

#if defined(SUN4) || defined(SUN4C)
/*
 * Level 10 (clock) interrupts from system counter.
 */
int
clockintr_4(cap)
	void *cap;
{
	volatile int discard;

	/* read the limit register to clear the interrupt */
	discard = timerreg4->t_c10.t_limit;
	hardclock((struct clockframe *)cap);
	return (1);
}
#endif /* SUN4/SUN4C */

#if defined(SUN4M)
int
clockintr_4m(cap)
	void *cap;
{
	volatile int discard;
	int s;

	/*
	 * Protect the clearing of the clock interrupt.  If we don't
	 * do this, and we're interrupted (by the zs, for example),
	 * the clock stops!
	 * XXX WHY DOES THIS HAPPEN?
	 */
	s = splhigh();

	/* read the limit register to clear the interrupt */
	discard = timerreg4m->t_limit;
	splx(s);

	hardclock((struct clockframe *)cap);
	return (1);
}
#endif /* SUN4M */

#if defined(MSIIEP)
int
clockintr_msiiep(cap)
	void *cap;
{
	volatile int discard;

	/* read the limit register to clear the interrupt */
	discard = msiiep->pcic_sclr;
	hardclock((struct clockframe *)cap);
	return (1);
}
#endif /* MSIIEP */

static __inline__ u_long
new_interval(void)
{
	u_long newint, r, var;

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
	return (newint);
}

/*
 * Level 14 (stat clock) interrupts from processor counter.
 */
#if defined(SUN4) || defined(SUN4C)
int
statintr_4(cap)
	void *cap;
{
	volatile int discard;
	u_long newint;

	/* read the limit register to clear the interrupt */
	discard = timerreg4->t_c14.t_limit;

	statclock((struct clockframe *)cap);

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
	return (1);
}
#endif /* SUN4/SUN4C */

#if defined(SUN4M)
int
statintr_4m(cap)
	void *cap;
{
	volatile int discard;
	u_long newint;

	/* read the limit register to clear the interrupt */
	discard = counterreg4m->t_limit;
	if (timerok == 0) {
		/* Stop the clock */
		printf("note: counter running!\n");
		discard = counterreg4m->t_limit;
		counterreg4m->t_limit = 0;
		counterreg4m->t_ss = 0;
		timerreg4m->t_cfg = TMR_CFG_USER;
		return 1;
	}

	statclock((struct clockframe *)cap);

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
	return (1);
}
#endif /* SUN4M */

#if defined(MSIIEP)
int
statintr_msiiep(cap)
	void *cap;
{
	volatile int discard;
	u_long newint;

	/* read the limit register to clear the interrupt */
	discard = msiiep->pcic_pclr;
	if (timerok == 0) {
		/* Stop the clock */
		printf("note: counter running!\n");
		/* 
		 * Turn interrupting processor counter
		 * into non-interrupting user timer.
		 */
		msiiep->pcic_pc_cfg = 1; /* make it a user timer */
		msiiep->pcic_pc_ctl = 0; /* stop user timer */
		return (1);
	}

	statclock((struct clockframe *)cap);

	/*
	 * Compute new randomized interval.
	 */
	newint = new_interval();

	/*
	 * Use the `non-resetting' limit register, so we don't
	 * loose the counter ticks that happened since this
	 * interrupt was raised.
	 */
	msiiep->pcic_pclr_nr = tmr_ustolimIIep(newint);
	return (1);
}
#endif /* MSIIEP */
