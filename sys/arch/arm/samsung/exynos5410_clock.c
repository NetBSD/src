/* $NetBSD: exynos5410_clock.c,v 1.2.8.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: exynos5410_clock.c,v 1.2.8.2 2017/12/03 11:35:56 jdolecek Exp $");

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

static struct clk *exynos5410_clock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func exynos5410_car_fdtclock_funcs = {
	.decode = exynos5410_clock_decode
};

/* DT clock ID to clock name mappings */
static struct exynos5410_clock_id {
	u_int		id;
	const char	*name;
} exynos5410_clock_ids[] = {
    /* core clocks */
    { 1, "fin_pll" },
    { 2, "fout_apll" },
    { 3, "fout_cpll" },
    { 4, "fout_dpll" },
    { 5, "fout_mpll" },
    { 6, "fout_kpll" },
    { 7, "fout_epll" },

    /* gate for special clocks (sclk) */
    { 128, "sclk_uart0" },
    { 129, "sclk_uart1" },
    { 130, "sclk_uart2" },
    { 131, "sclk_uart3" },
    { 132, "sclk_mmc0" },
    { 133, "sclk_mmc1" },
    { 134, "sclk_mmc2" },
    { 150, "sclk_usbd300" },
    { 151, "sclk_usbd301" },
    { 152, "sclk_usbphy300" },
    { 153, "sclk_usbphy301" },
    { 155, "sclk_pwm" },

    /* gate clocks */
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
    { 279, "pwm" },
    { 315, "mct" },
    { 316, "wdt" },
    { 317, "rtc" },
    { 318, "tmu" },
    { 351, "mmc0" },
    { 352, "mmc1" },
    { 353, "mmc2" },
    { 362, "pdma0" },
    { 363, "pdma1" },
    { 365, "usbh20" },
    { 366, "usbd300" },
    { 367, "usbd301" },
    { 471, "sss" },
};

static struct clk *exynos5410_clock_get(void *, const char *);
static void	exynos5410_clock_put(void *, struct clk *);
static u_int	exynos5410_clock_get_rate(void *, struct clk *);
static int	exynos5410_clock_set_rate(void *, struct clk *, u_int);
static int	exynos5410_clock_enable(void *, struct clk *);
static int	exynos5410_clock_disable(void *, struct clk *);
static int	exynos5410_clock_set_parent(void *, struct clk *, struct clk *);
static struct clk *exynos5410_clock_get_parent(void *, struct clk *);

static const struct clk_funcs exynos5410_clock_funcs = {
	.get = exynos5410_clock_get,
	.put = exynos5410_clock_put,
	.get_rate = exynos5410_clock_get_rate,
	.set_rate = exynos5410_clock_set_rate,
	.enable = exynos5410_clock_enable,
	.disable = exynos5410_clock_disable,
	.set_parent = exynos5410_clock_set_parent,
	.get_parent = exynos5410_clock_get_parent,
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

#define CLK_DIVF(_name, _parent, _reg, _bits, _f) {		\
	.base = { .name = (_name), .flags = (_f) },		\
	.type = EXYNOS_CLK_DIV,					\
	.parent = (_parent),					\
	.u = {							\
		.div = {					\
			.reg = (_reg),				\
			.bits = (_bits)				\
		}						\
	}							\
}

#define CLK_DIV(_name, _parent, _reg, _bits)			\
	CLK_DIVF(_name, _parent, _reg, _bits, 0)

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

#define EXYNOS5410_APLL_LOCK		0x00000
#define EXYNOS5410_APLL_CON0		0x00100
#define EXYNOS5410_MPLL_LOCK		0x04000
#define EXYNOS5410_MPLL_CON0		0x04100
#define EXYNOS5410_CPLL_LOCK		0x10020
#define EXYNOS5410_EPLL_LOCK		0x10040
#define EXYNOS5410_CPLL_CON0		0x10120
#define EXYNOS5410_EPLL_CON0		0x10130
#define EXYNOS5410_EPLL_CON1		0x10134
#define EXYNOS5410_EPLL_CON2		0x10138
#define EXYNOS5410_BPLL_LOCK		0x20010
#define EXYNOS5410_BPLL_CON0		0x20110
#define EXYNOS5410_KPLL_LOCK		0x28000
#define EXYNOS5410_KPLL_CON0		0x28100

