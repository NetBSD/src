/*	$NetBSD: e500wdog.c,v 1.1.4.1 2011/06/06 09:06:26 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include "ioconf.h"

#include <sys/intr.h>

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>

struct e500wdog_softc {
	device_t sc_dev;
	struct sysmon_wdog sc_smw;
	u_int sc_wdog_period;
	bool sc_wdog_armed;
	uint64_t sc_timebase;
};
#ifndef	PQ3WDOG_PERIOD_DEFAULT
#define	PQ3WDOG_PERIOD_DEFAULT	10
#endif

static int e500wdog_match(device_t, cfdata_t, void *);
static void e500wdog_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(e500wdog, sizeof(struct e500wdog_softc),
    e500wdog_match, e500wdog_attach, NULL, NULL);

static int
e500wdog_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;

	if (!board_info_get_bool("pq3")
	    || cnl->cnl_instance != 0
	    || (psc->sc_children & cna->cna_childmask)
	    || strcmp(cnl->cnl_name, "wdog") != 0)
		return 0;

	return 1;
}

static int
e500wdog_tickle(struct sysmon_wdog *smw)
{
	mtspr(SPR_TSR, TSR_ENW);
	return 0;
}

static int
e500wdog_setmode(struct sysmon_wdog *smw)
{
	struct e500wdog_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/*
		 * We can't disarm the watchdog.
		 */
		if (sc->sc_wdog_armed)
			return EBUSY;
	} else {
		/*
		 * If no changes, don't do anything.
		 */
		if (sc->sc_wdog_armed
		    && smw->smw_period == sc->sc_wdog_period) {
			mtspr(SPR_TSR, TSR_ENW);
			return 0;
		}
		printf("%s:%d %u %u\n", __func__, __LINE__,
		    sc->sc_wdog_period, smw->smw_period);
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			sc->sc_wdog_period = PQ3WDOG_PERIOD_DEFAULT;
			smw->smw_period = PQ3WDOG_PERIOD_DEFAULT;
		}
		mtspr(SPR_TSR, TSR_ENW);
		uint32_t tcr = mfspr(SPR_TCR);
		if (!sc->sc_wdog_armed)
			tcr = (tcr & ~TCR_WRC) | TCR_WRC_RESET | TCR_WIE;
		const uint64_t period = sc->sc_wdog_period * sc->sc_timebase / 3;
		const u_int wp = __builtin_clz(period) + 1;
		tcr &= ~(TCR_WP|TCR_WPEXT);
		tcr |= __SHIFTIN(wp, TCR_WP) | __SHIFTIN(wp >> 2, TCR_WPEXT);
		mtspr(SPR_TCR, tcr);
		sc->sc_wdog_period =
		     ((1ULL << (63 - wp)) + sc->sc_timebase - 1)
			/ sc->sc_timebase;
		sc->sc_wdog_armed = true;
	}
	return 0;
}

static void
e500wdog_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct e500wdog_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;

	psc->sc_children |= cna->cna_childmask;
	sc->sc_dev = self;

	sc->sc_timebase = board_info_get_number("timebase-frequency");
	const uint32_t tcr = mfspr(SPR_TCR);
	u_int wp = __SHIFTOUT(tcr, TCR_WP) | (__SHIFTOUT(tcr, TCR_WPEXT) << 2);

#if DEBUG > 1
	printf(" tcr=%#x wp=%u", tcr, wp);
	printf(" timebase=%"PRIu64, sc->sc_timebase);
#endif
	sc->sc_wdog_armed = (tcr & TCR_WRC) != 0;
	if (sc->sc_wdog_armed)
		sc->sc_wdog_period = ((1ULL << (63 - wp)) + sc->sc_timebase - 1)
		    / sc->sc_timebase;
	else
		sc->sc_wdog_period = PQ3WDOG_PERIOD_DEFAULT;

	aprint_normal(": default period is %u seconds%s\n",
	    sc->sc_wdog_period,
	    sc->sc_wdog_armed ? " (wdog is active)" : "");

	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = e500wdog_setmode;
	sc->sc_smw.smw_tickle = e500wdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "unable to register with sysmon\n");

#if 1
	if (sc->sc_wdog_armed) {
		int error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
		    sc->sc_wdog_period);
		if (error)
			aprint_error_dev(self,
			    "failed to start kernel tickler: %d\n", error);
	}
#else
	//mtspr(SPR_TSR, TSR_ENW);
#endif
}
