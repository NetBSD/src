/*	$NetBSD: oclock.c,v 1.7 2003/02/26 17:39:07 pk Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * sun4 intersil time-of-day clock driver. This chip also provides
 * the system timer.
 *
 * Only 4/100's and 4/200's have this old clock device.
 */
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/promlib.h>
#include <machine/autoconf.h>

#include <dev/clock_subr.h>
#include <dev/ic/intersil7170.h>

/* Imported from clock.c: */
extern todr_chip_handle_t todr_handle;
extern int oldclk;
extern int timerblurb;
extern void (*timer_init)(void);


static int oclockmatch(struct device *, struct cfdata *, void *);
static void oclockattach(struct device *, struct device *, void *);

CFATTACH_DECL(oclock, sizeof(struct device),
    oclockmatch, oclockattach, NULL, NULL);

#if defined(SUN4)
static bus_space_tag_t i7_bt;
static bus_space_handle_t i7_bh;

#define intersil_command(run, interrupt) \
    (run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
     INTERSIL_CMD_NORMAL_MODE)

#define intersil_disable() \
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD, \
		  intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE));

#define intersil_enable() \
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD, \
		  intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE));

#define intersil_clear() bus_space_read_1(i7_bt, i7_bh, INTERSIL_IINTR)

int oclockintr(void *);
static struct intrhand level10 = { oclockintr };
void oclock_init(void);
#endif /* SUN4 */

/*
 * old clock match routine
 */
static int
oclockmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	/* Only these sun4s have oclock */
	if (!CPU_ISSUN4 ||
	    (cpuinfo.cpu_type != CPUTYP_4_100 &&
	     cpuinfo.cpu_type != CPUTYP_4_200))
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/* ARGSUSED */
static void
oclockattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4)
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_tag_t bt = oba->oba_bustag;
	bus_space_handle_t bh;

	oldclk = 1;  /* we've got an oldie! */

	if (bus_space_map(bt,
			  oba->oba_paddr,
			  sizeof(struct intersil7170),
			  BUS_SPACE_MAP_LINEAR,	/* flags */
			  &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}
	i7_bt = bt;
	i7_bh = bh;

	/* 
	 * calibrate delay() 
	 */
	ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
	for (timerblurb = 1; ; timerblurb++) {
		int ival;

		/* Set to 1/100 second interval */
		bus_space_write_1(bt, bh, INTERSIL_IINTR,
				  INTERSIL_INTER_CSECONDS);

		/* enable clock */
		intersil_enable();

		while ((intersil_clear() & INTERSIL_INTER_PENDING) == 0)
			/* sync with interrupt */;
		while ((intersil_clear() & INTERSIL_INTER_PENDING) == 0)
			/* XXX: do it again, seems to need it */;

		/* Probe 1/100 sec delay */
		delay(10000);

		/* clear, save value */
		ival = intersil_clear();

		/* disable clock */
		intersil_disable();

		if ((ival & INTERSIL_INTER_PENDING) != 0) {
			printf(" delay constant %d%s\n", timerblurb,
				(timerblurb == 1) ? " [TOO SMALL?]" : "");
			break;
		}
		if (timerblurb > 10) {
			printf("\noclock: calibration failing; clamped at %d\n",
			       timerblurb);
			break;
		}
	}

	timer_init = oclock_init;

	/* link interrupt handler */
	intr_establish(10, 0, &level10, NULL);

	/* Our TOD clock year 0 represents 1968 */
	if ((todr_handle = intersil7170_attach(bt, bh, 1968)) == NULL)
		panic("Can't attach tod clock");
	printf("\n");
#endif /* SUN4 */
}

#if defined(SUN4)
/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
oclock_init()
{
	int dummy;

	profhz = hz = 100;
	tick = 1000000 / hz;

	/* Select 1/100 second interval */
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_IINTR,
			  INTERSIL_INTER_CSECONDS);

	ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
	intersil_disable();		/* disable clock */
	dummy = intersil_clear();	/* clear interrupts */
	ienab_bis(IE_L10);		/* enable l10 interrupt */
	intersil_enable();		/* enable clock */
}

/*
 * Level 10 (clock) interrupts from system counter.
 * If we are using the FORTH PROM for console input, we need to check
 * for that here as well, and generate a software interrupt to read it.
 */
int
oclockintr(cap)
	void *cap;
{
	volatile int discard;

	discard = intersil_clear();
	ienab_bic(IE_L10);  /* clear interrupt */
	ienab_bis(IE_L10);  /* enable interrupt */

	hardclock((struct clockframe *)cap);
	return (1);
}
#endif /* SUN4 */
