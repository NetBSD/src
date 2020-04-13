/* $NetBSD: am3_prcm.c,v 1.1.6.1 2020/04/13 08:03:38 martin Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: am3_prcm.c,v 1.1.6.1 2020/04/13 08:03:38 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#define	TI_PRCM_PRIVATE
#include <arm/ti/ti_prcm.h>

#define	AM3_PRCM_CM_PER		0x0000
#define	AM3_PRCM_CM_WKUP	0x0400
#define	AM3_PRCM_CM_DPLL	0x0500
#define	AM3_PRCM_CM_MPU		0x0600
#define	AM3_PRCM_CM_DEVICE	0x0700
#define	AM3_PRCM_CM_RTC		0x0800
#define	AM3_PRCM_CM_GFX		0x0900
#define	AM3_PRCM_CM_CEFUSE	0x0a00

#define	AM3_PRCM_CLKCTRL_MODULEMODE		__BITS(1,0)
#define	AM3_PRCM_CLKCTRL_MODULEMODE_ENABLE	0x2

#define	AM3_PRCM_CM_IDLEST_DPLL_DISP	(AM3_PRCM_CM_WKUP + 0x48)
#define	 AM3_PRCM_CM_IDLEST_DPLL_DISP_ST_MN_BYPASS	__BIT(8)
#define	 AM3_PRCM_CM_IDLEST_DPLL_DISP_ST_DPLL_CLK	__BIT(0)
#define	AM3_PRCM_CM_CLKSEL_DPLL_DISP	(AM3_PRCM_CM_WKUP + 0x54)
#define	 AM3_PRCM_CM_CLKSEL_DPLL_DISP_DPLL_MULT		__BITS(18,8)
#define	 AM3_PRCM_CM_CLKSEL_DPLL_DISP_DPLL_DIV		__BITS(6,0)
#define	AM3_PRCM_CM_CLKMODE_DPLL_DISP	(AM3_PRCM_CM_WKUP + 0x98)
#define	 AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN		__BITS(2,0)
#define	  AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN_MN_BYPASS	4
#define	  AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN_LOCK		7

#define	DPLL_DISP_RATE				297000000

static int am3_prcm_match(device_t, cfdata_t, void *);
static void am3_prcm_attach(device_t, device_t, void *);

static int
am3_prcm_hwmod_enable(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	uint32_t val;

	val = PRCM_READ(sc, tc->u.hwmod.reg);
	val &= ~AM3_PRCM_CLKCTRL_MODULEMODE;
	if (enable)
		val |= __SHIFTIN(AM3_PRCM_CLKCTRL_MODULEMODE_ENABLE,
				 AM3_PRCM_CLKCTRL_MODULEMODE);
	PRCM_WRITE(sc, tc->u.hwmod.reg, val);

	return 0;
}

static int
am3_prcm_hwmod_enable_display(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	uint32_t val;
	int retry;

	if (enable) {
		/* Put the DPLL in MN bypass mode */
		PRCM_WRITE(sc, AM3_PRCM_CM_CLKMODE_DPLL_DISP,
		    __SHIFTIN(AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN_MN_BYPASS,
			      AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN));
		for (retry = 10000; retry > 0; retry--) {
			val = PRCM_READ(sc, AM3_PRCM_CM_IDLEST_DPLL_DISP);
			if ((val & AM3_PRCM_CM_IDLEST_DPLL_DISP_ST_MN_BYPASS) != 0)
				break;
			delay(10);
		}

		/* Set DPLL frequency to DPLL_DISP_RATE (297 MHz) */
		val = __SHIFTIN(DPLL_DISP_RATE / 1000000, AM3_PRCM_CM_CLKSEL_DPLL_DISP_DPLL_MULT);
		val |= __SHIFTIN(24 - 1, AM3_PRCM_CM_CLKSEL_DPLL_DISP_DPLL_DIV);
		PRCM_WRITE(sc, AM3_PRCM_CM_CLKSEL_DPLL_DISP, val);

		/* Disable MN bypass mode */
		PRCM_WRITE(sc, AM3_PRCM_CM_CLKMODE_DPLL_DISP,
		    __SHIFTIN(AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN_LOCK,
			      AM3_PRCM_CM_CLKMODE_DPLL_DISP_DPLL_EN));
		for (retry = 10000; retry > 0; retry--) {
			val = PRCM_READ(sc, AM3_PRCM_CM_IDLEST_DPLL_DISP);
			if ((val & AM3_PRCM_CM_IDLEST_DPLL_DISP_ST_DPLL_CLK) != 0)
				break;
			delay(10);
		}
	}

	return am3_prcm_hwmod_enable(sc, tc, enable);
}

