/*	$NetBSD: imx7_usb.c,v 1.3 2018/03/17 18:34:09 ryo Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx7_usb.c,v 1.3 2018/03/17 18:34:09 ryo Exp $");

#include "locators.h"

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

#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7var.h>
#include <arm/imx/imx7_usbreg.h>
#include <arm/imx/imx7_ccmreg.h>
#include <arm/imx/imx7_ccmvar.h>
#include <arm/imx/imx7_iomuxreg.h>
#include <arm/imx/imx7_srcreg.h>
#include <arm/imx/imxusbreg.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imxgpiovar.h>

static int imx7_usbc_match(device_t, cfdata_t, void *);
static void imx7_usbc_attach(device_t, device_t, void *);
static void imx7_usb_init(struct imxehci_softc *);
static int imx7_usbc_search(device_t, cfdata_t, const int *, void *);
static int imx7_usbc_print(void *, const char *);

/* attach structures */
CFATTACH_DECL_NEW(imxusbc_axi, sizeof(struct imxusbc_softc),
    imx7_usbc_match, imx7_usbc_attach, NULL, NULL);

/* ARGSUSED */
static int
imx7_usbc_match(device_t parent __unused, cfdata_t cf __unused, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS3_USB_BASE):
		return 1;
	}

	return 0;
}

/* ARGSUSED */
static int
imx7_usbc_search(device_t parent, cfdata_t cf __unused,
                 const int *ldesc __unused, void *aux __unused)
{
	struct imxusbc_softc *sc;
	struct imxusbc_attach_args aa;

	sc = device_private(parent);
	aa.aa_iot = sc->sc_iot;
	aa.aa_ioh = sc->sc_ioh;
	aa.aa_dmat = &arm_generic_dma_tag;;
	aa.aa_unit = cf->cf_loc[IMXUSBCCF_UNIT];
	aa.aa_irq = cf->cf_loc[IMXUSBCCF_IRQ];

	if (config_match(parent, cf, &aa) > 0)
		config_attach(parent, cf, &aa, imx7_usbc_print);

	return 0;
}

/* ARGSUSED */
static int
imx7_usbc_print(void *aux, const char *name __unused)
{
	struct imxusbc_attach_args *iaa;

	iaa = (struct imxusbc_attach_args *)aux;

	aprint_normal(" unit %d intr %d", iaa->aa_unit, iaa->aa_irq);
	return UNCONF;
}

/* ARGSUSED */
static void
imx7_usbc_attach(device_t parent __unused, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;
	struct imxusbc_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_init_md_hook = imx7_usb_init;
	sc->sc_setup_md_hook = NULL;
	sc->sc_ehci_size = IMX7_USBC_EHCISIZE;	/* ehci register stride */

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = IMX7_USBC_SIZE;

	aprint_naive("\n");
	aprint_normal(": Universal Serial Bus Controller\n");

	/* Map entire USBOH registers.  Host controller drivers
	 * re-use subregions of this. */
	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "cannot map registers\n");
		return;
	}

	/* disable clock */
	imx7_ccm_write(CCM_CCGR_USB_HSIC_CLR, CCM_CCGR_MASK);

	/* set freq */
	imx7_ccm_write(CCM_CLKROOT_USB_HSIC_CLK_ROOT, 
	    CCM_TARGET_ROOT_ENABLE |
	    __SHIFTIN(1, CCM_TARGET_ROOT_MUX) |
	    __SHIFTIN(0, CCM_TARGET_ROOT_PRE_PODF) |
	    __SHIFTIN(0, CCM_TARGET_ROOT_POST_PODF));
	imx7_ccm_write(CCM_CLKROOT_USB_HSIC_CLK_ROOT_POST,
	    __SHIFTIN(0, CCM_POST_POST_PODF));
	imx7_ccm_write(CCM_CLKROOT_USB_HSIC_CLK_ROOT_PRE,
	    CCM_PRE_EN_A | __SHIFTIN(1, CCM_PRE_PRE_PODF_A));

	/* enable clock */
	imx7_ccm_write(CCM_CCGR_USB_CTRL_SET, CCM_CCGR_DOMAIN0_ALL);
	imx7_ccm_write(CCM_CCGR_USB_HSIC_SET, CCM_CCGR_DOMAIN0_ALL);
	imx7_ccm_write(CCM_CCGR_USB_PHY1_SET, CCM_CCGR_DOMAIN0_ALL);
	imx7_ccm_write(CCM_CCGR_USB_PHY2_SET, CCM_CCGR_DOMAIN0_ALL);

	/* attach OTG/EHCI host controllers */
	config_search_ia(imx7_usbc_search, self, "imxusbc", NULL);
}

#define EHCIREG_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, reg)
#define EHCIREG_WRITE(sc, reg, value)	\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, reg, value)

static void
imx7_usb_init(struct imxehci_softc *sc)
{
	prop_dictionary_t dict;
	struct imxusbc_softc *usbc;
	const char *iftype;
	uint32_t v;

	usbc = sc->sc_usbc;

	/* setup overcurrent */
	v = EHCIREG_READ(sc, USBNC_N_CTRL1);
	v |= USBNC_N_CTRL1_OVER_CUR_POL;
	EHCIREG_WRITE(sc, USBNC_N_CTRL1, v);

	v = EHCIREG_READ(sc, USBNC_N_CTRL1);
	v |= USBNC_N_CTRL1_OVER_CUR_DIS | USBNC_N_CTRL1_PWR_POL;
	EHCIREG_WRITE(sc, USBNC_N_CTRL1, v);

	/* set host mode */
	v = EHCIREG_READ(sc, USB_X_USBMODE);
	v &= ~USB_X_USBMODE_CM;
	v |= __SHIFTIN(3, USB_X_USBMODE_CM);
	EHCIREG_WRITE(sc, USB_X_USBMODE, v);

	/* enable port */
	v = EHCIREG_READ(sc, USB_X_PORTSC1);
	v |= USB_X_PORTSC1_PE;
	EHCIREG_WRITE(sc, USB_X_PORTSC1, v);

	iftype = NULL;
	dict = device_properties(usbc->sc_dev);
	switch (sc->sc_unit) {
	case 0:	/* OTG1 */
		prop_dictionary_get_cstring_nocopy(dict, "otg1-iftype",
		    &iftype);
		break;
	case 1:	/* OTG2 */
		prop_dictionary_get_cstring_nocopy(dict, "otg2-iftype",
		    &iftype);
		break;
	case 2:	/* HSIC */
		prop_dictionary_get_cstring_nocopy(dict, "hsic-iftype",
		    &iftype);
		break;
	default:
		aprint_error_dev(sc->sc_hsc.sc_dev, "unit %d not supported\n",
		    sc->sc_unit);
	}

	if ((iftype == NULL) || (strcmp(iftype, "UTMI_WIDE") == 0))
		sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;
	else if (strcmp(iftype, "UTMI") == 0)
		sc->sc_iftype = IMXUSBC_IF_UTMI;
	else if (strcmp(iftype, "ULPI") == 0)
		sc->sc_iftype = IMXUSBC_IF_ULPI;
	else if (strcmp(iftype, "SERIAL") == 0)
		sc->sc_iftype = IMXUSBC_IF_SERIAL;
	else if (strcmp(iftype, "PHILIPS") == 0)
		sc->sc_iftype = IMXUSBC_IF_PHILIPS;
	else if (strcmp(iftype, "HSIC") == 0)
		sc->sc_iftype = IMXUSBC_IF_HSIC;
	else
		sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;


	imxehci_reset(sc);
}
