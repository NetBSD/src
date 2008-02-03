/* $NetBSD: mcclock.c,v 1.14 2008/02/03 07:31:21 tsutsui Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock.c,v 1.14 2008/02/03 07:31:21 tsutsui Exp $");

#include "opt_clock_compat_osf1.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu_counter.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <alpha/alpha/mcclockvar.h>
#include <alpha/alpha/clockvar.h>

#ifdef CLOCK_COMPAT_OSF1
/*
 * According to OSF/1's /usr/sys/include/arch/alpha/clock.h,
 * the console adjusts the RTC years 13..19 to 93..99 and
 * 20..40 to 00..20. (historical reasons?)
 * DEC Unix uses an offset to the year to stay outside
 * the dangerous area for the next couple of years.
 */
#define UNIX_YEAR_OFFSET 52 /* 41=>1993, 12=>2064 */
#else
#define UNIX_YEAR_OFFSET 0
#endif


static void mcclock_set_pcc_freq(struct mc146818_softc *);
static void mcclock_init(void *);

void
mcclock_attach(struct mc146818_softc *sc)
{

	sc->sc_year0 = 1900 + UNIX_YEAR_OFFSET;
	sc->sc_flag = 0;	/* BINARY, 24HR */

	mc146818_attach(sc);

	aprint_normal("\n");

	/* Turn interrupts off, just in case. */
	(*sc->sc_mcwrite)(sc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

	mcclock_set_pcc_freq(sc);

	clockattach(mcclock_init, (void *)sc);
}

#define NLOOP	4

static void
mcclock_set_pcc_freq(struct mc146818_softc *sc)
{
	struct cpu_info *ci;
	uint64_t freq;
	uint32_t ctrdiff[NLOOP], pcc_start, pcc_end;
	uint8_t reg_a;
	int i;

	/* save REG_A */
	reg_a = (*sc->sc_mcread)(sc, MC_REGA);

	/* set interval 16Hz to measure pcc */
	(*sc->sc_mcwrite)(sc, MC_REGA, MC_BASE_32_KHz | MC_RATE_16_Hz);

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < NLOOP; i++) {
		/* clear interrupt flags */
		(void)(*sc->sc_mcread)(sc, MC_REGC);

		/* wait till the periodic interupt flag is set */
		while (((*sc->sc_mcread)(sc, MC_REGC) & MC_REGC_PF) == 0)
			;
		pcc_start = cpu_counter32();

		/* wait till the periodic interupt flag is set again */
		while (((*sc->sc_mcread)(sc, MC_REGC) & MC_REGC_PF) == 0)
			;
		pcc_end = cpu_counter32();

		ctrdiff[i] = pcc_end - pcc_start;
	}

	freq = ((ctrdiff[NLOOP - 2] + ctrdiff[NLOOP - 1]) / 2) * 16 /* Hz */;

	/* restore REG_A */
	(*sc->sc_mcwrite)(sc, MC_REGA, reg_a);

	/* XXX assume all processors have the same clock and frequency */
	for (ci = &cpu_info_primary; ci; ci = ci->ci_next)
		ci->ci_pcc_freq = freq;
}

static void
mcclock_init(void *dev)
{
	struct mc146818_softc *sc = dev;

	/* enable interval clock interrupt */
	(*sc->sc_mcwrite)(sc, MC_REGA, MC_BASE_32_KHz | MC_RATE_1024_Hz);
	(*sc->sc_mcwrite)(sc, MC_REGB,
	    MC_REGB_PIE | MC_REGB_SQWE | MC_REGB_BINARY | MC_REGB_24HR);
}
