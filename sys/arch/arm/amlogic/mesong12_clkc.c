/* $NetBSD: mesong12_clkc.c,v 1.6 2021/02/04 22:55:36 joerg Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: mesong12_clkc.c,v 1.6 2021/02/04 22:55:36 joerg Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_clk.h>
#include <arm/amlogic/mesong12_clkc.h>

#define	CBUS_REG(x)		((x) << 2)

#define HHI_GP0_PLL_CNTL0	CBUS_REG(0x10)
#define HHI_GP0_PLL_CNTL1	CBUS_REG(0x11)
#define HHI_GP0_PLL_CNTL2	CBUS_REG(0x12)
#define HHI_GP0_PLL_CNTL3	CBUS_REG(0x13)
#define HHI_GP0_PLL_CNTL4	CBUS_REG(0x14)
#define HHI_GP0_PLL_CNTL5	CBUS_REG(0x15)
#define HHI_GP0_PLL_CNTL6	CBUS_REG(0x16)
#define HHI_GP1_PLL_CNTL0	CBUS_REG(0x18)
#define HHI_GP1_PLL_CNTL1	CBUS_REG(0x19)
#define HHI_PCIE_PLL_CNTL0	CBUS_REG(0x26)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_LOCK		__BIT(31)
#define  HHI_PCIE_PLL_CNTL0_PCIE_HCSL_CAL_DONE		__BIT(30)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_RESET		__BIT(29)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_EN		__BIT(28)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_VCO_DIV_SEL	__BIT(27)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_AFC_START		__BIT(26)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_OD		__BITS(20,16)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_PREDIV_SEL	__BITS(14,10)
#define  HHI_PCIE_PLL_CNTL0_PCIE_APLL_FBKDIV		__BITS(7,0)
#define HHI_PCIE_PLL_CNTL1	CBUS_REG(0x27)
#define  HHI_PCIE_PLL_CNTL1_PCIE_APLL_SDM_EN		__BIT(12)
#define  HHI_PCIE_PLL_CNTL1_PCIE_APLL_SDM_FRAC		__BITS(11,0)
#define HHI_PCIE_PLL_CNTL2	CBUS_REG(0x28)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_SSC_DEP_SEL	__BITS(31,28)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_SSC_FREF_SEL	__BITS(25,24)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_SSC_MODE		__BITS(23,22)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_SSC_OFFSET	__BITS(21,20)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_STR_M		__BITS(19,18)
#define  HHI_PCIE_PLL_CNTL2_PCIE_APLL_RESERVE		__BITS(15,0)
#define HHI_PCIE_PLL_CNTL3	CBUS_REG(0x29)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_BYPASS_EN	__BIT(31)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_HOLD_T	__BITS(29,28)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_IN		__BITS(26,20)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_NT		__BIT(19)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_DIV		__BITS(18,17)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_BIAS_LPF_EN	__BIT(16)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_CP_ICAP		__BITS(15,12)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_CP_IRES		__BITS(11,8)
#define  HHI_PCIE_PLL_CNTL3_PCIE_APLL_CPI		__BITS(5,4)
#define HHI_PCIE_PLL_CNTL4	CBUS_REG(0x2a)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_SHIFT_EN		__BIT(31)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_SHIFT_T		__BITS(27,26)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_SHIFT_V		__BITS(25,24)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_VCTRL_MON_EN	__BIT(23)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LPF_CAP		__BITS(21,20)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LPF_CAPADJ	__BITS(19,16)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LPF_RES		__BITS(13,12)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LPF_SF		__BIT(11)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LVR_OD_EN		__BIT(10)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_REFCLK_MON_EN	__BIT(9)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_FBKCLK_MON_EN	__BIT(8)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LOAD		__BIT(7)
#define  HHI_PCIE_PLL_CNTL4_PCIE_APLL_LOAD_EN		__BIT(6)
#define HHI_PCIE_PLL_CNTL5	CBUS_REG(0x2b)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_ADJ_LDO		__BITS(30,28)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BGP_EN		__BIT(27)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BGR_ADJ		__BITS(24,20)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BGR_START		__BIT(19)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BGR_VREF		__BITS(16,12)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BY_IMP_IN		__BITS(11,8)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BY_IMP		__BIT(7)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_CAL_EN		__BIT(6)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_CAL_RSTN		__BIT(5)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_EDGEDRV_EN	__BIT(4)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_EN0		__BIT(3)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_IN_EN		__BIT(2)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_SEL_PW		__BIT(1)
#define  HHI_PCIE_PLL_CNTL5_PCIE_HCSL_SEL_STR		__BIT(0)

#define HHI_HIFI_PLL_CNTL0	CBUS_REG(0x36)
#define HHI_HIFI_PLL_CNTL1	CBUS_REG(0x37)
#define HHI_HIFI_PLL_CNTL2	CBUS_REG(0x38)
#define HHI_HIFI_PLL_CNTL3	CBUS_REG(0x39)
#define HHI_HIFI_PLL_CNTL4	CBUS_REG(0x3a)
#define HHI_HIFI_PLL_CNTL5	CBUS_REG(0x3b)
#define HHI_HIFI_PLL_CNTL6	CBUS_REG(0x3c)
#define HHI_MEM_PD_REG0		CBUS_REG(0x40)
#define HHI_GCLK_MPEG0		CBUS_REG(0x50)
#define HHI_GCLK_MPEG1		CBUS_REG(0x51)
#define HHI_GCLK_MPEG2		CBUS_REG(0x52)
#define HHI_GCLK_OTHER		CBUS_REG(0x54)
#define HHI_GCLK_OTHER2		CBUS_REG(0x55)
#define HHI_SYS_CPU_CLK_CNTL1	CBUS_REG(0x57)
#define HHI_MPEG_CLK_CNTL	CBUS_REG(0x5d)
#define HHI_TS_CLK_CNTL		CBUS_REG(0x64)
#define HHI_SYS_CPU_CLK_CNTL0	CBUS_REG(0x67)
#define HHI_VID_PLL_CLK_DIV	CBUS_REG(0x68)
#define HHI_SYS_CPUB_CLK_CNTL1	CBUS_REG(0x80)
#define HHI_SYS_CPUB_CLK_CNTL	CBUS_REG(0x82)
#define HHI_NAND_CLK_CNTL	CBUS_REG(0x97)
#define HHI_SD_EMMC_CLK_CNTL	CBUS_REG(0x99)
#define HHI_MPLL_CNTL0		CBUS_REG(0x9e)
#define HHI_MPLL_CNTL1		CBUS_REG(0x9f)
#define HHI_MPLL_CNTL2		CBUS_REG(0xa0)
#define HHI_MPLL_CNTL3		CBUS_REG(0xa1)
#define HHI_MPLL_CNTL4		CBUS_REG(0xa2)
#define HHI_MPLL_CNTL5		CBUS_REG(0xa3)
#define HHI_MPLL_CNTL6		CBUS_REG(0xa4)
#define HHI_MPLL_CNTL7		CBUS_REG(0xa5)
#define HHI_MPLL_CNTL8		CBUS_REG(0xa6)
#define HHI_FIX_PLL_CNTL0	CBUS_REG(0xa8)
#define HHI_FIX_PLL_CNTL1	CBUS_REG(0xa9)
#define HHI_FIX_PLL_CNTL2	CBUS_REG(0xaa)
#define HHI_FIX_PLL_CNTL3	CBUS_REG(0xab)
#define HHI_SYS_PLL_CNTL0	CBUS_REG(0xbd)
#define HHI_SYS_PLL_CNTL1	CBUS_REG(0xbe)
#define HHI_SYS_PLL_CNTL2	CBUS_REG(0xbf)
#define HHI_SYS_PLL_CNTL3	CBUS_REG(0xc0)
#define HHI_SYS_PLL_CNTL4	CBUS_REG(0xc1)
#define HHI_SYS_PLL_CNTL5	CBUS_REG(0xc2)
#define HHI_SYS_PLL_CNTL6	CBUS_REG(0xc3)
#define HHI_HDMI_PLL_CNTL0	CBUS_REG(0xc8)
#define HHI_HDMI_PLL_CNTL1	CBUS_REG(0xc9)
#define HHI_SYS1_PLL_CNTL0	CBUS_REG(0xe0)
#define HHI_SYS1_PLL_CNTL1	CBUS_REG(0xe1)
#define HHI_SYS1_PLL_CNTL2	CBUS_REG(0xe2)
#define HHI_SYS1_PLL_CNTL3	CBUS_REG(0xe3)
#define HHI_SYS1_PLL_CNTL4	CBUS_REG(0xe4)
#define HHI_SYS1_PLL_CNTL5	CBUS_REG(0xe5)
#define HHI_SYS1_PLL_CNTL6	CBUS_REG(0xe6)

static int mesong12_clkc_match(device_t, cfdata_t, void *);
static void mesong12_clkc_attach(device_t, device_t, void *);

static u_int mesong12_cpuclk_get_rate(struct meson_clk_softc *,
    struct meson_clk_clk *);
static int mesong12_cpuclk_set_rate(struct meson_clk_softc *,
    struct meson_clk_clk *, u_int);
static int mesong12_clk_pcie_pll_set_rate(struct meson_clk_softc *,
    struct meson_clk_clk *, u_int);

struct mesong12_clkc_config {
	const char *name;
	struct meson_clk_clk *clks;
	int nclks;
};

#define PARENTS(args...)	((const char *[]){ args })

/* fixed pll */
#define G12_CLK_fixed_pll_dco						\
	MESON_CLK_PLL(MESONG12_CLOCK_FIXED_PLL_DCO, "fixed_pll_dco",	\
	    "xtal",						/* parent */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL0, __BIT(28)),	/* enable */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL0, __BITS(7,0)),	/* m */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL0, __BITS(14,10)),/* n */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL1, __BITS(16,0)),	/* frac */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL0, __BIT(31)),	/* l */ \
	    MESON_CLK_PLL_REG(HHI_FIX_PLL_CNTL0, __BIT(29)),	/* reset */ \
	    0)
