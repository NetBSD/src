/*	$NetBSD: imx7_ioconfig.c,v 1.1.18.2 2017/12/03 11:36:05 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx7_ioconfig.c,v 1.1.18.2 2017/12/03 11:36:05 jdolecek Exp $");

#include "opt_evbarm_boardtype.h"
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7var.h>
#include <arm/imx/imx7_iomuxreg.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>
#include <evbarm/imx7/platform.h>

void imx7_setup_iomux(void);
void imx7_setup_gpio(void);
void imx7board_device_register(device_t, void *);

/*
 * define adhoc boardtype-id for board specific initialization. (#if ... #endif)
 * EVBARM_BOARDTYPE is defined in kernel config.
 */
#define	armadillo_iot_g3	1
#define	new_board_you_got	999


/* often-used mux/pads */
#define ALT0		IOMUX_CONFIG_ALT0
#define ALT1		IOMUX_CONFIG_ALT1
#define ALT2		IOMUX_CONFIG_ALT2
#define ALT3		IOMUX_CONFIG_ALT3
#define ALT4		IOMUX_CONFIG_ALT4
#define ALT5		IOMUX_CONFIG_ALT5
#define ALT6		IOMUX_CONFIG_ALT6
#define ALT7		IOMUX_CONFIG_ALT7
#define DAISY0		INPUT_DAISY_0
#define DAISY1		INPUT_DAISY_1
#define DAISY2		INPUT_DAISY_2
#define DAISY3		INPUT_DAISY_3
#define DAISY4		INPUT_DAISY_4
#define DAISY5		INPUT_DAISY_5
#define DAISY6		INPUT_DAISY_6
#define DAISY7		INPUT_DAISY_7
#define PAD_USDHC	(PAD_CTL_PS_47K_PU		\
			|PAD_CTL_PULL_ENABLE		\
			|PAD_CTL_HYSTERESIS_ENABLE	\
			|PAD_CTL_SRE_FAST		\
			|PAD_CTL_DSE_X2)	/* 0x59 */
#define PAD_USDHC_CLK	(PAD_CTL_PS_100K_PD		\
			|PAD_CTL_PULL_ENABLE		\
			|PAD_CTL_HYSTERESIS_ENABLE	\
			|PAD_CTL_SRE_FAST		\
			|PAD_CTL_DSE_X2)	/* 0x19 */
#define PAD_USDHC2	(PAD_CTL_PS_100K_PD		\
			|PAD_CTL_PULL_DISABLE		\
			|PAD_CTL_HYSTERESIS_DISABLE	\
			|PAD_CTL_SRE_SLOW		\
			|PAD_CTL_DSE_X4)	/* 0x06 */
#define PAD_ENET_X1	(PAD_CTL_PS_100K_PD		\
			|PAD_CTL_PULL_DISABLE		\
			|PAD_CTL_HYSTERESIS_DISABLE	\
			|PAD_CTL_SRE_FAST		\
			|PAD_CTL_DSE_X1)	/* 0x00 */
#define PAD_ENET_X2	(PAD_CTL_PS_100K_PD		\
			|PAD_CTL_PULL_DISABLE		\
			|PAD_CTL_HYSTERESIS_DISABLE	\
			|PAD_CTL_SRE_FAST		\
			|PAD_CTL_DSE_X2)	/* 0x01 */
#define PAD_ENET_X6	(PAD_CTL_PS_100K_PD		\
			|PAD_CTL_PULL_DISABLE		\
			|PAD_CTL_HYSTERESIS_DISABLE	\
			|PAD_CTL_SRE_FAST		\
			|PAD_CTL_DSE_X6)	/* 0x03 */



