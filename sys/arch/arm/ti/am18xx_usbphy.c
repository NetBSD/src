/* $NetBSD $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

/*
 * Driver for the USB PHYs (USB1.1 and 2.0) on the TI AM18XX family of SoCs.
 *
 * This driver only manages suspend/resume bits, but not the 48MHz PLL.
 * All PLL related logic is in the phy clock driver.
 *
 * Locking strategy: all hardware accesses should happen with sc->sc_lock held.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/clk/clk.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

struct am18xx_usbphy {
	struct clk *phy_clk48;		/* must be first */
	uint32_t phy_suspend_mask;	/* which bits to change to suspend */
	uint32_t phy_suspend_bits;	/* the values of these bits */
};

struct am18xx_usbphy_softc {
	struct am18xx_usbphy sc_usb11_phy;
	struct am18xx_usbphy sc_usb20_phy;
	kmutex_t sc_lock;
	struct syscon *sc_syscon;
};

static int am18xx_usbphy_match(device_t, cfdata_t, void *);
static void am18xx_usbphy_attach(device_t, device_t, void *);
static void *am18xx_usbphy_acquire(device_t, const void *, size_t);
static void am18xx_usbphy_release(device_t, void *);
static int am18xx_usbphy_enable(device_t, void *, bool);

CFATTACH_DECL_NEW(am18xxusbphy, sizeof(struct am18xx_usbphy_softc),
    am18xx_usbphy_match, am18xx_usbphy_attach, NULL, NULL);

#define AM18XX_CFGCHIP2 0x8
#define AM18XX_CFGCHIP2_USB11SUSPENDM __BIT(7)
#define AM18XX_CFGCHIP2_USB20OTGPWDN __BIT(9)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da830-usb-phy"},
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_phy_controller_func am18xx_usbphy_funcs = {
	.acquire = &am18xx_usbphy_acquire,
	.release = &am18xx_usbphy_release,
	.enable = &am18xx_usbphy_enable,
};

static void *
am18xx_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct am18xx_usbphy_softc *const sc = device_private(dev);
	const u_int *cells = data;

	if (len != 4)
		return NULL;

	const u_int index = be32toh(cells[0]);

	if (index == 0) {
		return &sc->sc_usb20_phy;
	} else if (index == 1) {
		return &sc->sc_usb11_phy;
	}

	return NULL;
}

static void
am18xx_usbphy_release(device_t dev, void *priv)
{
	/* do nothing */
}

static int
am18xx_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct am18xx_usbphy_softc *sc = device_private(dev);
	struct am18xx_usbphy *phy = (struct am18xx_usbphy *)priv;
	int error;
	uint32_t val;

	mutex_enter(&sc->sc_lock);

	if (enable) {
		/* enable phy clock */
		error = clk_enable(phy->phy_clk48);
		if (error) {
			goto cleanup;
		}

		/* remove phy from suspended state */
		syscon_lock(sc->sc_syscon);
		val = syscon_read_4(sc->sc_syscon, AM18XX_CFGCHIP2);

		/* clear suspend bits */
		val &= (~phy->phy_suspend_mask);

		/* so we can set the to leave suspend */
		val |= phy->phy_suspend_mask & (~phy->phy_suspend_bits);

		/* and force them into effect */
		syscon_write_4(sc->sc_syscon, AM18XX_CFGCHIP2, val);
		syscon_unlock(sc->sc_syscon);
	} else {
		/* put phy into suspended state */

		syscon_lock(sc->sc_syscon);
		val = syscon_read_4(sc->sc_syscon, AM18XX_CFGCHIP2);

		/* clear suspend bits */
		val &= (~phy->phy_suspend_mask);

		/* so we can set the to enter suspend */
		val |= phy->phy_suspend_bits;

		/* and force them into effect */
		syscon_write_4(sc->sc_syscon, AM18XX_CFGCHIP2, val);

		syscon_unlock(sc->sc_syscon);

		/* disable phy clock */
		error = clk_disable(phy->phy_clk48);
		if (error) {
			goto cleanup;
		}
	}

cleanup:
	mutex_exit(&sc->sc_lock);

	return error;
}

int
am18xx_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_usbphy_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_usb11_phy.phy_suspend_mask = AM18XX_CFGCHIP2_USB11SUSPENDM;
	sc->sc_usb11_phy.phy_suspend_bits = 0;

	sc->sc_usb20_phy.phy_suspend_mask = AM18XX_CFGCHIP2_USB20OTGPWDN;
	sc->sc_usb20_phy.phy_suspend_bits = AM18XX_CFGCHIP2_USB20OTGPWDN;

	/* usb 2.0 clock */
	sc->sc_usb20_phy.phy_clk48 = fdtbus_clock_get(phandle, "usb0_clk48");
	if (sc->sc_usb20_phy.phy_clk48 == NULL) {
		aprint_error(": failed to get usb 2.0 phy clk\n");
		return;
	}

	/* usb 1.1 clock */
	sc->sc_usb11_phy.phy_clk48 = fdtbus_clock_get(phandle, "usb1_clk48");
	if (sc->sc_usb11_phy.phy_clk48 == NULL) {
		aprint_error(": failed to get usb 1.1 phy clk\n");
		return;
	}

	/* cfgchip syscon */
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	/* register phy */
	error = fdtbus_register_phy_controller(self, phandle,
	    &am18xx_usbphy_funcs);
	if (error) {
		aprint_error(": failed to register usb phy\n");
		return;
	}

	aprint_normal("\n");
}
