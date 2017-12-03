/* $NetBSD: tegra124_car.c,v 1.14.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra124_car.c,v 1.14.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra124_carreg.h>
#include <arm/nvidia/tegra_clock.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra124_car_match(device_t, cfdata_t, void *);
static void	tegra124_car_attach(device_t, device_t, void *);

static struct clk *tegra124_car_clock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func tegra124_car_fdtclock_funcs = {
	.decode = tegra124_car_clock_decode
};

/* DT clock ID to clock name mappings */
static struct tegra124_car_clock_id {
	u_int		id;
	const char	*name;
} tegra124_car_clock_ids[] = {
	{ 3, "ispb" },
	{ 4, "rtc" },
	{ 5, "timer" },
	{ 6, "uarta" },
	{ 9, "sdmmc2" },
	{ 11, "i2s1" },
	{ 12, "i2c1" },
	{ 14, "sdmmc1" },
	{ 15, "sdmmc4" },
	{ 17, "pwm" },
	{ 18, "i2s2" },
	{ 22, "usbd" },
	{ 23, "isp" },
	{ 26, "disp2" },
	{ 27, "disp1" },
	{ 28, "host1x" },
	{ 29, "vcp" },
	{ 30, "i2s0" },
	{ 32, "mc" },
	{ 34, "apbdma" },
	{ 36, "kbc" },
	{ 40, "kfuse" },
	{ 41, "spi1" },
	{ 42, "nor" },
	{ 44, "spi2" },
	{ 46, "spi3" },
	{ 47, "i2c5" },
	{ 48, "dsia" },
	{ 50, "mipi" },
	{ 51, "hdmi" },
	{ 52, "csi" },
	{ 54, "i2c2" },
	{ 55, "uartc" },
	{ 56, "mipi_cal" },
	{ 57, "emc" },
	{ 58, "usb2" },
	{ 59, "usb3" },
	{ 61, "vde" },
	{ 62, "bsea" },
	{ 63, "bsev" },
	{ 65, "uartd" },
	{ 67, "i2c3" },
	{ 68, "spi4" },
	{ 69, "sdmmc3" },
	{ 70, "pcie" },
	{ 71, "owr" },
	{ 72, "afi" },
	{ 73, "csite" },
	{ 76, "la" },
	{ 77, "trace" },
	{ 78, "soc_therm" },
	{ 79, "dtv" },
	{ 81, "i2cslow" },
	{ 82, "dsib" },
	{ 83, "tsec" },
	{ 89, "xusb_host" },
	{ 91, "msenc" },
	{ 92, "csus" },
	{ 99, "mselect" },
	{ 100, "tsensor" },
	{ 101, "i2s3" },
	{ 102, "i2s4" },
	{ 103, "i2c4" },
	{ 104, "spi5" },
	{ 105, "spi6" },
	{ 106, "d_audio" },
	{ 107, "apbif" },
	{ 108, "dam0" },
	{ 109, "dam1" },
	{ 110, "dam2" },
	{ 111, "hda2codec_2x" },
	{ 113, "audio0_2x" },
	{ 114, "audio1_2x" },
	{ 115, "audio2_2x" },
	{ 116, "audio3_2x" },
	{ 117, "audio4_2x" },
	{ 118, "spdif_2x" },
	{ 119, "actmon" },
	{ 120, "extern1" },
	{ 121, "extern2" },
	{ 122, "extern3" },
	{ 123, "sata_oob" },
	{ 124, "sata" },
	{ 125, "hda" },
	{ 127, "se" },
	{ 128, "hda2hdmi" },
	{ 129, "sata_cold" },
	{ 144, "cilab" },
	{ 145, "cilcd" },
	{ 146, "cile" },
	{ 147, "dsialp" },
	{ 148, "dsiblp" },
	{ 149, "entropy" },
	{ 150, "dds" },
	{ 152, "dp2" },
	{ 153, "amx" },
	{ 154, "adx" },
	{ 156, "xusb_ss" },
	{ 166, "i2c6" },
	{ 171, "vim2_clk" },
	{ 176, "hdmi_audio" },
	{ 177, "clk72mhz" },
	{ 178, "vic03" },
	{ 180, "adx1" },
	{ 181, "dpaux" },
	{ 182, "sor0" },
	{ 184, "gpu" },
	{ 185, "amx1" },
	{ 192, "uartb" },
	{ 193, "vfir" },
	{ 194, "spdif_in" },
	{ 195, "spdif_out" },
	{ 196, "vi" },
	{ 197, "vi_sensor" },
	{ 198, "fuse" },
	{ 199, "fuse_burn" },
	{ 200, "clk_32k" },
	{ 201, "clk_m" },
	{ 202, "clk_m_div2" },
	{ 203, "clk_m_div4" },
	{ 204, "pll_ref" },
	{ 205, "pll_c" },
	{ 206, "pll_c_out1" },
	{ 207, "pll_c2" },
	{ 208, "pll_c3" },
	{ 209, "pll_m" },
	{ 210, "pll_m_out1" },
	{ 211, "pll_p_out0" },
	{ 212, "pll_p_out1" },
	{ 213, "pll_p_out2" },
	{ 214, "pll_p_out3" },
	{ 215, "pll_p_out4" },
	{ 216, "pll_a" },
	{ 217, "pll_a_out0" },
	{ 218, "pll_d" },
	{ 219, "pll_d_out0" },
	{ 220, "pll_d2" },
	{ 221, "pll_d2_out0" },
	{ 222, "pll_u" },
	{ 223, "pll_u_480m" },
	{ 224, "pll_u_60m" },
	{ 225, "pll_u_48m" },
	{ 226, "pll_u_12m" },
	{ 229, "pll_re_vco" },
	{ 230, "pll_re_out" },
	{ 231, "pll_e" },
	{ 232, "spdif_in_sync" },
	{ 233, "i2s0_sync" },
	{ 234, "i2s1_sync" },
	{ 235, "i2s2_sync" },
	{ 236, "i2s3_sync" },
	{ 237, "i2s4_sync" },
	{ 238, "vimclk_sync" },
	{ 239, "audio0" },
	{ 240, "audio1" },
	{ 241, "audio2" },
	{ 242, "audio3" },
	{ 243, "audio4" },
	{ 244, "spdif" },
	{ 245, "clk_out_1" },
	{ 246, "clk_out_2" },
	{ 247, "clk_out_3" },
	{ 248, "blink" },
	{ 252, "xusb_host_src" },
	{ 253, "xusb_falcon_src" },
	{ 254, "xusb_fs_src" },
	{ 255, "xusb_ss_src" },
	{ 256, "xusb_dev_src" },
	{ 257, "xusb_dev" },
	{ 258, "xusb_hs_src" },
	{ 259, "sclk" },
	{ 260, "hclk" },
	{ 261, "pclk" },
	{ 264, "dfll_ref" },
	{ 265, "dfll_soc" },
	{ 266, "vi_sensor2" },
	{ 267, "pll_p_out5" },
	{ 268, "cml0" },
	{ 269, "cml1" },
	{ 270, "pll_c4" },
	{ 271, "pll_dp" },
	{ 272, "pll_e_mux" },
	{ 273, "pll_d_dsi_out" },
	{ 300, "audio0_mux" },
	{ 301, "audio1_mux" },
	{ 302, "audio2_mux" },
	{ 303, "audio3_mux" },
	{ 304, "audio4_mux" },
	{ 305, "spdif_mux" },
	{ 306, "clk_out_1_mux" },
	{ 307, "clk_out_2_mux" },
	{ 308, "clk_out_3_mux" },
	{ 311, "sor0_lvds" },
	{ 312, "xusb_ss_div2" },
	{ 313, "pll_m_ud" },
	{ 314, "pll_c_ud" },
	{ 227, "pll_x" },
	{ 228, "pll_x_out0" },
	{ 262, "cclk_g" },
	{ 263, "cclk_lp" },
	{ 315, "clk_max" },
};

