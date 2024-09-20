/* $NetBSD: jh7110_clkc.c,v 1.6 2024/09/20 07:29:39 rin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: jh7110_clkc.c,v 1.6 2024/09/20 07:29:39 rin Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/starfive/jh71x0_clkc.h>

/* SYSCRG clocks */
#define JH7110_SYSCLK_CPU_ROOT			0
#define JH7110_SYSCLK_CPU_CORE			1
#define JH7110_SYSCLK_CPU_BUS			2
#define JH7110_SYSCLK_GPU_ROOT			3
#define JH7110_SYSCLK_PERH_ROOT			4
#define JH7110_SYSCLK_BUS_ROOT			5
#define JH7110_SYSCLK_NOCSTG_BUS		6
#define JH7110_SYSCLK_AXI_CFG0			7
#define JH7110_SYSCLK_STG_AXIAHB		8
#define JH7110_SYSCLK_AHB0			9
#define JH7110_SYSCLK_AHB1			10
#define JH7110_SYSCLK_APB_BUS			11
#define JH7110_SYSCLK_APB0			12
#define JH7110_SYSCLK_PLL0_DIV2			13
#define JH7110_SYSCLK_PLL1_DIV2			14
#define JH7110_SYSCLK_PLL2_DIV2			15
#define JH7110_SYSCLK_AUDIO_ROOT		16
#define JH7110_SYSCLK_MCLK_INNER		17
#define JH7110_SYSCLK_MCLK			18
#define JH7110_SYSCLK_MCLK_OUT			19
#define JH7110_SYSCLK_ISP_2X			20
#define JH7110_SYSCLK_ISP_AXI			21
#define JH7110_SYSCLK_GCLK0			22
#define JH7110_SYSCLK_GCLK1			23
#define JH7110_SYSCLK_GCLK2			24
#define JH7110_SYSCLK_CORE			25
#define JH7110_SYSCLK_CORE1			26
#define JH7110_SYSCLK_CORE2			27
#define JH7110_SYSCLK_CORE3			28
#define JH7110_SYSCLK_CORE4			29
#define JH7110_SYSCLK_DEBUG			30
#define JH7110_SYSCLK_RTC_TOGGLE		31
#define JH7110_SYSCLK_TRACE0			32
#define JH7110_SYSCLK_TRACE1			33
#define JH7110_SYSCLK_TRACE2			34
#define JH7110_SYSCLK_TRACE3			35
#define JH7110_SYSCLK_TRACE4			36
#define JH7110_SYSCLK_TRACE_COM			37
#define JH7110_SYSCLK_NOC_BUS_CPU_AXI		38
#define JH7110_SYSCLK_NOC_BUS_AXICFG0_AXI	39
#define JH7110_SYSCLK_OSC_DIV2			40
#define JH7110_SYSCLK_PLL1_DIV4			41
#define JH7110_SYSCLK_PLL1_DIV8			42
#define JH7110_SYSCLK_DDR_BUS			43
#define JH7110_SYSCLK_DDR_AXI			44
#define JH7110_SYSCLK_GPU_CORE			45
#define JH7110_SYSCLK_GPU_CORE_CLK		46
#define JH7110_SYSCLK_GPU_SYS_CLK		47
#define JH7110_SYSCLK_GPU_APB			48
#define JH7110_SYSCLK_GPU_RTC_TOGGLE		49
#define JH7110_SYSCLK_NOC_BUS_GPU_AXI		50
#define JH7110_SYSCLK_ISP_TOP_CORE		51
#define JH7110_SYSCLK_ISP_TOP_AXI		52
#define JH7110_SYSCLK_NOC_BUS_ISP_AXI		53
#define JH7110_SYSCLK_HIFI4_CORE		54
#define JH7110_SYSCLK_HIFI4_AXI			55
#define JH7110_SYSCLK_AXI_CFG1_MAIN		56
#define JH7110_SYSCLK_AXI_CFG1_AHB		57
#define JH7110_SYSCLK_VOUT_SRC			58
#define JH7110_SYSCLK_VOUT_AXI			59
#define JH7110_SYSCLK_NOC_BUS_DISP_AXI		60
#define JH7110_SYSCLK_VOUT_TOP_AHB		61
#define JH7110_SYSCLK_VOUT_TOP_AXI		62
#define JH7110_SYSCLK_VOUT_TOP_HDMITX0_MCLK	63
#define JH7110_SYSCLK_VOUT_TOP_MIPIPHY_REF	64
#define JH7110_SYSCLK_JPEGC_AXI			65
#define JH7110_SYSCLK_CODAJ12_AXI		66
#define JH7110_SYSCLK_CODAJ12_CORE		67
#define JH7110_SYSCLK_CODAJ12_APB		68
#define JH7110_SYSCLK_VDEC_AXI			69
#define JH7110_SYSCLK_WAVE511_AXI		70
#define JH7110_SYSCLK_WAVE511_BPU		71
#define JH7110_SYSCLK_WAVE511_VCE		72
#define JH7110_SYSCLK_WAVE511_APB		73
#define JH7110_SYSCLK_VDEC_JPG			74
#define JH7110_SYSCLK_VDEC_MAIN			75
#define JH7110_SYSCLK_NOC_BUS_VDEC_AXI		76
#define JH7110_SYSCLK_VENC_AXI			77
#define JH7110_SYSCLK_WAVE420L_AXI		78
#define JH7110_SYSCLK_WAVE420L_BPU		79
#define JH7110_SYSCLK_WAVE420L_VCE		80
#define JH7110_SYSCLK_WAVE420L_APB		81
#define JH7110_SYSCLK_NOC_BUS_VENC_AXI		82
#define JH7110_SYSCLK_AXI_CFG0_MAIN_DIV		83
#define JH7110_SYSCLK_AXI_CFG0_MAIN		84
#define JH7110_SYSCLK_AXI_CFG0_HIFI4		85
#define JH7110_SYSCLK_AXIMEM2_AXI		86
#define JH7110_SYSCLK_QSPI_AHB			87
#define JH7110_SYSCLK_QSPI_APB			88
#define JH7110_SYSCLK_QSPI_REF_SRC		89
#define JH7110_SYSCLK_QSPI_REF			90
#define JH7110_SYSCLK_SDIO0_AHB			91
#define JH7110_SYSCLK_SDIO1_AHB			92
#define JH7110_SYSCLK_SDIO0_SDCARD		93
#define JH7110_SYSCLK_SDIO1_SDCARD		94
#define JH7110_SYSCLK_USB_125M			95
#define JH7110_SYSCLK_NOC_BUS_STG_AXI		96
#define JH7110_SYSCLK_GMAC1_AHB			97
#define JH7110_SYSCLK_GMAC1_AXI			98
#define JH7110_SYSCLK_GMAC_SRC			99
#define JH7110_SYSCLK_GMAC1_GTXCLK		100
#define JH7110_SYSCLK_GMAC1_RMII_RTX		101
#define JH7110_SYSCLK_GMAC1_PTP			102
#define JH7110_SYSCLK_GMAC1_RX			103
#define JH7110_SYSCLK_GMAC1_RX_INV		104
#define JH7110_SYSCLK_GMAC1_TX			105
#define JH7110_SYSCLK_GMAC1_TX_INV		106
#define JH7110_SYSCLK_GMAC1_GTXC		107
#define JH7110_SYSCLK_GMAC0_GTXCLK		108
#define JH7110_SYSCLK_GMAC0_PTP			109
#define JH7110_SYSCLK_GMAC_PHY			110
#define JH7110_SYSCLK_GMAC0_GTXC		111
#define JH7110_SYSCLK_IOMUX_APB			112
#define JH7110_SYSCLK_MAILBOX_APB		113
#define JH7110_SYSCLK_INT_CTRL_APB		114
#define JH7110_SYSCLK_CAN0_APB			115
#define JH7110_SYSCLK_CAN0_TIMER		116
#define JH7110_SYSCLK_CAN0_CAN			117
#define JH7110_SYSCLK_CAN1_APB			118
#define JH7110_SYSCLK_CAN1_TIMER		119
#define JH7110_SYSCLK_CAN1_CAN			120
#define JH7110_SYSCLK_PWM_APB			121
#define JH7110_SYSCLK_WDT_APB			122
#define JH7110_SYSCLK_WDT_CORE			123
#define JH7110_SYSCLK_TIMER_APB			124
#define JH7110_SYSCLK_TIMER0			125
#define JH7110_SYSCLK_TIMER1			126
#define JH7110_SYSCLK_TIMER2			127
#define JH7110_SYSCLK_TIMER3			128
#define JH7110_SYSCLK_TEMP_APB			129
#define JH7110_SYSCLK_TEMP_CORE			130
#define JH7110_SYSCLK_SPI0_APB			131
#define JH7110_SYSCLK_SPI1_APB			132
#define JH7110_SYSCLK_SPI2_APB			133
#define JH7110_SYSCLK_SPI3_APB			134
#define JH7110_SYSCLK_SPI4_APB			135
#define JH7110_SYSCLK_SPI5_APB			136
#define JH7110_SYSCLK_SPI6_APB			137
#define JH7110_SYSCLK_I2C0_APB			138
#define JH7110_SYSCLK_I2C1_APB			139
#define JH7110_SYSCLK_I2C2_APB			140
#define JH7110_SYSCLK_I2C3_APB			141
#define JH7110_SYSCLK_I2C4_APB			142
#define JH7110_SYSCLK_I2C5_APB			143
#define JH7110_SYSCLK_I2C6_APB			144
#define JH7110_SYSCLK_UART0_APB			145
#define JH7110_SYSCLK_UART0_CORE		146
#define JH7110_SYSCLK_UART1_APB			147
#define JH7110_SYSCLK_UART1_CORE		148
#define JH7110_SYSCLK_UART2_APB			149
#define JH7110_SYSCLK_UART2_CORE		150
#define JH7110_SYSCLK_UART3_APB			151
#define JH7110_SYSCLK_UART3_CORE		152
#define JH7110_SYSCLK_UART4_APB			153
#define JH7110_SYSCLK_UART4_CORE		154
#define JH7110_SYSCLK_UART5_APB			155
#define JH7110_SYSCLK_UART5_CORE		156
#define JH7110_SYSCLK_PWMDAC_APB		157
#define JH7110_SYSCLK_PWMDAC_CORE		158
#define JH7110_SYSCLK_SPDIF_APB			159
#define JH7110_SYSCLK_SPDIF_CORE		160
#define JH7110_SYSCLK_I2STX0_APB		161
#define JH7110_SYSCLK_I2STX0_BCLK_MST		162
#define JH7110_SYSCLK_I2STX0_BCLK_MST_INV	163
#define JH7110_SYSCLK_I2STX0_LRCK_MST		164
#define JH7110_SYSCLK_I2STX0_BCLK		165
#define JH7110_SYSCLK_I2STX0_BCLK_INV		166
#define JH7110_SYSCLK_I2STX0_LRCK		167
#define JH7110_SYSCLK_I2STX1_APB		168
#define JH7110_SYSCLK_I2STX1_BCLK_MST		169
#define JH7110_SYSCLK_I2STX1_BCLK_MST_INV	170
#define JH7110_SYSCLK_I2STX1_LRCK_MST		171
#define JH7110_SYSCLK_I2STX1_BCLK		172
#define JH7110_SYSCLK_I2STX1_BCLK_INV		173
#define JH7110_SYSCLK_I2STX1_LRCK		174
#define JH7110_SYSCLK_I2SRX_APB			175
#define JH7110_SYSCLK_I2SRX_BCLK_MST		176
#define JH7110_SYSCLK_I2SRX_BCLK_MST_INV	177
#define JH7110_SYSCLK_I2SRX_LRCK_MST		178
#define JH7110_SYSCLK_I2SRX_BCLK		179
#define JH7110_SYSCLK_I2SRX_BCLK_INV		180
#define JH7110_SYSCLK_I2SRX_LRCK		181
#define JH7110_SYSCLK_PDM_DMIC			182
#define JH7110_SYSCLK_PDM_APB			183
#define JH7110_SYSCLK_TDM_AHB			184
#define JH7110_SYSCLK_TDM_APB			185
#define JH7110_SYSCLK_TDM_INTERNAL		186
#define JH7110_SYSCLK_TDM_TDM			187
#define JH7110_SYSCLK_TDM_TDM_INV		188
#define JH7110_SYSCLK_JTAG_CERTIFICATION_TRNG	189

