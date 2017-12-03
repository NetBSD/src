/*	$NetBSD: nitrogen6_iomux.c,v 1.4.4.2 2017/12/03 11:36:06 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nitrogen6_iomux.c,v 1.4.4.2 2017/12/03 11:36:06 jdolecek Exp $");

#include "opt_evbarm_boardtype.h"
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>
#include <evbarm/nitrogen6/platform.h>

struct gpio_conf {
	uint8_t group;
	uint8_t pin;
	uint8_t dir;
	uint8_t value;
};

void nitrogen6_setup_iomux(void);
void nitrogen6_device_register(device_t, void *);
static void nitrogen6_mux_config(const struct iomux_conf *);
static void nitrogen6_gpio_config(const struct gpio_conf *);


#define	nitrogen6x		1
#define	nitrogen6max		2
#define	cubox_i			3
#define	hummingboard		4
#define	hummingboard_edge	5
#define	ccimx6ulstarter		6

#define PAD_UART	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST)
#define PAD_USB		\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST)

#define PAD_USDHC_50MHZ		\
		(PAD_CTL_HYS | PAD_CTL_PUS_47K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_50MHZ | PAD_CTL_DSE_80OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_100MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_47K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_200MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_47K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_200MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC	PAD_USDHC_50MHZ


#define PAD_USDHC_DATA3_50MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_50MHZ | PAD_CTL_DSE_80OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_DATA3_100MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_DATA3_200MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_200MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)

#define PAD_USDHC_CLK_50MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PD |	\
		 PAD_CTL_SPEED_50MHZ | PAD_CTL_DSE_80OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_CLK_100MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PD |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)
#define PAD_USDHC_CLK_200MHZ	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PD |	\
		 PAD_CTL_SPEED_200MHZ | PAD_CTL_DSE_34OHM | PAD_CTL_SRE_FAST)

#define PAD_USDHC_VSELECT	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_SLOW)
#define PAD_USDHC_GPIO	\
		(PAD_CTL_HYS | PAD_CTL_PUS_22K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_50MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST)

#define PAD_WEAK_PULLUP	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_SLOW)
#define PAD_WEAK_PULLDOWN	\
		(PAD_CTL_HYS | PAD_CTL_PUS_100K_PD | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_SLOW)
#define PAD_OUTPUT_40OHM	(PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM)

#define PAD_ENET6UL \
		(PAD_CTL_PUS_100K_PU | PAD_CTL_PULL | \
		 PAD_CTL_SPEED_200MHZ | PAD_CTL_DSE_45OHM | PAD_CTL_SRE_FAST)
#define PAD_ENET6UL_REFCLK \
		(PAD_CTL_PUS_100K_PD | \
		 PAD_CTL_SPEED_LOW50MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST)

#define PAD_LCD_HSYNC \
		(PAD_CTL_PUS_100K_PD | PAD_CTL_KEEPER | \
		 PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_SLOW)

#define PAD_PCIE_GPIO \
		(PAD_CTL_HYS | PAD_CTL_PUS_22K_PU | PAD_CTL_PULL |	\
		 PAD_CTL_SPEED_50MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST)


/* iMX6 SoloLite */
static const struct iomux_conf iomux_data_6sl[] = {
	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};

