/*	$NetBSD: imx6_ccmvar.h,v 1.1 2020/12/23 14:42:38 skrll Exp $	*/
/*
 * Copyright (c) 2012,2019  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_NXP_IMX6_CCMVAR_H_
#define	_ARM_NXP_IMX6_CCMVAR_H_

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

struct imx6ccm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ioh_analog;

	struct clk_domain sc_clkdom;
};

void imx6ccm_attach_common(device_t);

struct clk *imx6_get_clock(const char *);
struct clk *imx6_get_clock_by_id(u_int);

/* Clock IDs */
#define IMX6CLK_DUMMY			0
#define IMX6CLK_CKIL			1
#define IMX6CLK_CKIH			2
#define IMX6CLK_OSC			3
#define IMX6CLK_PLL2_PFD0_352M		4
#define IMX6CLK_PLL2_PFD1_594M		5
#define IMX6CLK_PLL2_PFD2_396M		6
#define IMX6CLK_PLL3_PFD0_720M		7
#define IMX6CLK_PLL3_PFD1_540M		8
#define IMX6CLK_PLL3_PFD2_508M		9
#define IMX6CLK_PLL3_PFD3_454M		10
#define IMX6CLK_PLL2_198M		11
#define IMX6CLK_PLL3_120M		12
#define IMX6CLK_PLL3_80M		13
#define IMX6CLK_PLL3_60M		14
#define IMX6CLK_TWD			15
#define IMX6CLK_STEP			16
#define IMX6CLK_PLL1_SW			17
#define IMX6CLK_PERIPH_PRE		18
#define IMX6CLK_PERIPH2_PRE		19
#define IMX6CLK_PERIPH_CLK2_SEL		20
#define IMX6CLK_PERIPH2_CLK2_SEL	21
#define IMX6CLK_AXI_SEL			22
#define IMX6CLK_ESAI_SEL		23
#define IMX6CLK_ASRC_SEL		24
#define IMX6CLK_SPDIF_SEL		25
#define IMX6CLK_GPU2D_AXI		26
#define IMX6CLK_GPU3D_AXI		27
#define IMX6CLK_GPU2D_CORE_SEL		28
#define IMX6CLK_GPU3D_CORE_SEL		29
#define IMX6CLK_GPU3D_SHADER_SEL	30
#define IMX6CLK_IPU1_SEL		31
#define IMX6CLK_IPU2_SEL		32
#define IMX6CLK_LDB_DI0_SEL		33
#define IMX6CLK_LDB_DI1_SEL		34
#define IMX6CLK_IPU1_DI0_PRE_SEL	35
#define IMX6CLK_IPU1_DI1_PRE_SEL	36
#define IMX6CLK_IPU2_DI0_PRE_SEL	37
#define IMX6CLK_IPU2_DI1_PRE_SEL	38
#define IMX6CLK_IPU1_DI0_SEL		39
#define IMX6CLK_IPU1_DI1_SEL		40
#define IMX6CLK_IPU2_DI0_SEL		41
#define IMX6CLK_IPU2_DI1_SEL		42
#define IMX6CLK_HSI_TX_SEL		43
#define IMX6CLK_PCIE_AXI_SEL		44
#define IMX6CLK_SSI1_SEL		45
#define IMX6CLK_SSI2_SEL		46
#define IMX6CLK_SSI3_SEL		47
#define IMX6CLK_USDHC1_SEL		48
#define IMX6CLK_USDHC2_SEL		49
#define IMX6CLK_USDHC3_SEL		50
#define IMX6CLK_USDHC4_SEL		51
#define IMX6CLK_ENFC_SEL		52
#define IMX6CLK_EIM_SEL			53
#define IMX6CLK_EIM_SLOW_SEL		54
#define IMX6CLK_VDO_AXI_SEL		55
#define IMX6CLK_VPU_AXI_SEL		56
#define IMX6CLK_CKO1_SEL		57
#define IMX6CLK_PERIPH			58
#define IMX6CLK_PERIPH2			59
#define IMX6CLK_PERIPH_CLK2		60
#define IMX6CLK_PERIPH2_CLK2		61
#define IMX6CLK_IPG			62
#define IMX6CLK_IPG_PER			63
#define IMX6CLK_ESAI_PRED		64
#define IMX6CLK_ESAI_PODF		65
#define IMX6CLK_ASRC_PRED		66
#define IMX6CLK_ASRC_PODF		67
#define IMX6CLK_SPDIF_PRED		68
#define IMX6CLK_SPDIF_PODF		69
#define IMX6CLK_CAN_ROOT		70
#define IMX6CLK_ECSPI_ROOT		71
#define IMX6CLK_GPU2D_CORE_PODF		72
#define IMX6CLK_GPU3D_CORE_PODF		73
#define IMX6CLK_GPU3D_SHADER		74
#define IMX6CLK_IPU1_PODF		75
#define IMX6CLK_IPU2_PODF		76
#define IMX6CLK_LDB_DI0_PODF		77
#define IMX6CLK_LDB_DI1_PODF		78
#define IMX6CLK_IPU1_DI0_PRE		79
#define IMX6CLK_IPU1_DI1_PRE		80
#define IMX6CLK_IPU2_DI0_PRE		81
#define IMX6CLK_IPU2_DI1_PRE		82
#define IMX6CLK_HSI_TX_PODF		83
#define IMX6CLK_SSI1_PRED		84
#define IMX6CLK_SSI1_PODF		85
#define IMX6CLK_SSI2_PRED		86
#define IMX6CLK_SSI2_PODF		87
#define IMX6CLK_SSI3_PRED		88
#define IMX6CLK_SSI3_PODF		89
#define IMX6CLK_UART_SERIAL_PODF	90
#define IMX6CLK_USDHC1_PODF		91
#define IMX6CLK_USDHC2_PODF		92
#define IMX6CLK_USDHC3_PODF		93
#define IMX6CLK_USDHC4_PODF		94
#define IMX6CLK_ENFC_PRED		95
#define IMX6CLK_ENFC_PODF		96
#define IMX6CLK_EIM_PODF		97
#define IMX6CLK_EIM_SLOW_PODF		98
#define IMX6CLK_VPU_AXI_PODF		99
#define IMX6CLK_CKO1_PODF		100
#define IMX6CLK_AXI			101
#define IMX6CLK_MMDC_CH0_AXI_PODF	102
#define IMX6CLK_MMDC_CH1_AXI_PODF	103
#define IMX6CLK_ARM			104
#define IMX6CLK_AHB			105
#define IMX6CLK_APBH_DMA		106
#define IMX6CLK_ASRC			107
#define IMX6CLK_CAN1_IPG		108
#define IMX6CLK_CAN1_SERIAL		109
#define IMX6CLK_CAN2_IPG		110
#define IMX6CLK_CAN2_SERIAL		111
#define IMX6CLK_ECSPI1			112
#define IMX6CLK_ECSPI2			113
#define IMX6CLK_ECSPI3			114
#define IMX6CLK_ECSPI4			115
#define IMX6CLK_ECSPI5			116	/* i.MX6Q */
#define IMX6CLK_I2C4			116	/* i.MX6DL */
#define IMX6CLK_ENET			117
#define IMX6CLK_ESAI_EXTAL		118
#define IMX6CLK_GPT_IPG			119
#define IMX6CLK_GPT_IPG_PER		120
#define IMX6CLK_GPU2D_CORE		121
#define IMX6CLK_GPU3D_CORE		122
#define IMX6CLK_HDMI_IAHB		123
#define IMX6CLK_HDMI_ISFR		124
#define IMX6CLK_I2C1			125
#define IMX6CLK_I2C2			126
#define IMX6CLK_I2C3			127
#define IMX6CLK_IIM			128
#define IMX6CLK_ENFC			129
#define IMX6CLK_IPU1			130
#define IMX6CLK_IPU1_DI0		131
#define IMX6CLK_IPU1_DI1		132
#define IMX6CLK_IPU2			133
#define IMX6CLK_IPU2_DI0		134
#define IMX6CLK_LDB_DI0			135
#define IMX6CLK_LDB_DI1			136
#define IMX6CLK_IPU2_DI1		137
#define IMX6CLK_HSI_TX			138
#define IMX6CLK_MLB			139
#define IMX6CLK_MMDC_CH0_AXI		140
#define IMX6CLK_MMDC_CH1_AXI		141
#define IMX6CLK_OCRAM			142
#define IMX6CLK_OPENVG_AXI		143
#define IMX6CLK_PCIE_AXI		144
#define IMX6CLK_PWM1			145
#define IMX6CLK_PWM2			146
#define IMX6CLK_PWM3			147
#define IMX6CLK_PWM4			148
#define IMX6CLK_PER1_BCH		149
#define IMX6CLK_GPMI_BCH_APB		150
#define IMX6CLK_GPMI_BCH		151
#define IMX6CLK_GPMI_IO			152
#define IMX6CLK_GPMI_APB		153
#define IMX6CLK_SATA			154
#define IMX6CLK_SDMA			155
#define IMX6CLK_SPBA			156
#define IMX6CLK_SSI1			157
#define IMX6CLK_SSI2			158
#define IMX6CLK_SSI3			159
#define IMX6CLK_UART_IPG		160
#define IMX6CLK_UART_SERIAL		161
#define IMX6CLK_USBOH3			162
#define IMX6CLK_USDHC1			163
#define IMX6CLK_USDHC2			164
#define IMX6CLK_USDHC3			165
#define IMX6CLK_USDHC4			166
#define IMX6CLK_VDO_AXI			167
#define IMX6CLK_VPU_AXI			168
#define IMX6CLK_CKO1			169
#define IMX6CLK_PLL1_SYS		170
#define IMX6CLK_PLL2_BUS		171
#define IMX6CLK_PLL3_USB_OTG		172
#define IMX6CLK_PLL4_AUDIO		173
#define IMX6CLK_PLL5_VIDEO		174
#define IMX6CLK_PLL8_MLB		175
#define IMX6CLK_PLL7_USB_HOST		176
#define IMX6CLK_PLL6_ENET		177
#define IMX6CLK_SSI1_IPG		178
#define IMX6CLK_SSI2_IPG		179
#define IMX6CLK_SSI3_IPG		180
#define IMX6CLK_ROM			181
#define IMX6CLK_USBPHY1			182
#define IMX6CLK_USBPHY2			183
#define IMX6CLK_LDB_DI0_DIV_3_5		184
#define IMX6CLK_LDB_DI1_DIV_3_5		185
#define IMX6CLK_SATA_REF		186
#define IMX6CLK_SATA_REF_100M		187
#define IMX6CLK_PCIE_REF		188
#define IMX6CLK_PCIE_REF_125M		189
#define IMX6CLK_ENET_REF		190
#define IMX6CLK_USBPHY1_GATE		191
#define IMX6CLK_USBPHY2_GATE		192
#define IMX6CLK_PLL4_POST_DIV		193
#define IMX6CLK_PLL5_POST_DIV		194
#define IMX6CLK_PLL5_VIDEO_DIV		195
#define IMX6CLK_EIM_SLOW		196
#define IMX6CLK_SPDIF			197
#define IMX6CLK_CKO2_SEL		198
#define IMX6CLK_CKO2_PODF		199
#define IMX6CLK_CKO2			200
#define IMX6CLK_CKO			201
#define IMX6CLK_VDOA			202
#define IMX6CLK_PLL4_AUDIO_DIV		203
#define IMX6CLK_LVDS1_SEL		204
#define IMX6CLK_LVDS2_SEL		205
#define IMX6CLK_LVDS1_GATE		206
#define IMX6CLK_LVDS2_GATE		207
#define IMX6CLK_ESAI_IPG		208
#define IMX6CLK_ESAI_MEM		209
#define IMX6CLK_ASRC_IPG		210
#define IMX6CLK_ASRC_MEM		211
#define IMX6CLK_LVDS1_IN		212
#define IMX6CLK_LVDS2_IN		213
#define IMX6CLK_ANACLK1			214
#define IMX6CLK_ANACLK2			215
#define IMX6CLK_PLL1_BYPASS_SRC		216
#define IMX6CLK_PLL2_BYPASS_SRC		217
#define IMX6CLK_PLL3_BYPASS_SRC		218
#define IMX6CLK_PLL4_BYPASS_SRC		219
#define IMX6CLK_PLL5_BYPASS_SRC		220
#define IMX6CLK_PLL6_BYPASS_SRC		221
#define IMX6CLK_PLL7_BYPASS_SRC		222
#define IMX6CLK_PLL1			223
#define IMX6CLK_PLL2			224
#define IMX6CLK_PLL3			225
#define IMX6CLK_PLL4			226
#define IMX6CLK_PLL5			227
#define IMX6CLK_PLL6			228
#define IMX6CLK_PLL7			229
#define IMX6CLK_PLL1_BYPASS		230
#define IMX6CLK_PLL2_BYPASS		231
#define IMX6CLK_PLL3_BYPASS		232
#define IMX6CLK_PLL4_BYPASS		233
#define IMX6CLK_PLL5_BYPASS		234
#define IMX6CLK_PLL6_BYPASS		235
#define IMX6CLK_PLL7_BYPASS		236
#define IMX6CLK_GPT_3M			237
#define IMX6CLK_VIDEO_27M		238
#define IMX6CLK_MIPI_CORE_CFG		239
#define IMX6CLK_MIPI_IPG		240
#define IMX6CLK_CAAM_MEM		241
#define IMX6CLK_CAAM_ACLK		242
#define IMX6CLK_CAAM_IPG		243
#define IMX6CLK_SPDIF_GCLK		244
#define IMX6CLK_UART_SEL		245
#define IMX6CLK_IPG_PER_SEL		246
#define IMX6CLK_ECSPI_SEL		247
#define IMX6CLK_CAN_SEL			248
#define IMX6CLK_MMDC_CH1_AXI_CG		249
#define IMX6CLK_PRE0			250
#define IMX6CLK_PRE1			251
#define IMX6CLK_PRE2			252
#define IMX6CLK_PRE3			253
#define IMX6CLK_PRG0_AXI		254
#define IMX6CLK_PRG1_AXI		255
#define IMX6CLK_PRG0_APB		256
#define IMX6CLK_PRG1_APB		257
#define IMX6CLK_PRE_AXI			258
#define IMX6CLK_MLB_SEL			259
#define IMX6CLK_MLB_PODF		260
#define IMX6CLK_END			261

