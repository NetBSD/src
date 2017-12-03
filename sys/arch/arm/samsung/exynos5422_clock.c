/* $NetBSD: exynos5422_clock.c,v 1.6.4.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos5422_clock.c,v 1.6.4.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>

#include <dev/clk/clk_backend.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/exynos_clock.h>

#include <dev/fdt/fdtvar.h>

static struct clk *exynos5422_clock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func exynos5422_car_fdtclock_funcs = {
	.decode = exynos5422_clock_decode
};

/* DT clock ID to clock name mappings */
static struct exynos5422_clock_id {
	u_int		id;
	const char	*name;
} exynos5422_clock_ids[] = {
    { 1, "fin_pll" },
    { 2, "fout_apll" },
    { 3, "fout_cpll" },
    { 4, "fout_dpll" },
    { 5, "fout_epll" },
    { 6, "fout_rpll" },
    { 7, "fout_ipll" },
    { 8, "fout_spll" },
    { 9, "fout_vpll" },
    { 10, "fout_mpll" },
    { 11, "fout_bpll" },
    { 12, "fout_kpll" },
    { 128, "sclk_uart0" },
    { 129, "sclk_uart1" },
    { 130, "sclk_uart2" },
    { 131, "sclk_uart3" },
    { 132, "sclk_mmc0" },
    { 133, "sclk_mmc1" },
    { 134, "sclk_mmc2" },
    { 135, "sclk_spi0" },
    { 136, "sclk_spi1" },
    { 137, "sclk_spi2" },
    { 138, "sclk_i2s1" },
    { 139, "sclk_i2s2" },
    { 140, "sclk_pcm1" },
    { 141, "sclk_pcm2" },
    { 142, "sclk_spdif" },
    { 143, "sclk_hdmi" },
    { 144, "sclk_pixel" },
    { 145, "sclk_dp1" },
    { 146, "sclk_mipi1" },
    { 147, "sclk_fimd1" },
    { 148, "sclk_maudio0" },
    { 149, "sclk_maupcm0" },
    { 150, "sclk_usbd300" },
    { 151, "sclk_usbd301" },
    { 152, "sclk_usbphy300" },
    { 153, "sclk_usbphy301" },
    { 154, "sclk_unipro" },
    { 155, "sclk_pwm" },
    { 156, "sclk_gscl_wa" },
    { 157, "sclk_gscl_wb" },
    { 158, "sclk_hdmiphy" },
    { 159, "mau_epll" },
    { 160, "sclk_hsic_12m" },
    { 161, "sclk_mphy_ixtal24" },
    { 257, "uart0" },
    { 258, "uart1" },
    { 259, "uart2" },
    { 260, "uart3" },
    { 261, "i2c0" },
    { 262, "i2c1" },
    { 263, "i2c2" },
    { 264, "i2c3" },
    { 265, "usi0" },
    { 266, "usi1" },
    { 267, "usi2" },
    { 268, "usi3" },
    { 269, "i2c_hdmi" },
    { 270, "tsadc" },
    { 271, "spi0" },
    { 272, "spi1" },
    { 273, "spi2" },
    { 274, "keyif" },
    { 275, "i2s1" },
    { 276, "i2s2" },
    { 277, "pcm1" },
    { 278, "pcm2" },
    { 279, "pwm" },
    { 280, "spdif" },
    { 281, "usi4" },
    { 282, "usi5" },
    { 283, "usi6" },
    { 300, "aclk66_psgen" },
    { 301, "chipid" },
    { 302, "sysreg" },
    { 303, "tzpc0" },
    { 304, "tzpc1" },
    { 305, "tzpc2" },
    { 306, "tzpc3" },
    { 307, "tzpc4" },
    { 308, "tzpc5" },
    { 309, "tzpc6" },
    { 310, "tzpc7" },
    { 311, "tzpc8" },
    { 312, "tzpc9" },
    { 313, "hdmi_cec" },
    { 314, "seckey" },
    { 315, "mct" },
    { 316, "wdt" },
    { 317, "rtc" },
    { 318, "tmu" },
    { 319, "tmu_gpu" },
    { 330, "pclk66_gpio" },
    { 350, "aclk200_fsys2" },
    { 351, "mout_mmc0" },
    { 352, "mout_mmc1" },
    { 353, "mout_mmc2" },
    { 354, "sromc" },
    { 355, "ufs" },
    { 360, "aclk200_fsys" },
    { 361, "tsi" },
    { 362, "pdma0" },
    { 363, "pdma1" },
    { 364, "rtic" },
    { 365, "usbh20" },
    { 366, "usbd300" },
    { 367, "usbd301" },
    { 380, "aclk400_mscl" },
    { 381, "mscl0" },
    { 382, "mscl1" },
    { 383, "mscl2" },
    { 384, "smmu_mscl0" },
    { 385, "smmu_mscl1" },
    { 386, "smmu_mscl2" },
    { 400, "aclk333" },
    { 401, "mfc" },
    { 402, "smmu_mfcl" },
    { 403, "smmu_mfcr" },
    { 410, "aclk200_disp1" },
    { 411, "dsim1" },
    { 412, "dp1" },
    { 413, "hdmi" },
    { 420, "aclk300_disp1" },
    { 421, "fimd1" },
    { 422, "smmu_fimd1m0" },
    { 423, "smmu_fimd1m1" },
    { 430, "aclk166" },
    { 431, "mixer" },
    { 440, "aclk266" },
    { 441, "rotator" },
    { 442, "mdma1" },
    { 443, "smmu_rotator" },
    { 444, "smmu_mdma1" },
    { 450, "aclk300_jpeg" },
    { 451, "jpeg" },
    { 452, "jpeg2" },
    { 453, "smmu_jpeg" },
    { 454, "smmu_jpeg2" },
    { 460, "aclk300_gscl" },
    { 461, "smmu_gscl0" },
    { 462, "smmu_gscl1" },
    { 463, "gscl_wa" },
    { 464, "gscl_wb" },
    { 465, "gscl0" },
    { 466, "gscl1" },
    { 467, "fimc_3aa" },
    { 470, "aclk266_g2d" },
    { 471, "sss" },
    { 472, "slim_sss" },
    { 473, "mdma0" },
    { 480, "aclk333_g2d" },
    { 481, "g2d" },
    { 490, "aclk333_432_gscl" },
    { 491, "smmu_3aa" },
    { 492, "smmu_fimcl0" },
    { 493, "smmu_fimcl1" },
    { 494, "smmu_fimcl3" },
    { 495, "fimc_lite3" },
    { 496, "fimc_lite0" },
    { 497, "fimc_lite1" },
    { 500, "aclk_g3d" },
    { 501, "g3d" },
    { 502, "smmu_mixer" },
    { 503, "smmu_g2d" },
    { 504, "smmu_mdma0" },
    { 505, "mc" },
    { 506, "top_rtc" },
    { 510, "sclk_uart_isp" },
    { 511, "sclk_spi0_isp" },
    { 512, "sclk_spi1_isp" },
    { 513, "sclk_pwm_isp" },
    { 514, "sclk_isp_sensor0" },
    { 515, "sclk_isp_sensor1" },
    { 516, "sclk_isp_sensor2" },
    { 517, "aclk432_scaler" },
    { 518, "aclk432_cam" },
    { 519, "aclk_fl1550_cam" },
    { 520, "aclk550_cam" },
    { 640, "mout_hdmi" },
    { 641, "mout_g3d" },
    { 642, "mout_vpll" },
    { 643, "mout_maudio0" },
    { 644, "mout_user_aclk333" },
    { 645, "mout_sw_aclk333" },
    { 646, "mout_user_aclk200_disp1" },
    { 647, "mout_sw_aclk200" },
    { 648, "mout_user_aclk300_disp1" },
    { 649, "mout_sw_aclk300" },
    { 650, "mout_user_aclk400_disp1" },
    { 651, "mout_sw_aclk400" },
    { 768, "dout_pixel" },
};