#define JH7110_SYSCLK_NCLKS			190

/* external clocks */
#define JH7110_SYSCLK_PLL0_OUT			(JH7110_SYSCLK_NCLKS + 0)
#define JH7110_SYSCLK_PLL1_OUT			(JH7110_SYSCLK_NCLKS + 1)
#define JH7110_SYSCLK_PLL2_OUT			(JH7110_SYSCLK_NCLKS + 2)

/* AONCRG clocks */
#define JH7110_AONCLK_OSC_DIV4			0
#define JH7110_AONCLK_APB_FUNC			1
#define JH7110_AONCLK_GMAC0_AHB			2
#define JH7110_AONCLK_GMAC0_AXI			3
#define JH7110_AONCLK_GMAC0_RMII_RTX		4
#define JH7110_AONCLK_GMAC0_TX			5
#define JH7110_AONCLK_GMAC0_TX_INV		6
#define JH7110_AONCLK_GMAC0_RX			7
#define JH7110_AONCLK_GMAC0_RX_INV		8
#define JH7110_AONCLK_OTPC_APB			9
#define JH7110_AONCLK_RTC_APB			10
#define JH7110_AONCLK_RTC_INTERNAL		11
#define JH7110_AONCLK_RTC_32K			12
#define JH7110_AONCLK_RTC_CAL			13

#define JH7110_AONCLK_NCLKS			14

/* STGCRG clocks */
#define JH7110_STGCLK_HIFI4_CLK_CORE		0
#define JH7110_STGCLK_USB0_APB			1
#define JH7110_STGCLK_USB0_UTMI_APB		2
#define JH7110_STGCLK_USB0_AXI			3
#define JH7110_STGCLK_USB0_LPM			4
#define JH7110_STGCLK_USB0_STB			5
#define JH7110_STGCLK_USB0_APP_125		6
#define JH7110_STGCLK_USB0_REFCLK		7
#define JH7110_STGCLK_PCIE0_AXI_MST0		8
#define JH7110_STGCLK_PCIE0_APB			9
#define JH7110_STGCLK_PCIE0_TL			10
#define JH7110_STGCLK_PCIE1_AXI_MST0		11
#define JH7110_STGCLK_PCIE1_APB			12
#define JH7110_STGCLK_PCIE1_TL			13
#define JH7110_STGCLK_PCIE_SLV_MAIN		14
#define JH7110_STGCLK_SEC_AHB			15
#define JH7110_STGCLK_SEC_MISC_AHB		16
#define JH7110_STGCLK_GRP0_MAIN			17
#define JH7110_STGCLK_GRP0_BUS			18
#define JH7110_STGCLK_GRP0_STG			19
#define JH7110_STGCLK_GRP1_MAIN			20
#define JH7110_STGCLK_GRP1_BUS			21
#define JH7110_STGCLK_GRP1_STG			22
#define JH7110_STGCLK_GRP1_HIFI			23
#define JH7110_STGCLK_E2_RTC			24
#define JH7110_STGCLK_E2_CORE			25
#define JH7110_STGCLK_E2_DBG			26
#define JH7110_STGCLK_DMA1P_AXI			27
#define JH7110_STGCLK_DMA1P_AHB			28

