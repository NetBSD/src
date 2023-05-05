/*	$NetBSD: imx6sx_clk.c,v 1.3 2023/05/05 09:34:09 bouyer Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imx6sx_clk.c,v 1.3 2023/05/05 09:34:09 bouyer Exp $");

#include "opt_fdt.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/kmem.h>
#include <sys/param.h>

#include <arm/nxp/imx6_ccmreg.h>
#include <arm/nxp/imx6_ccmvar.h>

#include <dev/clk/clk_backend.h>
#include <dev/fdt/fdtvar.h>

/* Clock IDs - should match dt-bindings/clock/imx6sx-clock.h */

#define IMX6SXCLK_DUMMY				0
#define IMX6SXCLK_CKIL				1
#define IMX6SXCLK_CKIH				2
#define IMX6SXCLK_OSC				3
#define IMX6SXCLK_PLL1_SYS			4
#define IMX6SXCLK_PLL2_BUS			5
#define IMX6SXCLK_PLL3_USB_OTG			6
#define IMX6SXCLK_PLL4_AUDIO			7
#define IMX6SXCLK_PLL5_VIDEO			8
#define IMX6SXCLK_PLL6_ENET			9
#define IMX6SXCLK_PLL7_USB_HOST			10
#define IMX6SXCLK_USBPHY1			11
#define IMX6SXCLK_USBPHY2			12
#define IMX6SXCLK_USBPHY1_GATE			13
#define IMX6SXCLK_USBPHY2_GATE			14
#define IMX6SXCLK_PCIE_REF			15
#define IMX6SXCLK_PCIE_REF_125M			16
#define IMX6SXCLK_ENET_REF			17
#define IMX6SXCLK_PLL2_PFD0			18
#define IMX6SXCLK_PLL2_PFD1			19
#define IMX6SXCLK_PLL2_PFD2			20
#define IMX6SXCLK_PLL2_PFD3			21
#define IMX6SXCLK_PLL3_PFD0			22
#define IMX6SXCLK_PLL3_PFD1			23
#define IMX6SXCLK_PLL3_PFD2			24
#define IMX6SXCLK_PLL3_PFD3			25
#define IMX6SXCLK_PLL2_198M			26
#define IMX6SXCLK_PLL3_120M			27
#define IMX6SXCLK_PLL3_80M			28
#define IMX6SXCLK_PLL3_60M			29
#define IMX6SXCLK_TWD				30
#define IMX6SXCLK_PLL4_POST_DIV			31
#define IMX6SXCLK_PLL4_AUDIO_DIV		32
#define IMX6SXCLK_PLL5_POST_DIV			33
#define IMX6SXCLK_PLL5_VIDEO_DIV		34
#define IMX6SXCLK_STEP				35
#define IMX6SXCLK_PLL1_SW			36
#define IMX6SXCLK_OCRAM_SEL			37
#define IMX6SXCLK_PERIPH_PRE			38
#define IMX6SXCLK_PERIPH2_PRE			39
#define IMX6SXCLK_PERIPH_CLK2_SEL		40
#define IMX6SXCLK_PERIPH2_CLK2_SEL		41
#define IMX6SXCLK_PCIE_AXI_SEL			42
#define IMX6SXCLK_GPU_AXI_SEL			43
#define IMX6SXCLK_GPU_CORE_SEL			44
#define IMX6SXCLK_EIM_SLOW_SEL			45
#define IMX6SXCLK_USDHC1_SEL			46
#define IMX6SXCLK_USDHC2_SEL			47
#define IMX6SXCLK_USDHC3_SEL			48
#define IMX6SXCLK_USDHC4_SEL			49
#define IMX6SXCLK_SSI1_SEL			50
#define IMX6SXCLK_SSI2_SEL			51
#define IMX6SXCLK_SSI3_SEL			52
#define IMX6SXCLK_QSPI1_SEL			53
#define IMX6SXCLK_PERCLK_SEL			54
#define IMX6SXCLK_VID_SEL			55
#define IMX6SXCLK_ESAI_SEL			56
#define IMX6SXCLK_LDB_DI0_DIV_SEL		57
#define IMX6SXCLK_LDB_DI1_DIV_SEL		58
#define IMX6SXCLK_CAN_SEL			59
#define IMX6SXCLK_UART_SEL			60
#define IMX6SXCLK_QSPI2_SEL			61
#define IMX6SXCLK_LDB_DI1_SEL			62
#define IMX6SXCLK_LDB_DI0_SEL			63
#define IMX6SXCLK_SPDIF_SEL			64
#define IMX6SXCLK_AUDIO_SEL			65
#define IMX6SXCLK_ENET_PRE_SEL			66
#define IMX6SXCLK_ENET_SEL			67
#define IMX6SXCLK_M4_PRE_SEL			68
#define IMX6SXCLK_M4_SEL			69
#define IMX6SXCLK_ECSPI_SEL			70
#define IMX6SXCLK_LCDIF1_PRE_SEL		71
#define IMX6SXCLK_LCDIF2_PRE_SEL		72
#define IMX6SXCLK_LCDIF1_SEL			73
#define IMX6SXCLK_LCDIF2_SEL			74
#define IMX6SXCLK_DISPLAY_SEL			75
#define IMX6SXCLK_CSI_SEL			76
#define IMX6SXCLK_CKO1_SEL			77
#define IMX6SXCLK_CKO2_SEL			78
#define IMX6SXCLK_CKO				79
#define IMX6SXCLK_PERIPH_CLK2			80
#define IMX6SXCLK_PERIPH2_CLK2			81
#define IMX6SXCLK_IPG				82
#define IMX6SXCLK_GPU_CORE_PODF			83
#define IMX6SXCLK_GPU_AXI_PODF			84
#define IMX6SXCLK_LCDIF1_PODF			85
#define IMX6SXCLK_QSPI1_PODF			86
#define IMX6SXCLK_EIM_SLOW_PODF			87
#define IMX6SXCLK_LCDIF2_PODF			88
#define IMX6SXCLK_PERCLK			89
#define IMX6SXCLK_VID_PODF			90
#define IMX6SXCLK_CAN_PODF			91
#define IMX6SXCLK_USDHC1_PODF			92
#define IMX6SXCLK_USDHC2_PODF			93
#define IMX6SXCLK_USDHC3_PODF			94
#define IMX6SXCLK_USDHC4_PODF			95
#define IMX6SXCLK_UART_PODF			96
#define IMX6SXCLK_ESAI_PRED			97
#define IMX6SXCLK_ESAI_PODF			98
#define IMX6SXCLK_SSI3_PRED			99
#define IMX6SXCLK_SSI3_PODF			100
#define IMX6SXCLK_SSI1_PRED			101
#define IMX6SXCLK_SSI1_PODF			102
#define IMX6SXCLK_QSPI2_PRED			103
#define IMX6SXCLK_QSPI2_PODF			104
#define IMX6SXCLK_SSI2_PRED			105
#define IMX6SXCLK_SSI2_PODF			106
#define IMX6SXCLK_SPDIF_PRED			107
#define IMX6SXCLK_SPDIF_PODF			108
#define IMX6SXCLK_AUDIO_PRED			109
#define IMX6SXCLK_AUDIO_PODF			110
#define IMX6SXCLK_ENET_PODF			111
#define IMX6SXCLK_M4_PODF			112
#define IMX6SXCLK_ECSPI_PODF			113
#define IMX6SXCLK_LCDIF1_PRED			114
#define IMX6SXCLK_LCDIF2_PRED			115
#define IMX6SXCLK_DISPLAY_PODF			116
#define IMX6SXCLK_CSI_PODF			117
#define IMX6SXCLK_LDB_DI0_DIV_3_5		118
#define IMX6SXCLK_LDB_DI0_DIV_7			119
#define IMX6SXCLK_LDB_DI1_DIV_3_5		120
#define IMX6SXCLK_LDB_DI1_DIV_7			121
#define IMX6SXCLK_CKO1_PODF			122
#define IMX6SXCLK_CKO2_PODF			123
#define IMX6SXCLK_PERIPH			124
#define IMX6SXCLK_PERIPH2			125
#define IMX6SXCLK_OCRAM				126
#define IMX6SXCLK_AHB				127
#define IMX6SXCLK_MMDC_PODF			128
#define IMX6SXCLK_ARM				129
#define IMX6SXCLK_AIPS_TZ1			130
#define IMX6SXCLK_AIPS_TZ2			131
#define IMX6SXCLK_APBH_DMA			132
#define IMX6SXCLK_ASRC_GATE			133
#define IMX6SXCLK_CAAM_MEM			134
#define IMX6SXCLK_CAAM_ACLK			135
#define IMX6SXCLK_CAAM_IPG			136
#define IMX6SXCLK_CAN1_IPG			137
#define IMX6SXCLK_CAN1_SERIAL			138
#define IMX6SXCLK_CAN2_IPG			139
#define IMX6SXCLK_CAN2_SERIAL			140
#define IMX6SXCLK_CPU_DEBUG			141
#define IMX6SXCLK_DCIC1				142
#define IMX6SXCLK_DCIC2				143
#define IMX6SXCLK_AIPS_TZ3			144
#define IMX6SXCLK_ECSPI1			145
#define IMX6SXCLK_ECSPI2			146
#define IMX6SXCLK_ECSPI3			147
#define IMX6SXCLK_ECSPI4			148
#define IMX6SXCLK_ECSPI5			149
#define IMX6SXCLK_EPIT1				150
#define IMX6SXCLK_EPIT2				151
#define IMX6SXCLK_ESAI_EXTAL			152
#define IMX6SXCLK_WAKEUP			153
#define IMX6SXCLK_GPT_BUS			154
#define IMX6SXCLK_GPT_SERIAL			155
#define IMX6SXCLK_GPU				156
#define IMX6SXCLK_OCRAM_S			157
#define IMX6SXCLK_CANFD				158
#define IMX6SXCLK_CSI				159
#define IMX6SXCLK_I2C1				160
#define IMX6SXCLK_I2C2				161
#define IMX6SXCLK_I2C3				162
#define IMX6SXCLK_OCOTP				163
#define IMX6SXCLK_IOMUXC			164
#define IMX6SXCLK_IPMUX1			165
#define IMX6SXCLK_IPMUX2			166
#define IMX6SXCLK_IPMUX3			167
#define IMX6SXCLK_TZASC1			168
#define IMX6SXCLK_LCDIF_APB			169
#define IMX6SXCLK_PXP_AXI			170
#define IMX6SXCLK_M4				171
#define IMX6SXCLK_ENET				172
#define IMX6SXCLK_DISPLAY_AXI			173
#define IMX6SXCLK_LCDIF2_PIX			174
#define IMX6SXCLK_LCDIF1_PIX			175
#define IMX6SXCLK_LDB_DI0			176
#define IMX6SXCLK_QSPI1				177
#define IMX6SXCLK_MLB				178
#define IMX6SXCLK_MMDC_P0_FAST			179
#define IMX6SXCLK_MMDC_P0_IPG			180
#define IMX6SXCLK_AXI				181
#define IMX6SXCLK_PCIE_AXI			182
#define IMX6SXCLK_QSPI2				183
#define IMX6SXCLK_PER1_BCH			184
#define IMX6SXCLK_PER2_MAIN			185
#define IMX6SXCLK_PWM1				186
#define IMX6SXCLK_PWM2				187
#define IMX6SXCLK_PWM3				188
#define IMX6SXCLK_PWM4				189
#define IMX6SXCLK_GPMI_BCH_APB			190
#define IMX6SXCLK_GPMI_BCH			191
#define IMX6SXCLK_GPMI_IO			192
#define IMX6SXCLK_GPMI_APB			193
#define IMX6SXCLK_ROM				194
#define IMX6SXCLK_SDMA				195
#define IMX6SXCLK_SPBA				196
#define IMX6SXCLK_SPDIF				197
#define IMX6SXCLK_SSI1_IPG			198
#define IMX6SXCLK_SSI2_IPG			199
#define IMX6SXCLK_SSI3_IPG			200
#define IMX6SXCLK_SSI1				201
#define IMX6SXCLK_SSI2				202
#define IMX6SXCLK_SSI3				203
#define IMX6SXCLK_UART_IPG			204
#define IMX6SXCLK_UART_SERIAL			205
#define IMX6SXCLK_SAI1				206
#define IMX6SXCLK_SAI2				207
#define IMX6SXCLK_USBOH3			208
#define IMX6SXCLK_USDHC1			209
#define IMX6SXCLK_USDHC2			210
#define IMX6SXCLK_USDHC3			211
#define IMX6SXCLK_USDHC4			212
#define IMX6SXCLK_EIM_SLOW			213
#define IMX6SXCLK_PWM8				214
#define IMX6SXCLK_VADC				215
#define IMX6SXCLK_GIS				216
#define IMX6SXCLK_I2C4				217
#define IMX6SXCLK_PWM5				218
#define IMX6SXCLK_PWM6				219
#define IMX6SXCLK_PWM7				220
#define IMX6SXCLK_CKO1				221
#define IMX6SXCLK_CKO2				222
#define IMX6SXCLK_IPP_DI0			223
#define IMX6SXCLK_IPP_DI1			224
#define IMX6SXCLK_ENET_AHB			225
#define IMX6SXCLK_OCRAM_PODF			226
#define IMX6SXCLK_GPT_3M			227
#define IMX6SXCLK_ENET_PTP			228
#define IMX6SXCLK_ENET_PTP_REF			229
#define IMX6SXCLK_ENET2_REF			230
#define IMX6SXCLK_ENET2_REF_125M		231
#define IMX6SXCLK_AUDIO				232
#define IMX6SXCLK_LVDS1_SEL			233
#define IMX6SXCLK_LVDS1_OUT			234
#define IMX6SXCLK_ASRC_IPG			235
#define IMX6SXCLK_ASRC_MEM			236
#define IMX6SXCLK_SAI1_IPG			237
#define IMX6SXCLK_SAI2_IPG			238
#define IMX6SXCLK_ESAI_IPG			239
#define IMX6SXCLK_ESAI_MEM			240
#define IMX6SXCLK_LVDS1_IN			241
#define IMX6SXCLK_ANACLK1			242
#define IMX6SXCLK_PLL1_BYPASS_SRC		243
#define IMX6SXCLK_PLL2_BYPASS_SRC		244
#define IMX6SXCLK_PLL3_BYPASS_SRC		245
#define IMX6SXCLK_PLL4_BYPASS_SRC		246
#define IMX6SXCLK_PLL5_BYPASS_SRC		247
#define IMX6SXCLK_PLL6_BYPASS_SRC		248
#define IMX6SXCLK_PLL7_BYPASS_SRC		249
#define IMX6SXCLK_PLL1				250
#define IMX6SXCLK_PLL2				251
#define IMX6SXCLK_PLL3				252
#define IMX6SXCLK_PLL4				253
#define IMX6SXCLK_PLL5				254
#define IMX6SXCLK_PLL6				255
#define IMX6SXCLK_PLL7				256
#define IMX6SXCLK_PLL1_BYPASS			257
#define IMX6SXCLK_PLL2_BYPASS			258
#define IMX6SXCLK_PLL3_BYPASS			259
#define IMX6SXCLK_PLL4_BYPASS			260
#define IMX6SXCLK_PLL5_BYPASS			261
#define IMX6SXCLK_PLL6_BYPASS			262
#define IMX6SXCLK_PLL7_BYPASS			263
#define IMX6SXCLK_SPDIF_GCLK			264
#define IMX6SXCLK_LVDS2_SEL			265
#define IMX6SXCLK_LVDS2_OUT			266
#define IMX6SXCLK_LVDS2_IN			267
#define IMX6SXCLK_ANACLK2			268
#define IMX6SXCLK_MMDC_P1_IPG			269
#define IMX6SXCLK_END				270