static struct clk *tegra124_car_clock_get(void *, const char *);
static void	tegra124_car_clock_put(void *, struct clk *);
static u_int	tegra124_car_clock_get_rate(void *, struct clk *);
static int	tegra124_car_clock_set_rate(void *, struct clk *, u_int);
static int	tegra124_car_clock_enable(void *, struct clk *);
static int	tegra124_car_clock_disable(void *, struct clk *);
static int	tegra124_car_clock_set_parent(void *, struct clk *,
		    struct clk *);
static struct clk *tegra124_car_clock_get_parent(void *, struct clk *);

static const struct clk_funcs tegra124_car_clock_funcs = {
	.get = tegra124_car_clock_get,
	.put = tegra124_car_clock_put,
	.get_rate = tegra124_car_clock_get_rate,
	.set_rate = tegra124_car_clock_set_rate,
	.enable = tegra124_car_clock_enable,
	.disable = tegra124_car_clock_disable,
	.set_parent = tegra124_car_clock_set_parent,
	.get_parent = tegra124_car_clock_get_parent,
};

#define CLK_FIXED(_name, _rate) {				\
	.base = { .name = (_name) }, .type = TEGRA_CLK_FIXED,	\
	.u = { .fixed = { .rate = (_rate) } }			\
}

#define CLK_PLL(_name, _parent, _base, _divm, _divn, _divp) {	\
	.base = { .name = (_name) }, .type = TEGRA_CLK_PLL,	\
	.parent = (_parent),					\
	.u = {							\
		.pll = {					\
			.base_reg = (_base),			\
			.divm_mask = (_divm),			\
			.divn_mask = (_divn),			\
			.divp_mask = (_divp),			\
		}						\
	}							\
}

#define CLK_MUX(_name, _reg, _bits, _p) {			\
	.base = { .name = (_name) }, .type = TEGRA_CLK_MUX,	\
	.u = {							\
		.mux = {					\
			.nparents = __arraycount(_p),		\
			.parents = (_p),			\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define CLK_FIXED_DIV(_name, _parent, _div) {			\
	.base = { .name = (_name) }, .type = TEGRA_CLK_FIXED_DIV, \
	.parent = (_parent),					\
	.u = {							\
		.fixed_div = {					\
			.div = (_div)				\
		}						\
	}							\
}

#define CLK_DIV(_name, _parent, _reg, _bits) {			\
	.base = { .name = (_name) }, .type = TEGRA_CLK_DIV,	\
	.parent = (_parent),					\
	.u = {							\
		.div = {					\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define CLK_GATE(_name, _parent, _set, _clr, _bits) {		\
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = TEGRA_CLK_GATE,					\
	.parent = (_parent),					\
	.u = {							\
		.gate = {					\
			.set_reg = (_set),			\
			.clr_reg = (_clr),			\
			.bits = (_bits),			\
		}						\
	}							\
}

#define CLK_GATE_L(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_L_SET_REG, CAR_CLK_ENB_L_CLR_REG,	\
		 _bits)

#define CLK_GATE_H(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_H_SET_REG, CAR_CLK_ENB_H_CLR_REG,	\
		 _bits)

#define CLK_GATE_U(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_U_SET_REG, CAR_CLK_ENB_U_CLR_REG,	\
		 _bits)

#define CLK_GATE_V(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_V_SET_REG, CAR_CLK_ENB_V_CLR_REG,	\
		 _bits)

#define CLK_GATE_W(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_W_SET_REG, CAR_CLK_ENB_W_CLR_REG,	\
		 _bits)

#define CLK_GATE_X(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_X_SET_REG, CAR_CLK_ENB_X_CLR_REG,	\
		 _bits)

#define CLK_GATE_SIMPLE(_name, _parent, _reg, _bits)		\
	CLK_GATE(_name, _parent, _reg, _reg, _bits)

static const char *mux_uart_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_m_out0", NULL, "clk_m" };
static const char *mux_sdmmc_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_m_out0", "pll_e_out0", "clk_m" };
static const char *mux_i2c_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_m_out0", NULL, "clk_m" };
static const char *mux_spi_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_m_out0", NULL, "clk_m" };
static const char *mux_sata_p[] =
	{ "pll_p_out0", NULL, "pll_c_out0", NULL, "pll_m_out0", NULL, "clk_m" };
static const char *mux_hda_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_m_out0", NULL, "clk_m" };
static const char *mux_tsensor_p[] =
	{ "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0", "clk_m",
	  NULL, "clk_s" };
static const char *mux_soc_therm_p[] =
	{ "pll_m_out0", "pll_c_out0", "pll_p_out0", "pll_a_out0", "pll_c2_out0",
	  "pll_c3_out0" };
static const char *mux_host1x_p[] =
	{ "pll_m_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_p_out0", NULL, "pll_a_out0" };
static const char *mux_disp_p[] =
	{ "pll_p_out0", "pll_m_out0", "pll_d_out0", "pll_a_out0", "pll_c_out0",
	  "pll_d2_out0", "clk_m" };
static const char *mux_hdmi_p[] =
	{ "pll_p_out0", "pll_m_out0", "pll_d_out0", "pll_a_out0", "pll_c_out0",
	  "pll_d2_out0", "clk_m" };
static const char *mux_xusb_host_p[] =
	{ "clk_m", "pll_p_out0", "pll_c2_out0", "pll_c_out0", "pll_c3_out0",
	  "pll_re_out" };
static const char *mux_xusb_ss_p[] =
	{ "clk_m", "pll_re_out", "clk_s", "pll_u_480",
	  "pll_c_out0", "pll_c2_out0", "pll_c3_out0", NULL };
static const char *mux_xusb_fs_p[] =
	{ "clk_m", NULL, "pll_u_48", NULL, "pll_p_out0", NULL, "pll_u_480" };

static struct tegra_clk tegra124_car_clocks[] = {
	CLK_FIXED("clk_m", TEGRA124_REF_FREQ),

	CLK_PLL("pll_p", "clk_m", CAR_PLLP_BASE_REG,
		CAR_PLLP_BASE_DIVM, CAR_PLLP_BASE_DIVN, CAR_PLLP_BASE_DIVP),
	CLK_PLL("pll_c", "clk_m", CAR_PLLC_BASE_REG,
		CAR_PLLC_BASE_DIVM, CAR_PLLC_BASE_DIVN, CAR_PLLC_BASE_DIVP),
	CLK_PLL("pll_u", "clk_m", CAR_PLLU_BASE_REG,
		CAR_PLLU_BASE_DIVM, CAR_PLLU_BASE_DIVN, CAR_PLLU_BASE_VCO_FREQ),
	CLK_PLL("pll_x", "clk_m", CAR_PLLX_BASE_REG,
		CAR_PLLX_BASE_DIVM, CAR_PLLX_BASE_DIVN, CAR_PLLX_BASE_DIVP),
	CLK_PLL("pll_e", "clk_m", CAR_PLLE_BASE_REG,
		CAR_PLLE_BASE_DIVM, CAR_PLLE_BASE_DIVN, CAR_PLLE_BASE_DIVP_CML),
	CLK_PLL("pll_d", "clk_m", CAR_PLLD_BASE_REG,
		CAR_PLLD_BASE_DIVM, CAR_PLLD_BASE_DIVN, CAR_PLLD_BASE_DIVP),
	CLK_PLL("pll_d2", "clk_m", CAR_PLLD2_BASE_REG,
		CAR_PLLD2_BASE_DIVM, CAR_PLLD2_BASE_DIVN, CAR_PLLD2_BASE_DIVP),
	CLK_PLL("pll_re", "clk_m", CAR_PLLREFE_BASE_REG,
		CAR_PLLREFE_BASE_DIVM, CAR_PLLREFE_BASE_DIVN, CAR_PLLREFE_BASE_DIVP),

