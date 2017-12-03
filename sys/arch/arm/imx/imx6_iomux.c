/*	$NetBSD: imx6_iomux.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_iomux.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_iomuxreg.h>
#include "locators.h"

struct imxiomux_softc {
	device_t sc_dev;
	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static struct imxiomux_softc *iomux_softc;

static int imxiomux_match(device_t, struct cfdata *, void *);
static void imxiomux_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxiomux, sizeof(struct imxiomux_softc),
    imxiomux_match, imxiomux_attach, NULL, NULL);

/* ARGSUSED */
static int
imxiomux_match(device_t parent __unused, struct cfdata *match __unused, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	if (iomux_softc != NULL)
		return 0;

	switch (aa->aa_addr) {
	case (IMX6_AIPS1_BASE + AIPS1_IOMUXC_BASE):
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

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = AIPS1_IOMUXC_SIZE;

	aprint_naive("\n");
	aprint_normal(": IOMUX Controller\n");
	if (bus_space_map(sc->sc_iot, sc->sc_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	iomux_softc = sc;
	return;
}

uint32_t
iomux_read(uint32_t reg)
{
	if (iomux_softc == NULL)
		return 0;

	return bus_space_read_4(iomux_softc->sc_iot, iomux_softc->sc_ioh, reg);
}

void
iomux_write(uint32_t reg, uint32_t val)
{
	if (iomux_softc == NULL)
		return;

	bus_space_write_4(iomux_softc->sc_iot, iomux_softc->sc_ioh, reg, val);
}


static void
iomux_set_function_sub(struct imxiomux_softc *sc, uint32_t pin, uint32_t fn)
{
	bus_size_t mux_ctl_reg = IOMUX_PIN_TO_MUX_ADDRESS(pin);

	if (mux_ctl_reg != IOMUX_MUX_NONE)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    mux_ctl_reg, fn);
}

void
iomux_set_function(unsigned int pin, unsigned int fn)
{
	if (iomux_softc == NULL)
		return;

	iomux_set_function_sub(iomux_softc, pin, fn);
}

static void
iomux_set_pad_sub(struct imxiomux_softc *sc, uint32_t pin, uint32_t config)
{
	bus_size_t pad_ctl_reg = IOMUX_PIN_TO_PAD_ADDRESS(pin);

	if (pad_ctl_reg != IOMUX_PAD_NONE)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    pad_ctl_reg, config);
}

void
iomux_set_pad(unsigned int pin, unsigned int config)
{
	if (iomux_softc == NULL)
		return;

	iomux_set_pad_sub(iomux_softc, pin, config);
}

#if 0
void
iomux_set_input(unsigned int input, unsigned int config)
{
	bus_size_t input_ctl_reg = input;

	if (iomux_softc == NULL)
		return;

	bus_space_write_4(iomux_softc->sc_iot, iomux_softc->sc_ioh,
	    input_ctl_reg, config);
}
#endif

#if 0
void
iomux_mux_config(const struct iomux_conf *conflist)
{
	int i;

	if (iomux_softc == NULL)
		return;

	for (i = 0; conflist[i].pin != IOMUX_CONF_EOT; i++) {
		iomux_set_pad_sub(iomux_softc, conflist[i].pin, conflist[i].pad);
		iomux_set_function_sub(iomux_softc, conflist[i].pin,
		    conflist[i].mux);
	}
}

void
iomux_input_config(const struct iomux_input_conf *conflist)
{
	int i;

	if (iomux_softc == NULL)
		return;

	for (i = 0; conflist[i].inout != -1; i++) {
		iomux_set_inout(iomux_softc, conflist[i].inout,
		    conflist[i].inout_mode);
	}
}
#endif
