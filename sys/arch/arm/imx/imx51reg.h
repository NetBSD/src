/* $NetBSD: imx51reg.h,v 1.4.12.1 2014/08/10 06:53:51 tls Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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

#ifndef _ARM_IMX_IMX51REG_H_
#define	_ARM_IMX_IMX51REG_H_

#ifdef IMX50
#define	TZIC_BASE	0x0fffc000
#define	APB_BASE	0x40000000
#define	AIPSTZ1_BASE	0x50000000
#define	AIPSTZ2_BASE	0x60000000
#define	CSD0DDR_BASE	0x70000000
#else
#define	TZIC_BASE	0xe0000000
#define	AIPSTZ1_BASE	0x70000000
#define	AIPSTZ2_BASE	0x80000000
#define	CSD0DDR_BASE	0x90000000
#define	CSD1DDR_BASE	0xa0000000
#define	CSDDDR_SIZE	0x10000000	/* 256MiB */
#define	CS0_BASE	0xb0000000
#define	CS0_SIZE	0x08000000	/* 128MiB */
#define	CS1_BASE	0xb8000000
#define	CS1_SIZE	0x08000000	/* 128MiB */
#define	CS2_BASE	0xc0000000
#define	CS2_SIZE	0x08000000	/* 128MiB */
#define	CS3_BASE	0xc8000000
#define	CS3_SIZE	0x04000000	/* 64MiB */
#define	CS4_BASE	0xcc000000
#define	CS4_SIZE	0x02000000	/* 32MiB */
#define	CS5_BASE	0xcefe0000
#define	CS5_SIZE	0x00010000	/* 32MiB */
#define	NAND_FLASH_BASE	0xcfff0000	/* internal buffer */
#define	NAND_FLASH_SIZE	0x00010000

#define	GPU2D_BASE	0xd0000000
#define	GPU2D_SIZE	0x10000000
#endif

#define	BOOTROM_BASE	0x00000000
#define	BOOTROM_SIZE	0x9000

#define	SCCRAM_BASE	0x1ffe0000
#define	SCCRAM_SIZE	0x20000

#define	GPUMEM_BASE	0x20000000
#define	GPUMEM_SIZE	0x20000

#define	GPU_BASE	0x30000000
#define	GPU_SIZE	0x10000000

#ifdef IMX50
#define EPDC_BASE	(APB_BASE + 0x01010000)
#define EPDC_SIZE	0x2000
#endif

/* Image Prossasing Unit */
#define	IPU_BASE	0x40000000
#define	IPU_CM_BASE	(IPU_BASE + 0x1e000000)
#define	IPU_CM_SIZE	0x8000
#define	IPU_IDMAC_BASE	(IPU_BASE + 0x1e008000)
#define	IPU_IDMAC_SIZE	0x8000
#define	IPU_DP_BASE	(IPU_BASE + 0x1e018000)
#define	IPU_DP_SIZE	0x8000
#define	IPU_IC_BASE	(IPU_BASE + 0x1e020000)
#define	IPU_IC_SIZE	0x8000
#define	IPU_IRT_BASE	(IPU_BASE + 0x1e028000)
#define	IPU_IRT_SIZE	0x8000
#define	IPU_CSI0_BASE	(IPU_BASE + 0x1e030000)
#define	IPU_CSI0_SIZE	0x8000
#define	IPU_CSI1_BASE	(IPU_BASE + 0x1e038000)
#define	IPU_CSI1_SIZE	0x8000
#define	IPU_DI0_BASE	(IPU_BASE + 0x1e040000)
#define	IPU_DI0_SIZE	0x8000
#define	IPU_DI1_BASE	(IPU_BASE + 0x1e048000)
#define	IPU_DI1_SIZE	0x8000
#define	IPU_SMFC_BASE	(IPU_BASE + 0x1e050000)
#define	IPU_SMFC_SIZE	0x8000
#define	IPU_DC_BASE	(IPU_BASE + 0x1e058000)
#define	IPU_DC_SIZE	0x8000
#define	IPU_DMFC_BASE	(IPU_BASE + 0x1e060000)
#define	IPU_DMFC_SIZE	0x8000
#define	IPU_VDI_BASE	(IPU_BASE + 0x1e068000)
#define	IPU_VDI_SIZE	0x8000
#define	IPU_CPMEM_BASE	(IPU_BASE + 0x1f000000)
#define	IPU_CPMEM_SIZE	0x20000
#define	IPU_LUT_BASE	(IPU_BASE + 0x1f020000)
#define	IPU_LUT_SIZE	0x20000
#define	IPU_SRM_BASE	(IPU_BASE + 0x1f040000)
#define	IPU_SRM_SIZE	0x20000
#define	IPU_TPM_BASE	(IPU_BASE + 0x1f060000)
#define	IPU_TPM_SIZE	0x20000
#define	IPU_DCTMPL_BASE	(IPU_BASE + 0x1f080000)
#define	IPU_DCTMPL_SIZE	0x20000

