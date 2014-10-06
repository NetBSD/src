/*	$NetBSD: imx6_snvs.c,v 1.1 2014/10/06 10:15:40 ryo Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6 Secure Non-Volatile Storage
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_snvs.c,v 1.1 2014/10/06 10:15:40 ryo Exp $");

#include "locators.h"
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>
#include <dev/clock_subr.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_snvsreg.h>

struct imxsnvs_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct todr_chip_handle sc_todr;
};

#define SNVS_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, reg)

#define SNVS_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, reg, val)

static int imxsnvs_match(device_t, struct cfdata *, void *);
static void imxsnvs_attach(device_t, device_t, void *);
static int imxsnvs_rtc_enable(struct imxsnvs_softc *);
static int imxsnvs_rtc_disable(struct imxsnvs_softc *);
static int imxsnvs_gettime(todr_chip_handle_t, struct timeval *);
static int imxsnvs_settime(todr_chip_handle_t, struct timeval *);


CFATTACH_DECL_NEW(imxsnvs, sizeof(struct imxsnvs_softc),
    imxsnvs_match, imxsnvs_attach, NULL, NULL);

/* ARGSUSED */
static int
imxsnvs_match(device_t parent __unused, struct cfdata *match __unused, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	switch (aa->aa_addr) {
	case (IMX6_AIPS1_BASE + AIPS1_SNVS_BASE):
		return 1;
	}

	return 0;
}

/* ARGSUSED */
static void
imxsnvs_attach(device_t parent __unused, device_t self, void *aux)
{
	struct imxsnvs_softc *sc;
	struct axi_attach_args *aa;
	uint32_t v1, v2;

	aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = AIPS1_SNVS_SIZE;

	aprint_naive("\n");
	aprint_normal(": Secure Non-Volatile Storage\n");
	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	v1 = SNVS_READ(sc, SNVS_HPVIDR1);
	v2 = SNVS_READ(sc, SNVS_HPVIDR2);
	aprint_verbose_dev(self, "id=0x%llx, ver=%lld.%lld, ip_era=0x%llx, "
	    "intg_opt=0x%llx, eco_rev=0x%llx, config_opt=0x%llx\n",
	    __SHIFTOUT(v1, SNVS_HPVIDR1_IP_ID),
	    __SHIFTOUT(v1, SNVS_HPVIDR1_MAJOR_REV),
	    __SHIFTOUT(v1, SNVS_HPVIDR1_MINOR_REV),
	    __SHIFTOUT(v2, SNVS_HPVIDR2_IP_ERA),
	    __SHIFTOUT(v2, SNVS_HPVIDR2_INTG_OPT),
	    __SHIFTOUT(v2, SNVS_HPVIDR2_ECO_REV),
	    __SHIFTOUT(v2, SNVS_HPVIDR2_CONFIG_OPT));

	if (imxsnvs_rtc_enable(sc) != 0) {
		aprint_error_dev(self, "cannot enable RTC\n");
		return;
	}

	sc->sc_todr.todr_gettime = imxsnvs_gettime;
	sc->sc_todr.todr_settime = imxsnvs_settime;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);

	return;
}

static int
imxsnvs_rtc_enable(struct imxsnvs_softc *sc)
{
	uint32_t v;
	int timeout;

	/* enable SRTC */
	v = SNVS_READ(sc, SNVS_LPCR);
	SNVS_WRITE(sc, SNVS_LPCR, v | SNVS_LPCR_SRTC_ENV);
	for (timeout = 10000; timeout > 0; timeout--) {
		if (SNVS_READ(sc, SNVS_LPCR) & SNVS_LPCR_SRTC_ENV)
			break;
	}
	if (timeout == 0)
		return ETIMEDOUT;

	return 0;
}

static int
imxsnvs_rtc_disable(struct imxsnvs_softc *sc)
{
	uint32_t v;
	int timeout;

	/* disable SRTC */
	v = SNVS_READ(sc, SNVS_LPCR);
	SNVS_WRITE(sc, SNVS_LPCR, v & ~SNVS_LPCR_SRTC_ENV);
	for (timeout = 10000; timeout > 0; timeout--) {
		if (!(SNVS_READ(sc, SNVS_LPCR) & SNVS_LPCR_SRTC_ENV))
			break;
	}
	if (timeout == 0)
		return ETIMEDOUT;

	return 0;
}

static int
imxsnvs_gettime(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct imxsnvs_softc *sc;
	uint64_t c1, c2;

	sc = tch->cookie;

	c2 = ((uint64_t)SNVS_READ(sc, SNVS_LPSRTCMR) << 32) +
	    SNVS_READ(sc, SNVS_LPSRTCLR);
	do {
		c1 = c2;
		c2 = ((uint64_t)SNVS_READ(sc, SNVS_LPSRTCMR) << 32) +
		    SNVS_READ(sc, SNVS_LPSRTCLR);
	} while (c1 != c2);

	tvp->tv_sec = c1 >> SVNS_COUNTER_SHIFT;
	tvp->tv_usec = (c1 % SVNS_COUNTER_HZ) * 1000000 / SVNS_COUNTER_HZ;

	return 0;
}

static int
imxsnvs_settime(todr_chip_handle_t tch, struct timeval *tvp)
{
	struct imxsnvs_softc *sc;
	uint64_t c, h, l;
	int rv;

	c = (uint64_t)tvp->tv_sec * SVNS_COUNTER_HZ +
	    (uint64_t)tvp->tv_usec * SVNS_COUNTER_HZ / 1000000;
	h = __SHIFTIN((c >> 32) & SNVS_LPSRTCMR_SRTC, SNVS_LPSRTCMR_SRTC);
	l = c & 0xffffffff;

	sc = tch->cookie;
	if ((rv = imxsnvs_rtc_disable(sc)) != 0)
		return rv;

	SNVS_WRITE(sc, SNVS_LPSRTCMR, h);
	SNVS_WRITE(sc, SNVS_LPSRTCLR, l);

	if ((rv = imxsnvs_rtc_enable(sc)) != 0)
		return rv;

	return 0;
}