#define G12_CLK_fixed_pll						\
	MESON_CLK_DIV(MESONG12_CLOCK_FIXED_PLL, "fixed_pll",		\
	    "fixed_pll_dco",		/* parent */			\
	    HHI_FIX_PLL_CNTL0,		/* reg */			\
	    __BITS(17,16),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO)

/* sys pll */
#define G12_CLK_sys_pll_dco						\
	MESON_CLK_PLL(MESONG12_CLOCK_SYS_PLL_DCO, "sys_pll_dco",	\
	    "xtal",						/* parent */ \
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL0, __BIT(28)),	/* enable */ \
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL0, __BITS(7,0)),	/* m */	\
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL0, __BITS(14,10)),/* n */	\
	    MESON_CLK_PLL_REG_INVALID,				/* frac */ \
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL0, __BIT(31)),	/* l */	\
	    MESON_CLK_PLL_REG(HHI_SYS_PLL_CNTL0, __BIT(29)),	/* reset */ \
	    0)
#define G12_CLK_sys_pll							\
	MESON_CLK_DIV(MESONG12_CLOCK_SYS_PLL, "sys_pll",		\
	    "sys_pll_dco",		/* parent */			\
	    HHI_SYS_PLL_CNTL0,		/* reg */			\
	    __BITS(18,16),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO | MESON_CLK_DIV_SET_RATE_PARENT)

/* sys1 pll */
#define G12B_CLK_sys1_pll_dco						\
	MESON_CLK_PLL(MESONG12_CLOCK_SYS1_PLL_DCO, "sys1_pll_dco",	\
	    "xtal",						/* parent */ \
	    MESON_CLK_PLL_REG(HHI_SYS1_PLL_CNTL0, __BIT(28)),	/* enable */ \
	    MESON_CLK_PLL_REG(HHI_SYS1_PLL_CNTL0, __BITS(7,0)),	/* m */ \
	    MESON_CLK_PLL_REG(HHI_SYS1_PLL_CNTL0, __BITS(14,10)),/* n */ \
	    MESON_CLK_PLL_REG_INVALID,				/* frac */ \
	    MESON_CLK_PLL_REG(HHI_SYS1_PLL_CNTL0, __BIT(31)),	/* l */ \
	    MESON_CLK_PLL_REG(HHI_SYS1_PLL_CNTL0, __BIT(29)),	/* reset */ \
	    0)
#define G12B_CLK_sys1_pll						\
	MESON_CLK_DIV(MESONG12_CLOCK_SYS1_PLL, "sys1_pll",		\
	    "sys1_pll_dco",		/* parent */			\
	    HHI_SYS1_PLL_CNTL0,		/* reg */			\
	    __BITS(18,16),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO | MESON_CLK_DIV_SET_RATE_PARENT)

/* fclk div */
#define G12_CLK_fclk_div2_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV2_DIV, "fclk_div2_div", \
	    "fixed_pll",		/* parent */			\
	    2,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div2						\
	MESON_CLK_GATE(MESONG12_CLOCK_FCLK_DIV2, "fclk_div2",		\
	    "fclk_div2_div",		/* parent */			\
	    HHI_FIX_PLL_CNTL1,		/* reg */			\
	    24)				/* bit */
#define G12_CLK_fclk_div2p5_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV2P5_DIV,		\
	    "fclk_div2p5_div",						\
	    "fixed_pll_dco",		/* parent */			\
	    5,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div3_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV3_DIV,		\
	    "fclk_div3_div",						\
	    "fixed_pll",		/* parent */			\
	    3,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div3						\
	MESON_CLK_GATE(MESONG12_CLOCK_FCLK_DIV3, "fclk_div3",		\
	    "fclk_div3_div",		/* parent */			\
	    HHI_FIX_PLL_CNTL1,		/* reg */			\
	    20)				/* bit */
#define G12_CLK_fclk_div4_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV4_DIV, "fclk_div4_div", \
	    "fixed_pll",		/* parent */			\
	    4,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div4						\
	MESON_CLK_GATE(MESONG12_CLOCK_FCLK_DIV4, "fclk_div4",		\
	    "fclk_div4_div",		/* parent */			\
	    HHI_FIX_PLL_CNTL1,		/* reg */			\
	    21)				/* bit */
#define G12_CLK_fclk_div5_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV5_DIV, "fclk_div5_div", \
	    "fixed_pll",		/* parent */			\
	    5,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div5						\
	MESON_CLK_GATE(MESONG12_CLOCK_FCLK_DIV5, "fclk_div5",		\
	    "fclk_div5_div",		/* parent */			\
	    HHI_FIX_PLL_CNTL1,		/* reg */			\
	    22)				/* bit */
#define G12_CLK_fclk_div7_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_FCLK_DIV7_DIV, "fclk_div7_div", \
	    "fixed_pll",		/* parent */			\
	    7,				/* div */			\
	    1)				/* mult */
#define G12_CLK_fclk_div7						\
	MESON_CLK_GATE(MESONG12_CLOCK_FCLK_DIV7, "fclk_div7",		\
	    "fclk_div7_div",		/* parent */			\
	    HHI_FIX_PLL_CNTL1,		/* reg */			\
	    23)				/* bit */

/* mpll */
#define G12_CLK_mpll_prediv						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_MPLL_PREDIV, "mpll_prediv", \
	    "fixed_pll_dco",		/* parent */			\
	    2,				/* div */			\
	    1)				/* mult */
#define G12_CLK_mpll0_div						\
	MESON_CLK_MPLL(MESONG12_CLOCK_MPLL0_DIV, "mpll0_div",		\
	    "mpll_prediv",					/* parent */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL1, __BITS(13,0)),	/* sdm */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL1, __BIT(30)), /* sdm_enable */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL1, __BITS(28,20)),	/* n2 */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL1, __BIT(29)),	/* ssen */ \
	    0)
#define G12_CLK_mpll1_div						\
	MESON_CLK_MPLL(MESONG12_CLOCK_MPLL1_DIV, "mpll1_div",		\
	    "mpll_prediv",					/* parent */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL3, __BITS(13,0)),	/* sdm */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL3, __BIT(30)), /* sdm_enable */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL3, __BITS(28,20)),	/* n2 */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL3, __BIT(29)),	/* ssen */ \
	    0)
#define G12_CLK_mpll2_div						\
	MESON_CLK_MPLL(MESONG12_CLOCK_MPLL2_DIV, "mpll2_div",		\
	    "mpll_prediv",					/* parent */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL5, __BITS(13,0)),	/* sdm */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL5, __BIT(30)), /* sdm_enable */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL5, __BITS(28,20)),	/* n2 */ \
	    MESON_CLK_PLL_REG(HHI_MPLL_CNTL5, __BIT(29)),	/* ssen */ \
	    0)
