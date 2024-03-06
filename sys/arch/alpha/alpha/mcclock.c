/* $NetBSD: mcclock.c,v 1.23 2024/03/06 07:34:11 thorpej Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

__KERNEL_RCSID(0, "$NetBSD: mcclock.c,v 1.23 2024/03/06 07:34:11 thorpej Exp $");

#include "opt_clock_compat_osf1.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#if defined(MULTIPROCESSOR)
#include <sys/xcall.h>
#endif

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

#if defined(MULTIPROCESSOR)
struct mcclock_trampoline_arg {
	todr_chip_handle_t	handle;	/* IN */
	struct clock_ymdhms	*dt;	/* IN */
	int			rv;	/* OUT */
};

static void
mcclock_trampoline(void *arg1, void *arg2)
{
	int (*func)(todr_chip_handle_t, struct clock_ymdhms *) = arg1;
	struct mcclock_trampoline_arg *arg = arg2;

	arg->rv = (*func)(arg->handle, arg->dt);
}

static int
mcclock_bounce(int (*func)(todr_chip_handle_t, struct clock_ymdhms *),
    struct mcclock_trampoline_arg *arg)
{
	/*
	 * If we're not on the primary CPU, then we need to make
	 * a cross-call to the primary to access the clock registers.
	 * But we do a little work to avoid even calling into the
	 * cross-call code if we can avoid it.
	 */
	int bound = curlwp_bind();

	if (CPU_IS_PRIMARY(curcpu())) {
		mcclock_trampoline(func, arg);
		curlwp_bindx(bound);
	} else {
		curlwp_bindx(bound);
		uint64_t token = xc_unicast(0, mcclock_trampoline,
		    func, arg, &cpu_info_primary);
		xc_wait(token);
	}
	return arg->rv;
}

static int
mcclock_gettime_ymdhms(todr_chip_handle_t handle, struct clock_ymdhms *dt)
{
	struct mcclock_trampoline_arg arg = {
		.handle = handle,
		.dt = dt,
	};

	return mcclock_bounce(mc146818_gettime_ymdhms, &arg);
}

static int
mcclock_settime_ymdhms(todr_chip_handle_t handle, struct clock_ymdhms *dt)
{
	struct mcclock_trampoline_arg arg = {
		.handle = handle,
		.dt = dt,
	};

	return mcclock_bounce(mc146818_settime_ymdhms, &arg);
}
#endif /* MULTIPROCESSOR */

void
mcclock_attach(struct mcclock_softc *msc)
{
	struct mc146818_softc *sc = &msc->sc_mc146818;

	KASSERT(cold);
	KASSERT(CPU_IS_PRIMARY(curcpu()));

	sc->sc_year0 = 1900 + UNIX_YEAR_OFFSET;
	sc->sc_flag = 0;	/* BINARY, 24HR */

#if defined(MULTIPROCESSOR)
	/*
	 * Because some Alpha systems have clocks that can only be
	 * accessed by the primary CPU, we need to wrap the mc146818
	 * gettime/settime routines if we have such a clock.
	 */
	if (msc->sc_primary_only) {
		sc->sc_handle.todr_gettime_ymdhms = mcclock_gettime_ymdhms;
		sc->sc_handle.todr_settime_ymdhms = mcclock_settime_ymdhms;
	}
#endif

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

	KASSERT(cold);
	KASSERT(CPU_IS_PRIMARY(curcpu()));

	/* save REG_A */
	reg_a = (*sc->sc_mcread)(sc, MC_REGA);

	/* set interval 16Hz to measure pcc */
	(*sc->sc_mcwrite)(sc, MC_REGA, MC_BASE_32_KHz | MC_RATE_16_Hz);

	/* clear interrupt flags */
	(void)(*sc->sc_mcread)(sc, MC_REGC);

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < NLOOP; i++) {

		/* wait till the periodic interrupt flag is set */
		while (((*sc->sc_mcread)(sc, MC_REGC) & MC_REGC_PF) == 0)
			;
		pcc_start = cpu_counter32();

		/* wait till the periodic interrupt flag is set again */
		while (((*sc->sc_mcread)(sc, MC_REGC) & MC_REGC_PF) == 0)
			;
		pcc_end = cpu_counter32();

		ctrdiff[i] = pcc_end - pcc_start;
	}

	freq = ((ctrdiff[NLOOP - 2] + ctrdiff[NLOOP - 1]) * 16 /* Hz */) / 2;

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

	kpreempt_disable();

	/*
	 * The Alpha port calls clock init function is called on each CPU,
	 * but for this clock, we only want to do work on the primary.
	 */
	if (CPU_IS_PRIMARY(curcpu())) {
		/* enable interval clock interrupt */
		(*sc->sc_mcwrite)(sc, MC_REGA,
		    MC_BASE_32_KHz | MC_RATE_1024_Hz);
		(*sc->sc_mcwrite)(sc, MC_REGB,
		    MC_REGB_PIE | MC_REGB_SQWE | MC_REGB_BINARY | MC_REGB_24HR);
	}

	kpreempt_enable();
}
