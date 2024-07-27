/* $NetBSD: jh7100_clkc.c,v 1.3 2024/07/27 07:09:50 skrll Exp $ */

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jh7100_clkc.c,v 1.3 2024/07/27 07:09:50 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/starfive/jh71x0_clkc.h>


#define	JH7100_CLK_CPUNDBUS_ROOT	0
#define	JH7100_CLK_DSP_ROOT		2
#define	JH7100_CLK_GMACUSB_ROOT		3
#define	JH7100_CLK_PERH0_ROOT		4
#define	JH7100_CLK_PERH1_ROOT		5
#define	JH7100_CLK_VOUT_ROOT		7
#define	JH7100_CLK_AUDIO_ROOT		8
#define	JH7100_CLK_VOUTBUS_ROOT		11
#define	JH7100_CLK_CPUNBUS_ROOT_DIV	12
#define	JH7100_CLK_DSP_ROOT_DIV		13
#define	JH7100_CLK_PERH0_SRC		14
#define	JH7100_CLK_PERH1_SRC		15
#define	JH7100_CLK_PLL2_REF		19
#define	JH7100_CLK_CPU_CORE		20
#define	JH7100_CLK_CPU_AXI		21
#define	JH7100_CLK_AHB_BUS		22
#define	JH7100_CLK_APB1_BUS		23
#define	JH7100_CLK_APB2_BUS		24
#define	JH7100_CLK_DOM7AHB_BUS		26
#define	JH7100_CLK_SGDMA2P_AXI		31
#define	JH7100_CLK_SGDMA2P_AHB		33
#define	JH7100_CLK_VP6_CORE		38
#define	JH7100_CLK_JPEG_APB		50
#define	JH7100_CLK_SGDMA1P_BUS		84
#define	JH7100_CLK_SGDMA1P_AXI		85
#define	JH7100_CLK_AUDIO_DIV		95
#define	JH7100_CLK_AUDIO_SRC		96
#define	JH7100_CLK_AUDIO_12288		97
#define	JH7100_CLK_VOUT_SRC		109
#define	JH7100_CLK_DISPBUS_SRC		110
#define	JH7100_CLK_DISP_BUS		111
#define	JH7100_CLK_DISP_AXI		112
#define	JH7100_CLK_SDIO0_AHB		114
#define	JH7100_CLK_SDIO0_CCLKINT	115
#define	JH7100_CLK_SDIO0_CCLKINT_INV	116
#define	JH7100_CLK_SDIO1_AHB		117
#define	JH7100_CLK_SDIO1_CCLKINT	118
#define	JH7100_CLK_SDIO1_CCLKINT_INV	119
#define	JH7100_CLK_GMAC_AHB		120
#define	JH7100_CLK_GMAC_ROOT_DIV	121
#define	JH7100_CLK_GMAC_PTP_REF		122
#define	JH7100_CLK_GMAC_GTX		123
#define	JH7100_CLK_GMAC_RMII_TX		124
#define	JH7100_CLK_GMAC_RMII_RX		125
#define	JH7100_CLK_GMAC_TX		126
#define	JH7100_CLK_GMAC_TX_INV		127
#define	JH7100_CLK_GMAC_RX_PRE		128
#define	JH7100_CLK_GMAC_RX_INV		129
#define	JH7100_CLK_GMAC_RMII		130
#define	JH7100_CLK_GMAC_TOPHYREF	131
#define	JH7100_CLK_QSPI_AHB		137
#define	JH7100_CLK_SEC_AHB		140
#define	JH7100_CLK_TRNG_APB		144
#define	JH7100_CLK_OTP_APB		145
#define	JH7100_CLK_UART0_APB		146
#define	JH7100_CLK_UART0_CORE		147
#define	JH7100_CLK_UART1_APB		148
#define	JH7100_CLK_UART1_CORE		149
#define	JH7100_CLK_SPI0_APB		150
#define	JH7100_CLK_SPI0_CORE		151
#define	JH7100_CLK_SPI1_APB		152
#define	JH7100_CLK_SPI1_CORE		153
#define	JH7100_CLK_I2C0_APB		154
#define	JH7100_CLK_I2C0_CORE		155
#define	JH7100_CLK_I2C1_APB		156
#define	JH7100_CLK_I2C1_CORE		157
#define	JH7100_CLK_GPIO_APB		158
#define	JH7100_CLK_UART2_APB		159
#define	JH7100_CLK_UART2_CORE		160
#define	JH7100_CLK_UART3_APB		161
#define	JH7100_CLK_UART3_CORE		162
#define	JH7100_CLK_SPI2_APB		163
#define	JH7100_CLK_SPI2_CORE		164
#define	JH7100_CLK_SPI3_APB		165
#define	JH7100_CLK_SPI3_CORE		166
#define	JH7100_CLK_I2C2_APB		167
#define	JH7100_CLK_I2C2_CORE		168
#define	JH7100_CLK_I2C3_APB		169
#define	JH7100_CLK_I2C3_CORE		170
#define	JH7100_CLK_WDTIMER_APB		171
#define	JH7100_CLK_WDT_CORE		172
#define	JH7100_CLK_PWM_APB		181

