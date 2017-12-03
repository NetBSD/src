/*	$NetBSD: imx7_ccm.c,v 1.2.14.2 2017/12/03 11:35:53 jdolecek Exp $	*/

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
 * Clock Controller Module (CCM) for i.MX7
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_ccm.c,v 1.2.14.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_imx.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/malloc.h>
#include <sys/param.h>

#include <machine/cpu.h>
#include <arm/imx/imx7_ccmvar.h>
#include <arm/imx/imx7_ccmreg.h>
#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>

#ifdef CCM_DEBUG
int ccm_debug = 1;
#endif

struct imxccm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ioh_analog;

	/* for sysctl */
	struct sysctllog *sc_log;
	int sc_sysctlnode_arm_pll;
	int sc_sysctlnode_sys_pll;
	int sc_sysctlnode_enet_pll;
	int sc_sysctlnode_audio_pll;
	int sc_sysctlnode_video_pll;
	int sc_sysctlnode_ddr_pll;
	int sc_sysctlnode_usb_pll;

	int sc_sysctlnode_arm_a7_clk;
	int sc_sysctlnode_arm_m4_clk;
};

const struct imx7clkroottbl {
	int clksrc[8];
	uint32_t targetroot;
	int type;
#define CLKTYPE_CORE		0
#define CLKTYPE_DRAM		1
#define CLKTYPE_DRAM_PHYM	2
#define CLKTYPE_IP		3
#define CLKTYPE_AHB		4
#define CLKTYPE_BUS		5
} imx7clkroottbl[] = {
	[IMX7CLK_ARM_A7_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ARM_A7_CLK_ROOT,
		.type = CLKTYPE_CORE,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ARM_PLL,
			IMX7CLK_ENET_PLL_DIV2, IMX7CLK_DDR_PLL,
			IMX7CLK_SYS_PLL, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_AUDIO_PLL, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_ARM_M4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ARM_M4_CLK_ROOT,
		.type = CLKTYPE_BUS,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_MAIN_AXI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_MAIN_AXI_CLK_ROOT,
		.type = CLKTYPE_BUS,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD1,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV4,
			IMX7CLK_SYS_PLL_PFD5, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_DISP_AXI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_DISP_AXI_CLK_ROOT,
		.type = CLKTYPE_BUS,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD1,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV4,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_ENET_AXI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET_AXI_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV4,
			IMX7CLK_SYS_PLL_DIV2, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD4
		}
	},
	[IMX7CLK_NAND_USDHC_BUS_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_NAND_USDHC_BUS_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_AHB_CLK_ROOT,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD2_DIV2, IMX7CLK_SYS_PLL_PFD6,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_AUDIO_PLL
		}
	},
	[IMX7CLK_AHB_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_AHB_CLK_ROOT,
		.type = CLKTYPE_AHB,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_USB_PLL,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_IPG_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_IPG_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_AHB_CLK_ROOT, -1,
			-1, -1,
			-1, -1,
			-1, -1
		}
	},
	[IMX7CLK_DRAM_PHYM_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_DRAM_PHYM_CLK_ROOT,
		.type = CLKTYPE_DRAM_PHYM,
		.clksrc = {
			IMX7CLK_DDR_PLL, IMX7CLK_DRAM_PHYM_ALT_CLK_ROOT,
			-1, -1,
			-1, -1,
			-1, -1
		}
	},
	[IMX7CLK_DRAM_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_DRAM_CLK_ROOT,
		.type = CLKTYPE_DRAM,
		.clksrc = {
			IMX7CLK_DDR_PLL, IMX7CLK_DRAM_ALT_CLK_ROOT,
			-1, -1,
			-1, -1,
			-1, -1
		}
	},
	[IMX7CLK_DRAM_PHYM_ALT_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_DRAM_PHYM_ALT_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_SYS_PLL, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD7,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_DRAM_ALT_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_DRAM_ALT_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_SYS_PLL, IMX7CLK_ENET_PLL_DIV4,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_AUDIO_PLL, IMX7CLK_SYS_PLL_PFD2
		}
	},
	[IMX7CLK_USB_HSIC_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_USB_HSIC_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD3,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_SYS_PLL_PFD5,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_PCIE_CTRL_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PCIE_CTRL_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV4,
			IMX7CLK_SYS_PLL_DIV2, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD1, IMX7CLK_SYS_PLL_PFD6
		}
	},
	[IMX7CLK_PCIE_PHY_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PCIE_PHY_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_ENET_PLL_DIV2, IMX7CLK_EXT_CLK1,
			IMX7CLK_EXT_CLK2, IMX7CLK_EXT_CLK3,
			IMX7CLK_EXT_CLK4, IMX7CLK_SYS_PLL_PFD0
		}
	},
	[IMX7CLK_EPDC_PIXEL_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_EPDC_PIXEL_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD1,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL,
			IMX7CLK_SYS_PLL_PFD5, IMX7CLK_SYS_PLL_PFD6,
			IMX7CLK_SYS_PLL_PFD7, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_LCDIF_PIXEL_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_LCDIF_PIXEL_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD5,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_EXT_CLK3,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_MIPI_DSI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_MIPI_DSI_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD5,
			IMX7CLK_SYS_PLL_PFD3, IMX7CLK_SYS_PLL,
			IMX7CLK_SYS_PLL_PFD0_DIV2, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_AUDIO_PLL
		}
	},
	[IMX7CLK_MIPI_CSI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_MIPI_CSI_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_SYS_PLL_PFD3, IMX7CLK_SYS_PLL,
			IMX7CLK_SYS_PLL_PFD0_DIV2, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_AUDIO_PLL
		}
	},
	[IMX7CLK_MIPI_DPHY_REF_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_MIPI_DPHY_REF_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL_PFD5,
			IMX7CLK_REF_1M, IMX7CLK_EXT_CLK2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_EXT_CLK3
		}
	},
	[IMX7CLK_SAI1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_SAI1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_EXT_CLK2
		}
	},
	[IMX7CLK_SAI2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_SAI2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_EXT_CLK2
		}
	},
	[IMX7CLK_SAI3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_SAI3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_EXT_CLK3
		}
	},
	[IMX7CLK_ENET1_REF_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET1_REF_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV8,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_ENET_PLL_DIV40,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_EXT_CLK4
		}
	},
	[IMX7CLK_ENET1_TIME_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET1_TIME_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK1,
			IMX7CLK_EXT_CLK2, IMX7CLK_EXT_CLK3,
			IMX7CLK_EXT_CLK4, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_ENET2_REF_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET2_REF_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV8,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_ENET_PLL_DIV40,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_EXT_CLK4
		}
	},
	[IMX7CLK_ENET2_TIME_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET2_TIME_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK1,
			IMX7CLK_EXT_CLK2, IMX7CLK_EXT_CLK3,
			IMX7CLK_EXT_CLK4, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_ENET_PHY_REF_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ENET_PHY_REF_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV40,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_ENET_PLL_DIV8,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_SYS_PLL_PFD3
		}
	},
	[IMX7CLK_EIM_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_EIM_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD2, IMX7CLK_SYS_PLL_PFD3,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_NAND_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_NAND_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_SYS_PLL_PFD3, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_QSPI_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_QSPI_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD3, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_USDHC1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_USDHC1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_USDHC2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_USDHC2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_USDHC3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_USDHC3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD0,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_SYS_PLL_PFD2,
			IMX7CLK_SYS_PLL_PFD6, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_CAN1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_CAN1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_USB_PLL,
			IMX7CLK_EXT_CLK1, IMX7CLK_EXT_CLK4
		}
	},
	[IMX7CLK_CAN2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_CAN2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_SYS_PLL,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_USB_PLL,
			IMX7CLK_EXT_CLK1, IMX7CLK_EXT_CLK3
		}
	},
	[IMX7CLK_I2C1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_I2C1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD2_DIV2
		}
	},
	[IMX7CLK_I2C2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_I2C2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD2_DIV2
		}
	},
	[IMX7CLK_I2C3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_I2C3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD2_DIV2
		}
	},
	[IMX7CLK_I2C4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_I2C4_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_ENET_PLL_DIV20, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_AUDIO_PLL, IMX7CLK_VIDEO_PLL,
			IMX7CLK_USB_PLL, IMX7CLK_SYS_PLL_PFD2_DIV2
		}
	},
	[IMX7CLK_UART1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK3, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART4_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK3, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART5_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART5_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART6_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART6_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK3, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_UART7_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_UART7_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_EXT_CLK4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_ECSPI1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ECSPI1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_SYS_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_ECSPI2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ECSPI2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_SYS_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_ECSPI3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ECSPI3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_SYS_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_ECSPI4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_ECSPI4_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV25, IMX7CLK_SYS_PLL_DIV4,
			IMX7CLK_SYS_PLL, IMX7CLK_SYS_PLL_PFD4,
			IMX7CLK_ENET_PLL_DIV4, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_PWM1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PWM1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK1,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_PWM2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PWM2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK1,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_PWM3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PWM3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_PWM4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_PWM4_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK2,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_FLEXTIMER1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_FLEXTIMER1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK3,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_FLEXTIMER2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_FLEXTIMER2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK3,
			IMX7CLK_REF_1M, IMX7CLK_VIDEO_PLL
		}
	},
	[IMX7CLK_SIM1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_SIM1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_USB_PLL, IMX7CLK_AUDIO_PLL,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_SIM2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_SIM2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_USB_PLL, IMX7CLK_VIDEO_PLL,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_SYS_PLL_PFD7
		}
	},
	[IMX7CLK_GPT1_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_GPT1_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_PFD0, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_VIDEO_PLL, IMX7CLK_REF_1M,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK1
		}
	},
	[IMX7CLK_GPT2_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_GPT2_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_PFD0, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_VIDEO_PLL, IMX7CLK_REF_1M,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK2
		}
	},
	[IMX7CLK_GPT3_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_GPT3_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_PFD0, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_VIDEO_PLL, IMX7CLK_REF_1M,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK3
		}
	},
	[IMX7CLK_GPT4_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_GPT4_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_ENET_PLL_DIV10,
			IMX7CLK_SYS_PLL_PFD0, IMX7CLK_ENET_PLL_DIV25,
			IMX7CLK_VIDEO_PLL, IMX7CLK_REF_1M,
			IMX7CLK_AUDIO_PLL, IMX7CLK_EXT_CLK4
		}
	},
	[IMX7CLK_TRACE_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_TRACE_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_USB_PLL,
			IMX7CLK_EXT_CLK2, IMX7CLK_EXT_CLK3
		}
	},
	[IMX7CLK_WDOG_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_WDOG_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_USB_PLL,
			IMX7CLK_REF_1M, IMX7CLK_SYS_PLL_PFD1_DIV2
		}
	},
	[IMX7CLK_CSI_MCLK_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_CSI_MCLK_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_AUDIO_MCLK_CLK_ROOT] = {
		.targetroot = CCM_CLKROOT_AUDIO_MCLK_CLK_ROOT,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_PFD2_DIV2,
			IMX7CLK_SYS_PLL_DIV4, IMX7CLK_DDR_PLL_DIV2,
			IMX7CLK_ENET_PLL_DIV8, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_USB_PLL
		}
	},
	[IMX7CLK_CCM_CLKO1] = {
		.targetroot = CCM_CLKROOT_CCM_CLKO1,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL,
			IMX7CLK_SYS_PLL_DIV2, IMX7CLK_SYS_PLL_PFD0_DIV2,
			IMX7CLK_SYS_PLL_PFD3, IMX7CLK_ENET_PLL_DIV2,
			IMX7CLK_DDR_PLL_DIV2, IMX7CLK_REF_1M
		}
	},
	[IMX7CLK_CCM_CLKO2] = {
		.targetroot = CCM_CLKROOT_CCM_CLKO2,
		.type = CLKTYPE_IP,
		.clksrc = {
			IMX7CLK_OSC_24M, IMX7CLK_SYS_PLL_DIV2,
			IMX7CLK_SYS_PLL_PFD0, IMX7CLK_SYS_PLL_PFD1_DIV2,
			IMX7CLK_SYS_PLL_PFD4, IMX7CLK_AUDIO_PLL,
			IMX7CLK_VIDEO_PLL, IMX7CLK_OSC_32K
		}
	}
};