#define	DEBUGROM_BASE	0x60000000
#define	DEBUGROM_SIZE	0x1000

#define	ESDHC1_BASE	(AIPSTZ1_BASE + 0x00004000)
#define	ESDHC2_BASE	(AIPSTZ1_BASE + 0x00008000)
#define	ESDHC3_BASE	(AIPSTZ1_BASE + 0x00020000)
#define	ESDHC4_BASE	(AIPSTZ1_BASE + 0x00024000)
#define	ESDHC_SIZE	0x100

#define PWM1_BASE	(AIPSTZ1_BASE + 0x03fb4000)
#define PWM2_BASE	(AIPSTZ1_BASE + 0x03fb8000)

#define	UART1_BASE	(AIPSTZ1_BASE + 0x03fbc000)
#define	UART2_BASE	(AIPSTZ1_BASE + 0x03fc0000)
#define	UART3_BASE	(AIPSTZ1_BASE + 0x0000c000)
/* register definitions in imxuartreg.h */

#define	CCMC_BASE	(AIPSTZ1_BASE + 0x03fd4000)

#define	ECSPI1_BASE	(AIPSTZ1_BASE + 0x00010000)
#define	ECSPI2_BASE	(AIPSTZ2_BASE + 0x03fac000)
#define	ECSPI_SIZE	0x4000

#define	SSI1_BASE	(AIPSTZ2_BASE + 0x03fcc000)
#define	SSI2_BASE	(AIPSTZ1_BASE + 0x00014000)
#define	SSI3_BASE	(AIPSTZ2_BASE + 0x03fe8000)
/* register definitions in imxssireg.h */

#define	SPDIF_BASE	(AIPSTZ1_BASE + 0x00028000)
#define	SPDIF_SIZE	0x4000

#define	PATA_UDMA_BASE	(AIPSTZ1_BASE + 0x00030000)
#define	PATA_UDMA_SIZE	0x4000
#define	PATA_PIO_BASE	(AIPSTZ2_BASE + 0x03fe0000)
#define	PATA_PIO_SIZE	0x4000

#define	SLM_BASE	(AIPSTZ1_BASE + 0x00034000)
#define	SLM_SIZE	0x4000

#ifdef IMX50
#define	I2C3_BASE	(AIPSTZ1_BASE + 0x00038000)
#define	I2C3_SIZE	0x4000
#else
#define	HSI2C_BASE	(AIPSTZ1_BASE + 0x00038000)
#define	HSI2C_SIZE	0x4000
#endif

#define	SPBA_BASE	(AIPSTZ1_BASE + 0x0003c000)
#define	SPBA_SIZE	0x4000

