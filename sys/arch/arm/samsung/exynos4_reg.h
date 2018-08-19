/*	$NetBSD: exynos4_reg.h,v 1.15 2018/08/19 07:27:33 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#ifndef _ARM_SAMSUNG_EXYNOS4_REG_H_
#define _ARM_SAMSUNG_EXYNOS4_REG_H_

/*
 * Physical memory layout of Exynos SoCs as per documentation
 *
 * Base Address Limit Address  Size  Description
 * 0x00000000   0x00010000    64 KB  iROM
 * 0x02000000   0x02010000    64 KB  iROM (mirror of 0x0 to 0x10000)
 * 0x02020000   0x02060000   256 KB  iRAM
 * 0x03000000   0x03020000   128 KB  Data memory or general purpose of Samsung
 *                                   Reconfigurable Processor SRP.
 * 0x03020000   0x03030000    64 KB  I-cache or general purpose of SRP.
 * 0x03030000   0x03039000    36 KB  Configuration memory (write only) of SRP
 * 0x03810000   0x03830000      ­    AudioSS's SFR region
 * 0x04000000   0x05000000    16 MB  Bank0 of Static ROM Controller (SMC)
 *                                   (16-bit only)
 * 0x05000000   0x06000000    16 MB  Bank1 of SMC
 * 0x06000000   0x07000000    16 MB  Bank2 of SMC
 * 0x07000000   0x08000000    16 MB  Bank3 of SMC
 * 0x08000000   0x0C000000    64 MB  Reserved
 * 0x0C000000   0x0CD00000       ­   Reserved
 * 0x0CE00000   0x0D000000       ­   SFR region of Nand Flash Contr. (NFCON)
 * 0x10000000   0x14000000       ­   SFR region
 * 0x40000000   0xA0000000   1.5 GB  Memory of Dynamic Memory Contr. (DMC)-0
 * 0xA0000000   0x00000000   1.5 GB  Memory of DMC-1
*/

/*
 *
 * The exynos can boot from its iROM or from an external Nand memory. Since
 * these are normally hardly used they are excluded from the normal register
 * space here.
 *
 * XXX What about the audio subsystem region. Where are the docs?
 *
 * EXYNOS_CORE_PBASE points to the main SFR region.
 *
 * Notes:
 *
 * SFR		Special Function Register
 * ISP		In-System Programming, like a JTAG
 * ACP		Accelerator Coherency Port
 * SSS		Security Sub System
 * GIC		Generic Interurrupt Controller
 * PMU		Power Management Unit
 * DMC		2D Graphics engine
 * LEFTBUS	Data bus / Peripheral bus
 * RIGHTBUS	,,
 * G3D		3D Graphics engine
 * MFC		Multi-Format Codec
 * LCD0		LCD display
 * MCT		Multi Core Timer
 * CMU		Clock Management Unit
 * TMU		Thermal Management Unit
 * PPMU		Pin Parametric Measurement Unit (?)
 * MMU		Memory Management Unit
 * MCTimer	?
 * WDT		Watch Dog Timer
 * RTC		Real Time Clock
 * KEYIF	Keypad interface
 * SECKEY	?
 * TZPC		TrustZone Protection Controller
 * UART		Universal asynchronous receiver/transmitter
 * I2C		Inter IC Connect
 * SPI		Serial Peripheral Interface Bus
 * I2S		Inter-IC Sound, Integrated Interchip Sound, or IIS
 * PCM		Pulse-code modulation, audio stream at set fixed rate
 * SPDIF	Sony/Philips Digital Interface Format
 * Slimbus	Serial Low-power Inter-chip Media Bus
 * SMMU		System mmu. No idea as how its programmed (or not)
 * PERI-L	UART, I2C, SPI, I2S, PCM, SPDIF, PWM, I2CHDMI, Slimbus
 * PERI-R	CHIPID, SYSREG, PMU/CMU/TMU Bus I/F, MCTimer, WDT, RTC, KEYIF,
 * 		SECKEY, TZPC
 */

