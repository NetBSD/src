/* $NetBSD: tegra210_car.c,v 1.17 2017/09/28 09:44:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra210_car.c,v 1.17 2017/09/28 09:44:29 jmcneill Exp $");

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
#include <arm/nvidia/tegra210_carreg.h>
#include <arm/nvidia/tegra_clock.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra210_car_match(device_t, cfdata_t, void *);
static void	tegra210_car_attach(device_t, device_t, void *);

static struct clk *tegra210_car_clock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func tegra210_car_fdtclock_funcs = {
	.decode = tegra210_car_clock_decode
};

/* DT clock ID to clock name mappings */
static struct tegra210_car_clock_id {
	const char	*name;
	u_int		id;
} tegra210_car_clock_ids[] = {
	{ "ISPB", 3 },
	{ "RTC", 4 },
	{ "TIMER", 5 },
	{ "UARTA", 6 },
	{ "GPIO", 8 },
	{ "SDMMC2", 9 },
	{ "I2S1", 11 },
	{ "I2C1", 12 },
	{ "SDMMC1", 14 },
	{ "SDMMC4", 15 },
	{ "PWM", 17 },
	{ "I2S2", 18 },
	{ "USBD", 22 },
	{ "ISP", 23 },
	{ "DISP2", 26 },
	{ "DISP1", 27 },
	{ "HOST1X", 28 },
	{ "I2S0", 30 },
	{ "MC", 32 },
	{ "AHBDMA", 33 },
	{ "APBDMA", 34 },
	{ "PMC", 38 },
	{ "KFUSE", 40 },
	{ "SBC1", 41 },
	{ "SBC2", 44 },
	{ "SBC3", 46 },
	{ "I2C5", 47 },
	{ "DSIA", 48 },
	{ "CSI", 52 },
	{ "I2C2", 54 },
	{ "UARTC", 55 },
	{ "MIPI_CAL", 56 },
	{ "EMC", 57 },
	{ "USB2", 58 },
	{ "BSEV", 63 },
	{ "UARTD", 65 },
	{ "I2C3", 67 },
	{ "SBC4", 68 },
	{ "SDMMC3", 69 },
	{ "PCIE", 70 },
	{ "OWR", 71 },
	{ "AFI", 72 },
	{ "CSITE", 73 },
	{ "SOC_THERM", 78 },
	{ "DTV", 79 },
	{ "I2CSLOW", 81 },
	{ "DSIB", 82 },
	{ "TSEC", 83 },
	{ "XUSB_HOST", 89 },
	{ "CSUS", 92 },
	{ "MSELECT", 99 },
	{ "TSENSOR", 100 },
	{ "I2S3", 101 },
	{ "I2S4", 102 },
	{ "I2C4", 103 },
	{ "D_AUDIO", 106 },
	{ "APB2APE", 107 },
	{ "HDA2CODEC_2X", 111 },
	{ "SPDIF_2X", 118 },
	{ "ACTMON", 119 },
	{ "EXTERN1", 120 },
	{ "EXTERN2", 121 },
	{ "EXTERN3", 122 },
	{ "SATA_OOB", 123 },
	{ "SATA", 124 },
	{ "HDA", 125 },
	{ "HDA2HDMI", 128 },
	{ "XUSB_GATE", 143 },
	{ "CILAB", 144 },
	{ "CILCD", 145 },
	{ "CILE", 146 },
	{ "DSIALP", 147 },
	{ "DSIBLP", 148 },
	{ "ENTROPY", 149 },
	{ "XUSB_SS", 156 },
	{ "DMIC1", 161 },
	{ "DMIC2", 162 },
	{ "I2C6", 166 },
	{ "VIM2_CLK", 171 },
	{ "MIPIBIF", 173 },
	{ "CLK72MHZ", 177 },
	{ "VIC03", 178 },
	{ "DPAUX", 181 },
	{ "SOR0", 182 },
	{ "SOR1", 183 },
	{ "GPU", 184 },
	{ "DBGAPB", 185 },
	{ "PLL_P_OUT_ADSP", 187 },
	{ "PLL_G_REF", 189 },
	{ "SDMMC_LEGACY", 193 },
	{ "NVDEC", 194 },
	{ "NVJPG", 195 },
	{ "DMIC3", 197 },
	{ "APE", 198 },
	{ "MAUD", 202 },
	{ "TSECB", 206 },
	{ "DPAUX1", 207 },
	{ "VI_I2C", 208 },
	{ "HSIC_TRK", 209 },
	{ "USB2_TRK", 210 },
	{ "QSPI", 211 },
	{ "UARTAPE", 212 },
	{ "NVENC", 219 },
	{ "SOR_SAFE", 222 },
	{ "PLL_P_OUT_CPU", 223 },
	{ "UARTB", 224 },
	{ "VFIR", 225 },
	{ "SPDIF_IN", 226 },
	{ "SPDIF_OUT", 227 },
	{ "VI", 228 },
	{ "VI_SENSOR", 229 },
	{ "FUSE", 230 },
	{ "FUSE_BURN", 231 },
	{ "CLK_32K", 232 },
	{ "CLK_M", 233 },
	{ "CLK_M_DIV2", 234 },
	{ "CLK_M_DIV4", 235 },
	{ "PLL_REF", 236 },
	{ "PLL_C", 237 },
	{ "PLL_C_OUT1", 238 },
	{ "PLL_C2", 239 },
	{ "PLL_C3", 240 },
	{ "PLL_M", 241 },
	{ "PLL_M_OUT1", 242 },
	{ "PLL_P", 243 },
	{ "PLL_P_OUT1", 244 },
	{ "PLL_P_OUT2", 245 },
	{ "PLL_P_OUT3", 246 },
	{ "PLL_P_OUT4", 247 },
	{ "PLL_A", 248 },
	{ "PLL_A_OUT0", 249 },
	{ "PLL_D", 250 },
	{ "PLL_D_OUT0", 251 },
	{ "PLL_D2", 252 },
	{ "PLL_D2_OUT0", 253 },
	{ "PLL_U", 254 },
	{ "PLL_U_480M", 255 },
	{ "PLL_U_60M", 256 },
	{ "PLL_U_48M", 257 },
	{ "PLL_X", 259 },
	{ "PLL_X_OUT0", 260 },
	{ "PLL_RE_VCO", 261 },
	{ "PLL_RE_OUT", 262 },
	{ "PLL_E", 263 },
	{ "SPDIF_IN_SYNC", 264 },
	{ "I2S0_SYNC", 265 },
	{ "I2S1_SYNC", 266 },
	{ "I2S2_SYNC", 267 },
	{ "I2S3_SYNC", 268 },
	{ "I2S4_SYNC", 269 },
	{ "VIMCLK_SYNC", 270 },
	{ "AUDIO0", 271 },
	{ "AUDIO1", 272 },
	{ "AUDIO2", 273 },
	{ "AUDIO3", 274 },
	{ "AUDIO4", 275 },
	{ "SPDIF", 276 },
	{ "CLK_OUT_1", 277 },
	{ "CLK_OUT_2", 278 },
	{ "CLK_OUT_3", 279 },
	{ "BLINK", 280 },
	{ "SOR1_SRC", 282 },
	{ "XUSB_HOST_SRC", 284 },
	{ "XUSB_FALCON_SRC", 285 },
	{ "XUSB_FS_SRC", 286 },
	{ "XUSB_SS_SRC", 287 },
	{ "XUSB_DEV_SRC", 288 },
	{ "XUSB_DEV", 289 },
	{ "XUSB_HS_SRC", 290 },
	{ "SCLK", 291 },
	{ "HCLK", 292 },
	{ "PCLK", 293 },
	{ "CCLK_G", 294 },
	{ "CCLK_LP", 295 },
	{ "DFLL_REF", 296 },
	{ "DFLL_SOC", 297 },
	{ "VI_SENSOR2", 298 },
	{ "PLL_P_OUT5", 299 },
	{ "CML0", 300 },
	{ "CML1", 301 },
	{ "PLL_C4", 302 },
	{ "PLL_DP", 303 },
	{ "PLL_E_MUX", 304 },
	{ "PLL_MB", 305 },
	{ "PLL_A1", 306 },
	{ "PLL_D_DSI_OUT", 307 },
	{ "PLL_C4_OUT0", 308 },
	{ "PLL_C4_OUT1", 309 },
	{ "PLL_C4_OUT2", 310 },
	{ "PLL_C4_OUT3", 311 },
	{ "PLL_U_OUT", 312 },
	{ "PLL_U_OUT1", 313 },
	{ "PLL_U_OUT2", 314 },
	{ "USB2_HSIC_TRK", 315 },
	{ "PLL_P_OUT_HSIO", 316 },
	{ "PLL_P_OUT_XUSB", 317 },
	{ "XUSB_SSP_SRC", 318 },
	{ "PLL_RE_OUT1", 319 },
	{ "AUDIO0_MUX", 350 },
	{ "AUDIO1_MUX", 351 },
	{ "AUDIO2_MUX", 352 },
	{ "AUDIO3_MUX", 353 },
	{ "AUDIO4_MUX", 354 },
	{ "SPDIF_MUX", 355 },
	{ "CLK_OUT_1_MUX", 356 },
	{ "CLK_OUT_2_MUX", 357 },
	{ "CLK_OUT_3_MUX", 358 },
	{ "DSIA_MUX", 359 },
	{ "DSIB_MUX", 360 },
	{ "SOR0_LVDS", 361 },
	{ "XUSB_SS_DIV2", 362 },
	{ "PLL_M_UD", 363 },
	{ "PLL_C_UD", 364 },
	{ "SCLK_MUX", 365 },
};