/* Clock Parents Tables */
static const char *step_p[] = {
	"osc",
	"pll2_pfd2_396m"
};

static const char *pll1_sw_p[] = {
	"pll1_sys",
	"step"
};

static const char *periph_pre_p[] = {
	"pll2_bus",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m",
	"pll2_198m"
};

static const char *periph2_pre_p[] = {
	"pll2_bus",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m",
	"pll4_audio_div"
};

static const char *periph_clk2_p[] = {
	"pll3_usb_otg",
	"osc",
	"osc"
};

static const char *periph2_clk2_p[] = {
	"pll3_usb_otg",
	"pll2_bus"
};

static const char *audio_p[] = {
	"pll4_audio_div",
	"pll3_pfd2_508m",
	"pll5_video_div",
	"pll3_usb_otg"
};

static const char *gpu_axi_p[] = {
	"pll2_pfd2_396m",
	"pll3_pfd0_720m",
	"pll3_pfd1_540m",
	"pll2_bus"
};

static const char *gpu_core_p[] = {
	"pll3_pfd1_540m",
	"pll3_pfd0_720m",
	"pll2_bus",
	"pll2_pfd2_396m"
};

static const char *ldb_di0_div_p[] = {
	"ldb_di0_div_3_5",
	"ldb_di0_div_7",
};

static const char *ldb_di1_div_p[] = {
	"ldb_di1_div_3_5",
	"ldb_di1_div_7",
};

static const char *ldb_di0_p[] = {
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll2_pfd3_594m",
	"pll2_pfd1_594m",
	"pll3_pfd3_454m",
};

static const char *ldb_di1_p[] = {
	"pll3_usb_otg",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll2_bus",
	"pll3_pfd3_454m",
	"pll3_pfd2_508m",
};

static const char *pll_bypass_src_p[] = {
	"osc",
	"lvds1_in",
	"lvds2_in",
	"dummy"
};

static const char *pll1_bypass_p[] = {
	"pll1",
	"pll1_bypass_src"
};

static const char *pll2_bypass_p[] = {
	"pll2",
	"pll2_bypass_src"
};

static const char *pll3_bypass_p[] = {
	"pll3",
	"pll3_bypass_src"
};

static const char *pll4_bypass_p[] = {
	"pll4",
	"pll4_bypass_src"
};

static const char *pll5_bypass_p[] = {
	"pll5",
	"pll5_bypass_src"
};

static const char *pll6_bypass_p[] = {
	"pll6",
	"pll6_bypass_src"
};

static const char *pll7_bypass_p[] = {
	"pll7",
	"pll7_bypass_src"
};

static const char *periph_p[] = {
	"periph_pre",
	"periph_clk2"
};

static const char *periph2_p[] = {
	"periph2_pre",
	"periph2_clk2"
};

static const char *ocram_p[] = {
	"periph",
	"pll2_pfd2_396m",
	"periph",
	"pll3_pfd1_540m",
};

static const char *cko1_p[] = {
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"vadc",
	"ocram",
	"qspi2",
	"m4",
	"enet_ahb",
	"lcdif2_pix",
	"lcdif1_pix",
	"ahb",
	"ipg",
	"perclk",
	"ckil",
	"pll4_audio_div",
};

static const char *cko2_p[] = {
	"dummy",
	"mmdc_p0_fast",
	"usdhc4",
	"usdhc1",
	"dummy",
	"wrck",
	"ecspi_root",
	"dummy",
	"usdhc3",
	"pcie",
	"arm",
	"csi_core",
	"display_axi",
	"dummy",
	"osc",
	"dummy",
	"dummy",
	"usdhc2",
	"ssi1",
	"ssi2",
	"ssi3",
	"gpu_axi_podf",
	"dummy",
	"can_podf",
	"lvds1_out",
	"qspi1",
	"esai_extal",
	"eim_slow",
	"uart_serial",
	"spdif",
	"audio",
	"dummy",
	};

static const char *cko_p[] = {
	"cko1",
	"cko2"
};

static const char *pcie_axi_p[] = {
	"ocram",
	"ahb"
};

static const char *ssi_p[] = {
	"pll3_pfd2_508m",
	"pll5_video_div",
	"pll4_audio_div"
};

static const char *qspi1_p[] = {
	"pll3_usb_otg",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll2_bus",
	"pll3_pfd3_454m",
	"pll3_pfd2_508m",
};

