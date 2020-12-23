/*	$NetBSD: imx6_ccm.c,v 1.1 2020/12/23 14:42:38 skrll Exp $	*/

/*
 * Copyright (c) 2010-2012, 2014  Genetec Corporation.  All rights reserved.
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
/*
 * Clock Controller Module (CCM) for i.MX6
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_ccm.c,v 1.1 2020/12/23 14:42:38 skrll Exp $");

#include "opt_imx.h"
#include "opt_cputypes.h"

#include "locators.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/malloc.h>
#include <sys/param.h>

#include <machine/cpu.h>

#include <arm/nxp/imx6_ccmvar.h>
#include <arm/nxp/imx6_ccmreg.h>

#include <dev/clk/clk_backend.h>

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

static const char *periph_clk2_p[] = {
	"pll3_usb_otg",
	"osc",
	"osc",
	"dummy"
};

static const char *periph2_clk2_p[] = {
	"pll3_usb_otg",
	"pll2_bus"
};

static const char *axi_p[] = {
	"periph",
	"pll2_pfd2_396m",
	"periph",
	"pll3_pfd1_540m"
};

static const char *audio_p[] = {
	"pll4_audio_div",
	"pll3_pfd2_508m",
	"pll3_pfd3_454m",
	"pll3_usb_otg"
};

static const char *gpu2d_core_p[] = {
	"axi",
	"pll3_usb_otg",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m"
};

static const char *gpu3d_core_p[] = {
	"mmdc_ch0_axi",
	"pll3_usb_otg",
	"pll2_pfd1_594m",
	"pll2_pfd2_396m"
};

static const char *gpu3d_shader_p[] = {
	"mmdc_ch0_axi",
	"pll3_usb_otg",
	"pll2_pfd1_594m",
	"pll3_pfd0_720m"
};

static const char *ipu_p[] = {
	"mmdc_ch0_axi",
	"pll2_pfd2_396m",
	"pll3_120m",
	"pll3_pfd1_540m"
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

static const char *ipu_di_pre_p[] = {
	"mmdc_ch0_axi",
	"pll3_usb_otg",
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"pll3_pfd1_540m"
};

static const char *ipu1_di0_p[] = {
	"ipu1_di0_pre",
	"dummy",
	"dummy",
	"ldb_di0",
	"ldb_di1"
};

static const char *ipu1_di1_p[] = {
	"ipu1_di1_pre",
	"dummy",
	"dummy",
	"ldb_di0",
	"ldb_di1"
};

static const char *ipu2_di0_p[] = {
	"ipu2_di0_pre",
	"dummy",
	"dummy",
	"ldb_di0",
	"ldb_di1"
};

static const char *ipu2_di1_p[] = {
	"ipu2_di1_pre",
	"dummy",
	"dummy",
	"ldb_di0",
	"ldb_di1"
};

static const char *ldb_di_p[] = {
	"pll5_video_div",
	"pll2_pfd0_352m",
	"pll2_pfd2_396m",
	"mmdc_ch1_axi",
	"pll3_usb_otg"
};

static const char *periph_p[] = {
	"periph_pre",
	"periph_clk2"
};

static const char *periph2_p[] = {
	"periph2_pre",
	"periph2_clk2"
};

static const char *vdo_axi_p[] = {
	"axi",
	"ahb"
};

static const char *vpu_axi_p[] = {
	"axi",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m"
};

static const char *cko1_p[] = {
	"pll3_usb_otg",
	"pll2_bus",
	"pll1_sys",
	"pll5_video_div",
	"dummy",
	"axi",
	"enfc",
	"ipu1_di0",
	"ipu1_di1",
	"ipu2_di0",
	"ipu2_di1",
	"ahb",
	"ipg",
	"ipg_per",
	"ckil",
	"pll4_audio_div"
};

static const char *cko2_p[] = {
	"mmdc_ch0_axi",
	"mmdc_ch1_axi",
	"usdhc4",
	"usdhc1",
	"gpu2d_axi",
	"dummy",
	"ecspi_root",
	"gpu3d_axi",
	"usdhc3",
	"dummy",
	"arm",
	"ipu1",
	"ipu2",
	"vdo_axi",
	"osc",
	"gpu2d_core",
	"gpu3d_core",
	"usdhc2",
	"ssi1",
	"ssi2",
	"ssi3",
	"gpu3d_shader",
	"vpu_axi",
	"can_root",
	"ldb_di0",
	"ldb_di1",
	"esai_extal",
	"eim_slow",
	"uart_serial",
	"spdif",
	"asrc",
	"hsi_tx"
};

static const char *cko_p[] = {
	"cko1",
	"cko2"
};

static const char *hsi_tx_p[] = {
	"pll3_120m",
	"pll2_pfd2_396m"
};

static const char *pcie_axi_p[] = {
	"axi",
	"ahb"
};

static const char *ssi_p[] = {
	"pll3_pfd2_508m",
	"pll3_pfd3_454m",
	"pll4_audio_div"
};

static const char *usdhc_p[] = {
	"pll2_pfd2_396m",
	"pll2_pfd0_352m"
};

static const char *eim_p[] = {
	"pll2_pfd2_396m",
	"pll3_usb_otg",
	"axi",
	"pll2_pfd0_352m"
};

static const char *eim_slow_p[] = {
	"axi",
	"pll3_usb_otg",
	"pll2_pfd2_396m",
	"pll2_pfd0_352m"
};

static const char *enfc_p[] = {
	"pll2_pfd0_352m",
	"pll2_bus",
	"pll3_usb_otg",
	"pll2_pfd2_396m"
};

static const char *lvds_p[] = {
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"dummy",
	"pll4_audio",
	"pll5_video",
	"pll8_mlb",
	"enet_ref",
	"pcie_ref_125m",
	"sata_ref_100m"
};

/* DT clock ID to clock name mappings */
static struct imx_clock_id {
	u_int		id;
	const char	*name;
} imx6_clock_ids[] = {
	{ IMX6CLK_DUMMY,		"dummy" },
	{ IMX6CLK_CKIL,			"ckil" },
	{ IMX6CLK_CKIH,			"ckih" },
	{ IMX6CLK_OSC,			"osc" },
	{ IMX6CLK_PLL2_PFD0_352M,	"pll2_pfd0_352m" },
	{ IMX6CLK_PLL2_PFD1_594M,	"pll2_pfd1_594m" },
	{ IMX6CLK_PLL2_PFD2_396M,	"pll2_pfd2_396m" },
	{ IMX6CLK_PLL3_PFD0_720M,	"pll3_pfd0_720m" },
	{ IMX6CLK_PLL3_PFD1_540M,	"pll3_pfd1_540m" },
	{ IMX6CLK_PLL3_PFD2_508M,	"pll3_pfd2_508m" },
	{ IMX6CLK_PLL3_PFD3_454M,	"pll3_pfd3_454m" },
	{ IMX6CLK_PLL2_198M,		"pll2_198m" },
	{ IMX6CLK_PLL3_120M,		"pll3_120m" },
	{ IMX6CLK_PLL3_80M,		"pll3_80m" },
	{ IMX6CLK_PLL3_60M,		"pll3_60m" },
	{ IMX6CLK_TWD,			"twd" },
	{ IMX6CLK_STEP,			"step" },
	{ IMX6CLK_PLL1_SW,		"pll1_sw" },
	{ IMX6CLK_PERIPH_PRE,		"periph_pre" },
	{ IMX6CLK_PERIPH2_PRE,		"periph2_pre" },
	{ IMX6CLK_PERIPH_CLK2_SEL,	"periph_clk2_sel" },
	{ IMX6CLK_PERIPH2_CLK2_SEL,	"periph2_clk2_sel" },
	{ IMX6CLK_AXI_SEL,		"axi_sel" },
	{ IMX6CLK_ESAI_SEL,		"esai_sel" },
	{ IMX6CLK_ASRC_SEL,		"asrc_sel" },
	{ IMX6CLK_SPDIF_SEL,		"spdif_sel" },
	{ IMX6CLK_GPU2D_AXI,		"gpu2d_axi" },
	{ IMX6CLK_GPU3D_AXI,		"gpu3d_axi" },
	{ IMX6CLK_GPU2D_CORE_SEL,	"gpu2d_core_sel" },
	{ IMX6CLK_GPU3D_CORE_SEL,	"gpu3d_core_sel" },
	{ IMX6CLK_GPU3D_SHADER_SEL,	"gpu3d_shader_sel" },
	{ IMX6CLK_IPU1_SEL,		"ipu1_sel" },
	{ IMX6CLK_IPU2_SEL,		"ipu2_sel" },
	{ IMX6CLK_LDB_DI0_SEL,		"ldb_di0_sel" },
	{ IMX6CLK_LDB_DI1_SEL,		"ldb_di1_sel" },
	{ IMX6CLK_IPU1_DI0_PRE_SEL,	"ipu1_di0_pre_sel" },
	{ IMX6CLK_IPU1_DI1_PRE_SEL,	"ipu1_di1_pre_sel" },
	{ IMX6CLK_IPU2_DI0_PRE_SEL,	"ipu2_di0_pre_sel" },
	{ IMX6CLK_IPU2_DI1_PRE_SEL,	"ipu2_di1_pre_sel" },
	{ IMX6CLK_IPU1_DI0_SEL,		"ipu1_di0_sel" },
	{ IMX6CLK_IPU1_DI1_SEL,		"ipu1_di1_sel" },
	{ IMX6CLK_IPU2_DI0_SEL,		"ipu2_di0_sel" },
	{ IMX6CLK_IPU2_DI1_SEL,		"ipu2_di1_sel" },
	{ IMX6CLK_HSI_TX_SEL,		"hsi_tx_sel" },
	{ IMX6CLK_PCIE_AXI_SEL,		"pcie_axi_sel" },
	{ IMX6CLK_SSI1_SEL,		"ssi1_sel" },
	{ IMX6CLK_SSI2_SEL,		"ssi2_sel" },
	{ IMX6CLK_SSI3_SEL,		"ssi3_sel" },
	{ IMX6CLK_USDHC1_SEL,		"usdhc1_sel" },
	{ IMX6CLK_USDHC2_SEL,		"usdhc2_sel" },
	{ IMX6CLK_USDHC3_SEL,		"usdhc3_sel" },
	{ IMX6CLK_USDHC4_SEL,		"usdhc4_sel" },
	{ IMX6CLK_ENFC_SEL,		"enfc_sel" },
	{ IMX6CLK_EIM_SEL,		"eim_sel" },
	{ IMX6CLK_EIM_SLOW_SEL,		"eim_slow_sel" },
	{ IMX6CLK_VDO_AXI_SEL,		"vdo_axi_sel" },
	{ IMX6CLK_VPU_AXI_SEL,		"vpu_axi_sel" },
	{ IMX6CLK_CKO1_SEL,		"cko1_sel" },
	{ IMX6CLK_PERIPH,		"periph" },
	{ IMX6CLK_PERIPH2,		"periph2" },
	{ IMX6CLK_PERIPH_CLK2,		"periph_clk2" },
	{ IMX6CLK_PERIPH2_CLK2,		"periph2_clk2" },
	{ IMX6CLK_IPG,			"ipg" },
	{ IMX6CLK_IPG_PER,		"ipg_per" },
	{ IMX6CLK_ESAI_PRED,		"esai_pred" },
	{ IMX6CLK_ESAI_PODF,		"esai_podf" },
	{ IMX6CLK_ASRC_PRED,		"asrc_pred" },
	{ IMX6CLK_ASRC_PODF,		"asrc_podf" },
	{ IMX6CLK_SPDIF_PRED,		"spdif_pred" },
	{ IMX6CLK_SPDIF_PODF,		"spdif_podf" },
	{ IMX6CLK_CAN_ROOT,		"can_root" },
	{ IMX6CLK_ECSPI_ROOT,		"ecspi_root" },
	{ IMX6CLK_GPU2D_CORE_PODF,	"gpu2d_core_podf" },
	{ IMX6CLK_GPU3D_CORE_PODF,	"gpu3d_core_podf" },
	{ IMX6CLK_GPU3D_SHADER,		"gpu3d_shader" },
	{ IMX6CLK_IPU1_PODF,		"ipu1_podf" },
	{ IMX6CLK_IPU2_PODF,		"ipu2_podf" },
	{ IMX6CLK_LDB_DI0_PODF,		"ldb_di0_podf" },
	{ IMX6CLK_LDB_DI1_PODF,		"ldb_di1_podf" },
	{ IMX6CLK_IPU1_DI0_PRE,		"ipu1_di0_pre" },
	{ IMX6CLK_IPU1_DI1_PRE,		"ipu1_di1_pre" },
	{ IMX6CLK_IPU2_DI0_PRE,		"ipu2_di0_pre" },
	{ IMX6CLK_IPU2_DI1_PRE,		"ipu2_di1_pre" },
	{ IMX6CLK_HSI_TX_PODF,		"hsi_tx_podf" },
	{ IMX6CLK_SSI1_PRED,		"ssi1_pred" },
	{ IMX6CLK_SSI1_PODF,		"ssi1_podf" },
	{ IMX6CLK_SSI2_PRED,		"ssi2_pred" },
	{ IMX6CLK_SSI2_PODF,		"ssi2_podf" },
	{ IMX6CLK_SSI3_PRED,		"ssi3_pred" },
	{ IMX6CLK_SSI3_PODF,		"ssi3_podf" },
	{ IMX6CLK_UART_SERIAL_PODF,	"uart_serial_podf" },
	{ IMX6CLK_USDHC1_PODF,		"usdhc1_podf" },
	{ IMX6CLK_USDHC2_PODF,		"usdhc2_podf" },
	{ IMX6CLK_USDHC3_PODF,		"usdhc3_podf" },
	{ IMX6CLK_USDHC4_PODF,		"usdhc4_podf" },
	{ IMX6CLK_ENFC_PRED,		"enfc_pred" },
	{ IMX6CLK_ENFC_PODF,		"enfc_podf" },
	{ IMX6CLK_EIM_PODF,		"eim_podf" },
	{ IMX6CLK_EIM_SLOW_PODF,	"eim_slow_podf" },
	{ IMX6CLK_VPU_AXI_PODF,		"vpu_axi_podf" },
	{ IMX6CLK_CKO1_PODF,		"cko1_podf" },
	{ IMX6CLK_AXI,			"axi" },
	{ IMX6CLK_MMDC_CH0_AXI_PODF,	"mmdc_ch0_axi_podf" },
	{ IMX6CLK_MMDC_CH1_AXI_PODF,	"mmdc_ch1_axi_podf" },
	{ IMX6CLK_ARM,			"arm" },
	{ IMX6CLK_AHB,			"ahb" },
	{ IMX6CLK_APBH_DMA,		"apbh_dma" },
	{ IMX6CLK_ASRC,			"asrc" },
	{ IMX6CLK_CAN1_IPG,		"can1_ipg" },
	{ IMX6CLK_CAN1_SERIAL,		"can1_serial" },
	{ IMX6CLK_CAN2_IPG,		"can2_ipg" },
	{ IMX6CLK_CAN2_SERIAL,		"can2_serial" },
	{ IMX6CLK_ECSPI1,		"ecspi1" },
	{ IMX6CLK_ECSPI2,		"ecspi2" },
	{ IMX6CLK_ECSPI3,		"ecspi3" },
	{ IMX6CLK_ECSPI4,		"ecspi4" },
	{ IMX6CLK_ECSPI5,		"ecspi5" },
	{ IMX6CLK_ENET,			"enet" },
	{ IMX6CLK_ESAI_EXTAL,		"esai_extal" },
	{ IMX6CLK_GPT_IPG,		"gpt_ipg" },
	{ IMX6CLK_GPT_IPG_PER,		"gpt_ipg_per" },
	{ IMX6CLK_GPU2D_CORE,		"gpu2d_core" },
	{ IMX6CLK_GPU3D_CORE,		"gpu3d_core" },
	{ IMX6CLK_HDMI_IAHB,		"hdmi_iahb" },
	{ IMX6CLK_HDMI_ISFR,		"hdmi_isfr" },
	{ IMX6CLK_I2C1,			"i2c1" },
	{ IMX6CLK_I2C2,			"i2c2" },
	{ IMX6CLK_I2C3,			"i2c3" },
	{ IMX6CLK_IIM,			"iim" },
	{ IMX6CLK_ENFC,			"enfc" },
	{ IMX6CLK_IPU1,			"ipu1" },
	{ IMX6CLK_IPU1_DI0,		"ipu1_di0" },
	{ IMX6CLK_IPU1_DI1,		"ipu1_di1" },
	{ IMX6CLK_IPU2,			"ipu2" },
	{ IMX6CLK_IPU2_DI0,		"ipu2_di0" },
	{ IMX6CLK_LDB_DI0,		"ldb_di0" },
	{ IMX6CLK_LDB_DI1,		"ldb_di1" },
	{ IMX6CLK_IPU2_DI1,		"ipu2_di1" },
	{ IMX6CLK_HSI_TX,		"hsi_tx" },
	{ IMX6CLK_MLB,			"mlb" },
	{ IMX6CLK_MMDC_CH0_AXI,		"mmdc_ch0_axi" },
	{ IMX6CLK_MMDC_CH1_AXI,		"mmdc_ch1_axi" },
	{ IMX6CLK_OCRAM,		"ocram" },
	{ IMX6CLK_OPENVG_AXI,		"openvg_axi" },
	{ IMX6CLK_PCIE_AXI,		"pcie_axi" },
	{ IMX6CLK_PWM1,			"pwm1" },
	{ IMX6CLK_PWM2,			"pwm2" },
	{ IMX6CLK_PWM3,			"pwm3" },
	{ IMX6CLK_PWM4,			"pwm4" },
	{ IMX6CLK_PER1_BCH,		"per1_bch" },
	{ IMX6CLK_GPMI_BCH_APB,		"gpmi_bch_apb" },
	{ IMX6CLK_GPMI_BCH,		"gpmi_bch" },
	{ IMX6CLK_GPMI_IO,		"gpmi_io" },
	{ IMX6CLK_GPMI_APB,		"gpmi_apb" },
	{ IMX6CLK_SATA,			"sata" },
	{ IMX6CLK_SDMA,			"sdma" },
	{ IMX6CLK_SPBA,			"spba" },
	{ IMX6CLK_SSI1,			"ssi1" },
	{ IMX6CLK_SSI2,			"ssi2" },
	{ IMX6CLK_SSI3,			"ssi3" },
	{ IMX6CLK_UART_IPG,		"uart_ipg" },
	{ IMX6CLK_UART_SERIAL,		"uart_serial" },
	{ IMX6CLK_USBOH3,		"usboh3" },
	{ IMX6CLK_USDHC1,		"usdhc1" },
	{ IMX6CLK_USDHC2,		"usdhc2" },
	{ IMX6CLK_USDHC3,		"usdhc3" },
	{ IMX6CLK_USDHC4,		"usdhc4" },
	{ IMX6CLK_VDO_AXI,		"vdo_axi" },
	{ IMX6CLK_VPU_AXI,		"vpu_axi" },
	{ IMX6CLK_CKO1,			"cko1" },
	{ IMX6CLK_PLL1_SYS,		"pll1_sys" },
	{ IMX6CLK_PLL2_BUS,		"pll2_bus" },
	{ IMX6CLK_PLL3_USB_OTG,		"pll3_usb_otg" },
	{ IMX6CLK_PLL4_AUDIO,		"pll4_audio" },
	{ IMX6CLK_PLL5_VIDEO,		"pll5_video" },
	{ IMX6CLK_PLL8_MLB,		"pll8_mlb" },
	{ IMX6CLK_PLL7_USB_HOST,	"pll7_usb_host" },
	{ IMX6CLK_PLL6_ENET,		"pll6_enet" },
	{ IMX6CLK_SSI1_IPG,		"ssi1_ipg" },
	{ IMX6CLK_SSI2_IPG,		"ssi2_ipg" },
	{ IMX6CLK_SSI3_IPG,		"ssi3_ipg" },
	{ IMX6CLK_ROM,			"rom" },
	{ IMX6CLK_USBPHY1,		"usbphy1" },
	{ IMX6CLK_USBPHY2,		"usbphy2" },
	{ IMX6CLK_LDB_DI0_DIV_3_5,	"ldb_di0_div_3_5" },
	{ IMX6CLK_LDB_DI1_DIV_3_5,	"ldb_di1_div_3_5" },
	{ IMX6CLK_SATA_REF,		"sata_ref" },
	{ IMX6CLK_SATA_REF_100M,	"sata_ref_100m" },
	{ IMX6CLK_PCIE_REF,		"pcie_ref" },
	{ IMX6CLK_PCIE_REF_125M,	"pcie_ref_125m" },
	{ IMX6CLK_ENET_REF,		"enet_ref" },
	{ IMX6CLK_USBPHY1_GATE,		"usbphy1_gate" },
	{ IMX6CLK_USBPHY2_GATE,		"usbphy2_gate" },
	{ IMX6CLK_PLL4_POST_DIV,	"pll4_post_div" },
	{ IMX6CLK_PLL5_POST_DIV,	"pll5_post_div" },
	{ IMX6CLK_PLL5_VIDEO_DIV,	"pll5_video_div" },
	{ IMX6CLK_EIM_SLOW,		"eim_slow" },
	{ IMX6CLK_SPDIF,		"spdif" },
	{ IMX6CLK_CKO2_SEL,		"cko2_sel" },
	{ IMX6CLK_CKO2_PODF,		"cko2_podf" },
	{ IMX6CLK_CKO2,			"cko2" },
	{ IMX6CLK_CKO,			"cko" },
	{ IMX6CLK_VDOA,			"vdoa" },
	{ IMX6CLK_PLL4_AUDIO_DIV,	"pll4_audio_div" },
	{ IMX6CLK_LVDS1_SEL,		"lvds1_sel" },
	{ IMX6CLK_LVDS2_SEL,		"lvds2_sel" },
	{ IMX6CLK_LVDS1_GATE,		"lvds1_gate" },
	{ IMX6CLK_LVDS2_GATE,		"lvds2_gate" },
	{ IMX6CLK_ESAI_IPG,		"esai_ipg" },
	{ IMX6CLK_ESAI_MEM,		"esai_mem" },
	{ IMX6CLK_ASRC_IPG,		"asrc_ipg" },
	{ IMX6CLK_ASRC_MEM,		"asrc_mem" },
	{ IMX6CLK_LVDS1_IN,		"lvds1_in" },
	{ IMX6CLK_LVDS2_IN,		"lvds2_in" },
	{ IMX6CLK_ANACLK1,		"anaclk1" },
	{ IMX6CLK_ANACLK2,		"anaclk2" },
	{ IMX6CLK_PLL1_BYPASS_SRC,	"pll1_bypass_src" },
	{ IMX6CLK_PLL2_BYPASS_SRC,	"pll2_bypass_src" },
	{ IMX6CLK_PLL3_BYPASS_SRC,	"pll3_bypass_src" },
	{ IMX6CLK_PLL4_BYPASS_SRC,	"pll4_bypass_src" },
	{ IMX6CLK_PLL5_BYPASS_SRC,	"pll5_bypass_src" },
	{ IMX6CLK_PLL6_BYPASS_SRC,	"pll6_bypass_src" },
	{ IMX6CLK_PLL7_BYPASS_SRC,	"pll7_bypass_src" },
	{ IMX6CLK_PLL1,			"pll1" },
	{ IMX6CLK_PLL2,			"pll2" },
	{ IMX6CLK_PLL3,			"pll3" },
	{ IMX6CLK_PLL4,			"pll4" },
	{ IMX6CLK_PLL5,			"pll5" },
	{ IMX6CLK_PLL6,			"pll6" },
	{ IMX6CLK_PLL7,			"pll7" },
	{ IMX6CLK_PLL1_BYPASS,		"pll1_bypass" },
	{ IMX6CLK_PLL2_BYPASS,		"pll2_bypass" },
	{ IMX6CLK_PLL3_BYPASS,		"pll3_bypass" },
	{ IMX6CLK_PLL4_BYPASS,		"pll4_bypass" },
	{ IMX6CLK_PLL5_BYPASS,		"pll5_bypass" },
	{ IMX6CLK_PLL6_BYPASS,		"pll6_bypass" },
	{ IMX6CLK_PLL7_BYPASS,		"pll7_bypass" },
	{ IMX6CLK_GPT_3M,		"gpt_3m" },
	{ IMX6CLK_VIDEO_27M,		"video_27m" },
	{ IMX6CLK_MIPI_CORE_CFG,	"mipi_core_cfg" },
	{ IMX6CLK_MIPI_IPG,		"mipi_ipg" },
	{ IMX6CLK_CAAM_MEM,		"caam_mem" },
	{ IMX6CLK_CAAM_ACLK,		"caam_aclk" },
	{ IMX6CLK_CAAM_IPG,		"caam_ipg" },
	{ IMX6CLK_SPDIF_GCLK,		"spdif_gclk" },
	{ IMX6CLK_UART_SEL,		"uart_sel" },
	{ IMX6CLK_IPG_PER_SEL,		"ipg_per_sel" },
	{ IMX6CLK_ECSPI_SEL,		"ecspi_sel" },
	{ IMX6CLK_CAN_SEL,		"can_sel" },
	{ IMX6CLK_MMDC_CH1_AXI_CG,	"mmdc_ch1_axi_cg" },
	{ IMX6CLK_PRE0,			"pre0" },
	{ IMX6CLK_PRE1,			"pre1" },
	{ IMX6CLK_PRE2,			"pre2" },
	{ IMX6CLK_PRE3,			"pre3" },
	{ IMX6CLK_PRG0_AXI,		"prg0_axi" },
	{ IMX6CLK_PRG1_AXI,		"prg1_axi" },
	{ IMX6CLK_PRG0_APB,		"prg0_apb" },
	{ IMX6CLK_PRG1_APB,		"prg1_apb" },
	{ IMX6CLK_PRE_AXI,		"pre_axi" },
	{ IMX6CLK_MLB_SEL,		"mlb_sel" },
	{ IMX6CLK_MLB_PODF,		"mlb_podf" },
	{ IMX6CLK_END,			"end" },
};

