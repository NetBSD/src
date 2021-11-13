/* $NetBSD: rk3288_cru.c,v 1.5 2021/11/13 11:46:32 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(1, "$NetBSD: rk3288_cru.c,v 1.5 2021/11/13 11:46:32 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/rockchip/rk_cru.h>
#include <arm/rockchip/rk3288_cru.h>

#define	PLL_CON(n)	(0x0000 + (n) * 4)
#define	MODE_CON	0x0050
#define	CLKSEL_CON(n)	(0x0060 + (n) * 4)
#define	CLKGATE_CON(n)	(0x0160 + (n) * 4)
#define	SOFTRST_CON(n)	(0x01b8 + (n) * 4)

#define	GRF_SOC_CON4	0x0410
#define	GRF_MAC_CON1	0x0904

static int rk3288_cru_match(device_t, cfdata_t, void *);
static void rk3288_cru_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-cru" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(rk3288_cru, sizeof(struct rk_cru_softc),
	rk3288_cru_match, rk3288_cru_attach, NULL, NULL);

static const char * pll_parents[] = { "xin24m" };
static const char * aclk_cpu_src_parents[] = { "cpll_aclk_cpu", "gpll_aclk_cpu" };
static const char * uart0_parents[] = { "clk_uart0_div", "clk_uart0_frac", "xin24m" };
static const char * uart1_parents[] = { "clk_uart1_div", "clk_uart1_frac", "xin24m" };
static const char * uart2_parents[] = { "clk_uart2_div", "clk_uart2_frac", "xin24m" };
static const char * uart3_parents[] = { "clk_uart3_div", "clk_uart3_frac", "xin24m" };
static const char * uart4_parents[] = { "clk_uart4_div", "clk_uart4_frac", "xin24m" };
static const char * mmc_parents[] = { "cpll", "gpll", "xin24m", "xin24m" };
static const char * mac_parents[] = { "mac_pll_src", "ext_gmac" };
static const char * mux_2plls_parents[] = { "cpll", "gpll" };
static const char * mux_npll_cpll_gpll_parents[] = { "npll", "cpll", "gpll" };
static const char * mux_3plls_usb_parents[] = { "cpll", "gpll", "usbphy480m_src", "npll" };

static struct rk_cru_pll_rate rk3288_pll_rates[] = {
	/* TODO */
};

static struct rk_cru_clk rk3288_cru_clks[] = {
	RK3288_PLL(RK3288_PLL_CPLL, "cpll", pll_parents,
		   PLL_CON(8),		/* con_base */
		   MODE_CON,		/* mode_reg */
		   __BIT(8),		/* mode_mask */
		   __BIT(2),		/* lock_mask */
		   rk3288_pll_rates),
	RK3288_PLL(RK3288_PLL_GPLL, "gpll", pll_parents,
		   PLL_CON(12),		/* con_base */
		   MODE_CON,		/* mode_reg */
		   __BIT(12),		/* mode_mask */
		   __BIT(3),		/* lock_mask */
		   rk3288_pll_rates),
	RK3288_PLL(RK3288_PLL_NPLL, "npll", pll_parents,
		   PLL_CON(16),		/* con_base */
		   MODE_CON,		/* mode_reg */
		   __BIT(14),		/* mode_mask */
		   __BIT(4),		/* lock_mask */
		   rk3288_pll_rates),

