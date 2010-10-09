/*	$NetBSD: orionreg.h,v 1.1.2.2 2010/10/09 03:31:40 yamt Exp $	*/
/*
 * Copyright (c) 2007, 2008 KIYOHARA Takashi
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

#ifndef _ORIONREG_H_
#define _ORIONREG_H_

#include <arm/marvell/mvsocreg.h>

/*
 *        Ver  GbE SATA  USB  PCI PCIe IDMA XORE CESA
 * 1181:    1   -,   -,   -,   -,  x2,   ?,   -,   -
 * 1281:    2   -,   -,   -,   -,  x2,   ?,   -,   -
 * 5082:    1  x1,  x1,  x2,   -,  x1,   o,   -,   o
 * 5180N:   1  x1,   -,  x1,  x1,  x1,   o,   -,   -
 * 5181:    1  x1,   -,  x1,  x1,  x1,   o,   -,   o
 * 5182:    1  x1,  x1,  x2,  x1,  x1,   o,   o,   o
 * 5281:    2  x1,   -,  x1,  x1,  x1,   o,   -,   -
 * 6082:    1  x2?, x1,  x1,   -,  x1,   -,   -,   o
 * 6183:    1   ?,   -,  x?,   ?,   ?,   ?,   -,   -
 * 8660:    1  x1,   -,  x1,  x1,  x1,   o,   -,   -
 */

#define ORION_UNITID_DDR		MVSOC_UNITID_DDR
#define ORION_UNITID_DEVBUS		MVSOC_UNITID_DEVBUS
#define ORION_UNITID_MLMB		MVSOC_UNITID_MLMB
#define ORION_UNITID_PEX1		0x3			/* 1181 only */
#define ORION_UNITID_PCI		0x3	/* PCI registers */
#define ORION_UNITID_PEX		MVSOC_UNITID_PEX
#define ORION_UNITID_USB0		0x5	/* USB registers Port0 */
#define ORION_UNITID_IDMA		0x6	/* IDMA registers */
#define ORION_UNITID_XOR		0x6	/* XOR registers */
#define ORION_UNITID_GBE		0x7	/* Gigabit Ethernet registers */
#define ORION_UNITID_SATA		0x8	/* SATA registers */
#define ORION_UNITID_CRYPT		0x9	/* Cryptographic Engine reg */
#define ORION_UNITID_SA			0x9	/* Security Accelerator reg */
#define ORION_UNITID_USB1		0xa	/* USB registers Port1 */

#define ORION_ATTR_DEVICE_CS0		0x1e
#define ORION_ATTR_DEVICE_CS1		0x1d
#define ORION_ATTR_DEVICE_CS2		0x1b
#define ORION_ATTR_FLASH_CS		0x1b
#define ORION_ATTR_BOOT_CS		0x0f
#define ORION_ATTR_PEX_CFG		0x79	/* bug workaround ?? */
#define ORION_ATTR_PEX_MEM		0x59
#define ORION_ATTR_PEX_IO		0x51
#define ORION_ATTR_PCI_MEM		0x59
#define ORION_ATTR_PCI_IO		0x51
#define ORION_ATTR_CRYPT		0x00

/*
 * Interrupt numbers
 */
#define ORION_IRQ_BRIDGE		0	/* Local to System Bridge */
#define ORION_IRQ_HOST2CPU		1	/* Doorbell (Host-to-CPU) */
#define ORION_IRQ_CPU2HOST		2	/* Doorbell (CPU-to-Host) */
#define ORION_IRQ_UART0			3
#define ORION_IRQ_UART1			4
#define ORION_IRQ_TWSI			5	/* Two-Wire Serial Interface */
#define ORION_IRQ_GPIO7_0		6	/* GPIO[7:0] */
#define ORION_IRQ_GPIO15_8		7	/* GPIO[15:8] */
#define ORION_IRQ_GPIO23_16		8	/* GPIO[23:16] not 1181 */
#define ORION_IRQ_GPIO31_24		9	/* GPIO[31:24] not 1181 */
#define ORION_IRQ_PEX0ERR		10	/* PCI Express error */
#define ORION_IRQ_PEX0INT		11	/* PCIe INTA, B, C, D message */
#define ORION_IRQ_PEX1ERR		12			/* 1181 only */
#define ORION_IRQ_USBCNT1		12	/* USB Port1 controller (5182)*/
#define ORION_IRQ_PEX1INT		13			/* 1181 only */
#define ORION_IRQ_DEVERR		14	/* Device bus error */
#define ORION_IRQ_PCIERR		15	/* PCI error */
#define ORION_IRQ_USBBR			16	/* USB bridge Port0 or1 error */
#define ORION_IRQ_USBCNT0		17	/* USB Port0 controller */
#define ORION_IRQ_GBERX			18	/* GbE receive interrupt */
#define ORION_IRQ_GBETX			19	/* GbE transmit interrupt */
#define ORION_IRQ_GBEMISC		20	/* GbE miscellaneous intr */
#define ORION_IRQ_GBESUM		21	/* GbE summary */
#define ORION_IRQ_GBEERR		22	/* GbE error */
#define ORION_IRQ_DMAERR		23	/* DMA or XOR error */
#define ORION_IRQ_IDMA0			24	/* IDMA Channel0 completion */
#define ORION_IRQ_IDMA1			25	/* IDMA Channel1 completion */
#define ORION_IRQ_IDMA2			26	/* IDMA Channel2 completion */
#define ORION_IRQ_IDMA3			27	/* IDMA Channel3 completion */
#define ORION_IRQ_SECURITYINTR		28	/* Security accelerator intr */
#define ORION_IRQ_SATAINTR		29	/* Serial-ATA interrupt */
#define ORION_IRQ_XOR0			30	/* XOR engine 0 interrupt */
#define ORION_IRQ_XOR1			31	/* XOR engine 1 interrupt */