/* Clock Divider Tables */
static const int enet_ref_tbl[] = { 20, 10, 5, 4, 0 };
static const int post_div_tbl[] = { 4, 2, 1, 0 };
static const int audiovideo_div_tbl[] = { 1, 2, 1, 4, 0 };

static struct imx6_clk imx6_clks[] = {
	CLK_FIXED("dummy", 0),

	CLK_FIXED("ckil", IMX6_CKIL_FREQ),
	CLK_FIXED("ckih", IMX6_CKIH_FREQ),
	CLK_FIXED("osc", IMX6_OSC_FREQ),
	CLK_FIXED("anaclk1", IMX6_ANACLK1_FREQ),
	CLK_FIXED("anaclk2", IMX6_ANACLK2_FREQ),

	CLK_FIXED_FACTOR("sata_ref", "pll6_enet", 5, 1),
	CLK_FIXED_FACTOR("pcie_ref", "pll6_enet", 4, 1),
	CLK_FIXED_FACTOR("pll2_198m", "pll2_pfd2_396m", 2, 1),
	CLK_FIXED_FACTOR("pll3_120m", "pll3_usb_otg", 4, 1),
	CLK_FIXED_FACTOR("pll3_80m", "pll3_usb_otg", 6, 1),
	CLK_FIXED_FACTOR("pll3_60m", "pll3_usb_otg", 8, 1),
	CLK_FIXED_FACTOR("twd", "arm", 2, 1),
	CLK_FIXED_FACTOR("gpt_3m", "osc", 8, 1),
	CLK_FIXED_FACTOR("video_27m", "pll3_pfd1_540m", 20, 1),
	CLK_FIXED_FACTOR("gpu2d_axi", "mmdc_ch0_axi_podf", 1, 1),
	CLK_FIXED_FACTOR("gpu3d_axi", "mmdc_ch0_axi_podf", 1, 1),
	CLK_FIXED_FACTOR("ldb_di0_div_3_5", "ldb_di0_sel", 7, 2),
	CLK_FIXED_FACTOR("ldb_di1_div_3_5", "ldb_di1_sel", 7, 2),