#define JH7110_STGCLK_NCLKS			29

/* ISPCRG clocks */
#define JH7110_ISPCLK_DOM4_APB_FUNC		0
#define JH7110_ISPCLK_MIPI_RX0_PXL		1
#define JH7110_ISPCLK_DVP_INV			2
#define JH7110_ISPCLK_M31DPHY_CFG_IN		3
#define JH7110_ISPCLK_M31DPHY_REF_IN		4
#define JH7110_ISPCLK_M31DPHY_TX_ESC_LAN0	5
#define JH7110_ISPCLK_VIN_APB			6
#define JH7110_ISPCLK_VIN_SYS			7
#define JH7110_ISPCLK_VIN_PIXEL_IF0		8
#define JH7110_ISPCLK_VIN_PIXEL_IF1		9
#define JH7110_ISPCLK_VIN_PIXEL_IF2		10
#define JH7110_ISPCLK_VIN_PIXEL_IF3		11
#define JH7110_ISPCLK_VIN_P_AXI_WR		12
#define JH7110_ISPCLK_ISPV2_TOP_WRAPPER_C	13

#define JH7110_ISPCLK_NCLKS			14

/* VOUTCRG clocks */
#define JH7110_VOUTCLK_APB			0
#define JH7110_VOUTCLK_DC8200_PIX		1
#define JH7110_VOUTCLK_DSI_SYS			2
#define JH7110_VOUTCLK_TX_ESC			3
#define JH7110_VOUTCLK_DC8200_AXI		4
#define JH7110_VOUTCLK_DC8200_CORE		5
#define JH7110_VOUTCLK_DC8200_AHB		6
#define JH7110_VOUTCLK_DC8200_PIX0		7
#define JH7110_VOUTCLK_DC8200_PIX1		8
#define JH7110_VOUTCLK_DOM_VOUT_TOP_LCD		9
#define JH7110_VOUTCLK_DSITX_APB		10
#define JH7110_VOUTCLK_DSITX_SYS		11
#define JH7110_VOUTCLK_DSITX_DPI		12
#define JH7110_VOUTCLK_DSITX_TXESC		13
#define JH7110_VOUTCLK_MIPITX_DPHY_TXESC	14
#define JH7110_VOUTCLK_HDMI_TX_MCLK		15
#define JH7110_VOUTCLK_HDMI_TX_BCLK		16
#define JH7110_VOUTCLK_HDMI_TX_SYS		17

#define JH7110_VOUTCLK_NCLKS			18

static const char *cpu_root_parents[] = {
	"osc", "pll0_out"
};

static const char *gpu_root_parents[] = {
	"pll2_out", "pll1_out",
};

static const char *bus_root_parents[] = {
	"osc", "pll2_out",
};

static const char *mclk_parents[] = {
	"mclk_inner", "mclk_ext"
};

static const char *ddr_bus_parents[] = {
	"osc_div2", "pll1_div2", "pll1_div4", "pll1_div8"
};

static const char *qspi_ref_parents[] = {
	"osc", "qspi_ref_src",
};

static const char *isp_2x_parents[] = {
	"pll2_out", "pll1_out"
};

static const char *i2stx0_lrck_parents[] = {
	"i2stx0_lrck_mst", "i2stx_lrck_ext",
};

static const char *i2stx0_bclk_parents[] = {
	"i2stx0_bclk_mst", "i2stx_bclk_ext",
};

static const char *i2stx1_bclk_parents[] = {
	"i2stx1_bclk_mst", "i2stx_bclk_ext",
};

static const char *gmac1_rx_parents[] = {
	"gmac1_rgmii_rxin", "gmac1_rmii_rtx",
};

static const char *i2srx_bclk_root_parents[] = {
	"i2srx_bclk_mst", "i2srx_bclk_ext",
};

static const char *i2srx_lrck_parents[] = {
	"i2srx_lrck_mst", "i2srx_lrck_ext",
};

static const char *tdm_tdm_parents[] = {
	"tdm_internal", "tdm_ext",
};

static const char *i2stx1_lrck_parents[] = {
	"i2stx1_lrck_mst", "i2stx_lrck_ext",
};

static const char *i2stx1_lrck_mst_parents[] = {
	"i2stx1_bclk_mst_inv", "i2stx1_bclk_mst",
};

static const char *gmac1_tx_parents[] = {
	"gmac1_gtxclk", "gmac1_rmii_rtx",
};

static const char *perh_root_parents[] = {
	"pll0_out", "pll2_out",
};

static const char *i2stx0_lrck_mst_parents[] = {
	"i2stx0_bclk_mst_inv", "i2stx0_bclk_mst"
};

static const char *i2srx_lrck_mst_parents[] = {
	"i2srx_bclk_mst_inv", "i2srx_bclk_mst",
};

static struct jh71x0_clkc_clk jh7110_sysclk_clocks[] = {
	JH71X0CLKC_FIXED_FACTOR(JH7110_SYSCLK_PLL0_OUT,	"pll0_out",	"osc",	 3, 125),
	JH71X0CLKC_FIXED_FACTOR(JH7110_SYSCLK_PLL1_OUT,	"pll1_out",	"osc",	12, 533),
	JH71X0CLKC_FIXED_FACTOR(JH7110_SYSCLK_PLL2_OUT,	"pll2_out",	"osc",	 2,  99),

	JH71X0CLKC_MUX(JH7110_SYSCLK_CPU_ROOT, "cpu_root", cpu_root_parents),
	JH71X0CLKC_MUX(JH7110_SYSCLK_GPU_ROOT, "gpu_root", gpu_root_parents),
	JH71X0CLKC_MUX(JH7110_SYSCLK_BUS_ROOT, "bus_root", bus_root_parents),

	JH71X0CLKC_DIV(JH7110_SYSCLK_CPU_CORE, "cpu_core", 7, "cpu_root"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_CPU_BUS,  "cpu_bus", 2, "cpu_core"),

	JH71X0CLKC_MUXDIV(JH7110_SYSCLK_PERH_ROOT, "perh_root", 2, perh_root_parents),