static const char *perclk_p[] = {
	"ipg",
	"osc"
};

static const char *usdhc_p[] = {
	"pll2_pfd2_396m",
	"pll2_pfd0_352m"
};

static const char *vid_p[] = {
	"pll3_pfd1_540m",
	"pll3_usb_otg",
	"pll3_pfd3_454m",
	"pll4_audio_div",
	"pll5_video_div",
};

static const char *can_p[] = {
	"pll3_60m",
	"osc",
	"pll3_80m",
	"dummy",
};

static const char *uart_p[] = {
	"pll3_80m",
	"osc",
};

static const char *qspi2_p[] = {
	"pll2_pfd0_352m",
	"pll2_bus",
	"pll3_usb_otg",
	"pll2_pfd2_396m",
	"pll3_pfd3_454m",
	"dummy",
	"dummy",
	"dummy",
};

static const char *enet_pre_p[] = {
	"pll2_bus",
	"pll3_usb_otg",
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll3_pfd2_508m",
};

static const char *enet_p[] = {
	"enet_podf",
	"ipp_di0",
	"ipp_di1",
	"ldb_di0",
	"ldb_di1",
};

static const char *m4_pre_p[] = {
	"pll2_bus",
	"pll3_usb_otg",
	"osc",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll3_pfd3_454m",
};

static const char *m4_p[] = {
	"m4_pre_sel",
	"ipp_di0",
	"ipp_di1",
	"ldb_di0",
	"ldb_di1",
};

static const char *eim_slow_p[] = {
	"ocram",
	"pll3_usb_otg",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m"
};

static const char *ecspi_p[] = {
	"pll3_60m",
	"osc",
};

static const char *lcdif1_pre_p[] = {
	"pll2_bus",
	"pll3_pfd3_454m",
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd1_594m",
	"pll3_pfd1_540m",
};

static const char *lcdif1_p[] = {
	"lcdif1_podf",
	"ipp_di0",
	"ipp_di1",
	"ldb_di0",
	"ldb_di1",
};

static const char *lcdif2_pre_p[] = {
	"pll2_bus",
	"pll3_pfd3_454m",
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd3_594m",
	"pll3_pfd1_540m",
};

static const char *lcdif2_p[] = {
	"lcdif2_podf",
	"ipp_di0",
	"ipp_di1",
	"ldb_di0",
	"ldb_di1",
};

static const char *display_p[] = {
	"pll2_bus",
	"pll2_pfd2_396m",
	"pll3_usb_otg",
	"pll3_pfd1_540m",
};

static const char *csi_p[] = {
	"osc",
	"pll2_pfd2_396m",
	"pll3_120m",
	"pll3_pfd1_540m",
};

static const char *lvds_p[] = {
	"arm",
	"pll1_sys",
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"pll5_video_div",
	"dummy",
	"dummy",
	"pcie_ref_125m",
	"dummy",
	"usbphy1",
	"usbphy2",
};