/*
 * Note that this table is not 80-char friendly, this is done to allow more
 * elaborate comments to clarify the register offsets use
 */

/* CORE */
#define EXYNOS4_CORE_SIZE			0x04000000
#define EXYNOS4_SDRAM_PBASE			0x40000000

#define EXYNOS4_SYSREG_OFFSET			0x00010000
#define EXYNOS4_PMU_OFFSET			0x00020000	/* Power Management Unit */
#define EXYNOS4_CMU_TOP_PART_OFFSET		0x00030000	/* Clock(s) management unit */
#define   EXYNOS4_CMU_EPLL			0x0003C010	/* Audio and ext. interf. clock */
#define   EXYNOS4_CMU_VPLL			0x0003C020	/* Video core (dither?) clock */
#define EXYNOS4_CMU_CORE_ISP_PART_OFFSET	0x00040000	/* Clock(s) management unit */
#define   EXYNOS4_CMU_MPLL			0x00040008	/* MEM cntr. clock */
#define   EXYNOS4_CMU_APLL			0x00044000	/* ARM core clock */
#define EXYNOS4_MCT_OFFSET			0x00050000	/* Multi Core Timer */
#define EXYNOS4_WDT_OFFSET			0x00060000	/* Watch Dog Timer */
#define EXYNOS4_RTC_OFFSET			0x00070000	/* Real Time Clock */
#define EXYNOS4_KEYIF_OFFSET			0x000A0000	/* Keypad interface */
#define EXYNOS4_HDMI_CEC_OFFSET			0x000B0000	/* HDMI Consumer Electronic Control */
#define EXYNOS4_TMU_OFFSET			0x000C0000	/* Thermal Managment */
#define EXYNOS4_SECKEY_OFFSET			0x00100000	/* XXX unknown XXX */
#define EXYNOS4_TZPC0_OFFSET			0x00110000	/* ARM Trusted Zone Protection Controller */
#define EXYNOS4_TZPC1_OFFSET			0x00120000
#define EXYNOS4_TZPC2_OFFSET			0x00130000
#define EXYNOS4_TZPC3_OFFSET			0x00140000
#define EXYNOS4_TZPC4_OFFSET			0x00150000
#define EXYNOS4_TZPC5_OFFSET			0x00160000
#define EXYNOS4_INTCOMBINER_OFFSET		0x00440000	/* combines first 32 interrupt sources */
#define EXYNOS4_GIC_CNTR_OFFSET			0x00480000	/* generic interrupt controller offset */
#define EXYNOS4_GIC_DISTRIBUTOR_OFFSET		0x00490000
#define EXYNOS4_AP_C2C_OFFSET			0x00540000	/* Chip 2 Chip XXX doc? XXX */
#define EXYNOS4_CP_C2C_MODEM_OFFSET		0x00580000
#define EXYNOS4_DMC0_OFFSET			0x00600000	/* Dynamic Memory Controller */
#define EXYNOS4_DMC1_OFFSET			0x00610000
#define EXYNOS4_PPMU_DMC_L_OFFSET		0x006A0000	/* event counters XXX ? */
#define EXYNOS4_PPMU_DMC_R_OFFSET		0x006B0000
#define EXYNOS4_PPMU_CPU_OFFSET			0x006C0000
#define EXYNOS4_GPIO_C2C_OFFSET			0x006E0000
#define EXYNOS4_TZASC_LR_OFFSET			0x00700000	/* trust zone access control */
#define EXYNOS4_TZASC_LW_OFFSET			0x00710000
#define EXYNOS4_TZASC_RR_OFFSET			0x00720000
#define EXYNOS4_TZASC_RW_OFFSET			0x00730000
#define EXYNOS4_G2D_ACP_OFFSET			0x00800000	/* 2D graphics engine */
#define EXYNOS4_SSS_OFFSET			0x00830000	/* Security Sub System */
#define EXYNOS4_CORESIGHT_1_OFFSET		0x00880000	/* 1st region */
#define EXYNOS4_CORESIGHT_2_OFFSET		0x00890000	/* 2nd region */
#define EXYNOS4_CORESIGHT_3_OFFSET		0x008B0000	/* 3rd region */
#define EXYNOS4_SMMUG2D_ACP_OFFSET		0x00A40000	/* system mmu for 2D graphics engine */
#define EXYNOS4_SMMUSSS_OFFSET			0x00A50000	/* system mmu for SSS */
#define EXYNOS4_GPIO_RIGHT_OFFSET		0x01000000
#define EXYNOS4_GPIO_LEFT_OFFSET		0x01400000
#define EXYNOS4_FIMC0_OFFSET			0x01800000	/* image for display */
#define EXYNOS4_FIMC1_OFFSET			0x01810000
#define EXYNOS4_FIMC2_OFFSET			0x01820000
#define EXYNOS4_FIMC3_OFFSET			0x01830000
#define EXYNOS4_JPEG_OFFSET			0x01840000	/* JPEG Codec */
#define EXYNOS4_MIPI_CSI0_OFFSET		0x01880000	/* MIPI-Slim bus Interface */
#define EXYNOS4_MIPI_CSI1_OFFSET		0x01890000
#define EXYNOS4_SMMUFIMC0_OFFSET		0x01A20000	/* system mmus */
#define EXYNOS4_SMMUFIMC1_OFFSET		0x01A30000
#define EXYNOS4_SMMUFIMC2_OFFSET		0x01A40000
#define EXYNOS4_SMMUFIMC3_OFFSET		0x01A50000
#define EXYNOS4_SMMUJPEG_OFFSET			0x01A60000
#define EXYNOS4_FIMD0_OFFSET			0x01C00000	/* LCD0 */
#define EXYNOS4_MIPI_DSI0_OFFSET		0x01C80000	/* LCD0 */
#define EXYNOS4_SMMUFIMD0_OFFSET		0x01E20000	/* system mmus */
#define EXYNOS4_FIMC_ISP_OFFSET			0x02000000	/* (digital) camera video input */
#define EXYNOS4_FIMC_DRC_TOP_OFFSET		0x02010000
#define EXYNOS4_FIMC_FD_TOP_OFFSET		0x02040000
#define EXYNOS4_MPWM_ISP_OFFSET			0x02110000	/* (specialised?) PWM */
#define EXYNOS4_I2C0_ISP_OFFSET			0x02130000	/* I2C bus */
#define EXYNOS4_I2C1_ISP_OFFSET			0x02140000
#define EXYNOS4_MTCADC_ISP_OFFSET		0x02150000	/* (specialised?) AD Converter */
#define EXYNOS4_PWM_ISP_OFFSET			0x02160000	/* PWM */
#define EXYNOS4_WDT_ISP_OFFSET			0x02170000	/* Watch Dog Timer */
#define EXYNOS4_MCUCTL_ISP_OFFSET		0x02180000	/* XXX unknown XXX */
#define EXYNOS4_UART_ISP_OFFSET			0x02190000	/* uart base clock */
#define EXYNOS4_SPI0_ISP_OFFSET			0x021A0000
#define EXYNOS4_SPI1_ISP_OFFSET			0x021B0000
#define EXYNOS4_GIC_C_ISP_OFFSET		0x021E0000
#define EXYNOS4_GIC_D_ISP_OFFSET		0x021F0000
#define EXYNOS4_SYSMMU_FIMC_ISP_OFFSET		0x02260000
#define EXYNOS4_SYSMMU_FIMC_DRC_OFFSET		0x02270000
#define EXYNOS4_SYSMMU_FIMC_FD_OFFSET		0x022A0000
#define EXYNOS4_SYSMMU_ISPCPU_OFFSET		0x022B0000
#define EXYNOS4_FIMC_LITE0_OFFSET		0x02390000	/* external image input? */
#define EXYNOS4_FIMC_LITE1_OFFSET		0x023A0000
#define EXYNOS4_SYSMMU_FIMC_LITE0_OFFSET	0x023B0000
#define EXYNOS4_SYSMMU_FIMC_LITE1_OFFSET	0x023C0000
#define EXYNOS4_USBDEV0_OFFSET			0x02480000	/* XXX unknown XXX */
#define EXYNOS4_USBDEV0_1_OFFSET		0x02480000
#define EXYNOS4_USBDEV0_2_OFFSET		0x02490000
#define EXYNOS4_USBDEV0_3_OFFSET		0x024A0000
#define EXYNOS4_USBDEV0_4_OFFSET		0x024B0000
#define EXYNOS4_TSI_OFFSET			0x02500000	/* Transport Stream Interface */
#define EXYNOS4_SDMMC0_OFFSET			0x02510000	/* SD card interface */
#define EXYNOS4_SDMMC1_OFFSET			0x02520000
#define EXYNOS4_SDMMC2_OFFSET			0x02530000
#define EXYNOS4_SDMMC3_OFFSET			0x02540000
#define EXYNOS4_SDMMC4_OFFSET			0x02550000
#define EXYNOS4_MIPI_HSI_OFFSET			0x02560000	/* LCD0 */
#define EXYNOS4_SROMC_OFFSET			0x02570000