	JH71X0CLKC_DIV(JH7110_SYSCLK_NOCSTG_BUS, "nocstg_bus", 3, "bus_root"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_AXI_CFG0, "axi_cfg0", 3, "bus_root"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_STG_AXIAHB, "stg_axiahb", 2, "axi_cfg0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_AHB0, "ahb0", "stg_axiahb"), // CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_AHB1, "ahb1", "stg_axiahb"),// CLK_IS_CRITICAL,
	JH71X0CLKC_DIV(JH7110_SYSCLK_APB_BUS, "apb_bus", 8, "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_APB0, "apb0", "apb_bus"),// CLK_IS_CRITICAL,
	JH71X0CLKC_DIV(JH7110_SYSCLK_PLL0_DIV2, "pll0_div2", 2, "pll0_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_PLL1_DIV2, "pll1_div2", 2, "pll1_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_PLL2_DIV2, "pll2_div2", 2, "pll2_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_AUDIO_ROOT, "audio_root", 8, "pll2_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_MCLK_INNER, "mclk_inner", 64, "audio_root"),

	JH71X0CLKC_MUX(JH7110_SYSCLK_MCLK, "mclk", mclk_parents),

	JH71X0CLKC_GATE(JH7110_SYSCLK_MCLK_OUT, "mclk_out", "mclk_inner"),

	JH71X0CLKC_MUXDIV(JH7110_SYSCLK_ISP_2X, "isp_2x", 8, isp_2x_parents),

	JH71X0CLKC_DIV(JH7110_SYSCLK_ISP_AXI, "isp_axi", 4, "isp_2x"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GCLK0, "gclk0", 62, "pll0_div2"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GCLK1, "gclk1",62, "pll1_div2"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GCLK2, "gclk2",62, "pll2_div2"),
	/* cores */
	JH71X0CLKC_GATE(JH7110_SYSCLK_CORE, "core", "cpu_core"),// CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_CORE1, "core1", "cpu_core"),// CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_CORE2, "core2", "cpu_core"),// CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_CORE3, "core3", "cpu_core"),// CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_CORE4, "core4", "cpu_core"),// CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_DEBUG, "debug", "cpu_bus"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_RTC_TOGGLE, "rtc_toggle", 6, "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE0, "trace0", "cpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE1, "trace1", "cpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE2, "trace2", "cpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE3, "trace3", "cpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE4, "trace4", "cpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TRACE_COM, "trace_com", "cpu_bus"),
	/* noc */
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_CPU_AXI, "noc_bus_cpu_axi", "cpu_bus"), // CLK_IS_CRITICAL,
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_AXICFG0_AXI, "noc_bus_axicfg0_axi", "axi_cfg0"),// CLK_IS_CRITICAL,
	/* ddr */
	JH71X0CLKC_DIV(JH7110_SYSCLK_OSC_DIV2, "osc_div2", 2, "osc"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_PLL1_DIV4, "pll1_div4", 2, "pll1_div2"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_PLL1_DIV8, "pll1_div8", 2, "pll1_div4"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_DDR_BUS, "ddr_bus", ddr_bus_parents),