static struct clk *tegra210_car_clock_get(void *, const char *);
static void	tegra210_car_clock_put(void *, struct clk *);
static u_int	tegra210_car_clock_get_rate(void *, struct clk *);
static int	tegra210_car_clock_set_rate(void *, struct clk *, u_int);
static int	tegra210_car_clock_enable(void *, struct clk *);
static int	tegra210_car_clock_disable(void *, struct clk *);
static int	tegra210_car_clock_set_parent(void *, struct clk *,
		    struct clk *);
static struct clk *tegra210_car_clock_get_parent(void *, struct clk *);

static const struct clk_funcs tegra210_car_clock_funcs = {
	.get = tegra210_car_clock_get,
	.put = tegra210_car_clock_put,
	.get_rate = tegra210_car_clock_get_rate,
	.set_rate = tegra210_car_clock_set_rate,
	.enable = tegra210_car_clock_enable,
	.disable = tegra210_car_clock_disable,
	.set_parent = tegra210_car_clock_set_parent,
	.get_parent = tegra210_car_clock_get_parent,
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

#define CLK_GATE_Y(_name, _parent, _bits) 			\
	CLK_GATE(_name, _parent,				\
		 CAR_CLK_ENB_Y_SET_REG, CAR_CLK_ENB_Y_CLR_REG,	\
		 _bits)


#define CLK_GATE_SIMPLE(_name, _parent, _reg, _bits)		\
	CLK_GATE(_name, _parent, _reg, _reg, _bits)

static const char *mux_uart_p[] =
	{ "PLL_P", "PLL_C2", "PLL_C", "PLL_C4_OUT0",
	  NULL, "PLL_C4_OUT1", "CLK_M", "PLL_C4_OUT2" };

static const char *mux_sdmmc1_p[] =
	{ "PLL_P", "PLL_A", "PLL_C", "PLL_C4_OUT0",
	  "PLL_M", "PLL_E", "CLK_M", "PLL_C4_OUT0" };

static const char *mux_sdmmc2_4_p[] =
	{ "PLL_P", "PLL_C4_OUT2"/*LJ*/, "PLL_C4_OUT0"/*LJ*/, "PLL_C4_OUT2",
	  "PLL_M", "PLL_E", "CLK_M", "PLL_C4_OUT0" };

static const char *mux_sdmmc3_p[] =
	{ "PLL_P", "PLL_A", "PLL_C", "PLL_C4_OUT2",
	  "PLL_C4_OUT1", "PLL_E", "CLK_M", "PLL_C4_OUT0" };

static const char *mux_i2c_p[] =
	{ "PLL_P", "PLL_C2_OUT0", "PLL_C", "PLL_C4_OUT0",
	  NULL, "PLL_C4_OUT1", "CLK_M", "PLL_C4_OUT2" };

static const char *mux_xusb_host_p[] =
	{ "CLK_M", "PLL_P", NULL, NULL,
	  NULL, "PLL_REF", NULL, NULL };

static const char *mux_xusb_fs_p[] =
	{ "CLK_M", NULL, "PLL_U_48M", NULL,
	  "PLL_P", NULL, "PLL_U_480M", NULL };

static const char *mux_xusb_ss_p[] =
	{ "CLK_M", "PLL_REF", "CLK_32K", "PLL_U_480M",
	  NULL, NULL, NULL, NULL };

static const char *mux_mselect_p[] =
	{ "PLL_P", "PLL_C2", "PLL_C", "PLL_C4_OUT2",
	  "PLL_C4_OUT1", "CLK_S", "CLK_M", "PLL_C4_OUT0" };

static const char *mux_tsensor_p[] =
	{ "PLL_P", "PLL_C2", "PLL_C", "PLL_C4_OUT0",
	  "CLK_M", "PLL_C4_OUT1", "CLK_S", "PLL_C4_OUT2" };

static const char *mux_soc_therm_p[] =
	{ "CLK_M", "PLL_C", "PLL_P", "PLL_A",
	  "PLL_C2", "PLL_C4_OUT0", "PLL_C4_OUT1", "PLL_C4_OUT2" };

static const char *mux_hda2codec_2x_p[] =
	{ "PLL_P", "PLL_C2", "PLL_C4_OUT0", "PLL_A",
	  "PLL_A", "PLL_C4_OUT1", "CLK_M", "PLL_C4_OUT2" };

static const char *mux_hda_p[] =
	{ "PLL_P", "PLL_C2", "PLL_C", "PLL_C4_OUT0",
	  NULL, "PLL_C4_OUT1", "CLK_M", "PLL_C4_OUT2" };

static struct tegra_clk tegra210_car_clocks[] = {
	CLK_FIXED("CLK_M", TEGRA210_REF_FREQ),

	CLK_PLL("PLL_P", "CLK_M", CAR_PLLP_BASE_REG,
		CAR_PLLP_BASE_DIVM, CAR_PLLP_BASE_DIVN, CAR_PLLP_BASE_DIVP),
	CLK_PLL("PLL_C", "CLK_M", CAR_PLLC_BASE_REG,
		CAR_PLLC_BASE_DIVM, CAR_PLLC_BASE_DIVN, CAR_PLLC_BASE_DIVP),
	CLK_PLL("PLL_U", "CLK_M", CAR_PLLU_BASE_REG,
		CAR_PLLU_BASE_DIVM, CAR_PLLU_BASE_DIVN, CAR_PLLU_BASE_DIVP),
	CLK_PLL("PLL_X", "CLK_M", CAR_PLLX_BASE_REG,
		CAR_PLLX_BASE_DIVM, CAR_PLLX_BASE_DIVN, CAR_PLLX_BASE_DIVP),
	CLK_PLL("PLL_E", "CLK_M", CAR_PLLE_BASE_REG,
		CAR_PLLE_BASE_DIVM, CAR_PLLE_BASE_DIVN, CAR_PLLE_BASE_DIVP_CML),
	CLK_PLL("PLL_D", "CLK_M", CAR_PLLD_BASE_REG,
		CAR_PLLD_BASE_DIVM, CAR_PLLD_BASE_DIVN, CAR_PLLD_BASE_DIVP),
	CLK_PLL("PLL_D2", "CLK_M", CAR_PLLD2_BASE_REG,
		CAR_PLLD2_BASE_DIVM, CAR_PLLD2_BASE_DIVN, CAR_PLLD2_BASE_DIVP),
	CLK_PLL("PLL_REF", "CLK_M", CAR_PLLREFE_BASE_REG,
		CAR_PLLREFE_BASE_DIVM, CAR_PLLREFE_BASE_DIVN, CAR_PLLREFE_BASE_DIVP),

	CLK_FIXED_DIV("PLL_U_480M", "PLL_U", 1),
	CLK_FIXED_DIV("PLL_U_48M", "PLL_U", 10),

	CLK_MUX("MUX_UARTA", CAR_CLKSRC_UARTA_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("MUX_UARTB", CAR_CLKSRC_UARTB_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("MUX_UARTC", CAR_CLKSRC_UARTC_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),
	CLK_MUX("MUX_UARTD", CAR_CLKSRC_UARTD_REG, CAR_CLKSRC_UART_SRC,
		mux_uart_p),

	CLK_MUX("MUX_SDMMC1", CAR_CLKSRC_SDMMC1_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc1_p),
	CLK_MUX("MUX_SDMMC2", CAR_CLKSRC_SDMMC2_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc2_4_p),
	CLK_MUX("MUX_SDMMC3", CAR_CLKSRC_SDMMC3_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc3_p),
	CLK_MUX("MUX_SDMMC4", CAR_CLKSRC_SDMMC4_REG, CAR_CLKSRC_SDMMC_SRC,
	 	mux_sdmmc2_4_p),

	CLK_MUX("MUX_I2C1", CAR_CLKSRC_I2C1_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("MUX_I2C2", CAR_CLKSRC_I2C2_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("MUX_I2C3", CAR_CLKSRC_I2C3_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("MUX_I2C4", CAR_CLKSRC_I2C4_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("MUX_I2C5", CAR_CLKSRC_I2C5_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),
	CLK_MUX("MUX_I2C6", CAR_CLKSRC_I2C6_REG, CAR_CLKSRC_I2C_SRC, mux_i2c_p),

	CLK_MUX("MUX_XUSB_HOST",
		CAR_CLKSRC_XUSB_HOST_REG, CAR_CLKSRC_XUSB_HOST_SRC,
		mux_xusb_host_p),
	CLK_MUX("MUX_XUSB_FALCON",
		CAR_CLKSRC_XUSB_FALCON_REG, CAR_CLKSRC_XUSB_FALCON_SRC,
		mux_xusb_host_p),
	CLK_MUX("MUX_XUSB_SS",
		CAR_CLKSRC_XUSB_SS_REG, CAR_CLKSRC_XUSB_SS_SRC,
		mux_xusb_ss_p),
	CLK_MUX("MUX_XUSB_FS",
		CAR_CLKSRC_XUSB_FS_REG, CAR_CLKSRC_XUSB_FS_SRC,
		mux_xusb_fs_p),

	CLK_MUX("MUX_MSELECT",
		CAR_CLKSRC_MSELECT_REG, CAR_CLKSRC_MSELECT_SRC,
		mux_mselect_p),

	CLK_MUX("MUX_TSENSOR",
		CAR_CLKSRC_TSENSOR_REG, CAR_CLKSRC_TSENSOR_SRC,
		mux_tsensor_p),
	CLK_MUX("MUX_SOC_THERM",
		CAR_CLKSRC_SOC_THERM_REG, CAR_CLKSRC_SOC_THERM_SRC,
		mux_soc_therm_p),

	CLK_MUX("MUX_HDA2CODEC_2X",
		CAR_CLKSRC_HDA2CODEC_2X_REG, CAR_CLKSRC_HDA2CODEC_2X_SRC,
		mux_hda2codec_2x_p),
	CLK_MUX("MUX_HDA",
		CAR_CLKSRC_HDA_REG, CAR_CLKSRC_HDA_SRC,
		mux_hda_p),

	CLK_DIV("DIV_UARTA", "MUX_UARTA",
		CAR_CLKSRC_UARTA_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("DIV_UARTB", "MUX_UARTB",
		CAR_CLKSRC_UARTB_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("DIV_UARTC", "MUX_UARTC",
		CAR_CLKSRC_UARTC_REG, CAR_CLKSRC_UART_DIV),
	CLK_DIV("DIV_UARTD", "MUX_UARTD",
		CAR_CLKSRC_UARTD_REG, CAR_CLKSRC_UART_DIV),

	CLK_DIV("DIV_SDMMC1", "MUX_SDMMC1",
		CAR_CLKSRC_SDMMC1_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("DIV_SDMMC2", "MUX_SDMMC2",
		CAR_CLKSRC_SDMMC2_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("DIV_SDMMC3", "MUX_SDMMC3",
		CAR_CLKSRC_SDMMC3_REG, CAR_CLKSRC_SDMMC_DIV),
	CLK_DIV("DIV_SDMMC4", "MUX_SDMMC4",
		CAR_CLKSRC_SDMMC4_REG, CAR_CLKSRC_SDMMC_DIV),

	CLK_DIV("DIV_I2C1", "MUX_I2C1", 
		CAR_CLKSRC_I2C1_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("DIV_I2C2", "MUX_I2C2", 
		CAR_CLKSRC_I2C2_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("DIV_I2C3", "MUX_I2C3", 
		CAR_CLKSRC_I2C3_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("DIV_I2C4", "MUX_I2C4", 
		CAR_CLKSRC_I2C4_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("DIV_I2C5", "MUX_I2C5", 
		CAR_CLKSRC_I2C5_REG, CAR_CLKSRC_I2C_DIV),
	CLK_DIV("DIV_I2C6", "MUX_I2C6", 
		CAR_CLKSRC_I2C6_REG, CAR_CLKSRC_I2C_DIV),

	CLK_DIV("XUSB_HOST_SRC", "MUX_XUSB_HOST",
		CAR_CLKSRC_XUSB_HOST_REG, CAR_CLKSRC_XUSB_HOST_DIV),
	CLK_DIV("XUSB_SS_SRC", "MUX_XUSB_SS",
		CAR_CLKSRC_XUSB_SS_REG, CAR_CLKSRC_XUSB_SS_DIV),
	CLK_DIV("XUSB_FS_SRC", "MUX_XUSB_FS",
		CAR_CLKSRC_XUSB_FS_REG, CAR_CLKSRC_XUSB_FS_DIV),
	CLK_DIV("XUSB_FALCON_SRC", "MUX_XUSB_FALCON",
		CAR_CLKSRC_XUSB_FALCON_REG, CAR_CLKSRC_XUSB_FALCON_DIV),
	CLK_DIV("USB2_HSIC_TRK", "CLK_M",
		CAR_CLKSRC_USB2_HSIC_TRK_REG, CAR_CLKSRC_USB2_HSIC_TRK_DIV),
	CLK_DIV("DIV_PLL_U_OUT1", "PLL_U",
		CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT1_RATIO),
	CLK_DIV("DIV_PLL_U_OUT2", "PLL_U",
		CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT2_RATIO),

	CLK_DIV("DIV_MSELECT", "MUX_MSELECT",
		CAR_CLKSRC_MSELECT_REG, CAR_CLKSRC_MSELECT_DIV),

        CLK_DIV("DIV_TSENSOR", "MUX_TSENSOR",
                CAR_CLKSRC_TSENSOR_REG, CAR_CLKSRC_TSENSOR_DIV),
	CLK_DIV("DIV_SOC_THERM", "MUX_SOC_THERM",
		CAR_CLKSRC_SOC_THERM_REG, CAR_CLKSRC_SOC_THERM_DIV),

	CLK_DIV("DIV_HDA2CODEC_2X", "MUX_HDA2CODEC_2X",
		CAR_CLKSRC_HDA2CODEC_2X_REG, CAR_CLKSRC_HDA2CODEC_2X_DIV),
	CLK_DIV("DIV_HDA", "MUX_HDA",
		CAR_CLKSRC_HDA_REG, CAR_CLKSRC_HDA_DIV),

	CLK_GATE_SIMPLE("PLL_U_OUT1", "DIV_PLL_U_OUT1",
		 CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT1_CLKEN),
	CLK_GATE_SIMPLE("PLL_U_OUT2", "DIV_PLL_U_OUT2",
		 CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT2_CLKEN),

	CLK_GATE_SIMPLE("CML0", "PLL_E",
		 CAR_PLLE_AUX_REG, CAR_PLLE_AUX_CML0_OEN),
	CLK_GATE_SIMPLE("CML1", "PLL_E",
		 CAR_PLLE_AUX_REG, CAR_PLLE_AUX_CML1_OEN),

	CLK_GATE_L("UARTA", "DIV_UARTA", CAR_DEV_L_UARTA),
	CLK_GATE_L("UARTB", "DIV_UARTB", CAR_DEV_L_UARTB),
	CLK_GATE_H("UARTC", "DIV_UARTC", CAR_DEV_H_UARTC),
	CLK_GATE_U("UARTD", "DIV_UARTD", CAR_DEV_U_UARTD),
	CLK_GATE_L("SDMMC1", "DIV_SDMMC1", CAR_DEV_L_SDMMC1),
	CLK_GATE_L("SDMMC2", "DIV_SDMMC2", CAR_DEV_L_SDMMC2),
	CLK_GATE_U("SDMMC3", "DIV_SDMMC3", CAR_DEV_U_SDMMC3),
	CLK_GATE_L("SDMMC4", "DIV_SDMMC4", CAR_DEV_L_SDMMC4),
	CLK_GATE_L("I2C1", "DIV_I2C1", CAR_DEV_L_I2C1),
	CLK_GATE_H("I2C2", "DIV_I2C2", CAR_DEV_H_I2C2),
	CLK_GATE_U("I2C3", "DIV_I2C3", CAR_DEV_U_I2C3),
	CLK_GATE_V("I2C4", "DIV_I2C4", CAR_DEV_V_I2C4),
	CLK_GATE_H("I2C5", "DIV_I2C5", CAR_DEV_H_I2C5),
	CLK_GATE_X("I2C6", "DIV_I2C6", CAR_DEV_X_I2C6),
	CLK_GATE_W("XUSB_GATE", "CLK_M", CAR_DEV_W_XUSB),
	CLK_GATE_U("XUSB_HOST", "XUSB_HOST_SRC", CAR_DEV_U_XUSB_HOST),
	CLK_GATE_W("XUSB_SS", "XUSB_SS_SRC", CAR_DEV_W_XUSB_SS),
	CLK_GATE_H("FUSE", "CLK_M", CAR_DEV_H_FUSE),
	CLK_GATE_Y("USB2_TRK", "USB2_HSIC_TRK", CAR_DEV_Y_USB2_TRK),
	CLK_GATE_Y("HSIC_TRK", "USB2_HSIC_TRK", CAR_DEV_Y_HSIC_TRK),
	CLK_GATE_H("APBDMA", "CLK_M", CAR_DEV_H_APBDMA),
	CLK_GATE_L("USBD", "PLL_U_480M", CAR_DEV_L_USBD),
	CLK_GATE_H("USB2", "PLL_U_480M", CAR_DEV_H_USB2),
	CLK_GATE_V("MSELECT", "DIV_MSELECT", CAR_DEV_V_MSELECT),
	CLK_GATE_U("PCIE", "CLK_M", CAR_DEV_U_PCIE),
	CLK_GATE_U("AFI", "MSELECT", CAR_DEV_U_AFI),
	CLK_GATE_V("TSENSOR", "DIV_TSENSOR", CAR_DEV_V_TSENSOR),
	CLK_GATE_U("SOC_THERM", "DIV_SOC_THERM", CAR_DEV_U_SOC_THERM),
	CLK_GATE_W("HDA2HDMI", "CLK_M", CAR_DEV_W_HDA2HDMICODEC),
	CLK_GATE_V("HDA2CODEC_2X", "DIV_HDA2CODEC_2X", CAR_DEV_V_HDA2CODEC_2X),
	CLK_GATE_V("HDA", "DIV_HDA", CAR_DEV_V_HDA),
};

struct tegra210_init_parent {
	const char *clock;
	const char *parent;
	u_int rate;
	u_int enable;
} tegra210_init_parents[] = {
	{ "SDMMC1", 		"PLL_P", 0, 0 },
	{ "SDMMC2",		"PLL_P", 0, 0 },
	{ "SDMMC3",		"PLL_P", 0, 0 },
	{ "SDMMC4",		"PLL_P", 0, 0 },
	{ "SOC_THERM",		"PLL_P", 0, 0 },
	{ "TSENSOR",		"CLK_M", 0, 0 },
	{ "XUSB_GATE",		NULL, 0, 1 },
	{ "XUSB_HOST_SRC",	"PLL_P", 102000000, 0 },
	{ "XUSB_FALCON_SRC",	"PLL_P", 204000000, 0 },
	{ "XUSB_SS_SRC",	"PLL_U_480M", 120000000, 0 },
	{ "XUSB_FS_SRC",	"PLL_U_48M", 48000000, 0 },
	{ "PLL_U_OUT1",		NULL, 48000000, 1 },
	{ "PLL_U_OUT2",		NULL, 60000000, 1 },
	{ "CML0",		NULL, 0, 1 },
	{ "AFI",		NULL, 0, 1 },
	{ "PCIE",		NULL, 0, 1 },
};

struct tegra210_car_rst {
	u_int	set_reg;
	u_int	clr_reg;
	u_int	mask;
};

static struct tegra210_car_reset_reg {
	u_int	set_reg;
	u_int	clr_reg;
} tegra210_car_reset_regs[] = {
	{ CAR_RST_DEV_L_SET_REG, CAR_RST_DEV_L_CLR_REG },
	{ CAR_RST_DEV_H_SET_REG, CAR_RST_DEV_H_CLR_REG },
	{ CAR_RST_DEV_U_SET_REG, CAR_RST_DEV_U_CLR_REG },
	{ CAR_RST_DEV_V_SET_REG, CAR_RST_DEV_V_CLR_REG },
	{ CAR_RST_DEV_W_SET_REG, CAR_RST_DEV_W_CLR_REG },
	{ CAR_RST_DEV_X_SET_REG, CAR_RST_DEV_X_CLR_REG },
	{ CAR_RST_DEV_Y_SET_REG, CAR_RST_DEV_Y_CLR_REG },
};

static void *	tegra210_car_reset_acquire(device_t, const void *, size_t);
static void	tegra210_car_reset_release(device_t, void *);
static int	tegra210_car_reset_assert(device_t, void *);
static int	tegra210_car_reset_deassert(device_t, void *);

static const struct fdtbus_reset_controller_func tegra210_car_fdtreset_funcs = {
	.acquire = tegra210_car_reset_acquire,
	.release = tegra210_car_reset_release,
	.reset_assert = tegra210_car_reset_assert,
	.reset_deassert = tegra210_car_reset_deassert,
};

struct tegra210_car_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;

	u_int			sc_clock_cells;
	u_int			sc_reset_cells;

	kmutex_t		sc_rndlock;
	krndsource_t		sc_rndsource;
};

static void	tegra210_car_init(struct tegra210_car_softc *);
static void	tegra210_car_utmip_init(struct tegra210_car_softc *);
static void	tegra210_car_xusb_init(struct tegra210_car_softc *);
static void	tegra210_car_watchdog_init(struct tegra210_car_softc *);
static void	tegra210_car_parent_init(struct tegra210_car_softc *);


CFATTACH_DECL_NEW(tegra210_car, sizeof(struct tegra210_car_softc),
	tegra210_car_match, tegra210_car_attach, NULL, NULL);

static int
tegra210_car_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra210-car", NULL };
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
tegra210_car_attach(device_t parent, device_t self, void *aux)
{
	struct tegra210_car_softc * const sc = device_private(self);
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

	sc->sc_clkdom.funcs = &tegra210_car_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (n = 0; n < __arraycount(tegra210_car_clocks); n++)
		tegra210_car_clocks[n].base.domain = &sc->sc_clkdom;

	fdtbus_register_clock_controller(self, phandle,
	    &tegra210_car_fdtclock_funcs);
	fdtbus_register_reset_controller(self, phandle,
	    &tegra210_car_fdtreset_funcs);

	tegra210_car_init(sc);

#ifdef TEGRA210_CAR_DEBUG
	for (n = 0; n < __arraycount(tegra210_car_clocks); n++) {
		struct clk *clk = TEGRA_CLK_BASE(&tegra210_car_clocks[n]);
		struct clk *clk_parent = clk_get_parent(clk);
		device_printf(self, "clk %s (parent %s): ", clk->name,
		    clk_parent ? clk_parent->name : "none");
		printf("%u Hz\n", clk_get_rate(clk));
	}
#endif
}

static void
tegra210_car_init(struct tegra210_car_softc *sc)
{
	tegra210_car_parent_init(sc);
	tegra210_car_utmip_init(sc);
	tegra210_car_xusb_init(sc);
	tegra210_car_watchdog_init(sc);
}

static void
tegra210_car_parent_init(struct tegra210_car_softc *sc)
{
	struct clk *clk, *clk_parent;
	int error;
	u_int n;

	for (n = 0; n < __arraycount(tegra210_init_parents); n++) {
		clk = clk_get(&sc->sc_clkdom, tegra210_init_parents[n].clock);
		KASSERTMSG(clk != NULL, "tegra210 clock %s not found", tegra210_init_parents[n].clock);

		if (tegra210_init_parents[n].parent != NULL) {
			clk_parent = clk_get(&sc->sc_clkdom,
			    tegra210_init_parents[n].parent);
			KASSERT(clk_parent != NULL);

			error = clk_set_parent(clk, clk_parent);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't set '%s' parent to '%s': %d\n",
				    clk->name, clk_parent->name, error);
			}
			clk_put(clk_parent);
		}
		if (tegra210_init_parents[n].rate != 0) {
			error = clk_set_rate(clk, tegra210_init_parents[n].rate);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't set '%s' rate to %u Hz: %d\n",
				    clk->name, tegra210_init_parents[n].rate,
				    error);
			}
		}
		if (tegra210_init_parents[n].enable) {
			error = clk_enable(clk);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "couldn't enable '%s': %d\n", clk->name,
				    error);
			}
		}
		clk_put(clk);
	}
}

static void
tegra210_car_utmip_init(struct tegra210_car_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	/*
	 * Set up the UTMI PLL.
	 */
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG3_REG,
	    0, CAR_UTMIP_PLL_CFG3_REF_SRC_SEL);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG3_REG,
	    0, CAR_UTMIP_PLL_CFG3_REF_DIS);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    0, CAR_UTMIPLL_HW_PWRDN_CFG0_IDDQ_OVERRIDE);
	delay(10);
	/* TODO UTMIP_PLL_CFG0 */
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    CAR_UTMIP_PLL_CFG2_PHY_XTAL_CLOCKEN, 0);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    0, CAR_UTMIP_PLL_CFG2_ACTIVE_DLY_COUNT);	/* Don't care */
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    0, CAR_UTMIP_PLL_CFG2_STABLE_COUNT);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    0, CAR_UTMIP_PLL_CFG1_ENABLE_DLY_COUNT);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG,
	    0x3, CAR_UTMIP_PLL_CFG1_XTAL_FREQ_COUNT);

	bus_space_write_4(bst, bsh, CAR_CLK_ENB_U_SET_REG, CAR_DEV_U_AFI);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_U_SET_REG, CAR_DEV_U_PCIE);

	bus_space_write_4(bst, bsh, CAR_RST_DEV_L_CLR_REG, CAR_DEV_L_USBD);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_H_CLR_REG, CAR_DEV_H_USB2);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_W_CLR_REG, CAR_DEV_W_XUSB);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_U_CLR_REG, CAR_DEV_U_AFI);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_U_CLR_REG, CAR_DEV_U_PCIE);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_U_CLR_REG, CAR_DEV_U_PCIEXCLK);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_Y_CLR_REG, CAR_DEV_Y_PEX_USB_UPHY);
	bus_space_write_4(bst, bsh, CAR_RST_DEV_Y_CLR_REG, CAR_DEV_Y_SATA_USB_UPHY);

	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG2_REG,
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_A_POWERUP |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_B_POWERUP |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_C_POWERUP,
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_A_POWERDOWN |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_B_POWERDOWN |
	    CAR_UTMIP_PLL_CFG2_PD_SAMP_C_POWERDOWN);

	/*
	 * Set up UTMI PLL under hardware control
	 */
	tegra_reg_set_clear(bst, bsh, CAR_UTMIP_PLL_CFG1_REG, 0,
	    CAR_UTMIP_PLL_CFG1_PLL_ENABLE_POWERUP | CAR_UTMIP_PLL_CFG1_PLL_ENABLE_POWERDOWN);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    0, CAR_UTMIPLL_HW_PWRDN_CFG0_IDDQ_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    CAR_UTMIPLL_HW_PWRDN_CFG0_IDDQ_PD_INCLUDE, 0);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    0, CAR_UTMIPLL_HW_PWRDN_CFG0_CLK_ENABLE_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    CAR_UTMIPLL_HW_PWRDN_CFG0_USE_LOCKDET, 0);
	tegra_reg_set_clear(bst, bsh, CLK_RST_CONTROLLER_XUSB_PLL_CFG0_REG,
	    0, CLK_RST_CONTROLLER_XUSB_PLL_CFG0_UTMIPLL_LOCK_DLY);
	delay(1);
	tegra_reg_set_clear(bst, bsh, CAR_UTMIPLL_HW_PWRDN_CFG0_REG,
	    CAR_UTMIPLL_HW_PWRDN_CFG0_SEQ_ENABLE, 0);
}