#define G12_CLK_mpll0							\
	MESON_CLK_GATE(MESONG12_CLOCK_MPLL0, "mpll0",			\
	    "mpll0_div",		/* parent */			\
	    HHI_MPLL_CNTL1,		/* reg */			\
	    31)				/* bit */
#define G12_CLK_mpll1							\
	MESON_CLK_GATE(MESONG12_CLOCK_MPLL1, "mpll1",			\
	    "mpll1_div",		/* parent */			\
	    HHI_MPLL_CNTL3,		/* reg */			\
	    31)				/* bit */
#define G12_CLK_mpll2							\
	MESON_CLK_GATE(MESONG12_CLOCK_MPLL2, "mpll2",			\
	    "mpll2_div",		/* parent */			\
	    HHI_MPLL_CNTL5,		/* reg */			\
	    31)				/* bit */
#define G12_CLK_mpll_50m_div						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_MPLL_50M_DIV, "mpll_50m_div", \
	    "fixed_pll_dco",		/* parent */			\
	    80,				/* div */			\
	    1)				/* mult */
#define G12_CLK_mpll_50m						\
	MESON_CLK_MUX(MESONG12_CLOCK_MPLL_50M, "mpll_50m",		\
	    PARENTS("mpll_50m_div", "xtal"),				\
	    HHI_FIX_PLL_CNTL3,		/* reg */			\
	    __BIT(5),			/* sel */			\
	    0)

/* sd/emmc */
#define G12_CLK_sd_emmc_a_clk0_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_SD_EMMC_A_CLK0_SEL, "sd_emmc_a_clk0_sel", \
	    PARENTS("xtal", "fclk_div2", "fclk_div3",			\
	    "fclk_div5", "fclk_div7"),					\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    __BITS(11,9),		/* sel */			\
	    0)
#define G12_CLK_sd_emmc_b_clk0_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_SD_EMMC_B_CLK0_SEL, "sd_emmc_b_clk0_sel", \
	    PARENTS("xtal", "fclk_div2", "fclk_div3",			\
	    "fclk_div5", "fclk_div7"),					\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    __BITS(27,25),		/* sel */			\
	    0)
#define G12_CLK_sd_emmc_c_clk0_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_SD_EMMC_C_CLK0_SEL, "sd_emmc_c_clk0_sel", \
	    PARENTS("xtal", "fclk_div2", "fclk_div3",			\
	    "fclk_div5", "fclk_div7"),					\
	    HHI_NAND_CLK_CNTL,		/* reg */			\
	    __BITS(11,9),		/* sel */			\
	    0)
#define G12_CLK_sd_emmc_a_clk0_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_SD_EMMC_A_CLK0_DIV, "sd_emmc_a_clk0_div", \
	    "sd_emmc_a_clk0_sel",	/* parent */			\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    __BITS(6,0),		/* div */			\
	    0)
#define G12_CLK_sd_emmc_b_clk0_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_SD_EMMC_B_CLK0_DIV, "sd_emmc_b_clk0_div", \
	    "sd_emmc_b_clk0_sel",	/* parent */			\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    __BITS(22,16),		/* div */			\
	    0)
#define G12_CLK_sd_emmc_c_clk0_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_SD_EMMC_C_CLK0_DIV, "sd_emmc_c_clk0_div", \
	    "sd_emmc_c_clk0_sel",	/* parent */			\
	    HHI_NAND_CLK_CNTL,		/* reg */			\
	    __BITS(6,0),		/* div */			\
	    0)
#define G12_CLK_sd_emmc_a_clk0						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_A_CLK0, "sd_emmc_a_clk0",	\
	    "sd_emmc_a_clk0_div",	/* parent */			\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    7)				/* bit */
#define G12_CLK_sd_emmc_b_clk0						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_B_CLK0, "sd_emmc_b_clk0", \
	    "sd_emmc_b_clk0_div",	/* parent */			\
	    HHI_SD_EMMC_CLK_CNTL,	/* reg */			\
	    23)				/* bit */
#define G12_CLK_sd_emmc_c_clk0						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_C_CLK0, "sd_emmc_c_clk0",	\
	    "sd_emmc_c_clk0_div",	/* parent */			\
	    HHI_NAND_CLK_CNTL,		/* reg */			\
	    7)				/* bit */

/* source as mpeg_clk */
#define G12_CLK_mpeg_sel						\
	MESON_CLK_MUX(MESONG12_CLOCK_MPEG_SEL, "mpeg_sel",		\
	    PARENTS("xtal", NULL, "fclk_div7", "mpll1",			\
	    "mpll2", "fclk_div4", "fclk_div3", "fclk_div5"),		\
	    HHI_MPEG_CLK_CNTL,		/* reg */			\
	    __BITS(14,12),		/* sel */			\
	    0)
#define G12_CLK_mpeg_clk_div						\
	MESON_CLK_DIV(MESONG12_CLOCK_MPEG_DIV, "mpeg_clk_div",		\
	    "mpeg_sel",			/* parent */			\
	    HHI_MPEG_CLK_CNTL,		/* reg */			\
	    __BITS(6,0),		/* div */			\
	    MESON_CLK_DIV_SET_RATE_PARENT)
#define G12_CLK_clk81							\
	MESON_CLK_GATE(MESONG12_CLOCK_CLK81, "clk81",			\
	    "mpeg_clk_div",		/* parent */			\
	    HHI_MPEG_CLK_CNTL,		/* reg */			\
	    7)				/* bit */
#define G12_CLK_ddr							\
	MESON_CLK_GATE(MESONG12_CLOCK_DDR, "ddr",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    0)				/* bit */
#define G12_CLK_dos							\
	MESON_CLK_GATE(MESONG12_CLOCK_DOS, "dos",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    1)				/* bit */
#define G12_CLK_audio_locker						\
	MESON_CLK_GATE(MESONG12_CLOCK_AUDIO_LOCKER, "audio_locker",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    2)				/* bit */
