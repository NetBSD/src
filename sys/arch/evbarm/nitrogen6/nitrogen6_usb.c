/*	$NetBSD: nitrogen6_usb.c,v 1.3.4.1 2018/06/25 07:25:41 pgoyette Exp $	*/

/*
 * Copyright (c) 2013  Genetec Corporation.  All rights reserved.
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
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nitrogen6_usb.c,v 1.3.4.1 2018/06/25 07:25:41 pgoyette Exp $");

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
#include <arm/imx/imx6_usbreg.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imxusbreg.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imxgpiovar.h>
#include "locators.h"

struct nitrogen6_usbc_softc {
	struct imxusbc_softc sc_imxusbc;
};

static int nitrogen6_usbc_match(device_t, cfdata_t, void *);
static void nitrogen6_usbc_attach(device_t, device_t, void *);
static void nitrogen6_usb_init(struct imxehci_softc *);

static void init_otg(struct imxehci_softc *);
static void init_h1(struct imxehci_softc *);

/* attach structures */
CFATTACH_DECL_NEW(imxusbc_axi, sizeof(struct nitrogen6_usbc_softc),
    nitrogen6_usbc_match, nitrogen6_usbc_attach, NULL, NULL);

static int
nitrogen6_usbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (aa->aa_addr == IMX6_AIPS2_BASE + AIPS2_USBOH_BASE)
		return 1;

	return 0;
}

static void
nitrogen6_usbc_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;
	struct imxusbc_softc *sc = device_private(self);

	sc->sc_init_md_hook = nitrogen6_usb_init;
	sc->sc_setup_md_hook = NULL;

	aprint_naive("\n");
	aprint_normal(": Universal Serial Bus Controller\n");

	imxusbc_attach_common(parent, self, aa->aa_iot);
}

static void
nitrogen6_usb_init(struct imxehci_softc *sc)
{
	switch (sc->sc_unit) {
	case 0:	/* OTG controller */
		init_otg(sc);
		break;
	case 1:	/* EHCI Host 1 */
		init_h1(sc);
		break;
	case 2:	/* EHCI Host 2 */
	case 3:	/* EHCI Host 3 */
	default:
		aprint_error_dev(sc->sc_hsc.sc_dev, "unit %d not supported\n",
		    sc->sc_unit);
	}
}

static void
init_otg(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t v;

	sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;

	/* USB1 power */
	imx6_ccm_analog_write(USB_ANALOG_USB1_CHRG_DETECT,
	    USB_ANALOG_USB_CHRG_DETECT_EN_B |
	    USB_ANALOG_USB_CHRG_DETECT_CHK_CHRG_B);
	imx6_pll_power(CCM_ANALOG_PLL_USB1, 1, CCM_ANALOG_PLL_ENABLE);
	imx6_ccm_analog_write(CCM_ANALOG_PLL_USB1_CLR,
	    CCM_ANALOG_PLL_BYPASS);
	imx6_ccm_analog_write(CCM_ANALOG_PLL_USB1_SET,
	   CCM_ANALOG_PLL_ENABLE |
	   CCM_ANALOG_PLL_POWER |
	   CCM_ANALOG_PLL_EN_USB_CLK);

	imxehci_reset(sc);

	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBNC_USB_OTG_CTRL);
	v |= USBNC_USB_OTG_CTRL_WKUP_VBUS_EN;
	v |= USBNC_USB_OTG_CTRL_OVER_CUR_DIS;
	v |= USBNC_USB_OTG_CTRL_PWR_POL;
	v &= ~USBNC_USB_OTG_CTRL_UTMI_ON_CLOCK;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBNC_USB_OTG_CTRL, v);
}

static void
init_h1(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t v;

	sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;

	imx6_ccm_analog_write(USB_ANALOG_USB2_CHRG_DETECT,
	    USB_ANALOG_USB_CHRG_DETECT_EN_B |
	    USB_ANALOG_USB_CHRG_DETECT_CHK_CHRG_B);
	imx6_ccm_analog_write(CCM_ANALOG_PLL_USB2_CLR,
	    CCM_ANALOG_PLL_BYPASS);
	imx6_ccm_analog_write(CCM_ANALOG_PLL_USB2_SET,
	    CCM_ANALOG_PLL_ENABLE |
	    CCM_ANALOG_PLL_POWER |
	    CCM_ANALOG_PLL_EN_USB_CLK);

	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBNC_USB_UH1_CTRL);
	v |= USBNC_USB_UH1_CTRL_OVER_CUR_POL;
	v |= USBNC_USB_UH1_CTRL_OVER_CUR_DIS;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBNC_USB_UH1_CTRL, v);

	/* do reset */
	imxehci_reset(sc);

	/* set mode */
	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBC_UH1_USBMODE);
	v &= ~__SHIFTIN(USBC_UH_USBMODE_CM, 3);
	v |= __SHIFTIN(USBC_UH_USBMODE_CM, USBC_UH_USBMODE_CM_HOST_CONTROLLER);
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBC_UH1_USBMODE, v);
}