	CLK_FIXED_DIV("pll_p_out0", "pll_p", 1),
	CLK_FIXED_DIV("pll_u_480", "pll_u", 1),
	CLK_FIXED_DIV("pll_u_60", "pll_u", 8),
	CLK_FIXED_DIV("pll_u_48", "pll_u", 10),
	CLK_FIXED_DIV("pll_u_12", "pll_u", 40),
	CLK_FIXED_DIV("pll_d_out", "pll_d", 1),
	CLK_FIXED_DIV("pll_d_out0", "pll_d", 2),
	CLK_FIXED_DIV("pll_d2_out0", "pll_d2", 1),
	CLK_FIXED_DIV("pll_re_out", "pll_re", 1),

	CLK_MUX("mux_uarta", CAR_CLKSRC_UARTA_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("mux_uartb", CAR_CLKSRC_UARTB_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("mux_uartc", CAR_CLKSRC_UARTC_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("mux_uartd", CAR_CLKSRC_UARTD_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("mux_sdmmc1", CAR_CLKSRC_SDMMC1_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc_p),
	CLK_MUX("mux_sdmmc2", CAR_CLKSRC_SDMMC2_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc_p),
	CLK_MUX("mux_sdmmc3", CAR_CLKSRC_SDMMC3_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc_p),
	CLK_MUX("mux_sdmmc4", CAR_CLKSRC_SDMMC4_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc_p),
	CLK_MUX("mux_i2c1", CAR_CLKSRC_I2C1_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_i2c2", CAR_CLKSRC_I2C2_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_i2c3", CAR_CLKSRC_I2C3_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_i2c4", CAR_CLKSRC_I2C4_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_i2c5", CAR_CLKSRC_I2C5_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_i2c6", CAR_CLKSRC_I2C6_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("mux_spi1", CAR_CLKSRC_SPI1_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_spi2", CAR_CLKSRC_SPI2_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_spi3", CAR_CLKSRC_SPI3_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_spi4", CAR_CLKSRC_SPI4_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_spi5", CAR_CLKSRC_SPI5_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_spi6", CAR_CLKSRC_SPI6_REG, CAR_CLKSRC_SPI_SRC, mux_spi_p),
	CLK_MUX("mux_sata_oob",
		CAR_CLKSRC_SATA_OOB_REG, CAR_CLKSRC_SATA_OOB_SRC, mux_sata_p),
	CLK_MUX("mux_sata",
		CAR_CLKSRC_SATA_REG, CAR_CLKSRC_SATA_SRC, mux_sata_p),
	CLK_MUX("mux_hda2codec_2x",
		CAR_CLKSRC_HDA2CODEC_2X_REG, CAR_CLKSRC_HDA2CODEC_2X_SRC,
		mux_hda_p),
	CLK_MUX("mux_hda",
		CAR_CLKSRC_HDA_REG, CAR_CLKSRC_HDA_SRC, mux_hda_p),
	CLK_MUX("mux_soc_therm",
		CAR_CLKSRC_SOC_THERM_REG, CAR_CLKSRC_SOC_THERM_SRC,
		mux_soc_therm_p),
	CLK_MUX("mux_tsensor",
		CAR_CLKSRC_TSENSOR_REG, CAR_CLKSRC_TSENSOR_SRC,
		mux_tsensor_p),
	CLK_MUX("mux_host1x",
		CAR_CLKSRC_HOST1X_REG, CAR_CLKSRC_HOST1X_SRC,
		mux_host1x_p),
	CLK_MUX("mux_disp1",
		CAR_CLKSRC_DISP1_REG, CAR_CLKSRC_DISP_SRC,
		mux_disp_p),
	CLK_MUX("mux_disp2",
		CAR_CLKSRC_DISP2_REG, CAR_CLKSRC_DISP_SRC,
		mux_disp_p),
	CLK_MUX("mux_hdmi",
		CAR_CLKSRC_HDMI_REG, CAR_CLKSRC_HDMI_SRC,
		mux_hdmi_p),
	CLK_MUX("mux_xusb_host",
		CAR_CLKSRC_XUSB_HOST_REG, CAR_CLKSRC_XUSB_HOST_SRC,
		mux_xusb_host_p),
	CLK_MUX("mux_xusb_falcon",
		CAR_CLKSRC_XUSB_FALCON_REG, CAR_CLKSRC_XUSB_FALCON_SRC,
		mux_xusb_host_p),
	CLK_MUX("mux_xusb_ss",
		CAR_CLKSRC_XUSB_SS_REG, CAR_CLKSRC_XUSB_SS_SRC,
		mux_xusb_ss_p),
	CLK_MUX("mux_xusb_fs",
		CAR_CLKSRC_XUSB_FS_REG, CAR_CLKSRC_XUSB_FS_SRC,
		mux_xusb_fs_p),

	CLK_DIV("div_uarta", "mux_uarta",
		CAR_CLKSRC_UARTA_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("div_uartb", "mux_uartb",
		CAR_CLKSRC_UARTB_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("div_uartc", "mux_uartc",
		CAR_CLKSRC_UARTC_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("div_uartd", "mux_uartd",
		CAR_CLKSRC_UARTD_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("div_sdmmc1", "mux_sdmmc1",
		CAR_CLKSRC_SDMMC1_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("div_sdmmc2", "mux_sdmmc2",
		CAR_CLKSRC_SDMMC2_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("div_sdmmc3", "mux_sdmmc3",
		CAR_CLKSRC_SDMMC3_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("div_sdmmc4", "mux_sdmmc4",
		CAR_CLKSRC_SDMMC4_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("div_i2c1", "mux_i2c1", 
		CAR_CLKSRC_I2C1_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_i2c2", "mux_i2c2", 
		CAR_CLKSRC_I2C2_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_i2c3", "mux_i2c3", 
		CAR_CLKSRC_I2C3_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_i2c4", "mux_i2c4", 
		CAR_CLKSRC_I2C4_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_i2c5", "mux_i2c5", 
		CAR_CLKSRC_I2C5_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_i2c6", "mux_i2c6", 
		CAR_CLKSRC_I2C6_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("div_spi1", "mux_spi1",
		CAR_CLKSRC_SPI1_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_spi2", "mux_spi2",
		CAR_CLKSRC_SPI2_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_spi3", "mux_spi3",
		CAR_CLKSRC_SPI3_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_spi4", "mux_spi4",
		CAR_CLKSRC_SPI4_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_spi5", "mux_spi5",
		CAR_CLKSRC_SPI5_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_spi6", "mux_spi6",
		CAR_CLKSRC_SPI6_REG, CAR_CLKSRC_SPI_DIV),
	CLK_DIV("div_sata_oob", "mux_sata_oob",
		CAR_CLKSRC_SATA_OOB_REG, CAR_CLKSRC_SATA_OOB_DIV),
	CLK_DIV("div_sata", "mux_sata",
		CAR_CLKSRC_SATA_REG, CAR_CLKSRC_SATA_DIV),
	CLK_DIV("div_hda2codec_2x", "mux_hda2codec_2x",
		CAR_CLKSRC_HDA2CODEC_2X_REG, CAR_CLKSRC_HDA2CODEC_2X_DIV),
	CLK_DIV("div_hda", "mux_hda",
		CAR_CLKSRC_HDA_REG, CAR_CLKSRC_HDA_DIV),
	CLK_DIV("div_soc_therm", "mux_soc_therm",
		CAR_CLKSRC_SOC_THERM_REG, CAR_CLKSRC_SOC_THERM_DIV),
	CLK_DIV("div_tsensor", "mux_tsensor",
		CAR_CLKSRC_TSENSOR_REG, CAR_CLKSRC_TSENSOR_DIV),
	CLK_DIV("div_host1x", "mux_host1x",
		CAR_CLKSRC_HOST1X_REG, CAR_CLKSRC_HOST1X_CLK_DIVISOR),
	CLK_DIV("div_hdmi", "mux_hdmi",
		CAR_CLKSRC_HDMI_REG, CAR_CLKSRC_HDMI_DIV),
	CLK_DIV("div_pll_p_out5", "pll_p",
		CAR_PLLP_OUTC_REG, CAR_PLLP_OUTC_OUT5_RATIO),
	CLK_DIV("xusb_host_src", "mux_xusb_host",
		CAR_CLKSRC_XUSB_HOST_REG, CAR_CLKSRC_XUSB_HOST_DIV),
	CLK_DIV("xusb_ss_src", "mux_xusb_ss",
		CAR_CLKSRC_XUSB_SS_REG, CAR_CLKSRC_XUSB_SS_DIV),
	CLK_DIV("xusb_fs_src", "mux_xusb_fs",
		CAR_CLKSRC_XUSB_FS_REG, CAR_CLKSRC_XUSB_FS_DIV),
	CLK_DIV("xusb_falcon_src", "mux_xusb_falcon",
		CAR_CLKSRC_XUSB_FALCON_REG, CAR_CLKSRC_XUSB_FALCON_DIV),

	CLK_GATE_L("uarta", "div_uarta", CAR_DEV_L_UARTA),
	CLK_GATE_L("uartb", "div_uartb", CAR_DEV_L_UARTB),
	CLK_GATE_H("uartc", "div_uartc", CAR_DEV_H_UARTC),
	CLK_GATE_U("uartd", "div_uartd", CAR_DEV_U_UARTD),
	CLK_GATE_L("sdmmc1", "div_sdmmc1", CAR_DEV_L_SDMMC1),
	CLK_GATE_L("sdmmc2", "div_sdmmc2", CAR_DEV_L_SDMMC2),
	CLK_GATE_U("sdmmc3", "div_sdmmc3", CAR_DEV_U_SDMMC3),
	CLK_GATE_L("sdmmc4", "div_sdmmc4", CAR_DEV_L_SDMMC4),
	CLK_GATE_L("i2c1", "div_i2c1", CAR_DEV_L_I2C1),
	CLK_GATE_H("i2c2", "div_i2c2", CAR_DEV_H_I2C2),
	CLK_GATE_U("i2c3", "div_i2c3", CAR_DEV_U_I2C3),
	CLK_GATE_V("i2c4", "div_i2c4", CAR_DEV_V_I2C4),
	CLK_GATE_H("i2c5", "div_i2c5", CAR_DEV_H_I2C5),
	CLK_GATE_X("i2c6", "div_i2c6", CAR_DEV_X_I2C6),
	CLK_GATE_H("spi1", "div_spi1", CAR_DEV_H_SPI1),
	CLK_GATE_H("spi2", "div_spi2", CAR_DEV_H_SPI2),
	CLK_GATE_H("spi3", "div_spi3", CAR_DEV_H_SPI3),
	CLK_GATE_U("spi4", "div_spi4", CAR_DEV_U_SPI4),
	CLK_GATE_V("spi5", "div_spi5", CAR_DEV_V_SPI5),
	CLK_GATE_V("spi6", "div_spi6", CAR_DEV_V_SPI6),
	CLK_GATE_L("usbd", "pll_u_480", CAR_DEV_L_USBD),
	CLK_GATE_H("usb2", "pll_u_480", CAR_DEV_H_USB2),
	CLK_GATE_H("usb3", "pll_u_480", CAR_DEV_H_USB3),
	CLK_GATE_V("sata_oob", "div_sata_oob", CAR_DEV_V_SATA_OOB),
	CLK_GATE_V("sata", "div_sata", CAR_DEV_V_SATA),
	CLK_GATE_SIMPLE("cml0", "pll_e",
		CAR_PLLE_AUX_REG, CAR_PLLE_AUX_CML0_OEN),
	CLK_GATE_SIMPLE("cml1", "pll_e",
		CAR_PLLE_AUX_REG, CAR_PLLE_AUX_CML1_OEN),
	CLK_GATE_V("hda2codec_2x", "div_hda2codec_2x", CAR_DEV_V_HDA2CODEC_2X),
	CLK_GATE_V("hda", "div_hda", CAR_DEV_V_HDA),
	CLK_GATE_W("hda2hdmi", "clk_m", CAR_DEV_W_HDA2HDMICODEC),
	CLK_GATE_H("fuse", "clk_m", CAR_DEV_H_FUSE),
	CLK_GATE_U("soc_therm", "div_soc_therm", CAR_DEV_U_SOC_THERM),
	CLK_GATE_V("tsensor", "div_tsensor", CAR_DEV_V_TSENSOR),
	CLK_GATE_L("host1x", "div_host1x", CAR_DEV_L_HOST1X),
	CLK_GATE_L("disp1", "mux_disp1", CAR_DEV_L_DISP1),
	CLK_GATE_L("disp2", "mux_disp2", CAR_DEV_L_DISP2),
	CLK_GATE_H("hdmi", "div_hdmi", CAR_DEV_H_HDMI),
	CLK_GATE_SIMPLE("pll_p_out5", "div_pll_p_out5",
		CAR_PLLP_OUTC_REG, CAR_PLLP_OUTC_OUT5_CLKEN),
	CLK_GATE_U("xusb_host", "xusb_host_src", CAR_DEV_U_XUSB_HOST),
	CLK_GATE_W("xusb_ss", "xusb_ss_src", CAR_DEV_W_XUSB_SS),
	CLK_GATE_X("gpu", "pll_ref", CAR_DEV_X_GPU),
	CLK_GATE_H("apbdma", "clk_m", CAR_DEV_H_APBDMA),
};

struct tegra124_init_parent {
	const char *clock;
	const char *parent;
} tegra124_init_parents[] = {
	{ "sata_oob",		"pll_p_out0" },
	{ "sata",		"pll_p_out0" },
	{ "hda",		"pll_p_out0" },
	{ "hda2codec_2x",	"pll_p_out0" },
	{ "soc_therm",		"pll_p_out0" },
	{ "tsensor",		"clk_m" },
	{ "xusb_host_src",	"pll_p_out0" },
	{ "xusb_falcon_src",	"pll_p_out0" },
	{ "xusb_ss_src",	"pll_u_480" },
	{ "xusb_fs_src",	"pll_u_48" },
	{ "host1x",		"pll_p_out0" },
};

struct tegra124_car_rst {
	u_int	set_reg;
	u_int	clr_reg;
	u_int	mask;
};

static struct tegra124_car_reset_reg {
	u_int	set_reg;
	u_int	clr_reg;
} tegra124_car_reset_regs[] = {
	{ CAR_RST_DEV_L_SET_REG, CAR_RST_DEV_L_CLR_REG },
	{ CAR_RST_DEV_H_SET_REG, CAR_RST_DEV_H_CLR_REG },
	{ CAR_RST_DEV_U_SET_REG, CAR_RST_DEV_U_CLR_REG },
	{ CAR_RST_DEV_V_SET_REG, CAR_RST_DEV_V_CLR_REG },
	{ CAR_RST_DEV_W_SET_REG, CAR_RST_DEV_W_CLR_REG },
	{ CAR_RST_DEV_X_SET_REG, CAR_RST_DEV_X_CLR_REG },
};

static void *	tegra124_car_reset_acquire(device_t, const void *, size_t);
static void	tegra124_car_reset_release(device_t, void *);
static int	tegra124_car_reset_assert(device_t, void *);
static int	tegra124_car_reset_deassert(device_t, void *);

static const struct fdtbus_reset_controller_func tegra124_car_fdtreset_funcs = {
	.acquire = tegra124_car_reset_acquire,
	.release = tegra124_car_reset_release,
	.reset_assert = tegra124_car_reset_assert,
	.reset_deassert = tegra124_car_reset_deassert,
};

struct tegra124_car_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;

	u_int			sc_clock_cells;
	u_int			sc_reset_cells;

	kmutex_t		sc_rndlock;
	krndsource_t		sc_rndsource;
};

static void	tegra124_car_init(struct tegra124_car_softc *);
static void	tegra124_car_utmip_init(struct tegra124_car_softc *);
static void	tegra124_car_xusb_init(struct tegra124_car_softc *);
static void	tegra124_car_watchdog_init(struct tegra124_car_softc *);
static void	tegra124_car_parent_init(struct tegra124_car_softc *);

static void	tegra124_car_rnd_attach(device_t);
static void	tegra124_car_rnd_callback(size_t, void *);

CFATTACH_DECL_NEW(tegra124_car, sizeof(struct tegra124_car_softc),
	tegra124_car_match, tegra124_car_attach, NULL, NULL);

static int
tegra124_car_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-car", NULL };
	struct fdt_attach_args * const faa = aux;

#if 0
	return of_match_compatible(faa->faa_phandle, compatible);
#else
	if (of_match_compatible(faa->faa_phandle, compatible) == 0)
		return 0;

	return 999;
#endif
}

static void
tegra124_car_attach(device_t parent, device_t self, void *aux)
{
	struct tegra124_car_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error, n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}
	if (of_getprop_uint32(phandle, "#clock-cells", &sc->sc_clock_cells))
		sc->sc_clock_cells = 1;
	if (of_getprop_uint32(phandle, "#reset-cells", &sc->sc_reset_cells))
		sc->sc_reset_cells = 1;

	aprint_naive("\n");
	aprint_normal(": CAR\n");

	sc->sc_clkdom.funcs = &tegra124_car_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (n = 0; n < __arraycount(tegra124_car_clocks); n++)
		tegra124_car_clocks[n].base.domain = &sc->sc_clkdom;

	fdtbus_register_clock_controller(self, phandle,
	    &tegra124_car_fdtclock_funcs);
	fdtbus_register_reset_controller(self, phandle,
	    &tegra124_car_fdtreset_funcs);

	tegra124_car_init(sc);

	config_interrupts(self, tegra124_car_rnd_attach);
}

static void
tegra124_car_init(struct tegra124_car_softc *sc)
{
	tegra124_car_parent_init(sc);
	tegra124_car_utmip_init(sc);
	tegra124_car_xusb_init(sc);
	tegra124_car_watchdog_init(sc);
}

static void
tegra124_car_parent_init(struct tegra124_car_softc *sc)
{
	struct clk *clk, *clk_parent;
	int error;
	u_int n;

	for (n = 0; n < __arraycount(tegra124_init_parents); n++) {
		clk = clk_get(&sc->sc_clkdom, tegra124_init_parents[n].clock);
		KASSERT(clk != NULL);
		clk_parent = clk_get(&sc->sc_clkdom,
		    tegra124_init_parents[n].parent);
		KASSERT(clk_parent != NULL);

		error = clk_set_parent(clk, clk_parent);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't set '%s' parent to '%s': %d\n",
			    clk->name, clk_parent->name, error);
		}
		clk_put(clk_parent);
		clk_put(clk);
	}
}

static void
tegra124_car_utmip_init(struct tegra124_car_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	const u_int enable_dly_count = 0x02;
	const u_int stable_count = 0x2f;
	const u_int active_dly_count = 0x04;
	const u_int xtal_freq_count = 0x76;

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    __SHIFTIN(stable_count, CAR_UTMIP_PLL_CFG2_STABLE_COUNT) |
	    __SHIFTIN(active_dly_count, CAR_UTMIP_PLL_CFG2_ACTIVE_DLY_COUNT),
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_A_POWERDOWN |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_B_POWERDOWN |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_C_POWERDOWN |
	    CAR_UTMIP_PLL_CFG2_STABLE_COUNT |
	    CAR_UTMIP_PLL_CFG2_ACTIVE_DLY_COUNT);

        tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    __SHIFTIN(enable_dly_count, CAR_UTMIP_PLL_CFG1_ENABLE_DLY_COUNT) |
	    __SHIFTIN(xtal_freq_count, CAR_UTMIP_PLL_CFG1_XTAL_FREQ_COUNT),
	    CAR_UTMIP_PLL_CFG1_ENABLE_DLY_COUNT |
	    CAR_UTMIP_PLL_CFG1_XTAL_FREQ_COUNT);

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    0,
	    CAR_UTMIP_PLL_CFG1_PLLU_POWERDOWN |
	    CAR_UTMIP_PLL_CFG1_PLL_ENABLE_POWERDOWN);

}

static void
tegra124_car_xusb_init(struct tegra124_car_softc *sc)
{
	const bus_space_tag_t bst = sc->sc_bst;
	const bus_space_handle_t bsh = sc->sc_bsh;
	uint32_t val;

	/* XXX do this all better */

	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG, CAR_DEV_W_XUSB);

	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_MISC_REG,
	    0, CAR_PLLREFE_MISC_IDDQ);
	val = __SHIFTIN(25, CAR_PLLREFE_BASE_DIVN) |
	    __SHIFTIN(1, CAR_PLLREFE_BASE_DIVM);
	bus_space_write_4(bst, bsh, CAR_PLLREFE_BASE_REG, val);

	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_MISC_REG,
	    0, CAR_PLLREFE_MISC_LOCK_OVERRIDE);
	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_BASE_REG,
	    CAR_PLLREFE_BASE_ENABLE, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_MISC_REG,
	    CAR_PLLREFE_MISC_LOCK_ENABLE, 0);

	do {
		delay(2);
		val = bus_space_read_4(bst, bsh, CAR_PLLREFE_MISC_REG);
	} while ((val & CAR_PLLREFE_MISC_LOCK) == 0);

	tegra_reg_set_clear(bst, bsh, CAR_PLLE_MISC_REG,
	    CAR_PLLE_MISC_IDDQ_SWCTL, CAR_PLLE_MISC_IDDQ_OVERRIDE);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_BASE_REG,
	    CAR_PLLE_BASE_ENABLE, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_MISC_REG,
	    CAR_PLLE_MISC_LOCK_ENABLE, 0);