#define G12_CLK_mipi_dsi_host						\
	MESON_CLK_GATE(MESONG12_CLOCK_MIPI_DSI_HOST, "mipi_dsi_host",	\
	   "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    3)				/* bit */
#define G12_CLK_eth_phy							\
	MESON_CLK_GATE(MESONG12_CLOCK_ETH_PHY, "eth_phy",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    4)				/* bit */
#define G12_CLK_isa							\
	MESON_CLK_GATE(MESONG12_CLOCK_ISA, "isa",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    5)				/* bit */
#define G12_CLK_pl301							\
	MESON_CLK_GATE(MESONG12_CLOCK_PL301, "pl301",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    6)				/* bit */
#define G12_CLK_periphs							\
	MESON_CLK_GATE(MESONG12_CLOCK_PERIPHS, "periphs",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    7)				/* bit */
#define G12_CLK_spicc0							\
	MESON_CLK_GATE(MESONG12_CLOCK_SPICC0, "spicc0",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    8)				/* bit */
#define G12_CLK_i2c							\
	MESON_CLK_GATE(MESONG12_CLOCK_I2C, "i2c",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    9)				/* bit */
#define G12_CLK_sana							\
	MESON_CLK_GATE(MESONG12_CLOCK_SANA, "sana",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    10)				/* bit */
#define G12_CLK_sd							\
	MESON_CLK_GATE(MESONG12_CLOCK_SD, "sd",				\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    11)				/* bit */
#define G12_CLK_rng0							\
	MESON_CLK_GATE(MESONG12_CLOCK_RNG0, "rng0",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    12)				/* bit */
#define G12_CLK_uart0							\
	MESON_CLK_GATE(MESONG12_CLOCK_UART0, "uart0",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    13)				/* bit */
#define G12_CLK_spicc1							\
	MESON_CLK_GATE(MESONG12_CLOCK_SPICC1, "spicc1",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    14)				/* bit */
#define G12_CLK_hiu_iface						\
	MESON_CLK_GATE(MESONG12_CLOCK_HIU_IFACE, "hiu_iface",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    19)				/* bit */
#define G12_CLK_mipi_dsi_phy						\
	MESON_CLK_GATE(MESONG12_CLOCK_MIPI_DSI_PHY, "mipi_dsi_phy",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    20)				/* bit */
#define G12_CLK_assist_misc						\
	MESON_CLK_GATE(MESONG12_CLOCK_ASSIST_MISC, "assist_misc",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    23)				/* bit */
#define G12_CLK_sd_emmc_a						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_A, "sd_emmc_a",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    4)				/* bit */
#define G12_CLK_sd_emmc_b						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_B, "sd_emmc_b",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    25)				/* bit */
#define G12_CLK_sd_emmc_c						\
	MESON_CLK_GATE(MESONG12_CLOCK_SD_EMMC_C, "sd_emmc_c",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    26)				/* bit */
#define G12_CLK_audio_codec						\
	MESON_CLK_GATE(MESONG12_CLOCK_AUDIO_CODEC, "audio_codec",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG0,		/* reg */			\
	    28)				/* bit */
#define G12_CLK_audio							\
	MESON_CLK_GATE(MESONG12_CLOCK_AUDIO, "audio",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    0)				/* bit */
#define G12_CLK_eth							\
	MESON_CLK_GATE(MESONG12_CLOCK_ETH, "eth",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    3)				/* bit */
#define G12_CLK_demux							\
	MESON_CLK_GATE(MESONG12_CLOCK_DEMUX, "demux",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    4)				/* bit */
#define G12_CLK_audio_ififo						\
	MESON_CLK_GATE(MESONG12_CLOCK_AUDIO_IFIFO, "audio_ififo",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    11)				/* bit */
#define G12_CLK_adc							\
	MESON_CLK_GATE(MESONG12_CLOCK_ADC, "adc",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    13)				/* bit */
#define G12_CLK_uart1							\
	MESON_CLK_GATE(MESONG12_CLOCK_UART1, "uart1",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    16)				/* bit */
#define G12_CLK_g2d							\
	MESON_CLK_GATE(MESONG12_CLOCK_G2D, "g2d",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    20)				/* bit */
#define G12_CLK_reset							\
	MESON_CLK_GATE(MESONG12_CLOCK_RESET, "reset",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    23)				/* bit */
#define G12_CLK_pcie_comb						\
	MESON_CLK_GATE(MESONG12_CLOCK_PCIE_COMB, "pcie_comb",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    24)				/* bit */
#define G12_CLK_parser							\
	MESON_CLK_GATE(MESONG12_CLOCK_PARSER, "parser",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    25)				/* bit */
#define G12_CLK_usb							\
	MESON_CLK_GATE(MESONG12_CLOCK_USB, "usb",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    26)				/* bit */
#define G12_CLK_pcie_phy						\
	MESON_CLK_GATE(MESONG12_CLOCK_PCIE_PHY, "pcie_phy",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    27)				/* bit */
#define G12_CLK_ahb_arb0						\
	MESON_CLK_GATE(MESONG12_CLOCK_AHB_ARB0, "ahb_arb0",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG1,		/* reg */			\
	    29)				/* bit */
#define G12_CLK_ahb_data_bus						\
	MESON_CLK_GATE(MESONG12_CLOCK_AHB_DATA_BUS, "ahb_data_bus",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    1)				/* bit */
#define G12_CLK_ahb_ctrl_bus						\
	MESON_CLK_GATE(MESONG12_CLOCK_AHB_CTRL_BUS, "ahb_ctrl_bus",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    2)				/* bit */
#define G12_CLK_htx_hdcp22						\
	MESON_CLK_GATE(MESONG12_CLOCK_HTX_HDCP22, "htx_hdcp22",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    3)				/* bit */
#define G12_CLK_htx_pclk						\
	MESON_CLK_GATE(MESONG12_CLOCK_HTX_PCLK, "htx_pclk",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    4)				/* bit */
#define G12_CLK_bt656							\
	MESON_CLK_GATE(MESONG12_CLOCK_BT656, "bt656",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    6)				/* bit */
#define G12_CLK_usb1_ddr_bridge						\
	MESON_CLK_GATE(MESONG12_CLOCK_USB1_DDR_BRIDGE, "usb1_ddr_bridge", \
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    8)				/* bit */
#define G12_CLK_mmc_pclk						\
	MESON_CLK_GATE(MESONG12_CLOCK_MMC_PCLK, "mmc_pclk",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    11)				/* bit */
#define G12_CLK_uart2							\
	MESON_CLK_GATE(MESONG12_CLOCK_UART2, "uart2",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    15)				/* bit */
#define G12_CLK_vpu_intr						\
	MESON_CLK_GATE(MESONG12_CLOCK_VPU_INTR, "vpu_intr",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    25)				/* bit */
#define G12_CLK_gic							\
	MESON_CLK_GATE(MESONG12_CLOCK_GIC, "gic",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_MPEG2,		/* reg */			\
	    30)				/* bit */
#define G12_CLK_vclk2_venci0						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCI0, "vclk2_venci0",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    1)				/* bit */
#define G12_CLK_vclk2_venci1						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCI1, "vclk2_venci1",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    2)				/* bit */
#define G12_CLK_vclk2_vencp0						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCP0, "vclk2_vencp0",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    3)				/* bit */
#define G12_CLK_vclk2_vencp1						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCP1, "vclk2_vencp1",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    4)				/* bit */
#define G12_CLK_vclk2_venct0						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCT0, "vclk2_venct0",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    5)				/* bit */
#define G12_CLK_vclk2_venct1						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCT1, "vclk2_venct1",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    6)				/* bit */
#define G12_CLK_vclk2_other						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_OTHER, "vclk2_other",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    7)				/* bit */
#define G12_CLK_vclk2_enci						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_ENCI, "vclk2_enci",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    8)				/* bit */
#define G12_CLK_vclk2_encp						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_ENCP, "vclk2_encp",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    9)				/* bit */
#define G12_CLK_dac_clk							\
	MESON_CLK_GATE(MESONG12_CLOCK_DAC_CLK, "dac_clk",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    10)				/* bit */
#define G12_CLK_aoclk							\
	MESON_CLK_GATE(MESONG12_CLOCK_AOCLK, "aoclk",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    14)				/* bit */
#define G12_CLK_iec958							\
	MESON_CLK_GATE(MESONG12_CLOCK_IEC958, "iec958",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    16)				/* bit */
#define G12_CLK_enc480p							\
	MESON_CLK_GATE(MESONG12_CLOCK_ENC480P, "enc480p",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    20)				/* bit */
#define G12_CLK_rng1							\
	MESON_CLK_GATE(MESONG12_CLOCK_RNG1, "rng1",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    21)				/* bit */
#define G12_CLK_vclk2_enct						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_ENCT, "vclk2_enct",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    22)				/* bit */
#define G12_CLK_vclk2_encl						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_ENCL, "vclk2_encl",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    23)				/* bit */
#define G12_CLK_vclk2_venclmmc						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCLMMC, "vclk2_venclmmc",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    24)				/* bit */
#define G12_CLK_vclk2_vencl						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_VENCL, "vclk2_vencl",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    25)				/* bit */
#define G12_CLK_vclk2_other1						\
	MESON_CLK_GATE(MESONG12_CLOCK_VCLK2_OTHER1, "vclk2_other1",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER,		/* reg */			\
	    26)				/* bit */
#define G12_CLK_dma							\
	MESON_CLK_GATE(MESONG12_CLOCK_DMA, "dma",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER2,		/* reg */			\
	    0)				/* bit */
#define G12_CLK_efuse							\
	MESON_CLK_GATE(MESONG12_CLOCK_EFUSE, "efuse",			\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER2,		/* reg */			\
	    1)				/* bit */
#define G12_CLK_rom_boot						\
	MESON_CLK_GATE(MESONG12_CLOCK_ROM_BOOT, "rom_boot",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER2,		/* reg */			\
	    2)				/* bit */
#define G12_CLK_reset_sec						\
	MESON_CLK_GATE(MESONG12_CLOCK_RESET_SEC, "reset_sec",		\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER2,		/* reg */			\
	    3)				/* bit */
#define G12_CLK_sec_ahb_apb3						\
	MESON_CLK_GATE(MESONG12_CLOCK_SEC_AHB_APB3, "sec_ahb_apb3",	\
	    "clk81",			/* parent */			\
	    HHI_GCLK_OTHER2,		/* reg */			\
	    4)				/* bit */

/* little cpu cluster */
#define G12_CLK_cpu_clk_dyn0_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_CPU_CLK_DYN0_SEL, "cpu_clk_dyn0_sel", \
	    PARENTS("fclk_div2", "fclk_div3", "xtal"),			\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BITS(1,0),		/* sel */			\
	    0)