static void
tegra210_car_xusb_init(struct tegra210_car_softc *sc)
{
	const bus_space_tag_t bst = sc->sc_bst;
	const bus_space_handle_t bsh = sc->sc_bsh;
	uint32_t val;

	/*
	 * Set up the PLLU.
	 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, CAR_PLLU_BASE_OVERRIDE, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_MISC_REG, CAR_PLLU_MISC_IDDQ, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_MISC_REG, 0, CAR_PLLU_MISC_IDDQ);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_OUTA_REG, 0, CAR_PLLU_OUTA_OUT1_RSTN);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_OUTA_REG, 0, CAR_PLLU_OUTA_OUT2_RSTN);
	delay(5);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG,
	    __SHIFTIN(0x19, CAR_PLLU_BASE_DIVN) |
	    __SHIFTIN(0x2, CAR_PLLU_BASE_DIVM) |
	    __SHIFTIN(0x1, CAR_PLLU_BASE_DIVP),
	    CAR_PLLU_BASE_DIVN | CAR_PLLU_BASE_DIVM | CAR_PLLU_BASE_DIVP);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, CAR_PLLU_BASE_ENABLE, 0);
	do {
		delay(2);
		val = bus_space_read_4(bst, bsh, CAR_PLLU_BASE_REG);
	} while ((val & CAR_PLLU_BASE_LOCK) == 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, CAR_PLLU_BASE_CLKENABLE_ICUSB, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, CAR_PLLU_BASE_CLKENABLE_HSIC, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, CAR_PLLU_BASE_CLKENABLE_USB, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT1_RSTN, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_OUTA_REG, CAR_PLLU_OUTA_OUT2_RSTN, 0);
	delay(2);

	/*
	 * Now switch PLLU to hw controlled mode.
	 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, 0, CAR_PLLU_BASE_OVERRIDE);
	tegra_reg_set_clear(bst, bsh, CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_REG,
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_IDDQ_PD_INCLUDE |
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_USE_SWITCH_DETECT |
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_USE_LOCKDET,
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_CLK_ENABLE_SWCTL |
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_CLK_SWITCH_SWCTL);
	tegra_reg_set_clear(bst, bsh, CLK_RST_CONTROLLER_XUSB_PLL_CFG0_REG, 0,
	    CLK_RST_CONTROLLER_XUSB_PLL_CFG0_PLLU_LOCK_DLY);
	delay(1);
	tegra_reg_set_clear(bst, bsh, CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_REG,
	    CLK_RST_CONTROLLER_PLLU_HW_PWRDN_CFG0_SEQ_ENABLE, 0);
	delay(1);
	tegra_reg_set_clear(bst, bsh, CAR_PLLU_BASE_REG, 0, CAR_PLLU_BASE_CLKENABLE_USB);

	/*
	 * Set up PLLREFE
	 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_MISC_REG,
	    0, CAR_PLLREFE_MISC_IDDQ);
	delay(5);
	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_BASE_REG,
	    __SHIFTIN(0x4, CAR_PLLREFE_BASE_DIVM) |
	    __SHIFTIN(0x41, CAR_PLLREFE_BASE_DIVN) |
	    __SHIFTIN(0x0, CAR_PLLREFE_BASE_DIVP) |
	    __SHIFTIN(0x0, CAR_PLLREFE_BASE_KCP),
	    CAR_PLLREFE_BASE_DIVM |
	    CAR_PLLREFE_BASE_DIVN |
	    CAR_PLLREFE_BASE_DIVP |
	    CAR_PLLREFE_BASE_KCP);
	tegra_reg_set_clear(bst, bsh, CAR_PLLREFE_BASE_REG,
	    CAR_PLLREFE_BASE_ENABLE, 0);
	do {
		delay(2);
		val = bus_space_read_4(bst, bsh, CAR_PLLREFE_MISC_REG);
	} while ((val & CAR_PLLREFE_MISC_LOCK) == 0);

	/*
	 * Set up the PLLE.
	 */
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, 0, CAR_PLLE_AUX_REF_SEL_PLLREFE);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, 0, CAR_PLLE_AUX_REF_SRC);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_MISC_REG, 0, CAR_PLLE_MISC_IDDQ_OVERRIDE);
	delay(5);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_BASE_REG,
	    __SHIFTIN(0xe, CAR_PLLE_BASE_DIVP_CML) |
	    __SHIFTIN(0x7d, CAR_PLLE_BASE_DIVN) |
	    __SHIFTIN(0x2, CAR_PLLE_BASE_DIVM),
	    CAR_PLLE_BASE_DIVP_CML |
	    CAR_PLLE_BASE_DIVN |
	    CAR_PLLE_BASE_DIVM);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_MISC_REG,
	    CAR_PLLE_MISC_PTS,
	    CAR_PLLE_MISC_KCP | CAR_PLLE_MISC_VREG_CTRL | CAR_PLLE_MISC_KVCO);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_BASE_REG, CAR_PLLE_BASE_ENABLE, 0);
	do {
		delay(2);
		val = bus_space_read_4(bst, bsh, CAR_PLLE_MISC_REG);
	} while ((val & CAR_PLLE_MISC_LOCK) == 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_SS_CNTL_REG,
	    __SHIFTIN(1, CAR_PLLE_SS_CNTL_SSCINC) |
	    __SHIFTIN(0x23, CAR_PLLE_SS_CNTL_SSCINCINTRV) |
	    __SHIFTIN(0x21, CAR_PLLE_SS_CNTL_SSCMAX),
	    CAR_PLLE_SS_CNTL_SSCINC |
	    CAR_PLLE_SS_CNTL_SSCINCINTRV |
	    CAR_PLLE_SS_CNTL_SSCMAX |
	    CAR_PLLE_SS_CNTL_SSCINVERT |
	    CAR_PLLE_SS_CNTL_SSCCENTER |
	    CAR_PLLE_SS_CNTL_BYPASS_SS |
	    CAR_PLLE_SS_CNTL_SSCBYP);
	delay(1);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_SS_CNTL_REG, 0, CAR_PLLE_SS_CNTL_INTERP_RESET);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_MISC_REG, 0, CAR_PLLE_MISC_IDDQ_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, 0, CAR_PLLE_AUX_SS_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, 0, CAR_PLLE_AUX_ENABLE_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, CAR_PLLE_AUX_SS_SEQ_INCLUDE, 0);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, CAR_PLLE_AUX_USE_LOCKDET, 0);
	delay(1);
	tegra_reg_set_clear(bst, bsh, CAR_PLLE_AUX_REG, CAR_PLLE_AUX_SEQ_ENABLE, 0);

	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG, CAR_DEV_W_XUSB);
	bus_space_write_4(bst, bsh, CAR_CLK_ENB_W_SET_REG, CAR_DEV_W_XUSB_PADCTL);
}