#define EXYNOS5410_SRC_CPU		0x00200
#define EXYNOS5410_SRC_CPERI1		0x04204
#define EXYNOS5410_SRC_TOP0		0x10210
#define EXYNOS5410_SRC_TOP1		0x10214
#define EXYNOS5410_SRC_TOP2		0x10218
#define EXYNOS5410_SRC_FSYS		0x10244
#define EXYNOS5410_SRC_PERIC0		0x10250
#define EXYNOS5410_SRC_MASK_FSYS	0x10340
#define EXYNOS5410_SRC_MASK_PERIC0	0x10350
#define EXYNOS5410_SRC_CDREX		0x20200
#define EXYNOS5410_SRC_KFC		0x28200

#define EXYNOS5410_DIV_CPU0		0x00500
#define EXYNOS5410_DIV_TOP0		0x10510
#define EXYNOS5410_DIV_TOP1		0x10514
#define EXYNOS5410_DIV_FSYS0		0x10548
#define EXYNOS5410_DIV_FSYS1		0x1054c
#define EXYNOS5410_DIV_FSYS2		0x10550
#define EXYNOS5410_DIV_PERIC0		0x10558
#define EXYNOS5410_DIV_PERIC3		0x10564
#define EXYNOS5410_DIV_KFC0		0x28500

#define EXYNOS5410_GATE_IP_G2D		0x08800
#define EXYNOS5410_GATE_BUS_FSYS0	0x10740
#define EXYNOS5410_GATE_TOP_SCLK_FSYS	0x10840
#define EXYNOS5410_GATE_TOP_SCLK_PERIC	0x10850
#define EXYNOS5410_GATE_IP_FSYS		0x10944
#define EXYNOS5410_GATE_IP_PERIC	0x10950
#define EXYNOS5410_GATE_IP_PERIS	0x10960

static const char *mout_apll_p[] = { "fin_pll", "fout_apll" };
static const char *mout_bpll_p[] = { "fin_pll", "fout_bpll" };
static const char *mout_cpll_p[] = { "fin_pll", "fout_cpll" };
static const char *mout_epll_p[] = { "fin_pll", "fout_epll" };
static const char *mout_mpll_p[] = { "fin_pll", "fout_mpll" };
static const char *mout_kpll_p[] = { "fin_pll", "fout_kpll" };

static const char *mout_cpu_p[] = { "mout_apll", "sclk_mpll" };
static const char *mout_kfc_p[] = { "mout_kpll", "sclk_mpll" };

static const char *mout_mpll_user_p[] = { "fin_pll", "sclk_mpll" };
static const char *mout_bpll_user_p[] = { "fin_pll", "sclk_bpll" };
static const char *mout_mpll_bpll_p[] =
	{ "sclk_mpll_muxed", "sclk_bpll_muxed" };
static const char *mout_sclk_mpll_bpll_p[] = { "sclk_mpll_bpll", "fin_pll" };

static const char *mout_group2_p[] =
	{ "fin_pll", "fin_pll", "none", "none", "none", "none",
	  "sclk_mpll_bpll", "none", "none", "sclk_cpll" };

static struct exynos_clk exynos5410_clocks[] = {
	CLK_FIXED("fin_pll", EXYNOS_F_IN_FREQ),

	CLK_PLL("fout_apll", "fin_pll", EXYNOS5410_APLL_LOCK,
					EXYNOS5410_APLL_CON0),
	CLK_PLL("fout_bpll", "fin_pll", EXYNOS5410_BPLL_LOCK,
					EXYNOS5410_BPLL_CON0),
	CLK_PLL("fout_cpll", "fin_pll", EXYNOS5410_CPLL_LOCK,
					EXYNOS5410_CPLL_CON0),
	CLK_PLL("fout_epll", "fin_pll", EXYNOS5410_EPLL_LOCK,
					EXYNOS5410_EPLL_CON0),
	CLK_PLL("fout_mpll", "fin_pll", EXYNOS5410_MPLL_LOCK,
					EXYNOS5410_MPLL_CON0),
	CLK_PLL("fout_kpll", "fin_pll", EXYNOS5410_KPLL_LOCK,
					EXYNOS5410_KPLL_CON0),