	JH71X0CLKC_GATE(JH7110_SYSCLK_DDR_AXI, "ddr_axi", "ddr_bus"),// CLK_IS_CRITICAL,
	/* gpu */
	JH71X0CLKC_DIV(JH7110_SYSCLK_GPU_CORE, "gpu_core", 7, "gpu_root"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GPU_CORE_CLK, "gpu_core_clk", "gpu_core"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GPU_SYS_CLK, "gpu_sys_clk", "isp_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GPU_APB, "gpu_apb", "apb_bus"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GPU_RTC_TOGGLE, "gpu_rtc_toggle", 12, "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_GPU_AXI, "noc_bus_gpu_axi", "gpu_core"),
	/* isp */
	JH71X0CLKC_GATE(JH7110_SYSCLK_ISP_TOP_CORE, "isp_top_core", "isp_2x"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_ISP_TOP_AXI, "isp_top_axi", "isp_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_ISP_AXI, "noc_bus_isp_axi", "isp_axi"), // CLK_IS_CRITICAL,
	/* hifi4 */
	JH71X0CLKC_DIV(JH7110_SYSCLK_HIFI4_CORE, "hifi4_core", 15, "bus_root"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_HIFI4_AXI, "hifi4_axi", 2, "hifi4_core"),
	/* axi_cfg1 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXI_CFG1_MAIN, "axi_cfg1_main", "isp_axi"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXI_CFG1_AHB, "axi_cfg1_ahb", "ahb0"), // CLK_IS_CRITICAL
	/* vout */
	JH71X0CLKC_GATE(JH7110_SYSCLK_VOUT_SRC, "vout_src", "pll2_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_VOUT_AXI, "vout_axi", 7, "pll2_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_DISP_AXI, "noc_bus_disp_axi", "vout_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_VOUT_TOP_AHB, "vout_top_ahb", "ahb1"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_VOUT_TOP_AXI, "vout_top_axi", "vout_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_VOUT_TOP_HDMITX0_MCLK, "vout_top_hdmitx0_mclk", "mclk_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_VOUT_TOP_MIPIPHY_REF, "vout_top_mipiphy_ref", 2, "osc"),
	/* jpegc */
	JH71X0CLKC_DIV(JH7110_SYSCLK_JPEGC_AXI, "jpegc_axi", 16, "pll2_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_CODAJ12_AXI, "codaj12_axi", "jpegc_axi"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_CODAJ12_CORE, "codaj12_core",16, "pll2_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_CODAJ12_APB, "codaj12_apb", "apb_bus"),
	/* vdec */
	JH71X0CLKC_DIV(JH7110_SYSCLK_VDEC_AXI, "vdec_axi", 7, "bus_root"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_WAVE511_AXI, "wave511_axi", "vdec_axi"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_WAVE511_BPU, "wave511_bpu",7, "bus_root"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_WAVE511_VCE, "wave511_vce", 7, "pll0_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_WAVE511_APB, "wave511_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_VDEC_JPG, "vdec_jpg", "jpegc_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_VDEC_MAIN, "vdec_main", "vdec_axi"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_VDEC_AXI, "noc_bus_vdec_axi", "vdec_axi"),
	/* venc */
	JH71X0CLKC_DIV(JH7110_SYSCLK_VENC_AXI, "venc_axi", 15, "pll2_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_WAVE420L_AXI, "wave420l_axi", "venc_axi"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_WAVE420L_BPU, "wave420l_bpu",15, "pll2_out"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_WAVE420L_VCE, "wave420l_vce",15, "pll2_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_WAVE420L_APB, "wave420l_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_VENC_AXI, "noc_bus_venc_axi", "venc_axi"),
	/* axi_cfg0 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXI_CFG0_MAIN_DIV, "axi_cfg0_main_div", "ahb1"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXI_CFG0_MAIN, "axi_cfg0_main", "axi_cfg0"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXI_CFG0_HIFI4, "axi_cfg0_hifi4", "hifi4_axi"), // CLK_IS_CRITICAL,
	/* intmem */
	JH71X0CLKC_GATE(JH7110_SYSCLK_AXIMEM2_AXI, "aximem2_axi", "axi_cfg0"),
	/* qspi */
	JH71X0CLKC_GATE(JH7110_SYSCLK_QSPI_AHB, "qspi_ahb", "ahb1"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_QSPI_APB, "qspi_apb", "apb_bus"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_QSPI_REF_SRC, "qspi_ref_src", 16, "pll0_out"),
	JH71X0CLKC_MUXGATE(JH7110_SYSCLK_QSPI_REF, "qspi_ref", qspi_ref_parents),

	/* sdio */
	JH71X0CLKC_GATE(JH7110_SYSCLK_SDIO0_AHB, "sdio0_ahb", "ahb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SDIO1_AHB, "sdio1_ahb", "ahb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_SDIO0_SDCARD, "sdio0_sdcard", 15, "axi_cfg0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_SDIO1_SDCARD, "sdio1_sdcard", 15, "axi_cfg0"),
	/* stg */
	JH71X0CLKC_DIV(JH7110_SYSCLK_USB_125M, "usb_125m", 15, "pll0_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_NOC_BUS_STG_AXI, "noc_bus_stg_axi", "nocstg_bus"), // CLK_IS_CRITICAL,
	/* gmac1 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_GMAC1_AHB, "gmac1_ahb", "ahb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GMAC1_AXI, "gmac1_axi", "stg_axiahb"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_GMAC_SRC, "gmac_src", 7, "pll0_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_GMAC1_GTXCLK, "gmac1_gtxclk", 15, "pll0_out"),
	JH71X0CLKC_DIV(JH7110_SYSCLK_GMAC1_RMII_RTX, "gmac1_rmii_rtx", 30, "gmac1_rmii_refin"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GMAC1_PTP, "gmac1_ptp",31, "gmac_src"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_GMAC1_RX, "gmac1_rx", gmac1_rx_parents),

	JH71X0CLKC_INV(JH7110_SYSCLK_GMAC1_RX_INV, "gmac1_rx_inv", "gmac1_rx"),
	JH71X0CLKC_MUXGATE_FLAGS(JH7110_SYSCLK_GMAC1_TX, "gmac1_tx", gmac1_tx_parents, CLK_SET_RATE_PARENT), // CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
	JH71X0CLKC_INV(JH7110_SYSCLK_GMAC1_TX_INV, "gmac1_tx_inv", "gmac1_tx"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GMAC1_GTXC, "gmac1_gtxc", "gmac1_gtxclk"),
	/* gmac0 */
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GMAC0_GTXCLK, "gmac0_gtxclk",15, "pll0_out"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GMAC0_PTP, "gmac0_ptp",31, "gmac_src"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_GMAC_PHY, "gmac_phy",31, "gmac_src"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_GMAC0_GTXC, "gmac0_gtxc", "gmac0_gtxclk"),
	/* apb misc */
	JH71X0CLKC_GATE(JH7110_SYSCLK_IOMUX_APB, "iomux_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_MAILBOX_APB, "mailbox_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_INT_CTRL_APB, "int_ctrl_apb", "apb_bus"),
	/* can0 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_CAN0_APB, "can0_apb", "apb_bus"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_CAN0_TIMER, "can0_timer",24, "osc"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_CAN0_CAN, "can0_can",63, "perh_root"),
	/* can1 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_CAN1_APB, "can1_apb", "apb_bus"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_CAN1_TIMER, "can1_timer",24, "osc"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_CAN1_CAN, "can1_can",63, "perh_root"),
	/* pwm */
	JH71X0CLKC_GATE(JH7110_SYSCLK_PWM_APB, "pwm_apb", "apb_bus"),
	/* wdt */
	JH71X0CLKC_GATE(JH7110_SYSCLK_WDT_APB, "wdt_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_WDT_CORE, "wdt_core", "osc"),
	/* timer */
	JH71X0CLKC_GATE(JH7110_SYSCLK_TIMER_APB, "timer_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TIMER0, "timer0", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TIMER1, "timer1", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TIMER2, "timer2", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TIMER3, "timer3", "osc"),
	/* temp sensor */
	JH71X0CLKC_GATE(JH7110_SYSCLK_TEMP_APB, "temp_apb", "apb_bus"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_TEMP_CORE, "temp_core",24, "osc"),
	/* spi */
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI0_APB, "spi0_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI1_APB, "spi1_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI2_APB, "spi2_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI3_APB, "spi3_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI4_APB, "spi4_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI5_APB, "spi5_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPI6_APB, "spi6_apb", "apb_bus"),
	/* i2c */
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C0_APB, "i2c0_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C1_APB, "i2c1_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C2_APB, "i2c2_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C3_APB, "i2c3_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C4_APB, "i2c4_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C5_APB, "i2c5_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2C6_APB, "i2c6_apb", "apb_bus"),
	/* uart */
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART0_APB, "uart0_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART0_CORE, "uart0_core", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART1_APB, "uart1_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART1_CORE, "uart1_core", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART2_APB, "uart2_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART2_CORE, "uart2_core", "osc"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART3_APB, "uart3_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_UART3_CORE, "uart3_core",10, "perh_root"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART4_APB, "uart4_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_UART4_CORE, "uart4_core",10, "perh_root"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_UART5_APB, "uart5_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_UART5_CORE, "uart5_core",10, "perh_root"),
	/* pwmdac */
	JH71X0CLKC_GATE(JH7110_SYSCLK_PWMDAC_APB, "pwmdac_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_PWMDAC_CORE, "pwmdac_core",256, "audio_root"),
	/* spdif */
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPDIF_APB, "spdif_apb", "apb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_SPDIF_CORE, "spdif_core", "mclk_out"),
	/* i2stx0 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2STX0_APB, "i2stx0_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_I2STX0_BCLK_MST, "i2stx0_bclk_mst",32, "mclk_out"),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2STX0_BCLK_MST_INV, "i2stx0_bclk_mst_inv", "i2stx0_bclk_mst"),

	JH71X0CLKC_MUXDIV(JH7110_SYSCLK_I2STX0_LRCK_MST, "i2stx0_lrck_mst", 64, i2stx0_lrck_mst_parents),

	JH71X0CLKC_MUX(JH7110_SYSCLK_I2STX0_BCLK, "i2stx0_bclk", i2stx0_bclk_parents),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2STX0_BCLK_INV, "i2stx0_bclk_inv", "i2stx0_bclk"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_I2STX0_LRCK, "i2stx0_lrck", i2stx0_lrck_parents),
	/* i2stx1 */
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2STX1_APB, "i2stx1_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_I2STX1_BCLK_MST, "i2stx1_bclk_mst",32, "mclk_out"),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2STX1_BCLK_MST_INV, "i2stx1_bclk_mst_inv", "i2stx1_bclk_mst"),

	JH71X0CLKC_MUXDIV(JH7110_SYSCLK_I2STX1_LRCK_MST, "i2stx1_lrck_mst", 64, i2stx1_lrck_mst_parents),

	JH71X0CLKC_MUX(JH7110_SYSCLK_I2STX1_BCLK, "i2stx1_bclk", i2stx1_bclk_parents),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2STX1_BCLK_INV, "i2stx1_bclk_inv", "i2stx1_bclk"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_I2STX1_LRCK, "i2stx1_lrck", i2stx1_lrck_parents),
	/* i2srx */
	JH71X0CLKC_GATE(JH7110_SYSCLK_I2SRX_APB, "i2srx_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_I2SRX_BCLK_MST, "i2srx_bclk_mst", 32, "mclk_out"),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2SRX_BCLK_MST_INV, "i2srx_bclk_mst_inv", "i2srx_bclk_mst"),

	JH71X0CLKC_MUXDIV(JH7110_SYSCLK_I2SRX_LRCK_MST, "i2srx_lrck_mst", 64, i2srx_lrck_mst_parents),

	JH71X0CLKC_MUX(JH7110_SYSCLK_I2SRX_BCLK, "i2srx_bclk", i2srx_bclk_root_parents),
	JH71X0CLKC_INV(JH7110_SYSCLK_I2SRX_BCLK_INV, "i2srx_bclk_inv", "i2srx_bclk"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_I2SRX_LRCK, "i2srx_lrck", i2srx_lrck_parents),
	/* pdm */
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_PDM_DMIC, "pdm_dmic",64, "mclk_out"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_PDM_APB, "pdm_apb", "apb0"),
	/* tdm */
	JH71X0CLKC_GATE(JH7110_SYSCLK_TDM_AHB, "tdm_ahb", "ahb0"),
	JH71X0CLKC_GATE(JH7110_SYSCLK_TDM_APB, "tdm_apb", "apb0"),
	JH71X0CLKC_GATEDIV(JH7110_SYSCLK_TDM_INTERNAL, "tdm_internal",64, "mclk_out"),
	JH71X0CLKC_MUX(JH7110_SYSCLK_TDM_TDM, "tdm_tdm", tdm_tdm_parents),
	JH71X0CLKC_INV(JH7110_SYSCLK_TDM_TDM_INV, "tdm_tdm_inv", "tdm_tdm"),
	/* jtag */
	JH71X0CLKC_DIV(JH7110_SYSCLK_JTAG_CERTIFICATION_TRNG, "jtag_certification_trng", 4, "osc"),
};

static const char *apb_func_parents[] = {
	"osc_div4", "osc",
};

static const char *gmac0_tx_parents[] = {
	"gmac0_gtxclk", "gmac0_rmii_rtx"
};

static const char *gmac0_rx_parents[] = {
	"gmac0_rgmii_rxin", "gmac0_rmii_rtx",
};

static const char *rtc_32k_parents[] = {
	"rtc_osc", "rtc_internal",
};

static struct jh71x0_clkc_clk jh7110_aonclk_clocks[] = {
	/* source */
	JH71X0CLKC_DIV(JH7110_AONCLK_OSC_DIV4, "osc_div4", 4, "osc"),
	JH71X0CLKC_MUX(JH7110_AONCLK_APB_FUNC, "apb_func", apb_func_parents),
	/* gmac0 */
	JH71X0CLKC_GATE(JH7110_AONCLK_GMAC0_AHB, "gmac0_ahb", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_AONCLK_GMAC0_AXI, "gmac0_axi", "stg_axiahb"),
	JH71X0CLKC_DIV(JH7110_AONCLK_GMAC0_RMII_RTX, "gmac0_rmii_rtx", 30, "gmac0_rmii_refin"),
	JH71X0CLKC_MUXGATE(JH7110_AONCLK_GMAC0_TX, "gmac0_tx",gmac0_tx_parents),
	JH71X0CLKC_INV(JH7110_AONCLK_GMAC0_TX_INV, "gmac0_tx_inv", "gmac0_tx"),
	JH71X0CLKC_MUX_FLAGS(JH7110_AONCLK_GMAC0_RX, "gmac0_rx", gmac0_rx_parents, CLK_SET_RATE_PARENT), // CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT
	JH71X0CLKC_INV(JH7110_AONCLK_GMAC0_RX_INV, "gmac0_rx_inv", "gmac0_rx"),
	/* otpc */
	JH71X0CLKC_GATE(JH7110_AONCLK_OTPC_APB, "otpc_apb", "apb_bus"),
	/* rtc */
	JH71X0CLKC_GATE(JH7110_AONCLK_RTC_APB, "rtc_apb", "apb_bus"),
	JH71X0CLKC_DIV(JH7110_AONCLK_RTC_INTERNAL, "rtc_internal", 1022, "osc"),
	JH71X0CLKC_MUX(JH7110_AONCLK_RTC_32K, "rtc_32k", rtc_32k_parents),
	JH71X0CLKC_GATE(JH7110_AONCLK_RTC_CAL, "rtc_cal", "osc"),
};


static struct jh71x0_clkc_clk jh7110_stgclk_clocks[] = {
	/* hifi4 */
	JH71X0CLKC_GATE(JH7110_STGCLK_HIFI4_CLK_CORE, "hifi4_clk_core", "hifi4_core"),
	/* usb */
	JH71X0CLKC_GATE(JH7110_STGCLK_USB0_APB, "usb0_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_STGCLK_USB0_UTMI_APB, "usb0_utmi_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_STGCLK_USB0_AXI, "usb0_axi", "stg_axiahb"),
	JH71X0CLKC_GATEDIV(JH7110_STGCLK_USB0_LPM, "usb0_lpm", 2, "osc"),
	JH71X0CLKC_GATEDIV(JH7110_STGCLK_USB0_STB, "usb0_stb", 4, "osc"),
	JH71X0CLKC_GATE(JH7110_STGCLK_USB0_APP_125, "usb0_app_125", "usb_125m"),
	JH71X0CLKC_DIV(JH7110_STGCLK_USB0_REFCLK, "usb0_refclk", 2, "osc"),
	/* pci-e */
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE0_AXI_MST0, "pcie0_axi_mst0", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE0_APB, "pcie0_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE0_TL, "pcie0_tl", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE1_AXI_MST0, "pcie1_axi_mst0", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE1_APB, "pcie1_apb", "apb_bus"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE1_TL, "pcie1_tl", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_PCIE_SLV_MAIN, "pcie_slv_main", "stg_axiahb"), // CLK_IS_CRITICAL
	/* security */
	JH71X0CLKC_GATE(JH7110_STGCLK_SEC_AHB, "sec_ahb", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_SEC_MISC_AHB, "sec_misc_ahb", "stg_axiahb"),
	/* stg mtrx */
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP0_MAIN, "mtrx_grp0_main", "cpu_bus"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP0_BUS, "mtrx_grp0_bus", "nocstg_bus"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP0_STG, "mtrx_grp0_stg", "stg_axiahb"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP1_MAIN, "mtrx_grp1_main", "cpu_bus"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP1_BUS, "mtrx_grp1_bus", "nocstg_bus"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP1_STG, "mtrx_grp1_stg", "stg_axiahb"), // CLK_IS_CRITICAL
	JH71X0CLKC_GATE(JH7110_STGCLK_GRP1_HIFI, "mtrx_grp1_hifi", "hifi4_axi"), // CLK_IS_CRITICAL
	/* e24_rvpi */
	JH71X0CLKC_GATEDIV(JH7110_STGCLK_E2_RTC, "e2_rtc", 24, "osc"),
	JH71X0CLKC_GATE(JH7110_STGCLK_E2_CORE, "e2_core", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_E2_DBG, "e2_dbg", "stg_axiahb"),
	/* dw_sgdma1p */
	JH71X0CLKC_GATE(JH7110_STGCLK_DMA1P_AXI, "dma1p_axi", "stg_axiahb"),
	JH71X0CLKC_GATE(JH7110_STGCLK_DMA1P_AHB, "dma1p_ahb", "stg_axiahb"),
};

static const char *vin_p_axi_wr_parents[] = {
	"mipi_rx0_pxl", "dvp_inv",
};

static const char *ispv2_top_wrapper_c_parents[] = {
	"mipi_rx0_pxl", "dvp_inv",
};

static struct jh71x0_clkc_clk jh7110_ispclk_clocks[] = {
	/* syscon */
	JH71X0CLKC_DIV(JH7110_ISPCLK_DOM4_APB_FUNC, "dom4_apb_func", 15, "isp_top_axi"),
	JH71X0CLKC_DIV(JH7110_ISPCLK_MIPI_RX0_PXL, "mipi_rx0_pxl", 8, "isp_top_core"),
	JH71X0CLKC_INV(JH7110_ISPCLK_DVP_INV, "dvp_inv", "dvp_clk"),
	/* vin */
	JH71X0CLKC_DIV(JH7110_ISPCLK_M31DPHY_CFG_IN, "m31dphy_cfg_in", 16, "isp_top_core"),
	JH71X0CLKC_DIV(JH7110_ISPCLK_M31DPHY_REF_IN, "m31dphy_ref_in", 16, "isp_top_core"),
	JH71X0CLKC_DIV(JH7110_ISPCLK_M31DPHY_TX_ESC_LAN0, "m31dphy_tx_esc_lan0", 60, "isp_top_core"),
	JH71X0CLKC_GATE(JH7110_ISPCLK_VIN_APB, "vin_apb", "dom4_apb_func"),
	JH71X0CLKC_DIV(JH7110_ISPCLK_VIN_SYS, "vin_sys", 8, "isp_top_core"),
	JH71X0CLKC_GATE(JH7110_ISPCLK_VIN_PIXEL_IF0, "vin_pixel_if0", "mipi_rx0_pxl"),
	JH71X0CLKC_GATE(JH7110_ISPCLK_VIN_PIXEL_IF1, "vin_pixel_if1", "mipi_rx0_pxl"),
	JH71X0CLKC_GATE(JH7110_ISPCLK_VIN_PIXEL_IF2, "vin_pixel_if2", "mipi_rx0_pxl"),
	JH71X0CLKC_GATE(JH7110_ISPCLK_VIN_PIXEL_IF3, "vin_pixel_if3", "mipi_rx0_pxl"),
	JH71X0CLKC_MUX(JH7110_ISPCLK_VIN_P_AXI_WR, "vin_p_axi_wr", vin_p_axi_wr_parents),
	/* ispv2_top_wrapper */
	JH71X0CLKC_MUXGATE(JH7110_ISPCLK_ISPV2_TOP_WRAPPER_C, "ispv2_top_wrapper_c", ispv2_top_wrapper_c_parents),
};

static const char *dc8200_pix0_parents[] = {
	"dc8200_pix", "hdmitx0_pixelclk",
};

static const char *dc8200_pix1_parents[] = {
	"dc8200_pix", "hdmitx0_pixelclk",
};

static const char *dsiTx_dpi_parents[] = {
	"dc8200_pix", "hdmitx0_pixelclk",
};

static const char *dom_vout_top_lcd_parents[] = {
	"dc8200_pix0", "dc8200_pix1",
};

static struct jh71x0_clkc_clk jh7110_voutclk_clocks[] = {
	/* divider */
	JH71X0CLKC_DIV(JH7110_VOUTCLK_APB, "apb", 8, "vout_top_ahb"),
	JH71X0CLKC_DIV(JH7110_VOUTCLK_DC8200_PIX, "dc8200_pix", 63, "vout_src"),
	JH71X0CLKC_DIV(JH7110_VOUTCLK_DSI_SYS, "dsi_sys", 31, "vout_src"),
	JH71X0CLKC_DIV(JH7110_VOUTCLK_TX_ESC, "tx_esc", 31, "vout_top_ahb"),
	/* dc8200 */
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DC8200_AXI, "dc8200_axi", "vout_top_axi"),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DC8200_CORE, "dc8200_core", "vout_top_axi"),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DC8200_AHB, "dc8200_ahb", "vout_top_ahb"),
	JH71X0CLKC_MUXGATE(JH7110_VOUTCLK_DC8200_PIX0, "dc8200_pix0", dc8200_pix0_parents),
	JH71X0CLKC_MUXGATE(JH7110_VOUTCLK_DC8200_PIX1, "dc8200_pix1", dc8200_pix1_parents),
	/* LCD */
	JH71X0CLKC_MUXGATE(JH7110_VOUTCLK_DOM_VOUT_TOP_LCD, "dom_vout_top_lcd", dom_vout_top_lcd_parents),
	/* dsiTx */
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DSITX_APB, "dsiTx_apb", "dsi_sys"),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DSITX_SYS, "dsiTx_sys", "dsi_sys"),
	JH71X0CLKC_MUXGATE(JH7110_VOUTCLK_DSITX_DPI, "dsiTx_dpi", dsiTx_dpi_parents),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_DSITX_TXESC, "dsiTx_txesc", "tx_esc"),
	/* mipitx DPHY */
	JH71X0CLKC_GATE(JH7110_VOUTCLK_MIPITX_DPHY_TXESC, "mipitx_dphy_txesc", "tx_esc"),
	/* hdmi */
	JH71X0CLKC_GATE(JH7110_VOUTCLK_HDMI_TX_MCLK, "hdmi_tx_mclk", "vout_top_hdmitx0_mclk"),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_HDMI_TX_BCLK, "hdmi_tx_bclk", "i2stx0_bclk"),
	JH71X0CLKC_GATE(JH7110_VOUTCLK_HDMI_TX_SYS, "hdmi_tx_sys", "apb"),
};