#define EXYNOS4_USB2HOST_OFFSET			0x02580000
#define EXYNOS4_USB2_HOST_EHCI_OFFSET		0x02580000
#define EXYNOS4_USB2_HOST_OHCI_OFFSET		0x02590000
#define EXYNOS4_USB2_HOST_PHYCTRL_OFFSET	0x025B0000
#define EXYNOS4_USBOTG1_OFFSET			0x025B0000	/* USB On The Go interface */

#define EXYNOS4_PDMA0_OFFSET			0x02680000	/* Peripheral DMA */
#define EXYNOS4_PDMA1_OFFSET			0x02690000
#define EXYNOS4_GADC_OFFSET			0x026C0000	/* General AD Converter */
#define EXYNOS4_ROTATOR_OFFSET			0x02810000	/* Image rotator for video output */
#define EXYNOS4_SMDMA_OFFSET			0x02840000	/* (s) Memory DMA */
#define EXYNOS4_NSMDMA_OFFSET			0x02850000	/* (ns) Memory DMA */
#define EXYNOS4_SMMUROTATOR_OFFSET		0x02A30000	/* system mmu for rotator */
#define EXYNOS4_SMMUMDMA_OFFSET			0x02A40000
#define EXYNOS4_VP_OFFSET			0x02C00000	/* Video Processor */
#define EXYNOS4_MIXER_OFFSET			0x02C10000	/* Video mixer */
#define EXYNOS4_HDMI0_OFFSET			0x02D00000
#define EXYNOS4_HDMI1_OFFSET			0x02D10000
#define EXYNOS4_HDMI2_OFFSET			0x02D20000
#define EXYNOS4_HDMI3_OFFSET			0x02D30000
#define EXYNOS4_HDMI4_OFFSET			0x02D40000
#define EXYNOS4_HDMI5_OFFSET			0x02D50000
#define EXYNOS4_HDMI6_OFFSET			0x02D60000
#define EXYNOS4_SMMUTV_OFFSET			0x02E20000
#define EXYNOS4_G3D_OFFSET			0x03000000	/* 3D Graphics Accelerator */
#define EXYNOS4_PPMU_3D_OFFSET			0x03220000
#define EXYNOS4_MFC_OFFSET			0x03400000	/* Multi Format Codec */
#define EXYNOS4_SMMUMFC_L_OFFSET		0x03620000
#define EXYNOS4_SMMUMFC_R_OFFSET		0x03630000
#define EXYNOS4_PMMU_MFC_L_OFFSET		0x03660000	/* ? */
#define EXYNOS4_PMMU_MFC_R_OFFSET		0x03670000	/* ? */
#define EXYNOS4_UART0_OFFSET			0x03800000	/* serial port 0 */
#define EXYNOS4_UART1_OFFSET			0x03810000	/* serial port 1 */
#define EXYNOS4_UART2_OFFSET			0x03820000	/* serial port 2 */
#define EXYNOS4_UART3_OFFSET			0x03830000	/* serial port 3 */
#define EXYNOS4_UART4_OFFSET			0x03840000	/* serial port 4 */
#define EXYNOS4_I2C0_OFFSET			0x03860000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C1_OFFSET			0x03870000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C2_OFFSET			0x03880000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C3_OFFSET			0x03890000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C4_OFFSET			0x038A0000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C5_OFFSET			0x038B0000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C6_OFFSET			0x038C0000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2C7_OFFSET			0x038D0000	/* Inter Integrated Circuit (I2C) */
#define EXYNOS4_I2CHDMI_OFFSET			0x038E0000	/* I2C for HDMI */
#define EXYNOS4_SPI0_OFFSET			0x03920000	/* Serial Peripheral Interface0 */
#define EXYNOS4_SPI1_OFFSET			0x03930000	/* Serial Peripheral Interface0 */
#define EXYNOS4_SPI2_OFFSET			0x03940000	/* Serial Peripheral Interface0 */
#define EXYNOS4_I2S1_OFFSET			0x03960000	/* sound */
#define EXYNOS4_I2S2_OFFSET			0x03970000	/* sound */
#define EXYNOS4_PCM1_OFFSET			0x03980000	/* sound */
#define EXYNOS4_PCM2_OFFSET			0x03990000	/* sound */
#define EXYNOS4_AC97_OFFSET			0x039A0000	/* AC97 audio codec sound */
#define EXYNOS4_SPDIF_OFFSET			0x039B0000	/* SPDIF sound */
#define EXYNOS4_PWMTIMER_OFFSET			0x039D0000

