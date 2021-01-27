/* $NetBSD: imx7d_ccm.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: imx7d_ccm.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/nxp/imx_ccm.h>
#include <arm/nxp/imx7d_ccm.h>

static int imx7d_ccm_match(device_t, cfdata_t, void *);
static void imx7d_ccm_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx7d-ccm" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry anatop_compat_data[] = {
	{ .compat = "fsl,imx7d-anatop" },
	DEVICE_COMPAT_EOL
};

static const char *pll_bypass_p[] = {
	"osc", "dummy"
};
static const char *pll_sys_main_bypass_p[] = {
	"pll_sys_main", "pll_sys_main_src"
};
static const char *pll_enet_main_bypass_p[] = {
	"pll_enet_main", "pll_enet_main_src"
};
static const char *uart1357_p[] = {
	"osc", "pll_sys_main_240m_clk", "pll_enet_40m_clk", "pll_enet_100m_clk", "pll_sys_main_clk", "ext_clk_2", "ext_clk_4", "pll_usb_main_clk"
};
static const char *uart246_p[] = {
	"osc", "pll_sys_main_240m_clk", "pll_enet_40m_clk", "pll_enet_100m_clk", "pll_sys_main_clk", "ext_clk_2", "ext_clk_4", "pll_usb_main_clk"
};
static const char *i2c_p[] = {
	"osc", "pll_sys_main_120m_clk", "pll_enet_50m_clk", "pll_dram_533m_clk", "pll_audio_post_div", "pll_video_post_div", "pll_usb_main_clk", "pll_sys_pfd2_135m_clk"
};
static const char *enet_axi_p[] = {
	"osc", "pll_sys_pfd2_270m_clk", "pll_dram_533m_clk", "pll_enet_250m_clk", "pll_sys_main_240m_clk", "pll_audio_post_div", "pll_video_post_div", "pll_sys_pfd4_clk"
};
static const char *enet_time_p[] = {
	"osc", "pll_enet_100m_clk", "pll_audio_post_div", "ext_clk_1", "ext_clk_2", "ext_clk_3", "ext_clk_4", "pll_video_post_div"
};
static const char *enet_phy_ref_p[] = {
	"osc", "pll_enet_25m_clk", "pll_enet_50m_clk", "pll_enet_125m_clk", "pll_dram_533m_clk", "pll_audio_post_div", "pll_video_post_div", "pll_sys_pfd3_clk"
};
static const char *ahb_channel_p[] = {
	"osc", "pll_sys_pfd2_270m_clk", "pll_dram_533m_clk", "pll_sys_pfd0_392m_clk", "pll_enet_250m_clk", "pll_usb_main_clk", "pll_audio_post_div", "pll_video_post_div"
};
static const char *nand_usdhc_p[] = {
	"osc", "pll_sys_pfd2_270m_clk", "pll_dram_533m_clk", "pll_sys_main_240m_clk", "pll_sys_pfd2_135m_clk", "pll_sys_pfd6_clk", "pll_enet_250m_clk", "pll_audio_post_div"
};
static const char *usdhc_p[] = {
	"osc", "pll_sys_pfd0_392m_clk", "pll_dram_533m_clk", "pll_enet_500m_clk", "pll_sys_pfd4_clk", "pll_sys_pfd2_270m_clk", "pll_sys_pfd6_clk", "pll_sys_pfd7_clk"
};

CFATTACH_DECL_NEW(imx7d_ccm, sizeof(struct imx_ccm_softc),
	imx7d_ccm_match, imx7d_ccm_attach, NULL, NULL);

enum {
	REGIDX_CCM = 0,
	REGIDX_ANATOP = 1,
};

#define	ANATOP_MUX(_id, _name, _parents, _reg, _mask)			\
	IMX_MUX_INDEX(_id, REGIDX_ANATOP, _name, _parents, _reg, _mask)
#define	ANATOP_GATE(_id, _name, _parent, _reg, _mask)			\
	IMX_GATE_INDEX(_id, REGIDX_ANATOP, _name, _parent, _reg, _mask)
#define	ANATOP_PLL(_id, _name, _parent, _reg, _div_mask, _flags)	\
	IMX_PLL_INDEX(_id, REGIDX_ANATOP, _name, _parent, _reg, _div_mask, _flags)

static struct imx_ccm_clk imx7d_ccm_clks[] = {

	IMX_FIXED(CLK_DUMMY, "dummy", 0),
	IMX_EXTCLK(CKIL, "ckil"),
	IMX_EXTCLK(OSC_24M_CLK, "osc"),

	/*
	 * CCM_ANALOG
	 */
	ANATOP_MUX(PLL_SYS_MAIN_SRC, "pll_sys_main_src", pll_bypass_p, 0xb0, __BITS(15,14)),
	ANATOP_MUX(PLL_ENET_MAIN_SRC, "pll_enet_main_src", pll_bypass_p, 0xe0, __BITS(15,14)),

	ANATOP_PLL(PLL_SYS_MAIN, "pll_sys_main", "osc", 0xb0, __BIT(0), IMX_PLL_480M_528M),
	ANATOP_PLL(PLL_ENET_MAIN, "pll_enet_main", "osc", 0xe0, 1000000000, IMX_PLL_ENET),

	ANATOP_MUX(PLL_SYS_MAIN_BYPASS, "pll_sys_main_bypass", pll_sys_main_bypass_p, 0xb0, __BIT(16)),
	ANATOP_MUX(PLL_ENET_MAIN_BYPASS, "pll_enet_main_bypass", pll_enet_main_bypass_p, 0xe0, __BIT(16)),

	ANATOP_GATE(PLL_SYS_MAIN_CLK, "pll_sys_main_clk", "pll_sys_main_bypass", 0xb0, __BIT(13)),

	IMX_FIXED_FACTOR(PLL_SYS_MAIN_240M, "pll_sys_main_240m", "pll_sys_main_clk", 1, 2),

	ANATOP_GATE(PLL_SYS_MAIN_240M_CLK, "pll_sys_main_240m_clk", "pll_sys_main_240m", 0xb0, __BIT(5)),

	IMX_FIXED_FACTOR(PLL_ENET_MAIN_CLK, "pll_enet_main_clk", "pll_enet_main_bypass", 1, 1),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_500M, "pll_enet_500m", "pll_enet_main_clk", 1, 2),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_250M, "pll_enet_250m", "pll_enet_main_clk", 1, 4),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_125M, "pll_enet_125m", "pll_enet_main_clk", 1, 8),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_100M, "pll_enet_100m", "pll_enet_main_clk", 1, 10),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_50M, "pll_enet_50m", "pll_enet_main_clk", 1, 20),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_40M, "pll_enet_40m", "pll_enet_main_clk", 1, 25),
	IMX_FIXED_FACTOR(PLL_ENET_MAIN_25M, "pll_enet_25m", "pll_enet_main_clk", 1, 40),

	ANATOP_GATE(PLL_ENET_MAIN_500M_CLK, "pll_enet_500m_clk", "pll_enet_500m", 0xe0, __BIT(12)),
	ANATOP_GATE(PLL_ENET_MAIN_250M_CLK, "pll_enet_250m_clk", "pll_enet_250m", 0xe0, __BIT(11)),
	ANATOP_GATE(PLL_ENET_MAIN_250M_CLK, "pll_enet_250m_clk", "pll_enet_250m", 0xe0, __BIT(11)),
	ANATOP_GATE(PLL_ENET_MAIN_125M_CLK, "pll_enet_125m_clk", "pll_enet_125m", 0xe0, __BIT(10)),
	ANATOP_GATE(PLL_ENET_MAIN_100M_CLK, "pll_enet_100m_clk", "pll_enet_100m", 0xe0, __BIT(9)),
	ANATOP_GATE(PLL_ENET_MAIN_50M_CLK, "pll_enet_50m_clk", "pll_enet_50m", 0xe0, __BIT(8)),
	ANATOP_GATE(PLL_ENET_MAIN_40M_CLK, "pll_enet_40m_clk", "pll_enet_40m", 0xe0, __BIT(7)),
	ANATOP_GATE(PLL_ENET_MAIN_25M_CLK, "pll_enet_25m_clk", "pll_enet_25m", 0xe0, __BIT(6)),

	IMX_FIXED_FACTOR(USB1_MAIN_480M_CLK, "pll_usb1_main_clk", "osc", 20, 1),
	IMX_FIXED_FACTOR(USB_MAIN_480M_CLK, "pll_usb_main_clk", "osc", 20, 1),

	/*
	 * CCM (regidx=0)
	 */

	IMX_MUX(UART1_ROOT_SRC, "uart1_src", uart1357_p, 0xaf80, __BITS(26,24)),
	IMX_MUX(UART2_ROOT_SRC, "uart2_src", uart246_p, 0xb000, __BITS(26,24)),
	IMX_MUX(UART3_ROOT_SRC, "uart3_src", uart1357_p, 0xb080, __BITS(26,24)),
	IMX_MUX(UART4_ROOT_SRC, "uart4_src", uart246_p, 0xb100, __BITS(26,24)),
	IMX_MUX(UART5_ROOT_SRC, "uart5_src", uart1357_p, 0xb180, __BITS(26,24)),
	IMX_MUX(UART6_ROOT_SRC, "uart6_src", uart246_p, 0xb200, __BITS(26,24)),
	IMX_MUX(UART7_ROOT_SRC, "uart7_src", uart1357_p, 0xb280, __BITS(26,24)),

	IMX_GATE(UART1_ROOT_CG, "uart1_cg", "uart1_src", 0xaf80, __BIT(18)),
	IMX_GATE(UART2_ROOT_CG, "uart2_cg", "uart2_src", 0xb000, __BIT(18)),
	IMX_GATE(UART3_ROOT_CG, "uart3_cg", "uart3_src", 0xb080, __BIT(18)),
	IMX_GATE(UART4_ROOT_CG, "uart4_cg", "uart4_src", 0xb100, __BIT(18)),
	IMX_GATE(UART5_ROOT_CG, "uart5_cg", "uart5_src", 0xb180, __BIT(18)),
	IMX_GATE(UART6_ROOT_CG, "uart6_cg", "uart6_src", 0xb200, __BIT(18)),
	IMX_GATE(UART7_ROOT_CG, "uart7_cg", "uart7_src", 0xb280, __BIT(18)),

	IMX_DIV(UART1_ROOT_PRE_DIV, "uart1_pre_div", "uart1_cg", 0xaf80, __BITS(18,16), 0),
	IMX_DIV(UART2_ROOT_PRE_DIV, "uart2_pre_div", "uart2_cg", 0xb000, __BITS(18,16), 0),
	IMX_DIV(UART3_ROOT_PRE_DIV, "uart3_pre_div", "uart3_cg", 0xb080, __BITS(18,16), 0),
	IMX_DIV(UART4_ROOT_PRE_DIV, "uart4_pre_div", "uart4_cg", 0xb100, __BITS(18,16), 0),
	IMX_DIV(UART5_ROOT_PRE_DIV, "uart5_pre_div", "uart5_cg", 0xb100, __BITS(18,16), 0),
	IMX_DIV(UART6_ROOT_PRE_DIV, "uart6_pre_div", "uart6_cg", 0xb200, __BITS(18,16), 0),
	IMX_DIV(UART7_ROOT_PRE_DIV, "uart7_pre_div", "uart7_cg", 0xb280, __BITS(18,16), 0),

	IMX_DIV(UART1_ROOT_DIV, "uart1_post_div", "uart1_pre_div", 0xaf80, __BITS(5,0), 0),
	IMX_DIV(UART2_ROOT_DIV, "uart2_post_div", "uart2_pre_div", 0xb000, __BITS(5,0), 0),
	IMX_DIV(UART3_ROOT_DIV, "uart3_post_div", "uart3_pre_div", 0xb080, __BITS(5,0), 0),
	IMX_DIV(UART4_ROOT_DIV, "uart4_post_div", "uart4_pre_div", 0xb100, __BITS(5,0), 0),
	IMX_DIV(UART5_ROOT_DIV, "uart5_post_div", "uart5_pre_div", 0xb100, __BITS(5,0), 0),
	IMX_DIV(UART6_ROOT_DIV, "uart6_post_div", "uart6_pre_div", 0xb200, __BITS(5,0), 0),
	IMX_DIV(UART7_ROOT_DIV, "uart7_post_div", "uart7_pre_div", 0xb280, __BITS(5,0), 0),

	IMX_GATE(UART1_ROOT_CLK, "uart1_root_clk", "uart1_post_div", 0x4940, __BIT(0)),
	IMX_GATE(UART2_ROOT_CLK, "uart2_root_clk", "uart2_post_div", 0x4950, __BIT(0)),
	IMX_GATE(UART3_ROOT_CLK, "uart3_root_clk", "uart3_post_div", 0x4960, __BIT(0)),
	IMX_GATE(UART4_ROOT_CLK, "uart4_root_clk", "uart4_post_div", 0x4970, __BIT(0)),
	IMX_GATE(UART5_ROOT_CLK, "uart5_root_clk", "uart5_post_div", 0x4980, __BIT(0)),
	IMX_GATE(UART6_ROOT_CLK, "uart6_root_clk", "uart6_post_div", 0x4990, __BIT(0)),
	IMX_GATE(UART7_ROOT_CLK, "uart7_root_clk", "uart7_post_div", 0x49a0, __BIT(0)),

	IMX_MUX(I2C1_ROOT_SRC, "i2c1_src", i2c_p, 0xad80, __BITS(26,24)),
	IMX_MUX(I2C2_ROOT_SRC, "i2c2_src", i2c_p, 0xae00, __BITS(26,24)),
	IMX_MUX(I2C3_ROOT_SRC, "i2c3_src", i2c_p, 0xae80, __BITS(26,24)),
	IMX_MUX(I2C4_ROOT_SRC, "i2c4_src", i2c_p, 0xaf00, __BITS(26,24)),

	IMX_GATE(I2C1_ROOT_CG, "i2c1_cg", "i2c1_src", 0xad80, __BIT(0)),
	IMX_GATE(I2C2_ROOT_CG, "i2c2_cg", "i2c2_src", 0xae00, __BIT(0)),
	IMX_GATE(I2C3_ROOT_CG, "i2c3_cg", "i2c3_src", 0xae80, __BIT(0)),
	IMX_GATE(I2C4_ROOT_CG, "i2c4_cg", "i2c4_src", 0xaf00, __BIT(0)),

	IMX_DIV(I2C1_ROOT_PRE_DIV, "i2c1_pre_div", "i2c1_cg", 0xad80, __BITS(18,16), 0),
	IMX_DIV(I2C2_ROOT_PRE_DIV, "i2c2_pre_div", "i2c2_cg", 0xae00, __BITS(18,16), 0),
	IMX_DIV(I2C3_ROOT_PRE_DIV, "i2c3_pre_div", "i2c3_cg", 0xae80, __BITS(18,16), 0),
	IMX_DIV(I2C4_ROOT_PRE_DIV, "i2c4_pre_div", "i2c4_cg", 0xaf00, __BITS(18,16), 0),

	IMX_DIV(I2C1_ROOT_DIV, "i2c1_post_div", "i2c1_pre_div", 0xad80, __BITS(5,0), 0),
	IMX_DIV(I2C2_ROOT_DIV, "i2c2_post_div", "i2c2_pre_div", 0xae00, __BITS(5,0), 0),
	IMX_DIV(I2C3_ROOT_DIV, "i2c3_post_div", "i2c3_pre_div", 0xae80, __BITS(5,0), 0),
	IMX_DIV(I2C4_ROOT_DIV, "i2c4_post_div", "i2c4_pre_div", 0xaf00, __BITS(5,0), 0),

	IMX_GATE(I2C1_ROOT_CLK, "i2c1_root_clk", "i2c1_post_div", 0x4880, __BIT(0)),
	IMX_GATE(I2C2_ROOT_CLK, "i2c2_root_clk", "i2c2_post_div", 0x4890, __BIT(0)),
	IMX_GATE(I2C3_ROOT_CLK, "i2c3_root_clk", "i2c3_post_div", 0x48a0, __BIT(0)),
	IMX_GATE(I2C4_ROOT_CLK, "i2c4_root_clk", "i2c4_post_div", 0x48b0, __BIT(0)),

	IMX_MUX(ENET_AXI_ROOT_SRC, "enet_axi_src", enet_axi_p, 0x8900, __BITS(26,24)),
	IMX_GATE(ENET_AXI_ROOT_CG, "enet_axi_cg", "enet_axi_src", 0x8900, __BIT(28)),
	IMX_DIV(ENET_AXI_ROOT_PRE_DIV, "enet_axi_pre_div", "enet_axi_cg", 0x8900, __BITS(18,16), 0),
	IMX_DIV(ENET_AXI_ROOT_DIV, "enet_axi_post_div", "enet_axi_pre_div", 0x8900, __BITS(5,0), 0),

	IMX_MUX(ENET1_TIME_ROOT_SRC, "enet1_time_src", enet_time_p, 0xa780, __BITS(26,24)),
	IMX_MUX(ENET2_TIME_ROOT_SRC, "enet2_time_src", enet_time_p, 0xa880, __BITS(26,24)),
	IMX_GATE(ENET1_TIME_ROOT_CG, "enet1_time_cg", "enet1_time_src", 0xa780, __BIT(28)),
	IMX_GATE(ENET2_TIME_ROOT_CG, "enet2_time_cg", "enet2_time_src", 0xa880, __BIT(28)),
	IMX_DIV(ENET1_TIME_ROOT_PRE_DIV, "enet1_time_pre_div", "enet1_time_cg", 0xa780, __BITS(18,16), 0),
	IMX_DIV(ENET2_TIME_ROOT_PRE_DIV, "enet2_time_pre_div", "enet2_time_cg", 0xa880, __BITS(18,16), 0),
	IMX_DIV(ENET1_TIME_ROOT_DIV, "enet1_time_post_div", "enet1_time_pre_div", 0xa780, __BITS(5,0), 0),
	IMX_DIV(ENET2_TIME_ROOT_DIV, "enet2_time_post_div", "enet2_time_pre_div", 0xa880, __BITS(5,0), 0),
	IMX_GATE(ENET1_IPG_ROOT_CLK, "enet1_ipg_root_clk", "enet_axi_post_div", 0x4700, __BIT(0)),
	IMX_GATE(ENET2_IPG_ROOT_CLK, "enet2_ipg_root_clk", "enet_axi_post_div", 0x4710, __BIT(0)),
	IMX_GATE(ENET1_TIME_ROOT_CLK, "enet1_time_root_clk", "enet1_time_post_div", 0x4700, __BIT(0)),
	IMX_GATE(ENET2_TIME_ROOT_CLK, "enet2_time_root_clk", "enet2_time_post_div", 0x4710, __BIT(0)),
	IMX_GATE(ENET_AXI_ROOT_CLK, "enet_axi_root_clk", "enet_axi_post_div", 0x4060, __BIT(0)),

	IMX_MUX(ENET_PHY_REF_ROOT_SRC, "enet_phy_ref_src", enet_phy_ref_p, 0xa900, __BITS(26,24)),
	IMX_GATE(ENET_PHY_REF_ROOT_CG, "enet_phy_ref_cg", "enet_phy_ref_src", 0xa900, __BIT(28)),
	IMX_DIV(ENET_PHY_REF_ROOT_PRE_DIV, "enet_phy_ref_pre_div", "enet_phy_ref_cg", 0xa900, __BITS(18,16), 0),
	IMX_DIV(ENET_PHY_REF_ROOT_CLK, "enet_phy_ref_root_clk", "enet_phy_ref_pre_div", 0xa900, __BITS(5,0), 0),

	IMX_MUX(AHB_CHANNEL_ROOT_SRC, "ahb_src", ahb_channel_p, 0x9000, __BITS(26,24)),
	IMX_GATE(AHB_CHANNEL_ROOT_CG, "ahb_cg", "ahb_src", 0x9000, __BIT(28)),
	IMX_DIV(AHB_CHANNEL_ROOT_PRE_DIV, "ahb_pre_div", "ahb_cg", 0x9000, __BITS(18,16), 0),
	IMX_DIV(AHB_CHANNEL_ROOT_DIV, "ahb_root_clk", "ahb_pre_div", 0x9000, __BITS(5,0), 0),
	IMX_DIV(IPG_ROOT_CLK, "ipg_root_clk", "ahb_root_clk", 0x9080, __BITS(1,0), IMX_DIV_SET_RATE_PARENT),

	IMX_MUX(NAND_USDHC_BUS_ROOT_SRC, "nand_usdhc_src", nand_usdhc_p, 0x8980, __BITS(26,24)),
	IMX_GATE(NAND_USDHC_BUS_ROOT_CG, "nand_usdhc_cg", "nand_usdhc_src", 0x8980, __BIT(28)),
	IMX_DIV(NAND_USDHC_BUS_ROOT_PRE_DIV, "nand_usdhc_pre_div", "nand_usdhc_cg", 0x8980, __BITS(18,16), 0),
	IMX_DIV(NAND_USDHC_BUS_ROOT_CLK, "nand_usdhc_root_clk", "nand_usdhc_pre_div", 0x8980, __BITS(5,0), 0),

	IMX_MUX(USDHC1_ROOT_SRC, "usdhc1_src", usdhc_p, 0xab00, __BITS(26,24)),
	IMX_MUX(USDHC2_ROOT_SRC, "usdhc2_src", usdhc_p, 0xab80, __BITS(26,24)),
	IMX_MUX(USDHC3_ROOT_SRC, "usdhc3_src", usdhc_p, 0xac00, __BITS(26,24)),
	IMX_GATE(USDHC1_ROOT_CG, "usdhc1_cg", "usdhc1_src", 0xab00, __BIT(28)),
	IMX_GATE(USDHC2_ROOT_CG, "usdhc2_cg", "usdhc2_src", 0xab80, __BIT(28)),
	IMX_GATE(USDHC3_ROOT_CG, "usdhc3_cg", "usdhc3_src", 0xac00, __BIT(28)),
	IMX_DIV(USDHC1_ROOT_PRE_DIV, "usdhc1_pre_div", "usdhc1_cg", 0xab00, __BITS(18,16), 0),
	IMX_DIV(USDHC1_ROOT_PRE_DIV, "usdhc2_pre_div", "usdhc2_cg", 0xab80, __BITS(18,16), 0),
	IMX_DIV(USDHC1_ROOT_PRE_DIV, "usdhc3_pre_div", "usdhc3_cg", 0xac00, __BITS(18,16), 0),
	IMX_DIV(USDHC1_ROOT_DIV, "usdhc1_post_div", "usdhc1_pre_div", 0xab00, __BITS(5,0), 0),
	IMX_DIV(USDHC2_ROOT_DIV, "usdhc2_post_div", "usdhc2_pre_div", 0xab80, __BITS(5,0), 0),
	IMX_DIV(USDHC3_ROOT_DIV, "usdhc3_post_div", "usdhc3_pre_div", 0xac00, __BITS(5,0), 0),
	IMX_GATE(USDHC1_ROOT_CLK, "usdhc1_root_clk", "usdhc1_post_div", 0x46c0, __BIT(0)),
	IMX_GATE(USDHC2_ROOT_CLK, "usdhc2_root_clk", "usdhc2_post_div", 0x46d0, __BIT(0)),
	IMX_GATE(USDHC3_ROOT_CLK, "usdhc3_root_clk", "usdhc3_post_div", 0x46e0, __BIT(0)),

	IMX_GATE(USB_CTRL_CLK, "usb_ctrl_clk", "ahb_root_clk", 0x4680, __BIT(0)),
	IMX_GATE(USB_PHY1_CLK, "usb_phy1_clk", "pll_usb1_main_clk", 0x46a0, __BIT(0)),
	IMX_GATE(USB_PHY2_CLK, "usb_phy2_clk", "pll_usb_main_clk", 0x46b0, __BIT(0)),
};

static int
imx7d_ccm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx7d_ccm_attach(device_t parent, device_t self, void *aux)
{
	struct imx_ccm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t anatop_addr;
	bus_size_t anatop_size;
	int anatop = -1, child;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = imx7d_ccm_clks;
	sc->sc_nclks = __arraycount(imx7d_ccm_clks);

	for (child = OF_child(OF_parent(phandle)); child; child = OF_peer(child)) {
		if (of_compatible_match(child, anatop_compat_data)) {
			anatop = child;
			break;
		}
	}
	if (anatop == -1) {
		aprint_error(": couldn't find anatop node\n");
		return;
	}
	if (fdtbus_get_reg(anatop, 0, &anatop_addr, &anatop_size) != 0) {
		aprint_error(": couldn't get anatop registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, anatop_addr, anatop_size, 0, &sc->sc_bsh[REGIDX_ANATOP]) != 0) {
		aprint_error(": couldn't map anatop registers\n");
		return;
	}

	if (imx_ccm_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": Clock Control Module\n");

	imx_ccm_print(sc);
}