static struct clk *exynos5422_clock_get(void *, const char *);
static void	exynos5422_clock_put(void *, struct clk *);
static u_int	exynos5422_clock_get_rate(void *, struct clk *);
static int	exynos5422_clock_set_rate(void *, struct clk *, u_int);
static int	exynos5422_clock_enable(void *, struct clk *);
static int	exynos5422_clock_disable(void *, struct clk *);
static int	exynos5422_clock_set_parent(void *, struct clk *, struct clk *);
static struct clk *exynos5422_clock_get_parent(void *, struct clk *);

static const struct clk_funcs exynos5422_clock_funcs = {
	.get = exynos5422_clock_get,
	.put = exynos5422_clock_put,
	.get_rate = exynos5422_clock_get_rate,
	.set_rate = exynos5422_clock_set_rate,
	.enable = exynos5422_clock_enable,
	.disable = exynos5422_clock_disable,
	.set_parent = exynos5422_clock_set_parent,
	.get_parent = exynos5422_clock_get_parent,
};

#define CLK_FIXED(_name, _rate)	{				\
	.base = { .name = (_name) }, .type = EXYNOS_CLK_FIXED,	\
	.u = { .fixed = { .rate = (_rate) } }			\
}

#define CLK_PLL(_name, _parent, _lock, _con0) {			\
	.base = { .name = (_name) }, .type = EXYNOS_CLK_PLL,	\
	.parent = (_parent),					\
	.u = {							\
		.pll = {					\
			.lock_reg = (_lock),			\
			.con0_reg = (_con0),			\
		}						\
	}							\
}