/* AUDIOCORE */
#define EXYNOS4_AUDIOCORE_OFFSET		0x04000000	/* on 1Mb L1 chunk */
#define EXYNOS4_AUDIOCORE_VBASE			(EXYNOS_CORE_VBASE + EXYNOS4_AUDIOCORE_OFFSET)
#define EXYNOS4_AUDIOCORE_PBASE			0x03800000	/* Audio SFR */
#define EXYNOS4_AUDIOCORE_SIZE			0x00100000

#define EXYNOS4_GPIO_I2S0_OFFSET		(EXYNOS4_AUDIOCORE_OFFSET + 0x00060000)

/* used Exynos4 USB PHY registers */
#define USB_PHYPWR			0x00
#define   PHYPWR_FORCE_SUSPEND		__BIT(1)
#define   PHYPWR_ANALOG_POWERDOWN	__BIT(3)
#define   PHYPWR_OTG_DISABLE		__BIT(4)
#define   PHYPWR_SLEEP_PHY0		__BIT(5)
#define   PHYPWR_NORMAL_MASK		0x19
#define   PHYPWR_NORMAL_MASK_PHY0	(__BITS(3,3) | 1)
#define   PHYPWR_NORMAL_MASK_PHY1	__BITS(6,3)
#define   PHYPWR_NORMAL_MASK_HSIC0	__BITS(9,3)
#define   PHYPWR_NORMAL_MASK_HSIC1	__BITS(12,3)
#define USB_PHYCLK			0x04			/* holds FSEL_CLKSEL_ */
#define USB_RSTCON			0x08
#define   RSTCON_SWRST			__BIT(0)
#define   RSTCON_HLINK_RWRST		__BIT(1)
#define   RSTCON_DEVPHYLINK_SWRST	__BIT(2)
#define   RSTCON_DEVPHY_SWRST		__BITS(0,3)
#define   RSTCON_HOSTPHY_SWRST		__BITS(3,4)
#define   RSTCON_HOSTPHYLINK_SWRST	__BITS(7,4)


#endif /* _ARM_SAMSUNG_EXYNOS4_REG_H_ */