struct imxccm_softc *ccm_softc;

static int imxccm_match(device_t, cfdata_t, void *);
static void imxccm_attach(device_t, device_t, void *);

static int imxccm_sysctl_freq_helper(SYSCTLFN_PROTO);
static int imxccm_sysctl_setup(struct imxccm_softc *);

CFATTACH_DECL_NEW(imxccm, sizeof(struct imxccm_softc),
    imxccm_match, imxccm_attach, NULL, NULL);

/* ARGSUSED */
static int
imxccm_match(device_t parent __unused, cfdata_t cfdata __unused, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (ccm_softc != NULL)
		return 0;

	if (aa->aa_addr == IMX7_AIPS_BASE + AIPS1_CCM_BASE)
		return 1;

	return 0;
}

/* ARGSUSED */
static void
imxccm_attach(device_t parent __unused, device_t self, void *aux)
{
	struct imxccm_softc * const sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;

	ccm_softc = sc;
	sc->sc_dev = self;
	sc->sc_iot = iot;
	sc->sc_log = NULL;

	if (bus_space_map(iot, IMX7_AIPS_BASE + AIPS1_CCM_BASE,
	    AIPS1_CCM_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map CCM registers\n");
		return;
	}

	if (bus_space_map(iot, IMX7_AIPS_BASE + AIPS1_CCM_ANALOG_BASE,
	    AIPS1_CCM_ANALOG_SIZE, 0, &sc->sc_ioh_analog)) {
		aprint_error(": can't map CCM_ANALOG registers\n");
		bus_space_unmap(iot, sc->sc_ioh, AIPS1_CCM_SIZE);
		return;
	}

	aprint_normal(": Clock Control Module\n");
	aprint_naive("\n");

	imxccm_sysctl_setup(sc);

	aprint_verbose_dev(self, "ARM_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_ARM_PLL));
	aprint_verbose_dev(self, "SYS_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_SYS_PLL));
	aprint_verbose_dev(self, "ENET_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_ENET_PLL));
	aprint_verbose_dev(self, "AUDIO_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_AUDIO_PLL));
	aprint_verbose_dev(self, "VIDEO_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_VIDEO_PLL));
	aprint_verbose_dev(self, "DDR_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_DDR_PLL));
	aprint_verbose_dev(self, "USB_PLL clock=%d\n",
	    imx7_get_clock(IMX7CLK_USB_PLL));
	aprint_verbose_dev(self, "AXI clock=%d\n",
	    imx7_get_clock(IMX7CLK_MAIN_AXI_CLK_ROOT));
	aprint_verbose_dev(self, "ENET AXI clock=%d\n",
	    imx7_get_clock(IMX7CLK_ENET_AXI_CLK_ROOT));

#ifdef CCM_DEBUG
#define CLOCK_PRINT(name)	\
	printf(# name " clock=%d\n", imx7_get_clock(name))

	CLOCK_PRINT(IMX7CLK_OSC_24M);

	CLOCK_PRINT(IMX7CLK_ARM_PLL);
	CLOCK_PRINT(IMX7CLK_SYS_PLL);
	CLOCK_PRINT(IMX7CLK_ENET_PLL);
	CLOCK_PRINT(IMX7CLK_AUDIO_PLL);
	CLOCK_PRINT(IMX7CLK_VIDEO_PLL);
	CLOCK_PRINT(IMX7CLK_DDR_PLL);
	CLOCK_PRINT(IMX7CLK_USB_PLL);

	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD0);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD1);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD2);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD3);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD4);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD5);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD6);
	CLOCK_PRINT(IMX7CLK_SYS_PLL_PFD7);

	CLOCK_PRINT(IMX7CLK_ARM_A7_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_ARM_M4_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_MAIN_AXI_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_ENET_AXI_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_AHB_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_IPG_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_DRAM_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_DRAM_ALT_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_USB_HSIC_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_NAND_USDHC_BUS_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_USDHC1_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_USDHC2_CLK_ROOT);
	CLOCK_PRINT(IMX7CLK_USDHC3_CLK_ROOT);
#endif
}

static int
imxccm_sysctl_setup(struct imxccm_softc *sc)
{
	const struct sysctlnode *node, *imxnode, *freqnode, *pllnode;
	int rv;

	rv = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &imxnode,
	    0, CTLTYPE_NODE, "imx7", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &imxnode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &pllnode,
	    0, CTLTYPE_NODE, "pll", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "arm",
	    SYSCTL_DESCR("frequency of ARM clock (ARM_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_arm_pll = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "system",
	    SYSCTL_DESCR("frequency of system clock (SYS_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_sys_pll = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "enet",
	    SYSCTL_DESCR("frequency of enet clock (ENET_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_enet_pll = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "audio",
	    SYSCTL_DESCR("frequency of audio clock (AUDIO_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_audio_pll = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "video",
	    SYSCTL_DESCR("frequency of video clock (VIDEO_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_video_pll  = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "ddr",
	    SYSCTL_DESCR("frequency of DDR clock (DDR_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_ddr_pll = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT, "usb",
	    SYSCTL_DESCR("frequency of USB clock (USB_PLL)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_usb_pll = node->sysctl_num;


	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "arm_a7",
	    SYSCTL_DESCR("frequency of ARM A7 Root clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_arm_a7_clk = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "arm_m4",
	    SYSCTL_DESCR("frequency of ARM M4 Root clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_arm_m4_clk = node->sysctl_num;


	return 0;

 fail:
	aprint_error_dev(sc->sc_dev, "cannot initialize sysctl (err=%d)\n", rv);

	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return -1;
}

static int
imxccm_sysctl_freq_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct imxccm_softc *sc;
	int value, ovalue, err;

	node = *rnode;
	sc = node.sysctl_data;

	/* for sysctl read */
	if (rnode->sysctl_num == sc->sc_sysctlnode_arm_pll)
		value = imx7_get_clock(IMX7CLK_ARM_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_sys_pll)
		value = imx7_get_clock(IMX7CLK_SYS_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_enet_pll)
		value = imx7_get_clock(IMX7CLK_ENET_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_audio_pll)
		value = imx7_get_clock(IMX7CLK_AUDIO_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_video_pll)
		value = imx7_get_clock(IMX7CLK_VIDEO_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_ddr_pll)
		value = imx7_get_clock(IMX7CLK_DDR_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_usb_pll)
		value = imx7_get_clock(IMX7CLK_USB_PLL);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_arm_a7_clk)
		value = imx7_get_clock(IMX7CLK_ARM_A7_CLK_ROOT);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_arm_m4_clk)
		value = imx7_get_clock(IMX7CLK_ARM_M4_CLK_ROOT);
	else
		return EOPNOTSUPP;

#ifdef SYSCTL_BY_MHZ
	value /= 1000 * 1000;	/* Hz -> MHz */
#endif
	ovalue = value;

	node.sysctl_data = &value;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err != 0 || newp == NULL)
		return err;

	/* for sysctl write */
	if (value == ovalue)
		return 0;

#ifdef SYSCTL_BY_MHZ
	value *= 1000 * 1000;	/* MHz -> Hz */
#endif

	if (rnode->sysctl_num == sc->sc_sysctlnode_arm_pll)
		return imx7_set_clock(IMX7CLK_ARM_PLL, value);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_arm_a7_clk)
		return imx7_set_clock(IMX7CLK_ARM_A7_CLK_ROOT, value);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_arm_m4_clk)
		return imx7_set_clock(IMX7CLK_ARM_M4_CLK_ROOT, value);

	return EOPNOTSUPP;
}


uint32_t
imx7_ccm_read(uint32_t reg)
{
	if (ccm_softc == NULL)
		return 0;

	return bus_space_read_4(ccm_softc->sc_iot, ccm_softc->sc_ioh, reg);
}

void
imx7_ccm_write(uint32_t reg, uint32_t val)
{
	if (ccm_softc == NULL)
		return;

	bus_space_write_4(ccm_softc->sc_iot, ccm_softc->sc_ioh, reg, val);
}

uint32_t
imx7_ccm_analog_read(uint32_t reg)
{
	if (ccm_softc == NULL)
		return 0;

	return bus_space_read_4(ccm_softc->sc_iot, ccm_softc->sc_ioh_analog,
	    reg);
}

void
imx7_ccm_analog_write(uint32_t reg, uint32_t val)
{
	if (ccm_softc == NULL)
		return;

	bus_space_write_4(ccm_softc->sc_iot, ccm_softc->sc_ioh_analog, reg,
	    val);
}

static uint64_t
rootclk(int clk, bool nodiv)
{
	uint32_t d, v;
	uint64_t freq;

	/* read CCM_TARGET_ROOTxx */
	v = imx7_ccm_read(imx7clkroottbl[clk].targetroot);

	if ((v & CCM_TARGET_ROOT_ENABLE) == 0)
		return 0;

	d = __SHIFTOUT(v, CCM_TARGET_ROOT_MUX);
	freq = imx7_get_clock(imx7clkroottbl[clk].clksrc[d]);

#ifdef CCM_DEBUG
	if (ccm_debug) {
		printf("TARGET_ROOT[%08x]: value=%08x, mux=%d, freq=%llu\n",
		    imx7clkroottbl[clk].targetroot, v, d, freq);
	}
#endif

	if (nodiv)
		return freq;

	/* eval TARGET_ROOT[PRE_PODF] */
	if ((imx7clkroottbl[clk].type != CLKTYPE_CORE) &&
	    (imx7clkroottbl[clk].type != CLKTYPE_DRAM) &&
	    (imx7clkroottbl[clk].type != CLKTYPE_DRAM_PHYM)) {
		d = __SHIFTOUT(v, CCM_TARGET_ROOT_PRE_PODF) + 1;
		freq /= d;
#ifdef CCM_DEBUG
		if (ccm_debug) {
			printf("TARGET_ROOT[%08x]: pre_podf=%d, freq=%llu\n",
			    imx7clkroottbl[clk].targetroot, d, freq);
		}
#endif
	}

	/* eval TARGET_ROOT[POST_PODF] */
	if (clk != IMX7CLK_DRAM_PHYM_CLK_ROOT) {
		d = __SHIFTOUT(v, CCM_TARGET_ROOT_POST_PODF) + 1;
		if (clk == IMX7CLK_DRAM_CLK_ROOT)
			d &= 7;
		freq /= d;
#ifdef CCM_DEBUG
		if (ccm_debug) {
			printf("TARGET_ROOT[%08x]: post_podf=%d, freq=%llu\n",
			    imx7clkroottbl[clk].targetroot, d, freq);
		}
#endif
	}

	/* eval TARGET_ROOT[AUTO_PODF] (from u-boot source, not datasheet) */
	if ((imx7clkroottbl[clk].type != CLKTYPE_AHB) &&
	    (imx7clkroottbl[clk].type != CLKTYPE_BUS)) {
		if (v & CCM_TARGET_ROOT_AUTO_ENABLE) {
			d = __SHIFTOUT(v, CCM_TARGET_ROOT_AUTO_PODF) + 1;
			freq /= d;
#ifdef CCM_DEBUG
			if (ccm_debug) {
				printf("TARGET_ROOT[%08x]: "
				    "auto_podf=%d, freq=%llu\n",
				    imx7clkroottbl[clk].targetroot, d, freq);
			}
#endif
		}
	}

	return freq;
}

/* see also iMX7 Reference Manual 5.2.6.1 Input Clocks */
uint32_t
imx7_get_clock(enum imx7_clock clk)
{
	uint32_t d, denom, num, v;
	uint64_t freq = 0;

	if (ccm_softc == NULL)
		return 0;

	switch (clk) {
	case IMX7CLK_OSC_24M:
		freq = IMX7_OSC_FREQ;	/* fixed clock */
		break;
	case IMX7CLK_REF_1M:
		freq = IMX7_OSC_FREQ / 24;	/* fixed clock */
		break;
	case IMX7CLK_OSC_32K:
		freq = IMX7_OSC_FREQ / 768;	/* fixed clock */
		break;

	case IMX7CLK_ARM_PLL:
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_ARM);
		freq = IMX7_OSC_FREQ *
		    __SHIFTOUT(v, CCM_ANALOG_PLL_ARM_DIV_SELECT) / 2;
		break;

	case IMX7CLK_DDR_PLL:
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_DDR);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_DDR_DIV_SELECT);
		num = imx7_ccm_analog_read(CCM_ANALOG_PLL_DDR_NUM);
		denom = imx7_ccm_analog_read(CCM_ANALOG_PLL_DDR_DENOM);
		freq = (uint64_t)IMX7_OSC_FREQ * (d + num / denom);
		if (v & CCM_ANALOG_PLL_DDR_DIV2_ENABLE_CLK)
			freq /= 2;
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT);
		switch (d) {
		case 0:
			freq /= 4;
			break;
		case 1:
			freq /= 2;
			break;
		case 2:
		case 3:
			break;
		}
		break;
	case IMX7CLK_DDR_PLL_DIV2:
		freq = imx7_get_clock(IMX7CLK_DDR_PLL) / 2;
		break;

	case IMX7CLK_SYS_PLL:
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_480);
		freq = IMX7_OSC_FREQ *
		    ((v & CCM_ANALOG_PLL_480_DIV_SELECT) ? 22 : 20);
		break;
	case IMX7CLK_SYS_PLL_DIV2:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL) / 2;
		break;
	case IMX7CLK_SYS_PLL_DIV4:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL) / 4;
		break;
	case IMX7CLK_SYS_PLL_PFD0:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480A);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480A_PDF0_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD0_DIV2:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL_PFD0) / 2;
		break;
	case IMX7CLK_SYS_PLL_PFD1:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480A);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480A_PDF1_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD1_DIV2:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL_PFD1) / 2;
		break;
	case IMX7CLK_SYS_PLL_PFD2:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480A);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480A_PDF2_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD2_DIV2:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL_PFD2) / 2;
		break;
	case IMX7CLK_SYS_PLL_PFD3:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480A);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480A_PDF3_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD4:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480B);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480B_PDF4_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD5:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480B);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480B_PDF5_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD6:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480B);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480B_PDF6_FRAC);
		break;
	case IMX7CLK_SYS_PLL_PFD7:
		freq = imx7_get_clock(IMX7CLK_SYS_PLL);
		v = imx7_ccm_analog_read(CCM_ANALOG_PFD_480B);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480B_PDF7_FRAC);
		break;

	case IMX7CLK_ENET_PLL:
		freq = IMX7_ENET_PLL_CLK;	/* fixed clock */
		break;
	case IMX7CLK_ENET_PLL_DIV2:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 2;
		break;
	case IMX7CLK_ENET_PLL_DIV4:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 4;
		break;
	case IMX7CLK_ENET_PLL_DIV8:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 8;
		break;
	case IMX7CLK_ENET_PLL_DIV10:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 10;
		break;
	case IMX7CLK_ENET_PLL_DIV20:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 20;
		break;
	case IMX7CLK_ENET_PLL_DIV25:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 25;
		break;
	case IMX7CLK_ENET_PLL_DIV40:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 40;
		break;
	case IMX7CLK_ENET_PLL_DIV50:
		freq = imx7_get_clock(IMX7CLK_ENET_PLL) / 50;
		break;

	case IMX7CLK_USB_PLL:
		freq = IMX7_USB_PLL_CLK;	/* fixed clock */
		break;
	case IMX7CLK_AUDIO_PLL:
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_AUDIO);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_AUDIO_DIV_SELECT);
		num = imx7_ccm_analog_read(CCM_ANALOG_PLL_AUDIO_NUM);
		denom = imx7_ccm_analog_read(CCM_ANALOG_PLL_AUDIO_DENOM);
		freq = (uint64_t)IMX7_OSC_FREQ * (d + num / denom);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_AUDIO_TEST_DIV_SELECT);
		switch (d) {
		case 0:
			freq /= 4;
			break;
		case 1:
			freq /= 2;
			break;
		case 2:
		case 3:
			break;
		}
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT);
		switch (d) {
		case 0:
		case 2:
			break;
		case 1:
			freq /= 2;
			break;
		case 3:
			freq /= 4;
			break;
		}
		break;
	case IMX7CLK_VIDEO_PLL:
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_VIDEO);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_VIDEO_DIV_SELECT);
		num = imx7_ccm_analog_read(CCM_ANALOG_PLL_VIDEO_NUM);
		denom = imx7_ccm_analog_read(CCM_ANALOG_PLL_VIDEO_DENOM);
		freq = (uint64_t)IMX7_OSC_FREQ * (d + num / denom);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_VIDEO_TEST_DIV_SELECT);
		switch (d) {
		case 0:
			freq /= 4;
			break;
		case 1:
			freq /= 2;
			break;
		case 2:
		case 3:
			break;
		}
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT);
		switch (d) {
		case 0:
		case 2:
			break;
		case 1:
			freq /= 2;
			break;
		case 3:
			freq /= 4;
			break;
		}
		break;

	case IMX7CLK_ARM_A7_CLK_ROOT:
	case IMX7CLK_ARM_M4_CLK_ROOT:
	case IMX7CLK_MAIN_AXI_CLK_ROOT:
	case IMX7CLK_DISP_AXI_CLK_ROOT:
	case IMX7CLK_ENET_AXI_CLK_ROOT:
	case IMX7CLK_NAND_USDHC_BUS_CLK_ROOT:
	case IMX7CLK_AHB_CLK_ROOT:
	case IMX7CLK_IPG_CLK_ROOT:
	case IMX7CLK_DRAM_PHYM_CLK_ROOT:
	case IMX7CLK_DRAM_CLK_ROOT:
	case IMX7CLK_DRAM_PHYM_ALT_CLK_ROOT:
	case IMX7CLK_DRAM_ALT_CLK_ROOT:
	case IMX7CLK_USB_HSIC_CLK_ROOT:
	case IMX7CLK_PCIE_CTRL_CLK_ROOT:
	case IMX7CLK_PCIE_PHY_CLK_ROOT:
	case IMX7CLK_EPDC_PIXEL_CLK_ROOT:
	case IMX7CLK_LCDIF_PIXEL_CLK_ROOT:
	case IMX7CLK_MIPI_DSI_CLK_ROOT:
	case IMX7CLK_MIPI_CSI_CLK_ROOT:
	case IMX7CLK_MIPI_DPHY_REF_CLK_ROOT:
	case IMX7CLK_SAI1_CLK_ROOT:
	case IMX7CLK_SAI2_CLK_ROOT:
	case IMX7CLK_SAI3_CLK_ROOT:
	case IMX7CLK_ENET1_REF_CLK_ROOT:
	case IMX7CLK_ENET1_TIME_CLK_ROOT:
	case IMX7CLK_ENET2_REF_CLK_ROOT:
	case IMX7CLK_ENET2_TIME_CLK_ROOT:
	case IMX7CLK_ENET_PHY_REF_CLK_ROOT:
	case IMX7CLK_EIM_CLK_ROOT:
	case IMX7CLK_NAND_CLK_ROOT:
	case IMX7CLK_QSPI_CLK_ROOT:
	case IMX7CLK_USDHC1_CLK_ROOT:
	case IMX7CLK_USDHC2_CLK_ROOT:
	case IMX7CLK_USDHC3_CLK_ROOT:
	case IMX7CLK_CAN1_CLK_ROOT:
	case IMX7CLK_CAN2_CLK_ROOT:
	case IMX7CLK_I2C1_CLK_ROOT:
	case IMX7CLK_I2C2_CLK_ROOT:
	case IMX7CLK_I2C3_CLK_ROOT:
	case IMX7CLK_I2C4_CLK_ROOT:
	case IMX7CLK_UART1_CLK_ROOT:
	case IMX7CLK_UART2_CLK_ROOT:
	case IMX7CLK_UART3_CLK_ROOT:
	case IMX7CLK_UART4_CLK_ROOT:
	case IMX7CLK_UART5_CLK_ROOT:
	case IMX7CLK_UART6_CLK_ROOT:
	case IMX7CLK_UART7_CLK_ROOT:
	case IMX7CLK_ECSPI1_CLK_ROOT:
	case IMX7CLK_ECSPI2_CLK_ROOT:
	case IMX7CLK_ECSPI3_CLK_ROOT:
	case IMX7CLK_ECSPI4_CLK_ROOT:
	case IMX7CLK_PWM1_CLK_ROOT:
	case IMX7CLK_PWM2_CLK_ROOT:
	case IMX7CLK_PWM3_CLK_ROOT:
	case IMX7CLK_PWM4_CLK_ROOT:
	case IMX7CLK_FLEXTIMER1_CLK_ROOT:
	case IMX7CLK_FLEXTIMER2_CLK_ROOT:
	case IMX7CLK_SIM1_CLK_ROOT:
	case IMX7CLK_SIM2_CLK_ROOT:
	case IMX7CLK_GPT1_CLK_ROOT:
	case IMX7CLK_GPT2_CLK_ROOT:
	case IMX7CLK_GPT3_CLK_ROOT:
	case IMX7CLK_GPT4_CLK_ROOT:
	case IMX7CLK_TRACE_CLK_ROOT:
	case IMX7CLK_WDOG_CLK_ROOT:
	case IMX7CLK_CSI_MCLK_CLK_ROOT:
	case IMX7CLK_AUDIO_MCLK_CLK_ROOT:
	case IMX7CLK_CCM_CLKO1:
	case IMX7CLK_CCM_CLKO2:
		freq = rootclk(clk, false);
		break;

	default:
		aprint_error_dev(ccm_softc->sc_dev,
		    "%s: clockid %d: not supported\n", __func__, clk);
		return 0;
	}

	return freq;
}