#define	USBOH3_BASE	(AIPSTZ1_BASE + 0x03f80000)
#define	USBOH3_PL301_BASE	(AIPSTZ1_BASE + 0x03fc4000)
#define	USBOH3_EHCI_SIZE	0x200
#define	USBOH3_OTG	0x000
#define	USBOH3_EHCI(n)	(USBOH3_EHCI_SIZE*(n))	/* n=1,2,3 */

/* USB_CTRL register */
#define	USBOH3_USBCTRL			0x800
#define	 USBCTRL_OWIR			__BIT(31)	/* OTG Wakeup interrupt request */
#define	 USBCTRL_OSIC			__BITS(29,30)	/* OTG Serial interface configuration */
#define	 USBCTRL_OUIE			__BIT(28)	/* OTG Wake-up interrupt enable */
#define	 USBCTRL_OBPAL			__BITS(25,26)	/* OTG Bypass value */
#define	 USBCTRL_OPM			__BIT(24)	/* OTG Power Mask */
#define	 USBCTRL_ICVOL			__BIT(23)	/* Host1 IC_USB voltage status */
#define	 USBCTRL_ICTPIE			__BIT(19)	/* IC USB TP interrupt enable */
#define	 USBCTRL_UBPCKE			__BIT(18)	/* Bypass clock enable */
#define	 USBCTRL_H1TCKOEN		__BIT(17)	/* Host1 ULPO PHY clock enable */
#define	 USBCTRL_ICTPC			__BIT(16)	/* Clear IC TP interrupt flag */
#define	 USBCTRL_H1WIR			__BIT(15)	/* Host1 wakeup interrupt request */
#define	 USBCTRL_H1SIC			__BITS(13,14)	/* Host1 serial interface config */
#define	 USBCTRL_H1UIE			__BIT(12)	/* Host1 ILPI interrupt enable */
#define	 USBCTRL_H1WIE			__BIT(11)	/* Host1 wakeup interrupt enable */
#define	 USBCTRL_H1BPVAL		__BITS(9,10)	/* Host1 bypass value */
#define	 USBCTRL_H1PM			__BIT(8)	/* Host1 power mask */
#define	 USBCTRL_OHSTLL			__BIT(7)	/* OTG ULPI TLL enable */
#define	 USBCTRL_H1HSTLL		__BIT(6)	/* Host1 ULPI TLL enable */
#define	 USBCTRL_H1DISFSTTL		__BIT(4)	/* Host1 serial TLL disable */
#define	 USBCTRL_OTCKOEN		__BIT(1)	/* OTG ULPI PHY clock enable */
#define	 USBCTRL_BPE			__BIT(0)	/* Bypass enable */
#define	USBOH3_OTGMIRROR		0x804
#define	USBOH3_PHYCTRL0			0x808
#define	 PHYCTRL0_VLOAD			__BIT(31)
#define	 PHYCTRL0_VCONTROL		__BITS(27,30)
#define	 PHYCTRL0_CONF2			__BIT(26)
#define	 PHYCTRL0_CONF3			__BIT(25)
#define	 PHYCTRL0_CHGRDETEN		__BIT(24)
#define	 PHYCTRL0_CHGRDETON		__BIT(23)
#define	 PHYCTRL0_VSTATUS		__BITS(15,22)
#define	 PHYCTRL0_SUSPENDM		__BIT(12)
#define	 PHYCTRL0_RESET			__BIT(11)
#define	 PHYCTRL0_UTMI_ON_CLOCK		__BIT(10)
#define	 PHYCTRL0_OTG_OVER_CUR_POL	__BIT(9)
#define	 PHYCTRL0_OTG_OVER_CUR_DIS	__BIT(8)
#define	 PHYCTRL0_OTG_XCVR_CLK_SEL	__BIT(7)
#define	 PHYCTRL0_H1_OVER_CUR_POL	__BIT(6)
#define	 PHYCTRL0_H1_OVER_CUR_DIS	__BIT(5)
#define	 PHYCTRL0_H1_XCVR_CLK_SEL	__BIT(4)
#define	 PHYCTRL0_PWR_POL		__BIT(3)
#define	 PHYCTRL0_CHRGDET		__BIT(2)
#define	 PHYCTRL0_CHRGDET_INT_EN	__BIT(1)
#define	 PHYCTRL0_CHRGDET_INT_FLG	__BIT(0)
#define	USBOH3_PHYCTRL1			0x80c
#define	 PHYCTRL1_PLLDIVVALUE_MASK	__BITS(0,1)
#define	 PHYCTRL1_PLLDIVVALUE_19MHZ	0	/* 19.2MHz */
#define	 PHYCTRL1_PLLDIVVALUE_24MHZ	1
#define	 PHYCTRL1_PLLDIVVALUE_26MHZ	2
#define	 PHYCTRL1_PLLDIVVALUE_27MHZ	3
#define	USBOH3_USBCTRL1			0x810
#define	 USBCTRL1_UH3_EXT_CLK_EN	__BIT(27)
#define	 USBCTRL1_UH2_EXT_CLK_EN	__BIT(26)
#define	 USBCTRL1_UH1_EXT_CLK_EN	__BIT(25)
#define	 USBCTRL1_OTG_EXT_CLK_EN	__BIT(24)
#define	USBOH3_USBCTRL2			0x814
#define	USBOH3_USBCTRL3			0x818
#define	USBOH3_UH1_PHY_CTRL_0		0x81c
#define	USBOH3_UH1_PHY_CTRL_1		0x820
#define	USBOH3_USB_CLKONOFF_CTRL  	0x824
#define	 USB_CLKONOFF_CTRL_H1_AHBCLK_OFF	__BIT(18)
#define	 USB_CLKONOFF_CTRL_OTG_AHBCLK_OFF	__BIT(17)

