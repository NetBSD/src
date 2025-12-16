/* $NetBSD: omap4_prcm.c,v 1.1 2025/12/16 12:20:22 skrll Exp $ */

/*-
 * Copyright (c) 2025 Rui-Xiang Guo
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

__KERNEL_RCSID(1, "$NetBSD: omap4_prcm.c,v 1.1 2025/12/16 12:20:22 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#define	TI_PRCM_PRIVATE
#include <arm/ti/ti_prcm.h>

#define CM1_ABE		0x0500

#define CM2_CORE	0x0700
#define CM2_L3INIT	0x1300
#define CM2_L4PER	0x1400

#define CM_WKUP		0x1800

#define	CLKCTRL_MODULEMODE		__BITS(1,0)
#define	CLKCTRL_MODULEMODE_DISABLE	0x0
#define	CLKCTRL_MODULEMODE_AUTO		0x1
#define	CLKCTRL_MODULEMODE_ENABLE	0x2

#define	CLKCTRL_IDLEST			__BITS(17,16)
#define CLKCTRL_IDLEST_ENABLE		0x0
#define CLKCTRL_IDLEST_BUSY		0x1
#define CLKCTRL_IDLEST_IDLE		0x2
#define CLKCTRL_IDLEST_DISABLE		0x3

enum omap4_prcm_inst {
	PRCM_CM1 = 1,
	PRCM_CM2,
	PRCM_PRM,
};

struct omap4_prcm_softc {
	struct ti_prcm_softc	sc_prcm;	/* must be first */
	bus_addr_t		sc_regbase;
	enum omap4_prcm_inst	sc_inst;
};

static int omap4_prcm_match(device_t, cfdata_t, void *);
static void omap4_prcm_attach(device_t, device_t, void *);

static int
omap4_prcm_hwmod_enable(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	uint32_t val;
	int retry;

	val = PRCM_READ(sc, tc->u.hwmod.reg);
	val &= ~CLKCTRL_MODULEMODE;
	if (enable) {
		val |= __SHIFTIN(CLKCTRL_MODULEMODE_ENABLE,
					 CLKCTRL_MODULEMODE);
	} else
		val |= __SHIFTIN(CLKCTRL_MODULEMODE_DISABLE,
					 CLKCTRL_MODULEMODE);
	PRCM_WRITE(sc, tc->u.hwmod.reg, val);

	for (retry = 100; retry > 0; retry--) {
		val = PRCM_READ(sc, tc->u.hwmod.reg);
		if ((val & CLKCTRL_IDLEST) == CLKCTRL_IDLEST_ENABLE)
			break;
		delay(10);
	}

	return (retry >= 0) ? 0 : ETIMEDOUT;
}

static int
omap4_prcm_hwmod_enable_auto(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	uint32_t val;
	int retry;

	val = PRCM_READ(sc, tc->u.hwmod.reg);
	val &= ~CLKCTRL_MODULEMODE;
	if (enable) {
		val |= __SHIFTIN(CLKCTRL_MODULEMODE_AUTO,
					 CLKCTRL_MODULEMODE);
	} else
		val |= __SHIFTIN(CLKCTRL_MODULEMODE_DISABLE,
					 CLKCTRL_MODULEMODE);
	PRCM_WRITE(sc, tc->u.hwmod.reg, val);

	for (retry = 100; retry > 0; retry--) {
		val = PRCM_READ(sc, tc->u.hwmod.reg);
		if ((val & CLKCTRL_IDLEST) == CLKCTRL_IDLEST_ENABLE)
			break;
		delay(10);
	}

	return (retry >= 0) ? 0 : ETIMEDOUT;
}