/* iMX6 Solo/DualLite */
static const struct iomux_conf iomux_data_6sdl[] = {
	/* SolidRun CuBox-I, HummingBoard */
#if (EVBARM_BOARDTYPE == cubox_i)
	/* USDHC2 */
	{
		.pin = MUX_PIN(IMX6SDL, SD2_CLK),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_CLK */
		.pad = PAD_USDHC_CLK_100MHZ
	},
	{
		.pin = MUX_PIN(IMX6SDL, SD2_CMD),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_CMD */
		.pad = PAD_USDHC_100MHZ
	},
	{
		.pin = IOMUX_PIN(IMX6SDL_IOMUXC_USDHC2_CARD_CLK_IN_SELECT_INPUT,
		    IOMUX_PAD_NONE),
		.mux = INPUT_DAISY_1,		/* SD2_CLK_ALT0 */
	},
	{
		.pin = MUX_PIN(IMX6SDL, SD2_DATA0),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA0 */
		.pad = PAD_USDHC_100MHZ
	},
	{
		.pin = MUX_PIN(IMX6SDL, SD2_DATA1),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA1 */
		.pad = PAD_USDHC_100MHZ
	},
	{
		.pin = MUX_PIN(IMX6SDL, SD2_DATA2),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA2 */
		.pad = PAD_USDHC_100MHZ
	},
	{
		.pin = MUX_PIN(IMX6SDL, SD2_DATA3),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA3 */
		.pad = PAD_USDHC_100MHZ
	},
	{
		.pin = MUX_PIN(IMX6SDL, GPIO04),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO1_IO04 */
		.pad = PAD_USDHC_GPIO
	},
	{
		.pin = MUX_PIN(IMX6SDL, KEY_ROW1),
		.mux = IOMUX_CONFIG_ALT6,	/* SD2_VSELECT */
		.pad = PAD_USDHC_VSELECT
	},

	/* USB */
	{
		.pin = MUX_PIN(IMX6SDL, GPIO01),
		.mux = IOMUX_CONFIG_ALT3,	/* USB_OTG_ID */
		.pad = 0
	},
#endif
#if (EVBARM_BOARDTYPE == hummingboard) || \
    (EVBARM_BOARDTYPE == hummingboard_edge)
	{
		.pin = MUX_PIN(IMX6SDL, GPIO05),
		.mux = IOMUX_CONFIG_ALT3,	/* CCM_CLKO1 */
		.pad = PAD_USB
	},
#endif
#if (EVBARM_BOARDTYPE == cubox_i) || \
    (EVBARM_BOARDTYPE == hummingboard) || \
    (EVBARM_BOARDTYPE == hummingboard_edge)
	{
		.pin = MUX_PIN(IMX6SDL, EIM_DATA22),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO3_IO22 */
		.pad = PAD_USB
	},
	{
		.pin = MUX_PIN(IMX6SDL, GPIO00),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO1_IO00 */
		.pad = PAD_USB
	},
#endif
	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};

/* iMX6 Dual/Quad */
static const struct iomux_conf iomux_data_6dq[] = {
	/* Boundarydevices Nitrogen6 */
#if (EVBARM_BOARDTYPE == nitrogen6x) || \
    (EVBARM_BOARDTYPE == nitrogen6max)
	/* UART1 RX */
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA6),
		.mux = IOMUX_CONFIG_ALT1,	/* UART1_RX_DATA */
		.pad = PAD_UART
	},
	/* UART1 TX */
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA7),
		.mux = IOMUX_CONFIG_ALT1,	/* UART1_TX_DATA */
		.pad = PAD_UART
	},

	/* USB H1 */
	{
		.pin = MUX_PIN(IMX6DQ, EIM_DATA30),
		.mux = IOMUX_CONFIG_ALT6,	/* USB_H1_OC */
		.pad = PAD_WEAK_PULLUP
	},
	{
		.pin = MUX_PIN(IMX6DQ, GPIO17),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO7_IO12 */
		.pad = PAD_OUTPUT_40OHM
	},
	/* USB OTG */
	{
		.pin = MUX_PIN(IMX6DQ, GPIO01),
		.mux = IOMUX_CONFIG_ALT3,	/* USB_OTG_ID */
		.pad = PAD_WEAK_PULLUP
	},
	{
		.pin = MUX_PIN(IMX6DQ, KEY_COL4),
		.mux = IOMUX_CONFIG_ALT2,	/* USB_OTG_OC */
		.pad = PAD_WEAK_PULLUP
	},
	{
		.pin = MUX_PIN(IMX6DQ, EIM_DATA22),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO3_IO22 */
		.pad = PAD_OUTPUT_40OHM
	},

	/* USDHC2 (TiWi) */
	{
		.pin = MUX_PIN(IMX6DQ, SD2_CLK),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_CLK */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD2_CMD),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_CMD */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD2_DATA0),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA0 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD2_DATA1),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA1 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD2_DATA2),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA2 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD2_DATA3),
		.mux = IOMUX_CONFIG_ALT0,	/* SD2_DATA3 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, NAND_CS0_B),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO6_IO11 */
		.pad = PAD_WEAK_PULLUP
	},
	{
		.pin = MUX_PIN(IMX6DQ, NAND_CS2_B),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO6_IO15 */
		.pad = PAD_OUTPUT_40OHM
	},
	{
		.pin = MUX_PIN(IMX6DQ, NAND_CS3_B),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO6_IO16 */
		.pad = PAD_OUTPUT_40OHM
	},

	/* USDHC3 (sdcard) */
	{
		.pin = MUX_PIN(IMX6DQ, SD3_CLK),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_CLK */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_CMD),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_CMD */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA0),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_DATA0 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA1),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_DATA1 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA2),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_DATA2 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA3),
		.mux = IOMUX_CONFIG_ALT0,	/* SD3_DATA3 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA4),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO07_IO01 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD3_DATA5),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO07_IO00 */
		.pad = PAD_WEAK_PULLUP
	},
	{
		.pin = MUX_PIN(IMX6DQ, NAND_CS1_B),
		.mux = IOMUX_CONFIG_ALT2,	/* SD3_VSELECT */
		.pad = PAD_USDHC_VSELECT
	},

	/* USDHC4 (mmc/sdcard) */
	{
		.pin = MUX_PIN(IMX6DQ, SD4_CLK),
		.mux = IOMUX_CONFIG_ALT0,	/* SD4_CLK */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_CMD),
		.mux = IOMUX_CONFIG_ALT0,	/* SD4_CMD */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA0),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA0 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA1),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA1 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA2),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA2 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA3),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA3 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA4),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA4 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA5),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA5 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA6),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA6 */
		.pad = PAD_USDHC
	},
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA7),
		.mux = IOMUX_CONFIG_ALT1,	/* SD4_DATA7 */
		.pad = PAD_USDHC
	},
	/* CD or MMC reset */
	{
		.pin = MUX_PIN(IMX6DQ, NAND_DATA06),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO02_IO06 */
		.pad = PAD_WEAK_PULLUP
	},