/* DT clock ID to clock name mappings */
static struct imx_clock_id {
	u_int		id;
	const char	*name;
} imx6sx_clock_ids[] = {
	{ IMX6SXCLK_DUMMY,		"dummy" },
	{ IMX6SXCLK_CKIL,		"ckil" },
	{ IMX6SXCLK_OSC,		"osc" },
	{ IMX6SXCLK_PLL1_SYS,		"pll1_sys" },
	{ IMX6SXCLK_PLL2_BUS,		"pll2_bus" },
	{ IMX6SXCLK_PLL3_USB_OTG,	"pll3_usb_otg" },
	{ IMX6SXCLK_PLL4_AUDIO,		"pll4_audio" },
	{ IMX6SXCLK_PLL5_VIDEO,		"pll5_video" },
	{ IMX6SXCLK_PLL6_ENET,		"pll6_enet" },
	{ IMX6SXCLK_PLL7_USB_HOST,	"pll7_usb_host" },
	{ IMX6SXCLK_USBPHY1,		"usbphy1" },
	{ IMX6SXCLK_USBPHY2,		"usbphy2" },
	{ IMX6SXCLK_USBPHY1_GATE,	"usbphy1_gate" },
	{ IMX6SXCLK_USBPHY2_GATE,	"usbphy2_gate" },
	{ IMX6SXCLK_PCIE_REF,		"pcie_ref" },
	{ IMX6SXCLK_PCIE_REF_125M,	"pcie_ref_125m" },
	{ IMX6SXCLK_PLL2_PFD0,		"pll2_pfd0_352m" },
	{ IMX6SXCLK_PLL2_PFD1,		"pll2_pfd1_594m" },
	{ IMX6SXCLK_PLL2_PFD2,		"pll2_pfd2_396m" },
	{ IMX6SXCLK_PLL2_PFD3,		"pll2_pfd3_594m" },
	{ IMX6SXCLK_PLL3_PFD0,		"pll3_pfd0_720m" },
	{ IMX6SXCLK_PLL3_PFD1,		"pll3_pfd1_540m" },
	{ IMX6SXCLK_PLL3_PFD2,		"pll3_pfd2_508m" },
	{ IMX6SXCLK_PLL3_PFD3,		"pll3_pfd3_454m" },
	{ IMX6SXCLK_PLL2_198M,		"pll2_198m" },
	{ IMX6SXCLK_PLL3_120M,		"pll3_120m" },
	{ IMX6SXCLK_PLL3_80M,		"pll3_80m" },
	{ IMX6SXCLK_PLL3_60M,		"pll3_60m" },
	{ IMX6SXCLK_TWD,		"twd" },
	{ IMX6SXCLK_PLL4_POST_DIV,	"pll4_post_div" },
	{ IMX6SXCLK_PLL4_AUDIO_DIV,	"pll4_audio_div" },
	{ IMX6SXCLK_PLL5_POST_DIV,	"pll5_post_div" },
	{ IMX6SXCLK_PLL5_VIDEO_DIV,	"pll5_video_div" },
	{ IMX6SXCLK_STEP,		"step" },
	{ IMX6SXCLK_PLL1_SW,		"pll1_sw" },
	{ IMX6SXCLK_OCRAM_SEL,		"ocram_sel" },
	{ IMX6SXCLK_PERIPH_PRE,		"periph_pre" },
	{ IMX6SXCLK_PERIPH2_PRE,	"periph2_pre" },
	{ IMX6SXCLK_PERIPH_CLK2_SEL,	"periph_clk2_sel" },
	{ IMX6SXCLK_PERIPH2_CLK2_SEL,	"periph2_clk2_sel" },
	{ IMX6SXCLK_PCIE_AXI_SEL,	"pcie_axi_sel" },
	{ IMX6SXCLK_GPU_AXI_SEL,	"gpu_axi_sel" },
	{ IMX6SXCLK_GPU_CORE_SEL,	"gpu_core_sel" },
	{ IMX6SXCLK_EIM_SLOW_SEL,	"eim_slow_sel" },
	{ IMX6SXCLK_USDHC1_SEL,		"usdhc1_sel" },
	{ IMX6SXCLK_USDHC2_SEL,		"usdhc2_sel" },
	{ IMX6SXCLK_USDHC3_SEL,		"usdhc3_sel" },
	{ IMX6SXCLK_USDHC4_SEL,		"usdhc4_sel" },
	{ IMX6SXCLK_SSI1_SEL,		"ssi1_sel" },
	{ IMX6SXCLK_SSI2_SEL,		"ssi2_sel" },
	{ IMX6SXCLK_SSI3_SEL,		"ssi3_sel" },
	{ IMX6SXCLK_QSPI1_SEL,		"qspi1_sel" },
	{ IMX6SXCLK_PERCLK_SEL,		"perclk_sel" },
	{ IMX6SXCLK_VID_SEL,		"vid_sel" },
	{ IMX6SXCLK_ESAI_SEL,		"esai_sel" },
	{ IMX6SXCLK_LDB_DI0_DIV_SEL,	"ldb_di0_div_sel" },
	{ IMX6SXCLK_LDB_DI1_DIV_SEL,	"ldb_di1_div_sel" },
	{ IMX6SXCLK_CAN_SEL,		"can_sel" },
	{ IMX6SXCLK_UART_SEL,		"uart_sel" },
	{ IMX6SXCLK_QSPI2_SEL,		"qspi2_sel" },
	{ IMX6SXCLK_LDB_DI0_SEL,	"ldb_di0_sel" },
	{ IMX6SXCLK_LDB_DI1_SEL,	"ldb_di1_sel" },
	{ IMX6SXCLK_SPDIF_SEL,		"spdif_sel" },
	{ IMX6SXCLK_AUDIO_SEL,		"audio_sel" },
	{ IMX6SXCLK_ENET_PRE_SEL,	"enet_pre_sel" },
	{ IMX6SXCLK_ENET_SEL,		"enet_sel" },
	{ IMX6SXCLK_M4_PRE_SEL,		"m4_pre_sel" },
	{ IMX6SXCLK_M4_SEL,		"m4_sel" },
	{ IMX6SXCLK_ECSPI_SEL,		"ecspi_sel" },
	{ IMX6SXCLK_LCDIF1_PRE_SEL,	"lcdif1_pre_sel" },
	{ IMX6SXCLK_LCDIF2_PRE_SEL,	"lcdif2_pre_sel" },
	{ IMX6SXCLK_LCDIF1_SEL,		"lcdif1_sel" },
	{ IMX6SXCLK_LCDIF2_SEL,		"lcdif2_sel" },
	{ IMX6SXCLK_DISPLAY_SEL,	"display_sel" },
	{ IMX6SXCLK_CSI_SEL,		"csi_sel" },
	{ IMX6SXCLK_CKO1_SEL,		"cko1_sel" },
	{ IMX6SXCLK_CKO2_SEL,		"cko2_sel" },
	{ IMX6SXCLK_CKO,		"cko" },
	{ IMX6SXCLK_PERIPH_CLK2,	"periph_clk2" },
	{ IMX6SXCLK_PERIPH2_CLK2,	"periph2_clk2" },
	{ IMX6SXCLK_IPG,		"ipg" },
	{ IMX6SXCLK_GPU_CORE_PODF,	"gpu_core_podf" },
	{ IMX6SXCLK_GPU_AXI_PODF,	"gpu_axi_podf" },
	{ IMX6SXCLK_LCDIF1_PODF,	"lcdif1_podf" },
	{ IMX6SXCLK_QSPI1_PODF,		"qspi1_podf" },
	{ IMX6SXCLK_EIM_SLOW_PODF,	"eim_slow_podf" },
	{ IMX6SXCLK_LCDIF2_PODF,	"lcdif2_podf" },
	{ IMX6SXCLK_PERCLK,		"perclk" },
	{ IMX6SXCLK_VID_PODF,		"vid_podf" },
	{ IMX6SXCLK_CAN_PODF,		"can_podf" },
	{ IMX6SXCLK_USDHC1_PODF,	"usdhc1_podf" },
	{ IMX6SXCLK_USDHC2_PODF,	"usdhc2_podf" },
	{ IMX6SXCLK_USDHC3_PODF,	"usdhc3_podf" },
	{ IMX6SXCLK_USDHC4_PODF,	"usdhc4_podf" },
	{ IMX6SXCLK_UART_PODF,		"uart_podf" },
	{ IMX6SXCLK_ESAI_PRED,		"esai_pred" },
	{ IMX6SXCLK_ESAI_PODF,		"esai_podf" },
	{ IMX6SXCLK_SSI3_PRED,		"ssi3_pred" },
	{ IMX6SXCLK_SSI3_PODF,		"ssi3_podf" },
	{ IMX6SXCLK_SSI1_PRED,		"ssi1_pred" },
	{ IMX6SXCLK_SSI1_PODF,		"ssi1_podf" },
	{ IMX6SXCLK_QSPI2_PRED,		"qspi2_pred" },
	{ IMX6SXCLK_QSPI2_PODF,		"qspi2_podf" },
	{ IMX6SXCLK_SSI2_PRED,		"ssi2_pred" },
	{ IMX6SXCLK_SSI2_PODF,		"ssi2_podf" },
	{ IMX6SXCLK_SPDIF_PRED,		"spdif_pred" },
	{ IMX6SXCLK_SPDIF_PODF,		"spdif_podf" },
	{ IMX6SXCLK_AUDIO_PRED,		"audio_pred" },
	{ IMX6SXCLK_AUDIO_PODF,		"audio_podf" },
	{ IMX6SXCLK_ENET_PODF,		"enet_podf" },
	{ IMX6SXCLK_M4_PODF,		"m4_podf" },
	{ IMX6SXCLK_ECSPI_PODF,		"ecspi_podf" },
	{ IMX6SXCLK_LCDIF1_PRED,	"lcdif1_pred" },
	{ IMX6SXCLK_LCDIF2_PRED,	"lcdif2_pred" },
	{ IMX6SXCLK_DISPLAY_PODF,	"display_podf" },
	{ IMX6SXCLK_CSI_PODF,		"csi_podf" },
	{ IMX6SXCLK_LDB_DI0_DIV_3_5,	"ldb_di0_div_3_5" },
	{ IMX6SXCLK_LDB_DI0_DIV_7,	"ldb_di0_div_7" },
	{ IMX6SXCLK_LDB_DI1_DIV_3_5,	"ldb_di1_div_3_5" },
	{ IMX6SXCLK_LDB_DI1_DIV_7,	"ldb_di1_div_7" },
	{ IMX6SXCLK_CKO1_PODF,		"cko1_podf" },
	{ IMX6SXCLK_CKO2_PODF,		"cko2_podf" },
	{ IMX6SXCLK_PERIPH,		"periph" },
	{ IMX6SXCLK_PERIPH2,		"periph2" },
	{ IMX6SXCLK_OCRAM,		"ocram" },
	{ IMX6SXCLK_AHB,		"ahb" },
	{ IMX6SXCLK_MMDC_PODF,		"mmdc_podf" },
	{ IMX6SXCLK_ARM,		"arm" },
	{ IMX6SXCLK_AIPS_TZ1,		"aips_tz1" },
	{ IMX6SXCLK_AIPS_TZ2,		"aips_tz2" },
	{ IMX6SXCLK_APBH_DMA,		"apbh_dma" },
	{ IMX6SXCLK_CAAM_MEM,		"caam_mem" },
	{ IMX6SXCLK_CAAM_ACLK,		"caam_aclk" },
	{ IMX6SXCLK_CAAM_IPG,		"caam_ipg" },
	{ IMX6SXCLK_CAN1_IPG,		"can1_ipg" },
	{ IMX6SXCLK_CAN1_SERIAL,	"can1_serial" },
	{ IMX6SXCLK_CAN2_IPG,		"can2_ipg" },
	{ IMX6SXCLK_CAN2_SERIAL,	"can2_serial" },
	{ IMX6SXCLK_DCIC1,		"dcic1" },
	{ IMX6SXCLK_DCIC2,		"dcic2" },
	{ IMX6SXCLK_AIPS_TZ3,		"aips_tz3" },
	{ IMX6SXCLK_ECSPI1,		"ecspi1" },
	{ IMX6SXCLK_ECSPI2,		"ecspi2" },
	{ IMX6SXCLK_ECSPI3,		"ecspi3" },
	{ IMX6SXCLK_ECSPI4,		"ecspi4" },
	{ IMX6SXCLK_ECSPI5,		"ecspi5" },
	{ IMX6SXCLK_EPIT1,		"epit1" },
	{ IMX6SXCLK_EPIT2,		"epit2" },
	{ IMX6SXCLK_ESAI_EXTAL,		"esai_extal" },
	{ IMX6SXCLK_WAKEUP,		"wakeup" },
	{ IMX6SXCLK_GPT_BUS,		"gpt_bus" },
	{ IMX6SXCLK_GPT_SERIAL,		"gpt_serial" },
	{ IMX6SXCLK_GPU,		"gpu" },
	{ IMX6SXCLK_OCRAM_S,		"ocram_s" },
	{ IMX6SXCLK_CANFD,		"canfd" },
	{ IMX6SXCLK_CSI,		"csi" },
	{ IMX6SXCLK_I2C1,		"i2c1" },
	{ IMX6SXCLK_I2C2,		"i2c2" },
	{ IMX6SXCLK_I2C3,		"i2c3" },
	{ IMX6SXCLK_OCOTP,		"ocotp" },
	{ IMX6SXCLK_IOMUXC,		"iomuxc" },
	{ IMX6SXCLK_IPMUX1,		"ipmux1" },
	{ IMX6SXCLK_IPMUX2,		"ipmux2" },
	{ IMX6SXCLK_IPMUX3,		"ipmux3" },
	{ IMX6SXCLK_TZASC1,		"tzasc1" },
	{ IMX6SXCLK_LCDIF_APB,		"lcdif_apb" },
	{ IMX6SXCLK_PXP_AXI,		"pxp_axi" },
	{ IMX6SXCLK_M4,			"m4" },
	{ IMX6SXCLK_ENET,		"enet" },
	{ IMX6SXCLK_DISPLAY_AXI,	"display_axi" },
	{ IMX6SXCLK_LCDIF2_PIX,		"lcdif2_pix" },
	{ IMX6SXCLK_LCDIF1_PIX,		"lcdif1_pix" },
	{ IMX6SXCLK_LDB_DI0,		"ldb_di0" },
	{ IMX6SXCLK_QSPI1,		"qspi1" },
	{ IMX6SXCLK_MLB,		"mlb" },
	{ IMX6SXCLK_MMDC_P0_FAST,	"mmdc_p0_fast" },
	{ IMX6SXCLK_MMDC_P0_IPG,	"mmdc_p0_ipg" },
	{ IMX6SXCLK_PCIE_AXI,		"pcie_axi" },
	{ IMX6SXCLK_QSPI2,		"qspi2" },
	{ IMX6SXCLK_PER1_BCH,		"per1_bch" },
	{ IMX6SXCLK_PER2_MAIN,		"per2_main" },
	{ IMX6SXCLK_PWM1,		"pwm1" },
	{ IMX6SXCLK_PWM2,		"pwm2" },
	{ IMX6SXCLK_PWM3,		"pwm3" },
	{ IMX6SXCLK_PWM4,		"pwm4" },
	{ IMX6SXCLK_GPMI_BCH_APB,	"gpmi_bch_apb" },
	{ IMX6SXCLK_GPMI_BCH,		"gpmi_bch" },
	{ IMX6SXCLK_GPMI_IO,		"gpmi_io" },
	{ IMX6SXCLK_GPMI_APB,		"gpmi_apb" },
	{ IMX6SXCLK_ROM,		"rom" },
	{ IMX6SXCLK_SDMA,		"sdma" },
	{ IMX6SXCLK_SPBA,		"spba" },
	{ IMX6SXCLK_SPDIF,		"spdif" },
	{ IMX6SXCLK_SSI1_IPG,		"ssi1_ipg" },
	{ IMX6SXCLK_SSI2_IPG,		"ssi2_ipg" },
	{ IMX6SXCLK_SSI3_IPG,		"ssi3_ipg" },
	{ IMX6SXCLK_SSI1,		"ssi1" },
	{ IMX6SXCLK_SSI2,		"ssi2" },
	{ IMX6SXCLK_SSI3,		"ssi3" },
	{ IMX6SXCLK_UART_IPG,		"uart_ipg" },
	{ IMX6SXCLK_UART_SERIAL,	"uart_serial" },
	{ IMX6SXCLK_SAI1,		"sai1" },
	{ IMX6SXCLK_SAI2,		"sai2" },
	{ IMX6SXCLK_USBOH3,		"usboh3" },
	{ IMX6SXCLK_USDHC1,		"usdhc1" },
	{ IMX6SXCLK_USDHC2,		"usdhc2" },
	{ IMX6SXCLK_USDHC3,		"usdhc3" },
	{ IMX6SXCLK_USDHC4,		"usdhc4" },
	{ IMX6SXCLK_EIM_SLOW,		"eim_slow" },
	{ IMX6SXCLK_PWM8,		"pwm8" },
	{ IMX6SXCLK_VADC,		"vadc" },
	{ IMX6SXCLK_GIS,		"gis" },
	{ IMX6SXCLK_I2C4,		"i2c4" },
	{ IMX6SXCLK_PWM5,		"pwm5" },
	{ IMX6SXCLK_PWM6,		"pwm6" },
	{ IMX6SXCLK_PWM7,		"pwm7" },
	{ IMX6SXCLK_CKO1,		"cko1" },
	{ IMX6SXCLK_CKO2,		"cko2" },
	{ IMX6SXCLK_IPP_DI0,		"ipp_di0" },
	{ IMX6SXCLK_IPP_DI1,		"ipp_di1" },
	{ IMX6SXCLK_ENET_AHB,		"enet_ahb" },
	{ IMX6SXCLK_OCRAM_PODF,		"ocram_podf" },
	{ IMX6SXCLK_GPT_3M,		"gpt_3m" },
	{ IMX6SXCLK_ENET_PTP,		"enet_ptp_25m" },
	{ IMX6SXCLK_ENET_PTP_REF,	"enet_ptp_ref" },
	{ IMX6SXCLK_ENET2_REF,		"enet2_ref" },
	{ IMX6SXCLK_ENET2_REF_125M,	"enet2_ref_125m" },
	{ IMX6SXCLK_AUDIO,		"audio" },
	{ IMX6SXCLK_LVDS1_SEL,		"lvds1_sel" },
	{ IMX6SXCLK_LVDS1_OUT,		"lvds1_out" },
	{ IMX6SXCLK_ASRC_IPG,		"asrc_ipg" },
	{ IMX6SXCLK_ASRC_MEM,		"asrc_mem" },
	{ IMX6SXCLK_SAI1_IPG,		"sai1_ipg" },
	{ IMX6SXCLK_SAI2_IPG,		"sai2_ipg" },
	{ IMX6SXCLK_ESAI_IPG,		"esai_ipg" },
	{ IMX6SXCLK_ESAI_MEM,		"esai_mem" },
	{ IMX6SXCLK_LVDS1_IN,		"lvds1_in" },
	{ IMX6SXCLK_ANACLK1,		"anaclk1" },
	{ IMX6SXCLK_PLL1_BYPASS_SRC,	"pll1_bypass_src" },
	{ IMX6SXCLK_PLL2_BYPASS_SRC,	"pll2_bypass_src" },
	{ IMX6SXCLK_PLL3_BYPASS_SRC,	"pll3_bypass_src" },
	{ IMX6SXCLK_PLL4_BYPASS_SRC,	"pll4_bypass_src" },
	{ IMX6SXCLK_PLL5_BYPASS_SRC,	"pll5_bypass_src" },
	{ IMX6SXCLK_PLL6_BYPASS_SRC,	"pll6_bypass_src" },
	{ IMX6SXCLK_PLL7_BYPASS_SRC,	"pll7_bypass_src" },
	{ IMX6SXCLK_PLL1,		"pll1" },
	{ IMX6SXCLK_PLL2,		"pll2" },
	{ IMX6SXCLK_PLL3,		"pll3" },
	{ IMX6SXCLK_PLL4,		"pll4" },
	{ IMX6SXCLK_PLL5,		"pll5" },
	{ IMX6SXCLK_PLL6,		"pll6" },
	{ IMX6SXCLK_PLL7,		"pll7" },
	{ IMX6SXCLK_PLL1_BYPASS,	"pll1_bypass" },
	{ IMX6SXCLK_PLL2_BYPASS,	"pll2_bypass" },
	{ IMX6SXCLK_PLL3_BYPASS,	"pll3_bypass" },
	{ IMX6SXCLK_PLL4_BYPASS,	"pll4_bypass" },
	{ IMX6SXCLK_PLL5_BYPASS,	"pll5_bypass" },
	{ IMX6SXCLK_PLL6_BYPASS,	"pll6_bypass" },
	{ IMX6SXCLK_PLL7_BYPASS,	"pll7_bypass" },
	{ IMX6SXCLK_SPDIF_GCLK,		"spdif_gclk" },
	{ IMX6SXCLK_LVDS2_SEL,		"lvds2_sel" },
	{ IMX6SXCLK_LVDS2_OUT,		"lvds2_out" },
	{ IMX6SXCLK_LVDS2_IN,		"lvds2_in" },
	{ IMX6SXCLK_ANACLK2,		"anaclk2" },
	{ IMX6SXCLK_MMDC_P1_IPG,	"mmdc_p1_ipg" },
	{ IMX6SXCLK_END,		"end" },
};