	CLK_MUX("mout_apll", EXYNOS5410_SRC_CPU, __BIT(0), mout_apll_p),
	CLK_MUX("mout_cpu", EXYNOS5410_SRC_CPU, __BIT(16), mout_cpu_p),
	CLK_MUX("mout_kpll", EXYNOS5410_SRC_KFC, __BIT(0), mout_kpll_p),
	CLK_MUX("mout_kfc", EXYNOS5410_SRC_KFC, __BIT(16), mout_kfc_p),

	CLK_MUX("sclk_mpll", EXYNOS5410_SRC_CPERI1, __BIT(8), mout_mpll_p),
	CLK_MUX("sclk_mpll_muxed", EXYNOS5410_SRC_TOP2, __BIT(20), mout_mpll_user_p),
	CLK_MUX("sclk_bpll", EXYNOS5410_SRC_CDREX, __BIT(0), mout_bpll_p),
	CLK_MUX("sclk_bpll_muxed", EXYNOS5410_SRC_TOP2, __BIT(24), mout_bpll_user_p),
	CLK_MUX("sclk_epll", EXYNOS5410_SRC_TOP2, __BIT(12), mout_epll_p),
	CLK_MUX("sclk_cpll", EXYNOS5410_SRC_TOP2, __BIT(8), mout_cpll_p),
	CLK_MUX("sclk_mpll_bpll", EXYNOS5410_SRC_TOP1, __BIT(20), mout_mpll_bpll_p),

	CLK_MUX("mout_mmc0", EXYNOS5410_SRC_FSYS, __BITS(3,0), mout_group2_p),
	CLK_MUX("mout_mmc1", EXYNOS5410_SRC_FSYS, __BITS(7,4), mout_group2_p),
	CLK_MUX("mout_mmc2", EXYNOS5410_SRC_FSYS, __BITS(11,8), mout_group2_p),
	CLK_MUX("mout_usbd300", EXYNOS5410_SRC_FSYS, __BIT(28), mout_sclk_mpll_bpll_p),
	CLK_MUX("mout_usbd301", EXYNOS5410_SRC_FSYS, __BIT(29), mout_sclk_mpll_bpll_p),
	CLK_MUX("mout_uart0", EXYNOS5410_SRC_PERIC0, __BITS(3,0), mout_group2_p),
	CLK_MUX("mout_uart1", EXYNOS5410_SRC_PERIC0, __BITS(7,4), mout_group2_p),
	CLK_MUX("mout_uart2", EXYNOS5410_SRC_PERIC0, __BITS(11,8), mout_group2_p),
	CLK_MUX("mout_uart3", EXYNOS5410_SRC_PERIC0, __BITS(15,12), mout_group2_p),
	CLK_MUX("mout_pwm", EXYNOS5410_SRC_PERIC0, __BITS(27,24), mout_group2_p),
	CLK_MUX("mout_aclk200", EXYNOS5410_SRC_TOP0, __BIT(12), mout_mpll_bpll_p),
	CLK_MUX("mout_aclk400", EXYNOS5410_SRC_TOP0, __BIT(20), mout_mpll_bpll_p),

	CLK_DIV("div_arm", "mout_cpu", EXYNOS5410_DIV_CPU0, __BITS(2,0)),
	CLK_DIV("div_arm2", "div_arm", EXYNOS5410_DIV_CPU0, __BITS(30,28)),

	CLK_DIV("div_acp", "div_arm2", EXYNOS5410_DIV_CPU0, __BITS(10,8)),
	CLK_DIV("div_cpud", "div_arm2", EXYNOS5410_DIV_CPU0, __BITS(6,4)),
	CLK_DIV("div_atb", "div_arm2", EXYNOS5410_DIV_CPU0, __BITS(18,16)),
	CLK_DIV("pclk_dbg", "div_arm2", EXYNOS5410_DIV_CPU0, __BITS(22,20)),