#endif

	/* SolidRun CuBox-I, HummingBoard */
#if (EVBARM_BOARDTYPE == cubox_i)
	{
		.pin = MUX_PIN(IMX6DQ, GPIO01),
		.mux = IOMUX_CONFIG_ALT3,	/* USB_OTG_ID */
		.pad = 0
	},
#endif
#if (EVBARM_BOARDTYPE == hummingboard) || \
    (EVBARM_BOARDTYPE == hummingboard_edge)
	{
		.pin = MUX_PIN(IMX6DQ, GPIO05),
		.mux = IOMUX_CONFIG_ALT3,	/* CCM_CLKO1 */
		.pad = PAD_USB
	},
#endif
#if (EVBARM_BOARDTYPE == hummingboard)
	/* PCIe */
	{
		.pin = MUX_PIN(IMX6DQ, EIM_AD04),
		.mux = IOMUX_CONFIG_ALT5,
		.pad = PAD_PCIE_GPIO
	},
#endif
#if (EVBARM_BOARDTYPE == hummingboard_edge)
	/* PCIe */
	{
		.pin = MUX_PIN(IMX6DQ, SD4_DATA3),
		.mux = IOMUX_CONFIG_ALT5,
		.pad = PAD_PCIE_GPIO
	},
#endif
#if (EVBARM_BOARDTYPE == cubox_i) || \
    (EVBARM_BOARDTYPE == hummingboard) || \
    (EVBARM_BOARDTYPE == hummingboard_edge)
	{
		.pin = MUX_PIN(IMX6DQ, EIM_DATA22),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO3_IO22 */
		.pad = PAD_USB
	},
	{
		.pin = MUX_PIN(IMX6DQ, GPIO00),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO1_IO00 */
		.pad = PAD_USB
	},
#endif

	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};

