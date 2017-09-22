/*	$NetBSD: kobo_usb.c,v 1.2 2017/09/22 15:37:13 khorben Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kobo_usb.c,v 1.2 2017/09/22 15:37:13 khorben Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imxusbreg.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imx50_iomuxreg.h>
#include <arm/imx/imxgpiovar.h>
#include <arm/imx/imx51_ccmreg.h>
#include <arm/imx/imx51_ccmvar.h>

#include "locators.h"

struct kobo_usbc_softc {
	struct imxusbc_softc sc_imxusbc;
};

static int	imxusbc_match(device_t, cfdata_t, void *);
static void	imxusbc_attach(device_t, device_t, void *);
static void	kobo_usb_init(struct imxehci_softc *);

static void	init_otg(struct imxehci_softc *);
static void	init_h1(struct imxehci_softc *);

extern const struct iomux_conf iomux_usb1_config[];

/* attach structures */
CFATTACH_DECL_NEW(imxusbc_axi, sizeof(struct kobo_usbc_softc),
    imxusbc_match, imxusbc_attach, NULL, NULL);

static int
imxusbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (aa->aa_addr == USBOH3_BASE)
		return 1;
	return 0;
}

static void
imxusbc_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;
	struct kobo_usbc_softc *sc = device_private(self);

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_imxusbc.sc_init_md_hook = kobo_usb_init;
	sc->sc_imxusbc.sc_setup_md_hook = NULL;

	imxusbc_attach_common(parent, self, aa->aa_iot);
}

static void
kobo_usb_init(struct imxehci_softc *sc)
{
	switch (sc->sc_unit) {
	case 0:	/* OTG controller */
		init_otg(sc);
		break;
	case 1:	/* EHCI Host 1 */
		init_h1(sc);
		break;
	default:
		aprint_error_dev(sc->sc_hsc.sc_dev, "unit %d not supported\n",
		    sc->sc_unit);
	}
}

static void
init_otg(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t reg;

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USB_CLKONOFF_CTRL);
	reg &= ~USB_CLKONOFF_CTRL_OTG_AHBCLK_OFF;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USB_CLKONOFF_CTRL, reg);

	sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;

	imxehci_reset(sc);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL0);
	reg |= PHYCTRL0_OTG_OVER_CUR_DIS | PHYCTRL0_SUSPENDM;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL0, reg);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USBCTRL);
	reg &= ~USBCTRL_OWIR;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USBCTRL, reg);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL1);
	reg = (reg & ~PHYCTRL1_PLLDIVVALUE_MASK) | PHYCTRL1_PLLDIVVALUE_24MHZ;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL1, reg);
}

static void
init_h1(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t reg;

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USB_CLKONOFF_CTRL);
	reg &= ~USB_CLKONOFF_CTRL_H1_AHBCLK_OFF;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USB_CLKONOFF_CTRL, reg);

	imxehci_reset(sc);

	/* select INTERNAL PHY interface for Host 1 */
	sc->sc_iftype = IMXUSBC_IF_UTMI;

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh,
	    USBOH3_USBCTRL);
	reg |= USBCTRL_H1PM;
	reg &= ~(USBCTRL_H1WIE);
	reg &= ~(USBCTRL_H1UIE);
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh,
	    USBOH3_USBCTRL, reg);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL0);
	reg &= ~PHYCTRL0_H1_OVER_CUR_DIS;
	reg |= PHYCTRL0_H1_OVER_CUR_POL;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh,USBOH3_PHYCTRL0 , reg);

	delay(1000);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_UH1_PHY_CTRL_1);
	reg &= ~PHYCTRL1_PLLDIVVALUE_MASK;
	reg |= PHYCTRL1_PLLDIVVALUE_24MHZ;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_UH1_PHY_CTRL_1, reg);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_UH1_PHY_CTRL_0);
	reg &= ~PHYCTRL0_CHGRDETON;
	reg &= ~PHYCTRL0_CHGRDETEN;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_UH1_PHY_CTRL_0, reg);
}