	RK_COMPOSITE_NOGATE(0, "aclk_cpu_src", aclk_cpu_src_parents,
			    CLKSEL_CON(1),	/* muxdiv_reg */
			    __BIT(15),		/* mux_mask */
			    __BITS(7,3),	/* div_mask */
			    0),
	RK_COMPOSITE_NOMUX(RK3288_ACLK_CPU, "aclk_cpu", "aclk_cpu_pre",
			   CLKSEL_CON(1),	/* div_reg */
			   __BITS(9,8),		/* div_mask */
			   CLKGATE_CON(0),	/* gate_reg */
			   __BIT(4),		/* gate_mask */
			   0),
        RK_COMPOSITE_NOMUX(RK3288_PCLK_CPU, "pclk_cpu", "aclk_cpu_pre",
			   CLKSEL_CON(1),	/* div_reg */
			   __BITS(14,12),	/* div_mask */
			   CLKGATE_CON(0),	/* gate_reg */
			   __BIT(5),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(RK3288_HCLK_PERI, "hclk_peri", "aclk_peri_src",
			   CLKSEL_CON(10),	/* div_reg */
			   __BITS(9,8),		/* div_mask */
			   CLKGATE_CON(2),	/* gate_reg */
			   __BIT(2),		/* gate_mask */
			   RK_COMPOSITE_POW2),
	RK_COMPOSITE(0, "aclk_peri_src", mux_2plls_parents,
		     CLKSEL_CON(10),		/* muxdiv_reg */
		     __BIT(15),			/* mux_mask */
		     __BITS(4,0),		/* div_mask */
		     CLKGATE_CON(2),		/* gate_reg */
		     __BIT(0),			/* gate_mask */
		     0),
	RK_COMPOSITE_NOMUX(0, "pclk_pd_pmu", "gpll",
			   CLKSEL_CON(33),	/* div_reg */
			   __BITS(4,0),		/* div_mask */
			   CLKGATE_CON(5),	/* gate_reg */
			   __BIT(8),		/* gate_mask */
			   0),
	RK_COMPOSITE_NOMUX(RK3288_PCLK_PERI, "pclk_peri", "aclk_peri_src",
			   CLKSEL_CON(10),	/* div_reg */
			   __BITS(13,12),	/* div_mask */
			   CLKGATE_CON(2),	/* gate_reg */
			   __BIT(3),		/* gate_mask */
			   0),

	/* UARTs */
	RK_COMPOSITE(0, "clk_uart0_div", mux_3plls_usb_parents,
		     CLKSEL_CON(13),		/* muxdiv_reg */
		     __BITS(14,13),		/* mux_mask */
		     __BITS(6,0),		/* div_mask */
		     CLKGATE_CON(1),		/* gate_reg */
		     __BIT(8),			/* gate_mask */
		     0),
	/* XXX TODO: CRU_CLKGATE1_CON bit 9 (clk_uart0_frac_src_gate_en) */
	RK_COMPOSITE_FRAC(0, "clk_uart0_frac", "clk_uart0_div",
			  CLKSEL_CON(17),	/* fracdiv_reg */
			  0),
	RK_COMPOSITE_NOMUX(0, "clk_uart1_div", "uart_src",
			   CLKSEL_CON(14),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(1),	/* gate_reg */
			   __BIT(10),		/* gate_mask */
			   0),
	/* XXX TODO: CRU_CLKGATE1_CON bit 11 (clk_uart1_frac_src_gate_en) */
	RK_COMPOSITE_FRAC(0, "clk_uart1_frac", "clk_uart1_div",
			  CLKSEL_CON(18),	/* fracdiv_reg */
			  0),
	RK_COMPOSITE_NOMUX(0, "clk_uart2_div", "uart_src",
			   CLKSEL_CON(15),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(1),	/* gate_reg */
			   __BIT(12),		/* gate_mask */
			   0),
	/* XXX TODO: CRU_CLKGATE1_CON bit 13 (clk_uart2_frac_src_gate_en) */
	RK_COMPOSITE_FRAC(0, "clk_uart2_frac", "clk_uart2_div",
			  CLKSEL_CON(19),	/* fracdiv_reg */
			  0),
	RK_COMPOSITE_NOMUX(0, "clk_uart3_div", "uart_src",
			   CLKSEL_CON(16),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(1),	/* gate_reg */
			   __BIT(14),		/* gate_mask */
			   0),
	/* XXX TODO: CRU_CLKGATE1_CON bit 15 (clk_uart3_frac_src_gate_en) */
	RK_COMPOSITE_FRAC(0, "clk_uart3_frac", "clk_uart3_div",
			  CLKSEL_CON(20),	/* fracdiv_reg */
			  0),
	RK_COMPOSITE_NOMUX(0, "clk_uart4_div", "uart_src",
			   CLKSEL_CON(3),	/* div_reg */
			   __BITS(6,0),		/* div_mask */
			   CLKGATE_CON(2),	/* gate_reg */
			   __BIT(12),		/* gate_mask */
			   0),
	/* XXX TODO: CRU_CLKGATE2_CON bit 13 (clk_uart4_frac_src_gate_en) */
	RK_COMPOSITE_FRAC(0, "clk_uart4_frac", "clk_uart4_div",
			  CLKSEL_CON(7),	/* fracdiv_reg */
			  0),

	/* SD/eMMC/SDIO */
	RK_COMPOSITE(RK3288_SCLK_SDMMC, "sclk_sdmmc", mmc_parents,
		     CLKSEL_CON(11),		/* muxdiv_reg */
		     __BITS(7,6),		/* mux_mask */
		     __BITS(5,0),		/* div_mask */
		     CLKGATE_CON(13),		/* gate_reg */
		     __BIT(0),			/* gate_mask */
		     0),
	RK_COMPOSITE(RK3288_SCLK_SDIO0, "sclk_sdio0", mmc_parents,
		     CLKSEL_CON(12),		/* muxdiv_reg */
		     __BITS(7,6),		/* mux_mask */
		     __BITS(5,0),		/* div_mask */
		     CLKGATE_CON(13),		/* gate_reg */
		     __BIT(1),			/* gate_mask */
		     0),
	RK_COMPOSITE(RK3288_SCLK_SDIO1, "sclk_sdio1", mmc_parents,
		     CLKSEL_CON(34),		/* muxdiv_reg */
		     __BITS(15,14),		/* mux_mask */
		     __BITS(13,8),		/* div_mask */
		     CLKGATE_CON(13),		/* gate_reg */
		     __BIT(2),			/* gate_mask */
		     0),
	RK_COMPOSITE(RK3288_SCLK_EMMC, "sclk_emmc", mmc_parents,
		     CLKSEL_CON(12),		/* muxdiv_reg */
		     __BITS(15,14),		/* mux_mask */
		     __BITS(13,8),		/* div_mask */
		     CLKGATE_CON(13),		/* gate_reg */
		     __BIT(3),			/* gate_mask */
		     0),

	/* MAC */
	RK_COMPOSITE(0, "mac_pll_src", mux_npll_cpll_gpll_parents,
		     CLKSEL_CON(21),		/* muxdiv_reg */
		     __BITS(1,0),		/* mux_mask */
		     __BITS(12,8),		/* div_mask */
		     CLKGATE_CON(2),		/* gate_reg */
		     __BIT(5),			/* gate_mask */
		     0),

	/* Crypto */
	RK_COMPOSITE_NOMUX(RK3288_SCLK_CRYPTO, "crypto", "aclk_cpu_pre",
			   CLKSEL_CON(26),	/* div_reg */
			   __BITS(7,6),		/* div_mask */
			   CLKGATE_CON(5),	/* gate_reg */
			   __BIT(4),		/* gate_mask */
			   0),

	/* SPI */
	RK_COMPOSITE(RK3288_SCLK_SPI0, "sclk_spi0", mux_2plls_parents,
		     CLKSEL_CON(25),		/* muxdiv_reg */
		     __BIT(7),			/* mux_mask */
		     __BITS(6,0),		/* div_mask */
		     CLKGATE_CON(2),		/* gate_reg */
		     __BIT(9),			/* gate_mask */
		     0),
	RK_COMPOSITE(RK3288_SCLK_SPI1, "sclk_spi1", mux_2plls_parents,
		     CLKSEL_CON(25),		/* muxdiv_reg */
		     __BIT(15),			/* mux_mask */
		     __BITS(14,8),		/* div_mask */
		     CLKGATE_CON(2),		/* gate_reg */
		     __BIT(10),			/* gate_mask */
		     0),
	RK_COMPOSITE(RK3288_SCLK_SPI2, "sclk_spi2", mux_2plls_parents,
		     CLKSEL_CON(39),		/* muxdiv_reg */
		     __BIT(7),			/* mux_mask */
		     __BITS(6,0),		/* div_mask */
		     CLKGATE_CON(2),		/* gate_reg */
		     __BIT(11),			/* gate_mask */
		     0),

	/* TSADC */
	RK_COMPOSITE_NOMUX(RK3288_SCLK_TSADC, "sclk_tsadc", "xin32k",
			   CLKSEL_CON(2),	/* div_reg */
			   __BITS(5,0),		/* div_mask */
			   CLKGATE_CON(2),	/* gate_reg */
			   __BIT(7),		/* gate_mask */
			   0),

	RK_DIV(0, "aclk_cpu_pre", "aclk_cpu_src", CLKSEL_CON(1), __BITS(2,0), 0),
	RK_DIV(0, "clk_24m", "xin24m", CLKSEL_CON(2), __BITS(12,8), 0),
	RK_DIV(0, "pclk_pd_alive", "gpll", CLKSEL_CON(33), __BITS(12,8), 0),

	RK_MUX(RK3288_SCLK_UART0, "sclk_uart0", uart0_parents, CLKSEL_CON(13), __BITS(9,8)),
	RK_MUX(RK3288_SCLK_UART1, "sclk_uart1", uart1_parents, CLKSEL_CON(14), __BITS(9,8)),
	RK_MUX(RK3288_SCLK_UART2, "sclk_uart2", uart2_parents, CLKSEL_CON(15), __BITS(9,8)),
	RK_MUX(RK3288_SCLK_UART3, "sclk_uart3", uart3_parents, CLKSEL_CON(16), __BITS(9,8)),
	RK_MUX(RK3288_SCLK_UART4, "sclk_uart4", uart4_parents, CLKSEL_CON(3), __BITS(9,8)),
	RK_MUX(0, "uart_src", mux_2plls_parents, CLKSEL_CON(15), __BIT(15)),
	RK_MUX(RK3288_SCLK_MAC, "mac_clk", mac_parents, CLKSEL_CON(21), __BIT(4)),

	RK_GATE(RK3288_ACLK_CPU, "aclk_cpu", "aclk_cpu_pre", CLKGATE_CON(0), 3),
	RK_GATE(0, "gpll_aclk_cpu", "gpll", CLKGATE_CON(0), 10),
	RK_GATE(0, "cpll_aclk_cpu", "cpll", CLKGATE_CON(0), 11),
	RK_GATE(RK3288_ACLK_PERI, "aclk_peri", "aclk_peri_src", CLKGATE_CON(2), 1),
	RK_GATE(RK3288_SCLK_MAC_RX, "sclk_mac_rx", "mac_clk", CLKGATE_CON(5), 0),
	RK_GATE(RK3288_SCLK_MAC_TX, "sclk_mac_tx", "mac_clk", CLKGATE_CON(5), 1),
	RK_GATE(RK3288_SCLK_MACREF, "sclk_macref", "mac_clk", CLKGATE_CON(5), 2),
	RK_GATE(RK3288_SCLK_MACREF_OUT, "sclk_macref_out", "mac_clk", CLKGATE_CON(5), 3),
	RK_GATE(RK3288_PCLK_SPI0, "pclk_spi0", "pclk_peri", CLKGATE_CON(6), 4),
	RK_GATE(RK3288_PCLK_SPI1, "pclk_spi1", "pclk_peri", CLKGATE_CON(6), 5),
	RK_GATE(RK3288_PCLK_SPI2, "pclk_spi2", "pclk_peri", CLKGATE_CON(6), 6),
	RK_GATE(RK3288_PCLK_I2C1, "pclk_i2c1", "pclk_peri", CLKGATE_CON(6), 13),
	RK_GATE(RK3288_PCLK_I2C3, "pclk_i2c3", "pclk_peri", CLKGATE_CON(6), 14),
	RK_GATE(RK3288_PCLK_I2C4, "pclk_i2c4", "pclk_peri", CLKGATE_CON(6), 15),
	RK_GATE(RK3288_PCLK_I2C5, "pclk_i2c5", "pclk_peri", CLKGATE_CON(7), 0),
	RK_GATE(RK3288_PCLK_TSADC, "pclk_tsadc", "pclk_peri", CLKGATE_CON(7), 2),
	RK_GATE(RK3288_HCLK_USBHOST0, "hclk_host0", "hclk_peri", CLKGATE_CON(7), 6),
	RK_GATE(RK3288_HCLK_USBHOST1, "hclk_host1", "hclk_peri", CLKGATE_CON(7), 7),
	RK_GATE(RK3288_HCLK_HSIC, "hclk_hsic", "hclk_peri", CLKGATE_CON(7), 8),
	RK_GATE(RK3288_PCLK_GMAC, "pclk_gmac", "pclk_peri", CLKGATE_CON(8), 1),
	RK_GATE(RK3288_HCLK_SDMMC, "hclk_sdmmc", "hclk_peri", CLKGATE_CON(8), 3),
	RK_GATE(RK3288_HCLK_SDIO0, "hclk_sdio0", "hclk_peri", CLKGATE_CON(8), 4),
	RK_GATE(RK3288_HCLK_SDIO1, "hclk_sdio1", "hclk_peri", CLKGATE_CON(8), 5),
	RK_GATE(RK3288_ACLK_GMAC, "aclk_gmac", "aclk_peri", CLKGATE_CON(8), 0),
	RK_GATE(RK3288_HCLK_EMMC, "hclk_emmc", "hclk_peri", CLKGATE_CON(8), 6),
	RK_GATE(RK3288_PCLK_I2C0, "pclk_i2c0", "pclk_cpu", CLKGATE_CON(10), 2),
	RK_GATE(RK3288_PCLK_I2C2, "pclk_i2c2", "pclk_cpu", CLKGATE_CON(10), 3),
	RK_GATE(RK3288_ACLK_DMAC1, "aclk_dmac1", "aclk_cpu", CLKGATE_CON(10), 12),
	RK_GATE(RK3288_ACLK_CRYPTO, "aclk_crypto", "aclk_cpu", CLKGATE_CON(11), 6),
	RK_GATE(RK3288_HCLK_CRYPTO, "hclk_crypto", "hclk_cpu", CLKGATE_CON(11), 7),
	RK_GATE(RK3288_PCLK_UART2, "pclk_uart2", "pclk_cpu", CLKGATE_CON(11), 9),
	RK_GATE(RK3288_PCLK_RKPWM, "pclk_rkpwm", "pclk_cpu", CLKGATE_CON(11), 11),
	RK_GATE(RK3288_SCLK_OTGPHY0, "sclk_otgphy0", "xin24m", CLKGATE_CON(13), 4),
	RK_GATE(RK3288_SCLK_OTGPHY1, "sclk_otgphy1", "xin24m", CLKGATE_CON(13), 5),
	RK_GATE(RK3288_SCLK_OTGPHY2, "sclk_otgphy2", "xin24m", CLKGATE_CON(13), 6),
	RK_GATE(RK3288_PCLK_GPIO1, "pclk_gpio1", "pclk_pd_alive", CLKGATE_CON(14), 1),
	RK_GATE(RK3288_PCLK_GPIO2, "pclk_gpio2", "pclk_pd_alive", CLKGATE_CON(14), 2),
	RK_GATE(RK3288_PCLK_GPIO3, "pclk_gpio3", "pclk_pd_alive", CLKGATE_CON(14), 3),
	RK_GATE(RK3288_PCLK_GPIO4, "pclk_gpio4", "pclk_pd_alive", CLKGATE_CON(14), 4),
	RK_GATE(RK3288_PCLK_GPIO5, "pclk_gpio5", "pclk_pd_alive", CLKGATE_CON(14), 5),
	RK_GATE(RK3288_PCLK_GPIO6, "pclk_gpio6", "pclk_pd_alive", CLKGATE_CON(14), 6),
	RK_GATE(RK3288_PCLK_GPIO7, "pclk_gpio7", "pclk_pd_alive", CLKGATE_CON(14), 7),
	RK_GATE(RK3288_PCLK_GPIO8, "pclk_gpio8", "pclk_pd_alive", CLKGATE_CON(14), 8),
	RK_GATE(RK3288_PCLK_GPIO0, "pclk_gpio0", "pclk_pd_pmu", CLKGATE_CON(17), 4),
	RK_SECURE_GATE(RK3288_PCLK_WDT, "pclk_wdt", "pclk_pd_alive"),
};

static int
rk3288_cru_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk3288_cru_attach(device_t parent, device_t self, void *aux)
{
	struct rk_cru_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = rk3288_cru_clks;
	sc->sc_nclks = __arraycount(rk3288_cru_clks);

	sc->sc_grf_soc_status = 0x0284;
	sc->sc_softrst_base = SOFTRST_CON(0);

	if (rk_cru_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": RK3288 CRU\n");

	rk_cru_print(sc);
}
