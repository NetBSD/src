/*	$NetBSD: dovereg.h,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DOVEREG_H_
#define _DOVEREG_H_

#include <arm/marvell/mvsocreg.h>

#define DOVE_UNITID_DDR			MVSOC_UNITID_DDR
#define DOVE_UNITID_DEVBUS		MVSOC_UNITID_DEVBUS
#define DOVE_UNITID_DB			0x2	/* Downstream Bridge reg */
#define DOVE_UNITID_SA			0x3	/* Security Accelelerator reg */
#define DOVE_UNITID_PEX			MVSOC_UNITID_PEX
#define DOVE_UNITID_USB			0x5
#define DOVE_UNITID_XOR			0x6
#define DOVE_UNITID_GBE			0x7
#define DOVE_UNITID_PEX1		0x8
#define DOVE_UNITID_SDIO		0x9
#define DOVE_UNITID_SATA		0xa
#define DOVE_UNITID_I2S			0xb
#define DOVE_UNITID_NAND		0xc
#define DOVE_UNITID_PMU			0xd
#define DOVE_UNITID_AC97		0xe

#define DOVE_ATTR_PEX_MEM		0xe8
#define DOVE_ATTR_PEX_IO		0xe0
#define DOVE_ATTR_SA			0x00
#define DOVE_ATTR_SPI0			0xfe
#define DOVE_ATTR_SPI1			0xfb
#define DOVE_ATTR_BOOTROM		0xfd
#define DOVE_ATTR_NAND			0x00
#define DOVE_ATTR_PMU			0x00

#define DOVE_IRQ_BRIDGE			0	/* Downstream Bridge intr */
#define DOVE_IRQ_H2CPUDB		1	/* Doorbell Interrupt */
#define DOVE_IRQ_CPU2HDB		2	/* Doorbell Interrupt */
#define DOVE_IRQ_NF			3	/* NandFlash Interrupt */
#define DOVE_IRQ_PDMA			4	/* Peripheral DMA Interrupt */
#define DOVE_IRQ_SPI1			5	/* SPI1 Ready Interrupt */
#define DOVE_IRQ_SPI0			6	/* SPI9 Ready Interrupt */
#define DOVE_IRQ_UART0			7	/* UART0 Interrupt */
#define DOVE_IRQ_UART1			8	/* UART1 Interrupt */
#define DOVE_IRQ_UART2			9	/* UART2 Interrupt */
#define DOVE_IRQ_UART3			10	/* UART3 Interrupt */
#define DOVE_IRQ_TWSI			11	/* TWSI Interrupt */
#define DOVE_IRQ_GPIO7_0		12	/* GPIO[7:0] Interrupt */
#define DOVE_IRQ_GPIO15_8		13	/* GPIO[15:8] Interrupt */
#define DOVE_IRQ_GPIO23_16		14	/* GPIO[23:16] Interrupt */
#define DOVE_IRQ_PEX0_ERR		15	/* PCI Express0 Error */
#define DOVE_IRQ_PEX0_INT		16	/*PCI Express0 INT [A-D] mesg*/
#define DOVE_IRQ_PEX1_ERR		17	/* PCI Express1 Error */
#define DOVE_IRQ_PEX1_INT		18	/*PCI Express1 INT [A-D] mesg*/
#define DOVE_IRQ_AUDIO0_INT		19	/* Audio0 Interrupt */
#define DOVE_IRQ_AUDIO0_ERR		20	/* Audio0 Error */
#define DOVE_IRQ_AUDIO1_INT		21	/* Audio1 Interrupt */
#define DOVE_IRQ_AUDIO1_ERR		22	/* Audio1 Error */
#define DOVE_IRQ_USBBR			23	/* USB Bridge Error */
#define DOVE_IRQ_USB0CNT		24	/* USB0 Controller Interrupt */
#define DOVE_IRQ_USB1CNT		25	/* USB1 Controller Interrupt */
#define DOVE_IRQ_GBERX			26	/* GbE Receive Interrupt */
#define DOVE_IRQ_GBETX			27	/* GbE Transmit Interrupt */
#define DOVE_IRQ_GBEMISC		28	/* GbE Miscellaneous intr */
#define DOVE_IRQ_GBESUM			29	/* GbE Summary */
#define DOVE_IRQ_GBEERR			30	/* GbE Error */
#define DOVE_IRQ_SECURITYINT		31	/* Security Interrupt */
#define DOVE_IRQ_AC97			32	/* AC97 Interrupt */
#define DOVE_IRQ_PMU			33	/* Power Management Unit intr */
#define DOVE_IRQ_CAM			34	/* Cafe Camera Interrupt */
#define DOVE_IRQ_SD0			35	/* SD0 IRQ Interrupt */
#define DOVE_IRQ_SD1			36	/* SD1 IRQ Interrupt */
#define DOVE_IRQ_XOR0_DMA0		39	/* XOR Unit DMA0 Completion */
#define DOVE_IRQ_XOR0_DMA1		40	/* XOR Unit DMA1 Completion */
#define DOVE_IRQ_XOR0ERR		41	/* XOR Unit Error Interrupt */
#define DOVE_IRQ_XOR1_DMA0		42	/* XOR Unit DMA0 Completion */
#define DOVE_IRQ_XOR1_DMA1		43	/* XOR Unit DMA1 Completion */
#define DOVE_IRQ_XOR1ERR		44	/* XOR Unit Error Interrupt */
#define DOVE_IRQ_IRE_DCON		45	/* IRE OR DCON Interrupt */
#define DOVE_IRQ_LCD1			46	/* LCD1 Interrupt */
#define DOVE_IRQ_LCD0			47	/* LCD0 Interrupt */
#define DOVE_IRQ_GPU			48	/* GPU Interrupt */
#define DOVE_IRQ_VMETA			51     /*Video dec Unit Semaphore intr*/
#define DOVE_IRQ_SSPTIMER		54	/* SSP Timer Interrupt */
#define DOVE_IRQ_SSPINT			55	/* SSP Interrupt */
#define DOVE_IRQ_MEMORYERR		56	/*mem Controller or L2 ECC err*/
#define DOVE_IRQ_DWNSTRMEXCLTM		57
#define DOVE_IRQ_UPSTRMADDERR		58
#define DOVE_IRQ_SECURITYERR		59	/* Security Error */
#define DOVE_IRQ_GPIO_31_24		60	/* Interrupt from GPIO[31:24] */
#define DOVE_IRQ_HIGHGPIO		61	/* intr from High GPIO[31:0] */
#define DOVE_IRQ_SATAINT		62	/* SATA Interrupt */