#define	USBOH3_SIZE	0x828

/* GPIO module */

#define	GPIO_BASE(n)				      \
	(AIPSTZ1_BASE + (((n) <= 4) ?		      \
	    0x03f84000 + 0x4000 * ((n) - 1) :	      \
	    0x03fdc000 + 0x4000 * ((n) - 5)))

#define	GPIO1_BASE	GPIO_BASE(1)
#define	GPIO2_BASE	GPIO_BASE(2)
#define	GPIO3_BASE	GPIO_BASE(3)
#define	GPIO4_BASE	GPIO_BASE(4)
#define	GPIO5_BASE	GPIO_BASE(5)
#define	GPIO6_BASE	GPIO_BASE(6)

#ifdef IMX50
#define	GPIO_NGROUPS		6
#else
#define	GPIO_NGROUPS		4
#endif

#define	KPP_BASE	(AIPSTZ1_BASE + 0x03f94000)
/* register definitions in imxkppreg.h */

#define	WDOG1_BASE	(AIPSTZ1_BASE + 0x03f98000)
#define	WDOG2_BASE	(AIPSTZ1_BASE + 0x03f9c000)
#define	WDOG_SIZE	0x000a

#define	GPT_BASE	(AIPSTZ1_BASE + 0x03fa0000)
#define	GPT_SIZE	0x4000

#define	SRTC_BASE	(AIPSTZ1_BASE + 0x03fa4000)
#define	SRTC_SIZE	0x4000

/* IO multiplexor */
#define	IOMUXC_BASE	(AIPSTZ1_BASE + 0x03fa8000)
#define	IOMUXC_SIZE	0x4000