/*
 * resolve two clock divisors d3 and d6.
 * freq = maxfreq / d3(3bit) / d6(6bit)
 */
static void
getrootclkdiv(uint32_t maxfreq, uint32_t freq, uint32_t *d3p, uint32_t *d6p)
{
	uint32_t c_dif, c_d3, c_d6;
	uint32_t dif, d3, d6;
	uint32_t lim, base, f;

	c_dif = 0xffffffff;
	c_d3 = c_d6 = 0;
	for (d3 = 0; d3 < 8; d3++) {
		base = 0;
		for (lim = 64; lim != 0; lim >>= 1) {
			d6 = (lim >> 1) + base;
			f = maxfreq / (d3 + 1) / (d6 + 1);
			if (freq < f) {
				base = d6 + 1;
				lim--;
			}
			dif = (freq > f) ? freq - f : f - freq;
			if (c_dif > dif) {
				c_dif = dif;
				c_d3 = d3;
				c_d6 = d6;
			}
		}
	}
	*d3p = c_d3;
	*d6p = c_d6;
}

int
imx7_set_clock(enum imx7_clock clk, uint32_t freq)
{
	uint32_t v, x;
	uint32_t d3, d6, maxfreq;

	if (ccm_softc == NULL)
		return 0;

	switch (clk) {
	case IMX7CLK_ARM_PLL:
		x = (uint64_t)freq * 2 / IMX7_OSC_FREQ;
		v = imx7_ccm_analog_read(CCM_ANALOG_PLL_ARM);
		v &= ~CCM_ANALOG_PLL_ARM_DIV_SELECT;
		v |=  __SHIFTIN(x, CCM_ANALOG_PLL_ARM_DIV_SELECT);
		imx7_ccm_analog_write(CCM_ANALOG_PLL_ARM, v);
		break;

	/* core type clock */
	case IMX7CLK_ARM_A7_CLK_ROOT:
		maxfreq = rootclk(clk, true);
		getrootclkdiv(maxfreq, freq, &d3, &d6);
		v = imx7_ccm_read(imx7clkroottbl[clk].targetroot);
		/* core type clocks use AUTO_PODF and POST_PODF */
		v &= ~CCM_TARGET_ROOT_AUTO_PODF;
		v |= __SHIFTIN(d3, CCM_TARGET_ROOT_AUTO_PODF);
		v |= CCM_TARGET_ROOT_AUTO_ENABLE;
		v &= ~CCM_TARGET_ROOT_POST_PODF;
		v |= __SHIFTIN(d6, CCM_TARGET_ROOT_POST_PODF);
		imx7_ccm_write(imx7clkroottbl[clk].targetroot, v);
		break;

	/* bus type clocks */
	case IMX7CLK_ARM_M4_CLK_ROOT:
	case IMX7CLK_MAIN_AXI_CLK_ROOT:
	case IMX7CLK_DISP_AXI_CLK_ROOT:
		maxfreq = rootclk(clk, true);
		getrootclkdiv(maxfreq, freq, &d3, &d6);
		/* bus type clocks use PRE_PODF and POST_PODF */
		v = imx7_ccm_read(imx7clkroottbl[clk].targetroot);
		v &= ~CCM_TARGET_ROOT_PRE_PODF;
		v |= __SHIFTIN(d3, CCM_TARGET_ROOT_PRE_PODF);
		v &= ~CCM_TARGET_ROOT_POST_PODF;
		v |= __SHIFTIN(d6, CCM_TARGET_ROOT_POST_PODF);
		imx7_ccm_write(imx7clkroottbl[clk].targetroot, v);
		break;

	default:
		aprint_error_dev(ccm_softc->sc_dev,
		    "%s: clockid %d: not supported\n", __func__, clk);
		return EINVAL;
	}

	return 0;
}