/*
 * Physical address of integrated peripherals
 */

#define DOVE_UNITID2PHYS(uid)		((DOVE_UNITID_ ## uid) << 16)

/*
 * SPI Registers
 */
#define DOVE_SPI0_BASE		(MVSOC_DEVBUS_BASE + 0x0600)	/* 0x10600 */
#define DOVE_SPI1_BASE		(MVSOC_DEVBUS_BASE + 0x4600)	/* 0x14600 */

/*
 * UART Interface Registers
 */
				/* NS16550 compatible */
#define DOVE_COM2_BASE		(MVSOC_DEVBUS_BASE + 0x2200)
#define DOVE_COM3_BASE		(MVSOC_DEVBUS_BASE + 0x2300)

/*
 * Downstream Bridge Registers
 */
#define DOVE_DB_BASE		(DOVE_UNITID2PHYS(DB))

/* CPU Address Map Registers */
#define DOVE_DB_NWINDOW			8
#define DOVE_DB_NREMAP			4

/* Main Interrupt Controller Registers */
#define DOVE_DB_MICR			  0x200 /* Main Interrupt Cause reg */
#define DOVE_DB_MIRQIMR			  0x204 /* Main IRQ Interrupt Mask */
#define DOVE_DB_MFIQIMR			  0x208 /* Main FIQ Interrupt Mask */
#define DOVE_DB_EIMR			  0x20c /* Endpoint Interrupt Mask */
#define DOVE_DB_SMICR			  0x210 /* Second Main Intr Cause reg */
#define DOVE_DB_SMIRQIMR		  0x214 /* Second Main IRQ intr Mask */
#define DOVE_DB_SMFIQIMR		  0x218 /* Second Main FIQ intr Mask */
#define DOVE_DB_SEIMR			  0x21c /* Second Endpoint intr Mask */
#define DOVE_DB_PCIEIMR			  0x220 /* PCIe0/PCIe1 intr Mask reg */


/*
 * Cryptographic Engine and Security Accelerator (CESA) Registers
 */								/* 0x3d000 */