	CLK_PFD("pll2_pfd0_352m", "pll2_bus", PFD_528, 0),
	CLK_PFD("pll2_pfd1_594m", "pll2_bus", PFD_528, 1),
	CLK_PFD("pll2_pfd2_396m", "pll2_bus", PFD_528, 2),
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
	CLK_DIV("ipg", "ahb", CBCDR, IPG_PODF),
	CLK_DIV("esai_pred", "esai_sel", CS1CDR, ESAI_CLK_PRED),
	CLK_DIV("esai_podf", "esai_pred", CS1CDR, ESAI_CLK_PODF),
	CLK_DIV("asrc_pred", "asrc_sel", CDCDR, SPDIF1_CLK_PRED),
	CLK_DIV("asrc_podf", "asrc_pred", CDCDR, SPDIF1_CLK_PODF),
	CLK_DIV("spdif_pred", "spdif_sel", CDCDR, SPDIF0_CLK_PRED),
	CLK_DIV("spdif_podf", "spdif_pred", CDCDR, SPDIF0_CLK_PODF),
	CLK_DIV("ecspi_root", "pll3_60m", CSCDR2, ECSPI_CLK_PODF),
	CLK_DIV("can_root", "pll3_60m", CSCMR2, CAN_CLK_PODF),
	CLK_DIV("uart_serial_podf", "pll3_80m", CSCDR1, UART_CLK_PODF),
	CLK_DIV("gpu2d_core_podf", "gpu2d_core_sel", CBCMR, GPU2D_CORE_CLK_PODF),
	CLK_DIV("gpu3d_core_podf", "gpu3d_core_sel", CBCMR, GPU3D_CORE_PODF),
	CLK_DIV("gpu3d_shader", "gpu3d_shader_sel", CBCMR, GPU3D_SHADER_PODF),
	CLK_DIV("ipu1_podf", "ipu1_sel", CSCDR3, IPU1_HSP_PODF),
	CLK_DIV("ipu2_podf", "ipu2_sel", CSCDR3, IPU2_HSP_PODF),
	CLK_DIV("ldb_di0_podf", "ldb_di0_div_3_5", CSCMR2, LDB_DI0_IPU_DIV),
	CLK_DIV("ldb_di1_podf", "ldb_di1_div_3_5", CSCMR2, LDB_DI1_IPU_DIV),
	CLK_DIV("ipu1_di0_pre", "ipu1_di0_pre_sel", CHSCCDR, IPU1_DI0_PODF),
	CLK_DIV("ipu1_di1_pre", "ipu1_di1_pre_sel", CHSCCDR, IPU1_DI1_PODF),
	CLK_DIV("ipu2_di0_pre", "ipu2_di0_pre_sel", CSCDR2, IPU2_DI0_PODF),
	CLK_DIV("ipu2_di1_pre", "ipu2_di1_pre_sel", CSCDR2, IPU2_DI1_PODF),
	CLK_DIV("hsi_tx_podf", "hsi_tx_sel", CDCDR, HSI_TX_PODF),
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
	CLK_DIV("enfc_pred", "enfc_sel", CS2CDR, ENFC_CLK_PRED),
	CLK_DIV("enfc_podf", "enfc_pred", CS2CDR, ENFC_CLK_PODF),
	CLK_DIV("vpu_axi_podf", "vpu_axi_sel", CSCDR1, VPU_AXI_PODF),
	CLK_DIV("cko1_podf", "cko1_sel", CCOSR, CLKO1_DIV),
	CLK_DIV("cko2_podf", "cko2_sel", CCOSR, CLKO2_DIV),
	CLK_DIV("ipg_per", "ipg", CSCMR1, PERCLK_PODF),
	CLK_DIV("eim_podf", "eim_sel", CSCMR1, ACLK_PODF),
	CLK_DIV("eim_slow_podf", "eim_slow_sel", CSCMR1, ACLK_EIM_SLOW_PODF),