int
imx7_pll_power(uint32_t pllreg, int on)
{
	uint32_t v;
	int timeout;

	switch (pllreg) {
	case CCM_ANALOG_PLL_ENET:
		v = imx7_ccm_analog_read(pllreg);
		if (on)
			v &= ~CCM_ANALOG_PLL_ENET_POWERDOWN;
		else
			v |= CCM_ANALOG_PLL_ENET_POWERDOWN;
		imx7_ccm_analog_write(pllreg, v);

		for (timeout = 100000; timeout > 0; timeout--) {
			if (imx7_ccm_analog_read(pllreg) &
			    CCM_ANALOG_PLL_ENET_LOCK)
				break;
		}
		if (timeout <= 0)
			break;

		if (on) {
			v &= ~CCM_ANALOG_PLL_ENET_BYPASS;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_500MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_250MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_125MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_100MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_50MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_40MHZ;
			v |= CCM_ANALOG_PLL_ENET_ENABLE_CLK_25MHZ;
		} else {
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_500MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_250MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_125MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_100MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_50MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_40MHZ;
			v &= ~CCM_ANALOG_PLL_ENET_ENABLE_CLK_25MHZ;
		}
		imx7_ccm_analog_write(pllreg, v);
		return 0;

	case CCM_ANALOG_PLL_ARM:
	case CCM_ANALOG_PLL_DDR:
	case CCM_ANALOG_PLL_480:
	case CCM_ANALOG_PLL_AUDIO:
	case CCM_ANALOG_PLL_VIDEO:
	default:
		break;
	}

	return -1;
}