static void
tegra210_car_watchdog_init(struct tegra210_car_softc *sc)
{
	const bus_space_tag_t bst = sc->sc_bst;
	const bus_space_handle_t bsh = sc->sc_bsh;

	/* Enable watchdog timer reset for system */
	tegra_reg_set_clear(bst, bsh, CAR_RST_SOURCE_REG,
	    CAR_RST_SOURCE_WDT_EN|CAR_RST_SOURCE_WDT_SYS_RST_EN, 0);
}

static struct tegra_clk *
tegra210_car_clock_find(const char *name)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra210_car_clocks); n++) {
		if (strcmp(tegra210_car_clocks[n].base.name, name) == 0) {
			return &tegra210_car_clocks[n];
		}
	}

	return NULL;
}

static struct tegra_clk *
tegra210_car_clock_find_by_id(u_int clock_id)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra210_car_clock_ids); n++) {
		if (tegra210_car_clock_ids[n].id == clock_id) {
			const char *name = tegra210_car_clock_ids[n].name;
			return tegra210_car_clock_find(name);
		}
	}

	return NULL;
}

static struct clk *
tegra210_car_clock_decode(device_t dev, const void *data, size_t len)
{
	struct tegra210_car_softc * const sc = device_private(dev);
	struct tegra_clk *tclk;

	if (len != sc->sc_clock_cells * 4) {
		return NULL;
	}

	const u_int clock_id = be32dec(data);

	tclk = tegra210_car_clock_find_by_id(clock_id);
	if (tclk)
		return TEGRA_CLK_BASE(tclk);

	return NULL;
}