#define G12_CLK_cpu_clk_dyn1_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_CPU_CLK_DYN1_SEL, "cpu_clk_dyn1_sel", \
	    PARENTS("fclk_div2", "fclk_div3", "xtal"),			\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BITS(17,16),		/* sel */			\
	    0)
#define G12_CLK_cpu_clk_dyn0_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_CPU_CLK_DYN0_DIV, "cpu_clk_dyn0_div", \
	    "cpu_clk_dyn0_sel",		/* parent */			\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(26),			/* div */			\
	    0)
#define G12_CLK_cpu_clk_dyn1_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_CPU_CLK_DYN1_DIV, "cpu_clk_dyn1_div", \
	    "cpu_clk_dyn1_sel",		/* parent */			\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BITS(25,20),		/* div */			\
	    0)
#define G12_CLK_cpu_clk_dyn0						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPU_CLK_DYN0, "cpu_clk_dyn0",	\
	    PARENTS("cpu_clk_dyn0_div", "cpu_clk_dyn0_sel"),		\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(2),			/* sel */			\
	    0)
#define G12_CLK_cpu_clk_dyn1						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPU_CLK_DYN1, "cpu_clk_dyn1",	\
	    PARENTS("cpu_clk_dyn1_div", "cpu_clk_dyn1_sel"),		\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(18),			/* sel */			\
	    0)
#define G12_CLK_cpu_clk_dyn						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPU_CLK_DYN, "cpu_clk_dyn",	\
	    PARENTS("cpu_clk_dyn0", "cpu_clk_dyn1"),			\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(10),			/* sel */			\
	    0)
#define G12A_CLK_cpu_clk						\
	MESON_CLK_MUX_RATE(MESONG12_CLOCK_CPU_CLK, "cpu_clk",		\
	    PARENTS("cpu_clk_dyn", "sys_pll"),				\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(11),			/* sel */			\
	    mesong12_cpuclk_get_rate,					\
	    mesong12_cpuclk_set_rate,					\
	    0)
#define G12B_CLK_cpu_clk						\
	MESON_CLK_MUX_RATE(MESONG12_CLOCK_CPU_CLK, "cpu_clk",		\
	    PARENTS("cpu_clk_dyn", "sys1_pll"),				\
	    HHI_SYS_CPU_CLK_CNTL0,	/* reg */			\
	    __BIT(11),			/* sel */			\
	    mesong12_cpuclk_get_rate,					\
	    mesong12_cpuclk_set_rate,					\
	    0)

/* big cpu cluster */
#define G12_CLK_cpub_clk_dyn0_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_CPUB_CLK_DYN0_SEL, "cpub_clk_dyn0_sel", \
	    PARENTS("fclk_div2", "fclk_div3", "xtal"),			\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BITS(1,0),		/* sel */			\
	    0)
#define G12_CLK_cpub_clk_dyn0_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_CPUB_CLK_DYN0_DIV, "cpub_clk_dyn0_div", \
	    "cpub_clk_dyn0_sel",	/* parent */			\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BITS(9,4),		/* div */			\
	    0)
#define G12_CLK_cpub_clk_dyn0						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPUB_CLK_DYN0, "cpub_clk_dyn0",	\
	    PARENTS("cpub_clk_dyn0_div", "cpub_clk_dyn0_sel"),		\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BIT(2),			/* sel */			\
	    0)
#define G12_CLK_cpub_clk_dyn1_sel					\
	MESON_CLK_MUX(MESONG12_CLOCK_CPUB_CLK_DYN1_SEL, "cpub_clk_dyn1_sel", \
	    PARENTS("fclk_div2", "fclk_div3", "xtal", "xtal"),		\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BITS(17,16),		/* sel */			\
	    0)
#define G12_CLK_cpub_clk_dyn1_div					\
	MESON_CLK_DIV(MESONG12_CLOCK_CPUB_CLK_DYN1_DIV, "cpub_clk_dyn1_div", \
	    "cpub_clk_dyn1_sel",	/* parent */			\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BITS(25,20),		/* div */			\
	    0)
#define G12_CLK_cpub_clk_dyn1						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPUB_CLK_DYN1, "cpub_clk_dyn1",	\
	    PARENTS("cpub_clk_dyn1_div", "cpub_clk_dyn1_sel"),		\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BIT(18),			/* sel */			\
	    0)
#define G12_CLK_cpub_clk_dyn						\
	MESON_CLK_MUX(MESONG12_CLOCK_CPUB_CLK_DYN, "cpub_clk_dyn",	\
	    PARENTS("cpub_clk_dyn0", "cpub_clk_dyn1"),			\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BIT(10),			/* sel */			\
	    0)
#define G12_CLK_cpub_clk						\
	MESON_CLK_MUX_RATE(MESONG12_CLOCK_CPUB_CLK, "cpub_clk",		\
	    PARENTS("cpub_clk_dyn", "sys_pll"),				\
	    HHI_SYS_CPUB_CLK_CNTL,	/* reg */			\
	    __BIT(11),			/* sel */			\
	    mesong12_cpuclk_get_rate,					\
	    mesong12_cpuclk_set_rate,					\
	    0)

/* ts */
#define G12_CLK_ts_div							\
	MESON_CLK_DIV(MESONG12_CLOCK_TS_DIV, "ts_div",			\
	    "xtal",			/* parent */			\
	    HHI_TS_CLK_CNTL,		/* reg */			\
	    __BITS(7,0),		/* div */			\
	    0)
#define G12_CLK_ts							\
	MESON_CLK_GATE(MESONG12_CLOCK_TS, "ts",				\
	    "ts_div",			/* parent */			\
	    HHI_TS_CLK_CNTL,		/* ret */			\
	    8)				/* bit */

/* hdmi */
#define G12_CLK_hdmi_pll_dco						\
	MESON_CLK_PLL(MESONG12_CLOCK_HDMI_PLL_DCO, "hdmi_pll_dco",	\
	    "xtal",						/* parent */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL0, __BIT(28)),	/* enable */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL0, __BITS(7,0)),	/* m */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL0, __BITS(14,10)),/* n */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL1, __BITS(15,0)),/* frac */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL0, __BIT(30)),	/* l */ \
	    MESON_CLK_PLL_REG(HHI_HDMI_PLL_CNTL0, __BIT(29)),	/* reset */ \
	    0)
#define G12_CLK_hdmi_pll_od						\
	MESON_CLK_DIV(MESONG12_CLOCK_HDMI_PLL_OD, "hdmi_pll_od",	\
	    "hdmi_pll_dco",		/* parent */			\
	    HHI_HDMI_PLL_CNTL0,		/* reg */			\
	    __BITS(17,16),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO)
#define G12_CLK_hdmi_pll_od2						\
	MESON_CLK_DIV(MESONG12_CLOCK_HDMI_PLL_OD2, "hdmi_pll_od2",	\
	    "hdmi_pll_od",		/* parent */			\
	    HHI_HDMI_PLL_CNTL0,		/* reg */			\
	    __BITS(19,18),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO)
#define G12_CLK_hdmi_pll						\
	MESON_CLK_DIV(MESONG12_CLOCK_HDMI_PLL, "hdmi_pll",		\
	    "hdmi_pll_od2",		/* parent */			\
	    HHI_HDMI_PLL_CNTL0,		/* reg */			\
	    __BITS(21,20),		/* div */			\
	    MESON_CLK_DIV_POWER_OF_TWO)

#define G12_CLK_vid_pll_div						\
	MESON_CLK_DIV(MESONG12_CLOCK_VID_PLL_DIV, "vid_pll_div",	\
	    "hdmi_pll",			/* parent */			\
	    HHI_FIX_PLL_CNTL0,		/* reg */			\
	    __BITS(17,16),		/* div */			\
	    0)
#define G12_CLK_vid_pll_sel						\
	MESON_CLK_MUX(MESONG12_CLOCK_VID_PLL_SEL, "vid_pll_sel",	\
	    PARENTS("vid_pll_div", "hdmi_pll"),				\
	    HHI_VID_PLL_CLK_DIV,	/* reg */			\
	    __BIT(18),			/* sel */			\
	    0)