	CLK_DIV("div_kfc", "mout_kfc", EXYNOS5410_DIV_KFC0, __BITS(2,0)),
	CLK_DIV("div_aclk", "div_kfc", EXYNOS5410_DIV_KFC0, __BITS(6,4)),
	CLK_DIV("div_pclk", "div_kfc", EXYNOS5410_DIV_KFC0, __BITS(22,20)),

	CLK_DIV("aclk66_pre", "sclk_mpll_muxed", EXYNOS5410_DIV_TOP1, __BITS(26,24)),
	CLK_DIV("aclk66", "aclk66_pre", EXYNOS5410_DIV_TOP0, __BITS(2,0)),

	CLK_DIV("dout_usbphy300", "mout_usbd300", EXYNOS5410_DIV_FSYS0, __BITS(19,16)),
	CLK_DIV("dout_usbphy301", "mout_usbd301", EXYNOS5410_DIV_FSYS0, __BITS(23,20)),
	CLK_DIV("dout_usbd300", "mout_usbd300", EXYNOS5410_DIV_FSYS0, __BITS(27,24)),
	CLK_DIV("dout_usbd301", "mout_usbd301", EXYNOS5410_DIV_FSYS0, __BITS(31,28)),

	CLK_DIV("dout_mmc0", "mout_mmc0", EXYNOS5410_DIV_FSYS1, __BITS(3,0)),
	CLK_DIV("dout_mmc1", "mout_mmc1", EXYNOS5410_DIV_FSYS1, __BITS(19,16)),
	CLK_DIV("dout_mmc2", "mout_mmc2", EXYNOS5410_DIV_FSYS2, __BITS(3,0)),

	CLK_DIVF("dout_mmc_pre0", "dout_mmc0", EXYNOS5410_DIV_FSYS1, __BITS(15,8),
	    CLK_SET_RATE_PARENT),
	CLK_DIVF("dout_mmc_pre1", "dout_mmc1", EXYNOS5410_DIV_FSYS1, __BITS(31,24),
	    CLK_SET_RATE_PARENT),
	CLK_DIVF("dout_mmc_pre2", "dout_mmc2", EXYNOS5410_DIV_FSYS2, __BITS(15,8),
	    CLK_SET_RATE_PARENT),

	CLK_DIV("div_uart0", "mout_uart0", EXYNOS5410_DIV_PERIC0, __BITS(3,0)),
	CLK_DIV("div_uart1", "mout_uart1", EXYNOS5410_DIV_PERIC0, __BITS(7,4)),
	CLK_DIV("div_uart2", "mout_uart2", EXYNOS5410_DIV_PERIC0, __BITS(11,8)),
	CLK_DIV("div_uart3", "mout_uart3", EXYNOS5410_DIV_PERIC0, __BITS(15,12)),

	CLK_DIV("dout_pwm", "mout_pwm", EXYNOS5410_DIV_PERIC3, __BITS(3,0)),

	CLK_DIV("aclk200", "mout_aclk200", EXYNOS5410_DIV_TOP0, __BITS(14,12)),
	CLK_DIV("aclk266", "sclk_mpll_muxed", EXYNOS5410_DIV_TOP0, __BITS(18,16)),
	CLK_DIV("aclk400", "mout_aclk400", EXYNOS5410_DIV_TOP0, __BITS(26,24)),

	CLK_GATE("sss", "aclk266", EXYNOS5410_GATE_IP_G2D, __BIT(2), 0),

	CLK_GATE("mct", "aclk66", EXYNOS5410_GATE_IP_PERIS, __BIT(18), 0),
	CLK_GATE("wdt", "aclk66", EXYNOS5410_GATE_IP_PERIS, __BIT(19), 0),
	CLK_GATE("rtc", "aclk66", EXYNOS5410_GATE_IP_PERIS, __BIT(20), 0),
	CLK_GATE("tmu", "aclk66", EXYNOS5410_GATE_IP_PERIS, __BIT(21), 0),

