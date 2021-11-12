/* $NetBSD: rk3288_usb.c,v 1.1 2021/11/12 22:02:08 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: rk3288_usb.c,v 1.1 2021/11/12 22:02:08 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#define	GRF_UOCn_CON0_SIDDQ	__BIT(13)

static int rk3288_usb_match(device_t, cfdata_t, void *);
static void rk3288_usb_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-usb-phy" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(rk3288_usb, 0,
	rk3288_usb_match, rk3288_usb_attach, NULL, NULL);

static int
rk3288_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk3288_usb_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int child;

	aprint_naive("\n");
	aprint_normal(": USB PHY\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = fdtbus_get_string(child, "name");
		cfaa.faa_quiet = false;

		config_found(self, &cfaa, NULL, CFARGS_NONE);
	}
}

/*
 * USB PHY
 */

static int rk3288_usbphy_match(device_t, cfdata_t, void *);
static void rk3288_usbphy_attach(device_t, device_t, void *);

struct rk3288_usbphy_softc {
	device_t	sc_dev;
	int		sc_phandle;
	struct syscon	*sc_syscon;
	bus_addr_t	sc_reg;
};

CFATTACH_DECL_NEW(rk3288_usbphy, sizeof(struct rk3288_usbphy_softc),
	rk3288_usbphy_match, rk3288_usbphy_attach, NULL, NULL);

static void *
rk3288_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct rk3288_usbphy_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return sc;
}

static void
rk3288_usbphy_release(device_t dev, void *priv)
{
}

static int
rk3288_usbphy_enable(device_t dev, void *priv, bool enable)
{
	struct rk3288_usbphy_softc * const sc = device_private(dev);
	uint32_t write_mask, write_val;

	write_mask = GRF_UOCn_CON0_SIDDQ << 16;
	write_val = enable ? 0 : GRF_UOCn_CON0_SIDDQ;

	syscon_lock(sc->sc_syscon);
	syscon_write_4(sc->sc_syscon, sc->sc_reg, write_mask | write_val);
	syscon_unlock(sc->sc_syscon);

	return 0;
}

const struct fdtbus_phy_controller_func rk3288_usbphy_funcs = {
	.acquire = rk3288_usbphy_acquire,
	.release = rk3288_usbphy_release,
	.enable = rk3288_usbphy_enable,
};

static int
rk3288_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
rk3288_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct rk3288_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(OF_parent(phandle)));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't find syscon\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 0, &sc->sc_reg, NULL) != 0) {
		aprint_error(": couldn't find registers\n");
		return;
	}

	rst = fdtbus_reset_get(phandle, "phy-reset");
	if (rst == NULL || fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}
	if (fdtbus_clock_enable(phandle, "phyclk", true) != 0) {
		aprint_error(": couildn't enable clock\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": USB port\n");

	fdtbus_register_phy_controller(self, phandle, &rk3288_usbphy_funcs);
}