/* Clock Divider Tables */
static const int enet_ref_tbl[] = { 20, 10, 5, 4, 0 };
static const int post_div_tbl[] = { 4, 2, 1, 0 };
static const int audiovideo_div_tbl[] = { 1, 2, 1, 4, 0 };

static struct imx6_clk imx6sx_clks[] = {
	CLK_FIXED("dummy", 0),
	CLK_FIXED("ckil", IMX6_CKIL_FREQ),
	CLK_FIXED("osc", IMX6_OSC_FREQ),
	CLK_FIXED("ipp_di0", IMX6_OSC_FREQ), 
	CLK_FIXED("ipp_di1", IMX6_OSC_FREQ), 
	CLK_FIXED("anaclk1", IMX6_ANACLK1_FREQ),
	CLK_FIXED("anaclk2", IMX6_ANACLK2_FREQ),

	CLK_FIXED_FACTOR("pcie_ref", "pll6_enet", 5, 1),
	CLK_FIXED_FACTOR("pll2_198m", "pll2_pfd2_396m", 2, 1),
	CLK_FIXED_FACTOR("pll3_120m", "pll3_usb_otg", 4, 1),
	CLK_FIXED_FACTOR("pll3_80m", "pll3_usb_otg", 6, 1),
	CLK_FIXED_FACTOR("pll3_60m", "pll3_usb_otg", 8, 1),
	CLK_FIXED_FACTOR("twd", "arm", 2, 1),
	CLK_FIXED_FACTOR("gpt_3m", "osc", 8, 1),
	CLK_FIXED_FACTOR("ldb_di0_div_3_5", "ldb_di0_sel", 7, 2),
	CLK_FIXED_FACTOR("ldb_di0_div_7", "ldb_di0_sel", 7, 1),
	CLK_FIXED_FACTOR("ldb_di1_div_3_5", "ldb_di1_sel", 7, 2),
	CLK_FIXED_FACTOR("ldb_di1_div_7", "ldb_di1_sel", 7, 1),
	CLK_FIXED_FACTOR("enet_ptp_ref", "pll6_enet", 20, 1),

	CLK_PFD("pll2_pfd0_352m", "pll2_bus", PFD_528, 0),
	CLK_PFD("pll2_pfd1_594m", "pll2_bus", PFD_528, 1),
	CLK_PFD("pll2_pfd2_396m", "pll2_bus", PFD_528, 2),
	CLK_PFD("pll2_pfd3_594m", "pll2_bus", PFD_528, 3),
	CLK_PFD("pll3_pfd0_720m", "pll3_usb_otg", PFD_480, 0),
	CLK_PFD("pll3_pfd1_540m", "pll3_usb_otg", PFD_480, 1),
	CLK_PFD("pll3_pfd2_508m", "pll3_usb_otg", PFD_480, 2),
	CLK_PFD("pll3_pfd3_454m", "pll3_usb_otg", PFD_480, 3),

	CLK_PLL("pll1", "osc", SYS, PLL_ARM, DIV_SELECT, POWERDOWN, 0),
	CLK_PLL("pll2", "osc", GENERIC, PLL_SYS, DIV_SELECT, POWERDOWN, 0),
	CLK_PLL("pll3", "osc", USB, PLL_USB1, DIV_SELECT, POWER, 0),
	CLK_PLL("pll4", "osc", AUDIO_VIDEO, PLL_AUDIO, DIV_SELECT, POWERDOWN, 0),
	CLK_PLL("pll5", "osc", AUDIO_VIDEO, PLL_VIDEO, DIV_SELECT, POWERDOWN, 0),
	CLK_PLL("pll6", "osc", ENET, PLL_ENET, DIV_SELECT, POWERDOWN, 500000000),
	CLK_PLL("pll7", "osc", USB, PLL_USB2, DIV_SELECT, POWER, 0),