#define	JH7100_CLK_PLL0_OUT		186
#define	JH7100_CLK_PLL1_OUT		187
#define	JH7100_CLK_PLL2_OUT		188

#define	JH7100_NCLKS			189


static const char *cpundbus_root_parents[] = {
	"osc_sys", "pll0_out", "pll1_out", "pll2_out",
};

static const char *dsp_root_parents[] = {
	"osc_sys", "pll0_out", "pll1_out", "pll2_out",
};

static const char *gmacusb_root_parents[] = {
	"osc_sys", "pll0_out", "pll2_out",
};

static const char *perh0_root_parents[] = {
	"osc_sys", "pll0_out",
};

static const char *perh1_root_parents[] = {
	"osc_sys", "pll2_out",
};

static const char *pll2_refclk_parents[] = {
	"osc_sys", "osc_aud",
};

static const char *vout_root_parents[] = {
	"osc_aud", "pll0_out", "pll2_out",
};

static const char *gmac_rx_pre_parents[] = {
	"gmac_gr_mii_rxclk", "gmac_rmii_rxclk",
};

static const char *gmac_tx_parents[] = {
	"gmac_gtxclk", "gmac_tx_inv", "gmac_rmii_txclk",
};


static struct jh71x0_clkc_clk jh7100_clocks[] = {
	JH71X0CLKC_FIXED_FACTOR(JH7100_CLK_PLL0_OUT,	"pll0_out",	"osc_sys",	1, 40),
	JH71X0CLKC_FIXED_FACTOR(JH7100_CLK_PLL1_OUT,	"pll1_out",	"osc_sys",	1, 64),
	JH71X0CLKC_FIXED_FACTOR(JH7100_CLK_PLL2_OUT,	"pll2_out",	"pll2_refclk",	1, 55),

	JH71X0CLKC_MUX(JH7100_CLK_CPUNDBUS_ROOT,	"cpundbus_root", cpundbus_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_DSP_ROOT, 		"dsp_root",	 dsp_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_GMACUSB_ROOT,		"gmacusb_root",  gmacusb_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_PERH0_ROOT,   	"perh0_root",    perh0_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_PERH1_ROOT,   	"perh1_root",    perh1_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_PLL2_REF,		"pll2_refclk",   pll2_refclk_parents),

	JH71X0CLKC_MUX(JH7100_CLK_VOUT_ROOT,		"vout_root",	 vout_root_parents),
	JH71X0CLKC_MUX(JH7100_CLK_VOUTBUS_ROOT,		"voutbus_root",	 vout_root_parents),

	JH71X0CLKC_MUX_FLAGS(JH7100_CLK_GMAC_TX,	"gmac_tx",	gmac_tx_parents, CLK_SET_RATE_PARENT),

	JH71X0CLKC_MUX(JH7100_CLK_GMAC_RX_PRE,		"gmac_rx_pre",  gmac_rx_pre_parents),

	JH71X0CLKC_DIV(JH7100_CLK_CPUNBUS_ROOT_DIV,	"cpunbus_root_div",
									 2, "cpundbus_root"),
	JH71X0CLKC_DIV(JH7100_CLK_PERH0_SRC,		"perh0_src",	 4, "perh0_root"),
	JH71X0CLKC_DIV(JH7100_CLK_PERH1_SRC,		"perh1_src",	 4, "perh1_root"),