#define	IOMUXC_MUX_CTL		0x001c		/* multiprex control */
#define	 IOMUX_CONFIG_SION	__BIT(4)
#define	 IOMUX_CONFIG_ALT0	(0)
#define	 IOMUX_CONFIG_ALT1	(1)
#define	 IOMUX_CONFIG_ALT2	(2)
#define	 IOMUX_CONFIG_ALT3	(3)
#define	 IOMUX_CONFIG_ALT4	(4)
#define	 IOMUX_CONFIG_ALT5	(5)
#define	 IOMUX_CONFIG_ALT6	(6)
#define	 IOMUX_CONFIG_ALT7	(7)
#define	IOMUXC_PAD_CTL		0x03f0		/* pad control */
#define	 PAD_CTL_HVE		__BIT(13)
#define	 PAD_CTL_DDR_INPUT	__BIT(9)
#define	 PAD_CTL_HYS		__BIT(8)
#define	 PAD_CTL_PKE		__BIT(7)
#define	 PAD_CTL_PUE		__BIT(6)
#define	 PAD_CTL_PULL		(PAD_CTL_PKE|PAD_CTL_PUE)
#define	 PAD_CTL_KEEPER		(PAD_CTL_PKE|0)
#define	 PAD_CTL_PUS_MASK	__BITS(5, 4)
#define	 PAD_CTL_PUS_100K_PD	__SHIFTIN(0x0, PAD_CTL_PUS_MASK)
#define	 PAD_CTL_PUS_47K_PU	__SHIFTIN(0x1, PAD_CTL_PUS_MASK)
#define	 PAD_CTL_PUS_100K_PU	__SHIFTIN(0x2, PAD_CTL_PUS_MASK)
#define	 PAD_CTL_PUS_22K_PU	__SHIFTIN(0x3, PAD_CTL_PUS_MASK)
#define	 PAD_CTL_ODE		__BIT(3)	/* opendrain */
#define	 PAD_CTL_DSE_MASK	__BITS(2, 1)
#define	 PAD_CTL_DSE_LOW	__SHIFTIN(0x0, PAD_CTL_DSE_MASK)
#define	 PAD_CTL_DSE_MID	__SHIFTIN(0x1, PAD_CTL_DSE_MASK)
#define	 PAD_CTL_DSE_HIGH	__SHIFTIN(0x2, PAD_CTL_DSE_MASK)
#define	 PAD_CTL_DSE_MAX	__SHIFTIN(0x3, PAD_CTL_DSE_MASK)
#define	 PAD_CTL_SRE		__BIT(0)
#define	IOMUXC_INPUT_CTL	0x08c4		/* input control */
#define	 INPUT_DAISY_0		0
#define	 INPUT_DAISY_1		1
#define	 INPUT_DAISY_2		2
#define	 INPUT_DAISY_3		3
#define	 INPUT_DAISY_4		4
#define	 INPUT_DAISY_5		5
#define	 INPUT_DAISY_6		6
#define	 INPUT_DAISY_7		7

/*
 * IOMUX index
 */
#define	IOMUX_PIN_TO_MUX_ADDRESS(pin)	(((pin) >> 16) & 0xffff)
#define	IOMUX_PIN_TO_PAD_ADDRESS(pin)	(((pin) >>  0) & 0xffff)

#define	IOMUX_PIN(mux_adr, pad_adr)			\
	(((mux_adr) << 16) | (((pad_adr) << 0)))
#define	IOMUX_MUX_NONE	0xffff
#define	IOMUX_PAD_NONE	0xffff

/* EPIT */
#define	EPIT1_BASE	(AIPSTZ1_BASE + 0x03FAC000)
#define	EPIT2_BASE	(AIPSTZ1_BASE + 0x03FB0000)
/* register definitions in imxepitreg.h */

#define	PWM1_BASE	(AIPSTZ1_BASE + 0x03fb4000)
#define	PWM2_BASE	(AIPSTZ1_BASE + 0x03fb8000)
#define	PWM_SIZE	0x4000

#define	SRC_BASE	(AIPSTZ1_BASE + 0x03fd0000)
#define	SRC_SIZE	0x4000

#define	CCM_BASE	(AIPSTZ1_BASE + 0x03fd4000)
#define	CCM_SIZE	0x0088

#define	GPC_BASE	(AIPSTZ1_BASE + 0x03fd8000)
#define	GPC_SIZE	0x4000

