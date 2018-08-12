/* $NetBSD: rk3328_cru.c,v 1.4 2018/08/12 16:48:04 jmcneill Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: rk3328_cru.c,v 1.4 2018/08/12 16:48:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/rockchip/rk_cru.h>
#include <arm/rockchip/rk3328_cru.h>

#define	PLL_CON(n)	(0x0000 + (n) * 4)
#define	MISC_CON	0x0084	
#define	CLKSEL_CON(n)	(0x0100 + (n) * 4)
#define	CLKGATE_CON(n)	(0x0200 + (n) * 4)
#define	SOFTRST_CON(n)	(0x0300 + (n) * 4)

#define	GRF_SOC_CON4	0x0410
#define	GRF_MAC_CON1	0x0904

static int rk3328_cru_match(device_t, cfdata_t, void *);
static void rk3328_cru_attach(device_t, device_t, void *);

static const char * const compatible[] = {
	"rockchip,rk3328-cru",
	NULL
};

CFATTACH_DECL_NEW(rk3328_cru, sizeof(struct rk_cru_softc),
	rk3328_cru_match, rk3328_cru_attach, NULL, NULL);

static const struct rk_cru_pll_rate pll_rates[] = {
        RK_PLL_RATE(1608000000,  1,  67, 1, 1, 1, 0),
        RK_PLL_RATE(1584000000,  1,  66, 1, 1, 1, 0),
        RK_PLL_RATE(1560000000,  1,  65, 1, 1, 1, 0),
        RK_PLL_RATE(1536000000,  1,  64, 1, 1, 1, 0),
        RK_PLL_RATE(1512000000,  1,  63, 1, 1, 1, 0),
        RK_PLL_RATE(1488000000,  1,  62, 1, 1, 1, 0),
        RK_PLL_RATE(1464000000,  1,  61, 1, 1, 1, 0),
        RK_PLL_RATE(1440000000,  1,  60, 1, 1, 1, 0),
        RK_PLL_RATE(1416000000,  1,  59, 1, 1, 1, 0),
        RK_PLL_RATE(1392000000,  1,  58, 1, 1, 1, 0),
        RK_PLL_RATE(1368000000,  1,  57, 1, 1, 1, 0),
        RK_PLL_RATE(1344000000,  1,  56, 1, 1, 1, 0),
        RK_PLL_RATE(1320000000,  1,  55, 1, 1, 1, 0),
        RK_PLL_RATE(1296000000,  1,  54, 1, 1, 1, 0),
        RK_PLL_RATE(1272000000,  1,  53, 1, 1, 1, 0),
        RK_PLL_RATE(1248000000,  1,  52, 1, 1, 1, 0),
        RK_PLL_RATE(1200000000,  1,  50, 1, 1, 1, 0),
        RK_PLL_RATE(1188000000,  2,  99, 1, 1, 1, 0),
        RK_PLL_RATE(1104000000,  1,  46, 1, 1, 1, 0),
        RK_PLL_RATE(1100000000, 12, 550, 1, 1, 1, 0),
        RK_PLL_RATE(1008000000,  1,  84, 2, 1, 1, 0),
        RK_PLL_RATE(1000000000,  6, 500, 2, 1, 1, 0),
        RK_PLL_RATE( 984000000,  1,  82, 2, 1, 1, 0),
        RK_PLL_RATE( 960000000,  1,  80, 2, 1, 1, 0),
        RK_PLL_RATE( 936000000,  1,  78, 2, 1, 1, 0),
        RK_PLL_RATE( 912000000,  1,  76, 2, 1, 1, 0),
        RK_PLL_RATE( 900000000,  4, 300, 2, 1, 1, 0),
        RK_PLL_RATE( 888000000,  1,  74, 2, 1, 1, 0),
        RK_PLL_RATE( 864000000,  1,  72, 2, 1, 1, 0),
        RK_PLL_RATE( 840000000,  1,  70, 2, 1, 1, 0),
        RK_PLL_RATE( 816000000,  1,  68, 2, 1, 1, 0),
        RK_PLL_RATE( 800000000,  6, 400, 2, 1, 1, 0),
        RK_PLL_RATE( 700000000,  6, 350, 2, 1, 1, 0),
        RK_PLL_RATE( 696000000,  1,  58, 2, 1, 1, 0),
        RK_PLL_RATE( 600000000,  1,  75, 3, 1, 1, 0),
        RK_PLL_RATE( 594000000,  2,  99, 2, 1, 1, 0),
        RK_PLL_RATE( 504000000,  1,  63, 3, 1, 1, 0),
        RK_PLL_RATE( 500000000,  6, 250, 2, 1, 1, 0),
        RK_PLL_RATE( 408000000,  1,  68, 2, 2, 1, 0),
        RK_PLL_RATE( 312000000,  1,  52, 2, 2, 1, 0),
        RK_PLL_RATE( 216000000,  1,  72, 4, 2, 1, 0),
        RK_PLL_RATE(  96000000,  1,  64, 4, 4, 1, 0),
};

static const struct rk_cru_pll_rate pll_frac_rates[] = {
        RK_PLL_RATE(1016064000,  3, 127, 1, 1, 0, 134217),
        RK_PLL_RATE( 983040000, 24, 983, 1, 1, 0, 671088),
        RK_PLL_RATE( 491520000, 24, 983, 2, 1, 0, 671088),
        RK_PLL_RATE(  61440000,  6, 215, 7, 2, 0, 671088),
        RK_PLL_RATE(  56448000, 12, 451, 4, 4, 0, 9797894),
        RK_PLL_RATE(  40960000, 12, 409, 4, 5, 0, 10066329),
};

static const struct rk_cru_pll_rate pll_norates[] = {
};

static const struct rk_cru_arm_rate armclk_rates[] = {
	RK_ARM_RATE(1296000000, 1),
	RK_ARM_RATE(1200000000, 1),
	RK_ARM_RATE(1104000000, 1),
	RK_ARM_RATE(1008000000, 1),
	RK_ARM_RATE( 912000000, 1),
	RK_ARM_RATE( 816000000, 1),
	RK_ARM_RATE( 696000000, 1),
	RK_ARM_RATE( 600000000, 1),
	RK_ARM_RATE( 408000000, 1),
	RK_ARM_RATE( 312000000, 1),
	RK_ARM_RATE( 216000000, 1),
	RK_ARM_RATE(  96000000, 1),
};

static const char * pll_parents[] = { "xin24m" };
static const char * armclk_parents[] = { "apll", "gpll", "dpll", "npll" };
static const char * aclk_bus_pre_parents[] = { "cpll", "gpll", "hdmiphy" };
static const char * hclk_bus_pre_parents[] = { "aclk_bus_pre" };
static const char * pclk_bus_pre_parents[] = { "aclk_bus_pre" };
static const char * aclk_peri_pre_parents[] = { "cpll", "gpll", "hdmiphy_peri" };
static const char * mmc_parents[] = { "cpll", "gpll", "xin24m", "usb480m" };
static const char * phclk_peri_parents[] = { "aclk_peri_pre" };
static const char * mux_hdmiphy_parents[] = { "hdmi_phy", "xin24m" };
static const char * mux_usb480m_parents[] = { "usb480m_phy", "xin24m" };
static const char * mux_uart0_parents[] = { "clk_uart0_div", "clk_uart0_frac", "xin24m" };
static const char * mux_uart1_parents[] = { "clk_uart1_div", "clk_uart1_frac", "xin24m" };
static const char * mux_uart2_parents[] = { "clk_uart2_div", "clk_uart2_frac", "xin24m" };
static const char * mux_mac2io_src_parents[] = { "clk_mac2io_src", "gmac_clkin" };
static const char * mux_mac2io_ext_parents[] = { "clk_mac2io", "gmac_clkin" };
static const char * mux_2plls_parents[] = { "cpll", "gpll" };
static const char * mux_2plls_hdmiphy_parents[] = { "cpll", "gpll", "dummy_hdmiphy" };
static const char * comp_uart_parents[] = { "cpll", "gpll", "usb480m" };
static const char * pclk_gmac_parents[] = { "aclk_gmac" };

static struct rk_cru_clk rk3328_cru_clks[] = {
	RK_PLL(RK3328_PLL_APLL, "apll", pll_parents,
	       PLL_CON(0),		/* con_base */
	       0x80,			/* mode_reg */
	       __BIT(0),		/* mode_mask */
	       __BIT(4),		/* lock_mask */
	       pll_frac_rates),
	RK_PLL(RK3328_PLL_DPLL, "dpll", pll_parents,
	       PLL_CON(8),		/* con_base */
	       0x80,			/* mode_reg */
	       __BIT(4),		/* mode_mask */
	       __BIT(3),		/* lock_mask */
	       pll_norates),
	RK_PLL(RK3328_PLL_CPLL, "cpll", pll_parents,
	       PLL_CON(16),		/* con_base */
	       0x80,			/* mode_reg */
	       __BIT(8),		/* mode_mask */
	       __BIT(2),		/* lock_mask */
	       pll_rates),
	RK_PLL(RK3328_PLL_GPLL, "gpll", pll_parents,
	       PLL_CON(24),		/* con_base */
	       0x80,			/* mode_reg */
	       __BIT(12),		/* mode_mask */
	       __BIT(1),		/* lock_mask */
	       pll_frac_rates),
	RK_PLL(RK3328_PLL_NPLL, "npll", pll_parents,
	       PLL_CON(40),		/* con_base */
	       0x80,			/* mode_reg */
	       __BIT(1),		/* mode_mask */
	       __BIT(0),		/* lock_mask */
	       pll_rates),

	RK_ARM(RK3328_ARMCLK, "armclk", armclk_parents,
	       CLKSEL_CON(0),		/* reg */
	       __BITS(7,6), 3, 1,	/* mux_mask, mux_main, mux_alt */
	       __BITS(4,0),		/* div_mask */
	       armclk_rates),

	RK_COMPOSITE(RK3328_ACLK_BUS_PRE, "aclk_bus_pre", aclk_bus_pre_parents,
		     CLKSEL_CON(0),	/* muxdiv_reg */
		     __BITS(14,13),	/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_HCLK_BUS_PRE, "hclk_bus_pre", hclk_bus_pre_parents,
		     CLKSEL_CON(1),	/* muxdiv_reg */
		     0,			/* mux_mask */
		     __BITS(9,8),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_PCLK_BUS_PRE, "pclk_bus_pre", pclk_bus_pre_parents,
		     CLKSEL_CON(1),	/* muxdiv_reg */
		     0,			/* mux_mask */
		     __BITS(14,12),	/* div_mask */
		     CLKGATE_CON(8),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_ACLK_PERI_PRE, "aclk_peri_pre", aclk_peri_pre_parents,
		     CLKSEL_CON(28),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     0,	0,		/* gate_reg, gate_mask */
		     0),
	RK_COMPOSITE(RK3328_PCLK_PERI, "pclk_peri", phclk_peri_parents,
		     CLKSEL_CON(29),	/* muxdiv_reg */
		     0,			/* mux_mask */
		     __BITS(1,0),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_HCLK_PERI, "hclk_peri", phclk_peri_parents,
		     CLKSEL_CON(29),	/* muxdiv_reg */
		     0,			/* mux_mask */
		     __BITS(6,4),	/* div_mask */
		     CLKGATE_CON(10),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_SDMMC, "clk_sdmmc", mmc_parents,
		     CLKSEL_CON(30),		/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(7,0),	/* div_mask */
		     CLKGATE_CON(4),	/* gate_reg */
		     __BIT(3),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_COMPOSITE(RK3328_SCLK_SDIO, "clk_sdio", mmc_parents,
		     CLKSEL_CON(31),	/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(7,0),	/* div_mask */
		     CLKGATE_CON(4),	/* gate_reg */
		     __BIT(4),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_COMPOSITE(RK3328_SCLK_EMMC, "clk_emmc", mmc_parents,
		     CLKSEL_CON(32),	/* muxdiv_reg */
		     __BITS(9,8),	/* mux_mask */
		     __BITS(7,0),	/* div_mask */
		     CLKGATE_CON(4),	/* gate_reg */
		     __BIT(5),		/* gate_mask */
		     RK_COMPOSITE_ROUND_DOWN),
	RK_COMPOSITE(0, "clk_uart0_div", comp_uart_parents,
		     CLKSEL_CON(14),	/* muxdiv_reg */
		     __BITS(13,12),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(1),	/* gate_reg */
		     __BIT(14),		/* gate_mask */
		     0),
	RK_COMPOSITE(0, "clk_uart1_div", comp_uart_parents,
		     CLKSEL_CON(16),	/* muxdiv_reg */
		     __BITS(13,12),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE(0, "clk_uart2_div", comp_uart_parents,
		     CLKSEL_CON(18),	/* muxdiv_reg */
		     __BITS(13,12),	/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_ACLK_GMAC, "aclk_gmac", mux_2plls_hdmiphy_parents,
		     CLKSEL_CON(35),	/* muxdiv_reg */
		     __BITS(7,6),	/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(3),	/* gate_reg */
		     __BIT(2),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_PCLK_GMAC, "pclk_gmac", pclk_gmac_parents,
		     CLKSEL_CON(25),	/* muxdiv_reg */
		     0,			/* mux_mask */
		     __BITS(10,8),	/* div_mask */
		     CLKGATE_CON(9),	/* gate_reg */
		     __BIT(0),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_MAC2IO_SRC, "clk_mac2io_src", mux_2plls_parents,
		     CLKSEL_CON(27),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(4,0),	/* div_mask */
		     CLKGATE_CON(3),	/* gate_reg */
		     __BIT(1),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_MAC2IO_OUT, "clk_mac2io_out", mux_2plls_parents,
		     CLKSEL_CON(27),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(12,8),	/* div_mask */
		     CLKGATE_CON(3),	/* gate_reg */
		     __BIT(5),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_I2C0, "clk_i2c0", mux_2plls_parents,
		     CLKSEL_CON(34),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(9),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_I2C1, "clk_i2c1", mux_2plls_parents,
		     CLKSEL_CON(34),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(10),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_I2C2, "clk_i2c2", mux_2plls_parents,
		     CLKSEL_CON(35),	/* muxdiv_reg */
		     __BIT(7),		/* mux_mask */
		     __BITS(6,0),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(11),		/* gate_mask */
		     0),
	RK_COMPOSITE(RK3328_SCLK_I2C3, "clk_i2c3", mux_2plls_parents,
		     CLKSEL_CON(35),	/* muxdiv_reg */
		     __BIT(15),		/* mux_mask */
		     __BITS(14,8),	/* div_mask */
		     CLKGATE_CON(2),	/* gate_reg */
		     __BIT(12),		/* gate_mask */
		     0),

	RK_GATE(0, "apll_core", "apll", CLKGATE_CON(0), 0),
	RK_GATE(0, "dpll_core", "dpll", CLKGATE_CON(0), 1),
	RK_GATE(0, "gpll_core", "gpll", CLKGATE_CON(0), 2),
	RK_GATE(0, "npll_core", "npll", CLKGATE_CON(0), 12),
	RK_GATE(0, "gpll_peri", "gpll", CLKGATE_CON(4), 0),
	RK_GATE(0, "cpll_peri", "cpll", CLKGATE_CON(4), 1),
	RK_GATE(0, "hdmiphy_peri", "hdmiphy", CLKGATE_CON(4), 2),
	RK_GATE(0, "pclk_bus", "pclk_bus_pre", CLKGATE_CON(8), 3),
	RK_GATE(0, "pclk_phy_pre", "pclk_bus_pre", CLKGATE_CON(8), 4),
	RK_GATE(RK3328_ACLK_PERI, "aclk_peri", "aclk_peri_pre", CLKGATE_CON(10), 0),
	RK_GATE(RK3328_PCLK_I2C0, "pclk_i2c0", "pclk_bus", CLKGATE_CON(15), 10),
	RK_GATE(RK3328_PCLK_I2C1, "pclk_i2c1", "pclk_bus", CLKGATE_CON(16), 0),
	RK_GATE(RK3328_PCLK_I2C2, "pclk_i2c2", "pclk_bus", CLKGATE_CON(16), 1),
	RK_GATE(RK3328_PCLK_I2C3, "pclk_i2c3", "pclk_bus", CLKGATE_CON(16), 2),
	RK_GATE(RK3328_PCLK_GPIO0, "pclk_gpio0", "pclk_bus", CLKGATE_CON(16), 7),
	RK_GATE(RK3328_PCLK_GPIO1, "pclk_gpio1", "pclk_bus", CLKGATE_CON(16), 8),
	RK_GATE(RK3328_PCLK_GPIO2, "pclk_gpio2", "pclk_bus", CLKGATE_CON(16), 9),
	RK_GATE(RK3328_PCLK_GPIO3, "pclk_gpio3", "pclk_bus", CLKGATE_CON(16), 10),
	RK_GATE(RK3328_PCLK_UART0, "pclk_uart0", "pclk_bus", CLKGATE_CON(16), 11),
	RK_GATE(RK3328_PCLK_UART1, "pclk_uart1", "pclk_bus", CLKGATE_CON(16), 12),
	RK_GATE(RK3328_PCLK_UART2, "pclk_uart2", "pclk_bus", CLKGATE_CON(16), 13),
	RK_GATE(RK3328_SCLK_MAC2IO_REF, "clk_mac2io_ref", "clk_mac2io", CLKGATE_CON(9), 7),
	RK_GATE(RK3328_SCLK_MAC2IO_RX, "clk_mac2io_rx", "clk_mac2io", CLKGATE_CON(9), 4),
	RK_GATE(RK3328_SCLK_MAC2IO_TX, "clk_mac2io_tx", "clk_mac2io", CLKGATE_CON(9), 5),
	RK_GATE(RK3328_SCLK_MAC2IO_REFOUT, "clk_mac2io_refout", "clk_mac2io", CLKGATE_CON(9), 6),
	RK_GATE(0, "pclk_phy_niu", "pclk_phy_pre", CLKGATE_CON(15), 15),
	RK_GATE(RK3328_ACLK_USB3OTG, "aclk_usb3otg", "aclk_peri", CLKGATE_CON(19), 4),
	RK_GATE(RK3328_HCLK_SDMMC, "hclk_sdmmc", "hclk_peri", CLKGATE_CON(19), 0),
	RK_GATE(RK3328_HCLK_SDIO, "hclk_sdio", "hclk_peri", CLKGATE_CON(19), 1),
	RK_GATE(RK3328_HCLK_EMMC, "hclk_emmc", "hclk_peri", CLKGATE_CON(19), 2),
	RK_GATE(RK3328_HCLK_SDMMC_EXT, "hclk_sdmmc_ext", "hclk_peri", CLKGATE_CON(19), 15),
	RK_GATE(RK3328_HCLK_HOST0, "hclk_host0", "hclk_peri", CLKGATE_CON(19), 6),
	RK_GATE(RK3328_HCLK_HOST0_ARB, "hclk_host0_arb", "hclk_peri", CLKGATE_CON(19), 7),
	RK_GATE(RK3328_HCLK_OTG, "hclk_otg", "hclk_peri", CLKGATE_CON(19), 8),
	RK_GATE(RK3328_HCLK_OTG_PMU, "hclk_otg_pmu", "hclk_peri", CLKGATE_CON(19), 9),
	RK_GATE(RK3328_ACLK_MAC2IO, "aclk_mac2io", "aclk_gmac", CLKGATE_CON(26), 2),
	RK_GATE(RK3328_PCLK_MAC2IO, "pclk_mac2io", "pclk_gmac", CLKGATE_CON(26), 3),
	RK_GATE(0, "aclk_gmac_niu", "aclk_gmac", CLKGATE_CON(26), 4),
	RK_GATE(0, "pclk_gmac_niu", "pclk_gmac", CLKGATE_CON(26), 5),

	RK_MUX(RK3328_HDMIPHY, "hdmiphy", mux_hdmiphy_parents, MISC_CON, __BIT(13)),
	RK_MUX(RK3328_USB480M, "usb480m", mux_usb480m_parents, MISC_CON, __BIT(15)),
	RK_MUX(RK3328_SCLK_UART0, "sclk_uart0", mux_uart0_parents, CLKSEL_CON(14), __BITS(9,8)),
	RK_MUX(RK3328_SCLK_UART1, "sclk_uart1", mux_uart1_parents, CLKSEL_CON(16), __BITS(9,8)),
	RK_MUX(RK3328_SCLK_UART2, "sclk_uart2", mux_uart2_parents, CLKSEL_CON(18), __BITS(9,8)),
	RK_MUXGRF(RK3328_SCLK_MAC2IO, "clk_mac2io", mux_mac2io_src_parents, GRF_MAC_CON1, __BIT(10)),
	RK_MUXGRF(RK3328_SCLK_MAC2IO_EXT, "clk_mac2io_ext", mux_mac2io_ext_parents, GRF_SOC_CON4, __BIT(14)),
};

static int
rk3328_cru_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
rk3328_cru_attach(device_t parent, device_t self, void *aux)
{
	struct rk_cru_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = rk3328_cru_clks;
	sc->sc_nclks = __arraycount(rk3328_cru_clks);

	sc->sc_softrst_base = SOFTRST_CON(0);

	if (rk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": RK3328 CRU\n");

	rk_cru_print(sc);
}