	CLK_DIV_BUSY("axi", "axi_sel", CBCDR, AXI_PODF, CDHIPR, AXI_PODF_BUSY),
	CLK_DIV_BUSY("mmdc_ch0_axi_podf", "periph", CBCDR, MMDC_CH0_AXI_PODF, CDHIPR, MMDC_CH0_PODF_BUSY),
	CLK_DIV_BUSY("mmdc_ch1_axi_podf", "periph2", CBCDR, MMDC_CH1_AXI_PODF, CDHIPR, MMDC_CH1_PODF_BUSY),
	CLK_DIV_BUSY("arm", "pll1_sw", CACRR, ARM_PODF, CDHIPR, ARM_PODF_BUSY),
	CLK_DIV_BUSY("ahb", "periph", CBCDR, AHB_PODF, CDHIPR, AHB_PODF_BUSY),

	CLK_DIV_TABLE("pll4_post_div", "pll4_audio", PLL_AUDIO, POST_DIV_SELECT, post_div_tbl),
	CLK_DIV_TABLE("pll4_audio_div", "pll4_post_div", MISC2, AUDIO_DIV_LSB, audiovideo_div_tbl),
	CLK_DIV_TABLE("pll5_post_div", "pll5_video", PLL_VIDEO, POST_DIV_SELECT, post_div_tbl),
	CLK_DIV_TABLE("pll5_video_div", "pll5_post_div", MISC2, VIDEO_DIV, audiovideo_div_tbl),
	CLK_DIV_TABLE("enet_ref", "pll6_enet", PLL_ENET, DIV_SELECT, enet_ref_tbl),

