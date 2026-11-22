/* $NetBSD: sun60i_w2_rtc_ccu.c,v 1.1 2026/11/22 10:17:13 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(1, "$NetBSD: sun60i_w2_rtc_ccu.c,v 1.1 2026/11/22 10:17:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun60i_w2_rtc_ccu.h>
	
#define	LOSC_CTRL_REG			0x000
#define	LOSC_AUTO_SWT_STA_REG		0x004
#define	CLK32K_FOUT_CTRL_GATING_REG	0x060
#define	XO_CTRL0_REG			0x160
#define	RTC_SPI_CLK_CTRL_REG		0x310

static int sun60i_w2_rtc_ccu_match(device_t, cfdata_t, void *);
static void sun60i_w2_rtc_ccu_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun60iw2-rtc-ccu" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_w2_rtc_ccu, sizeof(struct sunxi_ccu_softc),
	sun60i_w2_rtc_ccu_match, sun60i_w2_rtc_ccu_attach, NULL, NULL);

static const char *osc32k_parents[] = { "iosc-div32k", "ext32k-gate" };
static const char *rtc32k_parents[] = { "osc32k", "dcxo24M-div32k" };
static const char *rtc_32k_fanout_parents[] = { "osc32k", "ext32k-gate", "dcxo24M-div32k" };
static const char *dcxo_parents[] = { "dcxo24M", "dcxo19_2M", "dcxo26M", "dcxo24M" };

static struct sunxi_ccu_clk sun60i_w2_rtc_ccu_clks[] = {
	SUNXI_CCU_GATE(W2_CLK_IOSC, "iosc", "rc-16m",
	    XO_CTRL0_REG, 0),

	SUNXI_CCU_GATE(W2_CLK_EXT32K_GATE, "ext32k-gate", "ext-32k",
	    LOSC_CTRL_REG, 4),	// XXX with key

	SUNXI_CCU_FIXED_FACTOR(W2_CLK_IOSC_DIV32K, "iosc-div32k", "iosc", 500, 1),

	SUNXI_CCU_MUX(W2_CLK_OSC32K, "osc32k", osc32k_parents,
	    LOSC_CTRL_REG,		/* reg */
	    __BIT(1),			/* sel */
	    0),

	SUNXI_CCU_GATE_FIXED_RATE(W2_CLK_DCXO24M_DIV32K, "dcxo24M-div32k", "dcxo",
	    CLK32K_FOUT_CTRL_GATING_REG,	/* reg */
	    16,					/* bit */
	    32768),				/* fixed_rate */

	SUNXI_CCU_DIV_GATE(W2_CLK_RTC32K, "rtc32k", rtc32k_parents,
	    LOSC_CTRL_REG,			/* reg */
	    0,					/* div */
	    __BIT(1),				/* sel */
	    __BIT(0),				/* enable */
	    0),	// XXX with key

	SUNXI_CCU_FIXED_FACTOR(W2_CLK_RTC_1K,
	    "rtc-1k", "rtc-32k",
	    32, 1),

	SUNXI_CCU_DIV_GATE(W2_CLK_RTC_32K_FANOUT, "rtc-32k-fanout", rtc_32k_fanout_parents,
	    CLK32K_FOUT_CTRL_GATING_REG,	/* reg */
	    0,					/* div */
	    __BITS(2, 1),			/* sel */
	    __BIT(0),				/* enable */
	    0),

	SUNXI_CCU_GATE(W2_CLK_RTC_SPI, "rtc-spi", "r-ahb", 
	    RTC_SPI_CLK_CTRL_REG, 31),

	SUNXI_CCU_MUX(W2_CLK_DCXO, "dcxo", dcxo_parents,
	    XO_CTRL0_REG,		/* reg */
	    __BITS(15, 14),		/* sel */
	    0),
};

static int
sun60i_w2_rtc_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun60i_w2_rtc_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = NULL;
	sc->sc_nresets = 0;

	sc->sc_clks = sun60i_w2_rtc_ccu_clks;
	sc->sc_nclks = __arraycount(sun60i_w2_rtc_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": W2 RTC CCU\n");

	sunxi_ccu_print(sc);
}
