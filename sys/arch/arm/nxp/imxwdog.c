/*	$NetBSD: imxwdog.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imxwdog.c,v 1.3 2021/01/27 03:10:20 thorpej Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imxwdogreg.h>

struct imxwdog_softc {
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


int imxwdog_match(device_t, cfdata_t, void *);
void imxwdog_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(imxwdog, sizeof(struct imxwdog_softc),
    imxwdog_match, imxwdog_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx21-wdt" },
	{ .compat = "fsl,imx6q-wdt" },
	DEVICE_COMPAT_EOL
};

int
imxwdog_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}


static inline uint16_t
wdog_read(struct imxwdog_softc *sc, bus_size_t o)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, o);
}

static inline void
wdog_write(struct imxwdog_softc *sc, bus_size_t o, uint16_t v)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, o, v);
}

static int
wdog_tickle(struct sysmon_wdog *smw)
{
	struct imxwdog_softc * const sc = smw->smw_cookie;

	wdog_write(sc, IMX_WDOG_WSR, WSR_MAGIC1);
	wdog_write(sc, IMX_WDOG_WSR, WSR_MAGIC2);

	return 0;
}

static int
wdog_setmode(struct sysmon_wdog *smw)
{
	struct imxwdog_softc * const sc = smw->smw_cookie;
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
imxwdog_attach(device_t parent, device_t self, void *aux)
{
	struct imxwdog_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	int error = bus_space_map(bst, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error(": couldn't map %" PRIxBUSADDR ": %d", addr, error);
		return;
	}

#if 0
	char intrstr[128];
	if (!fdtbus_intr_str(ifsc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return NULL;
	}
	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    imxwdog_intr, sc, device_xname(sc->sc_dev));
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish interrupt on %s\n",
		    intrstr);
		return NULL;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);
#endif

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	sc->sc_wdog_armed = __SHIFTOUT(wdog_read(sc, IMX_WDOG_WCR), WCR_WDE);
	/*
	 * Does the config file tell us to turn on the watchdog?
	 */
	if (device_cfdata(self)->cf_flags & 1)
		sc->sc_wdog_armed = true;

	sc->sc_wdog_max_period = 0xff / 2;
	sc->sc_wdog_period = IMXWDOG_PERIOD_DEFAULT;

	uint16_t reg = wdog_read(sc, IMX_WDOG_WCR);
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
		error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
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