	CLK_GATE("sclk_mmc0", "dout_mmc_pre0", EXYNOS5410_SRC_MASK_FSYS,
	    __BIT(0), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_mmc1", "dout_mmc_pre1", EXYNOS5410_SRC_MASK_FSYS,
	    __BIT(4), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_mmc2", "dout_mmc_pre2", EXYNOS5410_SRC_MASK_FSYS,
	    __BIT(8), CLK_SET_RATE_PARENT),

	CLK_GATE("mmc0", "aclk200", EXYNOS5410_GATE_BUS_FSYS0, __BIT(12), 0),
	CLK_GATE("mmc1", "aclk200", EXYNOS5410_GATE_BUS_FSYS0, __BIT(13), 0),
	CLK_GATE("mmc2", "aclk200", EXYNOS5410_GATE_BUS_FSYS0, __BIT(14), 0),
	CLK_GATE("pdma1", "aclk200", EXYNOS5410_GATE_BUS_FSYS0, __BIT(2), 0),
	CLK_GATE("pdma0", "aclk200", EXYNOS5410_GATE_BUS_FSYS0, __BIT(1), 0),

	CLK_GATE("sclk_usbphy301", "dout_usbphy301", EXYNOS5410_GATE_TOP_SCLK_FSYS,
	    __BIT(7), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_usbphy300", "dout_usbphy300", EXYNOS5410_GATE_TOP_SCLK_FSYS,
	    __BIT(8), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_usbd301", "dout_usbd301", EXYNOS5410_GATE_TOP_SCLK_FSYS,
	    __BIT(9), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_usbd301", "dout_usbd301", EXYNOS5410_GATE_TOP_SCLK_FSYS,
	    __BIT(10), CLK_SET_RATE_PARENT),

	CLK_GATE("sclk_pwm", "dout_pwm", EXYNOS5410_GATE_TOP_SCLK_PERIC,
	    __BIT(11), CLK_SET_RATE_PARENT),

	CLK_GATE("uart0", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(0), 0),
	CLK_GATE("uart1", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(1), 0),
	CLK_GATE("uart2", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(2), 0),
	CLK_GATE("uart3", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(3), 0),
	CLK_GATE("i2c0", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(6), 0),
	CLK_GATE("i2c1", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(7), 0),
	CLK_GATE("i2c2", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(8), 0),
	CLK_GATE("i2c3", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(9), 0),
	CLK_GATE("usi0", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(10), 0),
	CLK_GATE("usi1", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(11), 0),
	CLK_GATE("usi2", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(12), 0),
	CLK_GATE("usi3", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(13), 0),
	CLK_GATE("pwm", "aclk66", EXYNOS5410_GATE_IP_PERIC, __BIT(24), 0),

	CLK_GATE("sclk_uart0", "div_uart0", EXYNOS5410_SRC_MASK_PERIC0,
	    __BIT(0), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart1", "div_uart1", EXYNOS5410_SRC_MASK_PERIC0,
	    __BIT(4), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart2", "div_uart2", EXYNOS5410_SRC_MASK_PERIC0,
	    __BIT(8), CLK_SET_RATE_PARENT),
	CLK_GATE("sclk_uart3", "div_uart3", EXYNOS5410_SRC_MASK_PERIC0,
	    __BIT(12), CLK_SET_RATE_PARENT),

	CLK_GATE("usbh20", "aclk200", EXYNOS5410_GATE_IP_FSYS, __BIT(18), 0),
	CLK_GATE("usbd301", "aclk200", EXYNOS5410_GATE_IP_FSYS, __BIT(19), 0),
	CLK_GATE("usbd300", "aclk200", EXYNOS5410_GATE_IP_FSYS, __BIT(20), 0),
};

static int	exynos5410_clock_match(device_t, cfdata_t, void *);
static void	exynos5410_clock_attach(device_t, device_t, void *);

struct exynos5410_clock_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;
};

static void	exynos5410_clock_print_header(void);
static void	exynos5410_clock_print(struct exynos5410_clock_softc *,
		    struct exynos_clk *);

CFATTACH_DECL_NEW(exynos5410_clock, sizeof(struct exynos5410_clock_softc),
	exynos5410_clock_match, exynos5410_clock_attach, NULL, NULL);

