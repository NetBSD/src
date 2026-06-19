/*	$NetBSD: amcc460ex.h,v 1.3 2026/06/19 18:55:24 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * AMCC/AppliedMicro 460EX system-on-chip
 */

#ifndef _IBM4XX_AMCC460EX_H_
#define	_IBM4XX_AMCC460EX_H_

/* Upper 4 bits (ERPN) of the physical address of the I/O region */
#define	AMCC460EX_OPB_PA_HIGH		0x4

#define	AMCC460EX_OPB_BASE		0xef600000

#define	AMCC460EX_UART0_BASE		0xef600300
#define	AMCC460EX_UART1_BASE		0xef600400
#define	AMCC460EX_UART2_BASE		0xef600500
#define	AMCC460EX_UART3_BASE		0xef600600
#define	AMCC460EX_IIC0_BASE		0xef600700
#define	AMCC460EX_IIC1_BASE		0xef600800
#define	AMCC460EX_GPIO0_BASE		0xef600b00
#define	AMCC460EX_ZMII0_BASE		0xef600d00
#define	AMCC460EX_EMAC0_BASE		0xef600e00
#define	AMCC460EX_EMAC1_BASE		0xef600f00
#define	AMCC460EX_TAH0_BASE		0xef601350
#define	AMCC460EX_TAH1_BASE		0xef601450
#define	AMCC460EX_RGMII0_BASE		0xef601500

/* UARTs get freq from external 11.0592 MHz osc */
#define	AMCC460EX_COM_FREQ		11059200

/*
 * PLB-PCIX host bridge. 
 */
#define	AMCC460EX_PCIX0_CFG_PLBA	0x0ec00000	/* ERPN 0xc */
#define	AMCC460EX_PCIX0_REGS_PLBA	0x0ec80000	/* ERPN 0xc */
#define	AMCC460EX_PCIX0_CFG_PA_HIGH	0xc
#define	AMCC460EX_PCIX0_IO_PLBA		0x08000000	/* ERPN 0xc */
#define	AMCC460EX_PCIX0_IO_PA_HIGH	0xc
#define	AMCC460EX_PCIX0_MEM_BASE	0x80000000	/* ERPN 0xd */
#define	AMCC460EX_PCIX0_MEM_PLBA_H	0xd
/* How much of the (128MB) outbound memory window has pinned mappings */
#define	AMCC460EX_PCIX0_MEM_SIZE	0x05000000	/* 80MB */

/*
 * Second outbound memory window (POM1)
 */
#define	AMCC460EX_PCIX0_PMEM_BASE	0x88000000	/* ERPN 0xd */
#define	AMCC460EX_PCIX0_PMEM_PLBA_H	0xd
#define	AMCC460EX_PCIX0_PMEM_SIZE	0x10000000	/* 256MB POM1/extent */
/* How much of the prefetchable window gets pinned into kernel VA */
#define	AMCC460EX_PCIX0_PMEM_MAP	0x04000000	/* 64MB */

/*
 * PCI Express root complexes (PCIE0/PCIE1).
 */
#define	AMCC460EX_PCIE0_DCR_BASE	0x100	/* PEGPL register block */
#define	AMCC460EX_PCIE1_DCR_BASE	0x120

#define	AMCC460EX_PCIE_CFG_PA_HIGH	0xd	/* ERPN of config windows */
#define	AMCC460EX_PCIE0_CFG_PLBA	0x30000000
#define	AMCC460EX_PCIE1_CFG_PLBA	0x38000000
#define	AMCC460EX_PCIE_CFG_SIZE		0x01000000	/* buses 0-15 */

#define	AMCC460EX_PCIE_MEM_PA_HIGH	0xe	/* ERPN of memory windows */
#define	AMCC460EX_PCIE0_MEM_PLBA	0x10000000
#define	AMCC460EX_PCIE1_MEM_PLBA	0x90000000
#define	AMCC460EX_PCIE_MEM_BASE		0xa0000000	/* PCI-side base */
#define	AMCC460EX_PCIE_MEM_SIZE		0x01000000

/* INTA-INTD, flat irq numbers (UIC3 bits 12-15 and 16-19) */
#define	AMCC460EX_PCIE0_INTA_IRQ	108
#define	AMCC460EX_PCIE1_INTA_IRQ	112

/*
 * AHB peripherals (USB). 
 */
#define	AMCC460EX_AHB_PA_HIGH		0x4	/* ERPN */
#define	AMCC460EX_AHB_BASE		0xbf000000
#define	AMCC460EX_AHB_SIZE		0x01000000

#define	AMCC460EX_USB_OTG_BASE		0xbff80000	/* DWC OTG, 256KB */
#define	AMCC460EX_USB_OTG_SIZE		0x00040000
#define	AMCC460EX_USB_OHCI_BASE		0xbffd0000
#define	AMCC460EX_USB_OHCI_SIZE		0x00000400
#define	AMCC460EX_USB_EHCI_BASE		0xbffd0400
#define	AMCC460EX_USB_EHCI_SIZE		0x00000400

/*
 * On-chip SATA, the lousy DWC SATA-II core
 */
#define	AMCC460EX_SATA_DMA_BASE		0xbffd0800
#define	AMCC460EX_SATA_DMA_SIZE		0x00000400
#define	AMCC460EX_SATA_BASE		0xbffd1000
#define	AMCC460EX_SATA_SIZE		0x00000800