static struct clk *
tegra210_car_clock_get(void *priv, const char *name)
{
	struct tegra_clk *tclk;

	tclk = tegra210_car_clock_find(name);
	if (tclk == NULL)
		return NULL;

	atomic_inc_uint(&tclk->refcnt);

	return TEGRA_CLK_BASE(tclk);
}

static void
tegra210_car_clock_put(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);

	KASSERT(tclk->refcnt > 0);

	atomic_dec_uint(&tclk->refcnt);
}

static u_int
tegra210_car_clock_get_rate_pll(struct tegra210_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_pll_clk *tpll = &tclk->u.pll;
	struct tegra_clk *tclk_parent;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	u_int divm, divn, divp;
	uint64_t rate;

	KASSERT(tclk->type == TEGRA_CLK_PLL);

	tclk_parent = tegra210_car_clock_find(tclk->parent);
	KASSERT(tclk_parent != NULL);

	const u_int rate_parent = tegra210_car_clock_get_rate(sc,
	    TEGRA_CLK_BASE(tclk_parent));

	const uint32_t base = bus_space_read_4(bst, bsh, tpll->base_reg);
	divm = __SHIFTOUT(base, tpll->divm_mask);
	divn = __SHIFTOUT(base, tpll->divn_mask);
	if (tpll->base_reg == CAR_PLLU_BASE_REG) {
		divp = __SHIFTOUT(base, tpll->divp_mask) ? 0 : 1;
	} else if (tpll->base_reg == CAR_PLLP_BASE_REG) {
		/* XXX divp is not applied to PLLP's primary output */
		divp = 0;
	} else if (tpll->base_reg == CAR_PLLE_BASE_REG) {
		divp = 0;
		divm *= __SHIFTOUT(base, tpll->divp_mask);
	} else {
		divp = __SHIFTOUT(base, tpll->divp_mask);
	}

	rate = (uint64_t)rate_parent * divn;
	return rate / (divm << divp);
}