#define G12_CLK_vid_pll							\
	MESON_CLK_GATE(MESONG12_CLOCK_VID_PLL, "vid_pll",		\
	    "vid_pll_sel",		/* parent */			\
	    HHI_VID_PLL_CLK_DIV,	/* reg */			\
	    19)				/* bit */

/* USB3/PCIe */
#define G12_CLK_pcie_pll_dco						\
	MESON_CLK_PLL_RATE(MESONG12_CLOCK_PCIE_PLL_DCO, "pcie_pll_dco",	\
	    "xtal",						/* parent */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL0, __BIT(28)),	/* enable */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL0, __BITS(7,0)),	/* m */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL0, __BITS(14,10)),/* n */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL1, __BITS(11,0)),/* frac */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL0, __BIT(31)),	/* l */ \
	    MESON_CLK_PLL_REG(HHI_PCIE_PLL_CNTL0, __BIT(29)),	/* reset */ \
	    mesong12_clk_pcie_pll_set_rate,				\
	    0)
#define G12_CLK_pcie_pll_dco_div2					\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_PCIE_PLL_DCO_DIV2,	\
	    "pcie_pll_dco_div2",					\
	    "pcie_pll_dco",		/* parent */			\
	    2,				/* div */			\
	    1)				/* mult */
#define G12_CLK_pcie_pll_od						\
	MESON_CLK_DIV(MESONG12_CLOCK_PCIE_PLL_OD, "pcie_pll_od",	\
	    "pcie_pll_dco_div2",	/* parent */			\
	    HHI_PCIE_PLL_CNTL0,		/* reg */			\
	    __BITS(20,16),		/* div */			\
	    MESON_CLK_DIV_SET_RATE_PARENT)
#define G12_CLK_pcie_pll_pll						\
	MESON_CLK_FIXED_FACTOR(MESONG12_CLOCK_PCIE_PLL, "pcie_pll_pll",	\
	    "pcie_pll_od",		/* parent */			\
	    2,				/* div */			\
	    1)				/* mult */

/* not all clocks are defined */
static struct meson_clk_clk mesong12a_clkc_clks[] = {
	G12_CLK_fixed_pll_dco,
	G12_CLK_fixed_pll,
	G12_CLK_sys_pll_dco,
	G12_CLK_sys_pll,
	G12_CLK_fclk_div2_div,
	G12_CLK_fclk_div2,
	G12_CLK_fclk_div2p5_div,
	G12_CLK_fclk_div3_div,
	G12_CLK_fclk_div3,
	G12_CLK_fclk_div4_div,
	G12_CLK_fclk_div4,
	G12_CLK_fclk_div5_div,
	G12_CLK_fclk_div5,
	G12_CLK_fclk_div7_div,
	G12_CLK_fclk_div7,
	G12_CLK_mpll_prediv,
	G12_CLK_mpll0_div,
	G12_CLK_mpll1_div,
	G12_CLK_mpll2_div,
	G12_CLK_mpll0,
	G12_CLK_mpll1,
	G12_CLK_mpll2,
	G12_CLK_mpeg_sel,
	G12_CLK_mpeg_clk_div,
	G12_CLK_clk81,
	G12_CLK_mpll_50m_div,
	G12_CLK_mpll_50m,
	G12_CLK_sd_emmc_a_clk0_sel,
	G12_CLK_sd_emmc_b_clk0_sel,
	G12_CLK_sd_emmc_c_clk0_sel,
	G12_CLK_sd_emmc_a_clk0_div,
	G12_CLK_sd_emmc_b_clk0_div,
	G12_CLK_sd_emmc_c_clk0_div,
	G12_CLK_sd_emmc_a_clk0,
	G12_CLK_sd_emmc_b_clk0,
	G12_CLK_sd_emmc_c_clk0,
	G12_CLK_ddr,
	G12_CLK_dos,
	G12_CLK_audio_locker,
	G12_CLK_mipi_dsi_host,
	G12_CLK_eth_phy,
	G12_CLK_isa,
	G12_CLK_pl301,
	G12_CLK_periphs,
	G12_CLK_spicc0,
	G12_CLK_i2c,
	G12_CLK_sana,
	G12_CLK_sd,
	G12_CLK_rng0,
	G12_CLK_uart0,
	G12_CLK_spicc1,
	G12_CLK_hiu_iface,
	G12_CLK_mipi_dsi_phy,
	G12_CLK_assist_misc,
	G12_CLK_sd_emmc_a,
	G12_CLK_sd_emmc_b,
	G12_CLK_sd_emmc_c,
	G12_CLK_audio_codec,
	G12_CLK_audio,
	G12_CLK_eth,
	G12_CLK_demux,
	G12_CLK_audio_ififo,
	G12_CLK_adc,
	G12_CLK_uart1,
	G12_CLK_g2d,
	G12_CLK_reset,
	G12_CLK_pcie_comb,
	G12_CLK_parser,
	G12_CLK_usb,
	G12_CLK_pcie_phy,
	G12_CLK_ahb_arb0,
	G12_CLK_ahb_data_bus,
	G12_CLK_ahb_ctrl_bus,
	G12_CLK_htx_hdcp22,
	G12_CLK_htx_pclk,
	G12_CLK_bt656,
	G12_CLK_usb1_ddr_bridge,
	G12_CLK_mmc_pclk,
	G12_CLK_uart2,
	G12_CLK_vpu_intr,
	G12_CLK_gic,
	G12_CLK_vclk2_venci0,
	G12_CLK_vclk2_venci1,
	G12_CLK_vclk2_vencp0,
	G12_CLK_vclk2_vencp1,
	G12_CLK_vclk2_venct0,
	G12_CLK_vclk2_venct1,
	G12_CLK_vclk2_other,
	G12_CLK_vclk2_enci,
	G12_CLK_vclk2_encp,
	G12_CLK_dac_clk,
	G12_CLK_aoclk,
	G12_CLK_iec958,
	G12_CLK_enc480p,
	G12_CLK_rng1,
	G12_CLK_vclk2_enct,
	G12_CLK_vclk2_encl,
	G12_CLK_vclk2_venclmmc,
	G12_CLK_vclk2_vencl,
	G12_CLK_vclk2_other1,
	G12_CLK_dma,
	G12_CLK_efuse,
	G12_CLK_rom_boot,
	G12_CLK_reset_sec,
	G12_CLK_sec_ahb_apb3,
	G12_CLK_cpu_clk_dyn0_sel,
	G12_CLK_cpu_clk_dyn1_sel,
	G12_CLK_cpu_clk_dyn0_div,
	G12_CLK_cpu_clk_dyn1_div,
	G12_CLK_cpu_clk_dyn0,
	G12_CLK_cpu_clk_dyn1,
	G12_CLK_cpu_clk_dyn,
	G12A_CLK_cpu_clk,
	G12_CLK_ts_div,
	G12_CLK_ts,
	G12_CLK_hdmi_pll_dco,
	G12_CLK_hdmi_pll_od,
	G12_CLK_hdmi_pll_od2,
	G12_CLK_hdmi_pll,
	G12_CLK_vid_pll_div,
	G12_CLK_vid_pll_sel,
	G12_CLK_vid_pll,
	G12_CLK_pcie_pll_dco,
	G12_CLK_pcie_pll_dco_div2,
	G12_CLK_pcie_pll_od,
	G12_CLK_pcie_pll_pll
};