	do {
		delay(2);
		val = bus_space_read_4(bst, bsh, CAR_PLLE_MISC_REG);
	} while ((val & CAR_PLLE_MISC_LOCK) == 0);

	tegra_reg_set_clear(bst, bsh, CAR_CLKSRC_XUSB_SS_REG,
	    CAR_CLKSRC_XUSB_SS_HS_CLK_BYPASS, 0);
}

static void
tegra124_car_watchdog_init(struct tegra124_car_softc *sc)
{
	const bus_space_tag_t bst = sc->sc_bst;
	const bus_space_handle_t bsh = sc->sc_bsh;

	/* Enable watchdog timer reset for system */
	tegra_reg_set_clear(bst, bsh, CAR_RST_SOURCE_REG,
	    CAR_RST_SOURCE_WDT_EN|CAR_RST_SOURCE_WDT_SYS_RST_EN, 0);
}

static void
tegra124_car_rnd_attach(device_t self)
{
	struct tegra124_car_softc * const sc = device_private(self);

	mutex_init(&sc->sc_rndlock, MUTEX_DEFAULT, IPL_VM);
	rndsource_setcb(&sc->sc_rndsource, tegra124_car_rnd_callback, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(sc->sc_dev),
	    RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
	tegra124_car_rnd_callback(RND_POOLBITS / NBBY, sc);
}

static void
tegra124_car_rnd_callback(size_t bytes_wanted, void *priv)
{
	struct tegra124_car_softc * const sc = priv;
	uint16_t buf[512];
	uint32_t cnt;

	mutex_enter(&sc->sc_rndlock);
	while (bytes_wanted) {
		const u_int nbytes = MIN(bytes_wanted, 1024);
		for (cnt = 0; cnt < bytes_wanted / 2; cnt++) {
			buf[cnt] = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			    CAR_PLL_LFSR_REG) & 0xffff;
		}
		rnd_add_data_sync(&sc->sc_rndsource, buf, nbytes,
		    nbytes * NBBY);
		bytes_wanted -= MIN(bytes_wanted, nbytes);
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_exit(&sc->sc_rndlock);
}

static struct tegra_clk *
tegra124_car_clock_find(const char *name)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra124_car_clocks); n++) {
		if (strcmp(tegra124_car_clocks[n].base.name, name) == 0) {
			return &tegra124_car_clocks[n];
		}
	}

	return NULL;
}