struct jh7110_clock_config {
	struct jh71x0_clkc_clk *jhcc_clocks;
	size_t jhcc_nclks;
};

static struct jh7110_clock_config jh7110_aonclk_config = {
	.jhcc_clocks = jh7110_aonclk_clocks,
	.jhcc_nclks = __arraycount(jh7110_aonclk_clocks),
};

static struct jh7110_clock_config jh7110_ispclk_config = {
	.jhcc_clocks = jh7110_ispclk_clocks,
	.jhcc_nclks = __arraycount(jh7110_ispclk_clocks),
};

static struct jh7110_clock_config jh7110_stgclk_config = {
	.jhcc_clocks = jh7110_stgclk_clocks,
	.jhcc_nclks = __arraycount(jh7110_stgclk_clocks),
};

static struct jh7110_clock_config jh7110_sysclk_config = {
	.jhcc_clocks = jh7110_sysclk_clocks,
	.jhcc_nclks = __arraycount(jh7110_sysclk_clocks),
};

static struct jh7110_clock_config jh7110_voutclk_config = {
	.jhcc_clocks = jh7110_voutclk_clocks,
	.jhcc_nclks = __arraycount(jh7110_voutclk_clocks),
};


#define JH7110_SYSRST_ASSERT			0x2f8
#define JH7110_SYSRST_STATUS			0x308
#define JH7110_SYSRST_NRESETS			126