#define DOVE_CESA_BASE		(DOVE_UNITID2PHYS(SA) + 0xd000)

/*
 * USB 2.0 Interface Registers
 */
#define DOVE_USB0_BASE		(DOVE_UNITID2PHYS(USB))		/* 0x50000 */
#define DOVE_USB1_BASE		(DOVE_UNITID2PHYS(USB) + 0x1000)

/*
 * XOR DMA Engine Registers
 */
#define DOVE_XORE_BASE		(DOVE_UNITID2PHYS(XOR))		/* 0x60000 */

/*
 * Gigabit Ethernet Controller (GbE) Registers
 */
#define DOVE_GBE_BASE		(DOVE_UNITID2PHYS(GBE))		/* 0x70000 */

/*
 * PCI Express (PCIe) Registers
 */
#define DOVE_PEX1_BASE		(DOVE_UNITID2PHYS(PEX1))	/* 0x80000 */

/*
 * Camera and SDIO Registers
 */
#define DOVE_SDHC_SIZE	0x2000
#define DOVE_SDHC1_BASE		(DOVE_UNITID2PHYS(SDIO))	/* 0x90000 */
#define DOVE_SDHC0_BASE		(DOVE_SDHC1_BASE + DOVE_SDHC_SIZE)
#define DOVE_CAMERA_BASE	(DOVE_SDHC0_BASE + DOVE_SDHC_SIZE)

/*
 * Serial-ATA Host Controller Registers
 */
#define DOVE_SATAHC_BASE	(DOVE_UNITID2PHYS(SATA))	/* 0xa0000 */

/*
 * Audio (I2S/S/PDIF) Interface Registers
 */
#define DOVE_AUDIO0_BASE	(DOVE_UNITID2PHYS(I2S))		/* 0xb0000 */
#define DOVE_AUDIO1_BASE	(DOVE_UNITID2PHYS(I2S) + 0x4000)

/*
 * NAND Flash Registers
 */
#define DOVE_NAND_BASE		(DOVE_UNITID2PHYS(NAND))	/* 0xc0000 */

/*
 * Power Management Registers
 */
#define DOVE_PMU_BASE		(DOVE_UNITID2PHYS(PMU))		/* 0xd0000 */
#define DOVE_MISC_BASE		(DOVE_UNITID2PHYS(PMU) + 0x0200)
#define DOVE_GPIO_BASE		(DOVE_UNITID2PHYS(PMU) + 0x0400)
#define DOVE_PMU_BASE2		(DOVE_UNITID2PHYS(PMU) + 0x8000)
#define DOVE_RTC_BASE		(DOVE_UNITID2PHYS(PMU) + 0x8500)
#define DOVE_PMU_SRAM_BASE	(DOVE_UNITID2PHYS(PMU) + 0xc000)
#define DOVE_PMU_SRAM_SIZE		0x800