#define	AHBMAX_BASE	(AIPSTZ2_BASE + 0x03f94000)
#define	AHBMAX_SIZE	0x4000

#define	IIM_BASE	(AIPSTZ2_BASE + 0x03f98000)
#define	IIM_SIZE	0x4000

#define	CSU_BASE	(AIPSTZ2_BASE + 0x03f9c000)
#define	CSU_SIZE	0x4000

#define	OWIRE_BASE	(AIPSTZ2_BASE + 0x03fa4000)
#define	OWIRE_SIZE	0x4000

#define	FIRI_BASE	(AIPSTZ2_BASE + 0x03fa8000)
#define	FIRI_SIZE	0x4000


#define	SDMA_BASE	(AIPSTZ2_BASE + 0x03fb0000)
#define	SDMA_SIZE	0x4000
/* see imxsdmareg.h for register definitions */

#define	SCC_BASE	(AIPSTZ2_BASE + 0x03fb4000)
#define	SCC_SIZE	0x4000

#define	ROMCP_BASE	(AIPSTZ2_BASE + 0x03fb8000)
#define	ROMCP_SIZE	0x4000

#define	RTIC_BASE	(AIPSTZ2_BASE + 0x03fbc000)
#define	RTIC_SIZE	0x4000

#define	CSPI_BASE	(AIPSTZ2_BASE + 0x03fc0000)
#define	CSPI_SIZE	0x4000

#define	I2C1_BASE	(AIPSTZ2_BASE + 0x03fc8000)
#define	I2C2_BASE	(AIPSTZ2_BASE + 0x03fc4000)
/* register definitions in imxi2creg.h */

#define	AUDMUX_BASE	(AIPSTZ2_BASE + 0x03fd0000)
#define	AUDMUX_SIZE	0x4000
#define	AUDMUX_PTCR(n)	((n - 1) * 0x8)
#define	 PTCR_TFSDIR	(1 << 31)
#define	 PTCR_TFSEL(x)	(((x) & 0x7) << 27)
#define	 PTCR_TCLKDIR	(1 << 26)
#define	 PTCR_TCSEL(x)	(((x) & 0x7) << 22)
#define	 PTCR_RFSDIR	(1 << 21)
#define	 PTCR_RFSEL(x)	(((x) & 0x7) << 17)
#define	 PTCR_RCLKDIR	(1 << 16)
#define	 PTCR_RCSEL(x)	(((x) & 0x7) << 12)
#define	 PTCR_SYN	(1 << 11)

#define	AUDMUX_PDCR(n)	((n - 1) * 0x8 + 0x4)
#define	 PDCR_RXDSEL(x)	(((x) & 0x7) << 13)
#define	 PDCR_TXRXEN	(1 << 12)
#define	 PDCR_MODE(x)	(((x) & 0x3) << 8)
#define	 PDCR_INMMASK(x)	(((x) & 0xff) << 0)
#define	AUDMUX_CNMCR	0x38

#define	EMI_BASE	(AIPSTZ2_BASE + 0x03fd8000)
#define	EMI_SIZE	0x4000

#define	SIM_BASE	(AIPSTZ2_BASE + 0x03fe4000)
#define	SIM_SIZE	0x4000

#define	FEC_BASE	(AIPSTZ2_BASE + 0x03fec000)
#define	FEC_SIZE	0x4000
#define	TVE_BASE	(AIPSTZ2_BASE + 0x03ff0000)
#define	TVE_SIZE	0x4000
#define	VPU_BASE	(AIPSTZ2_BASE + 0x03ff4000)
#define	VPU_SIZE	0x4000
#define	SAHARA_BASE	(AIPSTZ2_BASE + 0x03ff8000)
#define	SAHARA_SIZE	0x4000

#define	DPLL_BASE(n)	((AIPSTZ2_BASE + 0x03F80000 + (0x4000 * ((n)-1))))
#define	DPLL_SIZE	0x100

#endif /* _ARM_IMX_IMX51REG_H_ */