/* iMX6 UltraLight */
static const struct iomux_conf iomux_data_6ul[] = {
	/* Digi-Key i.MX6UL Starter Kit */
#if (EVBARM_BOARDTYPE == ccimx6ulstarter)
	{
		.pin = MUX_PIN(IMX6UL, GPIO1_IO07),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_MDC */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, GPIO1_IO06),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_MDIO */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_RX_EN),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_RX_EN */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_RX_ER),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_RX_ER */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_RX_DATA0),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_RDATA00 */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_RX_DATA1),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_RDATA01 */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_TX_EN),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_TX_EN */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_TX_DATA0),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_TDATA00 */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_TX_DATA1),
		.mux = IOMUX_CONFIG_ALT0,	/* ENET1_TDATA01 */
		.pad = PAD_ENET6UL
	},
	{
		.pin = MUX_PIN(IMX6UL, ENET1_TX_CLK),
		.mux = IOMUX_CONFIG_ALT4 |	/* ENET1_REF_CLK */
		    IOMUX_CONFIG_SION,		/* Force input ENET1_TX_CLK */
		.pad = PAD_ENET6UL_REFCLK
	},
	{
		.pin = MUX_PIN(IMX6UL, LCD_HSYNC),
		.mux = IOMUX_CONFIG_ALT5,	/* GPIO3_IO02 */
		.pad = PAD_LCD_HSYNC
	},
#endif
	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};

static const struct gpio_conf gpio_data[] = {
	/*	GPIOn, PIN,	dir,		value	*/
#if (EVBARM_BOARDTYPE == nitrogen6x)
	{	3,	22,	GPIO_DIR_OUT,	1	}, /* USB OTG */
	{	7,	0,	GPIO_DIR_IN,	0	}, /* SD3 CD */
	{	2,	6,	GPIO_DIR_IN,	0	}, /* SD4 CD */
#endif
#if (EVBARM_BOARDTYPE == nitrogen6max)
	{	3,	22,	GPIO_DIR_OUT,	1	}, /* USB OTG */
	{	7,	12,	GPIO_DIR_OUT,	1	}, /* USB HUB RESET */
	{	6,	14,	GPIO_DIR_OUT,	1	}, /* SD3 VSELECT */
	{	7,	0,	GPIO_DIR_IN,	0	}, /* SD3 CD */
	{	2,	6,	GPIO_DIR_IN,	0	}, /* SD4 CD */
#endif
#if (EVBARM_BOARDTYPE == cubox_i)
	{	4,	29,	GPIO_DIR_OUT,	1	}, /* FRONT LED? */
	{	3,	22,	GPIO_DIR_OUT,	1	}, /* USB OTG */
	{	1,	0,	GPIO_DIR_OUT,	1	}, /* USB H1 */
	{	1,	4,	GPIO_DIR_IN,	0	}, /* USDHC2 */
#endif
#if (EVBARM_BOARDTYPE == hummingboard)
	{	3,	22,	GPIO_DIR_OUT,	1	}, /* USB OTG */
	{	1,	0,	GPIO_DIR_OUT,	1	}, /* USB H1 */
	{	1,	4,	GPIO_DIR_IN,	0	}, /* USDHC2 */
	{	3,	4,	GPIO_DIR_OUT,	0	}, /* PCIe */
#endif
#if (EVBARM_BOARDTYPE == hummingboard_edge)
	{	3,	22,	GPIO_DIR_OUT,	1	}, /* USB OTG */
	{	1,	0,	GPIO_DIR_OUT,	1	}, /* USB H1 */
	{	1,	4,	GPIO_DIR_IN,	0	}, /* USDHC2 */
	{	2,	11,	GPIO_DIR_OUT,	0	}, /* PCIe */
#endif
#if (EVBARM_BOARDTYPE == ccimx6ulstarter)
	{	3,	2,	GPIO_DIR_OUT,	1	}, /* ENET1 phy */
#endif

	/* end of table */
	{	0,	0,	0,		0	},
};


#define AIPS1_ADDR(addr)	\
	((volatile uint32_t *)(KERNEL_IO_IOREG_VBASE + (addr)))
#define IOMUX_READ(reg)	\
	(*AIPS1_ADDR(AIPS1_IOMUXC_BASE + (reg)))
#define IOMUX_WRITE(reg, val)	\
	(*AIPS1_ADDR(AIPS1_IOMUXC_BASE + (reg)) = (val))
#define GPIO_ADDR(group, reg)	\
	(AIPS1_GPIO1_BASE + ((group) - 1) * 0x4000 + (reg))