static const struct iomux_conf iomux_data[] = {
#if (EVBARM_BOARDTYPE == armadillo_iot_g3)
	/* ENET2 */
	IOMUXCONF_MUX_PAD(GPIO1_IO15,		ALT2, PAD_ENET_X6),
	IOMUXCONF_MUX_PAD(GPIO1_IO14,		ALT2, PAD_ENET_X6),
	IOMUXCONF_INPUT(ENET2_MDIO, 0),

	IOMUXCONF_MUX_PAD(EPDC_GDSP,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_GDRL,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDCE2,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDCE3,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_GDCLK,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_GDOE,		ALT2, PAD_ENET_X2),

	IOMUXCONF_MUX_PAD(EPDC_SDCE1,		ALT2, PAD_ENET_X2),
	IOMUXCONF_INPUT(ENET2_RX_CLK, 0),
	IOMUXCONF_MUX_PAD(EPDC_SDCE0,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDCLK,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDLE,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDOE,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_SDSHR,		ALT2, PAD_ENET_X2),
	IOMUXCONF_MUX_PAD(EPDC_BDR1,		ALT5, PAD_ENET_X1),


	/* uSDHC1 */
	IOMUXCONF_MUX_PAD(SD1_CMD,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD1_CLK,		ALT0, PAD_USDHC_CLK),
	IOMUXCONF_MUX_PAD(SD1_DATA0,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD1_DATA1,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD1_DATA2,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD1_DATA3,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX(    SD1_CD_B,		ALT5),
	IOMUXCONF_MUX(    SD1_RESET_B,		ALT5),

	/* uSDHC2 */
	IOMUXCONF_MUX_PAD(SD2_CMD,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_CLK,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_DATA0,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_DATA1,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_DATA2,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_DATA3,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_DATA3,		ALT0, PAD_USDHC2),
	IOMUXCONF_MUX_PAD(SD2_WP,		ALT5, PAD_USDHC),

	/* uSDHC3 */
	IOMUXCONF_MUX_PAD(SD3_CMD,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_CLK,		ALT0, PAD_USDHC_CLK),
	IOMUXCONF_MUX_PAD(SD3_DATA0,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA1,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA2,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA3,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA4,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA5,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA6,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_DATA7,		ALT0, PAD_USDHC),
	IOMUXCONF_MUX_PAD(SD3_RESET_B,		ALT0, PAD_USDHC),

	/* USBHUB */
	IOMUXCONF_MUX_PAD(EPDC_DATA13,		ALT5, 0),
	IOMUXCONF_MUX_PAD(EPDC_DATA14,		ALT5,
	    PAD_CTL_PS_100K_PD | PAD_CTL_PULL_DISABLE |
	    PAD_CTL_HYSTERESIS_DISABLE | PAD_CTL_SRE_SLOW | PAD_CTL_DSE_X1),
	IOMUXCONF_MUX_PAD(EPDC_DATA15,		ALT5, 0),
#endif

	/* end of table */
	{ .muxaddr = 0, .padaddr = 0, .inputaddr = 0 }
};


#define AIPS_WRITE(addr, val)						\
	(*(volatile uint32_t *)(KERNEL_IO_IOREG_VBASE + (addr)) = (val))
#define AIPS_READ(addr)							\
	(*(volatile uint32_t *)(KERNEL_IO_IOREG_VBASE + (addr)))
#define IOMUX_WRITE(regaddr, val)					\
	AIPS_WRITE((regaddr) - IMX7_AIPS_BASE, val)
#define IOMUX_READ(regaddr)						\
	AIPS_READ((regaddr) - IMX7_AIPS_BASE)

/* GPIO manipulation */
#define GPIO_ADDR(unit)							\
	(AIPS1_GPIO1_BASE + 0x10000 * ((unit) - 1))
#define GPIO_SETDIR(unit, pin, dir)					\
	AIPS_WRITE(GPIO_ADDR(unit) + GPIO_DIR,				\
	    (AIPS_READ(GPIO_ADDR(unit) + GPIO_DIR) & ~(1 << (pin))) |	\
	    ((dir) << (pin)))
#define GPIO_WRITE(unit, pin, data)					\
	AIPS_WRITE(GPIO_ADDR(unit) + GPIO_DR,				\
	    (AIPS_READ(GPIO_ADDR(unit) + GPIO_DR) & ~(1 << (pin))) |	\
	    ((data) << (pin)))
/* GPIO set dir & write data */
#define GPIO_DIROUT_WRITE(unit, pin, data)				\
	do {								\
		GPIO_SETDIR(unit, pin, GPIO_DIR_OUT);			\
		GPIO_WRITE(unit, pin, data);				\
	} while (0 /* CONSTCOND */)

void
imx7_setup_iomux(void)
{
	int i;
	uint32_t reg, v;

	for (i = 0; (iomux_data[i].muxaddr != 0) &&
	    (iomux_data[i].padaddr != 0) && (iomux_data[i].inputaddr != 0);
	    i++) {
		if ((reg = iomux_data[i].muxaddr) != 0)
			IOMUX_WRITE(reg, iomux_data[i].mux);
		if ((reg = iomux_data[i].padaddr) != 0)
			IOMUX_WRITE(reg, iomux_data[i].pad);
		if ((reg = iomux_data[i].inputaddr) != 0)
			IOMUX_WRITE(reg, iomux_data[i].input);
	}

	/*
	 * IOMUX GPR settings
	 */

	/* ENET */
	v = IOMUX_READ(IOMUXC_GPR_GPR1);
	v &= ~IOMUXC_GPR_GPR1_ENET2_CLK_DIR;
	v &= ~IOMUXC_GPR_GPR1_ENET1_CLK_DIR;
	v &= ~IOMUXC_GPR_GPR1_ENET2_TX_CLK_SEL;
	v &= ~IOMUXC_GPR_GPR1_ENET1_TX_CLK_SEL;
	IOMUX_WRITE(IOMUXC_GPR_GPR1, v);
}

void
imx7_setup_gpio(void)
{
#if (EVBARM_BOARDTYPE == armadillo_iot_g3)
	/* USB OTG1 VBUS */
	GPIO_DIROUT_WRITE(4, 15, 1);

	/* UHUB USB3503 */
	GPIO_DIROUT_WRITE(2, 14, 0);
	GPIO_DIROUT_WRITE(2, 15, 1);
	GPIO_DIROUT_WRITE(2, 13, 0);
#endif
}

void
imx7board_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	imx7_device_register(self, aux);

	if (device_is_a(self, "sdhc") &&
	    device_is_a(device_parent(self), "axi")) {
#if (EVBARM_BOARDTYPE == armadillo_iot_g3)
		prop_dictionary_set_cstring(dict, "usdhc1-cd-gpio", "!5,0");
		prop_dictionary_set_cstring(dict, "usdhc1-wp-gpio", "!5,1");
#endif
	}

	if (device_is_a(self, "enet") &&
	    device_is_a(device_parent(self), "axi")) {
		/*
		 *  OCOTP[MAC_ADDR0]: 0123
		 *  OCOTP[MAC_ADDR1]: 4567
		 *  OCOTP[MAC_ADDR2]: 89ab
		 *
		 *  default layout, ENET1: 6:7:0:1:2:3
		 *                  ENET2: 8:9:a:b:4:5
		 */
#if (EVBARM_BOARDTYPE == armadillo_iot_g3)
		/*
		 * XXX: armadillo_iot_g3 has illegal layout of macaddr in OCOTP
		 */
		prop_dictionary_set_cstring(dict, "enet2-ocotp-mac", "0189ab");
#endif
	}

	if (device_is_a(self, "imxusbc") &&
	    device_is_a(device_parent(self), "axi")) {
#if (EVBARM_BOARDTYPE == armadillo_iot_g3)
		prop_dictionary_set_cstring(dict, "otg1-iftype", "UTMI_WIDE");
#endif
	}
}
