/* $NetBSD: wdog.c,v 1.13 2026/06/14 00:02:35 rkujawa Exp $ */

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Watchdog timer support for the IBM 405GP and 440/460EX processors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdog.c,v 1.13 2026/06/14 00:02:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/wdog.h>

#include <prop/proplib.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/opbvar.h>

#include <dev/sysmon/sysmonvar.h>

static int wdog_match(device_t, cfdata_t, void *);
static void wdog_attach(device_t, device_t, void *);
static int wdog_tickle(struct sysmon_wdog *);
static int wdog_setmode(struct sysmon_wdog *);
#ifdef PPC_IBM440
static void wdog_ktickle(void *);
#endif

struct wdog_softc {
	device_t sc_dev;
	struct sysmon_wdog sc_smw;
	bool sc_wdog_armed;
	int sc_wdog_period;
#ifdef PPC_IBM440
	struct callout sc_tickler;	/* self-tickle after "disarm" */
#endif
};

CFATTACH_DECL_NEW(wdog, sizeof(struct wdog_softc),
    wdog_match, wdog_attach, NULL, NULL);

static int
wdog_match(device_t parent, cfdata_t cf, void *aux)
{
	struct opb_attach_args * const oaa = aux;

	/* match only watchdog devices */
	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return (0);

	return (1);
}

static void
wdog_attach(device_t parent, device_t self, void *aux)
{
	struct wdog_softc * const sc = device_private(self);
	unsigned int processor_freq;
	prop_number_t freq;

	freq = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(freq != NULL);
	processor_freq = (unsigned int) prop_number_integer_value(freq);

#ifdef PPC_IBM440
	/*
	 * The 440 watchdog period encodings sit four powers of two
	 * above the 405's... for whatever reason.
	 */
	sc->sc_wdog_period = (2LL << 33) / processor_freq;
#else
	sc->sc_wdog_period = (2LL << 29) / processor_freq;
#endif
	aprint_normal(": %d second period\n", sc->sc_wdog_period);

#ifdef PPC_IBM440
	/*
	 * TSR[WRS] survives a watchdog-forced reset; report and clear
	 * it (TSR is write-1-to-clear) so a rebooted headless board
	 * leaves evidence of why.
	 *
	 * Btw. on Sam460ex firmware messes with TSR[WRS], so it's 
	 * pointless.
	 */
	if (mfspr(SPR_TSR) & TSR_WRS_MASK) {
		aprint_normal_dev(self,
		    "previous reset was forced by the watchdog\n");
		mtspr(SPR_TSR, TSR_WRS_MASK);
	}

	callout_init(&sc->sc_tickler, 0);
	callout_setfunc(&sc->sc_tickler, wdog_ktickle, sc);
#endif

	sc->sc_dev = self;
	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = wdog_setmode;
	sc->sc_smw.smw_tickle = wdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "unable to register with sysmon\n");
}

static int
wdog_tickle(struct sysmon_wdog *smw)
{
#ifdef PPC_IBM440
	/*
	 * TSR is write-1-to-clear
	 */
	mtspr(SPR_TSR, TSR_ENW | TSR_WIS);
#else
	uint32_t tsr;

	tsr = mfspr(SPR_TSR);
	tsr |= TSR_ENW | TSR_WIS;
	mtspr(SPR_TSR, tsr);
#endif
	return (0);
}

#ifdef PPC_IBM440
/*
 * TCR[WRC] is sticky: once the watchdog has been armed with a reset
 * action it cannot be turned off until the next reset.  "Disarming"
 * therefore hands the tickling duty to this callout instead.
 */
static void
wdog_ktickle(void *arg)
{
	struct wdog_softc * const sc = arg;

	mtspr(SPR_TSR, TSR_ENW | TSR_WIS);
	callout_schedule(&sc->sc_tickler, hz * sc->sc_wdog_period / 3);
}
#endif

static int
wdog_setmode(struct sysmon_wdog *smw)
{
	struct wdog_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		if (sc->sc_wdog_armed) {
#ifdef PPC_IBM440
			/*
			 * The hardware cannot be disarmed (sticky
			 * TCR[WRC]); keep it fed from a callout so the
			 * system stays up without userland's help.
			 */
			sc->sc_wdog_armed = false;
			wdog_ktickle(sc);
#else
			uint32_t tsr = mfspr(SPR_TSR);
			tsr &= ~(TSR_ENW | TSR_WIS);
			mtspr(SPR_TSR, tsr);
			sc->sc_wdog_armed = false;
#endif
		}
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;
		else if (smw->smw_period != sc->sc_wdog_period) {
			/*
			 * There's 4 set watchdog periods, but we only
			 * support the longest one (2.684 seconds on a
			 * 400MHz 405GP, 14.9 seconds on a 1.155GHz
			 * 460EX).
			 */
			return (EOPNOTSUPP);
		}
		sc->sc_wdog_armed = true;

#ifdef PPC_IBM440
		/*
		 * TCR is shared with the clock (PIE/FIE) and the tprof
		 * FIT backend (FP), all read-modify-write: keep the
		 * update atomic with respect to them.
		 */
		const int s = splhigh();
		uint32_t tcr = mfspr(SPR_TCR);
		tcr |= TCR_WP_2_33 | TCR_WRC_SYSTEM;
		mtspr(SPR_TCR, tcr);
		splx(s);
		callout_stop(&sc->sc_tickler);
#else
		uint32_t tcr = mfspr(SPR_TCR);
		tcr |= TCR_WP_2_29 | TCR_WRC_SYSTEM;
		mtspr(SPR_TCR, tcr);
#endif

		/* Arm the watchdog. */
		wdog_tickle(smw);
	}
	return (0);
}