static int
tegra210_car_clock_set_rate_pll(struct tegra210_car_softc *sc,
    struct tegra_clk *tclk, u_int rate)
{
	struct tegra_pll_clk *tpll = &tclk->u.pll;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	uint32_t bp, base;

	clk_parent = tegra210_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return EIO;
	const u_int rate_parent = tegra210_car_clock_get_rate(sc, clk_parent);
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
		aprint_error_dev(sc->sc_dev, "failed to set %s rate to %u\n",
		    tclk->base.name, rate);
		/* TODO */
		return EOPNOTSUPP;
	}
}

static int
tegra210_car_clock_set_parent_mux(struct tegra210_car_softc *sc,
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

	v = bus_space_read_4(bst, bsh, tmux->reg);
	v &= ~tmux->bits;
	v |= __SHIFTIN(src, tmux->bits);
	bus_space_write_4(bst, bsh, tmux->reg, v);

	return 0;
}

static struct tegra_clk *
tegra210_car_clock_get_parent_mux(struct tegra210_car_softc *sc,
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

	return tegra210_car_clock_find(tmux->parents[src]);
}

static u_int
tegra210_car_clock_get_rate_fixed_div(struct tegra210_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_fixed_div_clk *tfixed_div = &tclk->u.fixed_div;
	struct clk *clk_parent;

	clk_parent = tegra210_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return 0;
	const u_int parent_rate = tegra210_car_clock_get_rate(sc, clk_parent);

	return parent_rate / tfixed_div->div;
}