	CLK_MUX("step", step_p, CCM, CCSR, STEP_SEL),
	CLK_MUX("pll1_sw", pll1_sw_p, CCM, CCSR, PLL1_SW_CLK_SEL),
	CLK_MUX("periph_pre", periph_pre_p, CCM, CBCMR, PRE_PERIPH_CLK_SEL),
	CLK_MUX("periph2_pre", periph_pre_p, CCM, CBCMR, PRE_PERIPH2_CLK_SEL),
	CLK_MUX("periph_clk2_sel", periph_clk2_p, CCM,CBCMR, PERIPH_CLK2_SEL),
	CLK_MUX("periph2_clk2_sel", periph2_clk2_p, CCM,CBCMR, PERIPH2_CLK2_SEL),
	CLK_MUX("axi_sel", axi_p, CCM, CBCDR, AXI_SEL),
	CLK_MUX("asrc_sel", audio_p, CCM, CDCDR, SPDIF1_CLK_SEL),
	CLK_MUX("spdif_sel", audio_p, CCM, CDCDR, SPDIF0_CLK_SEL),
	CLK_MUX("gpu2d_core_sel", gpu2d_core_p, CCM, CBCMR, GPU2D_CLK_SEL),
	CLK_MUX("gpu3d_core_sel", gpu3d_core_p, CCM, CBCMR, GPU3D_CORE_CLK_SEL),
	CLK_MUX("gpu3d_shader_sel", gpu3d_shader_p, CCM,CBCMR, GPU3D_SHADER_CLK_SEL),
	CLK_MUX("esai_sel", audio_p, CCM, CSCMR2, ESAI_CLK_SEL),
	CLK_MUX("ipu1_sel", ipu_p, CCM, CSCDR3, IPU1_HSP_CLK_SEL),
	CLK_MUX("ipu2_sel", ipu_p, CCM, CSCDR3, IPU2_HSP_CLK_SEL),
	CLK_MUX("ipu1_di0_pre_sel", ipu_di_pre_p, CCM, CHSCCDR, IPU1_DI0_PRE_CLK_SEL),
	CLK_MUX("ipu1_di1_pre_sel", ipu_di_pre_p, CCM, CHSCCDR, IPU1_DI1_PRE_CLK_SEL),
	CLK_MUX("ipu2_di0_pre_sel", ipu_di_pre_p, CCM, CSCDR2, IPU2_DI0_PRE_CLK_SEL),
	CLK_MUX("ipu2_di1_pre_sel", ipu_di_pre_p, CCM, CSCDR2, IPU2_DI1_PRE_CLK_SEL),
	CLK_MUX("ipu1_di0_sel", ipu1_di0_p, CCM, CHSCCDR, IPU1_DI0_CLK_SEL),
	CLK_MUX("ipu1_di1_sel", ipu1_di1_p, CCM, CHSCCDR, IPU1_DI1_CLK_SEL),
	CLK_MUX("ipu2_di0_sel", ipu2_di0_p, CCM, CSCDR2, IPU2_DI0_CLK_SEL),
	CLK_MUX("ipu2_di1_sel", ipu2_di1_p, CCM, CSCDR2, IPU2_DI1_CLK_SEL),
	CLK_MUX("ldb_di0_sel", ldb_di_p, CCM, CS2CDR, LDB_DI0_CLK_SEL),
	CLK_MUX("ldb_di1_sel", ldb_di_p, CCM, CS2CDR, LDB_DI1_CLK_SEL),
	CLK_MUX("vdo_axi_sel", vdo_axi_p, CCM, CBCMR, VDOAXI_CLK_SEL),
	CLK_MUX("vpu_axi_sel", vpu_axi_p, CCM, CBCMR, VPU_AXI_CLK_SEL),
	CLK_MUX("cko1_sel", cko1_p, CCM, CCOSR, CLKO1_SEL),
	CLK_MUX("cko2_sel", cko2_p, CCM, CCOSR, CLKO2_SEL),
	CLK_MUX("cko", cko_p, CCM, CCOSR, CLK_OUT_SEL),
	CLK_MUX("hsi_tx_sel", hsi_tx_p, CCM, CDCDR, HSI_TX_CLK_SEL),
	CLK_MUX("pcie_axi_sel", pcie_axi_p, CCM, CBCMR, PCIE_AXI_CLK_SEL),
	CLK_MUX("ssi1_sel", ssi_p, CCM, CSCMR1, SSI1_CLK_SEL),
	CLK_MUX("ssi2_sel", ssi_p, CCM, CSCMR1, SSI2_CLK_SEL),
	CLK_MUX("ssi3_sel", ssi_p, CCM, CSCMR1, SSI3_CLK_SEL),
	CLK_MUX("usdhc1_sel", usdhc_p, CCM, CSCMR1, USDHC1_CLK_SEL),
	CLK_MUX("usdhc2_sel", usdhc_p, CCM, CSCMR1, USDHC2_CLK_SEL),
	CLK_MUX("usdhc3_sel", usdhc_p, CCM, CSCMR1, USDHC3_CLK_SEL),
	CLK_MUX("usdhc4_sel", usdhc_p, CCM, CSCMR1, USDHC4_CLK_SEL),
	CLK_MUX("eim_sel", eim_p, CCM, CSCMR1, ACLK_SEL),
	CLK_MUX("eim_slow_sel", eim_slow_p, CCM, CSCMR1, ACLK_EIM_SLOW_SEL),
	CLK_MUX("enfc_sel", enfc_p, CCM, CS2CDR, ENFC_CLK_SEL),

