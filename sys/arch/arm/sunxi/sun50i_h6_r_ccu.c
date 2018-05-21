/* $NetBSD: sun50i_h6_r_ccu.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sun50i_h6_r_ccu.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_ccu.h>
#include <arm/sunxi/sun50i_h6_r_ccu.h>

#define	AR100_CFG_REG		0x00
#define	APB1_CFG_REG		0x0c
#define	APB2_CFG_REG		0x10
#define	IR_CFG_REG		0x1c0
#define	W1_CFG_REG		0x1e0

static int sun50i_h6_r_ccu_match(device_t, cfdata_t, void *);
static void sun50i_h6_r_ccu_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"allwinner,sun50i-h6-r-ccu",
	NULL
};

CFATTACH_DECL_NEW(sunxi_h6_r_ccu, sizeof(struct sunxi_ccu_softc),
	sun50i_h6_r_ccu_match, sun50i_h6_r_ccu_attach, NULL, NULL);

static struct sunxi_ccu_reset sun50i_h6_r_ccu_resets[] = {
	SUNXI_CCU_RESET(H6_R_RST_APB1_TIMER, 0x11c, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB1_TWD, 0x12c, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB1_PWM, 0x13c, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB2_UART, 0x18c, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB2_I2C, 0x19c, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB1_IR, 0x1cc, 16),
	SUNXI_CCU_RESET(H6_R_RST_APB1_W1, 0x1ec, 16),
};

static const char *ar100_parents[] = { "hosc", "losc", "pll-periph", "iosc" };
static const char *apb1_parents[] = { "ahb" };
static const char *apb2_parents[] = { "hosc", "losc", "pll-periph", "iosc" };
static const char *mod_parents[] = { "losc", "hosc" };

static struct sunxi_ccu_clk sun50i_h6_r_ccu_clks[] = {
	SUNXI_CCU_PREDIV(H6_R_CLK_AR100, "ar100", ar100_parents,
	    AR100_CFG_REG,	/* reg */
	    __BITS(4,0),	/* prediv */
	    __BIT(2),		/* prediv_sel */
	    __BITS(9,8),	/* div */
	    __BITS(25,24),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_FIXED_FACTOR(H6_R_CLK_AHB, "ahb", "ar100", 1, 1),

	SUNXI_CCU_DIV(H6_R_CLK_APB1, "apb1", apb1_parents,
	    APB1_CFG_REG,	/* reg */
	    __BITS(1,0),	/* div */
	    0,			/* sel */
	    SUNXI_CCU_DIV_POWER_OF_TWO),

	SUNXI_CCU_PREDIV(H6_R_CLK_APB2, "apb2", apb2_parents,
	    APB2_CFG_REG,	/* reg */
	    __BITS(4,0),	/* prediv */
	    __BIT(2),		/* prediv_sel */
	    __BITS(9,8),	/* div */
	    __BITS(25,24),	/* sel */
	    SUNXI_CCU_PREDIV_POWER_OF_TWO),

	SUNXI_CCU_GATE(H6_R_CLK_APB1_TIMER, "apb1-timer", "apb1", 0x11c, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB1_TWD, "apb1-twd", "apb1", 0x12c, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB1_PWM, "apb1-pwm", "apb1", 0x13c, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB2_UART, "apb2-uart", "apb2", 0x18c, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB2_I2C, "apb2-i2c", "apb2", 0x19c, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB1_IR, "apb1-ir", "apb1", 0x1cc, 0),
	SUNXI_CCU_GATE(H6_R_CLK_APB1_W1, "apb1-w1", "apb1", 0x1ec, 0),

	SUNXI_CCU_NM(H6_R_CLK_IR, "ir", mod_parents,
	    IR_CFG_REG,		/* reg */
	    __BITS(4,0),	/* n */
	    __BITS(9,8),	/* m */
	    __BIT(24),		/* sel */
	    __BIT(31),		/* enable */
	    0),

	SUNXI_CCU_NM(H6_R_CLK_W1, "w1", mod_parents,
	    W1_CFG_REG,		/* reg */
	    __BITS(4,0),	/* n */
	    __BITS(9,8),	/* m */
	    __BIT(24),		/* sel */
	    __BIT(31),		/* enable */
	    0),
};

static int
sun50i_h6_r_ccu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun50i_h6_r_ccu_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_ccu_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_resets = sun50i_h6_r_ccu_resets;
	sc->sc_nresets = __arraycount(sun50i_h6_r_ccu_resets);

	sc->sc_clks = sun50i_h6_r_ccu_clks;
	sc->sc_nclks = __arraycount(sun50i_h6_r_ccu_clks);

	if (sunxi_ccu_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": H6 PRCM CCU\n");

	sunxi_ccu_print(sc);
}
