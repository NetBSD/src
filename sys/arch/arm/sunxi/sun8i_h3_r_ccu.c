/* $NetBSD: sun8i_h3_r_ccu.c,v 1.1 2017/09/30 12:48:58 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun8i_h3_r_ccu.c,v 1.1 2017/09/30 12:48:58 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun8i_h3_r_ccu.h>

#define	AR100_CFG_REG		0x00
#define	APB0_CFG_REG		0x0c
#define	APB0_GATE_REG		0x28
#define	APB0_RESET_REG		0xb0

static int sun8i_h3_r_ccu_match(device_t, cfdata_t, void *);
static void sun8i_h3_r_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun8i-h3-r-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_h3_r_ccu, sizeof(struct sunxi_ccu_softc),
	sun8i_h3_r_ccu_match, sun8i_h3_r_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun8i_h3_r_ccu_resets[] = {
	SUNXI_CCU_RESET(H3_R_RST_APB0_IR, APB0_RESET_REG, 1),
	SUNXI_CCU_RESET(H3_R_RST_APB0_TIMER, APB0_RESET_REG, 2),
	SUNXI_CCU_RESET(H3_R_RST_APB0_UART, APB0_RESET_REG, 4),
	SUNXI_CCU_RESET(H3_R_RST_APB0_I2C, APB0_RESET_REG, 6),
};

static const char *ar100_parents[] = { "losc", "hosc", "pll_periph0", "losc" };
static const char *apb0_parents[] = { "ahb0" };

static struct sunxi_ccu_clk sun8i_h3_r_ccu_clks[] = {
	SUNXI_CCU_PREDIV(H3_R_CLK_AR100, "ar100", ar100_parents,
	    AR100_CFG_REG,	/* reg */
	    __BITS(12,8),	/* prediv */
	    __BIT(2),		/* prediv_sel */
	    __BITS(5,4),	/* div */
	    __BITS(17,16),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_FIXED_FACTOR(H3_R_CLK_AHB0, "ahb0", "ar100", 1, 1),

	SUNXI_CCU_DIV(H3_R_CLK_APB0, "apb0", apb0_parents,
	    APB0_CFG_REG,	/* reg */
	    __BITS(1,0),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_GATE(H3_R_CLK_APB0_PIO, "apb0-pio", "apb0",
	    APB0_GATE_REG, 0),
	SUNXI_CCU_GATE(H3_R_CLK_APB0_IR, "apb0-ir", "apb0",
	    APB0_GATE_REG, 1),
	SUNXI_CCU_GATE(H3_R_CLK_APB0_TIMER, "apb0-timer", "apb0",
	    APB0_GATE_REG, 2),
	SUNXI_CCU_GATE(H3_R_CLK_APB0_UART, "apb0-uart", "apb0",
	    APB0_GATE_REG, 4),
	SUNXI_CCU_GATE(H3_R_CLK_APB0_I2C, "apb0-i2c", "apb0",
	    APB0_GATE_REG, 6),
};

static int
sun8i_h3_r_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun8i_h3_r_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun8i_h3_r_ccu_resets;
	sc->sc_nresets = __arraycount(sun8i_h3_r_ccu_resets);

	sc->sc_clks = sun8i_h3_r_ccu_clks;
	sc->sc_nclks = __arraycount(sun8i_h3_r_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": H3 PRCM CCU\n");

	sunxi_ccu_print(sc);
}
