/*	$NetBSD: imx7_iomux.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_iomux.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7_iomuxreg.h>

struct imxiomux_softc {
	device_t sc_dev;
	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh_iomuxc;		/* IOMUXC */
	bus_space_handle_t sc_ioh_iomuxc_gpr;		/* IOMUXC_GPR */
	bus_space_handle_t sc_ioh_iomuxc_lpsr;		/* IOMUXC_LPSR */
	bus_space_handle_t sc_ioh_iomuxc_lpsr_gpr;	/* IOMUXC_LPSR_GPR */
};

static struct imxiomux_softc *iomux_softc;

static int imxiomux_match(device_t, struct cfdata *, void *);
static void imxiomux_attach(device_t, device_t, void *);
static int iomux_getoffset(struct imxiomux_softc *, uint32_t,
                           bus_space_handle_t *, uint32_t *);

CFATTACH_DECL_NEW(imxiomux, sizeof(struct imxiomux_softc),
    imxiomux_match, imxiomux_attach, NULL, NULL);

/* ARGSUSED */
static int
imxiomux_match(device_t parent __unused, struct cfdata *match __unused,
               void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	if (iomux_softc != NULL)
		return 0;

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS1_IOMUXC_BASE):
		return 1;
	}

	return 0;
}

/* ARGSUSED */
static void
imxiomux_attach(device_t parent __unused, device_t self, void *aux)
{
	struct imxiomux_softc *sc;
	struct axi_attach_args *aa;

	aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_addr = aa->aa_addr;

	aprint_naive("\n");
	aprint_normal(": IOMUX Controller\n");

	if (bus_space_map(sc->sc_iot, IMX7_AIPS_BASE + AIPS1_IOMUXC_BASE,
	    AIPS1_IOMUXC_SIZE, 0, &sc->sc_ioh_iomuxc)) {
		aprint_error_dev(self, "Cannot map IOMUXC registers\n");
		goto failure0;
	}
	if (bus_space_map(sc->sc_iot, IMX7_AIPS_BASE + AIPS1_IOMUXC_GPR_BASE,
	    AIPS1_IOMUXC_GPR_SIZE, 0, &sc->sc_ioh_iomuxc_gpr)) {
		aprint_error_dev(self, "Cannot map IOMUXC_GPR registers\n");
		goto failure1;
		return;
	}
	if (bus_space_map(sc->sc_iot, IMX7_AIPS_BASE + AIPS1_IOMUXC_BASE,
	    AIPS1_IOMUXC_LPSR_SIZE, 0, &sc->sc_ioh_iomuxc_lpsr)) {
		aprint_error_dev(self, "Cannot map IOMUXC_LPSR registers\n");
		goto failure2;
		return;
	}
	if (bus_space_map(sc->sc_iot, IMX7_AIPS_BASE + AIPS1_IOMUXC_GPR_BASE,
	    AIPS1_IOMUXC_LPSR_GPR_SIZE, 0, &sc->sc_ioh_iomuxc_lpsr_gpr)) {
		aprint_error_dev(self, "Cannot map IOMUXC_LPSR registers\n");
		goto failure3;
	}

	iomux_softc = sc;
	return;

 failure3:
	bus_space_unmap(sc->sc_iot,
	    sc->sc_ioh_iomuxc_lpsr, AIPS1_IOMUXC_LPSR_SIZE);
 failure2:
	bus_space_unmap(sc->sc_iot,
	    sc->sc_ioh_iomuxc_gpr, AIPS1_IOMUXC_GPR_SIZE);
 failure1:
	bus_space_unmap(sc->sc_iot,
	    sc->sc_ioh_iomuxc, AIPS1_IOMUXC_SIZE);
 failure0:
	return;
}

static int
iomux_getoffset(struct imxiomux_softc *sc, uint32_t regaddr,
                bus_space_handle_t *iohp, uint32_t *regoff)
{
	switch (regaddr & 0xffff0000) {
	case AIPS1_IOMUXC_GPR_BASE:
		*iohp = sc->sc_ioh_iomuxc;
		*regoff = regaddr - AIPS1_IOMUXC_GPR_BASE;
		break;
	case AIPS1_IOMUXC_BASE:
		*iohp = sc->sc_ioh_iomuxc;
		*regoff = regaddr - AIPS1_IOMUXC_BASE;
		break;
	case AIPS1_IOMUXC_LPSR_BASE:
		*iohp = sc->sc_ioh_iomuxc;
		*regoff = regaddr - AIPS1_IOMUXC_LPSR_BASE;
		break;
	case AIPS1_IOMUXC_LPSR_GPR_BASE:
		*iohp = sc->sc_ioh_iomuxc;
		*regoff = regaddr - AIPS1_IOMUXC_LPSR_GPR_BASE;
		break;
	default:
		return -1;
	}
	return 0;
}

uint32_t
iomux_read(uint32_t reg)
{
	bus_space_handle_t ioh;
	uint32_t regoff;

	if (iomux_softc == NULL)
		return 0;
	if (iomux_getoffset(iomux_softc, reg, &ioh, &regoff) != 0)
		return 0;

	return bus_space_read_4(iomux_softc->sc_iot, ioh, regoff);
}

void
iomux_write(uint32_t reg, uint32_t val)
{
	bus_space_handle_t ioh;
	uint32_t regoff;

	if (iomux_softc == NULL)
		return;
	if (iomux_getoffset(iomux_softc, reg, &ioh, &regoff) != 0)
		return;

	bus_space_write_4(iomux_softc->sc_iot, ioh, regoff, val);
}