static struct meson_clk_clk mesong12b_clkc_clks[] = {
	G12_CLK_fixed_pll_dco,
	G12_CLK_fixed_pll,
	G12_CLK_sys_pll_dco,
	G12_CLK_sys_pll,
	G12B_CLK_sys1_pll_dco,
	G12B_CLK_sys1_pll,
	G12_CLK_fclk_div2_div,
	G12_CLK_fclk_div2,
	G12_CLK_fclk_div2p5_div,
	G12_CLK_fclk_div3_div,
	G12_CLK_fclk_div3,
	G12_CLK_fclk_div4_div,
	G12_CLK_fclk_div4,
	G12_CLK_fclk_div5_div,
	G12_CLK_fclk_div5,
	G12_CLK_fclk_div7_div,
	G12_CLK_fclk_div7,
	G12_CLK_mpll_prediv,
	G12_CLK_mpll0_div,
	G12_CLK_mpll1_div,
	G12_CLK_mpll2_div,
	G12_CLK_mpll0,
	G12_CLK_mpll1,
	G12_CLK_mpll2,
	G12_CLK_mpeg_sel,
	G12_CLK_mpeg_clk_div,
	G12_CLK_clk81,
	G12_CLK_mpll_50m_div,
	G12_CLK_mpll_50m,
	G12_CLK_sd_emmc_a_clk0_sel,
	G12_CLK_sd_emmc_b_clk0_sel,
	G12_CLK_sd_emmc_c_clk0_sel,
	G12_CLK_sd_emmc_a_clk0_div,
	G12_CLK_sd_emmc_b_clk0_div,
	G12_CLK_sd_emmc_c_clk0_div,
	G12_CLK_sd_emmc_a_clk0,
	G12_CLK_sd_emmc_b_clk0,
	G12_CLK_sd_emmc_c_clk0,
	G12_CLK_ddr,
	G12_CLK_dos,
	G12_CLK_audio_locker,
	G12_CLK_mipi_dsi_host,
	G12_CLK_eth_phy,
	G12_CLK_isa,
	G12_CLK_pl301,
	G12_CLK_periphs,
	G12_CLK_spicc0,
	G12_CLK_i2c,
	G12_CLK_sana,
	G12_CLK_sd,
	G12_CLK_rng0,
	G12_CLK_uart0,
	G12_CLK_spicc1,
	G12_CLK_hiu_iface,
	G12_CLK_mipi_dsi_phy,
	G12_CLK_assist_misc,
	G12_CLK_sd_emmc_a,
	G12_CLK_sd_emmc_b,
	G12_CLK_sd_emmc_c,
	G12_CLK_audio_codec,
	G12_CLK_audio,
	G12_CLK_eth,
	G12_CLK_demux,
	G12_CLK_audio_ififo,
	G12_CLK_adc,
	G12_CLK_uart1,
	G12_CLK_g2d,
	G12_CLK_reset,
	G12_CLK_pcie_comb,
	G12_CLK_parser,
	G12_CLK_usb,
	G12_CLK_pcie_phy,
	G12_CLK_ahb_arb0,
	G12_CLK_ahb_data_bus,
	G12_CLK_ahb_ctrl_bus,
	G12_CLK_htx_hdcp22,
	G12_CLK_htx_pclk,
	G12_CLK_bt656,
	G12_CLK_usb1_ddr_bridge,
	G12_CLK_mmc_pclk,
	G12_CLK_uart2,
	G12_CLK_vpu_intr,
	G12_CLK_gic,
	G12_CLK_vclk2_venci0,
	G12_CLK_vclk2_venci1,
	G12_CLK_vclk2_vencp0,
	G12_CLK_vclk2_vencp1,
	G12_CLK_vclk2_venct0,
	G12_CLK_vclk2_venct1,
	G12_CLK_vclk2_other,
	G12_CLK_vclk2_enci,
	G12_CLK_vclk2_encp,
	G12_CLK_dac_clk,
	G12_CLK_aoclk,
	G12_CLK_iec958,
	G12_CLK_enc480p,
	G12_CLK_rng1,
	G12_CLK_vclk2_enct,
	G12_CLK_vclk2_encl,
	G12_CLK_vclk2_venclmmc,
	G12_CLK_vclk2_vencl,
	G12_CLK_vclk2_other1,
	G12_CLK_dma,
	G12_CLK_efuse,
	G12_CLK_rom_boot,
	G12_CLK_reset_sec,
	G12_CLK_sec_ahb_apb3,
	G12_CLK_cpu_clk_dyn0_sel,
	G12_CLK_cpu_clk_dyn1_sel,
	G12_CLK_cpu_clk_dyn0_div,
	G12_CLK_cpu_clk_dyn1_div,
	G12_CLK_cpu_clk_dyn0,
	G12_CLK_cpu_clk_dyn1,
	G12_CLK_cpu_clk_dyn,
	G12B_CLK_cpu_clk,
	G12_CLK_cpub_clk_dyn0_sel,
	G12_CLK_cpub_clk_dyn0_div,
	G12_CLK_cpub_clk_dyn0,
	G12_CLK_cpub_clk_dyn1_sel,
	G12_CLK_cpub_clk_dyn1_div,
	G12_CLK_cpub_clk_dyn1,
	G12_CLK_cpub_clk_dyn,
	G12_CLK_cpub_clk,
	G12_CLK_ts_div,
	G12_CLK_ts,
	G12_CLK_hdmi_pll_dco,
	G12_CLK_hdmi_pll_od,
	G12_CLK_hdmi_pll_od2,
	G12_CLK_hdmi_pll,
	G12_CLK_vid_pll_div,
	G12_CLK_vid_pll_sel,
	G12_CLK_vid_pll,
	G12_CLK_pcie_pll_dco,
	G12_CLK_pcie_pll_dco_div2,
	G12_CLK_pcie_pll_od,
	G12_CLK_pcie_pll_pll
};

/*
 * XXX:
 * mesong12_cpuclk_get_rate() is needed because the source clock exceeds 32bit
 * and sys/dev/clk cannot handle it.
 * By modifying sys/dev/clk to be able to handle 64-bit clocks, this function
 * will no longer be needed.
 */
static u_int
mesong12_cpuclk_get_rate(struct meson_clk_softc *sc, struct meson_clk_clk *clk)
{
	bus_size_t reg_cntl0;
	uint64_t freq;
	uint32_t val, m, n, div_shift, div;
	uint64_t xtal_clock = clk_get_rate(fdtbus_clock_byname("xtal"));

	KASSERT(clk->type == MESON_CLK_MUX);
	if (sc->sc_clks == mesong12a_clkc_clks) {
		reg_cntl0 = HHI_SYS_PLL_CNTL0;
	} else {
		switch (clk->u.mux.reg) {
		case HHI_SYS_CPU_CLK_CNTL0:
			reg_cntl0 = HHI_SYS1_PLL_CNTL0;
			break;
		case HHI_SYS_CPUB_CLK_CNTL:
			reg_cntl0 = HHI_SYS_PLL_CNTL0;
			break;
		default:
			panic("%s: illegal clk table\n", __func__);
		}
	}
	CLK_LOCK(sc);

	if ((CLK_READ(sc, clk->u.mux.reg) & __BIT(11)) == 0) {
		CLK_UNLOCK(sc);

		/* use dyn clock instead of sys1?_pll */
		struct clk *clkp;

		switch (clk->u.mux.reg) {
		case HHI_SYS_CPU_CLK_CNTL0:
			clkp = (struct clk *)meson_clk_clock_find(sc,
			    "cpu_clk_dyn");
			freq = clk_get_rate(clkp);
			break;
		case HHI_SYS_CPUB_CLK_CNTL:
			clkp = (struct clk *)meson_clk_clock_find(sc,
			    "cpub_clk_dyn");
			freq = clk_get_rate(clkp);
			break;
		default:
			freq = 0;
			break;
		}
		return freq;
	}
	val = CLK_READ(sc, reg_cntl0);
	m = __SHIFTOUT(val, __BITS(7,0));
	n = __SHIFTOUT(val, __BITS(14,10));
	div_shift = __SHIFTOUT(val, __BITS(18,16));
	div = 1 << div_shift;
	CLK_UNLOCK(sc);

	freq = xtal_clock * m / n / div;
	return freq;
}

