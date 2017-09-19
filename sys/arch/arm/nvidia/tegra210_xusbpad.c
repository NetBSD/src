/* $NetBSD: tegra210_xusbpad.c,v 1.1 2017/09/19 23:18:01 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra210_xusbpad.c,v 1.1 2017/09/19 23:18:01 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_xusbpad.h>

#include <dev/fdt/fdtvar.h>

struct tegra210_xusbpad_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct fdtbus_reset	*sc_rst;
};

#define	RD4(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	SETCLR4(sc, reg, set, clr)			\
	tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static const char * tegra210_xusbpad_usb2_func[] = { "snps", "xusb", "uart" };
static const char * tegra210_xusbpad_hsic_func[] = { "snps", "xusb" };
static const char * tegra210_xusbpad_pcie_func[] = { "pcie-x1", "usb3-ss", "sata", "pcie-x4" };

#define	XUSBPAD_LANE(n, r, m, f)		\
	{					\
		.name = (n),			\
		.reg = (r),			\
		.mask = (m),			\
		.funcs = (f),			\
		.nfuncs = __arraycount(f)	\
	}

static const struct tegra210_xusbpad_lane {
	const char		*name;
	bus_size_t		reg;
	uint32_t		mask;
	const char		**funcs;
	int			nfuncs;
} tegra210_xusbpad_lanes[] = {
	XUSBPAD_LANE("usb2-0", 0x04, __BITS(1,0), tegra210_xusbpad_usb2_func),
	XUSBPAD_LANE("usb2-1", 0x04, __BITS(3,2), tegra210_xusbpad_usb2_func),
	XUSBPAD_LANE("usb2-2", 0x04, __BITS(5,4), tegra210_xusbpad_usb2_func),
	XUSBPAD_LANE("usb2-3", 0x04, __BITS(7,6), tegra210_xusbpad_usb2_func),

	XUSBPAD_LANE("hsic-0", 0x04, __BIT(14), tegra210_xusbpad_hsic_func),
	XUSBPAD_LANE("hsic-1", 0x04, __BIT(15), tegra210_xusbpad_hsic_func),

	XUSBPAD_LANE("pcie-0", 0x28, __BITS(13,12), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-1", 0x28, __BITS(15,14), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-2", 0x28, __BITS(17,16), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-3", 0x28, __BITS(19,18), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-4", 0x28, __BITS(21,20), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-5", 0x28, __BITS(23,22), tegra210_xusbpad_pcie_func),
	XUSBPAD_LANE("pcie-6", 0x28, __BITS(25,24), tegra210_xusbpad_pcie_func),

	XUSBPAD_LANE("sata-0", 0x28, __BITS(31,30), tegra210_xusbpad_pcie_func),
};

static int
tegra210_xusbpad_find_func(const struct tegra210_xusbpad_lane *lane,
    const char *func)
{
	for (int n = 0; n < lane->nfuncs; n++)
		if (strcmp(lane->funcs[n], func) == 0)
			return n;
	return -1;
}

static const struct tegra210_xusbpad_lane *
tegra210_xusbpad_find_lane(const char *name)
{
	for (int n = 0; n < __arraycount(tegra210_xusbpad_lanes); n++)
		if (strcmp(tegra210_xusbpad_lanes[n].name, name) == 0)
			return &tegra210_xusbpad_lanes[n];
	return NULL;
}

static void
tegra210_xusbpad_configure_lane(struct tegra210_xusbpad_softc *sc,
    int phandle)
{
	const struct tegra210_xusbpad_lane *lane;
	const char *name, *function;
	int func;

	name = fdtbus_get_string(phandle, "name");
	if (name == NULL) {
		aprint_error_dev(sc->sc_dev, "no 'name' property\n");
		return;
	}
	function = fdtbus_get_string(phandle, "nvidia,function");
	if (function == NULL) {
		aprint_error_dev(sc->sc_dev, "no 'nvidia,function' property\n");
		return;
	}

	lane = tegra210_xusbpad_find_lane(name);
	if (lane == NULL) {
		aprint_error_dev(sc->sc_dev, "unsupported lane '%s'\n", name);
		return;
	}
	func = tegra210_xusbpad_find_func(lane, function);
	if (func == -1) {
		aprint_error_dev(sc->sc_dev, "unsupported function '%s'\n", function);
		return;
	}

	aprint_debug_dev(sc->sc_dev, "[%s] set func %s\n", name, function);
	SETCLR4(sc, lane->reg, __SHIFTIN(func, lane->mask), lane->mask);
}

static void
tegra210_xusbpad_configure_pads(struct tegra210_xusbpad_softc *sc,
    const char *name)
{
	struct fdtbus_reset *rst;
	struct clk *clk;
	int phandle, child;

	/* Search for the pad's node */
	phandle = of_find_firstchild_byname(sc->sc_phandle, "pads");
	if (phandle == -1) {
		aprint_error_dev(sc->sc_dev, "no 'pads' node\n");
		return;
	}
	phandle = of_find_firstchild_byname(phandle, name);
	if (phandle == -1) {
		aprint_error_dev(sc->sc_dev, "no 'pads/%s' node\n", name);
		return;
	}

	if (!fdtbus_status_okay(phandle))
		return;		/* pad is disabled */

	/* Enable the pad's resources */
	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk && clk_enable(clk) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't enable %s's clock\n", name);
		return;
	}
	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst && fdtbus_reset_deassert(rst) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't de-assert %s's reset\n", name);
		return;
	}

	/* Configure lanes */
	phandle = of_find_firstchild_byname(phandle, "lanes");
	if (phandle == -1) {
		aprint_error_dev(sc->sc_dev, "no 'pads/%s/lanes' node\n", name);
		return;
	}
	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		tegra210_xusbpad_configure_lane(sc, child);
	}
}

static void
tegra210_xusbpad_sata_enable(device_t dev)
{
}

static void
tegra210_xusbpad_xhci_enable(device_t dev)
{
}

static const struct tegra_xusbpad_ops tegra210_xusbpad_ops = {
	.sata_enable = tegra210_xusbpad_sata_enable,
	.xhci_enable = tegra210_xusbpad_xhci_enable,
};

static int
tegra210_xusbpad_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-xusb-padctl",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra210_xusbpad_attach(device_t parent, device_t self, void *aux)
{
	struct tegra210_xusbpad_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(faa->faa_phandle, "padctl");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset padctl\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": XUSB PADCTL\n");

	fdtbus_reset_deassert(sc->sc_rst);

	tegra_xusbpad_register(self, &tegra210_xusbpad_ops);

	tegra210_xusbpad_configure_pads(sc, "usb2");
	tegra210_xusbpad_configure_pads(sc, "hsic");
	tegra210_xusbpad_configure_pads(sc, "pcie");
	tegra210_xusbpad_configure_pads(sc, "sata");
}

CFATTACH_DECL_NEW(tegra210_xusbpad, sizeof(struct tegra210_xusbpad_softc),
	tegra210_xusbpad_match, tegra210_xusbpad_attach, NULL, NULL);
