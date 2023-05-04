/*	$NetBSD: imx6_usbphy.c,v 1.3 2023/05/04 13:29:33 bouyer Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(1, "$NetBSD: imx6_usbphy.c,v 1.3 2023/05/04 13:29:33 bouyer Exp $");

#include "opt_fdt.h"

#include "locators.h"
#include "ohci.h"
#include "ehci.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/nxp/imx6_usbphyreg.h>

#include <dev/fdt/fdtvar.h>

struct imx6_usbphy_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct clk *sc_clk;
};

static int imx6_usbphy_match(device_t, cfdata_t, void *);
static void imx6_usbphy_attach(device_t, device_t, void *);

static int imx6_usbphy_init_clocks(device_t);
static int imx6_usbphy_enable(device_t, void *, bool);

CFATTACH_DECL_NEW(imxusbphy, sizeof(struct imx6_usbphy_softc),
    imx6_usbphy_match, imx6_usbphy_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-usbphy" },
	{ .compat = "fsl,imx6sx-usbphy" },
	DEVICE_COMPAT_EOL
};

static int
imx6_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx6_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct imx6_usbphy_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get iomux registers\n");
		return;
	}

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr, error);
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = bst;
	sc->sc_ioh = bsh;

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	imx6_usbphy_init_clocks(self);
	imx6_usbphy_enable(self, NULL, true);
}

static int
imx6_usbphy_init_clocks(device_t dev)
{
	struct imx6_usbphy_softc * const sc = device_private(dev);
	int error;

	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable: %d\n", error);
		return error;
	}

	return 0;
}

#define	USBPHY_READ(sc, reg)						      \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define	USBPHY_WRITE(sc, reg, val)					      \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int
imx6_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct imx6_usbphy_softc * const sc = device_private(dev);

	/* USBPHY enable */
	USBPHY_WRITE(sc, USBPHY_CTRL, USBPHY_CTRL_CLKGATE);

	/* do reset */
	USBPHY_WRITE(sc, USBPHY_CTRL_SET, USBPHY_CTRL_SFTRST);
	delay(100);

	/* clear reset, and run clocks */
	USBPHY_WRITE(sc, USBPHY_CTRL_CLR,
	    USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE);
	delay(100);

	/* power on */
	USBPHY_WRITE(sc, USBPHY_PWD, 0);

	/* UTMI+Level2, Level3 */
	USBPHY_WRITE(sc, USBPHY_CTRL_SET,
	    USBPHY_CTRL_ENUTMILEVEL2 | USBPHY_CTRL_ENUTMILEVEL3);

	return 0;
}