static int
mesong12_cpuclk_set_rate(struct meson_clk_softc *sc, struct meson_clk_clk *clk,
    u_int rate)
{
	bus_size_t reg_cntl0;
	uint32_t val;
	uint64_t xtal_clock = clk_get_rate(fdtbus_clock_byname("xtal"));
	int new_m, new_n, new_div, i, error;

	KASSERT(clk->type == MESON_CLK_MUX);
	if (sc->sc_clks == mesong12a_clkc_clks) {
		reg_cntl0 = HHI_SYS_PLL_CNTL0;
	} else {
		switch (clk->u.mux.reg) {
		case HHI_SYS_CPU_CLK_CNTL0:
			reg_cntl0 = HHI_SYS1_PLL_CNTL0;
			break;
		case HHI_SYS_CPUB_CLK_CNTL:
			reg_cntl0 = HHI_SYS_PLL_CNTL0;
			break;
		default:
			panic("%s: illegal clk table\n", __func__);
		}
	}

	new_div = 7;
	new_n = 1;
	new_m = (uint64_t)rate * (1 << new_div) / xtal_clock;
	while (new_m >= 250 && (new_div > 0)) {
		new_div--;
		new_m /= 2;
	}

	if (new_m >= 256)
		return EINVAL;

	CLK_LOCK(sc);

	/* use cpub?_clk_dyn temporary */
	val = CLK_READ(sc, clk->u.mux.reg);
	CLK_WRITE(sc, clk->u.mux.reg, val & ~__BIT(11));
	DELAY(100);

#define MESON_PLL_CNTL_REG_LOCK	__BIT(31)
#define MESON_PLL_CNTL_REG_RST	__BIT(29)
#define MESON_PLL_CNTL_REG_EN	__BIT(28)
	/* disable */
	val = CLK_READ(sc, reg_cntl0);
	CLK_WRITE(sc, reg_cntl0, val | MESON_PLL_CNTL_REG_RST);
	val = CLK_READ(sc, reg_cntl0);
	CLK_WRITE(sc, reg_cntl0, val & ~MESON_PLL_CNTL_REG_EN);

	/* HHI_SYS{,1}_PLL_CNTL{1,2,3,4,5} */
	CLK_WRITE(sc, reg_cntl0 + CBUS_REG(1), 0x00000000);
	CLK_WRITE(sc, reg_cntl0 + CBUS_REG(2), 0x00000000);
	CLK_WRITE(sc, reg_cntl0 + CBUS_REG(3), 0x48681c00);
	CLK_WRITE(sc, reg_cntl0 + CBUS_REG(4), 0x88770290);
	CLK_WRITE(sc, reg_cntl0 + CBUS_REG(5), 0x39272000);
	DELAY(100);

	/* write new M, N, and DIV */
	val = CLK_READ(sc, reg_cntl0);
	val &= ~__BITS(7,0);
	val |= __SHIFTIN(new_m, __BITS(7,0));
	val &= ~__BITS(14,10);
	val |= __SHIFTIN(new_n, __BITS(14,10));
	val &= ~__BITS(18,16);
	val |= __SHIFTIN(new_div, __BITS(18,16));
	CLK_WRITE(sc, reg_cntl0, val);

	/* enable */
	val = CLK_READ(sc, reg_cntl0);
	CLK_WRITE(sc, reg_cntl0, val | MESON_PLL_CNTL_REG_RST);
	val = CLK_READ(sc, reg_cntl0);
	CLK_WRITE(sc, reg_cntl0, val | MESON_PLL_CNTL_REG_EN);
	DELAY(1000);
	val = CLK_READ(sc, reg_cntl0);
	CLK_WRITE(sc, reg_cntl0, val & ~MESON_PLL_CNTL_REG_RST);

	error = ETIMEDOUT;
	for (i = 24000000; i > 0; i--) {
		if ((CLK_READ(sc, reg_cntl0) & MESON_PLL_CNTL_REG_LOCK) != 0) {
			error = 0;
			break;
		}
	}

	/* XXX: always use sys1?_pll. cpub?_clk_dyn should be used for <1GHz */
	val = CLK_READ(sc, clk->u.mux.reg);
	CLK_WRITE(sc, clk->u.mux.reg, val | __BIT(11));
	DELAY(100);

	CLK_UNLOCK(sc);

	return error;
}

static int
mesong12_clk_pcie_pll_set_rate(struct meson_clk_softc *sc,
    struct meson_clk_clk *clk, u_int new_rate)
{
	struct meson_clk_pll *pll = &clk->u.pll;

	KASSERT(clk->type == MESON_CLK_PLL);

	/*
	 * A strict register sequence is required to set the PLL to the
	 * fine-tuned 100MHz for PCIe
	 */
	if (new_rate == (100000000 * 2 * 2)) { /* "2*2" is fixed factor */
		uint32_t cntl0, cntl4, cntl3, cntl5;

		cntl0 = __SHIFTIN(9, HHI_PCIE_PLL_CNTL0_PCIE_APLL_OD) |
		    __SHIFTIN(1, HHI_PCIE_PLL_CNTL0_PCIE_APLL_PREDIV_SEL) |
		    __SHIFTIN(150, HHI_PCIE_PLL_CNTL0_PCIE_APLL_FBKDIV);
		cntl4 =  __SHIFTIN(1, HHI_PCIE_PLL_CNTL4_PCIE_APLL_LPF_CAPADJ) |
		    HHI_PCIE_PLL_CNTL4_PCIE_APLL_LOAD |
		    HHI_PCIE_PLL_CNTL4_PCIE_APLL_LOAD_EN;
		cntl3 = __SHIFTIN(1, HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_HOLD_T) |
		    __SHIFTIN(2, HHI_PCIE_PLL_CNTL3_PCIE_APLL_AFC_DIV) |
		    HHI_PCIE_PLL_CNTL3_PCIE_APLL_BIAS_LPF_EN |
		    __SHIFTIN(8, HHI_PCIE_PLL_CNTL3_PCIE_APLL_CP_ICAP) |
		    __SHIFTIN(14, HHI_PCIE_PLL_CNTL3_PCIE_APLL_CP_IRES);
		cntl5 = __SHIFTIN(6, HHI_PCIE_PLL_CNTL5_PCIE_HCSL_ADJ_LDO) |
		    HHI_PCIE_PLL_CNTL5_PCIE_HCSL_BGP_EN |
		    HHI_PCIE_PLL_CNTL5_PCIE_HCSL_CAL_EN |
		    HHI_PCIE_PLL_CNTL5_PCIE_HCSL_EN0;

		CLK_LOCK(sc);
		CLK_WRITE_BITS(sc, pll->reset.reg, pll->reset.mask, 1);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL0, cntl0 |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_RESET);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL0, cntl0 |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_RESET |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_EN);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL1, 0);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL2,
		    __SHIFTIN(0x1100, HHI_PCIE_PLL_CNTL2_PCIE_APLL_RESERVE));
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL3, cntl3);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL4, cntl4);
		delay(20);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL5, cntl5);
		delay(10);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL5, cntl5 |
		    HHI_PCIE_PLL_CNTL5_PCIE_HCSL_CAL_RSTN);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL4, cntl4 |
		    HHI_PCIE_PLL_CNTL4_PCIE_APLL_VCTRL_MON_EN);
		delay(10);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL0, cntl0 |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_RESET |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_EN |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_AFC_START);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL0, cntl0 |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_EN |
		    HHI_PCIE_PLL_CNTL0_PCIE_APLL_AFC_START);
		CLK_WRITE(sc, HHI_PCIE_PLL_CNTL2,
		    __SHIFTIN(0x1000, HHI_PCIE_PLL_CNTL2_PCIE_APLL_RESERVE));

		CLK_WRITE_BITS(sc, pll->reset.reg, pll->reset.mask, 0);
		/* XXX: pll_wait_lock() sometimes times out? but ignore it */
		meson_clk_pll_wait_lock(sc, pll);
		CLK_UNLOCK(sc);
		return 0;
	}

	return meson_clk_pll_set_rate(sc, clk, new_rate);
}

static const struct mesong12_clkc_config g12a_config = {
	.name = "Meson G12A",
	.clks = mesong12a_clkc_clks,
	.nclks = __arraycount(mesong12a_clkc_clks),
};

static const struct mesong12_clkc_config g12b_config = {
	.name = "Meson G12B",
	.clks = mesong12b_clkc_clks,
	.nclks = __arraycount(mesong12b_clkc_clks),
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,g12a-clkc",	.data = &g12a_config },
	{ .compat = "amlogic,g12b-clkc",	.data = &g12b_config },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(mesong12_clkc, sizeof(struct meson_clk_softc),
    mesong12_clkc_match, mesong12_clkc_attach, NULL, NULL);

static int
mesong12_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesong12_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct meson_clk_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const struct mesong12_clkc_config *conf;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(sc->sc_phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	conf = of_compatible_lookup(phandle, compat_data)->data;
	sc->sc_clks = conf->clks;
	sc->sc_nclks = conf->nclks;

	aprint_naive("\n");
	aprint_normal(": %s clock controller\n", conf->name);

	meson_clk_attach(sc);
	meson_clk_print(sc);
}