/*
 * Per-port PCIe SDR registers
 */
#define	AMCC460EX_PESDR0_LOOP		0x303
#define	AMCC460EX_PESDR0_RCSSET		0x304
#define	AMCC460EX_PESDR0_RCSSTS		0x305
#define	AMCC460EX_PESDR1_LOOP		0x343
#define	AMCC460EX_PESDR1_RCSSET		0x344
#define	AMCC460EX_PESDR1_RCSSTS		0x345

#define	AMCC460EX_PESDR_LOOP_LNKUP	0x00001000	/* link trained */

/*
 * Interrupt numbers in the flat PIC space:
 * UIC0 : irqs 0-31
 * UIC1 : cascaded via UIC0 bit 30) is irqs 32-63
 * UIC2 : (UIC0 bit 10) is irqs 64-95
 * UIC3 : (UIC0 bit 16) is irqs 96-127
 */
#define	AMCC460EX_UART0_IRQ		33	/* UIC1 bit 1 */
#define	AMCC460EX_UART1_IRQ		1	/* UIC0 bit 1 */
#define	AMCC460EX_IIC0_IRQ		2	/* UIC0 bit 2 */
#define	AMCC460EX_IIC1_IRQ		3	/* UIC0 bit 3 */
#define	AMCC460EX_PCI_IRQ		32	/* UIC1 bit 0: PCI INTA */
#define	AMCC460EX_MAL_TXEOB_IRQ		70	/* UIC2 bit 6 */
#define	AMCC460EX_MAL_RXEOB_IRQ		71	/* UIC2 bit 7 */
#define	AMCC460EX_EMAC0_IRQ		80	/* UIC2 bit 16 */
#define	AMCC460EX_EMAC1_IRQ		81	/* UIC2 bit 17 */
#define	AMCC460EX_USB_EHCI_IRQ		93	/* UIC2 bit 29 */
#define	AMCC460EX_USB_OHCI_IRQ		94	/* UIC2 bit 30 */
#define	AMCC460EX_SATA_IRQ		96	/* UIC3 bit 0 */
#define	AMCC460EX_SATA_DMA_IRQ		101	/* UIC3 bit 5: AHB DMAC */

/*
 * L2 cache controller (L2C0) device control registers.
 */
#define	DCR_L2C0_CFG		0x030	/* L2 cache configuration */
#define	  L2C_CFG_L2M		0x80000000	/* SRAM array used as L2 */
#define	  L2C_CFG_ICU		0x40000000	/* I-cache uses L2 */
#define	  L2C_CFG_DCU		0x20000000	/* D-cache uses L2 */
#define	  L2C_CFG_FRAN		0x00200000	/* fast read ack (best perf) */
#define	  L2C_CFG_SS_256KB	0x00000000	/* SRAM size 256KB (only) */
#define	  L2C_CFG_SNPCI		0x00000020	/* snoop cache-inhibit writes */
#define	  L2C_CFG_RDBW		0x00000008	/* read byte write (required) */
#define	DCR_L2C0_CMD		0x031	/* L2 cache command */
#define	  L2C_CMD_INV		0x20000000	/* invalidate at L2C0_ADDR */
#define	  L2C_CMD_CCP		0x10000000	/* clear cache parity error */
#define	  L2C_CMD_CTE		0x08000000	/* clear tag error */
#define	  L2C_CMD_HCC		0x00800000	/* hardware clear whole cache */
#define	DCR_L2C0_ADDR		0x032	/* L2 cache address */
#define	DCR_L2C0_DATA		0x033	/* L2 cache data */
#define	DCR_L2C0_SR		0x034	/* L2 cache status */
#define	  L2C_SR_CC		0x80000000	/* command complete */
#define	DCR_L2C0_REVID		0x035	/* L2 cache revision id */
#define	DCR_L2C0_SNP0		0x036	/* L2 snoop region 0 */
#define	DCR_L2C0_SNP1		0x037	/* L2 snoop region 1 */
#define	  L2C_SNP_SSR_SHIFT	12	/* size field (bits 16:19) shift */
#define	  L2C_SNP_ESR		0x00000800	/* enable snoop region */

/* Internal SRAM0 bank config registers; zeroed to free the array for L2. */
#define	DCR_SRAM0_SB0CR		0x020
#define	DCR_SRAM0_SB1CR		0x021
#define	DCR_SRAM0_SB2CR		0x022
#define	DCR_SRAM0_SB3CR		0x023

/*
 * AHB-to-PLB bridge CSRs
 */
#define	DCR_AHB_REV		0x0a0	/* revision id */
#define	DCR_AHB_TOP		0x0a4	/* PLB2AHB top address */
#define	DCR_AHB_BOT		0x0a5	/* PLB2AHB bottom address */
#define	DCR_AHB_ATT		0x0a6	/* PLB2AHB attribute */
#define	DCR_AHB_CR		0x0a7	/* AHB2PLB control (PUOA in [6:3]) */
#define	  AHB_CR_PUOA_MASK	0x00000078	/* PLB upper order address */
#define	  AHB_CR_PUOA_SHIFT	3

#endif	/* _IBM4XX_AMCC460EX_H_ */
