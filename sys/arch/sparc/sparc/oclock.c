/*	$NetBSD: oclock.c,v 1.20.66.1 2021/04/03 22:28:38 thorpej Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: oclock.c,v 1.20.66.1 2021/04/03 22:28:38 thorpej Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <machine/promlib.h>
#include <machine/autoconf.h>

#include <sparc/sparc/timervar.h>

#include <dev/clock_subr.h>
#include <dev/ic/intersil7170reg.h>
#include <dev/ic/intersil7170var.h>

static int oclockmatch(device_t, cfdata_t, void *);
static void oclockattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(oclock, sizeof(struct intersil7170_softc),
    oclockmatch, oclockattach, NULL, NULL);

#if defined(SUN4)
static bus_space_tag_t i7_bt;
static bus_space_handle_t i7_bh;

#define intersil_disable()						\
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD,			\
	    INTERSIL_COMMAND(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE));

#define intersil_enable()						\
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_ICMD,			\
	    INTERSIL_COMMAND(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE));

#define intersil_clear() bus_space_read_1(i7_bt, i7_bh, INTERSIL_IINTR)

int oclockintr(void *);
static struct intrhand level10 = { oclockintr };
void oclock_init(void);
#endif /* SUN4 */

/*
 * old clock match routine
 */
static int
oclockmatch(device_t parent, cfdata_t cf, void *aux)
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
oclockattach(device_t parent, device_t self, void *aux)
{
#if defined(SUN4)
	struct intersil7170_softc *sc = device_private(self);
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;

	oldclk = 1;  /* we've got an oldie! */

	sc->sc_dev = self;
	sc->sc_bst = oba->oba_bustag;
	if (bus_space_map(sc->sc_bst,
			  oba->oba_paddr,
			  sizeof(struct intersil7170),
			  BUS_SPACE_MAP_LINEAR,	/* flags */
			  &sc->sc_bsh) != 0) {
		aprint_error(": can't map register\n");
		return;
	}
	i7_bt = sc->sc_bst;
	i7_bh = sc->sc_bsh;

	/*
	 * calibrate delay()
	 */
	ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
	for (timerblurb = 1; ; timerblurb++) {
		int ival;

		/* Set to 1/100 second interval */
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, INTERSIL_IINTR,
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
			aprint_normal(" delay constant %d%s\n", timerblurb,
				(timerblurb == 1) ? " [TOO SMALL?]" : "");
			break;
		}
		if (timerblurb > 10) {
			aprint_normal("\n");
			aprint_error_dev(self, "calibration failing; "
			    "clamped at %d\n", timerblurb);
			break;
		}
	}

	timer_init = oclock_init;

	/* link interrupt handler */
	intr_establish(10, 0, &level10, NULL, false);

	/* Our TOD clock year 0 represents 1968 */
	sc->sc_year0 = 1968;
	intersil7170_attach(sc);

	aprint_normal("\n");
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
oclock_init(void)
{

	profhz = hz = 100;
	tick = 1000000 / hz;

	/* Select 1/100 second interval */
	bus_space_write_1(i7_bt, i7_bh, INTERSIL_IINTR,
			  INTERSIL_INTER_CSECONDS);

	ienab_bic(IE_L14 | IE_L10);	/* disable all clock intrs */
	intersil_disable();		/* disable clock */
	(void)intersil_clear();		/* clear interrupts */
	ienab_bis(IE_L10);		/* enable l10 interrupt */
	intersil_enable();		/* enable clock */
}

/*
 * Level 10 (clock) interrupts from system counter.
 * If we are using the FORTH PROM for console input, we need to check
 * for that here as well, and generate a software interrupt to read it.
 */
int
oclockintr(void *cap)
{
	int s;

	/*
	 * Protect the clearing of the clock interrupt.  If we don't
	 * do this, and we're interrupted (by the zs, for example),
	 * the clock stops!
	 * XXX WHY DOES THIS HAPPEN?
	 */
	s = splhigh();

	(void)intersil_clear();
	ienab_bic(IE_L10);  /* clear interrupt */
	ienab_bis(IE_L10);  /* enable interrupt */
	splx(s);

	hardclock((struct clockframe *)cap);
	return (1);
}
#endif /* SUN4 */
