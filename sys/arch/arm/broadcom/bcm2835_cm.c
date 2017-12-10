/*	$NetBSD: bcm2835_cm.c,v 1.2 2017/12/10 21:38:26 skrll Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael van Elst
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
 * Driver for BCM2835 Clock Manager
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_cm.c,v 1.2 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_cm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

struct bcm2835cm_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

#define CM_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define CM_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))

static int bcmcm_match(device_t, cfdata_t, void *);
static void bcmcm_attach(device_t, device_t, void *);
static int bcmcm_wait(struct bcm2835cm_softc *, int, int);

CFATTACH_DECL_NEW(bcmcm_fdt, sizeof(struct bcm2835cm_softc),
    bcmcm_match, bcmcm_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmcm_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = {
	    "brcm,bcm2835-cprman",
	    NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcmcm_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835cm_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	aprint_naive("\n");
	aprint_normal(": CM\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	const int phandle = faa->faa_phandle;

	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	/* Success!  */

	return;
}

static int
bcmcm_wait(struct bcm2835cm_softc *sc, int ctlreg, int onoff)
{
	int i;
	uint32_t r;

	for (i=0; i<100; ++i) {
		r = CM_READ(sc, ctlreg);
		if (((r & CM_CTL_BUSY) != 0) == onoff)
			break;
		delay(10);
	}
	if (i >= 100) {
		device_printf(sc->sc_dev, "busy (addr=%#x)\n", ctlreg);
		return EIO;
	}

	return 0;
}

int
bcm_cm_set(enum bcm_cm_clock clk, uint32_t ctl, uint32_t div)
{
	struct bcm2835cm_softc *sc;
	device_t dev;
	int ctlreg, divreg;
	uint32_t r;

	dev = device_find_by_driver_unit("bcmcm", 0);
	if (dev == NULL)
		return ENXIO;
	sc = device_private(dev);

	switch (clk) {
	case BCM_CM_GP0:
		ctlreg = CM_GP0CTL;
		divreg = CM_GP0DIV;
		break;
	case BCM_CM_GP1:
		ctlreg = CM_GP1CTL;
		divreg = CM_GP1DIV;
		break;
	case BCM_CM_GP2:
		ctlreg = CM_GP2CTL;
		divreg = CM_GP2DIV;
		break;
	case BCM_CM_PCM:
		ctlreg = CM_PCMCTL;
		divreg = CM_PCMDIV;
		break;
	case BCM_CM_PWM:
		ctlreg = CM_PWMCTL;
		divreg = CM_PWMDIV;
		break;
	default:
		return EINVAL;
	}

	ctl &= ~CM_CTL_PASSWD;
	ctl |= __SHIFTIN(CM_PASSWD, CM_CTL_PASSWD);
	div &= ~CM_DIV_PASSWD;
	div |= __SHIFTIN(CM_PASSWD, CM_DIV_PASSWD);

	/*
	 * if clock is running, turn it off and wait for
	 * the cycle to end
	 */
	r = CM_READ(sc, ctlreg);
	if (r & CM_CTL_ENAB) {
		r &= ~CM_CTL_PASSWD;
		r |= __SHIFTIN(CM_PASSWD, CM_CTL_PASSWD);
		r &= ~CM_CTL_ENAB;
		CM_WRITE(sc, ctlreg, r);
	}

	bcmcm_wait(sc, ctlreg, 0);

	/* configure new divider, mode, don't enable */
	CM_WRITE(sc, divreg, div);
	CM_WRITE(sc, ctlreg, ctl & ~CM_CTL_ENAB);

	/* enable it */
	if (ctl & CM_CTL_ENAB) {
		CM_WRITE(sc, ctlreg, ctl);
		return bcmcm_wait(sc, ctlreg, 1);
	}

	return 0;
}

int
bcm_cm_get(enum bcm_cm_clock clk, uint32_t *ctlp, uint32_t *divp)
{
	struct bcm2835cm_softc *sc;
	device_t dev;
	int ctlreg, divreg;

	dev = device_find_by_driver_unit("bcmcm", 0);
	if (dev == NULL)
		return ENXIO;
	sc = device_private(dev);

	switch (clk) {
	case BCM_CM_GP0:
		ctlreg = CM_GP0CTL;
		divreg = CM_GP0DIV;
		break;
	case BCM_CM_GP1:
		ctlreg = CM_GP1CTL;
		divreg = CM_GP1DIV;
		break;
	case BCM_CM_GP2:
		ctlreg = CM_GP2CTL;
		divreg = CM_GP2DIV;
		break;
	case BCM_CM_PCM:
		ctlreg = CM_PCMCTL;
		divreg = CM_PCMDIV;
		break;
	case BCM_CM_PWM:
		ctlreg = CM_PWMCTL;
		divreg = CM_PWMDIV;
		break;
	default:
		return EINVAL;
	}

	if (ctlp != NULL)
		*ctlp = CM_READ(sc, ctlreg);
	if (divp != NULL)
		*divp = CM_READ(sc, divreg);

	return 0;
}