	CLK_MUX("pll1_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_ARM, BYPASS_CLK_SRC),
	CLK_MUX("pll2_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_SYS, BYPASS_CLK_SRC),
	CLK_MUX("pll3_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_USB1, BYPASS_CLK_SRC),
	CLK_MUX("pll4_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_AUDIO, BYPASS_CLK_SRC),
	CLK_MUX("pll5_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_VIDEO, BYPASS_CLK_SRC),
	CLK_MUX("pll6_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_ENET, BYPASS_CLK_SRC),
	CLK_MUX("pll7_bypass_src", pll_bypass_src_p, CCM_ANALOG, PLL_USB2, BYPASS_CLK_SRC),
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

	CLK_GATE("apbh_dma", "usdhc3", CCM, CCGR0, APBHDMA_HCLK_ENABLE),
	CLK_GATE("asrc", "asrc_podf", CCM, CCGR0, ASRC_CLK_ENABLE),
	CLK_GATE("asrc_ipg", "ahb", CCM, CCGR0, ASRC_CLK_ENABLE),
	CLK_GATE("asrc_mem", "ahb", CCM, CCGR0, ASRC_CLK_ENABLE),
	CLK_GATE("caam_mem", "ahb", CCM, CCGR0, CAAM_SECURE_MEM_CLK_ENABLE),
	CLK_GATE("caam_aclk", "ahb", CCM, CCGR0, CAAM_WRAPPER_ACLK_ENABLE),
	CLK_GATE("caam_ipg", "ipg", CCM, CCGR0, CAAM_WRAPPER_IPG_ENABLE),
	CLK_GATE("can1_ipg", "ipg", CCM, CCGR0, CAN1_CLK_ENABLE),
	CLK_GATE("can1_serial", "can_root", CCM, CCGR0, CAN1_SERIAL_CLK_ENABLE),
	CLK_GATE("can2_ipg", "ipg", CCM, CCGR0, CAN2_CLK_ENABLE),
	CLK_GATE("can2_serial", "can_root", CCM, CCGR0, CAN2_SERIAL_CLK_ENABLE),
	CLK_GATE("ecspi1", "ecspi_root", CCM, CCGR1, ECSPI1_CLK_ENABLE),
	CLK_GATE("ecspi2", "ecspi_root", CCM, CCGR1, ECSPI2_CLK_ENABLE),
	CLK_GATE("ecspi3", "ecspi_root", CCM, CCGR1, ECSPI3_CLK_ENABLE),
	CLK_GATE("ecspi4", "ecspi_root", CCM, CCGR1, ECSPI4_CLK_ENABLE),
	CLK_GATE("ecspi5", "ecspi_root", CCM, CCGR1, ECSPI5_CLK_ENABLE),
	CLK_GATE("enet", "ipg", CCM, CCGR1, ENET_CLK_ENABLE),
	CLK_GATE("esai_extal", "esai_podf", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("esai_ipg", "ahb", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("esai_mem", "ahb", CCM, CCGR1, ESAI_CLK_ENABLE),
	CLK_GATE("gpt_ipg", "ipg", CCM, CCGR1, GPT_CLK_ENABLE),
	CLK_GATE("gpt_ipg_per", "ipg_per", CCM, CCGR1, GPT_SERIAL_CLK_ENABLE),
	CLK_GATE("gpu2d_core", "gpu2d_core_podf", CCM, CCGR1, GPU2D_CLK_ENABLE),
	CLK_GATE("gpu3d_core", "gpu3d_core_podf", CCM, CCGR1, GPU3D_CLK_ENABLE),
	CLK_GATE("hdmi_iahb", "ahb", CCM, CCGR2, HDMI_TX_IAHBCLK_ENABLE),
	CLK_GATE("hdmi_isfr", "video_27m", CCM, CCGR2, HDMI_TX_ISFRCLK_ENABLE),
	CLK_GATE("i2c1", "ipg_per", CCM, CCGR2, I2C1_SERIAL_CLK_ENABLE),
	CLK_GATE("i2c2", "ipg_per", CCM, CCGR2, I2C2_SERIAL_CLK_ENABLE),
	CLK_GATE("i2c3", "ipg_per", CCM, CCGR2, I2C3_SERIAL_CLK_ENABLE),
	CLK_GATE("iim", "ipg", CCM, CCGR2, IIM_CLK_ENABLE),
	CLK_GATE("enfc", "enfc_podf", CCM, CCGR2, IOMUX_IPT_CLK_IO_CLK_ENABLE),
	CLK_GATE("vdoa", "vdo_axi", CCM, CCGR2, IPSYNC_VDOA_IPG_CLK_ENABLE),
	CLK_GATE("ipu1", "ipu1_podf", CCM, CCGR3, IPU1_IPU_CLK_ENABLE),
	CLK_GATE("ipu1_di0", "ipu1_di0_sel", CCM, CCGR3, IPU1_IPU_DI0_CLK_ENABLE),
	CLK_GATE("ipu1_di1", "ipu1_di1_sel", CCM, CCGR3, IPU1_IPU_DI1_CLK_ENABLE),
	CLK_GATE("ipu2", "ipu2_podf", CCM, CCGR3, IPU2_IPU_CLK_ENABLE),
	CLK_GATE("ipu2_di0", "ipu2_di0_sel", CCM, CCGR3, IPU2_IPU_DI0_CLK_ENABLE),
	CLK_GATE("ldb_di0", "ldb_di0_podf", CCM, CCGR3, LDB_DI0_CLK_ENABLE),
	CLK_GATE("ldb_di1", "ldb_di1_podf", CCM, CCGR3, LDB_DI1_CLK_ENABLE),
	CLK_GATE("ipu2_di1", "ipu2_di1_sel", CCM, CCGR3, IPU2_IPU_DI1_CLK_ENABLE),
	CLK_GATE("hsi_tx", "hsi_tx_podf", CCM, CCGR3, MIPI_CORE_CFG_CLK_ENABLE),
	CLK_GATE("mipi_core_cfg", "video_27m", CCM, CCGR3, MIPI_CORE_CFG_CLK_ENABLE),
	CLK_GATE("mipi_ipg", "ipg", CCM, CCGR3, MIPI_CORE_CFG_CLK_ENABLE),
	CLK_GATE("mlb", "axi", CCM, CCGR3, MLB_CLK_ENABLE),
	CLK_GATE("mmdc_ch0_axi", "mmdc_ch0_axi_podf", CCM, CCGR3, MMDC_CORE_ACLK_FAST_CORE_P0_ENABLE),
	CLK_GATE("mmdc_ch1_axi", "mmdc_ch1_axi_podf", CCM, CCGR3, MMDC_CORE_ACLK_FAST_CORE_P1_ENABLE),
	CLK_GATE("ocram", "ahb", CCM, CCGR3, OCRAM_CLK_ENABLE),
	CLK_GATE("openvg_axi", "axi", CCM, CCGR3, OPENVGAXICLK_CLK_ROOT_ENABLE),
	CLK_GATE("pcie_axi", "pcie_axi_sel", CCM, CCGR4, PCIE_ROOT_ENABLE),
	CLK_GATE("per1_bch", "usdhc3", CCM, CCGR4, PL301_MX6QPER1_BCHCLK_ENABLE),
	CLK_GATE("pwm1", "ipg_per", CCM, CCGR4, PWM1_CLK_ENABLE),
	CLK_GATE("pwm2", "ipg_per", CCM, CCGR4, PWM2_CLK_ENABLE),
	CLK_GATE("pwm3", "ipg_per", CCM, CCGR4, PWM3_CLK_ENABLE),
	CLK_GATE("pwm4", "ipg_per", CCM, CCGR4, PWM4_CLK_ENABLE),
	CLK_GATE("gpmi_bch_apb", "usdhc3", CCM, CCGR4, RAWNAND_U_BCH_INPUT_APB_CLK_ENABLE),
	CLK_GATE("gpmi_bch", "usdhc4", CCM, CCGR4, RAWNAND_U_GPMI_BCH_INPUT_BCH_CLK_ENABLE),
	CLK_GATE("gpmi_io", "enfc", CCM, CCGR4, RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_CLK_ENABLE),
	CLK_GATE("gpmi_apb", "usdhc3", CCM, CCGR4, RAWNAND_U_GPMI_INPUT_APB_CLK_ENABLE),
	CLK_GATE("rom", "ahb", CCM, CCGR5, ROM_CLK_ENABLE),
	CLK_GATE("sata", "ahb", CCM, CCGR5, SATA_CLK_ENABLE),
	CLK_GATE("sdma", "ahb", CCM, CCGR5, SDMA_CLK_ENABLE),
	CLK_GATE("spba", "ipg", CCM, CCGR5, SPBA_CLK_ENABLE),
	CLK_GATE("spdif", "spdif_podf", CCM, CCGR5, SPDIF_CLK_ENABLE),
	CLK_GATE("spdif_gclk", "ipg", CCM, CCGR5, SPDIF_CLK_ENABLE),
	CLK_GATE("ssi1_ipg", "ipg", CCM, CCGR5, SSI1_CLK_ENABLE),
	CLK_GATE("ssi2_ipg", "ipg", CCM, CCGR5, SSI2_CLK_ENABLE),
	CLK_GATE("ssi3_ipg", "ipg", CCM, CCGR5, SSI3_CLK_ENABLE),
	CLK_GATE("ssi1", "ssi1_podf", CCM, CCGR5, SSI1_CLK_ENABLE),
	CLK_GATE("ssi2", "ssi2_podf", CCM, CCGR5, SSI2_CLK_ENABLE),
	CLK_GATE("ssi3", "ssi3_podf", CCM, CCGR5, SSI3_CLK_ENABLE),
	CLK_GATE("uart_ipg", "ipg", CCM, CCGR5, UART_CLK_ENABLE),
	CLK_GATE("uart_serial", "uart_serial_podf", CCM, CCGR5, UART_SERIAL_CLK_ENABLE),
	CLK_GATE("usboh3", "ipg", CCM, CCGR6, USBOH3_CLK_ENABLE),
	CLK_GATE("usdhc1", "usdhc1_podf", CCM, CCGR6, USDHC1_CLK_ENABLE),
	CLK_GATE("usdhc2", "usdhc2_podf", CCM, CCGR6, USDHC2_CLK_ENABLE),
	CLK_GATE("usdhc3", "usdhc3_podf", CCM, CCGR6, USDHC3_CLK_ENABLE),
	CLK_GATE("usdhc4", "usdhc4_podf", CCM, CCGR6, USDHC4_CLK_ENABLE),
	CLK_GATE("eim_slow", "eim_slow_podf", CCM, CCGR6, EIM_SLOW_CLK_ENABLE),
	CLK_GATE("vdo_axi", "vdo_axi_sel", CCM, CCGR6, VDOAXICLK_CLK_ENABLE),
	CLK_GATE("vpu_axi", "vpu_axi_podf", CCM, CCGR6, VPU_CLK_ENABLE),
	CLK_GATE("cko1", "cko1_podf", CCM, CCOSR, CLKO1_EN),
	CLK_GATE("cko2", "cko2_podf", CCM, CCOSR, CLKO2_EN),

	CLK_GATE("sata_ref_100m", "sata_ref", CCM_ANALOG, PLL_ENET, ENABLE_100M),
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

	CLK_GATE_EXCLUSIVE("lvds1_gate", "lvds1_sel", CCM_ANALOG, MISC1, LVDS_CLK1_OBEN, LVDS_CLK1_IBEN),
	CLK_GATE_EXCLUSIVE("lvds2_gate", "lvds2_sel", CCM_ANALOG, MISC1, LVDS_CLK2_OBEN, LVDS_CLK2_IBEN),
	CLK_GATE_EXCLUSIVE("lvds1_in", "anaclk1", CCM_ANALOG, MISC1, LVDS_CLK1_IBEN, LVDS_CLK1_OBEN),
	CLK_GATE_EXCLUSIVE("lvds2_in", "anaclk2", CCM_ANALOG, MISC1, LVDS_CLK2_IBEN, LVDS_CLK2_OBEN),
};

static struct imx6_clk *imx6_clk_find(const char *);
static struct imx6_clk *imx6_clk_find_by_id(u_int);

static void imxccm_init_clocks(struct imx6ccm_softc *);
static struct clk *imxccm_clk_get(void *, const char *);
static void imxccm_clk_put(void *, struct clk *);
static u_int imxccm_clk_get_rate(void *, struct clk *);
static int imxccm_clk_set_rate(void *, struct clk *, u_int);
static int imxccm_clk_enable(void *, struct clk *);
static int imxccm_clk_disable(void *, struct clk *);
static int imxccm_clk_set_parent(void *, struct clk *, struct clk *);
static struct clk *imxccm_clk_get_parent(void *, struct clk *);

static const struct clk_funcs imxccm_clk_funcs = {
	.get = imxccm_clk_get,
	.put = imxccm_clk_put,
	.get_rate = imxccm_clk_get_rate,
	.set_rate = imxccm_clk_set_rate,
	.enable = imxccm_clk_enable,
	.disable = imxccm_clk_disable,
	.set_parent = imxccm_clk_set_parent,
	.get_parent = imxccm_clk_get_parent,
};

void
imx6ccm_attach_common(device_t self)
{
	struct imx6ccm_softc * const sc = device_private(self);

	sc->sc_dev = self;

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &imxccm_clk_funcs;
	sc->sc_clkdom.priv = sc;
	for (u_int n = 0; n < __arraycount(imx6_clks); n++) {
		imx6_clks[n].base.domain = &sc->sc_clkdom;
		clk_attach(&imx6_clks[n].base);
	}

	imxccm_init_clocks(sc);

	for (int n = 0; n < __arraycount(imx6_clks); n++) {
		struct clk *clk = &imx6_clks[n].base;
		struct clk *clk_parent = clk_get_parent(clk);
		const char *parent_str = clk_parent ? clk_parent->name : "none";
		aprint_verbose_dev(self, "%s (%s) : %u Hz\n", clk->name,
		    parent_str, clk_get_rate(clk));
	}
}

struct clk *
imx6_get_clock(const char *name)
{
	struct imx6_clk *iclk;
	iclk = imx6_clk_find(name);

	if (iclk == NULL)
		return NULL;

	return &iclk->base;
}

struct clk *
imx6_get_clock_by_id(u_int clock_id)
{
	struct imx6_clk *iclk;
	iclk = imx6_clk_find_by_id(clock_id);

	if (iclk == NULL)
		return NULL;

	return &iclk->base;
}

static struct imx6_clk *
imx6_clk_find(const char *name)
{
	if (name == NULL)
		return NULL;

	for (int n = 0; n < __arraycount(imx6_clks); n++) {
		if (strcmp(imx6_clks[n].base.name, name) == 0)
			return &imx6_clks[n];
	}

	return NULL;
}

static struct imx6_clk *
imx6_clk_find_by_id(u_int clock_id)
{
	for (int n = 0; n < __arraycount(imx6_clock_ids); n++) {
		if (imx6_clock_ids[n].id == clock_id) {
			const char *name = imx6_clock_ids[n].name;
			return imx6_clk_find(name);
		}
	}

	return NULL;
}

struct imxccm_init_parent {
	const char *clock;
	const char *parent;
} imxccm_init_parents[] = {
	{ "pll1_bypass",	"pll1" },
	{ "pll2_bypass",	"pll2" },
	{ "pll3_bypass",	"pll3" },
	{ "pll4_bypass",	"pll4" },
	{ "pll5_bypass",	"pll5" },
	{ "pll6_bypass",	"pll6" },
	{ "pll7_bypass",	"pll7" },
	{ "lvds1_sel",		"sata_ref_100m" },
};

static void
imxccm_init_clocks(struct imx6ccm_softc *sc)
{
	struct clk *clk;
	struct clk *clk_parent;

	for (u_int n = 0; n < __arraycount(imxccm_init_parents); n++) {
		clk = clk_get(&sc->sc_clkdom, imxccm_init_parents[n].clock);
		KASSERT(clk != NULL);
		clk_parent = clk_get(&sc->sc_clkdom, imxccm_init_parents[n].parent);
		KASSERT(clk_parent != NULL);

		int error = clk_set_parent(clk, clk_parent);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't set '%s' parent to '%s': %d\n",
			    clk->name, clk_parent->name, error);
		}
		clk_put(clk_parent);
		clk_put(clk);
	}
}

static u_int
imxccm_clk_get_rate_pll_generic(struct imx6ccm_softc *sc, struct imx6_clk *iclk,
    const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT((pll->type == IMX6_CLK_PLL_GENERIC) ||
	    (pll->type == IMX6_CLK_PLL_USB));

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);

	return freq * ((div == 1) ? 22 : 20);
}

static u_int
imxccm_clk_get_rate_pll_sys(struct imx6ccm_softc *sc, struct imx6_clk *iclk,
    const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT(pll->type == IMX6_CLK_PLL_SYS);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);

	return freq * div / 2;
}

#define PLL_AUDIO_VIDEO_NUM_OFFSET	0x10
#define PLL_AUDIO_VIDEO_DENOM_OFFSET	0x20

static u_int
imxccm_clk_get_rate_pll_audio_video(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	uint64_t freq = rate_parent;

	KASSERT(pll->type == IMX6_CLK_PLL_AUDIO_VIDEO);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pll->reg);
	uint32_t div = __SHIFTOUT(v, pll->mask);
	uint32_t num = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog,
	    pll->reg + PLL_AUDIO_VIDEO_NUM_OFFSET);
	uint32_t denom = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog,
	    pll->reg + PLL_AUDIO_VIDEO_DENOM_OFFSET);

	uint64_t tmp = freq * num / denom;

	return freq * div + tmp;
}

static u_int
imxccm_clk_get_rate_pll_enet(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, const u_int rate_parent)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;