	JH71X0CLKC_DIV(JH7100_CLK_AHB_BUS,		"ahb_bus",	 8, "cpunbus_root_div"),
	JH71X0CLKC_DIV(JH7100_CLK_APB1_BUS,		"apb1_bus",	 8, "ahb_bus"),
	JH71X0CLKC_DIV(JH7100_CLK_APB2_BUS,		"apb2_bus",	 8, "ahb_bus"),
	JH71X0CLKC_DIV(JH7100_CLK_CPU_CORE,		"cpu_core",	 8, "cpunbus_root_div"),
	JH71X0CLKC_DIV(JH7100_CLK_CPU_AXI,		"cpu_axi",	 8, "cpu_core"),
	JH71X0CLKC_DIV(JH7100_CLK_GMAC_ROOT_DIV,	"gmac_root_div", 8, "gmacusb_root"),
	JH71X0CLKC_DIV(JH7100_CLK_DSP_ROOT_DIV,		"dsp_root_div",	 4, "dsp_root"),
	JH71X0CLKC_DIV(JH7100_CLK_SGDMA1P_BUS,		"sgdma1p_bus",	 8, "cpunbus_root_div"),

	JH71X0CLKC_DIV(JH7100_CLK_DISPBUS_SRC,		"dispbus_src",	 4, "voutbus_root"),

	JH71X0CLKC_DIV(JH7100_CLK_DISP_BUS,		"disp_bus",	 4, "dispbus_src"),

	JH71X0CLKC_GATE(JH7100_CLK_UART0_APB,		"uart0_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_UART1_APB,		"uart1_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_UART2_APB,		"uart2_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_UART3_APB,		"uart3_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SGDMA2P_AXI,		"sgdma2p_axi",	"cpu_axi"),
	JH71X0CLKC_GATE(JH7100_CLK_SGDMA2P_AHB,		"sgdma2p_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SGDMA1P_AXI,		"sgdma1p_axi",	"sgdma1p_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_GPIO_APB,		"gpio_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_I2C0_APB,		"i2c0_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_I2C1_APB,		"i2c1_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_I2C2_APB,		"i2c2_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_I2C3_APB,		"i2c3_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_TRNG_APB,		"trng_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SEC_AHB,		"sec_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_GMAC_AHB,		"gmac_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_GMAC_AHB,		"gmac_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_JPEG_APB,		"jpeg_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_PWM_APB,		"pwm_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_QSPI_AHB,		"qspi_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SPI0_APB,		"spi0_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SPI1_APB,		"spi1_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SPI2_APB,		"spi2_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SPI3_APB,		"spi3_apb",	"apb2_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SDIO0_AHB,		"sdio0_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_SDIO1_AHB,		"sdio1_ahb",	"ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_OTP_APB,		"otp_apb",	"apb1_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_DOM7AHB_BUS,		"dom7ahb_bus",  "ahb_bus"),
	JH71X0CLKC_GATE(JH7100_CLK_AUDIO_SRC,		"audio_src",	"audio_div"),
	JH71X0CLKC_GATE(JH7100_CLK_AUDIO_12288,		"audio_12288",	"osc_aud"),

	JH71X0CLKC_GATE(JH7100_CLK_DISP_AXI,		"disp_axi",	"disp_bus"),

	JH71X0CLKC_GATE(JH7100_CLK_GMAC_RMII,		"gmac_rmii",	"gmac_rmii_ref"),

	JH71X0CLKC_GATE(JH7100_CLK_WDTIMER_APB,		"wdtimer_apb",	"apb2_bus"),

	JH71X0CLKC_GATEDIV(JH7100_CLK_UART0_CORE,	"uart0_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_UART1_CORE,	"uart1_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_UART2_CORE,	"uart2_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_UART3_CORE,	"uart3_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_I2C0_CORE,	"i2c0_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_I2C1_CORE,	"i2c1_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_I2C2_CORE,	"i2c2_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_I2C3_CORE,	"i2c3_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_GMAC_PTP_REF,	"gmac_ptp_refclk",	 31, "gmac_root_div"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_VP6_CORE,		"vp6_core",		  4, "dsp_root_div"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_VP6_CORE,		"vp6_core",		  4, "dsp_root_div"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SPI0_CORE,	"spi0_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SPI1_CORE,	"spi1_core",		 63, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SPI2_CORE,	"spi2_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SPI3_CORE,	"spi3_core",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_VP6_CORE,		"vp6_core",		  4, "dsp_root_div"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SDIO0_CCLKINT,	"sdio0_cclkint",	 24, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_SDIO1_CCLKINT, 	"sdio1_cclkint",	 24, "perh1_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_AUDIO_ROOT,	"audio_root",		  8, "pll0_out"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_VOUT_SRC,		"vout_src",		  4, "vout_root"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_WDT_CORE,		"wdt_coreclk",		 63, "perh0_src"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_GMAC_GTX,		"gmac_gtxclk",		255, "gmac_root_div"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_GMAC_RMII_TX,	"gmac_rmii_txclk",	  8, "gmac_rmii_ref"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_GMAC_RMII_RX,	"gmac_rmii_rxclk",	  8, "gmac_rmii_ref"),
	JH71X0CLKC_GATEDIV(JH7100_CLK_GMAC_TOPHYREF,	"gmac_tophyref",	127, "gmac_root_div"),

	JH71X0CLKC_FRACDIV(JH7100_CLK_AUDIO_DIV,	"audio_div",		"audio_root"),

	JH71X0CLKC_INV(JH7100_CLK_SDIO0_CCLKINT_INV,	"sdio0_cclkint_inv",	"sdio0_cclkint"),
	JH71X0CLKC_INV(JH7100_CLK_SDIO1_CCLKINT_INV,	"sdio1_cclkint_inv",	"sdio1_cclkint"),
	JH71X0CLKC_INV(JH7100_CLK_GMAC_RX_INV,		"gmac_rx_inv",		"gmac_rx_pre"),
	JH71X0CLKC_INV(JH7100_CLK_GMAC_TX_INV,		"gmac_tx_inv",		"gmac_tx"),
};


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7100-clkgen" },
	DEVICE_COMPAT_EOL
};


static struct clk *
jh7100_clkc_fdt_decode(device_t dev, int phandle, const void *data,
    size_t len)
{
	struct jh71x0_clkc_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	u_int id = be32dec(data);
	if (id >= sc->sc_nclks)
		return NULL;

	if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN) {
		printf("Unknown clock %d\n", id);
		return NULL;
	}
	return &sc->sc_clk[id].jcc_clk;
}