	CLK_DIV("periph_clk2", "periph_clk2_sel", CBCDR, PERIPH_CLK2_PODF),
	CLK_DIV("periph2_clk2", "periph2_clk2_sel", CBCDR, PERIPH2_CLK2_PODF),
	CLK_DIV_BUSY("ocram_podf", "ocram_sel", CBCDR, AXI_PODF, CDHIPR, AXI_PODF_BUSY),
	CLK_DIV("ipg", "ahb", CBCDR, IPG_PODF),
	CLK_DIV("gpu_core_podf", "gpu_core_sel", CBCMR, GPU3D_SHADER_PODF),
	CLK_DIV("gpu_axi_podf", "gpu_axi_sel", CBCMR, GPU3D_CORE_PODF),
	CLK_DIV("lcdif1_podf", "lcdif1_pred", CBCMR, GPU2D_CORE_CLK_PODF),
	CLK_DIV("esai_pred", "esai_sel", CS1CDR, ESAI_CLK_PRED),
	CLK_DIV("esai_podf", "esai_pred", CS1CDR, ESAI_CLK_PODF),
	CLK_DIV("spdif_pred", "spdif_sel", CDCDR, SPDIF0_CLK_PRED),
	CLK_DIV("spdif_podf", "spdif_pred", CDCDR, SPDIF0_CLK_PODF),
	CLK_DIV("audio_pred", "audio_sel", CDCDR, SPDIF1_CLK_PRED),
	CLK_DIV("audio_podf", "audio_pred", CDCDR, SPDIF1_CLK_PODF),
	CLK_DIV("vid_podf", "vid_sel", CSCMR2, VID_CLK_PODF),
	CLK_DIV("can_podf", "can_sel", CSCMR2, CAN_CLK_PODF),
	CLK_DIV("display_podf", "display_sel", CSCDR3, IPU2_HSP_PODF),
	CLK_DIV("csi_podf", "csi_sel", CSCDR3, IPU1_HSP_PODF),
	CLK_DIV("enet_podf", "enet_pre_sel", CHSCCDR, IPU1_DI1_PODF),
	CLK_DIV("m4_podf", "m4_sel", CHSCCDR, IPU1_DI0_PODF),
	CLK_DIV("ecspi_podf", "ecspi_sel", CSCDR2, ECSPI_CLK_PODF),
	CLK_DIV("lcdif1_pred", "lcdif1_pre_sel", CSCDR2, IPU2_DI1_PODF),
	CLK_DIV("lcdif2_pred", "lcdif2_pre_sel", CSCDR2, IPU2_DI0_PODF),
	CLK_DIV("ssi1_pred", "ssi1_sel", CS1CDR, SSI1_CLK_PRED),
	CLK_DIV("ssi1_podf", "ssi1_pred", CS1CDR, SSI1_CLK_PODF),
	CLK_DIV("ssi2_pred", "ssi2_sel", CS2CDR, SSI2_CLK_PRED),
	CLK_DIV("ssi2_podf", "ssi2_pred", CS2CDR, SSI2_CLK_PODF),
	CLK_DIV("ssi3_pred", "ssi3_sel", CS1CDR, SSI3_CLK_PRED),
	CLK_DIV("ssi3_podf", "ssi3_pred", CS1CDR, SSI3_CLK_PODF),
	CLK_DIV("usdhc1_podf", "usdhc1_sel", CSCDR1, USDHC1_PODF),
	CLK_DIV("usdhc2_podf", "usdhc2_sel", CSCDR1, USDHC2_PODF),
	CLK_DIV("usdhc3_podf", "usdhc3_sel", CSCDR1, USDHC3_PODF),
	CLK_DIV("usdhc4_podf", "usdhc4_sel", CSCDR1, USDHC4_PODF),
	CLK_DIV("uart_podf", "uart_sel", CSCDR1, UART_CLK_PODF),
	CLK_DIV("qspi2_pred", "qspi2_sel", CS2CDR, ENFC_CLK_PRED),
	CLK_DIV("qspi2_podf", "qspi2_pred", CS2CDR, ENFC_CLK_PODF),
	CLK_DIV("cko1_podf", "cko1_sel", CCOSR, CLKO1_DIV),
	CLK_DIV("cko2_podf", "cko2_sel", CCOSR, CLKO2_DIV),
	CLK_DIV("qspi1_podf", "qspi1_sel", CSCMR1, QSPI1_PODF),
	CLK_DIV("eim_slow_podf", "eim_slow_sel", CSCMR1, ACLK_EIM_SLOW_PODF),
	CLK_DIV("lcdif2_podf", "lcdif2_pred", CSCMR1, ACLK_PODF),
	CLK_DIV("perclk", "perclk_sel", CSCMR1, PERCLK_PODF),

	CLK_DIV_BUSY("arm", "pll1_sw", CACRR, ARM_PODF, CDHIPR, ARM_PODF_BUSY),
	CLK_DIV_BUSY("ahb", "periph", CBCDR, AHB_PODF, CDHIPR, AHB_PODF_BUSY),
	CLK_DIV_BUSY("mmdc_podf", "periph2", CBCDR, MMDC_CH1_AXI_PODF, CDHIPR, MMDC_CH1_PODF_BUSY),

	CLK_DIV_TABLE("pll4_post_div", "pll4_audio", PLL_AUDIO, POST_DIV_SELECT, post_div_tbl),
	CLK_DIV_TABLE("pll4_audio_div", "pll4_post_div", MISC2, AUDIO_DIV_LSB, audiovideo_div_tbl),
	CLK_DIV_TABLE("pll5_post_div", "pll5_video", PLL_VIDEO, POST_DIV_SELECT, post_div_tbl),
	CLK_DIV_TABLE("pll5_video_div", "pll5_post_div", MISC2, VIDEO_DIV, audiovideo_div_tbl),
	CLK_DIV_TABLE("enet_ref", "pll6_enet", PLL_ENET, DIV_SELECT, enet_ref_tbl),
	CLK_DIV_TABLE("enet2_ref", "pll6_enet", PLL_ENET, DIV2_SELECT, enet_ref_tbl),