static void
nitrogen6_mux_config(const struct iomux_conf *conflist)
{
	int i;
	uint32_t reg;

	for (i = 0; conflist[i].pin != IOMUX_CONF_EOT; i++) {
		reg = IOMUX_PIN_TO_PAD_ADDRESS(conflist[i].pin);
		if (reg != IOMUX_PAD_NONE) {
#ifdef IOMUX_DEBUG
			printf("IOMUX PAD[%08x] = %08x -> %08x\n", reg, IOMUX_READ(reg), conflist[i].pad);
#endif
			IOMUX_WRITE(reg, conflist[i].pad);
		}

		reg = IOMUX_PIN_TO_MUX_ADDRESS(conflist[i].pin);
		if (reg != IOMUX_MUX_NONE) {
#ifdef IOMUX_DEBUG
			printf("IOMUX MUX[%08x] = %08x -> %08x\n", reg, IOMUX_READ(reg), conflist[i].mux);
#endif
			IOMUX_WRITE(reg, conflist[i].mux);
		}
	}
}

static void
nitrogen6_gpio_config(const struct gpio_conf *conflist)
{
	const struct gpio_conf *c;

	for (c = conflist; c->group != 0; c++) {
		if (c->dir == GPIO_DIR_IN) {
			*AIPS1_ADDR(GPIO_ADDR(c->group, GPIO_DIR)) &=
			    ~(1 << c->pin);
		} else if (c->dir == GPIO_DIR_OUT) {
			*AIPS1_ADDR(GPIO_ADDR(c->group, GPIO_DIR)) |=
			    (1 << c->pin);

			if (c->value == 0)
				*AIPS1_ADDR(GPIO_ADDR(c->group, GPIO_DR)) &=
				    ~(1 << c->pin);
			else if (c->value == 1)
				*AIPS1_ADDR(GPIO_ADDR(c->group, GPIO_DR)) |=
				    (1 << c->pin);
		}
	}
}

void
nitrogen6_setup_iomux(void)
{
	switch (IMX6_CHIPID_MAJOR(imx6_chip_id())) {
	case CHIPID_MAJOR_IMX6SL:
		nitrogen6_mux_config(iomux_data_6sl);
		break;
	case CHIPID_MAJOR_IMX6DL:
	case CHIPID_MAJOR_IMX6SOLO:
		nitrogen6_mux_config(iomux_data_6sdl);
		break;
	case CHIPID_MAJOR_IMX6Q:
		nitrogen6_mux_config(iomux_data_6dq);
		break;
	case CHIPID_MAJOR_IMX6UL:
		nitrogen6_mux_config(iomux_data_6ul);
		break;
	default:
		break;
	}
	nitrogen6_gpio_config(gpio_data);
}

void
nitrogen6_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	(void)&dict;

	imx6_device_register(self, aux);

	if (device_is_a(self, "sdhc") &&
	    device_is_a(device_parent(self), "axi")) {

#if (EVBARM_BOARDTYPE == nitrogen6x)
		prop_dictionary_set_cstring(dict, "usdhc3-cd-gpio", "!7,0");
		prop_dictionary_set_cstring(dict, "usdhc4-cd-gpio", "!2,6");
#endif
#if (EVBARM_BOARDTYPE == nitrogen6max)
		prop_dictionary_set_cstring(dict, "usdhc3-cd-gpio", "!7,0");
#endif
#if (EVBARM_BOARDTYPE == cubox_i)
		prop_dictionary_set_cstring(dict, "usdhc2-cd-gpio", "!1,4");
#endif
#if (EVBARM_BOARDTYPE == hummingboard)
		prop_dictionary_set_cstring(dict, "usdhc2-cd-gpio", "!1,4");
#endif
#if (EVBARM_BOARDTYPE == hummingboard_edge)
		prop_dictionary_set_cstring(dict, "usdhc2-cd-gpio", "!1,4");
#endif
	}
	if (device_is_a(self, "imxpcie") &&
	    device_is_a(device_parent(self), "axi")) {
#if (EVBARM_BOARDTYPE == nitrogen6max)
		prop_dictionary_set_cstring(dict, "imx6pcie-reset-gpio", "!6,31");
#endif
#if (EVBARM_BOARDTYPE == hummingboard)
		prop_dictionary_set_cstring(dict, "imx6pcie-reset-gpio", "!3,4");
#endif
#if (EVBARM_BOARDTYPE == hummingboard_edge)
		prop_dictionary_set_cstring(dict, "imx6pcie-reset-gpio", "!2,11");
#endif
	}
}