#define CLK_MUXF(_name, _alias, _reg, _bits, _f, _p) {		\
	.base = { .name = (_name), .flags = (_f) },		\
	.type = EXYNOS_CLK_MUX,					\
	.alias = (_alias),					\
	.u = {							\
		.mux = {					\
	  		.nparents = __arraycount(_p),		\
	  		.parents = (_p),			\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define CLK_MUXA(_name, _alias, _reg, _bits, _p)		\
	CLK_MUXF(_name, _alias, _reg, _bits, 0, _p)

#define CLK_MUX(_name, _reg, _bits, _p)				\
	CLK_MUXF(_name, NULL, _reg, _bits, 0, _p)

#define CLK_DIV(_name, _parent, _reg, _bits) {			\
	.base = { .name = (_name) }, .type = EXYNOS_CLK_DIV,	\
	.parent = (_parent),					\
	.u = {							\
		.div = {					\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define CLK_GATE(_name, _parent, _reg, _bits, _f) {		\
	.base = { .name = (_name), .flags = (_f) },		\
	.type = EXYNOS_CLK_GATE,				\
	.parent = (_parent),					\
	.u = {							\
		.gate = {					\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define EXYNOS5422_APLL_LOCK		0x00000
#define EXYNOS5422_APLL_CON0		0x00100
#define EXYNOS5422_CPLL_LOCK		0x10020
#define EXYNOS5422_DPLL_LOCK		0x10030
#define EXYNOS5422_EPLL_LOCK		0x10040
#define EXYNOS5422_RPLL_LOCK		0x10050
#define EXYNOS5422_IPLL_LOCK		0x10060
#define EXYNOS5422_SPLL_LOCK		0x10070
#define EXYNOS5422_VPLL_LOCK		0x10080
#define EXYNOS5422_MPLL_LOCK		0x10090
#define EXYNOS5422_CPLL_CON0		0x10120
#define EXYNOS5422_DPLL_CON0		0x10128
#define EXYNOS5422_EPLL_CON0		0x10130
#define EXYNOS5422_EPLL_CON1		0x10134
#define EXYNOS5422_EPLL_CON2		0x10138
#define EXYNOS5422_RPLL_CON0		0x10140
#define EXYNOS5422_RPLL_CON1		0x10144
#define EXYNOS5422_RPLL_CON2		0x10148
#define EXYNOS5422_IPLL_CON0		0x10150
#define EXYNOS5422_SPLL_CON0		0x10160
#define EXYNOS5422_VPLL_CON0		0x10170
#define EXYNOS5422_MPLL_CON0		0x10180
#define EXYNOS5422_BPLL_LOCK		0x20010
#define EXYNOS5422_BPLL_CON0		0x20110
#define EXYNOS5422_KPLL_LOCK		0x28000
#define EXYNOS5422_KPLL_CON0		0x28100

#define EXYNOS5422_SRC_CPU		0x00200
#define EXYNOS5422_SRC_TOP0		0x10200
#define EXYNOS5422_SRC_TOP1		0x10204
#define EXYNOS5422_SRC_TOP2		0x10208
#define EXYNOS5422_SRC_TOP3		0x1020c
#define EXYNOS5422_SRC_TOP4		0x10210
#define EXYNOS5422_SRC_TOP5		0x10214
#define EXYNOS5422_SRC_TOP6		0x10218
#define EXYNOS5422_SRC_TOP7		0x1021c
#define EXYNOS5422_SRC_DISP10		0x1022c
#define EXYNOS5422_SRC_MAU		0x10240
#define EXYNOS5422_SRC_FSYS		0x10244
#define EXYNOS5422_SRC_PERIC0		0x10250
#define EXYNOS5422_SRC_PERIC1		0x10254
#define EXYNOS5422_SRC_ISP		0x10270
#define EXYNOS5422_SRC_TOP10		0x10280
#define EXYNOS5422_SRC_TOP11		0x10280
#define EXYNOS5422_SRC_TOP12		0x10280

#define EXYNOS5422_DIV_FSYS1		0x1054c
#define EXYNOS5422_DIV_PERIC0		0x10558

#define EXYNOS5422_GATE_TOP_SCLK_FSYS	0x10840
#define EXYNOS5422_GATE_TOP_SCLK_PERIC	0x10850

static const char *mout_cpll_p[] = { "fin_pll", "fout_cpll" };
static const char *mout_dpll_p[] = { "fin_pll", "fout_dpll" };
static const char *mout_mpll_p[] = { "fin_pll", "fout_mpll" };
static const char *mout_spll_p[] = { "fin_pll", "fout_spll" };
static const char *mout_ipll_p[] = { "fin_pll", "fout_ipll" };
static const char *mout_epll_p[] = { "fin_pll", "fout_epll" };
static const char *mout_rpll_p[] = { "fin_pll", "fout_rpll" };
static const char *mout_group2_p[] =
	{ "fin_pll", "sclk_cpll", "sclk_dpll", "sclk_mpll",
	  "sclk_spll", "sclk_ipll", "sclk_epll", "sclk_rpll" };

static struct exynos_clk exynos5422_clocks[] = {
	CLK_FIXED("fin_pll", EXYNOS_F_IN_FREQ),

	CLK_PLL("fout_apll", "fin_pll", EXYNOS5422_APLL_LOCK,
					EXYNOS5422_APLL_CON0),
	CLK_PLL("fout_cpll", "fin_pll", EXYNOS5422_CPLL_LOCK,
					EXYNOS5422_CPLL_CON0),
	CLK_PLL("fout_dpll", "fin_pll", EXYNOS5422_DPLL_LOCK,
					EXYNOS5422_DPLL_CON0),
	CLK_PLL("fout_epll", "fin_pll", EXYNOS5422_EPLL_LOCK,
					EXYNOS5422_EPLL_CON0),
	CLK_PLL("fout_rpll", "fin_pll", EXYNOS5422_RPLL_LOCK,
					EXYNOS5422_RPLL_CON0),
	CLK_PLL("fout_ipll", "fin_pll", EXYNOS5422_IPLL_LOCK,
					EXYNOS5422_IPLL_CON0),
	CLK_PLL("fout_spll", "fin_pll", EXYNOS5422_SPLL_LOCK,
					EXYNOS5422_SPLL_CON0),
	CLK_PLL("fout_vpll", "fin_pll", EXYNOS5422_VPLL_LOCK,
					EXYNOS5422_VPLL_CON0),
	CLK_PLL("fout_mpll", "fin_pll", EXYNOS5422_MPLL_LOCK,
					EXYNOS5422_MPLL_CON0),
	CLK_PLL("fout_bpll", "fin_pll", EXYNOS5422_BPLL_LOCK,
					EXYNOS5422_BPLL_CON0),
	CLK_PLL("fout_kpll", "fin_pll", EXYNOS5422_KPLL_LOCK,
					EXYNOS5422_KPLL_CON0),

	CLK_MUXA("sclk_cpll", "mout_cpll", EXYNOS5422_SRC_TOP6, __BIT(28),
	    mout_cpll_p),
	CLK_MUXA("sclk_dpll", "mout_dpll", EXYNOS5422_SRC_TOP6, __BIT(24),
	    mout_dpll_p),
	CLK_MUXA("sclk_mpll", "mout_mpll", EXYNOS5422_SRC_TOP6, __BIT(0),
	    mout_mpll_p),
	CLK_MUXA("sclk_spll", "mout_spll", EXYNOS5422_SRC_TOP6, __BIT(8),
	    mout_spll_p),
	CLK_MUXA("sclk_ipll", "mout_ipll", EXYNOS5422_SRC_TOP6, __BIT(12),
	    mout_ipll_p),
	CLK_MUXF("sclk_epll", "mout_epll", EXYNOS5422_SRC_TOP6, __BIT(20),
	    CLK_SET_RATE_PARENT, mout_epll_p),
	CLK_MUXF("sclk_rpll", "mout_rpll", EXYNOS5422_SRC_TOP6, __BIT(16),
	    CLK_SET_RATE_PARENT, mout_rpll_p),

	CLK_MUX("mout_mmc0", EXYNOS5422_SRC_FSYS, __BITS(10,8),
	    mout_group2_p),
	CLK_MUX("mout_mmc1", EXYNOS5422_SRC_FSYS, __BITS(14,12),
	    mout_group2_p),
	CLK_MUX("mout_mmc2", EXYNOS5422_SRC_FSYS, __BITS(18,16),
	    mout_group2_p),
	CLK_MUX("mout_uart0", EXYNOS5422_SRC_PERIC0, __BITS(6,4),
	    mout_group2_p),
	CLK_MUX("mout_uart1", EXYNOS5422_SRC_PERIC0, __BITS(10,8),
	    mout_group2_p),
	CLK_MUX("mout_uart2", EXYNOS5422_SRC_PERIC0, __BITS(14,12),
	    mout_group2_p),
	CLK_MUX("mout_uart3", EXYNOS5422_SRC_PERIC0, __BITS(18,16),
	    mout_group2_p),

	CLK_DIV("dout_mmc0", "mout_mmc0", EXYNOS5422_DIV_FSYS1, __BITS(9,0)),
	CLK_DIV("dout_mmc1", "mout_mmc1", EXYNOS5422_DIV_FSYS1, __BITS(19,10)),
	CLK_DIV("dout_mmc2", "mout_mmc2", EXYNOS5422_DIV_FSYS1, __BITS(29,20)),
	CLK_DIV("dout_uart0", "mout_uart0", EXYNOS5422_DIV_PERIC0,
	    __BITS(11,8)),
	CLK_DIV("dout_uart1", "mout_uart1", EXYNOS5422_DIV_PERIC0,
	    __BITS(15,12)),
	CLK_DIV("dout_uart2", "mout_uart2", EXYNOS5422_DIV_PERIC0,
	    __BITS(19,16)),
	CLK_DIV("dout_uart3", "mout_uart3", EXYNOS5422_DIV_PERIC0,
	    __BITS(23,20)),

	CLK_GATE("sclk_mmc0", "dout_mmc0", EXYNOS5422_GATE_TOP_SCLK_FSYS,
	    __BIT(0), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_mmc1", "dout_mmc1", EXYNOS5422_GATE_TOP_SCLK_FSYS,
	    __BIT(1), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_mmc2", "dout_mmc2", EXYNOS5422_GATE_TOP_SCLK_FSYS,
	    __BIT(2), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart0", "dout_uart0", EXYNOS5422_GATE_TOP_SCLK_PERIC,
	    __BIT(0), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart1", "dout_uart1", EXYNOS5422_GATE_TOP_SCLK_PERIC,
	    __BIT(1), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart2", "dout_uart2", EXYNOS5422_GATE_TOP_SCLK_PERIC,
	    __BIT(2), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart3", "dout_uart3", EXYNOS5422_GATE_TOP_SCLK_PERIC,
	    __BIT(3), CLK_SET_RATE_PARENT),
};

static int	exynos5422_clock_match(device_t, cfdata_t, void *);
static void	exynos5422_clock_attach(device_t, device_t, void *);

struct exynos5422_clock_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
};

static void	exynos5422_clock_print_header(void);
static void	exynos5422_clock_print(struct exynos5422_clock_softc *,
		    struct exynos_clk *);

CFATTACH_DECL_NEW(exynos5422_clock, sizeof(struct exynos5422_clock_softc),
	exynos5422_clock_match, exynos5422_clock_attach, NULL, NULL);

#define CLOCK_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CLOCK_WRITE(sc, reg, val)	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
exynos5422_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5800-clock", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos5422_clock_attach(device_t parent, device_t self, void *aux)
{
	struct exynos5422_clock_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Exynos5422 Clock Controller\n");

	sc->sc_clkdom.funcs = &exynos5422_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (u_int n = 0; n < __arraycount(exynos5422_clocks); n++) {
		exynos5422_clocks[n].base.domain = &sc->sc_clkdom;
	}

	fdtbus_register_clock_controller(self, faa->faa_phandle,
	    &exynos5422_car_fdtclock_funcs);

	exynos5422_clock_print_header();
	for (u_int n = 0; n < __arraycount(exynos5422_clocks); n++) {
		exynos5422_clock_print(sc, &exynos5422_clocks[n]);
	}
}

static struct exynos_clk *
exynos5422_clock_find(const char *name)
{
	u_int n;

	for (n = 0; n < __arraycount(exynos5422_clocks); n++) {
		if (strcmp(exynos5422_clocks[n].base.name, name) == 0) {
			return &exynos5422_clocks[n];
		}
	}

	return NULL;
}

static struct exynos_clk *
exynos5422_clock_find_by_id(u_int clock_id)
{
	u_int n;

	for (n = 0; n < __arraycount(exynos5422_clock_ids); n++) {
		if (exynos5422_clock_ids[n].id == clock_id) {
			const char *name = exynos5422_clock_ids[n].name;
			return exynos5422_clock_find(name);
		}
	}

	return NULL;
}

static void
exynos5422_clock_print_header(void)
{
	printf("  %-10s %2s %-10s %-5s %10s\n",
	    "clock", "", "parent", "type", "rate");
	printf("  %-10s %2s %-10s %-5s %10s\n",
	    "=====", "", "======", "====", "====");
}

static void
exynos5422_clock_print(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_clk *eclk_parent;
	struct clk *clk_parent;
	const char *type = "?";

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		type = "fixed";
		break;
	case EXYNOS_CLK_PLL:
		type = "pll";
		break;
	case EXYNOS_CLK_MUX:
		type = "mux";
		break;
	case EXYNOS_CLK_DIV:
		type = "div";
		break;
	case EXYNOS_CLK_GATE:
		type = "gate";
		break;
	}

	clk_parent = exynos5422_clock_get_parent(sc, &eclk->base);
	eclk_parent = (struct exynos_clk *)clk_parent;

	printf("  %-10s %2s %-10s %-5s %10d Hz\n",
	    eclk->base.name,
	    eclk_parent ? "<-" : "",
	    eclk_parent ? eclk_parent->base.name : "",
	    type, clk_get_rate(&eclk->base));
}

static struct clk *
exynos5422_clock_decode(device_t dev, const void *data, size_t len)
{
	struct exynos_clk *eclk;

	/* #clock-cells should be 1 */
	if (len != 4) {
		return NULL;
	}

	const u_int clock_id = be32dec(data);

	eclk = exynos5422_clock_find_by_id(clock_id);
	if (eclk)
		return &eclk->base;

	return NULL;
}

static u_int
exynos5422_clock_get_rate_pll(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_pll_clk *epll = &eclk->u.pll;
	struct exynos_clk *clk_parent;

	KASSERT(eclk->type == EXYNOS_CLK_PLL);

	clk_parent = exynos5422_clock_find(eclk->parent);
	KASSERT(clk_parent != NULL);
	const u_int rate_parent = exynos5422_clock_get_rate(sc,
	    &clk_parent->base);

	const uint32_t v = CLOCK_READ(sc, epll->con0_reg);
	
	return PLL_FREQ(rate_parent, v);
}

static int
exynos5422_clock_set_rate_pll(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk, u_int rate)
{
	/* TODO */
	return EOPNOTSUPP;
}

static int
exynos5422_clock_set_parent_mux(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk, struct exynos_clk *eclk_parent)
{
	struct exynos_mux_clk *emux = &eclk->u.mux;
	const char *pname = eclk_parent->base.name;
	u_int sel;

	KASSERT(eclk->type == EXYNOS_CLK_MUX);

	for (sel = 0; sel < emux->nparents; sel++) {
		if (strcmp(pname, emux->parents[sel]) == 0) {
			break;
		}
	}
	if (sel == emux->nparents) {
		return EINVAL;
	}

	uint32_t v = CLOCK_READ(sc, emux->reg);
	v &= ~emux->bits;
	v |= __SHIFTIN(sel, emux->bits);
	CLOCK_WRITE(sc, emux->reg, v);

	return 0;
}

static struct exynos_clk *
exynos5422_clock_get_parent_mux(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_mux_clk *emux = &eclk->u.mux;

	KASSERT(eclk->type == EXYNOS_CLK_MUX);

	const uint32_t v = CLOCK_READ(sc, emux->reg);
	const u_int sel = __SHIFTOUT(v, emux->bits);

	KASSERT(sel < emux->nparents);

	return exynos5422_clock_find(emux->parents[sel]);
}

static u_int
exynos5422_clock_get_rate_div(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_div_clk *ediv = &eclk->u.div;
	struct clk *clk_parent;

	KASSERT(eclk->type == EXYNOS_CLK_DIV);

	clk_parent = exynos5422_clock_get_parent(sc, &eclk->base);
	const u_int parent_rate = exynos5422_clock_get_rate(sc, clk_parent);

	const uint32_t v = CLOCK_READ(sc, ediv->reg);
	const u_int div = __SHIFTOUT(v, ediv->bits);

	return parent_rate / (div + 1);
}

static int
exynos5422_clock_set_rate_div(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk, u_int rate)
{
	struct exynos_div_clk *ediv = &eclk->u.div;
	struct clk *clk_parent;
	int tmp_div, new_div = -1;
	u_int tmp_rate;

	KASSERT(eclk->type == EXYNOS_CLK_DIV);

	clk_parent = exynos5422_clock_get_parent(sc, &eclk->base);
	const u_int parent_rate = exynos5422_clock_get_rate(sc, clk_parent);

	for (tmp_div = 0; tmp_div < popcount32(ediv->bits); tmp_div++) {
		tmp_rate = parent_rate / (tmp_div + 1);
		if (tmp_rate <= rate) {
			new_div = tmp_div;
			break;
		}
	}
	if (new_div == -1)
		return EINVAL;

	uint32_t v = CLOCK_READ(sc, ediv->reg);
	v &= ~ediv->bits;
	v |= __SHIFTIN(new_div, ediv->bits);
	CLOCK_WRITE(sc, ediv->reg, v);

	return 0;
}

static int
exynos5422_clock_enable_gate(struct exynos5422_clock_softc *sc,
    struct exynos_clk *eclk, bool enable)
{
	struct exynos_gate_clk *egate = &eclk->u.gate;

	KASSERT(eclk->type == EXYNOS_CLK_GATE);

	uint32_t v = CLOCK_READ(sc, egate->reg);
	if (enable) {
		v |= egate->bits;
	} else {
		v &= ~egate->bits;
	}
	CLOCK_WRITE(sc, egate->reg, v);

	return 0;
}

/*
 * clk api
 */

static struct clk *
exynos5422_clock_get(void *priv, const char *name)
{
	struct exynos_clk *eclk;

	eclk = exynos5422_clock_find(name);
	if (eclk == NULL)
		return NULL;

	atomic_inc_uint(&eclk->refcnt);

	return &eclk->base;
}

static void
exynos5422_clock_put(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	KASSERT(eclk->refcnt > 0);

	atomic_dec_uint(&eclk->refcnt);
}

static u_int
exynos5422_clock_get_rate(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;
	struct clk *clk_parent;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return eclk->u.fixed.rate;
	case EXYNOS_CLK_PLL:
		return exynos5422_clock_get_rate_pll(priv, eclk);
	case EXYNOS_CLK_MUX:
	case EXYNOS_CLK_GATE:
		clk_parent = exynos5422_clock_get_parent(priv, clk);
		return exynos5422_clock_get_rate(priv, clk_parent);
	case EXYNOS_CLK_DIV:
		return exynos5422_clock_get_rate_div(priv, eclk);
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5422_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	KASSERT((clk->flags & CLK_SET_RATE_PARENT) == 0);

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return EIO;
	case EXYNOS_CLK_PLL:
		return exynos5422_clock_set_rate_pll(priv, eclk, rate);
	case EXYNOS_CLK_MUX:
		return EIO;
	case EXYNOS_CLK_DIV:
		return exynos5422_clock_set_rate_div(priv, eclk, rate);
	case EXYNOS_CLK_GATE:
		return EINVAL;
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5422_clock_enable(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return 0;	/* always on */
	case EXYNOS_CLK_PLL:
		return 0;	/* XXX */
	case EXYNOS_CLK_MUX:
	case EXYNOS_CLK_DIV:
		return 0;
	case EXYNOS_CLK_GATE:
		return exynos5422_clock_enable_gate(priv, eclk, true);
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5422_clock_disable(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return EINVAL;	/* always on */
	case EXYNOS_CLK_PLL:
		return EINVAL;	/* XXX */
	case EXYNOS_CLK_MUX:
	case EXYNOS_CLK_DIV:
		return EINVAL;
	case EXYNOS_CLK_GATE:
		return exynos5422_clock_enable_gate(priv, eclk, false);
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5422_clock_set_parent(void *priv, struct clk *clk, struct clk *clk_parent)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;
	struct exynos_clk *eclk_parent = (struct exynos_clk *)clk_parent;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
	case EXYNOS_CLK_PLL:
	case EXYNOS_CLK_DIV:
	case EXYNOS_CLK_GATE:
		return EINVAL;
	case EXYNOS_CLK_MUX:
		return exynos5422_clock_set_parent_mux(priv, eclk, eclk_parent);
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}
}

static struct clk *
exynos5422_clock_get_parent(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;
	struct exynos_clk *eclk_parent = NULL;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
	case EXYNOS_CLK_PLL:
	case EXYNOS_CLK_DIV:
	case EXYNOS_CLK_GATE:
		if (eclk->parent != NULL) {
			eclk_parent = exynos5422_clock_find(eclk->parent);
		}
		break;
	case EXYNOS_CLK_MUX:
		eclk_parent = exynos5422_clock_get_parent_mux(priv, eclk);
		break;
	default:
		panic("exynos5422: unknown eclk type %d", eclk->type);
	}

	return (struct clk *)eclk_parent;
}
