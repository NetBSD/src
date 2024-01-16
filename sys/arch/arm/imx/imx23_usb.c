/* $Id: imx23_usb.c,v 1.5.6.2 2024/01/16 08:25:48 martin Exp $ */

/*
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/errno.h>

#include <arm/pic/picvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imxusbreg.h>

#include <arm/mainbus/mainbus.h>

#include <arm/imx/imx23_clkctrlvar.h>
#include <arm/imx/imx23_digctlvar.h>
#include <arm/imx/imx23_pinctrlvar.h>
#include <arm/imx/imx23var.h>

#include "locators.h"

struct imx23_usb_softc {
	struct imxusbc_softc  sc_imxusbc; /* Must be first */
};

static int	imx23_usb_match(device_t, cfdata_t, void *);
static void	imx23_usb_attach(device_t, device_t, void *);
static int	imx23_usb_activate(device_t, enum devact);

static int      imxusbc_search(device_t, cfdata_t, const int *, void *);
static void	imx23_usb_init(struct imxehci_softc *, uintptr_t);

CFATTACH_DECL3_NEW(imxusbc,
        sizeof(struct imx23_usb_softc),
        imx23_usb_match,
        imx23_usb_attach,
        NULL,
        imx23_usb_activate,
        NULL,
        NULL,
        0
);

#define AHB_USB		0x80080000
#define AHB_USB_SIZE	0x40000

static int
imx23_usb_match(device_t parent, cfdata_t match, void *aux)
{
	struct ahb_attach_args *aa = aux;

	if ((aa->aa_addr == AHB_USB) && (aa->aa_size == AHB_USB_SIZE))
		return 1;

	return 0;
}

static void
imx23_usb_attach(device_t parent, device_t self, void *aux)
{
	struct imxusbc_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_iot = &imx23_bus_space;
	sc->sc_ehci_size = IMXUSB_EHCI_SIZE;
	sc->sc_ehci_offset = IMXUSB_EHCI_SIZE;

	sc->sc_init_md_hook = imx23_usb_init;
	sc->sc_intr_establish_md_hook = NULL;
	sc->sc_setup_md_hook = NULL;

	if (bus_space_map(sc->sc_iot, AHB_USB, AHB_USB_SIZE, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "Unable to map bus space");
		return;
	}

	/* Enable PLL outputs for USB PHY. */
	clkctrl_en_usb();

	/* Enable external USB chip. */
	imx23_pinctrl_en_usb();

	/* USB clock on. */
	digctl_usb_clkgate(0);

	aprint_normal("\n");

	/* attach OTG/EHCI host controllers */
	config_search(self, NULL,
	    CFARGS(.search = imxusbc_search));

	return;
}

static int
imx23_usb_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

static int
imxusbc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
        struct imxusbc_softc *sc = device_private(parent);
        struct imxusbc_attach_args aa;

        aa.aa_iot = sc->sc_iot;
        aa.aa_ioh = sc->sc_ioh;
        aa.aa_dmat = &imx23_bus_dma_tag;
        aa.aa_unit = cf->cf_loc[IMXUSBCCF_UNIT];
        aa.aa_irq = cf->cf_loc[IMXUSBCCF_IRQ];

        if (config_probe(parent, cf, &aa))
                config_attach(parent, cf, &aa, NULL, CFARGS_NONE);

        return 0;
}

static
void imx23_usb_init(struct imxehci_softc *sc, uintptr_t data)
{

	sc->sc_iftype = IMXUSBC_IF_UTMI;

	return;
}