static u_int
tegra210_car_clock_get_rate_div(struct tegra210_car_softc *sc,
    struct tegra_clk *tclk)
{
	struct tegra_div_clk *tdiv = &tclk->u.div;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	u_int rate;

	KASSERT(tclk->type == TEGRA_CLK_DIV);

	clk_parent = tegra210_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	const u_int parent_rate = tegra210_car_clock_get_rate(sc, clk_parent);

	const uint32_t v = bus_space_read_4(bst, bsh, tdiv->reg);
	u_int raw_div = __SHIFTOUT(v, tdiv->bits);

	switch (tdiv->reg) {
	case CAR_CLKSRC_I2C1_REG:
	case CAR_CLKSRC_I2C2_REG:
	case CAR_CLKSRC_I2C3_REG:
	case CAR_CLKSRC_I2C4_REG:
	case CAR_CLKSRC_I2C5_REG:
	case CAR_CLKSRC_I2C6_REG:
		rate = parent_rate / (raw_div + 1);
		break;
	case CAR_CLKSRC_UARTA_REG:
	case CAR_CLKSRC_UARTB_REG:
	case CAR_CLKSRC_UARTC_REG:
	case CAR_CLKSRC_UARTD_REG:
		if (v & CAR_CLKSRC_UART_DIV_ENB) {
			rate = parent_rate / ((raw_div / 2) + 1);
		} else {
			rate = parent_rate;
		}
		break;
	case CAR_CLKSRC_SDMMC2_REG:
	case CAR_CLKSRC_SDMMC4_REG:
		switch (__SHIFTOUT(v, CAR_CLKSRC_SDMMC_SRC)) {
		case 1:
		case 2:
		case 5:
			raw_div = 0;	/* ignore divisor for _LJ options */
			break;
		}
		/* FALLTHROUGH */
	default:
		rate = parent_rate / ((raw_div / 2) + 1);
		break;
	}

	return rate;
}

static int
tegra210_car_clock_set_rate_div(struct tegra210_car_softc *sc,
    struct tegra_clk *tclk, u_int rate)
{
	struct tegra_div_clk *tdiv = &tclk->u.div;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	struct clk *clk_parent;
	u_int raw_div;
	uint32_t v;

	KASSERT(tclk->type == TEGRA_CLK_DIV);

	clk_parent = tegra210_car_clock_get_parent(sc, TEGRA_CLK_BASE(tclk));
	if (clk_parent == NULL)
		return EINVAL;
	const u_int parent_rate = tegra210_car_clock_get_rate(sc, clk_parent);

	v = bus_space_read_4(bst, bsh, tdiv->reg);

	raw_div = __SHIFTOUT(tdiv->bits, tdiv->bits);