static struct tegra_clk *
tegra124_car_clock_find_by_id(u_int clock_id)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra124_car_clock_ids); n++) {
		if (tegra124_car_clock_ids[n].id == clock_id) {
			const char *name = tegra124_car_clock_ids[n].name;
			return tegra124_car_clock_find(name);
		}
	}

	return NULL;
}

static struct clk *
tegra124_car_clock_decode(device_t dev, const void *data, size_t len)
{
	struct tegra124_car_softc * const sc = device_private(dev);
	struct tegra_clk *tclk;

	if (len != sc->sc_clock_cells * 4) {
		return NULL;
	}

	const u_int clock_id = be32dec(data);

	tclk = tegra124_car_clock_find_by_id(clock_id);
	if (tclk)
		return TEGRA_CLK_BASE(tclk);

	return NULL;
}

static struct clk *
tegra124_car_clock_get(void *priv, const char *name)
{
	struct tegra_clk *tclk;

	tclk = tegra124_car_clock_find(name);
	if (tclk == NULL)
		return NULL;

	atomic_inc_uint(&tclk->refcnt);

	return TEGRA_CLK_BASE(tclk);
}

static void
tegra124_car_clock_put(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);

	KASSERT(tclk->refcnt > 0);

	atomic_dec_uint(&tclk->refcnt);
}

