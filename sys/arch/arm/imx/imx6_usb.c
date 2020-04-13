/*	$NetBSD: imx6_usb.c,v 1.4.2.1 2020/04/13 08:03:35 martin Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_usb.c,v 1.4.2.1 2020/04/13 08:03:35 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_usbreg.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imxusbreg.h>

#include "locators.h"

static int imxusbc_search(device_t, cfdata_t, const int *, void *);
static int imxusbc_print(void *, const char *);
static int imxusbc_init_clocks(struct imxusbc_softc *);

int
imxusbc_attach_common(device_t parent, device_t self, bus_space_tag_t iot,
    bus_addr_t addr, bus_size_t size)
{
	struct imxusbc_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_iot = iot;
	sc->sc_ehci_size = IMXUSB_EHCI_SIZE;
	sc->sc_ehci_offset = IMXUSB_EHCI_SIZE;

	/* Map entire USBOH registers.  Host controller drivers
	 * re-use subregions of this. */
	if (bus_space_map(iot, addr, size, 0, &sc->sc_ioh))
		return -1;
	if (bus_space_subregion(iot, sc->sc_ioh, USBNC_BASE, USBNC_SIZE, &sc->sc_ioh_usbnc))
		return -1;

	sc->sc_clk = imx6_get_clock("usboh3");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock usboh3\n");
		return -1;
	}
	if (imxusbc_init_clocks(sc) != 0) {
		aprint_error_dev(self, "couldn't init clocks\n");
		return -1;
	}

	/* attach OTG/EHCI host controllers */
	config_search_ia(imxusbc_search, self, "imxusbc", NULL);

	return 0;
}

static int
imxusbc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct imxusbc_softc *sc;
	struct imxusbc_attach_args aa;

	sc = device_private(parent);
	aa.aa_iot = sc->sc_iot;
	aa.aa_ioh = sc->sc_ioh;
	aa.aa_dmat = &arm_generic_dma_tag;
	aa.aa_unit = cf->cf_loc[IMXUSBCCF_UNIT];
	aa.aa_irq = cf->cf_loc[IMXUSBCCF_IRQ];

	if (config_match(parent, cf, &aa) > 0)
		config_attach(parent, cf, &aa, imxusbc_print);

	return 0;
}

/* ARGSUSED */
static int
imxusbc_print(void *aux, const char *name __unused)
{
	struct imxusbc_attach_args *iaa;

	iaa = (struct imxusbc_attach_args *)aux;

	aprint_normal(" unit %d intr %d", iaa->aa_unit, iaa->aa_irq);
	return UNCONF;
}

static int
imxusbc_init_clocks(struct imxusbc_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable usboh3: %d\n", error);
		return error;
	}

	return 0;
}

