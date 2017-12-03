/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: netwalker_usb.c,v 1.3.2.1 2017/12/03 11:36:06 jdolecek Exp $");

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

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imxusbreg.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imx51_iomuxreg.h>
#include <arm/imx/imxgpiovar.h>
#include "locators.h"


struct netwalker_usbc_softc {
	struct imxusbc_softc  sc_imxusbc;
};


static int	imxusbc_match(device_t, cfdata_t, void *);
static void	imxusbc_attach(device_t, device_t, void *);
static void	netwalker_usb_init(struct imxehci_softc *);

static void	init_otg(struct imxehci_softc *);
static void	init_h1(struct imxehci_softc *);

extern const struct iomux_conf iomux_usb1_config[];

/* attach structures */
CFATTACH_DECL_NEW(imxusbc_axi, sizeof(struct netwalker_usbc_softc),
    imxusbc_match, imxusbc_attach, NULL, NULL);

static int
imxusbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args *aa = aux;

	printf("%s\n", __func__);

	if (aa->aa_addr == USBOH3_BASE)
		return 1;
	return 0;
}

static void
imxusbc_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;
	struct imxusbc_softc *sc = device_private(self);

	sc->sc_init_md_hook = netwalker_usb_init;
	sc->sc_setup_md_hook = NULL;

	imxusbc_attach_common(parent, self, aa->aa_iot);

}

static void
netwalker_usb_init(struct imxehci_softc *sc)
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

	sc->sc_iftype = IMXUSBC_IF_UTMI;

	imxehci_reset(sc);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL0);
	reg |= PHYCTRL0_OTG_OVER_CUR_DIS;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_PHYCTRL0, reg);

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBOH3_USBCTRL);
	reg &= ~(USBCTRL_OWIR|USBCTRL_OPM);
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

	/* output HIGH to USBH1_STP */
	gpio_data_write(GPIO_NO(1, 27), 1);
	gpio_set_direction(GPIO_NO(1, 27), GPIO_DIR_OUT);

	iomux_mux_config(iomux_usb1_config);

	delay(100 * 1000);

	/* XXX enable USB clock */

	imxehci_reset(sc);

	/* select external clock for Host 1 */
	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh,
			       USBOH3_USBCTRL1);
	reg |= USBCTRL1_UH1_EXT_CLK_EN;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh,
			  USBOH3_USBCTRL1, reg);


	/* select ULPI interface for Host 1 */
	sc->sc_iftype = IMXUSBC_IF_ULPI;

	reg = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh,
			       USBOH3_USBCTRL);
	reg &= ~(USBCTRL_H1PM);
	reg |= USBCTRL_H1UIE|USBCTRL_H1WIE;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh,
			  USBOH3_USBCTRL, reg);

	iomux_set_function(MUX_PIN(USBH1_STP), IOMUX_CONFIG_ALT0);


	/* HUB RESET release */
	gpio_data_write(GPIO_NO(1, 7), 1);
	gpio_set_direction(GPIO_NO(1, 7), GPIO_DIR_OUT);

	/* Drive 26M_OSC_EN line high 3_1 */
	gpio_data_write(GPIO_NO(3, 1), 1);
	gpio_set_direction(GPIO_NO(3, 1), GPIO_DIR_OUT);

	/* Drive USB_CLK_EN_B line low  2_1 */
	gpio_data_write(GPIO_NO(2, 1), 0);
	gpio_set_direction(GPIO_NO(2, 1), GPIO_DIR_IN);

	/* MX51_PIN_EIM_D21 - De-assert USB PHY RESETB */
	delay(10 * 1000);
	gpio_data_write(GPIO_NO(2, 5), 1);
	gpio_set_direction(GPIO_NO(2, 5), GPIO_DIR_OUT);
	iomux_set_function(MUX_PIN(EIM_D21), IOMUX_CONFIG_ALT1);
	delay(5 * 1000);
}

/*
 * IOMUX setting for USB Host1
 * taken from Linux driver
 */
const struct iomux_conf iomux_usb1_config[] = {

	{
		/* Initially setup this pin for GPIO, and change to
		 * USBH1_STP later */
		.pin = MUX_PIN(USBH1_STP),
		.mux = IOMUX_CONFIG_ALT2,
		.pad = (PAD_CTL_SRE | PAD_CTL_DSE_HIGH |
		    PAD_CTL_KEEPER | PAD_CTL_HYS)
	},

	{
		/* Clock */
		.pin = MUX_PIN(USBH1_CLK),
		.mux = IOMUX_CONFIG_ALT0,
		.pad = (PAD_CTL_SRE | PAD_CTL_DSE_HIGH |
		    PAD_CTL_KEEPER | PAD_CTL_HYS)
	},
	{
		/* DIR */
		.pin = MUX_PIN(USBH1_DIR),
		.mux = IOMUX_CONFIG_ALT0,
		.pad = (PAD_CTL_SRE | PAD_CTL_DSE_HIGH |
		    PAD_CTL_KEEPER | PAD_CTL_HYS)
	},

	{
		/* NXT */
		.pin = MUX_PIN(USBH1_NXT),
		.mux = IOMUX_CONFIG_ALT0,
		.pad = (PAD_CTL_SRE | PAD_CTL_DSE_HIGH |
		    PAD_CTL_KEEPER | PAD_CTL_HYS)
	},

#define	USBH1_DATA_CONFIG(n)					\
	{							\
		/* DATA n */					\
		.pin = MUX_PIN(USBH1_DATA##n),			\
		.mux = IOMUX_CONFIG_ALT0,			\
		.pad = (PAD_CTL_SRE | PAD_CTL_DSE_HIGH |	\
		    PAD_CTL_KEEPER | PAD_CTL_PUS_100K_PU |	\
		    PAD_CTL_HYS),				\
		/* XXX: what does 100K_PU with KEEPER ? */	\
	}

	USBH1_DATA_CONFIG(0),
	USBH1_DATA_CONFIG(1),
	USBH1_DATA_CONFIG(2),
	USBH1_DATA_CONFIG(3),
	USBH1_DATA_CONFIG(4),
	USBH1_DATA_CONFIG(5),
	USBH1_DATA_CONFIG(6),
	USBH1_DATA_CONFIG(7),

	{
		/* USB_CLK_EN_B  GPIO2[1]*/
		.pin = MUX_PIN(EIM_D17),
		.mux = IOMUX_CONFIG_ALT1,
		.pad = (PAD_CTL_DSE_HIGH | PAD_CTL_PKE | PAD_CTL_SRE),
	},

	{
		/* USB PHY RESETB */
		.pin = MUX_PIN(EIM_D21),
		.mux = IOMUX_CONFIG_ALT1,
		.pad = (PAD_CTL_DSE_HIGH | PAD_CTL_KEEPER |
		    PAD_CTL_PUS_100K_PU | PAD_CTL_SRE)
	},
	{
		/* USB HUB RESET */
		.pin = MUX_PIN(GPIO1_7),
		.mux = IOMUX_CONFIG_ALT0,
		.pad = (PAD_CTL_DSE_HIGH | PAD_CTL_SRE),
	},
	{
		/* 26M_OSC pin settings */
		.pin = MUX_PIN(DI1_PIN12),
		.mux = IOMUX_CONFIG_ALT4,
		.pad = (PAD_CTL_DSE_HIGH | PAD_CTL_KEEPER |
		    PAD_CTL_SRE),
	},

	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};