#define DOVE_PMU_SIZE			0x200
#define DOVE_PMU_CPUSDFSCR		0x00
#define DOVE_PMU_CPUSDFSCR_DFSEN		(1 << 0)
#define DOVE_PMU_CPUSDFSCR_CPUSLOWEN		(1 << 1)
#define DOVE_PMU_CPUSDFSCR_DDRLRATIO(x)		(((x) & 0x3f) << 3)
#define DOVE_PMU_CPUSDFSCR_CPUL2CR(x)		(((x) & 0x3f) << 9)
#define DOVE_PMU_CPUSDFSCR_CHNGPLLEN		(1 << 16)
#define DOVE_PMU_CPUSDFSSR		0x04
#define DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_MASK		(1 << 1)
#define DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_TURBO	(0 << 1)
#define DOVE_PMU_CPUSDFSSR_CPUSLOWMODESTTS_SLOW		(1 << 1)
#define DOVE_PMU_TM_BASE		0x1c
#define DOVE_PMU_CGCR			0x38	/* Clock Gating Control reg */
#define DOVE_PMU_CPUCDC0R		0x44	/* CPU Clock Divider Control */
#define DOVE_PMU_CPUCDC0R_DPRATIO(x)		(((x) >> 24) & 0x3f) /* DRAM */
#define DOVE_PMU_CPUCDC0R_XPRATIO(x)		(((x) >> 16) & 0x3f) /* L2C */
#define DOVE_PMU_CPUCDC0R_BPRATIO(x)		(((x) >> 8) & 0x3f) /* AXI DS */
#define DOVE_PMU_CPUCDC0R_PPRATIO(x)		(((x) >> 0) & 0x3f) /* CPU */
#define DOVE_PMU_PMUICR			0x50	/* PMU Interruts Cause reg */
#define DOVE_PMU_PMUIMR			0x54	/* PMU Interruts Mask reg */
#define DOVE_PMU_PMUI_DFSDONE			(1 << 0) /* DFS Done */
#define DOVE_PMU_PMUI_DVSDONE			(1 << 1) /* DVS Done */
#define DOVE_PMU_PMUI_THERMCOOLING		(1 << 3) /* Thermal Cooling */
#define DOVE_PMU_PMUI_THERMOVERHEAT		(1 << 4) /*Thermal Overheating*/
#define DOVE_PMU_PMUI_RTCALARM			(1 << 5)
#define DOVE_PMU_PMUI_BATTFAULT			(1 << 6) /* Battery Fault */
#define DOVE_PMU_TDC0R			0x5c	/* Thermal Diode Control 0 */
#define DOVE_PMU_TDC0R_THERMSLEEP		(1 << 30) /* sleep mode */
#define DOVE_PMU_TDC0R_THERMOTFCALB		(1 << 29) /* On-The-Fly calb */
#define DOVE_PMU_TDC0R_THERMDOUBLESLOPE		(1 << 28)
#define DOVE_PMU_TDC0R_THERMAVGNUM_MASK		(0x7 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_NO		(0x0 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_2		(0x1 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_4		(0x2 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_8		(0x3 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_16		(0x4 << 25)
#define DOVE_PMU_TDC0R_THERMAVGNUM_32		(0x5 << 25)	/* ? */
#define DOVE_PMU_TDC0R_THERMSELCALCAPSRC_MASK	(0x7 << 20)
#define DOVE_PMU_TDC0R_THERMREFCALCOUNT_MASK	(0x1ff << 11)
#define DOVE_PMU_TDC0R_THERMREFCALCOUNT(x)	((x) << 11)
#define DOVE_PMU_TDC0R_THERMATEST		(0x3 << 10)
#define DOVE_PMU_TDC0R_THERMSELIREF		(1 << 8)
#define DOVE_PMU_TDC0R_THERMVBEBYPASS		(1 << 7)
#define DOVE_PMU_TDC0R_THERMSELVCAL_MASK	(0x3 << 5)
#define DOVE_PMU_TDC0R_THERMSELVCAL(x)		((x) << 5)
#define DOVE_PMU_TDC0R_THERMTCTRIP		(0x7 << 2)
#define DOVE_PMU_TDC0R_THERMSOFTRESET		(1 << 1)
#define DOVE_PMU_TDC0R_THERMPOWERDOWN		(1 << 0)
#define DOVE_PMU_TDC1R			0x60	/* Thermal Diode Control 1 */

#define DOVE_PMU_PMUCR			0x00	/* PMU Control Register */
#define DOVE_PMU_PMUCR_MASKFIQ			(1 << 28)
#define DOVE_PMU_PMUCR_MASKIRQ			(1 << 24)
#define DOVE_PMU_PMUCR_MCSLEEPREQACK		(1 << 19)
#define DOVE_PMU_PMUCR_MCSLEEPREQ		(1 << 18)
#define DOVE_PMU_PMUCR_MCHALTREQACK		(1 << 17)
#define DOVE_PMU_PMUCR_MCHALTREQ		(1 << 16)
#define DOVE_PMU_PMUCR_DDRSELFREFEN		(1 << 5)
#define DOVE_PMU_PMUCR_STDBYPWREN		(1 << 4)
#define DOVE_PMU_PMUCR_DEEPIDLEPWREN		(1 << 3)
#define DOVE_PMU_PMUCR_EBOOKMODE		(1 << 2)
#define DOVE_PMU_PMUCR_MEMRETENTIONEN		(1 << 0)

#define DOVE_MISC_SAMPLE_AT_RESET0	0x14

/*
 * Audio Codec'97 (AC'97) Registers
 */
#define DOVE_AC97_BASE		(DOVE_UNITID2PHYS(AC97))	/* 0xe0000 */
#define DOVE_SSP_BASE		(DOVE_UNITID2PHYS(AC97) + 0xc000)

#endif	/* _DOVEREG_H_ */
