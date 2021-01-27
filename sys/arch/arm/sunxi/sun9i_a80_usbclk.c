/* $NetBSD: sun9i_a80_usbclk.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sun9i_a80_usbclk.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>

enum {
	CLK_BUS_HCI0 = 0,
	CLK_USB_OHCI0,
	CLK_BUS_HCI1,
	CLK_BUS_HCI2,
	CLK_USB_OHCI2,
	CLK_USB0_PHY,
	CLK_USB1_HSIC,
	CLK_USB1_PHY,
	CLK_USB2_HSIC,
	CLK_USB2_PHY,
	CLK_USB_HSIC
};

enum {
	RST_USB0_HCI = 0,
	RST_USB1_HCI,
	RST_USB2_HCI,
	RST_USB0_PHY,
	RST_USB1_HSIC,
	RST_USB1_PHY,
	RST_USB2_HSIC,
	RST_USB2_PHY
};

#define	HCI_SCR		0x00
#define	HCI_PCR		0x04

static int sun9i_a80_usbclk_match(device_t, cfdata_t, void *);
static void sun9i_a80_usbclk_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun9i-a80-usb-clks" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_a80_usbclk, sizeof(struct sunxi_ccu_softc),
	sun9i_a80_usbclk_match, sun9i_a80_usbclk_attach, NULL, NULL);

static struct sunxi_ccu_reset sun9i_a80_usbclk_resets[] = {
	SUNXI_CCU_RESET(RST_USB0_HCI, HCI_SCR, 17),
	SUNXI_CCU_RESET(RST_USB1_HCI, HCI_SCR, 18),
	SUNXI_CCU_RESET(RST_USB2_HCI, HCI_SCR, 19),
	SUNXI_CCU_RESET(RST_USB0_PHY, HCI_PCR, 17),

	SUNXI_CCU_RESET(RST_USB1_HSIC, HCI_PCR, 18),
	SUNXI_CCU_RESET(RST_USB1_PHY, HCI_PCR, 19),
	SUNXI_CCU_RESET(RST_USB2_HSIC, HCI_PCR, 20),
	SUNXI_CCU_RESET(RST_USB2_PHY, HCI_PCR, 21),
};

static struct sunxi_ccu_clk sun9i_a80_usbclk_clks[] = {
	SUNXI_CCU_GATE(CLK_BUS_HCI0, "bus-hci0", "bus", HCI_SCR, 1),
	SUNXI_CCU_GATE(CLK_USB_OHCI0, "usb-ohci0", "hosc", HCI_SCR, 2),
	SUNXI_CCU_GATE(CLK_BUS_HCI1, "bus-hci1", "bus", HCI_SCR, 3),
	SUNXI_CCU_GATE(CLK_BUS_HCI2, "bus-hci2", "bus", HCI_SCR, 5),
	SUNXI_CCU_GATE(CLK_USB_OHCI2, "usb-ohci2", "hosc", HCI_SCR, 6),

	SUNXI_CCU_GATE(CLK_USB0_PHY, "usb0-phy", "hosc", HCI_PCR, 1),
	SUNXI_CCU_GATE(CLK_USB1_HSIC, "usb1-hsic", "hosc", HCI_PCR, 2),
	SUNXI_CCU_GATE(CLK_USB1_PHY, "usb1-phy", "hosc", HCI_PCR, 3),
	SUNXI_CCU_GATE(CLK_USB2_HSIC, "usb2-hsic", "hosc", HCI_PCR, 4),
	SUNXI_CCU_GATE(CLK_USB2_PHY, "usb2-phy", "hosc", HCI_PCR, 5),
	SUNXI_CCU_GATE(CLK_USB_HSIC, "usb-hsic", "hosc", HCI_PCR, 10),
};

static int
sun9i_a80_usbclk_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun9i_a80_usbclk_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun9i_a80_usbclk_resets;
	sc->sc_nresets = __arraycount(sun9i_a80_usbclk_resets);

	sc->sc_clks = sun9i_a80_usbclk_clks;
	sc->sc_nclks = __arraycount(sun9i_a80_usbclk_clks);

	clk = fdtbus_clock_get(phandle, "bus");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": A80 USB HCI clocks\n");

	sunxi_ccu_print(sc);
}
