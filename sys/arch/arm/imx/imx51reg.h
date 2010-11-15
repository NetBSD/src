/* $NetBSD: imx51reg.h,v 1.1.2.2 2010/11/15 14:38:22 uebayasi Exp $ */
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

#define	BOOTROM_BASE	0x00000000
#define	BOOTROM_SIZE	0x9000

#define	SCCRAM_BASE	0x1ffe0000
#define	SCCRAM_SIZE	0x20000

#define	GPUMEM_BASE	0x20000000
#define	GPUMEM_SIZE	0x20000

#define	GPU_BASE	0x30000000
#define	GPU_SIZE	0x10000000

/* LCD controller */
#define	IPUEX_BASE	0x40000000
#define	IPUEX_SIZE	0x20000000

#define	DEBUGROM_BASE	0x60000000
#define	DEBUGROM_SIZE	0x1000

#define	ESDHC1_BASE	0x70004000
#define	ESDHC2_BASE	0x70008000
#define	ESDHC3_BASE	0x70020000
#define	ESDHC4_BASE	0x70024000
#define	ESDHC_SIZE	0x4000

#define	UART1_BASE	0x73fbc000
#define	UART2_BASE	0x73fc0000
#define	UART3_BASE	0x7000c000
/* register definitions in imxuartreg.h */

#define	ECSPI1_BASE	0x70010000
#define	ECSPI2_BASE	0x83fac000
#define	ECSPI_SIZE	0x4000

#define	SSI1_BASE	0x83fcc000
#define	SSI2_BASE	0x70014000
#define	SSI3_BASE	0x83fe8000
/* register definitions in imxssireg.h */

#define	SPDIF_BASE	0x70028000
#define	SPDIF_SIZE	0x4000

#define	PATA_UDMA_BASE	0x70030000
#define	PATA_UDMA_SIZE	0x4000
#define	PATA_PIO_BASE	0x83fe0000
#define	PATA_PIO_SIZE	0x4000

#define	SLM_BASE	0x70034000
#define	SLM_SIZE	0x4000

#define	HSI2C_BASE	0x70038000
#define	HSI2C_SIZE	0x4000

#define	SPBA_BASE	0x7003c000
#define	SPBA_SIZE	0x4000

#define	USBOH3_BASE	0x73f80000
#define	USBOH3_PL301_BASE	0x73fc4000
#define	USB_OTG_BASE	0x43F88000
#define	USB_EHCI1_BASE	0x43F88200
#define	USB_EHCI2_BASE	0x43F88400
#define	USB_EHCI_SIZE	0x0200
#define	USB_CONTROL	0x43F88600


/* GPIO module */

#define	GPIO_BASE(n)	(0x73f84000 + 0x4000 * ((n)-1))

#define	GPIO1_BASE	GPIO_BASE(1)
#define	GPIO2_BASE	GPIO_BASE(2)
#define	GPIO3_BASE	GPIO_BASE(3)
#define	GPIO4_BASE	GPIO_BASE(4)

#define	GPIO_NPINS		32
#define	GPIO_NGROUPS		3

#define	KPP_BASE	0x73f94000
/* register definitions in imxkppreg.h */

#define	WDOG1_BASE	0x73f98000
#define	WDOG2_BASE	0x73f9c000
#define	WDOG_SIZE	0x4000

#define	GPT_BASE	0x73fa0000
#define	GPT_SIZE	0x4000

#define	SRTC_BASE	0x73fa4000
#define	SRTC_SIZE	0x4000

/* IO multiplexor */
#define	IOMUXC_BASE	0x73fa8000
#define	IOMUXC_SIZE	0x4000