static const struct fdtbus_clock_controller_func jh7100_clkc_fdt_funcs = {
	.decode = jh7100_clkc_fdt_decode
};

static int
jh7100_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7100_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct jh71x0_clkc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	struct clk * const osclk = fdtbus_clock_get(phandle, "osc_sys");
	if (osclk == NULL) {
		aprint_error(": couldn't get osc_sys\n");
		return;
	}
	u_int osclk_rate = clk_get_rate(osclk);

	struct clk * const oaclk = fdtbus_clock_get(phandle, "osc_aud");
	if (oaclk == NULL) {
		aprint_error(": couldn't get osc_aud\n");
		return;
	}
	u_int oaclk_rate = clk_get_rate(oaclk);

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &jh71x0_clkc_funcs;
	sc->sc_clkdom.priv = sc;

	sc->sc_clk = jh7100_clocks;
	sc->sc_nclks = __arraycount(jh7100_clocks);
	for (size_t id = 0; id < sc->sc_nclks; id++) {
		if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN)
			continue;

		sc->sc_clk[id].jcc_clk.domain = &sc->sc_clkdom;
		clk_attach(&sc->sc_clk[id].jcc_clk);
	}

	aprint_naive("\n");
	aprint_normal(": JH7100 (OSC0 %u Hz, OSC1 %u Hz)\n",
	    osclk_rate, oaclk_rate);

	for (size_t id = 0; id < sc->sc_nclks; id++) {
		if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN)
			continue;

		struct clk * const clk = &sc->sc_clk[id].jcc_clk;

		aprint_debug_dev(self, "id %zu [%s]: %u Hz\n", id,
		    clk->name ? clk->name : "<none>", clk_get_rate(clk));
	}

	fdtbus_register_clock_controller(self, phandle, &jh7100_clkc_fdt_funcs);
}

CFATTACH_DECL_NEW(jh7100_clkc, sizeof(struct jh71x0_clkc_softc),
    jh7100_clkc_match, jh7100_clkc_attach, NULL, NULL);
