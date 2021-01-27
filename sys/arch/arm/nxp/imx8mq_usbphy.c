/* $NetBSD: imx8mq_usbphy.c,v 1.5 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: imx8mq_usbphy.c,v 1.5 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/fdt/fdtvar.h>

#define	PHY_CTL0_ADDR		0x00
#define	 REF_SSP_EN		__BIT(2)

#define	PHY_CTL1_ADDR		0x04
#define	 PHY_VDATDATENB0	__BIT(20)
#define	 PHY_VDATSRCENB0	__BIT(19)
#define	 PHY_ATERESET		__BIT(3)
#define	 PHY_COMMONONN		__BIT(1)
#define	 PHY_RESET		__BIT(0)

#define	PHY_CTL2_ADDR		0x08
#define	 PHY_TXENABLEN0		__BIT(8)

static int imx8mq_usbphy_match(device_t, cfdata_t, void *);
static void imx8mq_usbphy_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx8mq-usb-phy" },
	DEVICE_COMPAT_EOL
};

struct imx8mq_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
};

#define	PHY_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PHY_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(imx8mqusbphy, sizeof(struct imx8mq_usbphy_softc),
	imx8mq_usbphy_match, imx8mq_usbphy_attach, NULL, NULL);

static void *
imx8mq_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	if (len != 0)
		return NULL;

	return (void *)(uintptr_t)1;
}

static void
imx8mq_usbphy_release(device_t dev, void *priv)
{
}

static int
imx8mq_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct imx8mq_usbphy_softc * const sc = device_private(dev);
	struct fdtbus_regulator *reg;
	uint32_t val;
	int error;

	if (enable) {
		if (of_hasprop(sc->sc_phandle, "vbus-supply")) {
			reg = fdtbus_regulator_acquire(sc->sc_phandle, "vbus-supply");
			if (reg != NULL) {
				error = fdtbus_regulator_enable(reg);
				if (error != 0)
					device_printf(dev, "WARNING: couldn't enable vbus-supply: %d\n", error);
			} else {
				device_printf(dev, "WARNING: couldn't acquire vbus-supply\n");
			}
		}

		val = PHY_READ(sc, PHY_CTL1_ADDR);
		val &= ~PHY_VDATDATENB0;
		val &= ~PHY_VDATSRCENB0;
		val &= ~PHY_COMMONONN;
		val |= PHY_RESET;
		val |= PHY_ATERESET;
		PHY_WRITE(sc, PHY_CTL1_ADDR, val);

		val = PHY_READ(sc, PHY_CTL0_ADDR);
		val |= REF_SSP_EN;
		PHY_WRITE(sc, PHY_CTL0_ADDR, val);

		val = PHY_READ(sc, PHY_CTL2_ADDR);
		val |= PHY_TXENABLEN0;
		PHY_WRITE(sc, PHY_CTL2_ADDR, val);

		val = PHY_READ(sc, PHY_CTL1_ADDR);
		val &= ~PHY_RESET;
		val &= ~PHY_ATERESET;
		PHY_WRITE(sc, PHY_CTL1_ADDR, val);
	}

	return 0;
}

const struct fdtbus_phy_controller_func imx8mq_usbphy_funcs = {
	.acquire = imx8mq_usbphy_acquire,
	.release = imx8mq_usbphy_release,
	.enable = imx8mq_usbphy_enable,
};

static int
imx8mq_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx8mq_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct imx8mq_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = phandle;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Enable clocks */
	fdtbus_clock_assign(phandle);
	if (fdtbus_clock_enable(phandle, "phy", true) != 0) {
		aprint_error(": couldn't enable phy clock\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	fdtbus_register_phy_controller(self, phandle, &imx8mq_usbphy_funcs);
}