#define JH7110_AONRST_ASSERT			0x38
#define JH7110_AONRST_STATUS			0x3c
#define JH7110_AONRST_NRESETS			8

#define JH7110_STGRST_ASSERT			0x74
#define JH7110_STGRST_STATUS			0x78
#define JH7110_STGRST_NRESETS			23

#define JH7110_ISPRST_ASSERT			0x38
#define JH7110_ISPRST_STATUS			0x3c
#define JH7110_ISPRST_NRESETS			12

#define JH7110_VOUTRST_ASSERT			0x48
#define JH7110_VOUTRST_STATUS			0x4c
#define JH7110_VOUTRST_NRESETS			12

struct jh7110_reset_config {
	size_t jhcr_nresets;
	bus_size_t jhcr_assert;
	bus_size_t jhcr_status;
};

static struct jh7110_reset_config jh7110_sysrst_config = {
	.jhcr_nresets = JH7110_SYSRST_NRESETS,
	.jhcr_assert = JH7110_SYSRST_ASSERT,
	.jhcr_status = JH7110_SYSRST_STATUS,
};

static struct jh7110_reset_config jh7110_aonrst_config = {
	.jhcr_nresets = JH7110_AONRST_NRESETS,
	.jhcr_assert = JH7110_AONRST_ASSERT,
	.jhcr_status = JH7110_AONRST_STATUS,
};

static struct jh7110_reset_config jh7110_stgrst_config = {
	.jhcr_nresets = JH7110_STGRST_NRESETS,
	.jhcr_assert = JH7110_STGRST_ASSERT,
	.jhcr_status = JH7110_STGRST_STATUS,
};

static struct jh7110_reset_config jh7110_isprst_config = {
	.jhcr_nresets = JH7110_ISPRST_NRESETS,
	.jhcr_assert = JH7110_ISPRST_ASSERT,
	.jhcr_status = JH7110_ISPRST_STATUS,
};

static struct jh7110_reset_config jh7110_voutrst_config = {
	.jhcr_nresets = JH7110_VOUTRST_NRESETS,
	.jhcr_assert = JH7110_VOUTRST_ASSERT,
	.jhcr_status = JH7110_VOUTRST_STATUS,
};