	KASSERT(pll->type == IMX6_CLK_PLL_ENET);

	return pll->ref;
}

static u_int
imxccm_clk_get_rate_fixed_factor(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_fixed_factor *fixed_factor = &iclk->clk.fixed_factor;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_FIXED_FACTOR);

	parent = imx6_clk_find(iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	return rate_parent * fixed_factor->mult / fixed_factor->div;
}

static u_int
imxccm_clk_get_rate_pll(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_PLL);

	parent = imx6_clk_find(iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	switch(pll->type) {
	case IMX6_CLK_PLL_GENERIC:
		return imxccm_clk_get_rate_pll_generic(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_SYS:
		return imxccm_clk_get_rate_pll_sys(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_USB:
		return imxccm_clk_get_rate_pll_generic(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_AUDIO_VIDEO:
		return imxccm_clk_get_rate_pll_audio_video(sc, iclk, rate_parent);
	case IMX6_CLK_PLL_ENET:
		return imxccm_clk_get_rate_pll_enet(sc, iclk, rate_parent);
	default:
		panic("imx6: unknown pll type %d", iclk->type);
	}
}

static u_int
imxccm_clk_get_rate_div(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_div *div = &iclk->clk.div;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_DIV);

	parent = imx6_clk_find(iclk->parent);
	KASSERT(parent != NULL);

	u_int rate = imxccm_clk_get_rate(sc, &parent->base);

	bus_space_handle_t ioh;
	if (div->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, div->reg);
	uint32_t n = __SHIFTOUT(v, div->mask);

	if (div->type == IMX6_CLK_DIV_TABLE) {
		KASSERT(div->tbl != NULL);

		for (int i = 0; div->tbl[i] != 0; i++)
			if (i == n)
				rate /= div->tbl[i];
	} else {
		rate /= n + 1;
	}

	return rate;
}

static u_int
imxccm_clk_get_rate_pfd(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_pfd *pfd = &iclk->clk.pfd;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_PFD);

	parent = imx6_clk_find(iclk->parent);
	KASSERT(parent != NULL);

	uint64_t rate_parent = imxccm_clk_get_rate(sc, &parent->base);

	uint32_t v = bus_space_read_4(sc->sc_iot, sc->sc_ioh_analog, pfd->reg);
	uint32_t n = __SHIFTOUT(v, __BITS(5, 0) << (pfd->index * 8));

	KASSERT(n != 0);

	return (rate_parent * 18) / n;
}

static int
imxccm_clk_mux_wait(struct imx6ccm_softc *sc, struct imx6_clk_mux *mux)
{
	KASSERT(mux->busy_reg == 0);
	KASSERT(mux->busy_mask == 0);

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	while (bus_space_read_4(sc->sc_iot, ioh, mux->busy_reg) & mux->busy_mask)
		delay(10);

	return 0;
}

static int
imxccm_clk_set_parent_mux(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, struct clk *parent)
{
	struct imx6_clk_mux *mux = &iclk->clk.mux;
	const char *pname = parent->name;
	u_int sel;

	KASSERT(iclk->type == IMX6_CLK_MUX);

	for (sel = 0; sel < mux->nparents; sel++)
		if (strcmp(pname, mux->parents[sel]) == 0)
			break;

	if (sel == mux->nparents)
		return EINVAL;

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, mux->reg);
	v &= ~mux->mask;
	v |= __SHIFTIN(sel, mux->mask);

	bus_space_write_4(sc->sc_iot, ioh, mux->reg, v);

	iclk->parent = pname;

	if (mux->type == IMX6_CLK_MUX_BUSY)
		imxccm_clk_mux_wait(sc, mux);

	return 0;
}