static u_int
tegra124_car_clock_get_rate_pll(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_pll_clk *tpll = &tclk->u.pll;
	struct tegra_clk *tclk_parent;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	u_int divm, divn, divp;
	uint64_t rate;

	KASSERT(tclk->type == TEGRA_CLK_PLL);

	tclk_parent = tegra124_car_clock_find(tclk->parent);
	KASSERT(tclk_parent != NULL);

	const u_int rate_parent = tegra124_car_clock_get_rate(sc,
	    TEGRA_CLK_BASE(tclk_parent));

	const uint32_t base = bus_space_read_4(bst, bsh, tpll->base_reg);
	divm = __SHIFTOUT(base, tpll->divm_mask);
	divn = __SHIFTOUT(base, tpll->divn_mask);
	if (tpll->base_reg == CAR_PLLU_BASE_REG) {
		divp = __SHIFTOUT(base, tpll->divp_mask) ? 0 : 1;
	} else {
		divp = __SHIFTOUT(base, tpll->divp_mask);
	}

	rate = (uint64_t)rate_parent * divn;
	return rate / (divm << divp);
}

static int
tegra124_car_clock_set_rate_pll(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk, u_int rate)
{
	struct tegra_pll_clk *tpll = &tclk->u.pll;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	uint32_t bp, base;

	clk_parent = tegra124_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return EIO;
	const u_int rate_parent = tegra124_car_clock_get_rate(sc, clk_parent);
	if (rate_parent == 0)
		return EIO;

	if (tpll->base_reg == CAR_PLLX_BASE_REG) {
		const u_int divm = 1;
		const u_int divn = rate / rate_parent;
		const u_int divp = 0;

		bp = bus_space_read_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG);
		bp &= ~CAR_CCLKG_BURST_POLICY_CPU_STATE;
		bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CPU_STATE_IDLE,
				CAR_CCLKG_BURST_POLICY_CPU_STATE);
		bp &= ~CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE;
		bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CWAKEUP_SOURCE_CLKM,
				CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE);
		bus_space_write_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG, bp);

		base = bus_space_read_4(bst, bsh, CAR_PLLX_BASE_REG);
		base &= ~CAR_PLLX_BASE_DIVM;
		base &= ~CAR_PLLX_BASE_DIVN;
		base &= ~CAR_PLLX_BASE_DIVP;
		base |= __SHIFTIN(divm, CAR_PLLX_BASE_DIVM);
		base |= __SHIFTIN(divn, CAR_PLLX_BASE_DIVN);
		base |= __SHIFTIN(divp, CAR_PLLX_BASE_DIVP);
		bus_space_write_4(bst, bsh, CAR_PLLX_BASE_REG, base);

		tegra_reg_set_clear(bst, bsh, CAR_PLLX_MISC_REG,
		    CAR_PLLX_MISC_LOCK_ENABLE, 0);
		do {
			delay(2);
			base = bus_space_read_4(bst, bsh, tpll->base_reg);
		} while ((base & CAR_PLLX_BASE_LOCK) == 0);
		delay(100);

		bp &= ~CAR_CCLKG_BURST_POLICY_CPU_STATE;
		bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CPU_STATE_RUN,
				CAR_CCLKG_BURST_POLICY_CPU_STATE);
		bp &= ~CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE;
		bp |= __SHIFTIN(CAR_CCLKG_BURST_POLICY_CWAKEUP_SOURCE_PLLX_OUT0_LJ,
				CAR_CCLKG_BURST_POLICY_CWAKEUP_IDLE_SOURCE);
		bus_space_write_4(bst, bsh, CAR_CCLKG_BURST_POLICY_REG, bp);

		return 0;
	} else if (tpll->base_reg == CAR_PLLD2_BASE_REG) {
		const u_int divm = 1;
		const u_int pldiv = 1;
		const u_int divn = (rate << pldiv) / rate_parent;

		/* Set frequency */
		tegra_reg_set_clear(bst, bsh, tpll->base_reg,
		    __SHIFTIN(divm, CAR_PLLD2_BASE_DIVM) |
		    __SHIFTIN(divn, CAR_PLLD2_BASE_DIVN) |
		    __SHIFTIN(pldiv, CAR_PLLD2_BASE_DIVP),
		    CAR_PLLD2_BASE_REF_SRC_SEL |
		    CAR_PLLD2_BASE_DIVM |
		    CAR_PLLD2_BASE_DIVN |
		    CAR_PLLD2_BASE_DIVP);

		return 0;
	} else {
		/* TODO */
		return EOPNOTSUPP;
	}
}