/*
 * Physical address of integrated peripherals
 */

#define ORION_UNITID2PHYS(uid)	((ORION_UNITID_ ## uid) << 16)

/*
 * Pin Multiplexing Interface Registers
 */
#define ORION_PMI_BASE			(MVSOC_DEVBUS_BASE + 0x0000)
#define ORION_PMI_SIZE			  0x100		/* XXXX */
#define ORION_PMI_MPPCR0		   0x00
#define ORION_PMI_MPPCR1		   0x04
#define ORION_PMI_MPPCR2		   0x50
#define ORION_PMI_DEVMULTICR		   0x08
#define ORION_PMI_SAMPLE_AT_RESET	   0x10
#define ORION_PMISMPL_ARMDDRCLK_MASK		0x0f
#define ORION_PMISMPL_ARMDDRCLK_H_MASK		(1 << 23)
#define ORION_PMISMPL_ARMDDRCLK_333_167		0x00
#define ORION_PMISMPL_ARMDDRCLK_400_200		0x01
#define ORION_PMISMPL_ARMDDRCLK_400_133		0x02
#define ORION_PMISMPL_ARMDDRCLK_500_167		0x03
#define ORION_PMISMPL_ARMDDRCLK_533_133		0x04
#define ORION_PMISMPL_ARMDDRCLK_600_200		0x05
#define ORION_PMISMPL_ARMDDRCLK_667_167		0x06
#define ORION_PMISMPL_ARMDDRCLK_800_200		0x07
#define ORION_PMISMPL_ARMDDRCLK_480_160		0x0c
#define ORION_PMISMPL_ARMDDRCLK_550_183		0x0d
#define ORION_PMISMPL_ARMDDRCLK_525_175		0x0e
#define ORION_PMISMPL_ARMDDRCLK_466_233		0x11
#define ORION_PMISMPL_ARMDDRCLK_500_250		0x12
#define ORION_PMISMPL_ARMDDRCLK_533_266		0x13
#define ORION_PMISMPL_ARMDDRCLK_600_300		0x14
#define ORION_PMISMPL_ARMDDRCLK_450_150		0x15
#define ORION_PMISMPL_ARMDDRCLK_533_178		0x16
#define ORION_PMISMPL_ARMDDRCLK_575_192		0x17
#define ORION_PMISMPL_ARMDDRCLK_700_175		0x18
#define ORION_PMISMPL_ARMDDRCLK_733_183		0x19
#define ORION_PMISMPL_ARMDDRCLK_750_187		0x1a
#define ORION_PMISMPL_ARMDDRCLK_775_194		0x1b
#define ORION_PMISMPL_ARMDDRCLK_500_125		0x1c
#define ORION_PMISMPL_ARMDDRCLK_500_100		0x1d
#define ORION_PMISMPL_ARMDDRCLK_600_150		0x1e
#define ORION_PMISMPL_TCLK_MASK			0x3
#define ORION_PMISMPL_TCLK_133			0x0
#define ORION_PMISMPL_TCLK_150			0x1
#define ORION_PMISMPL_TCLK_166			0x2

/*
 * Mbus-L to Mbus Bridge Registers
 */
/* CPU Address Map Registers */
#define ORION_MLMB_NWINDOW		8          
#define ORION_MLMB_NREMAP		2

/* Main Interrupt Controller Registers */
#define ORION_MLMB_MICR			  0x200	/* Main Interrupt Cause reg */
#define ORION_MLMB_MIRQIMR		  0x204	/* Main IRQ Interrupt Mask */
#define ORION_MLMB_MFIQIMR		  0x208	/* Main FIQ Interrupt Mask */
#define ORION_MLMB_EIMR			  0x20c	/* Endpoint Interrupt Mask */

/*
 * PCI Express Interface Registers
 *   or PCI Interface Registers
 */
#define ORION_PEX1_BASE		(ORION_UNITID2PHYS(PEX1))	/* 0x30000 */
#define ORION_PCI_BASE		(ORION_UNITID2PHYS(PCI))	/* 0x30000 */

/*
 * USB 2.0 Interface Registers
 */
#define ORION_USB0_BASE		(ORION_UNITID2PHYS(USB0))	/* 0x50000 */
#define ORION_USB1_BASE		(ORION_UNITID2PHYS(USB1))	/* 0xa0000 */

/*
 * IDMA Controller and XOR Engine Registers
 */
#define ORION_IDMAC_BASE	(ORION_UNITID2PHYS(IDMA))	/* 0x60000 */

/*
 * Gigabit Ethernet Registers
 */
#define ORION_GBE_BASE		(ORION_UNITID2PHYS(GBE))	/* 0x70000 */

/*
 * Serial-ATA Host Controller (SATAHC) Registers
 */
#define ORION_SATAHC_BASE	(ORION_UNITID2PHYS(SATA))	/* 0x80000 */

/*
 * Cryptographic Engine and Security Accelerator Registers
 */
#define ORION_CESA_BASE		(ORION_UNITID2PHYS(CRYPT))	/* 0x90000 */

#endif	/* _ORIONREG_H_ */
