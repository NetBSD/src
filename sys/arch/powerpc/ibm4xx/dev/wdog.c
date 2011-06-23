/* $NetBSD: wdog.c,v 1.10.8.1 2011/06/23 14:19:30 cherry Exp $ */

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
 * Watchdog timer support for the IBM 405GP processor.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdog.c,v 1.10.8.1 2011/06/23 14:19:30 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
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

struct wdog_softc {
	device_t sc_dev;
	struct sysmon_wdog sc_smw;
	bool sc_wdog_armed;
	int sc_wdog_period;
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

	sc->sc_wdog_period = (2LL << 29) / processor_freq;
	aprint_normal(": %d second period\n", sc->sc_wdog_period);

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
	uint32_t tsr;

	tsr = mfspr(SPR_TSR);
	tsr |= TSR_ENW | TSR_WIS;
	mtspr(SPR_TSR, tsr);
	return (0);
}

static int
wdog_setmode(struct sysmon_wdog *smw)
{
	struct wdog_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		if (sc->sc_wdog_armed) {
			uint32_t tsr = mfspr(SPR_TSR);
			tsr &= ~(TSR_ENW | TSR_WIS);
			mtspr(SPR_TSR, tsr);
			sc->sc_wdog_armed = false;
		}
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;
		else if (smw->smw_period != sc->sc_wdog_period) {
			/*
			 * There's 4 set watchdog periods on the 405GP,
			 * but we only support the longest one (2.684
			 * seconds).
			 */
			return (EOPNOTSUPP);
		}
		sc->sc_wdog_armed = true;

		uint32_t tcr = mfspr(SPR_TCR);
		tcr |= TCR_WP_2_29 | TCR_WRC_SYSTEM;
		mtspr(SPR_TCR, tcr);

		/* Arm the watchdog. */
		wdog_tickle(smw);
	}
	return (0);
}