static int
tegra124_car_clock_set_parent_mux(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk, struct tegra_clk *tclk_parent)
{
	struct tegra_mux_clk *tmux = &tclk->u.mux;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	uint32_t v;
	u_int src;

	KASSERT(tclk->type == TEGRA_CLK_MUX);

	for (src = 0; src < tmux->nparents; src++) {
		if (tmux->parents[src] == NULL) {
			continue;
		}
		if (strcmp(tmux->parents[src], tclk_parent->base.name) == 0) {
			break;
		}
	}
	if (src == tmux->nparents) {
		return EINVAL;
	}

	if (tmux->reg == CAR_CLKSRC_HDMI_REG &&
	    src == CAR_CLKSRC_HDMI_SRC_PLLD2_OUT0) {
		/* Change IDDQ from 1 to 0 */
		tegra_reg_set_clear(bst, bsh, CAR_PLLD2_BASE_REG,
		    0, CAR_PLLD2_BASE_IDDQ);
		delay(2);

		/* Enable lock */
		tegra_reg_set_clear(bst, bsh, CAR_PLLD2_MISC_REG,
		    CAR_PLLD2_MISC_LOCK_ENABLE, 0);

		/* Enable PLLD2 */
		tegra_reg_set_clear(bst, bsh, CAR_PLLD2_BASE_REG,
		    CAR_PLLD2_BASE_ENABLE, 0);

		/* Wait for lock */
		do {
			delay(2);
			v = bus_space_read_4(bst, bsh, CAR_PLLD2_BASE_REG);
		} while ((v & CAR_PLLD2_BASE_LOCK) == 0);

		delay(200);
	}

	v = bus_space_read_4(bst, bsh, tmux->reg);
	v &= ~tmux->bits;
	v |= __SHIFTIN(src, tmux->bits);
	bus_space_write_4(bst, bsh, tmux->reg, v);

	return 0;
}

static struct tegra_clk *
tegra124_car_clock_get_parent_mux(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_mux_clk *tmux = &tclk->u.mux;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	KASSERT(tclk->type == TEGRA_CLK_MUX);

	const uint32_t v = bus_space_read_4(bst, bsh, tmux->reg);
	const u_int src = __SHIFTOUT(v, tmux->bits);

	KASSERT(src < tmux->nparents);

	if (tmux->parents[src] == NULL) {
		return NULL;
	}

	return tegra124_car_clock_find(tmux->parents[src]);
}

static u_int
tegra124_car_clock_get_rate_fixed_div(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_fixed_div_clk *tfixed_div = &tclk->u.fixed_div;
	struct clk *clk_parent;

	clk_parent = tegra124_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return 0;
	const u_int parent_rate = tegra124_car_clock_get_rate(sc, clk_parent);

	return parent_rate / tfixed_div->div;
}

static u_int
tegra124_car_clock_calc_rate_frac_div(u_int rate, u_int raw_div)
{
	raw_div += 2;
	rate *= 2;
	rate += raw_div - 1;
	rate /= raw_div;
	return rate;
}

static u_int
tegra124_car_clock_get_rate_div(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_div_clk *tdiv = &tclk->u.div;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	u_int rate;

	KASSERT(tclk->type == TEGRA_CLK_DIV);

	clk_parent = tegra124_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	const u_int parent_rate = tegra124_car_clock_get_rate(sc, clk_parent);

	const uint32_t v = bus_space_read_4(bst, bsh, tdiv->reg);
	const u_int raw_div = __SHIFTOUT(v, tdiv->bits);

	switch (tdiv->reg) {
	case CAR_CLKSRC_I2C1_REG:
	case CAR_CLKSRC_I2C2_REG:
	case CAR_CLKSRC_I2C3_REG:
	case CAR_CLKSRC_I2C4_REG:
	case CAR_CLKSRC_I2C5_REG:
	case CAR_CLKSRC_I2C6_REG:
		rate = parent_rate * 1 / (raw_div + 1);
		break;
	case CAR_CLKSRC_UARTA_REG:
	case CAR_CLKSRC_UARTB_REG:
	case CAR_CLKSRC_UARTC_REG:
	case CAR_CLKSRC_UARTD_REG:
		if (v & CAR_CLKSRC_UART_DIV_ENB) {
			rate = tegra124_car_clock_calc_rate_frac_div(
			    parent_rate, raw_div);
		} else {
			rate = parent_rate;
		}
		break;
	default:
		rate = tegra124_car_clock_calc_rate_frac_div(parent_rate,
		    raw_div);
		break;
	}

	return rate;
}

static int
tegra124_car_clock_set_rate_div(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk, u_int rate)
{
	struct tegra_div_clk *tdiv = &tclk->u.div;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	u_int raw_div;
	uint32_t v;

	KASSERT(tclk->type == TEGRA_CLK_DIV);

	clk_parent = tegra124_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return EINVAL;
	const u_int parent_rate = tegra124_car_clock_get_rate(sc, clk_parent);

	v = bus_space_read_4(bst, bsh, tdiv->reg);

	raw_div = __SHIFTOUT(tdiv->bits, tdiv->bits);

	switch (tdiv->reg) {
	case CAR_CLKSRC_UARTA_REG:
	case CAR_CLKSRC_UARTB_REG:
	case CAR_CLKSRC_UARTC_REG:
	case CAR_CLKSRC_UARTD_REG:
		if (rate == parent_rate) {
			v &= ~CAR_CLKSRC_UART_DIV_ENB;
		} else {
			v |= CAR_CLKSRC_UART_DIV_ENB;
			raw_div = (parent_rate * 2) / rate - 2;
		}
		break;
	case CAR_CLKSRC_SATA_REG:
		if (rate) {
			tegra_reg_set_clear(bst, bsh, CAR_SATA_PLL_CFG0_REG,
			    0, CAR_SATA_PLL_CFG0_PADPLL_RESET_SWCTL);
			v |= CAR_CLKSRC_SATA_AUX_CLK_ENB;
			raw_div = (parent_rate * 2) / rate - 2;
		} else {
			v &= ~CAR_CLKSRC_SATA_AUX_CLK_ENB;
		}
		break;
	case CAR_CLKSRC_I2C1_REG:
	case CAR_CLKSRC_I2C2_REG:
	case CAR_CLKSRC_I2C3_REG:
	case CAR_CLKSRC_I2C4_REG:
	case CAR_CLKSRC_I2C5_REG:
	case CAR_CLKSRC_I2C6_REG:
		if (rate)
			raw_div = parent_rate / rate - 1;
		break;
	case CAR_CLKSRC_SDMMC1_REG:
	case CAR_CLKSRC_SDMMC2_REG:
	case CAR_CLKSRC_SDMMC3_REG:
	case CAR_CLKSRC_SDMMC4_REG:
		if (rate) {
			for (raw_div = 0x00; raw_div <= 0xff; raw_div++) {
				u_int calc_rate =
				    tegra124_car_clock_calc_rate_frac_div(
					parent_rate, raw_div);
				if (calc_rate <= rate)
					break;
			}
			if (raw_div == 0x100)
				return EINVAL;
		}
		break;
	default:
		if (rate)
			raw_div = (parent_rate * 2) / rate - 2;
		break;
	}

	v &= ~tdiv->bits;
	v |= __SHIFTIN(raw_div, tdiv->bits);

	bus_space_write_4(bst, bsh, tdiv->reg, v);

	return 0;
}

