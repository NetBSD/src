/*	$NetBSD: imxwdog.c,v 1.3 2014/09/25 05:05:28 ryo Exp $	*/

/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxwdog.c,v 1.3 2014/09/25 05:05:28 ryo Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <prop/proplib.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/imx/imxwdogreg.h>
#include <arm/imx/imxwdogvar.h>

struct wdog_softc {
	struct sysmon_wdog sc_smw;
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	u_int sc_wdog_max_period;
	u_int sc_wdog_period;
	bool sc_wdog_armed;
};

#ifndef IMXWDOG_PERIOD_DEFAULT
#define	IMXWDOG_PERIOD_DEFAULT	10
#endif

CFATTACH_DECL_NEW(imxwdog, sizeof(struct wdog_softc),
    wdog_match, wdog_attach, NULL, NULL);

static inline uint16_t
wdog_read(struct wdog_softc *sc, bus_size_t o)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, o);
}

static inline void
wdog_write(struct wdog_softc *sc, bus_size_t o, uint16_t v)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, o, v);
}

static int
wdog_tickle(struct sysmon_wdog *smw)
{
	struct wdog_softc * const sc = smw->smw_cookie;

	wdog_write(sc, IMX_WDOG_WSR, WSR_MAGIC1);
	wdog_write(sc, IMX_WDOG_WSR, WSR_MAGIC2);

	return 0;
}

static int
wdog_setmode(struct sysmon_wdog *smw)
{
	struct wdog_softc * const sc = smw->smw_cookie;
	uint16_t reg;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* this chip do not support wdt disable */
		aprint_debug_dev(sc->sc_dev, "setmode disable\n");
		return sc->sc_wdog_armed ? EBUSY : 0;
	}

	/*
	 * If no changes, just tickle it and return.
	 */
	if (sc->sc_wdog_armed && smw->smw_period == sc->sc_wdog_period) {
		wdog_tickle(smw);
		aprint_debug_dev(sc->sc_dev, "setmode refresh\n");
		return 0;
	}

	/* set default */
	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		sc->sc_wdog_period = IMXWDOG_PERIOD_DEFAULT;
		smw->smw_period = IMXWDOG_PERIOD_DEFAULT;
	}

	/*
	 * Make sure we don't overflow the counter.
	 */
	if (smw->smw_period >= sc->sc_wdog_max_period)
		return EINVAL;

	sc->sc_wdog_period = smw->smw_period;
	sc->sc_wdog_armed = true;

	reg = wdog_read(sc, IMX_WDOG_WCR);
	reg &= ~WCR_WT;
	reg |= __SHIFTIN(sc->sc_wdog_period * 2 - 1, WCR_WT);
	reg |= WCR_WDE;
	wdog_write(sc, IMX_WDOG_WCR, reg);

	return 0;
}

void
wdog_attach_common(device_t parent, device_t self,
    bus_space_tag_t iot, paddr_t addr, size_t size, int irq)
{
	struct wdog_softc *sc = device_private(self);
	uint16_t reg;

	sc->sc_dev = self;
	sc->sc_iot = iot;
	if (bus_space_map(iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map\n");
		return;
	}

	sc->sc_wdog_armed = __SHIFTOUT(wdog_read(sc, IMX_WDOG_WCR), WCR_WDE);
	/*
	 * Does the config file tell us to turn on the watchdog?
	 */
	if (device_cfdata(self)->cf_flags & 1)
		sc->sc_wdog_armed = true;

	sc->sc_wdog_max_period = 0xff / 2;
	sc->sc_wdog_period = IMXWDOG_PERIOD_DEFAULT;

	reg = wdog_read(sc, IMX_WDOG_WCR);
	reg &= ~WCR_WT;
	reg |= __SHIFTIN(sc->sc_wdog_period * 2 - 1, WCR_WT);
	wdog_write(sc, IMX_WDOG_WCR, reg);

	aprint_naive("\n");
	aprint_normal(": i.MX Watchdog Timer, default period is %u seconds%s\n",
	    sc->sc_wdog_period,
	    sc->sc_wdog_armed ? " (armed)" : "");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = wdog_setmode;
	sc->sc_smw.smw_tickle = wdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "unable to register with sysmon\n");

	if (sc->sc_wdog_armed) {
		int error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
		    sc->sc_wdog_period);
		if (error)
			aprint_error_dev(self,
			    "failed to start kernel tickler: %d\n", error);
		else {
			reg = wdog_read(sc, IMX_WDOG_WCR);
			reg |= WCR_WDE;
			wdog_write(sc, IMX_WDOG_WCR, reg);
		}
	}
}