struct jh7110_crg {
	const char *jhc_name;
	struct jh7110_clock_config *jhc_clk;
	struct jh7110_reset_config *jhc_rst;
	bool jhc_debug;
};


static struct jh7110_crg jh7110_sys_config = {
	.jhc_name = "System",
	.jhc_clk = &jh7110_sysclk_config,
	.jhc_rst = &jh7110_sysrst_config,
	.jhc_debug = true,
};


static struct jh7110_crg jh7110_aon_config = {
	.jhc_name = "Always-On",
	.jhc_clk = &jh7110_aonclk_config,
	.jhc_rst = &jh7110_aonrst_config,
	.jhc_debug = true,
};

static struct jh7110_crg jh7110_isp_config = {
	.jhc_name = "Image-Signal-Process",
	.jhc_clk = &jh7110_ispclk_config,
	.jhc_rst = &jh7110_isprst_config,
};

static struct jh7110_crg jh7110_stg_config = {
	.jhc_name = "System-Top-Group",
	.jhc_clk = &jh7110_stgclk_config,
	.jhc_rst = &jh7110_stgrst_config,
};

static struct jh7110_crg jh7110_vout_config = {
	.jhc_name = "Video Output",
	.jhc_clk = &jh7110_voutclk_config,
	.jhc_rst = &jh7110_voutrst_config,
};


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-syscrg", .data = &jh7110_sys_config },
	{ .compat = "starfive,jh7110-aoncrg", .data = &jh7110_aon_config },
	{ .compat = "starfive,jh7110-ispcrg", .data = &jh7110_isp_config },
	{ .compat = "starfive,jh7110-stgcrg", .data = &jh7110_stg_config },
	{ .compat = "starfive,jh7110-voutcrg", .data = &jh7110_vout_config },
	DEVICE_COMPAT_EOL
};

#define CLK_LOCK(sc)							\
	mutex_enter(&sc->sc_lock);
#define CLK_UNLOCK(sc)							\
	mutex_exit(&sc->sc_lock);

#define RD4(sc, reg)							\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define JH7110_RESET_RETRIES 1000

static void *
jh7110_clkc_reset_acquire(device_t dev, const void *data, size_t len)
{
	struct jh71x0_clkc_softc * const sc = device_private(dev);

	if (len != sizeof(uint32_t))
		return NULL;

	const uint32_t reset_id = be32dec(data);
	if (reset_id >= sc->sc_nrsts)
		return NULL;

	uint32_t *reset = kmem_alloc(sizeof(uint32_t), KM_SLEEP);
	*reset = reset_id;

	return reset;
}

static void
jh7110_clkc_reset_release(device_t dev, void *priv)
{

	kmem_free(priv, sizeof(uint32_t));
}

static int
jh7110_clkc_reset_set(struct jh71x0_clkc_softc *sc, unsigned reset_id,
    bool assert)
{
	const uint32_t off = (reset_id / 32) * sizeof(uint32_t);
	const uint32_t bit = reset_id % 32;
	const bus_size_t assert_reg = sc->sc_reset_assert + off;
	const bus_size_t status_reg = sc->sc_reset_status + off;

	CLK_LOCK(sc);

	const uint32_t val = RD4(sc, assert_reg);
	if (assert)
		WR4(sc, assert_reg, val | __BIT(bit));
	else
		WR4(sc, assert_reg, val & ~__BIT(bit));

	unsigned i;
	uint32_t status;
	for (i = 0; i < JH7110_RESET_RETRIES; i++) {
		status = RD4(sc, status_reg);
		bool asserted = (status & __BIT(bit)) == 0;
		if (asserted == assert)
			break;
	}
	CLK_UNLOCK(sc);

	if (i >= JH7110_RESET_RETRIES) {
		printf("%s: reset %3d status %#010x / %2d didn't %sassert\n",
		    __func__, reset_id, status, bit, assert ? "" : "de");
		return ETIMEDOUT;
	}

	return 0;
}

static int
jh7110_clkc_reset_assert(device_t dev, void *priv)
{
	struct jh71x0_clkc_softc * const sc = device_private(dev);
	const uint32_t *reset = priv;
	const uint32_t reset_id = *reset;

	return jh7110_clkc_reset_set(sc, reset_id, true);
}

static int
jh7110_clkc_reset_deassert(device_t dev, void *priv)
{
	struct jh71x0_clkc_softc * const sc = device_private(dev);
	const uint32_t *reset = priv;
	const uint32_t reset_id = *reset;

	return jh7110_clkc_reset_set(sc, reset_id, false);
}


static const struct fdtbus_reset_controller_func jh7110_clkc_fdtreset_funcs = {
	.acquire = jh7110_clkc_reset_acquire,
	.release = jh7110_clkc_reset_release,
	.reset_assert = jh7110_clkc_reset_assert,
	.reset_deassert = jh7110_clkc_reset_deassert,
};


static struct clk *
jh7110_clkc_clock_decode(device_t dev, int phandle, const void *data,
    size_t len)
{
	struct jh71x0_clkc_softc * const sc = device_private(dev);

	if (len != sizeof(uint32_t))
		return NULL;

	u_int id = be32dec(data);
	if (id >= sc->sc_nclks) {
		return NULL;
	}
	if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN) {
		printf("Unknown clock %d\n", id);
		return NULL;
	}
	return &sc->sc_clk[id].jcc_clk;
}

static const struct fdtbus_clock_controller_func jh7110_clkc_fdtclock_funcs = {
	.decode = jh7110_clkc_clock_decode
};

static int
jh7110_clkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_clkc_attach(device_t parent, device_t self, void *aux)
{
	struct jh71x0_clkc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &jh71x0_clkc_funcs;
	sc->sc_clkdom.priv = sc;

	const struct jh7110_crg *jhc =
	    of_compatible_lookup(phandle, compat_data)->data;
	KASSERT(jhc != NULL);

	struct jh7110_clock_config * const jhcc = jhc->jhc_clk;
	struct jh7110_reset_config * const jhcr = jhc->jhc_rst;
	sc->sc_clk = jhcc->jhcc_clocks;
	sc->sc_nclks = jhcc->jhcc_nclks;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_nrsts = jhcr->jhcr_nresets;
	sc->sc_reset_assert = jhcr->jhcr_assert;
	sc->sc_reset_status = jhcr->jhcr_status;

	for (size_t id = 0; id < sc->sc_nclks; id++) {
		if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN)
			continue;

		sc->sc_clk[id].jcc_clk.domain = &sc->sc_clkdom;
		// Names already populated.
		clk_attach(&sc->sc_clk[id].jcc_clk);
	}

	aprint_naive("\n");
	aprint_normal(": JH7110 %s Clock and Reset Generator\n",
	    jhc->jhc_name);

	if (jhc->jhc_debug) {
		for (size_t id = 0; id < sc->sc_nclks; id++) {
			if (sc->sc_clk[id].jcc_type == JH71X0CLK_UNKNOWN)
				continue;

			struct clk * const clk = &sc->sc_clk[id].jcc_clk;

			aprint_debug_dev(self, "id %zu [%s]: %u Hz\n", id,
			    clk->name ? clk->name : "<none>", clk_get_rate(clk));
		}
	}

	fdtbus_register_clock_controller(self, phandle, &jh7110_clkc_fdtclock_funcs);
	fdtbus_register_reset_controller(self, phandle, &jh7110_clkc_fdtreset_funcs);
}

CFATTACH_DECL_NEW(jh7110_clkc, sizeof(struct jh71x0_clkc_softc),
	jh7110_clkc_match, jh7110_clkc_attach, NULL, NULL);