#define	AM3_PRCM_HWMOD_PER(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), AM3_PRCM_CM_PER + (_reg), (_parent), am3_prcm_hwmod_enable)
#define	AM3_PRCM_HWMOD_PER_DISP(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), AM3_PRCM_CM_PER + (_reg), (_parent), am3_prcm_hwmod_enable_display)
#define	AM3_PRCM_HWMOD_WKUP(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), AM3_PRCM_CM_WKUP + (_reg), (_parent), am3_prcm_hwmod_enable)

static const char * const compatible[] = {
	"ti,am3-prcm",
	NULL
};

CFATTACH_DECL_NEW(am3_prcm, sizeof(struct ti_prcm_softc),
	am3_prcm_match, am3_prcm_attach, NULL, NULL);

static struct ti_prcm_clk am3_prcm_clks[] = {
	/* XXX until we get a proper clock tree */
	TI_PRCM_FIXED("FIXED_32K", 32768),
	TI_PRCM_FIXED("FIXED_24MHZ", 24000000),
	TI_PRCM_FIXED("FIXED_48MHZ", 48000000),
	TI_PRCM_FIXED("FIXED_96MHZ", 96000000),
	TI_PRCM_FIXED("DISPLAY_CLK", DPLL_DISP_RATE),
	TI_PRCM_FIXED_FACTOR("PERIPH_CLK", 1, 1, "FIXED_48MHZ"),
	TI_PRCM_FIXED_FACTOR("MMC_CLK", 1, 1, "FIXED_96MHZ"),

	AM3_PRCM_HWMOD_PER("uart1", 0x6c, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart2", 0x70, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart3", 0x74, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart4", 0x78, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart5", 0x38, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_WKUP("i2c1", 0xb8, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("i2c2", 0x48, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("i2c3", 0x44, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_WKUP("gpio1", 0x8, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("gpio2", 0xac, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("gpio3", 0xb0, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("gpio4", 0xb4, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_WKUP("timer1", 0x10, "FIXED_32K"),
	AM3_PRCM_HWMOD_PER("timer2", 0x80, "FIXED_24MHZ"),
	AM3_PRCM_HWMOD_PER("timer3", 0x84, "FIXED_24MHZ"),
	AM3_PRCM_HWMOD_PER("timer4", 0x88, "FIXED_24MHZ"),
	AM3_PRCM_HWMOD_PER("timer5", 0xec, "FIXED_24MHZ"),
	AM3_PRCM_HWMOD_PER("timer6", 0xf0, "FIXED_24MHZ"),
	AM3_PRCM_HWMOD_PER("timer7", 0x7c, "FIXED_24MHZ"),

	AM3_PRCM_HWMOD_WKUP("wd_timer2", 0xd4, "FIXED_32K"),

	AM3_PRCM_HWMOD_PER("mmc1", 0x3c, "MMC_CLK"),
	AM3_PRCM_HWMOD_PER("mmc2", 0xf4, "MMC_CLK"),
	AM3_PRCM_HWMOD_PER("mmc3", 0xf8, "MMC_CLK"),

	AM3_PRCM_HWMOD_PER("tpcc", 0xbc, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("tptc0", 0x24, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("tptc1", 0xfc, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("tptc2", 0x100, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_PER("usb_otg_hs", 0x1c, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_PER("rng", 0x90, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_PER_DISP("lcdc", 0x18, "DISPLAY_CLK"),
};

static int
am3_prcm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
am3_prcm_attach(device_t parent, device_t self, void *aux)
{
	struct ti_prcm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	int clocks;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = am3_prcm_clks;
	sc->sc_nclks = __arraycount(am3_prcm_clks);

	if (ti_prcm_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": AM3xxx PRCM\n");

	clocks = of_find_firstchild_byname(sc->sc_phandle, "clocks");
	if (clocks > 0)
		fdt_add_bus(self, clocks, faa);
}