	switch (tdiv->reg) {
	case CAR_CLKSRC_UARTA_REG:
	case CAR_CLKSRC_UARTB_REG:
	case CAR_CLKSRC_UARTC_REG:
	case CAR_CLKSRC_UARTD_REG:
		if (rate == parent_rate) {
			v &= ~CAR_CLKSRC_UART_DIV_ENB;
		} else if (rate) {
			v |= CAR_CLKSRC_UART_DIV_ENB;
			raw_div = (parent_rate / rate) * 2;
			if (raw_div >= 2)
				raw_div -= 2;
		}
		break;
	case CAR_CLKSRC_I2C1_REG:
	case CAR_CLKSRC_I2C2_REG:
	case CAR_CLKSRC_I2C3_REG:
	case CAR_CLKSRC_I2C4_REG:
	case CAR_CLKSRC_I2C5_REG:
	case CAR_CLKSRC_I2C6_REG:
		if (rate)
			raw_div = (parent_rate / rate) - 1;
		break;
	case CAR_CLKSRC_SDMMC1_REG:
	case CAR_CLKSRC_SDMMC2_REG:
	case CAR_CLKSRC_SDMMC3_REG:
	case CAR_CLKSRC_SDMMC4_REG:
		if (rate) {
			for (raw_div = 0x00; raw_div <= 0xff; raw_div++) {
				u_int calc_rate =
				    parent_rate / ((raw_div / 2) + 1);
				if (calc_rate <= rate)
					break;
			}
			if (raw_div == 0x100)
				return EINVAL;
		}
		break;
	default:
		if (rate) {
			raw_div = (parent_rate / rate) * 2;
			if (raw_div >= 2)
				raw_div -= 2;
		}
		break;
	}

	v &= ~tdiv->bits;
	v |= __SHIFTIN(raw_div, tdiv->bits);

	bus_space_write_4(bst, bsh, tdiv->reg, v);

	return 0;
}

static int
tegra210_car_clock_enable_gate(struct tegra210_car_softc *sc,
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
		bus_space_write_4(bst, bsh, reg, tgate->bits);
	}

	return 0;
}

static u_int
tegra210_car_clock_get_rate(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	switch (tclk->type) {
	case TEGRA_CLK_FIXED:
		return tclk->u.fixed.rate;
	case TEGRA_CLK_PLL:
		return tegra210_car_clock_get_rate_pll(priv, tclk);
	case TEGRA_CLK_MUX:
	case TEGRA_CLK_GATE:
		clk_parent = tegra210_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return EINVAL;
		return tegra210_car_clock_get_rate(priv, clk_parent);
	case TEGRA_CLK_FIXED_DIV:
		return tegra210_car_clock_get_rate_fixed_div(priv, tclk);
	case TEGRA_CLK_DIV:
		return tegra210_car_clock_get_rate_div(priv, tclk);
	default:
		panic("tegra210: unknown tclk type %d", tclk->type);
	}
}

static int
tegra210_car_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	KASSERT((clk->flags & CLK_SET_RATE_PARENT) == 0);

	switch (tclk->type) {
	case TEGRA_CLK_FIXED:
	case TEGRA_CLK_MUX:
		return EIO;
	case TEGRA_CLK_FIXED_DIV:
		clk_parent = tegra210_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return EIO;
		return tegra210_car_clock_set_rate(priv, clk_parent,
		    rate * tclk->u.fixed_div.div);
	case TEGRA_CLK_GATE:
		return EINVAL;
	case TEGRA_CLK_PLL:
		return tegra210_car_clock_set_rate_pll(priv, tclk, rate);
	case TEGRA_CLK_DIV:
		return tegra210_car_clock_set_rate_div(priv, tclk, rate);
	default:
		panic("tegra210: unknown tclk type %d", tclk->type);
	}
}

static int
tegra210_car_clock_enable(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct clk *clk_parent;

	if (tclk->type != TEGRA_CLK_GATE) {
		clk_parent = tegra210_car_clock_get_parent(priv, clk);
		if (clk_parent == NULL)
			return 0;
		return tegra210_car_clock_enable(priv, clk_parent);
	}

	return tegra210_car_clock_enable_gate(priv, tclk, true);
}

static int
tegra210_car_clock_disable(void *priv, struct clk *clk)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);

	if (tclk->type != TEGRA_CLK_GATE)
		return EINVAL;

	return tegra210_car_clock_enable_gate(priv, tclk, false);
}

static int
tegra210_car_clock_set_parent(void *priv, struct clk *clk,
    struct clk *clk_parent)
{
	struct tegra_clk *tclk = TEGRA_CLK_PRIV(clk);
	struct tegra_clk *tclk_parent = TEGRA_CLK_PRIV(clk_parent);
	struct clk *nclk_parent;

	if (tclk->type != TEGRA_CLK_MUX) {
		nclk_parent = tegra210_car_clock_get_parent(priv, clk);
		if (nclk_parent == clk_parent || nclk_parent == NULL)
			return EINVAL;
		return tegra210_car_clock_set_parent(priv, nclk_parent,
		    clk_parent);
	}

	return tegra210_car_clock_set_parent_mux(priv, tclk, tclk_parent);
}

static struct clk *
tegra210_car_clock_get_parent(void *priv, struct clk *clk)
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
			tclk_parent = tegra210_car_clock_find(tclk->parent);
		}
		break;
	case TEGRA_CLK_MUX:
		tclk_parent = tegra210_car_clock_get_parent_mux(priv, tclk);
		break;
	}

	if (tclk_parent == NULL)
		return NULL;

	return TEGRA_CLK_BASE(tclk_parent);
}

static void *
tegra210_car_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct tegra210_car_softc * const sc = device_private(dev);
	struct tegra210_car_rst *rst;

	if (len != sc->sc_reset_cells * 4)
		return NULL;

	const u_int reset_id = be32dec(data);

	if (reset_id >= __arraycount(tegra210_car_reset_regs) * 32)
		return NULL;

	const u_int reg = reset_id / 32;

	rst = kmem_alloc(sizeof(*rst), KM_SLEEP);
	rst->set_reg = tegra210_car_reset_regs[reg].set_reg;
	rst->clr_reg = tegra210_car_reset_regs[reg].clr_reg;
	rst->mask = __BIT(reset_id % 32);

	return rst;
}

static void
tegra210_car_reset_release(device_t dev, void *priv)
{
	struct tegra210_car_rst *rst = priv;

	kmem_free(rst, sizeof(*rst));
}

static int
tegra210_car_reset_assert(device_t dev, void *priv)
{
	struct tegra210_car_softc * const sc = device_private(dev);
	struct tegra210_car_rst *rst = priv;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, rst->set_reg, rst->mask);

	return 0;
}

static int
tegra210_car_reset_deassert(device_t dev, void *priv)
{
	struct tegra210_car_softc * const sc = device_private(dev);
	struct tegra210_car_rst *rst = priv;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, rst->clr_reg, rst->mask);

	return 0;
}

void
tegra210_car_xusbio_enable_hw_control(void)
{
	device_t dev = device_find_by_driver_unit("tegra210car", 0);
	KASSERT(dev != NULL);
	struct tegra210_car_softc * const sc = device_private(dev);
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	tegra_reg_set_clear(bst, bsh, CAR_XUSBIO_PLL_CFG0_REG,
	    0,
	    CAR_XUSBIO_PLL_CFG0_CLK_ENABLE_SWCTL |
	    CAR_XUSBIO_PLL_CFG0_PADPLL_RESET_SWCTL);
	tegra_reg_set_clear(bst, bsh, CAR_XUSBIO_PLL_CFG0_REG,
	    CAR_XUSBIO_PLL_CFG0_PADPLL_SLEEP_IDDQ |
	    CAR_XUSBIO_PLL_CFG0_PADPLL_USE_LOCKDET,
	    0);
}

void
tegra210_car_xusbio_enable_hw_seq(void)
{
	device_t dev = device_find_by_driver_unit("tegra210car", 0);
	KASSERT(dev != NULL);
	struct tegra210_car_softc * const sc = device_private(dev);
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	tegra_reg_set_clear(bst, bsh, CAR_XUSBIO_PLL_CFG0_REG,
	    CAR_XUSBIO_PLL_CFG0_SEQ_ENABLE, 0);
}