enum imx6_clk_type {
	IMX6_CLK_FIXED,
	IMX6_CLK_FIXED_FACTOR,
	IMX6_CLK_PLL,
	IMX6_CLK_MUX,
	IMX6_CLK_GATE,
	IMX6_CLK_PFD,
	IMX6_CLK_DIV,
};

enum imx6_clk_reg {
	IMX6_CLK_REG_CCM,
	IMX6_CLK_REG_CCM_ANALOG,
};

enum imx6_clk_pll_type {
	IMX6_CLK_PLL_GENERIC,
	IMX6_CLK_PLL_SYS,
	IMX6_CLK_PLL_USB,
	IMX6_CLK_PLL_AUDIO_VIDEO,
	IMX6_CLK_PLL_ENET,
};

enum imx6_clk_div_type {
	IMX6_CLK_DIV_NORMAL,
	IMX6_CLK_DIV_BUSY,
	IMX6_CLK_DIV_TABLE,
};

enum imx6_clk_mux_type {
	IMX6_CLK_MUX_NORMAL,
	IMX6_CLK_MUX_BUSY,
};

struct imx6_clk_fixed {
	u_int rate;
};

struct imx6_clk_fixed_factor {
	u_int div;
	u_int mult;
};

struct imx6_clk_pfd {
	uint32_t reg;
	int index;
};

