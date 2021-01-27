/* $NetBSD: omap3_cm.c,v 1.5 2021/01/27 03:10:20 thorpej Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: omap3_cm.c,v 1.5 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#define	TI_PRCM_PRIVATE
#include <arm/ti/ti_prcm.h>

#define	CM_CORE1_BASE		0x0a00
#define	CM_CORE3_BASE		0x0a08
#define	CM_WKUP_BASE		0x0c00
#define	CM_PER_BASE		0x1000
#define	CM_USBHOST_BASE		0x1400

#define	CM_FCLKEN		0x00
#define	CM_ICLKEN		0x10
#define	CM_AUTOIDLE		0x30
#define	CM_CLKSEL		0x40

static int omap3_cm_match(device_t, cfdata_t, void *);
static void omap3_cm_attach(device_t, device_t, void *);

static int
omap3_cm_hwmod_nopenable(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	return 0;
}

static int
omap3_cm_hwmod_enable(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc, int enable)
{
	uint32_t val;

	val = PRCM_READ(sc, tc->u.hwmod.reg + CM_FCLKEN);
	if (enable)
		val |= tc->u.hwmod.mask;
	else
		val &= ~tc->u.hwmod.mask;
	PRCM_WRITE(sc, tc->u.hwmod.reg + CM_FCLKEN, val);

	val = PRCM_READ(sc, tc->u.hwmod.reg + CM_ICLKEN);
	if (enable)
		val |= tc->u.hwmod.mask;
	else
		val &= ~tc->u.hwmod.mask;
	PRCM_WRITE(sc, tc->u.hwmod.reg + CM_ICLKEN, val);

	if (tc->u.hwmod.flags & TI_HWMOD_DISABLE_AUTOIDLE) {
		val = PRCM_READ(sc, tc->u.hwmod.reg + CM_AUTOIDLE);
		val &= ~tc->u.hwmod.mask;
		PRCM_WRITE(sc, tc->u.hwmod.reg + CM_AUTOIDLE, val);
	}

	return 0;
}

#define	OMAP3_CM_HWMOD_CORE1(_name, _bit, _parent, _flags)	\
	TI_PRCM_HWMOD_MASK((_name), CM_CORE1_BASE, __BIT(_bit), (_parent), omap3_cm_hwmod_enable, (_flags))
#define	OMAP3_CM_HWMOD_CORE3(_name, _bit, _parent, _flags)	\
	TI_PRCM_HWMOD_MASK((_name), CM_CORE3_BASE, __BIT(_bit), (_parent), omap3_cm_hwmod_enable, (_flags))
#define	OMAP3_CM_HWMOD_WKUP(_name, _bit, _parent, _flags)	\
	TI_PRCM_HWMOD_MASK((_name), CM_WKUP_BASE, __BIT(_bit), (_parent), omap3_cm_hwmod_enable, (_flags))
#define	OMAP3_CM_HWMOD_PER(_name, _bit, _parent, _flags)	\
	TI_PRCM_HWMOD_MASK((_name), CM_PER_BASE, __BIT(_bit), (_parent), omap3_cm_hwmod_enable, (_flags))
#define	OMAP3_CM_HWMOD_USBHOST(_name, _bit, _parent, _flags)	\
	TI_PRCM_HWMOD_MASK((_name), CM_USBHOST_BASE, __BIT(_bit), (_parent), omap3_cm_hwmod_enable, (_flags))
#define	OMAP3_CM_HWMOD_NOP(_name, _parent)			\
	TI_PRCM_HWMOD_MASK((_name), 0, 0, (_parent), omap3_cm_hwmod_nopenable, 0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap3-cm" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(omap3_cm, sizeof(struct ti_prcm_softc),
	omap3_cm_match, omap3_cm_attach, NULL, NULL);

static struct ti_prcm_clk omap3_cm_clks[] = {
	/* XXX until we get a proper clock tree */
	TI_PRCM_FIXED("FIXED_32K", 32768),
	TI_PRCM_FIXED("SYS_CLK", 13000000),
	TI_PRCM_FIXED("MMC_CLK", 96000000),
	TI_PRCM_FIXED_FACTOR("PERIPH_CLK", 1, 1, "SYS_CLK"),

	OMAP3_CM_HWMOD_CORE1("usb_otg_hs", 4, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcbsp1", 9, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcbsp5", 10, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("timer10", 11, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("timer11", 12, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("uart1", 13, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("uart2", 14, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("i2c1", 15, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("i2c2", 16, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("i2c3", 17, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcspi1", 18, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcspi2", 19, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcspi3", 20, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mcspi4", 21, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("hdq1w", 22, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mmc1", 24, "MMC_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mmc2", 25, "MMC_CLK", 0),
	OMAP3_CM_HWMOD_CORE1("mmc3", 30, "MMC_CLK", 0),

	OMAP3_CM_HWMOD_CORE3("usb_tll_hs", 2, "PERIPH_CLK", TI_HWMOD_DISABLE_AUTOIDLE),

	OMAP3_CM_HWMOD_WKUP("timer1", 0, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_WKUP("counter_32k", 2, "FIXED_32K", 0),
	OMAP3_CM_HWMOD_WKUP("gpio1", 3, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_WKUP("wd_timer2", 5, "FIXED_32K", 0),

	OMAP3_CM_HWMOD_PER("mcbsp2", 0, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("mcbsp3", 1, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("mcbsp4", 2, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer2", 3, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer3", 4, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer4", 5, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer5", 6, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer6", 7, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer7", 8, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer8", 9, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("timer9", 10, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("uart3", 11, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("wd_timer3", 12, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("gpio2", 13, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("gpio3", 14, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("gpio4", 15, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("gpio5", 16, "PERIPH_CLK", 0),
	OMAP3_CM_HWMOD_PER("gpio6", 17, "PERIPH_CLK", 0),

	OMAP3_CM_HWMOD_USBHOST("usb_host_hs", 0, "PERIPH_CLK", 0),

	OMAP3_CM_HWMOD_NOP("gpmc", "PERIPH_CLK"),
};

static void
omap3_cm_initclocks(struct ti_prcm_softc *sc)
{
	uint32_t val;

	/* Select SYS_CLK for GPTIMER 2 and 3 */
	val = PRCM_READ(sc, CM_PER_BASE + CM_CLKSEL);
	val |= __BIT(0);	/* CLKSEL_GPT2  0x1: source is SYS_CLK */
	val |= __BIT(1);	/* CLKSEL_GPT3  0x1: source is SYS_CLK */
	PRCM_WRITE(sc, CM_PER_BASE + CM_CLKSEL, val);
}

static int
omap3_cm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
omap3_cm_attach(device_t parent, device_t self, void *aux)
{
	struct ti_prcm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	int clocks;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = omap3_cm_clks;
	sc->sc_nclks = __arraycount(omap3_cm_clks);

	if (ti_prcm_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": OMAP3xxx CM\n");

	omap3_cm_initclocks(sc);

	clocks = of_find_firstchild_byname(sc->sc_phandle, "clocks");
	if (clocks > 0)
		fdt_add_bus(self, clocks, faa);
}