#define CLOCK_READ(sc, reg)		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CLOCK_WRITE(sc, reg, val)	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
exynos5410_clock_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5410-clock", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos5410_clock_attach(device_t parent, device_t self, void *aux)
{
	struct exynos5410_clock_softc * const sc = device_private(self);
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
	aprint_normal(": Exynos5410 Clock Controller\n");

	sc->sc_clkdom.funcs = &exynos5410_clock_funcs;
	sc->sc_clkdom.priv = sc;
	for (u_int n = 0; n < __arraycount(exynos5410_clocks); n++) {
		exynos5410_clocks[n].base.domain = &sc->sc_clkdom;
	}

	fdtbus_register_clock_controller(self, faa->faa_phandle,
	    &exynos5410_car_fdtclock_funcs);

	exynos5410_clock_print_header();
	for (u_int n = 0; n < __arraycount(exynos5410_clocks); n++) {
		exynos5410_clock_print(sc, &exynos5410_clocks[n]);
	}
}

static struct exynos_clk *
exynos5410_clock_find(const char *name)
{
	u_int n;

	for (n = 0; n < __arraycount(exynos5410_clocks); n++) {
		if (strcmp(exynos5410_clocks[n].base.name, name) == 0) {
			return &exynos5410_clocks[n];
		}
	}

	return NULL;
}

static struct exynos_clk *
exynos5410_clock_find_by_id(u_int clock_id)
{
	u_int n;

	for (n = 0; n < __arraycount(exynos5410_clock_ids); n++) {
		if (exynos5410_clock_ids[n].id == clock_id) {
			const char *name = exynos5410_clock_ids[n].name;
			return exynos5410_clock_find(name);
		}
	}

	return NULL;
}

static void
exynos5410_clock_print_header(void)
{
	printf("  %-10s %2s %-10s %-5s %10s\n",
	    "clock", "", "parent", "type", "rate");
	printf("  %-10s %2s %-10s %-5s %10s\n",
	    "=====", "", "======", "====", "====");
}

static void
exynos5410_clock_print(struct exynos5410_clock_softc *sc,
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

	clk_parent = exynos5410_clock_get_parent(sc, &eclk->base);
	eclk_parent = (struct exynos_clk *)clk_parent;

	printf("  %-10s %2s %-10s %-5s %10d Hz\n",
	    eclk->base.name,
	    eclk_parent ? "<-" : "",
	    eclk_parent ? eclk_parent->base.name : "",
	    type, clk_get_rate(&eclk->base));
}

static struct clk *
exynos5410_clock_decode(device_t dev, const void *data, size_t len)
{
	struct exynos_clk *eclk;

	/* #clock-cells should be 1 */
	if (len != 4) {
		return NULL;
	}

	const u_int clock_id = be32dec(data);

	eclk = exynos5410_clock_find_by_id(clock_id);
	if (eclk)
		return &eclk->base;

	return NULL;
}

static u_int
exynos5410_clock_get_rate_pll(struct exynos5410_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_pll_clk *epll = &eclk->u.pll;
	struct exynos_clk *clk_parent;

	KASSERT(eclk->type == EXYNOS_CLK_PLL);

	clk_parent = exynos5410_clock_find(eclk->parent);
	KASSERT(clk_parent != NULL);
	const u_int rate_parent = exynos5410_clock_get_rate(sc,
	    &clk_parent->base);

	const uint32_t v = CLOCK_READ(sc, epll->con0_reg);
	
	return PLL_FREQ(rate_parent, v);
}

static int
exynos5410_clock_set_rate_pll(struct exynos5410_clock_softc *sc,
    struct exynos_clk *eclk, u_int rate)
{
	/* TODO */
	return EOPNOTSUPP;
}