struct imx6_clk_pll {
	enum imx6_clk_pll_type type;
	uint32_t reg;
	uint32_t mask;
	uint32_t powerdown;
	unsigned long ref;
};

struct imx6_clk_div {
	enum imx6_clk_div_type type;
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	uint32_t busy_reg;
	uint32_t busy_mask;
	const int *tbl;
};

struct imx6_clk_mux {
	enum imx6_clk_mux_type type;
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	const char **parents;
	u_int nparents;
	uint32_t busy_reg;
	uint32_t busy_mask;
};

struct imx6_clk_gate {
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	uint32_t exclusive_mask;
};

struct imx6_clk {
	struct clk base;		/* must be first */

	const char *parent;
	u_int refcnt;

	enum imx6_clk_type type;
	union {
		struct imx6_clk_fixed fixed;
		struct imx6_clk_fixed_factor fixed_factor;
		struct imx6_clk_pfd pfd;
		struct imx6_clk_pll pll;
		struct imx6_clk_div div;
		struct imx6_clk_mux mux;
		struct imx6_clk_gate gate;
	} clk;
};

#define CLK_FIXED(_name, _rate) {				\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_FIXED,					\
	.clk = {						\
		.fixed = {					\
			.rate = (_rate),			\
		}						\
	}							\
}

