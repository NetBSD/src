/* $NetBSD: am3_prcm.c,v 1.1 2017/10/26 23:28:15 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: am3_prcm.c,v 1.1 2017/10/26 23:28:15 jmcneill Exp $");

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

#define	AM3_PRCM_HWMOD_PER(_name, _reg, _parent)	\
	TI_PRCM_HWMOD((_name), AM3_PRCM_CM_PER + (_reg), (_parent), am3_prcm_hwmod_enable)
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
	TI_PRCM_FIXED("FIXED_48MHZ", 48000000),
	TI_PRCM_FIXED("FIXED_96MHZ", 96000000),
	TI_PRCM_FIXED_FACTOR("PERIPH_CLK", 1, 1, "FIXED_48MHZ"),
	TI_PRCM_FIXED_FACTOR("MMC_CLK", 1, 1, "FIXED_96MHZ"),

	AM3_PRCM_HWMOD_PER("uart1", 0x6c, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart2", 0x70, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart3", 0x74, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart4", 0x78, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("uart5", 0x38, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_WKUP("timer0", 0x10, "FIXED_32K"),
	AM3_PRCM_HWMOD_PER("timer2", 0x80, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("timer3", 0x84, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("timer4", 0x88, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("timer5", 0xec, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("timer6", 0xf0, "PERIPH_CLK"),
	AM3_PRCM_HWMOD_PER("timer7", 0x7c, "PERIPH_CLK"),

	AM3_PRCM_HWMOD_PER("mmc0", 0x3c, "MMC_CLK"),
	AM3_PRCM_HWMOD_PER("mmc1", 0xf4, "MMC_CLK"),
	AM3_PRCM_HWMOD_PER("mmc2", 0xf8, "MMC_CLK"),
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

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = am3_prcm_clks;
	sc->sc_nclks = __arraycount(am3_prcm_clks);

	if (ti_prcm_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": AM3xxx PRCM\n");
}