#define	IOMUXC_MUX_CTL		0x001c		/* multiprex control */
#define	 IOMUX_CONFIG_ALT0	(0)
#define	 IOMUX_CONFIG_ALT1	(1)
#define	 IOMUX_CONFIG_ALT2	(2)
#define	 IOMUX_CONFIG_ALT3	(3)
#define	 IOMUX_CONFIG_ALT4	(4)
#define	 IOMUX_CONFIG_ALT5	(5)
#define	 IOMUX_CONFIG_ALT6	(6)
#define	 IOMUX_CONFIG_ALT7	(7)
#define	IOMUXC_PAD_CTL		0x03f0		/* pad control */
#define	 PAD_CTL_HYS_NONE	(0x0 << 8)
#define	 PAD_CTL_HYS_ENABLE	(0x1 << 8)
#define	 PAD_CTL_PKE_NONE	(0x0 << 7)
#define	 PAD_CTL_PKE_ENABLE	(0x1 << 7)
#define	 PAD_CTL_PUE_KEEPER	(0x0 << 6)
#define	 PAD_CTL_PUE_PULL	(0x1 << 6)
#define	 PAD_CTL_PUS_100K_PD	(0x0 << 4)
#define	 PAD_CTL_PUS_47K_PU	(0x1 << 4)
#define	 PAD_CTL_PUS_100K_PU	(0x2 << 4)
#define	 PAD_CTL_PUS_22K_PU	(0x3 << 4)
#define	 PAD_CTL_ODE_CMOS	(0x0 << 3)
#define	 PAD_CTL_ODE_OpenDrain	(0x1 << 3)
#define	 PAD_CTL_DSE_LOW	(0x0 << 1)
#define	 PAD_CTL_DSE_MID	(0x1 << 1)
#define	 PAD_CTL_DSE_HIGH	(0x2 << 1)
#define	 PAD_CTL_DSE_MAX	(0x3 << 1)
#define	 PAD_CTL_SRE_SLOW	(0x0 << 0)
#define	 PAD_CTL_SRE_FAST	(0x1 << 0)
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
#define	EPIT1_BASE	0x73FAC000
#define	EPIT2_BASE	0x73FB0000
/* register definitions in imxepitreg.h */

#define	PWM1_BASE	0x73fb4000
#define	PWM2_BASE	0x73fb8000
#define	PWM_SIZE	0x4000

#define	SRC_BASE	0x73fd0000
#define	SRC_SIZE	0x4000

#define	CCM_BASE	0x73fd4000
#define	CCM_SIZE	0x4000

#define	GPC_BASE	0x73fd8000
#define	GPC_SIZE	0x4000

#define	DPLLIP1_BASE	0x83f80000
#define	DPLLIP2_BASE	0x83f84000
#define	DPLLIP3_BASE	0x83f88000
#define	DPLLIP_SIZE	0x4000

#define	AHBMAX_BASE	0x83f94000
#define	AHBMAX_SIZE	0x4000

#define	IIM_BASE	0x83f98000
#define	IIM_SIZE	0x4000

#define	CSU_BASE	0x83f9c000
#define	CSU_SIZE	0x4000

#define	OWIRE_BASE	0x83fa4000	/* 1-wire */
#define	OWIRE_SIZE	0x4000

#define	FIRI_BASE	0x83fa8000
#define	FIRI_SIZE	0x4000


#define	SDMA_BASE	0x83fb0000
#define	SDMA_SIZE	0x4000
/* see imxsdmareg.h for register definitions */

#define	SCC_BASE	0x83fb4000
#define	SCC_SIZE	0x4000

#define	ROMCP_BASE	0x83fb8000
#define	ROMCP_SIZE	0x4000

#define	RTIC_BASE	0x83fbc000
#define	RTIC_SIZE	0x4000

#define	CSPI_BASE	0x83fc0000
#define	CSPI_SIZE	0x4000

#define	I2C1_BASE	0x83fc8000
#define	I2C2_BASE	0x83fc4000
/* register definitions in imxi2creg.h */

#define	AUDMUX_BASE	0x83fd0000
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

#define	EMI_BASE	0x83fd8000
#define	EMI_SIZE	0x4000

#define	SIM_BASE	0x83fe4000
#define	SIM_SIZE	0x4000

#define	FEC_BASE	0x83fec000
#define	FEC_SIZE	0x4000
#define	TVE_BASE	0x83ff0000
#define	TVE_SIZE	0x4000
#define	VPU_BASE	0x83ff4000
#define	VPU_SIZE	0x4000
#define	SAHARA_BASE	0x83ff8000
#define	SAHARA_SIZE	0x4000

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

#define	TZIC_BASE		0xe0000000
/* register definitions in imx51_tzicreg.h */

#endif /* _ARM_IMX_IMX51REG_H_ */