#define CLK_FIXED_FACTOR(_name, _parent, _div, _mult) {		\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_FIXED_FACTOR,				\
	.parent = (_parent),					\
	.clk = {						\
		.fixed_factor = {				\
			.div = (_div),				\
			.mult = (_mult),			\
		}						\
	}							\
}

#define CLK_PFD(_name, _parent, _reg, _index) {			\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_PFD,					\
	.parent = (_parent),					\
	.clk = {						\
		.pfd = {					\
			.reg = (CCM_ANALOG_##_reg),		\
			.index = (_index),			\
		}						\
	}							\
}

#define CLK_PLL(_name, _parent, _type, _reg, _mask, _powerdown, _ref) { \
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_PLL,					\
	.parent = (_parent),					\
	.clk = {						\
		.pll = {					\
			.type = (IMX6_CLK_PLL_##_type),		\
			.reg = (CCM_ANALOG_##_reg),		\
			.mask = (CCM_ANALOG_##_reg##_##_mask),	\
			.powerdown = (CCM_ANALOG_##_reg##_##_powerdown), \
			.ref = (_ref),				\
		}						\
	}							\
}

#define CLK_DIV(_name, _parent, _reg, _mask) {			\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_NORMAL),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),		\
		}						\
	}							\
}

#define CLK_DIV_BUSY(_name, _parent, _reg, _mask, _busy_reg, _busy_mask) { \
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_BUSY),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),	\
			.busy_reg = (CCM_##_busy_reg),		\
			.busy_mask = (CCM_##_busy_reg##_##_busy_mask) \
		}						\
	}							\
}

