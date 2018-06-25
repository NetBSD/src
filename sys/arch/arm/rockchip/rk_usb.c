/* $NetBSD: rk_usb.c,v 1.2.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: rk_usb.c,v 1.2.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int rk_usb_match(device_t, cfdata_t, void *);
static void rk_usb_attach(device_t, device_t, void *);

#define	CON0_REG	0x00
#define	CON1_REG	0x04
#define	CON2_REG	0x08
#define	 USBPHY_COMMONONN	__BIT(4)

enum rk_usb_type {
	USB_RK3328 = 1,
};

static const struct of_compat_data compat_data[] = {
	{ "rockchip,rk3328-usb2phy",		USB_RK3328 },
	{ NULL }
};

struct rk_usb_clk {
	struct clk		base;
	bus_size_t		reg;
};

struct rk_usb_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	enum rk_usb_type	sc_type;

	struct clk_domain	sc_clkdom;
	struct rk_usb_clk	sc_usbclk;
};

#define USB_READ(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define USB_WRITE(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(rk_usb, sizeof(struct rk_usb_softc),
	rk_usb_match, rk_usb_attach, NULL, NULL);

static struct clk *
rk_usb_clk_get(void *priv, const char *name)
{
	struct rk_usb_softc * const sc = priv;

	if (strcmp(name, sc->sc_usbclk.base.name) != 0)
		return NULL;

	return &sc->sc_usbclk.base;
}

static void
rk_usb_clk_put(void *priv, struct clk *clk)
{
}

static u_int
rk_usb_clk_get_rate(void *priv, struct clk *clk)
{
	return 480000000;
}

static int
rk_usb_clk_enable(void *priv, struct clk *clk)
{
	struct rk_usb_softc * const sc = priv;

	const uint32_t write_mask = USBPHY_COMMONONN << 16;
	const uint32_t write_val = 0;
	USB_WRITE(sc, CON2_REG, write_mask | write_val);

	return 0;
}

static int
rk_usb_clk_disable(void *priv, struct clk *clk)
{
	struct rk_usb_softc * const sc = priv;

	const uint32_t write_mask = USBPHY_COMMONONN << 16;
	const uint32_t write_val = USBPHY_COMMONONN;
	USB_WRITE(sc, CON2_REG, write_mask | write_val);

	return 0;
}

static const struct clk_funcs rk_usb_clk_funcs = {
	.get = rk_usb_clk_get,
	.put = rk_usb_clk_put,
	.get_rate = rk_usb_clk_get_rate,
	.enable = rk_usb_clk_enable,
	.disable = rk_usb_clk_disable,
};

static struct clk *
rk_usb_fdt_decode(device_t dev, const void *data, size_t len)
{
	struct rk_usb_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return &sc->sc_usbclk.base;
}

static const struct fdtbus_clock_controller_func rk_usb_fdt_funcs = {
	.decode = rk_usb_fdt_decode
};

static int
rk_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
rk_usb_attach(device_t parent, device_t self, void *aux)
{
	struct rk_usb_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t grf_addr, phy_addr, phy_size;
	int child;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;

	if (fdtbus_get_reg(OF_parent(phandle), 0, &grf_addr, NULL) != 0) {
		aprint_error(": couldn't get grf registers\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 0, &phy_addr, &phy_size) != 0) {
		aprint_error(": couldn't get phy registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, grf_addr + phy_addr, phy_size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map phy registers\n");
		return;
	}

	const char *clkname = fdtbus_get_string(phandle, "clock-output-names");
	if (clkname == NULL)
		clkname = faa->faa_name;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &rk_usb_clk_funcs;
	sc->sc_clkdom.priv = sc;
	sc->sc_usbclk.base.domain = &sc->sc_clkdom;
	sc->sc_usbclk.base.name = kmem_asprintf("%s", clkname);
	clk_attach(&sc->sc_usbclk.base);

	aprint_naive("\n");
	aprint_normal(": USB2 PHY\n");

	fdtbus_register_clock_controller(self, phandle, &rk_usb_fdt_funcs);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;

		struct fdt_attach_args cfaa = *faa;
		cfaa.faa_phandle = child;
		cfaa.faa_name = fdtbus_get_string(child, "name");
		cfaa.faa_quiet = false;

		config_found(self, &cfaa, NULL);
	}
}

/*
 * USB PHY
 */

static int rk_usbphy_match(device_t, cfdata_t, void *);
static void rk_usbphy_attach(device_t, device_t, void *);

struct rk_usbphy_softc {
	device_t	sc_dev;
	int		sc_phandle;
};

CFATTACH_DECL_NEW(rk_usbphy, sizeof(struct rk_usbphy_softc),
	rk_usbphy_match, rk_usbphy_attach, NULL, NULL);

static void *
rk_usbphy_acquire(device_t dev, const void *data, size_t len)
{
	struct rk_usbphy_softc * const sc = device_private(dev);

	if (len != 0)
		return NULL;

	return sc;
}

static void
rk_usbphy_release(device_t dev, void *priv)
{
}

static int
rk_usbphy_otg_enable(device_t dev, void *priv, bool enable)
{
	struct rk_usb_softc * const usb_sc = device_private(device_parent(dev));

	const uint32_t write_mask = 0x1ffU << 16;
	const uint32_t write_val = enable ? 0 : 0x1d1;
	USB_WRITE(usb_sc, CON0_REG, write_mask | write_val);

	return 0;
}

static int
rk_usbphy_host_enable(device_t dev, void *priv, bool enable)
{
	struct rk_usb_softc * const usb_sc = device_private(device_parent(dev));

	const uint32_t write_mask = 0x1ffU << 16;
	const uint32_t write_val = enable ? 0 : 0x1d1;
	USB_WRITE(usb_sc, CON1_REG, write_mask | write_val);

	return 0;
}

const struct fdtbus_phy_controller_func rk_usbphy_otg_funcs = {
	.acquire = rk_usbphy_acquire,
	.release = rk_usbphy_release,
	.enable = rk_usbphy_otg_enable,
};

const struct fdtbus_phy_controller_func rk_usbphy_host_funcs = {
	.acquire = rk_usbphy_acquire,
	.release = rk_usbphy_release,
	.enable = rk_usbphy_host_enable,
};

static int
rk_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *name = fdtbus_get_string(phandle, "name");

	if (strcmp(name, "otg-port") == 0 || strcmp(name, "host-port") == 0)
		return 1;

	return 0;
}

static void
rk_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct rk_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *name = fdtbus_get_string(phandle, "name");

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");

	if (strcmp(name, "otg-port") == 0) {
		aprint_normal(": USB2 OTG port\n");
		fdtbus_register_phy_controller(self, phandle, &rk_usbphy_otg_funcs);
	} else if (strcmp(name, "host-port") == 0) {
		aprint_normal(": USB2 host port\n");
		fdtbus_register_phy_controller(self, phandle, &rk_usbphy_host_funcs);
	}
}
