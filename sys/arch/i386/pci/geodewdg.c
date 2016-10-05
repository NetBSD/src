/*	$NetBSD: geodewdg.c,v 1.11.30.1 2016/10/05 20:55:28 skrll Exp $	*/

/*-
 * Copyright (c) 2005 David Young.  All rights reserved.
 *
 * This code was written by David Young.
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
 * THIS SOFTWARE IS PROVIDED BY DAVID YOUNG ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DAVID
 * YOUNG BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * Device driver for the watchdog timer built into the
 * AMD Geode SC1100 processor.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: geodewdg.c,v 1.11.30.1 2016/10/05 20:55:28 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>
#include <uvm/uvm_extern.h>
#include <sys/bus.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <arch/i386/pci/geodevar.h>
#include <arch/i386/pci/geodereg.h>
#include <dev/sysmon/sysmonvar.h>

#ifdef GEODE_DEBUG
#define	GEODE_DPRINTF(__x) printf __x
#else /* GEODE_DEBUG */
#define	GEODE_DPRINTF(__x) /* nothing */
#endif

struct geode_wdog_softc {
	struct geode_gcb_softc *sc_gcb_dev;

	uint16_t		sc_countdown;
	uint8_t			sc_prescale;
	struct sysmon_wdog      sc_smw;
};

static int attached = 0;

static void
geode_wdog_disable(struct geode_wdog_softc *sc)
{
	uint16_t wdcnfg;

	/* cancel any pending countdown */
	sc->sc_countdown = 0;
	bus_space_write_2(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_WDTO, 0);
	/* power-down clock */
	wdcnfg = bus_space_read_2(sc->sc_gcb_dev->sc_iot,
	    sc->sc_gcb_dev->sc_ioh, SC1100_GCB_WDCNFG);

	GEODE_DPRINTF(("%s: wdcnfg %#04" PRIx16 " -> ", __func__, wdcnfg));

	wdcnfg |= SC1100_WDCNFG_WD32KPD;
	wdcnfg &= ~(SC1100_WDCNFG_WDTYPE2_MASK | SC1100_WDCNFG_WDTYPE1_MASK);
	/* This no-op is for the reader's benefit. */
        wdcnfg |= SC1100_WDCNFG_WDTYPE1_NOACTION |
                  SC1100_WDCNFG_WDTYPE2_NOACTION;
	bus_space_write_2(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_WDCNFG, wdcnfg);

	GEODE_DPRINTF(("%#04" PRIx16 "\n", wdcnfg));
}

static void
geode_wdog_enable(struct geode_wdog_softc *sc)
{
	uint16_t wdcnfg;

	/* power-up clock and set prescale */
	wdcnfg = bus_space_read_2(sc->sc_gcb_dev->sc_iot,
	    sc->sc_gcb_dev->sc_ioh, SC1100_GCB_WDCNFG);

	GEODE_DPRINTF(("%s: wdcnfg %#04" PRIx16 " -> ", __func__, wdcnfg));

	wdcnfg &= ~(SC1100_WDCNFG_WD32KPD | SC1100_WDCNFG_WDPRES_MASK |
	            SC1100_WDCNFG_WDTYPE1_MASK | SC1100_WDCNFG_WDTYPE2_MASK);
	wdcnfg |= __SHIFTIN(sc->sc_prescale, SC1100_WDCNFG_WDPRES_MASK);
        wdcnfg |= SC1100_WDCNFG_WDTYPE1_RESET | SC1100_WDCNFG_WDTYPE2_NOACTION;

	bus_space_write_2(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_WDCNFG, wdcnfg);

	GEODE_DPRINTF(("%#04" PRIx16 "\n", wdcnfg));
}

static void
geode_wdog_reset(struct geode_wdog_softc *sc)
{
	/* set countdown */
	bus_space_write_2(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_WDTO, sc->sc_countdown);
}

static int
geode_wdog_tickle(struct sysmon_wdog *smw)
{
	int s;
	struct geode_wdog_softc *sc = smw->smw_cookie;

	s = splhigh();
	geode_wdog_reset(sc);
	splx(s);
	return 0;
}

static int
geode_wdog_setmode(struct sysmon_wdog *smw)
{
	struct geode_wdog_softc *sc = smw->smw_cookie;
	uint32_t ticks;
	int prescale, s;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		s = splhigh();
		geode_wdog_disable(sc);
		splx(s);
		return 0;
	}
	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		smw->smw_period = 32;
	else if (smw->smw_period > SC1100_WDIVL_MAX) /* too big? */
		return EINVAL;

	GEODE_DPRINTF(("%s: period %u\n", __func__, smw->smw_period));

	ticks = smw->smw_period * SC1100_WDCLK_HZ;

	GEODE_DPRINTF(("%s: ticks0 %" PRIu32 "\n", __func__, ticks));

	for (prescale = 0; ticks > UINT16_MAX; prescale++)
		ticks /= 2;

	GEODE_DPRINTF(("%s: ticks %" PRIu32 "\n", __func__, ticks));
	GEODE_DPRINTF(("%s: prescale %d\n", __func__, prescale));

	KASSERT(prescale <= SC1100_WDCNFG_WDPRES_MAX);
	KASSERT(ticks <= UINT16_MAX);

	s = splhigh();

	sc->sc_prescale = (uint8_t)prescale;
	sc->sc_countdown = (uint16_t)ticks;

	geode_wdog_enable(sc);

	geode_wdog_reset(sc);

	splx(s);
	return 0;
}

static int
geode_wdog_match(device_t parent, cfdata_t match, void *aux)
{
	return !attached;
}

static void
geode_wdog_attach(device_t parent, device_t self, void *aux)
{
	struct geode_wdog_softc *sc = device_private(self);
	uint8_t wdsts;

	aprint_naive(": Watchdog Timer\n");
	aprint_normal(": AMD Geode SC1100 Watchdog Timer\n");


	/*
	 * Hook up the watchdog timer.
	 */
	sc->sc_gcb_dev = device_private(parent);
	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = geode_wdog_setmode;
	sc->sc_smw.smw_tickle = geode_wdog_tickle;
	sc->sc_smw.smw_period = 32;

	/*
	 * Determine cause of the last reset, and issue a warning if it
	 * was due to watchdog expiry.
	 */
	wdsts = bus_space_read_1(sc->sc_gcb_dev->sc_iot,
	    sc->sc_gcb_dev->sc_ioh, SC1100_GCB_WDSTS);

	GEODE_DPRINTF(("%s: status %#02" PRIx8 "\n", device_xname(self),
	    wdsts));

	if (wdsts & SC1100_WDSTS_WDRST)
		aprint_error(
		    "%s: WARNING: LAST RESET DUE TO WATCHDOG EXPIRATION!\n",
		    device_xname(self));

	/* reset WDOVF by writing 1 to it */
	bus_space_write_1(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_WDSTS, wdsts & SC1100_WDSTS_WDOVF);

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register watchdog with sysmon\n",
		    device_xname(self));

	/* cancel any pending countdown */
	geode_wdog_disable(sc);

	attached = 1;
}

static int
geode_wdog_detach(device_t self, int flags)
{
	int rc;
	struct geode_wdog_softc *sc = device_private(self);

	if ((rc = sysmon_wdog_unregister(&sc->sc_smw)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	/* cancel any pending countdown */
	geode_wdog_disable(sc);

	attached = 0;

	return 0;
}

CFATTACH_DECL_NEW(geodewdog, sizeof(struct geode_wdog_softc),
	      geode_wdog_match, geode_wdog_attach, geode_wdog_detach, NULL);
