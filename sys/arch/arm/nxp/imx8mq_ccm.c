/* $NetBSD: imx8mq_ccm.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: imx8mq_ccm.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/nxp/imx_ccm.h>
#include <arm/nxp/imx8mq_ccm.h>

static int imx8mq_ccm_match(device_t, cfdata_t, void *);
static void imx8mq_ccm_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx8mq-ccm" },
	DEVICE_COMPAT_EOL
};

static const char *uart_p[] = {
	"osc_25m", "sys1_pll_80m", "sys2_pll_200m", "sys2_pll_100m", "sys3_pll_out", "clk_ext2", "clk_ext4", "audio_pll2_out"
};
static const char *usdhc_p[] = {
	"osc_25m", "sys1_pll_400m", "sys1_pll_800m", "sys2_pll_500m", "audio_pll2_out", "sys1_pll_266m", "sys3_pll_out", "sys1_pll_100m"
};
static const char *enet_axi_p[] = {
	"osc_25m", "sys1_pll_266m", "sys1_pll_800m", "sys2_pll_250m", "sys2_pll_200m", "audio_pll1_out", "video_pll1_out", "sys3_pll_out"
};
static const char *enet_ref_p[] = {
	"osc_25m", "sys2_pll_125m", "sys2_pll_500m", "sys2_pll_100m", "sys1_pll_160m", "audio_pll1_out", "video_pll1_out", "clk_ext4"
};
static const char *enet_timer_p[] = {
	"osc_25m", "sys2_pll_100m", "audio_pll1_out", "clk_ext1", "clk_ext2", "clk_ext3", "clk_ext4", "video_pll1_out"
};
static const char *enet_phy_ref_p[] = {
	"osc_25m", "sys2_pll_50m", "sys2_pll_125m", "sys2_pll_500m", "audio_pll1_out", "video_pll1_out", "audio_pll2_out"
};
static const char *usb_bus_p[] = {
	"osc_25m", "sys2_pll_500m", "sys1_pll_800m", "sys2_pll_100m", "sys2_pll_200m", "clk_ext2", "clk_ext4", "audio_pll2_out"
};
static const char *usb_core_phy_p[] = {
	"osc_25m", "sys1_pll_100m", "sys1_pll_40m", "sys2_pll_100m", "sys2_pll_200m", "clk_ext2", "clk_ext3", "audio_pll2_out"
};
static const char *i2c_p[] = {
	"osc_25m", "sys1_pll_160m", "sys2_pll_50m", "sys3_pll_out", "audio_pll1_out", "video_pll1_out", "audio_pll2_out", "sys1_pll_133m"
};

CFATTACH_DECL_NEW(imx8mq_ccm, sizeof(struct imx_ccm_softc),
	imx8mq_ccm_match, imx8mq_ccm_attach, NULL, NULL);

static struct imx_ccm_clk imx8mq_ccm_clks[] = {

	IMX_FIXED(CLK_DUMMY, "dummy", 0),
	IMX_EXTCLK(CLK_32K, "ckil"),
	IMX_EXTCLK(CLK_25M, "osc_25m"),
	IMX_EXTCLK(CLK_27M, "osc_27m"),
	IMX_EXTCLK(CLK_EXT1, "clk_ext1"),
	IMX_EXTCLK(CLK_EXT2, "clk_ext2"),
	IMX_EXTCLK(CLK_EXT3, "clk_ext3"),
	IMX_EXTCLK(CLK_EXT4, "clk_ext4"),

	IMX_FIXED(SYS1_PLL_OUT, "sys1_pll_out", 800000000),
	IMX_FIXED(SYS2_PLL_OUT, "sys2_pll_out", 1000000000),

	IMX_GATE(SYS1_PLL_40M_CG, "sys1_pll_40m_cg", "sys1_pll_out", 0x30, __BIT(9)),
	IMX_GATE(SYS1_PLL_80M_CG, "sys1_pll_80m_cg", "sys1_pll_out", 0x30, __BIT(11)),
	IMX_GATE(SYS1_PLL_100M_CG, "sys1_pll_100m_cg", "sys1_pll_out", 0x30, __BIT(13)),
	IMX_GATE(SYS1_PLL_133M_CG, "sys1_pll_133m_cg", "sys1_pll_out", 0x30, __BIT(15)),
	IMX_GATE(SYS1_PLL_160M_CG, "sys1_pll_160m_cg", "sys1_pll_out", 0x30, __BIT(17)),
	IMX_GATE(SYS1_PLL_200M_CG, "sys1_pll_200m_cg", "sys1_pll_out", 0x30, __BIT(19)),
	IMX_GATE(SYS1_PLL_266M_CG, "sys1_pll_266m_cg", "sys1_pll_out", 0x30, __BIT(21)),
	IMX_GATE(SYS1_PLL_400M_CG, "sys1_pll_400m_cg", "sys1_pll_out", 0x30, __BIT(23)),
	IMX_GATE(SYS1_PLL_800M_CG, "sys1_pll_800m_cg", "sys1_pll_out", 0x30, __BIT(25)),

	IMX_FIXED_FACTOR(SYS1_PLL_40M, "sys1_pll_40m", "sys1_pll_40m_cg", 1, 20),
	IMX_FIXED_FACTOR(SYS1_PLL_80M, "sys1_pll_80m", "sys1_pll_80m_cg", 1, 10),
	IMX_FIXED_FACTOR(SYS1_PLL_100M, "sys1_pll_100m", "sys1_pll_100m_cg", 1, 8),
	IMX_FIXED_FACTOR(SYS1_PLL_133M, "sys1_pll_133m", "sys1_pll_133m_cg", 1, 6),
	IMX_FIXED_FACTOR(SYS1_PLL_160M, "sys1_pll_160m", "sys1_pll_160m_cg", 1, 5),
	IMX_FIXED_FACTOR(SYS1_PLL_200M, "sys1_pll_200m", "sys1_pll_200m_cg", 1, 4),
	IMX_FIXED_FACTOR(SYS1_PLL_266M, "sys1_pll_266m", "sys1_pll_266m_cg", 1, 3),
	IMX_FIXED_FACTOR(SYS1_PLL_400M, "sys1_pll_400m", "sys1_pll_400m_cg", 1, 2),
	IMX_FIXED_FACTOR(SYS1_PLL_800M, "sys1_pll_800m", "sys1_pll_800m_cg", 1, 1),

	IMX_GATE(SYS2_PLL_50M_CG, "sys2_pll_50m_cg", "sys2_pll_out", 0x3c, __BIT(9)),
	IMX_GATE(SYS2_PLL_100M_CG, "sys2_pll_100m_cg", "sys2_pll_out", 0x3c, __BIT(11)),
	IMX_GATE(SYS2_PLL_125M_CG, "sys2_pll_125m_cg", "sys2_pll_out", 0x3c, __BIT(13)),
	IMX_GATE(SYS2_PLL_166M_CG, "sys2_pll_166m_cg", "sys2_pll_out", 0x3c, __BIT(15)),
	IMX_GATE(SYS2_PLL_200M_CG, "sys2_pll_200m_cg", "sys2_pll_out", 0x3c, __BIT(17)),
	IMX_GATE(SYS2_PLL_250M_CG, "sys2_pll_250m_cg", "sys2_pll_out", 0x3c, __BIT(19)),
	IMX_GATE(SYS2_PLL_333M_CG, "sys2_pll_333m_cg", "sys2_pll_out", 0x3c, __BIT(21)),
	IMX_GATE(SYS2_PLL_500M_CG, "sys2_pll_500m_cg", "sys2_pll_out", 0x3c, __BIT(23)),
	IMX_GATE(SYS2_PLL_1000M_CG, "sys2_pll_1000m_cg", "sys2_pll_out", 0x3c, __BIT(25)),

	IMX_FIXED_FACTOR(SYS2_PLL_50M, "sys2_pll_50m", "sys2_pll_50m_cg", 1, 20),
	IMX_FIXED_FACTOR(SYS2_PLL_100M, "sys2_pll_100m", "sys2_pll_100m_cg", 1, 10),
	IMX_FIXED_FACTOR(SYS2_PLL_125M, "sys2_pll_125m", "sys2_pll_125m_cg", 1, 8),
	IMX_FIXED_FACTOR(SYS2_PLL_166M, "sys2_pll_166m", "sys2_pll_166m_cg", 1, 6),
	IMX_FIXED_FACTOR(SYS2_PLL_200M, "sys2_pll_200m", "sys2_pll_200m_cg", 1, 5),
	IMX_FIXED_FACTOR(SYS2_PLL_250M, "sys2_pll_250m", "sys2_pll_250m_cg", 1, 4),
	IMX_FIXED_FACTOR(SYS2_PLL_333M, "sys2_pll_333m", "sys2_pll_333m_cg", 1, 3),
	IMX_FIXED_FACTOR(SYS2_PLL_500M, "sys2_pll_500m", "sys2_pll_500m_cg", 1, 2),
	IMX_FIXED_FACTOR(SYS2_PLL_1000M, "sys2_pll_1000m", "sys2_pll_1000m_cg", 1, 1),

	IMX_COMPOSITE(CLK_UART1, "uart1", uart_p, 0xaf00, 0),
	IMX_COMPOSITE(CLK_UART2, "uart2", uart_p, 0xaf80, 0),
	IMX_COMPOSITE(CLK_UART3, "uart3", uart_p, 0xb000, 0),
	IMX_COMPOSITE(CLK_UART4, "uart4", uart_p, 0xb080, 0),

	IMX_ROOT_GATE(CLK_UART1_ROOT, "uart1_root_clk", "uart1", 0x4490),
	IMX_ROOT_GATE(CLK_UART2_ROOT, "uart2_root_clk", "uart2", 0x44a0),
	IMX_ROOT_GATE(CLK_UART3_ROOT, "uart3_root_clk", "uart3", 0x44b0),
	IMX_ROOT_GATE(CLK_UART4_ROOT, "uart4_root_clk", "uart4", 0x44c0),

	IMX_COMPOSITE(CLK_USDHC1, "usdhc1", usdhc_p, 0xac00, IMX_COMPOSITE_ROUND_DOWN),
	IMX_COMPOSITE(CLK_USDHC2, "usdhc2", usdhc_p, 0xac80, IMX_COMPOSITE_ROUND_DOWN),

	IMX_ROOT_GATE(CLK_USDHC1_ROOT, "usdhc1_root_clk", "usdhc1", 0x4510),
	IMX_ROOT_GATE(CLK_USDHC2_ROOT, "usdhc2_root_clk", "usdhc2", 0x4520),

	IMX_COMPOSITE(CLK_ENET_AXI, "enet_axi", enet_axi_p, 0x8800, 0),
	IMX_COMPOSITE(CLK_ENET_REF, "enet_ref", enet_ref_p, 0xa980, 0),
	IMX_COMPOSITE(CLK_ENET_TIMER, "enet_timer", enet_timer_p, 0xaa00, 0),
	IMX_COMPOSITE(CLK_ENET_PHY_REF, "enet_phy_ref", enet_phy_ref_p, 0xaa80, 0),

	IMX_ROOT_GATE(CLK_ENET1_ROOT, "enet1_root_clk", "enet_axi", 0x40a0),

	IMX_COMPOSITE(CLK_USB_BUS, "usb_bus", usb_bus_p, 0x8b80, 0),
	IMX_COMPOSITE(CLK_USB_CORE_REF, "usb_core_ref", usb_core_phy_p, 0xb100, 0),
	IMX_COMPOSITE(CLK_USB_PHY_REF, "usb_phy_ref", usb_core_phy_p, 0xb180, 0),

	IMX_ROOT_GATE(CLK_USB1_CTRL_ROOT, "usb1_ctrl_root_clk", "usb_bus", 0x44d0),
	IMX_ROOT_GATE(CLK_USB2_CTRL_ROOT, "usb2_ctrl_root_clk", "usb_bus", 0x44e0),
	IMX_ROOT_GATE(CLK_USB1_PHY_ROOT, "usb1_phy_root_clk", "usb_phy_ref", 0x44f0),
	IMX_ROOT_GATE(CLK_USB2_PHY_ROOT, "usb2_phy_root_clk", "usb_phy_ref", 0x4500),

	IMX_COMPOSITE(CLK_I2C1, "i2c1", i2c_p, 0xad00, 0),
	IMX_COMPOSITE(CLK_I2C2, "i2c2", i2c_p, 0xad80, 0),
	IMX_COMPOSITE(CLK_I2C3, "i2c3", i2c_p, 0xae00, 0),
	IMX_COMPOSITE(CLK_I2C4, "i2c4", i2c_p, 0xae80, 0),

	IMX_ROOT_GATE(CLK_I2C1_ROOT, "i2c1_root_clk", "i2c1", 0x4170),
	IMX_ROOT_GATE(CLK_I2C2_ROOT, "i2c2_root_clk", "i2c2", 0x4180),
	IMX_ROOT_GATE(CLK_I2C3_ROOT, "i2c3_root_clk", "i2c3", 0x4190),
	IMX_ROOT_GATE(CLK_I2C4_ROOT, "i2c4_root_clk", "i2c4", 0x41a0),
};

static int
imx8mq_ccm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx8mq_ccm_attach(device_t parent, device_t self, void *aux)
{
	struct imx_ccm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_clks = imx8mq_ccm_clks;
	sc->sc_nclks = __arraycount(imx8mq_ccm_clks);

	if (imx_ccm_attach(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": Clock Control Module\n");

	imx_ccm_print(sc);
}