static int
exynos5410_clock_set_parent_mux(struct exynos5410_clock_softc *sc,
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
exynos5410_clock_get_parent_mux(struct exynos5410_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_mux_clk *emux = &eclk->u.mux;

	KASSERT(eclk->type == EXYNOS_CLK_MUX);

	const uint32_t v = CLOCK_READ(sc, emux->reg);
	const u_int sel = __SHIFTOUT(v, emux->bits);

	KASSERT(sel < emux->nparents);

	return exynos5410_clock_find(emux->parents[sel]);
}

static u_int
exynos5410_clock_get_rate_div(struct exynos5410_clock_softc *sc,
    struct exynos_clk *eclk)
{
	struct exynos_div_clk *ediv = &eclk->u.div;
	struct clk *clk_parent;

	KASSERT(eclk->type == EXYNOS_CLK_DIV);

	clk_parent = exynos5410_clock_get_parent(sc, &eclk->base);
	const u_int parent_rate = exynos5410_clock_get_rate(sc, clk_parent);

	const uint32_t v = CLOCK_READ(sc, ediv->reg);
	const u_int div = __SHIFTOUT(v, ediv->bits);

	return parent_rate / (div + 1);
}

static int
exynos5410_clock_set_rate_div(struct exynos5410_clock_softc *sc,
    struct exynos_clk *eclk, u_int rate)
{
	struct exynos_div_clk *ediv = &eclk->u.div;
	struct clk *clk_parent;
	int tmp_div, new_div = -1;
	u_int tmp_rate;

	KASSERT(eclk->type == EXYNOS_CLK_DIV);

	clk_parent = exynos5410_clock_get_parent(sc, &eclk->base);
	const u_int parent_rate = exynos5410_clock_get_rate(sc, clk_parent);

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
exynos5410_clock_enable_gate(struct exynos5410_clock_softc *sc,
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
exynos5410_clock_get(void *priv, const char *name)
{
	struct exynos_clk *eclk;

	eclk = exynos5410_clock_find(name);
	if (eclk == NULL)
		return NULL;

	atomic_inc_uint(&eclk->refcnt);

	return &eclk->base;
}

static void
exynos5410_clock_put(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	KASSERT(eclk->refcnt > 0);

	atomic_dec_uint(&eclk->refcnt);
}

static u_int
exynos5410_clock_get_rate(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;
	struct clk *clk_parent;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return eclk->u.fixed.rate;
	case EXYNOS_CLK_PLL:
		return exynos5410_clock_get_rate_pll(priv, eclk);
	case EXYNOS_CLK_MUX:
	case EXYNOS_CLK_GATE:
		clk_parent = exynos5410_clock_get_parent(priv, clk);
		return exynos5410_clock_get_rate(priv, clk_parent);
	case EXYNOS_CLK_DIV:
		return exynos5410_clock_get_rate_div(priv, eclk);
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5410_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;

	KASSERT((clk->flags & CLK_SET_RATE_PARENT) == 0);

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
		return EIO;
	case EXYNOS_CLK_PLL:
		return exynos5410_clock_set_rate_pll(priv, eclk, rate);
	case EXYNOS_CLK_MUX:
		return EIO;
	case EXYNOS_CLK_DIV:
		return exynos5410_clock_set_rate_div(priv, eclk, rate);
	case EXYNOS_CLK_GATE:
		return EINVAL;
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5410_clock_enable(void *priv, struct clk *clk)
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
		return exynos5410_clock_enable_gate(priv, eclk, true);
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5410_clock_disable(void *priv, struct clk *clk)
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
		return exynos5410_clock_enable_gate(priv, eclk, false);
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}
}

static int
exynos5410_clock_set_parent(void *priv, struct clk *clk, struct clk *clk_parent)
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
		return exynos5410_clock_set_parent_mux(priv, eclk, eclk_parent);
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}
}

static struct clk *
exynos5410_clock_get_parent(void *priv, struct clk *clk)
{
	struct exynos_clk *eclk = (struct exynos_clk *)clk;
	struct exynos_clk *eclk_parent = NULL;

	switch (eclk->type) {
	case EXYNOS_CLK_FIXED:
	case EXYNOS_CLK_PLL:
	case EXYNOS_CLK_DIV:
	case EXYNOS_CLK_GATE:
		if (eclk->parent != NULL) {
			eclk_parent = exynos5410_clock_find(eclk->parent);
		}
		break;
	case EXYNOS_CLK_MUX:
		eclk_parent = exynos5410_clock_get_parent_mux(priv, eclk);
		break;
	default:
		panic("exynos5410: unknown eclk type %d", eclk->type);
	}

	return (struct clk *)eclk_parent;
}