#define	OMAP4_PRCM_HWMOD_CM1_ABE(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM1_ABE + (_reg), (_parent), omap4_prcm_hwmod_enable)
#define	OMAP4_PRCM_HWMOD_CM2_L3INIT(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM2_L3INIT + (_reg), (_parent), omap4_prcm_hwmod_enable)
#define	OMAP4_PRCM_HWMOD_CM2_L3INIT_AUTO(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM2_L3INIT + (_reg), (_parent), omap4_prcm_hwmod_enable_auto)
#define	OMAP4_PRCM_HWMOD_CM2_L4PER(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM2_L4PER + (_reg), (_parent), omap4_prcm_hwmod_enable)
#define	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM2_L4PER + (_reg), (_parent), omap4_prcm_hwmod_enable_auto)
#define	OMAP4_PRCM_HWMOD_CM_WKUP(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM_WKUP + (_reg), (_parent), omap4_prcm_hwmod_enable)
#define	OMAP4_PRCM_HWMOD_CM_WKUP_AUTO(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), CM_WKUP + (_reg), (_parent), omap4_prcm_hwmod_enable_auto)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap4-cm1",	.value = PRCM_CM1 },
	{ .compat = "ti,omap4-cm2",	.value = PRCM_CM2 },
	{ .compat = "ti,omap4-prm",	.value = PRCM_PRM },
	{ .compat = "ti,omap4-scrm",	.value = PRCM_PRM },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry cm_compat_data[] = {
	{ .compat = "ti,omap4-cm" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry clkctrl_compat_data[] = {
	{ .compat = "ti,clkctrl" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(omap4_prcm, sizeof(struct omap4_prcm_softc),
	omap4_prcm_match, omap4_prcm_attach, NULL, NULL);

static struct ti_prcm_clk omap4_cm1_clks[] = {
	/* XXX until we get a proper clock tree */
	TI_PRCM_FIXED("FIXED_32K", 32768),
	TI_PRCM_FIXED("FIXED_24MHZ", 24000000),

	OMAP4_PRCM_HWMOD_CM1_ABE("timer5", 0x68, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM1_ABE("timer6", 0x70, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM1_ABE("timer7", 0x78, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM1_ABE("timer8", 0x80, "FIXED_24MHZ"),
//	OMAP4_PRCM_HWMOD_CM1_ABE("wd_timer3", 0x88, "FIXED_32K"),
};

static struct ti_prcm_clk omap4_cm2_clks[] = {
	/* XXX until we get a proper clock tree */
	TI_PRCM_FIXED("FIXED_32K", 32768),
	TI_PRCM_FIXED("FIXED_24MHZ", 24000000),
	TI_PRCM_FIXED("FIXED_48MHZ", 48000000),
	TI_PRCM_FIXED("FIXED_96MHZ", 96000000),
	TI_PRCM_FIXED_FACTOR("PERIPH_CLK", 1, 1, "FIXED_48MHZ"),
	TI_PRCM_FIXED_FACTOR("MMC_CLK", 1, 1, "FIXED_96MHZ"),

	OMAP4_PRCM_HWMOD_CM2_L4PER("uart1", 0x140, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("uart2", 0x148, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("uart3", 0x150, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("uart4", 0x158, "PERIPH_CLK"),

	OMAP4_PRCM_HWMOD_CM2_L4PER("i2c1", 0xa0, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("i2c2", 0xa8, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("i2c3", 0xb0, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("i2c4", 0xb8, "PERIPH_CLK"),

	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("gpio2", 0x60, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("gpio3", 0x68, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("gpio4", 0x70, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("gpio5", 0x78, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("gpio6", 0x80, "PERIPH_CLK"),

	OMAP4_PRCM_HWMOD_CM2_L4PER("timer2", 0x38, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("timer3", 0x40, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("timer4", 0x48, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("timer9", 0x50, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("timer10", 0x28, "FIXED_24MHZ"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("timer11", 0x30, "FIXED_24MHZ"),

	OMAP4_PRCM_HWMOD_CM2_L3INIT("mmc1", 0x28, "MMC_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L3INIT("mmc2", 0x30, "MMC_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("mmc3", 0x120, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("mmc4", 0x128, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L4PER("mmc5", 0x160, "PERIPH_CLK"),

	OMAP4_PRCM_HWMOD_CM2_L3INIT("usb_host_hs", 0x58, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L3INIT_AUTO("usb_otg_hs", 0x60, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM2_L3INIT_AUTO("usb_tll_hs", 0x68, "PERIPH_CLK"),

//	OMAP4_PRCM_HWMOD_CM2_L4PER_AUTO("rng", 0x1c0, "PERIPH_CLK"),
};

static struct ti_prcm_clk omap4_prm_clks[] = {
	/* XXX until we get a proper clock tree */
	TI_PRCM_FIXED("FIXED_32K", 32768),
	TI_PRCM_FIXED("FIXED_48MHZ", 48000000),
	TI_PRCM_FIXED_FACTOR("PERIPH_CLK", 1, 1, "FIXED_48MHZ"),

	OMAP4_PRCM_HWMOD_CM_WKUP_AUTO("gpio1", 0x38, "PERIPH_CLK"),
	OMAP4_PRCM_HWMOD_CM_WKUP("timer1", 0x40, "FIXED_32K"),
	OMAP4_PRCM_HWMOD_CM_WKUP("wd_timer2", 0x30, "FIXED_32K"),
};

static struct clk *
omap4_prcm_clock_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct omap4_prcm_softc * const sc = device_private(dev);
	const u_int *cells = data;
	bus_addr_t regbase;
	u_int n;

	if (len != 8)
		return NULL;

	bus_size_t regoff = be32toh(cells[0]);
	const u_int clock_index = be32toh(cells[1]);

	/* XXX not sure how to handle this yet */
	if (clock_index != 0)
		return NULL;

	/*
	 * Register offset in specifier is relative to base address of the
	 * clock node. Translate this to an address relative to the start
	 * of PRCM space.
	 */
	if (fdtbus_get_reg(cc_phandle, 0, &regbase, NULL) != 0)
		return NULL;
	regoff += (regbase - sc->sc_regbase);

	/*
	 * Look for a matching hwmod.
	 */
	for (n = 0; n < sc->sc_prcm.sc_nclks; n++) {
		struct ti_prcm_clk *tclk = &sc->sc_prcm.sc_clks[n];
		if (tclk->type != TI_PRCM_HWMOD)
			continue;

		if (tclk->u.hwmod.reg == regoff)
			return &tclk->base;
	}

	/* Not found */
	return NULL;
}

static const struct fdtbus_clock_controller_func omap4_prcm_clock_fdt_funcs = {
	.decode = omap4_prcm_clock_decode
};

static int
omap4_prcm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
omap4_prcm_attach(device_t parent, device_t self, void *aux)
{
	struct omap4_prcm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int clocks, child, cm_child;
	char iname[5];

	if (fdtbus_get_reg(phandle, 0, &sc->sc_regbase, NULL) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_prcm.sc_dev = self;
	sc->sc_prcm.sc_phandle = phandle;
	sc->sc_prcm.sc_bst = faa->faa_bst;
	sc->sc_inst = of_compatible_lookup(phandle, compat_data)->value;

	switch (sc->sc_inst) {
	case PRCM_CM1:
		sc->sc_prcm.sc_clks = omap4_cm1_clks;
		sc->sc_prcm.sc_nclks = __arraycount(omap4_cm1_clks);
		strcpy(iname, "CM1");
		break;
	case PRCM_CM2:
		sc->sc_prcm.sc_clks = omap4_cm2_clks;
		sc->sc_prcm.sc_nclks = __arraycount(omap4_cm2_clks);
		strcpy(iname, "CM2");
		break;
	case PRCM_PRM:
		sc->sc_prcm.sc_clks = omap4_prm_clks;
		sc->sc_prcm.sc_nclks = __arraycount(omap4_prm_clks);
		strcpy(iname, "PRM");
		break;
	default:
		aprint_error(": unsupported instance\n");
		return;
	}

	if (ti_prcm_attach(&sc->sc_prcm) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": OMAP44xx PRCM (%s)\n", iname);

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (of_compatible_match(child, cm_compat_data) == 0)
			continue;

		for (cm_child =	OF_child(child); cm_child;
		     cm_child = OF_peer(cm_child)) {
			if (of_compatible_match(cm_child,
						 clkctrl_compat_data) == 0)
				continue;

			aprint_debug_dev(self, "clkctrl: %s\n", fdtbus_get_string(cm_child, "name"));
			fdtbus_register_clock_controller(self, cm_child,
			    &omap4_prcm_clock_fdt_funcs);
		}
	}

	clocks = of_find_firstchild_byname(phandle, "clocks");
	if (clocks > 0)
		fdt_add_bus(self, clocks, faa);
}
