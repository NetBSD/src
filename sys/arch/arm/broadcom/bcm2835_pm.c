/*	$NetBSD: bcm2835_pm.c,v 1.2 2013/01/10 02:23:26 jmcneill Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_pm.c,v 1.2 2013/01/10 02:23:26 jmcneill Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/wdog.h>
#include <sys/bus.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>
#include <arm/broadcom/bcm2835_pmvar.h>

#ifndef BCM2835_PM_DEFAULT_PERIOD
#define BCM2835_PM_DEFAULT_PERIOD	15	/* seconds */
#endif

#define	 BCM2835_PM_PASSWORD		0x5a000000

#define	BCM2835_PM_RSTC		0x1c
#define	 BCM2835_PM_RSTC_CONFIGMASK	0x00000030
#define	 BCM2835_PM_RSTC_FULL_RESET	0x00000020
#define	 BCM2835_PM_RSTC_RESET		0x00000102

#define	BCM2835_PM_WDOG		0x24
#define	 BCM2835_PM_WDOG_TIMEMASK	0x000fffff

struct bcm2835pm_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct sysmon_wdog sc_smw;
};

static struct bcm2835pm_softc *bcm2835pm_sc;

static int bcmpm_match(device_t, cfdata_t, void *);
static void bcmpm_attach(device_t, device_t, void *);

static void bcmpm_wdog_set_timeout(struct bcm2835pm_softc *, uint32_t);

static int bcmpm_wdog_setmode(struct sysmon_wdog *);
static int bcmpm_wdog_tickle(struct sysmon_wdog *);

CFATTACH_DECL_NEW(bcmpm_amba, sizeof(struct bcm2835pm_softc),
    bcmpm_match, bcmpm_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmpm_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmpm") != 0)
		return 0;

	return 1;
}

static void
bcmpm_attach(device_t parent, device_t self, void *aux)
{
        struct bcm2835pm_softc *sc = device_private(self);
 	struct amba_attach_args *aaa = aux;

	aprint_naive("\n");
	aprint_normal(": Power management, Reset and Watchdog controller\n");

	if (bcm2835pm_sc == NULL)
		bcm2835pm_sc = sc;
	
	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, BCM2835_PM_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	/* watchdog */
	sc->sc_smw.smw_name = device_xname(sc->sc_dev);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = bcmpm_wdog_setmode;
	sc->sc_smw.smw_tickle = bcmpm_wdog_tickle;
	sc->sc_smw.smw_period = BCM2835_PM_DEFAULT_PERIOD;
	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "couldn't register watchdog\n");
}

static void
bcmpm_wdog_set_timeout(struct bcm2835pm_softc *sc, uint32_t ticks)
{
	uint32_t tmp, rstc, wdog;

	tmp = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BCM2835_PM_RSTC);

	rstc = wdog = BCM2835_PM_PASSWORD;

	rstc |= tmp & ~BCM2835_PM_RSTC_CONFIGMASK;
	rstc |= BCM2835_PM_RSTC_FULL_RESET;
	
	wdog |= ticks & BCM2835_PM_WDOG_TIMEMASK;
	
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_PM_WDOG, wdog);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_PM_RSTC, rstc);
}

static int
bcmpm_wdog_setmode(struct sysmon_wdog *smw)
{
	struct bcm2835pm_softc *sc = smw->smw_cookie;
	int error = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_PM_RSTC,
		    BCM2835_PM_PASSWORD | BCM2835_PM_RSTC_RESET);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = BCM2835_PM_DEFAULT_PERIOD;
		if (smw->smw_period > (BCM2835_PM_WDOG_TIMEMASK >> 16))
			return EINVAL;
		error = bcmpm_wdog_tickle(smw);
	}

	return error;
}

static int
bcmpm_wdog_tickle(struct sysmon_wdog *smw)
{
	struct bcm2835pm_softc *sc = smw->smw_cookie;
	uint32_t timeout = smw->smw_period << 16;

	bcmpm_wdog_set_timeout(sc, timeout);

	return 0;
}

void
bcm2835_system_reset(void)
{
	struct bcm2835pm_softc *sc = bcm2835pm_sc;
	uint32_t timeout = 10;

	bcmpm_wdog_set_timeout(sc, timeout);
}