	CLK_MUX("step", step_p, CCM, CCSR, STEP_SEL),
	CLK_MUX("pll1_sw", pll1_sw_p, CCM, CCSR, PLL1_SW_CLK_SEL),
	CLK_MUX("ocram_sel", ocram_p, CCM, CBCDR, AXI_SEL),
	CLK_MUX("periph_pre", periph_pre_p, CCM, CBCMR, PRE_PERIPH_CLK_SEL),
	CLK_MUX("periph2_pre", periph2_pre_p, CCM, CBCMR, PRE_PERIPH2_CLK_SEL),
	CLK_MUX("periph_clk2_sel", periph_clk2_p, CCM,CBCMR, PERIPH_CLK2_SEL),
	CLK_MUX("periph2_clk2_sel", periph2_clk2_p, CCM,CBCMR, PERIPH2_CLK2_SEL),
	CLK_MUX("spdif_sel", audio_p, CCM, CDCDR, SPDIF0_CLK_SEL),
	CLK_MUX("audio_sel", audio_p, CCM, CDCDR, SPDIF1_CLK_SEL),
	CLK_MUX("vid_sel", vid_p, CCM, CSCMR2, VID_CLK_SEL),
	CLK_MUX("esai_sel", audio_p, CCM, CSCMR2, ESAI_CLK_SEL),
	CLK_MUX("ldb_di0_div_sel", ldb_di0_div_p, CCM, CSCMR2, LDB_DI0_IPU_DIV),
	CLK_MUX("ldb_di1_div_sel", ldb_di1_div_p, CCM, CSCMR2, LDB_DI1_IPU_DIV),
	CLK_MUX("can_sel", can_p, CCM, CSCMR2, CAN_CLK_SEL),
	CLK_MUX("uart_sel", uart_p, CCM, CSCDR1, UART_CLK_SEL),
	CLK_MUX("enet_pre_sel", enet_pre_p, CCM, CHSCCDR, ENET_PRE_CLK_SEL),
	CLK_MUX("enet_sel", enet_p, CCM, CHSCCDR, ENET_CLK_SEL),
	CLK_MUX("m4_pre_sel", m4_pre_p, CCM, CHSCCDR, M4_PRE_CLK_SEL),
	CLK_MUX("m4_sel", m4_p, CCM, CHSCCDR, M4_CLK_SEL),
	CLK_MUX("ecspi_sel", ecspi_p, CCM, CSCDR2, ECSPI_SEL),
	CLK_MUX("lcdif1_sel", lcdif1_p, CCM, CSCDR2, IPU2_DI1_CLK_SEL),
	CLK_MUX("lcdif1_pre_sel", lcdif1_pre_p, CCM, CSCDR2, IPU2_DI1_PRE_CLK_SEL),
	CLK_MUX("lcdif1_sel", lcdif1_p, CCM, CSCDR2, IPU2_DI1_CLK_SEL),
	CLK_MUX("lcdif2_pre_sel", lcdif2_pre_p, CCM, CSCDR2, IPU2_DI0_PRE_CLK_SEL),
	CLK_MUX("lcdif2_sel", lcdif2_p, CCM, CSCDR2, IPU2_DI0_CLK_SEL),
	CLK_MUX("display_sel", display_p, CCM, CSCDR3, IPU2_HSP_CLK_SEL),
	CLK_MUX("csi_sel", csi_p, CCM, CSCDR3, IPU1_HSP_CLK_SEL),
	CLK_MUX("qspi2_sel", qspi2_p, CCM, CS2CDR, QSPI2_CLK_SEL),
	CLK_MUX("ldb_di0_sel", ldb_di0_p, CCM, CS2CDR, LDB_DI0_CLK_SEL),
	CLK_MUX("ldb_di1_sel", ldb_di1_p, CCM, CS2CDR, LDB_DI1_CLK_SEL),
	CLK_MUX("cko1_sel", cko1_p, CCM, CCOSR, CLKO1_SEL),
	CLK_MUX("cko2_sel", cko2_p, CCM, CCOSR, CLKO2_SEL),
	CLK_MUX("cko", cko_p, CCM, CCOSR, CLK_OUT_SEL),
	CLK_MUX("pcie_axi_sel", pcie_axi_p, CCM, CBCMR, PCIE_AXI_CLK_SEL),
	CLK_MUX("gpu_axi_sel", gpu_axi_p, CCM, CBCMR, GPU3D_SHADER_CLK_SEL),
	CLK_MUX("gpu_core_sel", gpu_core_p, CCM, CBCMR, GPU3D_CORE_CLK_SEL),
	CLK_MUX("ssi1_sel", ssi_p, CCM, CSCMR1, SSI1_CLK_SEL),
	CLK_MUX("ssi2_sel", ssi_p, CCM, CSCMR1, SSI2_CLK_SEL),
	CLK_MUX("ssi3_sel", ssi_p, CCM, CSCMR1, SSI3_CLK_SEL),
	CLK_MUX("usdhc1_sel", usdhc_p, CCM, CSCMR1, USDHC1_CLK_SEL),
	CLK_MUX("usdhc2_sel", usdhc_p, CCM, CSCMR1, USDHC2_CLK_SEL),
	CLK_MUX("usdhc3_sel", usdhc_p, CCM, CSCMR1, USDHC3_CLK_SEL),
	CLK_MUX("usdhc4_sel", usdhc_p, CCM, CSCMR1, USDHC4_CLK_SEL),
	CLK_MUX("qspi1_sel", qspi1_p, CCM, CSCMR1, QSOI1_SEL),
	CLK_MUX("perclk_sel", perclk_p, CCM, CSCMR1, PERCLK_SEL),
	CLK_MUX("eim_slow_sel", eim_slow_p, CCM, CSCMR1, ACLK_EIM_SLOW_SEL),
	CLK_MUX("pll1_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_ARM, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll2_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_SYS, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll3_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_USB1, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll4_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_AUDIO, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll5_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_VIDEO, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll6_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_ENET, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll7_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_USB2, BYPASS_CLK_SRC_6SX),
	CLK_MUX("pll1_bypass", pll1_bypass_p, CCM_ANALOG, PLL_ARM, BYPASS),
	CLK_MUX("pll2_bypass", pll2_bypass_p, CCM_ANALOG, PLL_SYS, BYPASS),
	CLK_MUX("pll3_bypass", pll3_bypass_p, CCM_ANALOG, PLL_USB1, BYPASS),
	CLK_MUX("pll4_bypass", pll4_bypass_p, CCM_ANALOG, PLL_AUDIO, BYPASS),
	CLK_MUX("pll5_bypass", pll5_bypass_p, CCM_ANALOG, PLL_VIDEO, BYPASS),
	CLK_MUX("pll6_bypass", pll6_bypass_p, CCM_ANALOG, PLL_ENET, BYPASS),
	CLK_MUX("pll7_bypass", pll7_bypass_p, CCM_ANALOG, PLL_USB2, BYPASS),

	CLK_MUX("lvds1_sel", lvds_p, CCM_ANALOG, MISC1, LVDS_CLK1_SRC),
	CLK_MUX("lvds2_sel", lvds_p, CCM_ANALOG, MISC1, LVDS_CLK2_SRC),

	CLK_MUX_BUSY("periph", periph_p, CBCDR, PERIPH_CLK_SEL, CDHIPR, PERIPH_CLK_SEL_BUSY),
	CLK_MUX_BUSY("periph2", periph2_p, CBCDR, PERIPH2_CLK_SEL, CDHIPR, PERIPH2_CLK_SEL_BUSY),

	CLK_GATE("aips_tz1", "ahb", CCM, CCGR0, AIPS_TZ1_CLK_ENABLE),
	CLK_GATE("aips_tz2", "ahb", CCM, CCGR0, AIPS_TZ2_CLK_ENABLE),
	CLK_GATE("apbh_dma", "usdhc3", CCM, CCGR0, APBHDMA_HCLK_ENABLE),
	CLK_GATE("asrc_ipg", "ahb", CCM, CCGR0, ASRC_CLK_ENABLE),
	CLK_GATE("asrc_mem", "ahb", CCM, CCGR0, ASRC_CLK_ENABLE),
	CLK_GATE("caam_mem", "ahb", CCM, CCGR0, CAAM_SECURE_MEM_CLK_ENABLE),
	CLK_GATE("caam_aclk", "ahb", CCM, CCGR0, CAAM_WRAPPER_ACLK_ENABLE),
	CLK_GATE("caam_ipg", "ipg", CCM, CCGR0, CAAM_WRAPPER_IPG_ENABLE),
	CLK_GATE("can1_ipg", "ipg", CCM, CCGR0, CAN1_CLK_ENABLE),
	CLK_GATE("can1_serial", "can_podf", CCM, CCGR0, CAN1_SERIAL_CLK_ENABLE),
	CLK_GATE("can2_ipg", "ipg", CCM, CCGR0, CAN2_CLK_ENABLE),
	CLK_GATE("can2_serial", "can_podf", CCM, CCGR0, CAN2_SERIAL_CLK_ENABLE),
	CLK_GATE("dcic1", "display_podf", CCM, CCGR0, DCIC1_CLK_ENABLE),
	CLK_GATE("dcic2", "display_podf", CCM, CCGR0, DCIC2_CLK_ENABLE),
	CLK_GATE("aips_tz3", "ahb", CCM, CCGR0, TZ3_CLK_ENABLE),
	CLK_GATE("ecspi1", "ecspi_podf", CCM, CCGR1, ECSPI1_CLK_ENABLE),
	CLK_GATE("ecspi2", "ecspi_podf", CCM, CCGR1, ECSPI2_CLK_ENABLE),
	CLK_GATE("ecspi3", "ecspi_podf", CCM, CCGR1, ECSPI3_CLK_ENABLE),
	CLK_GATE("ecspi4", "ecspi_podf", CCM, CCGR1, ECSPI4_CLK_ENABLE),
	CLK_GATE("ecspi5", "ecspi_podf", CCM, CCGR1, ECSPI5_CLK_ENABLE),
	CLK_GATE("epit1", "perclk", CCM, CCGR1, EPIT1_CLK_ENABLE),
	CLK_GATE("epit2", "perclk", CCM, CCGR1, EPIT2_CLK_ENABLE),
	CLK_GATE("esai_extal", "esai_podf", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("esai_ipg", "ahb", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("esai_mem", "ahb", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("wakeup", "ipg", CCM, CCGR1, WAKEUP_CLK_ENABLE),
	CLK_GATE("gpt_bus", "perclk", CCM, CCGR1, GPT_CLK_ENABLE),
	CLK_GATE("gpt_serial", "perclk", CCM, CCGR1, GPT_SERIAL_CLK_ENABLE),
	CLK_GATE("gpu", "gpu_core_podf", CCM, CCGR1, GPU3D_CLK_ENABLE),
	CLK_GATE("ocram_s", "ahb", CCM, CCGR1, OCRAM_CLK_ENABLE),
	CLK_GATE("canfd", "can_podf", CCM, CCGR1, CANFD_CLK_ENABLE),
	CLK_GATE("csi", "csi_podf", CCM, CCGR2, CSI_CLK_ENABLE),
	CLK_GATE("i2c1", "perclk", CCM, CCGR2, I2C1_SERIAL_CLK_ENABLE),
	CLK_GATE("i2c2", "perclk", CCM, CCGR2, I2C2_SERIAL_CLK_ENABLE),
	CLK_GATE("i2c3", "perclk", CCM, CCGR2, I2C3_SERIAL_CLK_ENABLE),
	CLK_GATE("ocotp", "ipg", CCM, CCGR2, IIM_CLK_ENABLE),
	CLK_GATE("iomuxc", "lcdif1_podf", CCM, CCGR2, IOMUX_IPT_CLK_IO_CLK_ENABLE),
	CLK_GATE("ipmux1", "ahb", CCM, CCGR2, IPMUX1_CLK_ENABLE),
	CLK_GATE("ipmux2", "ahb", CCM, CCGR2, IPMUX2_CLK_ENABLE),
	CLK_GATE("ipmux3", "ahb", CCM, CCGR2, IPMUX3_CLK_ENABLE),
	CLK_GATE("tzasc1", "mmdc_podf", CCM, CCGR2, IPSYNC_IP2APB_TZASC1_IPG_CLK_ENABLE),
	CLK_GATE("lcdif_apb", "display_podf", CCM, CCGR2, LCDIF_APB_CLK_ENABLE),
	CLK_GATE("pxp_axi", "display_podf", CCM, CCGR2, PXP_AXI_CLK_ENABLE),
	CLK_GATE("enet", "ipg", CCM, CCGR3, IPU1_IPU_DI1_CLK_ENABLE),
	CLK_GATE("enet_ahb", "enet_sel", CCM, CCGR3, IPU1_IPU_DI1_CLK_ENABLE),
	CLK_GATE("m4", "m4_podf", CCM, CCGR3, IPU1_IPU_DI0_CLK_ENABLE),
	CLK_GATE("display_axi", "display_podf", CCM, CCGR3, IPU2_IPU_CLK_ENABLE),
	CLK_GATE("lcdif2_pix", "lcdif2_sel", CCM, CCGR3, IPU2_IPU_DI0_CLK_ENABLE),
	CLK_GATE("lcdif1_pix", "lcdif1_sel", CCM, CCGR3, IPU2_IPU_DI1_CLK_ENABLE),
	CLK_GATE("ldb_di0", "ldb_di0_div_sel", CCM, CCGR3, LDB_DI0_CLK_ENABLE),
	CLK_GATE("qspi1", "qspi1_podf", CCM, CCGR3, LDB_DI1_CLK_ENABLE),
	CLK_GATE("mlb", "ahb", CCM, CCGR3, MLB_CLK_ENABLE),
	CLK_GATE("mmdc_p0_fast", "mmdc_podf", CCM, CCGR3, MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE),
	CLK_GATE("mmdc_p0_ipg", "ipg", CCM, CCGR3, MMDC_CORE_IPG_CLK_P0_ENABLE),
	CLK_GATE("mmdc_p1_ipg", "ipg", CCM, CCGR3, MMDC_P1_IPG_CLK_ENABLE),
	CLK_GATE("ocram", "ahb", CCM, CCGR3, OCRAM_CLK_ENABLE),
	CLK_GATE("pcie_axi", "display_podf", CCM, CCGR4, PCIE_ROOT_ENABLE),
	CLK_GATE("qspi2", "qspi2_podf", CCM, CCGR4, QSPI2_ENABLE),
	CLK_GATE("per1_bch", "usdhc3", CCM, CCGR4, PL301_MX6QPER1_BCHCLK_ENABLE),
	CLK_GATE("per2_main", "ahb", CCM, CCGR4, PL301_MX6QPER2_MAINCLK_ENABLE),
	CLK_GATE("pwm1", "perclk", CCM, CCGR4, PWM1_CLK_ENABLE),
	CLK_GATE("pwm2", "perclk", CCM, CCGR4, PWM2_CLK_ENABLE),
	CLK_GATE("pwm3", "perclk", CCM, CCGR4, PWM3_CLK_ENABLE),
	CLK_GATE("pwm4", "perclk", CCM, CCGR4, PWM4_CLK_ENABLE),
	CLK_GATE("gpmi_bch_apb", "usdhc3", CCM, CCGR4, RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE),
	CLK_GATE("gpmi_bch", "usdhc4", CCM, CCGR4, RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE),
	CLK_GATE("gpmi_io", "qspi2_podf", CCM, CCGR4, RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE),
	CLK_GATE("gpmi_apb", "usdhc3", CCM, CCGR4, RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE),
	CLK_GATE("rom", "ahb", CCM, CCGR5, ROM_CLK_ENABLE),
	CLK_GATE("sdma", "ahb", CCM, CCGR5, SDMA_CLK_ENABLE),
	CLK_GATE("spba", "ipg", CCM, CCGR5, SPBA_CLK_ENABLE),
	CLK_GATE("audio", "audio_podf", CCM, CCGR5, SPDIF_CLK_ENABLE),
	CLK_GATE("spdif", "spdif_podf", CCM, CCGR5, SPDIF_CLK_ENABLE),
	CLK_GATE("spdif_gclk", "ipg", CCM, CCGR5, SPDIF_CLK_ENABLE),
	CLK_GATE("ssi1_ipg", "ipg", CCM, CCGR5, SSI1_CLK_ENABLE),
	CLK_GATE("ssi2_ipg", "ipg", CCM, CCGR5, SSI2_CLK_ENABLE),
	CLK_GATE("ssi3_ipg", "ipg", CCM, CCGR5, SSI3_CLK_ENABLE),
	CLK_GATE("ssi1", "ssi1_podf", CCM, CCGR5, SSI1_CLK_ENABLE),
	CLK_GATE("ssi2", "ssi2_podf", CCM, CCGR5, SSI2_CLK_ENABLE),
	CLK_GATE("ssi3", "ssi3_podf", CCM, CCGR5, SSI3_CLK_ENABLE),
	CLK_GATE("uart_ipg", "ipg", CCM, CCGR5, UART_CLK_ENABLE),
	CLK_GATE("uart_serial", "uart_podf", CCM, CCGR5, UART_SERIAL_CLK_ENABLE),
	CLK_GATE("sai1", "ssi1_podf", CCM, CCGR5, SAI1_ENABLE),
	CLK_GATE("sai2", "ssi2_podf", CCM, CCGR5, SAI2_ENABLE),
	CLK_GATE("sai1_ipg", "ipg", CCM, CCGR5, SAI1_ENABLE),
	CLK_GATE("sai2_ipg", "ipg", CCM, CCGR5, SAI2_ENABLE),
	CLK_GATE("usboh3", "ipg", CCM, CCGR6, USBOH3_CLK_ENABLE),
	CLK_GATE("usdhc1", "usdhc1_podf", CCM, CCGR6, USDHC1_CLK_ENABLE),
	CLK_GATE("usdhc2", "usdhc2_podf", CCM, CCGR6, USDHC2_CLK_ENABLE),
	CLK_GATE("usdhc3", "usdhc3_podf", CCM, CCGR6, USDHC3_CLK_ENABLE),
	CLK_GATE("usdhc4", "usdhc4_podf", CCM, CCGR6, USDHC4_CLK_ENABLE),
	CLK_GATE("eim_slow", "eim_slow_podf", CCM, CCGR6, EIM_SLOW_CLK_ENABLE),
	CLK_GATE("pwm8", "perclk", CCM, CCGR6, PWM8_CLK_ENABLE),
	CLK_GATE("vadc", "vid_podf", CCM, CCGR6, VADC_CLK_ENABLE),
	CLK_GATE("gis", "display_podf", CCM, CCGR6, GIS_CLK_ENABLE),
	CLK_GATE("i2c4", "perclk", CCM, CCGR6, I2CS4_CLK_ENABLE),
	CLK_GATE("pwm5", "perclk", CCM, CCGR6, PWM5_CLK_ENABLE),
	CLK_GATE("pwm6", "perclk", CCM, CCGR6, PWM6_CLK_ENABLE),
	CLK_GATE("pwm7", "perclk", CCM, CCGR6, PWM7_CLK_ENABLE),
	CLK_GATE("cko1", "cko1_podf", CCM, CCOSR, CLKO1_EN),
	CLK_GATE("cko2", "cko2_podf", CCM, CCOSR, CLKO2_EN),
	CLK_GATE("enet_ptp_25m", "enet_ptp_ref", CCM_ANALOG, PLL_ENET, ENET_25M_REF_EN),
	CLK_GATE("enet2_ref_125m", "enet2_ref", CCM_ANALOG, PLL_ENET, ENABLE_100M),
	CLK_GATE("pcie_ref_125m", "pcie_ref", CCM_ANALOG, PLL_ENET, ENABLE_125M),
	CLK_GATE("pll1_sys", "pll1_bypass", CCM_ANALOG, PLL_ARM, ENABLE),
	CLK_GATE("pll2_bus", "pll2_bypass", CCM_ANALOG, PLL_SYS, ENABLE),
	CLK_GATE("pll3_usb_otg", "pll3_bypass", CCM_ANALOG, PLL_USB1, ENABLE),
	CLK_GATE("pll4_audio", "pll4_bypass", CCM_ANALOG, PLL_AUDIO, ENABLE),
	CLK_GATE("pll5_video", "pll5_bypass", CCM_ANALOG, PLL_VIDEO, ENABLE),
	CLK_GATE("pll6_enet", "pll6_bypass", CCM_ANALOG, PLL_ENET, ENABLE),
	CLK_GATE("pll7_usb_host", "pll7_bypass", CCM_ANALOG, PLL_USB2, ENABLE),

	CLK_GATE("usbphy1", "pll3_usb_otg", CCM_ANALOG, PLL_USB1, RESERVED),
	CLK_GATE("usbphy2", "pll7_usb_host", CCM_ANALOG, PLL_USB2, RESERVED),
	CLK_GATE("usbphy1_gate", "dummy", CCM_ANALOG, PLL_USB2, EN_USB_CLK),
	CLK_GATE("usbphy2_gate", "dummy", CCM_ANALOG, PLL_USB2, EN_USB_CLK),

	CLK_GATE_EXCLUSIVE("lvds2_out", "lvds2_sel", CCM_ANALOG, MISC1, LVDS_CLK2_OBEN, LVDS_CLK2_IBEN),
	CLK_GATE_EXCLUSIVE("lvds2_in", "anaclk2", CCM_ANALOG, MISC1, LVDS_CLK2_IBEN, LVDS_CLK2_OBEN),
	CLK_GATE_EXCLUSIVE("lvds1_in", "anaclk1", CCM_ANALOG, MISC1, LVDS_CLK1_IBEN, LVDS_CLK1_OBEN),
};

struct imxccm_init_parent imx6sxccm_init_parents[] = {
	{ "pll1_bypass",        "pll1" },
	{ "pll2_bypass",        "pll2" },
	{ "pll3_bypass",        "pll3" },
	{ "pll4_bypass",        "pll4" },
	{ "pll5_bypass",        "pll5" },
	{ "pll6_bypass",        "pll6" },
	{ "pll7_bypass",        "pll7" },
	{ "lvds1_sel",          "pcie_ref_125m" },
	{ 0 },
};

static struct imx6_clk *
imx6sx_clk_find_by_id(struct imx6ccm_softc *sc, u_int clock_id)
{
	for (int n = 0; n < __arraycount(imx6sx_clock_ids); n++) {
		if (imx6sx_clock_ids[n].id == clock_id) {
			const char *name = imx6sx_clock_ids[n].name;
			return imx6_clk_find(sc, name);
		}
	}

	return NULL;
}

static struct clk *
imx6sx_get_clock_by_id(struct imx6ccm_softc *sc, u_int clock_id)
{
	struct imx6_clk *iclk;
	iclk = imx6sx_clk_find_by_id(sc, clock_id);

	if (iclk == NULL)
		return NULL;

	return &iclk->base;
}

static struct clk *imx6sx_clk_decode(device_t, int, const void *, size_t);

static const struct fdtbus_clock_controller_func imx6sx_ccm_fdtclock_funcs = {
	.decode = imx6sx_clk_decode
};

static struct clk *
imx6sx_clk_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct clk *clk;
	struct imx6ccm_softc *sc = device_private(dev);

	/* #clock-cells should be 1 */
	if (len != 4)
		return NULL;

	const u_int clock_id = be32dec(data);

	clk = imx6sx_get_clock_by_id(sc, clock_id);
	if (clk)
		return clk;

	return NULL;
}

static void
imx6sx_clk_fixed_from_fdt(struct imx6ccm_softc *sc, const char *name)
{
	struct imx6_clk *iclk = (struct imx6_clk *)imx6_get_clock(sc, name);

	KASSERTMSG((iclk != NULL), "failed to find clock %s", name);

	char *path = kmem_asprintf("/clock-%s", name);
	/* in device tree path, '_' are remplaced by '-' */
	for (char *p = path; *p != '\0'; p++) {
		if (*p == '_')
			*p = '-';
	}
	int phandle = OF_finddevice(path);
	KASSERTMSG((phandle >= 0), "failed to find device %s", path);
	kmem_free(path, strlen(path) + 1);

	if (of_getprop_uint32(phandle, "clock-frequency", &iclk->clk.fixed.rate) != 0)
		iclk->clk.fixed.rate = 0;
}

static int imx6sxccm_match(device_t, cfdata_t, void *);
static void imx6sxccm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx6sxccm, sizeof(struct imx6ccm_softc),
    imx6sxccm_match, imx6sxccm_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6sx-ccm" },
	DEVICE_COMPAT_EOL
};

static int
imx6sxccm_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx6sxccm_attach(device_t parent, device_t self, void *aux)
{
	struct imx6ccm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error(": can't map ccm registers\n");
		return;
	}

	int phandle = of_find_bycompat(OF_finddevice("/"), "fsl,imx6sx-anatop");

	if (phandle == -1) {
		aprint_error(": can't find anatop device\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": can't get anatop registers\n");
		return;
	}
		
	
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh_analog)) {
		aprint_error(": can't map anatop registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Clock Control Module\n");

	imx6ccm_attach_common(self, &imx6sx_clks[0], __arraycount(imx6sx_clks),
	    imx6sxccm_init_parents);

	imx6sx_clk_fixed_from_fdt(sc, "ckil");
	imx6sx_clk_fixed_from_fdt(sc, "osc");
	imx6sx_clk_fixed_from_fdt(sc, "ipp_di0");
	imx6sx_clk_fixed_from_fdt(sc, "ipp_di1");
	imx6sx_clk_fixed_from_fdt(sc, "anaclk1");
	imx6sx_clk_fixed_from_fdt(sc, "anaclk2");

	fdtbus_register_clock_controller(self, faa->faa_phandle,
	    &imx6sx_ccm_fdtclock_funcs);
}