static int
tegra124_car_clock_enable_gate(struct tegra124_car_softc *sc,
    struct tegra_clk *tclk, bool enable)
{
	struct tegra_gate_clk *tgate = &tclk->u.gate;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	bus_size_t reg;

	KASSERT(tclk->type == TEGRA_CLK_GATE);

	if (tgate->set_reg == tgate->clr_reg) {
		uint32_t v = bus_space_read_4(bst, bsh, tgate->set_reg);
		if (enable) {
			v |= tgate->bits;
		} else {
			v &= ~tgate->bits;
		}
		bus_space_write_4(bst, bsh, tgate->set_reg, v);
	} else {
		if (enable) {
			reg = tgate->set_reg;
		} else {
			reg = tgate->clr_reg;
		}

		if (reg == CAR_CLK_ENB_V_SET_REG &&
		    tgate->bits == CAR_DEV_V_SATA) {
			/* De-assert reset to SATA PADPLL */
			tegra_reg_set_clear(bst, bsh, CAR_SATA_PLL_CFG0_REG,
			    0, CAR_SATA_PLL_CFG0_PADPLL_RESET_OVERRIDE_VALUE);
			delay(15);
		}
		bus_space_write_4(bst, bsh, reg, tgate->bits);
	}

	return 0;
}

static u_int
tegra124_car_clock_get_rate(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	switch (tclk->type) {
	case TEGRA_CLK_FIXED:
		return tclk->u.fixed.rate;
	case TEGRA_CLK_PLL:
		return tegra124_car_clock_get_rate_pll(priv, tclk);
	case TEGRA_CLK_MUX:
	case TEGRA_CLK_GATE:
		clk_parent = tegra124_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return EINVAL;
		return tegra124_car_clock_get_rate(priv, clk_parent);
	case TEGRA_CLK_FIXED_DIV:
		return tegra124_car_clock_get_rate_fixed_div(priv, tclk);
	case TEGRA_CLK_DIV:
		return tegra124_car_clock_get_rate_div(priv, tclk);
	default:
		panic("tegra124: unknown tclk type %d", tclk->type);
	}
}

static int
tegra124_car_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	KASSERT((clk->flags & CLK_SET_RATE_PARENT) == 0);

	switch (tclk->type) {
	case TEGRA_CLK_FIXED:
	case TEGRA_CLK_MUX:
		return EIO;
	case TEGRA_CLK_FIXED_DIV:
		clk_parent = tegra124_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return EIO;
		return tegra124_car_clock_set_rate(priv, clk_parent,
		    rate * tclk->u.fixed_div.div);
	case TEGRA_CLK_GATE:
		return EINVAL;
	case TEGRA_CLK_PLL:
		return tegra124_car_clock_set_rate_pll(priv, tclk, rate);
	case TEGRA_CLK_DIV:
		return tegra124_car_clock_set_rate_div(priv, tclk, rate);
	default:
		panic("tegra124: unknown tclk type %d", tclk->type);
	}
}

static int
tegra124_car_clock_enable(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	if (tclk->type != TEGRA_CLK_GATE) {
		clk_parent = tegra124_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return 0;
		return tegra124_car_clock_enable(priv, clk_parent);
	}

	return tegra124_car_clock_enable_gate(priv, tclk, true);
}

static int
tegra124_car_clock_disable(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);

	if (tclk->type != TEGRA_CLK_GATE)
		return EINVAL;

	return tegra124_car_clock_enable_gate(priv, tclk, false);
}

static int
tegra124_car_clock_set_parent(void *priv, struct clk *clk,
    struct clk *clk_parent)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct tegra_clk *tclk_parent = TEGRA_CLK_PRIV(clk_parent);
	struct clk *nclk_parent;

	if (tclk->type != TEGRA_CLK_MUX) {
		nclk_parent = tegra124_car_clock_get_parent(priv, clk);
		if (nclk_parent == clk_parent || nclk_parent == NULL)
			return EINVAL;
		return tegra124_car_clock_set_parent(priv, nclk_parent,
		    clk_parent);
	}

	return tegra124_car_clock_set_parent_mux(priv, tclk, tclk_parent);
}

static struct clk *
tegra124_car_clock_get_parent(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct tegra_clk *tclk_parent = NULL;

	switch (tclk->type) {
	case TEGRA_CLK_FIXED:
	case TEGRA_CLK_PLL:
	case TEGRA_CLK_FIXED_DIV:
	case TEGRA_CLK_DIV:
	case TEGRA_CLK_GATE:
		if (tclk->parent) {
			tclk_parent = tegra124_car_clock_find(tclk->parent);
		}
		break;
	case TEGRA_CLK_MUX:
		tclk_parent = tegra124_car_clock_get_parent_mux(priv, tclk);
		break;
	}

	if (tclk_parent == NULL)
		return NULL;

	return TEGRA_CLK_BASE(tclk_parent);
}

static void *
tegra124_car_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct tegra124_car_softc * const sc = device_private(dev);
	struct tegra124_car_rst *rst;

	if (len != sc->sc_reset_cells * 4)
		return NULL;

	const u_int reset_id = be32dec(data);

	if (reset_id >= __arraycount(tegra124_car_reset_regs) * 32)
		return NULL;

	const u_int reg = reset_id / 32;

	rst = kmem_alloc(sizeof(*rst), KM_SLEEP);
	rst->set_reg = tegra124_car_reset_regs[reg].set_reg;
	rst->clr_reg = tegra124_car_reset_regs[reg].clr_reg;
	rst->mask = __BIT(reset_id % 32);

	return rst;
}

static void
tegra124_car_reset_release(device_t dev, void *priv)
{
	struct tegra124_car_rst *rst = priv;

	kmem_free(rst, sizeof(*rst));
}

static int
tegra124_car_reset_assert(device_t dev, void *priv)
{
	struct tegra124_car_softc * const sc = device_private(dev);
	struct tegra124_car_rst *rst = priv;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, rst->set_reg, rst->mask);

	return 0;
}

static int
tegra124_car_reset_deassert(device_t dev, void *priv)
{
	struct tegra124_car_softc * const sc = device_private(dev);
	struct tegra124_car_rst *rst = priv;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, rst->clr_reg, rst->mask);

	return 0;
}
