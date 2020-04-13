/*	$NetBSD: imx6_usbphy.c,v 1.1.6.2 2020/04/13 08:03:35 martin Exp $	*/

/*
 * Copyright (c) 2017  Genetec Corporation.  All rights reserved.
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

#include "locators.h"
#include "ohci.h"
#include "ehci.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: imx6_usbphy.c,v 1.1.6.2 2020/04/13 08:03:35 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>

#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_usbphyreg.h>

struct imx6_usbphy_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct clk *sc_clk;
};

static int	imx6_usbphy_match(device_t, cfdata_t, void *);
static void	imx6_usbphy_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxusbphy, sizeof(struct imx6_usbphy_softc),
    imx6_usbphy_match, imx6_usbphy_attach, NULL, NULL);

#define	USBPHY_READ(sc, reg)						      \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	USBPHY_WRITE(sc, reg, val)					      \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
imx6_usbphy_enable(device_t dev, bool enable)
{
	struct imx6_usbphy_softc * const sc = device_private(dev);

	/* USBPHY enable */
	USBPHY_WRITE(sc, USBPHY_CTRL, USBPHY_CTRL_CLKGATE);

	/* do reset */
	USBPHY_WRITE(sc, USBPHY_CTRL_SET, USBPHY_CTRL_SFTRST);
	delay(100);

	/* clear reset, and run clocks */
	USBPHY_WRITE(sc, USBPHY_CTRL_CLR, USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE);
	delay(100);

	/* power on */
	USBPHY_WRITE(sc, USBPHY_PWD, 0);

	/* UTMI+Level2, Level3 */
	USBPHY_WRITE(sc, USBPHY_CTRL_SET,
	    USBPHY_CTRL_ENUTMILEVEL2 | USBPHY_CTRL_ENUTMILEVEL3);

	return 0;
}

static int
imx6_usbphy_init_clocks(device_t dev)
{
	struct imx6_usbphy_softc * const sc = device_private(dev);

	int error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable: %d\n", error);
		return error;
	}

	return 0;
}

static int
imx6_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case IMX6_AIPS1_BASE + AIPS1_USBPHY1_BASE:
	case IMX6_AIPS1_BASE + AIPS1_USBPHY2_BASE:
		return 1;
	}

	return 0;
}


static void
imx6_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct imx6_usbphy_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_addr_t addr = aa->aa_addr;
	bus_size_t size = AIPS1_USBPHY_SIZE;
	int error;

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_iot;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d",
		    addr, error);
		return;
	}

	switch (device_unit(self)) {
	case 0:
		sc->sc_clk = imx6_get_clock("usbphy1");
		break;
	case 1:
		sc->sc_clk = imx6_get_clock("usbphy2");
		break;
	}

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	imx6_usbphy_init_clocks(self);
	imx6_usbphy_enable(self, true);
}