#define CLK_DIV_TABLE(_name, _parent, _reg, _mask, _tbl) {	\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_TABLE),		\
			.base = (IMX6_CLK_REG_CCM_ANALOG),	\
			.reg = (CCM_ANALOG_##_reg),		\
			.mask = (CCM_ANALOG_##_reg##_##_mask),	\
			.tbl = (_tbl)				\
		}						\
	}							\
}

#define CLK_MUX(_name, _parents, _base, _reg, _mask) {		\
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_MUX,					\
	.clk = {						\
		.mux = {					\
			.type = (IMX6_CLK_MUX_NORMAL),		\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),	\
			.parents = (_parents),			\
			.nparents = __arraycount(_parents)	\
		}						\
	}							\
}

#define CLK_MUX_BUSY(_name, _parents, _reg, _mask, _busy_reg, _busy_mask) { \
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_MUX,					\
	.clk = {						\
		.mux = {					\
			.type = (IMX6_CLK_MUX_BUSY),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),		\
			.parents = (_parents),			\
			.nparents = __arraycount(_parents),	\
			.busy_reg = (CCM_##_busy_reg),		\
			.busy_mask = (CCM_##_busy_reg##_##_busy_mask) \
		}						\
	}							\
}

#define CLK_GATE(_name, _parent, _base, _reg, _mask) {		\
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_GATE,					\
	.parent = (_parent),					\
	.clk = {						\
		.gate = {					\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),	\
			.exclusive_mask = 0			\
		}						\
	}							\
}

#define CLK_GATE_EXCLUSIVE(_name, _parent, _base, _reg, _mask, _exclusive_mask) {  \
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_GATE,					\
	.parent = (_parent),					\
	.clk = {						\
		.gate = {					\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),     \
			.exclusive_mask = (_base##_##_reg##_##_exclusive_mask) \
		}						\
	}							\
}

#endif	/* _ARM_NXP_IMX6_CCMVAR_H_ */