static struct imx6_clk *
imxccm_clk_get_parent_mux(struct imx6ccm_softc *sc, struct imx6_clk *iclk)
{
	struct imx6_clk_mux *mux = &iclk->clk.mux;

	KASSERT(iclk->type == IMX6_CLK_MUX);

	bus_space_handle_t ioh;
	if (mux->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, mux->reg);
	u_int sel = __SHIFTOUT(v, mux->mask);

	KASSERT(sel < mux->nparents);

	iclk->parent = mux->parents[sel];

	return imx6_clk_find(iclk->parent);
}

static int
imxccm_clk_set_rate_pll(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, u_int rate)
{
	/* ToDo */

	return EOPNOTSUPP;
}

static int
imxccm_clk_set_rate_div(struct imx6ccm_softc *sc,
    struct imx6_clk *iclk, u_int rate)
{
	struct imx6_clk_div *div = &iclk->clk.div;
	struct imx6_clk *parent;

	KASSERT(iclk->type == IMX6_CLK_DIV);

	parent = imx6_clk_find(iclk->parent);
	KASSERT(parent != NULL);

	u_int rate_parent = imxccm_clk_get_rate(sc, &parent->base);
	u_int divider = uimax(1, rate_parent / rate);

	bus_space_handle_t ioh;
	if (div->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, div->reg);
	v &= ~div->mask;
	if (div->type == IMX6_CLK_DIV_TABLE) {
		int n = -1;

		KASSERT(div->tbl != NULL);
		for (int i = 0; div->tbl[i] != 0; i++)
			if (div->tbl[i] == divider)
				n = i;

		if (n >= 0)
			v |= __SHIFTIN(n, div->mask);
		else
			return EINVAL;
	} else {
		v |= __SHIFTIN(divider - 1, div->mask);
	}
	bus_space_write_4(sc->sc_iot, ioh, div->reg, v);

	return 0;
}

/*
 * CLK Driver APIs
 */
static struct clk *
imxccm_clk_get(void *priv, const char *name)
{
	struct imx6_clk *iclk;

	iclk = imx6_clk_find(name);
	if (iclk == NULL)
		return NULL;

	atomic_inc_uint(&iclk->refcnt);

	return &iclk->base;
}

static void
imxccm_clk_put(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;

	KASSERT(iclk->refcnt > 0);

	atomic_dec_uint(&iclk->refcnt);
}

static u_int
imxccm_clk_get_rate(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct clk *parent;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
		return iclk->clk.fixed.rate;
	case IMX6_CLK_FIXED_FACTOR:
		return imxccm_clk_get_rate_fixed_factor(sc, iclk);
	case IMX6_CLK_PLL:
		return imxccm_clk_get_rate_pll(sc, iclk);
	case IMX6_CLK_MUX:
	case IMX6_CLK_GATE:
		parent = imxccm_clk_get_parent(sc, clk);
		return imxccm_clk_get_rate(sc, parent);
	case IMX6_CLK_DIV:
		return imxccm_clk_get_rate_div(sc, iclk);
	case IMX6_CLK_PFD:
		return imxccm_clk_get_rate_pfd(sc, iclk);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return ENXIO;
	case IMX6_CLK_PLL:
		return imxccm_clk_set_rate_pll(sc, iclk, rate);
	case IMX6_CLK_MUX:
		return ENXIO;
	case IMX6_CLK_GATE:
		return ENXIO;
	case IMX6_CLK_DIV:
		return imxccm_clk_set_rate_div(sc, iclk, rate);
	case IMX6_CLK_PFD:
		return EINVAL;
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_enable_pll(struct imx6ccm_softc *sc, struct imx6_clk *iclk, bool enable)
{
	struct imx6_clk_pll *pll = &iclk->clk.pll;

	KASSERT(iclk->type == IMX6_CLK_PLL);

	/* Power up bit */
	if (pll->type == IMX6_CLK_PLL_USB)
		enable = !enable;

	bus_space_handle_t ioh = sc->sc_ioh_analog;
	uint32_t  v = bus_space_read_4(sc->sc_iot, ioh, pll->reg);
	if (__SHIFTOUT(v, pll->powerdown) != enable)
		return 0;
	if (enable)
		v &= ~pll->powerdown;
	else
		v |= pll->powerdown;
	bus_space_write_4(sc->sc_iot, ioh, pll->reg, v);

	/* wait look */
	while (!(bus_space_read_4(sc->sc_iot, ioh, pll->reg) & CCM_ANALOG_PLL_LOCK))
		delay(10);

	return 0;
}

static int
imxccm_clk_enable_gate(struct imx6ccm_softc *sc, struct imx6_clk *iclk, bool enable)
{
	struct imx6_clk_gate *gate = &iclk->clk.gate;

	KASSERT(iclk->type == IMX6_CLK_GATE);

	bus_space_handle_t ioh;
	if (gate->base == IMX6_CLK_REG_CCM_ANALOG)
		ioh = sc->sc_ioh_analog;
	else
		ioh = sc->sc_ioh;

	uint32_t v = bus_space_read_4(sc->sc_iot, ioh, gate->reg);
	if (enable) {
		if (gate->exclusive_mask)
			v &= ~gate->exclusive_mask;
		v |= gate->mask;
	} else {
		if (gate->exclusive_mask)
			v |= gate->exclusive_mask;
		v &= ~gate->mask;
	}
	bus_space_write_4(sc->sc_iot, ioh, gate->reg, v);

	return 0;
}

static int
imxccm_clk_enable(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6_clk *parent = NULL;
	struct imx6ccm_softc *sc = priv;

	if ((parent = imx6_clk_find(iclk->parent)) != NULL)
		imxccm_clk_enable(sc, &parent->base);

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return 0;	/* always on */
	case IMX6_CLK_PLL:
		return imxccm_clk_enable_pll(sc, iclk, true);
	case IMX6_CLK_MUX:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return 0;
	case IMX6_CLK_GATE:
		return imxccm_clk_enable_gate(sc, iclk, true);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_disable(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
		return EINVAL;	/* always on */
	case IMX6_CLK_PLL:
		return imxccm_clk_enable_pll(sc, iclk, false);
	case IMX6_CLK_MUX:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return EINVAL;
	case IMX6_CLK_GATE:
		return imxccm_clk_enable_gate(sc, iclk, false);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static int
imxccm_clk_set_parent(void *priv, struct clk *clk, struct clk *parent)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
	case IMX6_CLK_PLL:
	case IMX6_CLK_GATE:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		return EINVAL;
	case IMX6_CLK_MUX:
		return imxccm_clk_set_parent_mux(sc, iclk, parent);
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}
}

static struct clk *
imxccm_clk_get_parent(void *priv, struct clk *clk)
{
	struct imx6_clk *iclk = (struct imx6_clk *)clk;
	struct imx6_clk *parent = NULL;
	struct imx6ccm_softc *sc = priv;

	switch (iclk->type) {
	case IMX6_CLK_FIXED:
	case IMX6_CLK_FIXED_FACTOR:
	case IMX6_CLK_PLL:
	case IMX6_CLK_GATE:
	case IMX6_CLK_DIV:
	case IMX6_CLK_PFD:
		if (iclk->parent != NULL)
			parent = imx6_clk_find(iclk->parent);
		break;
	case IMX6_CLK_MUX:
		parent = imxccm_clk_get_parent_mux(sc, iclk);
		break;
	default:
		panic("imx6: unknown clk type %d", iclk->type);
	}

	return (struct clk *)parent;
}
