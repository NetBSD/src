/*	$NetBSD: mv78xx0reg.h,v 1.1.4.2 2010/10/22 09:23:11 uebayasi Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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

#ifndef _MV78XX0REG_H_
#define _MV78XX0REG_H_

#include <arm/marvell/mvsocreg.h>

/*
 *              MHz  PCIe  GbE  SATA TDMI  COM
 * MV76100:    800, 2      x2,   x1,   -,  x3
 * MV78100:    1.2, 2 x4,  x2,   x2,  x2,  x4
 * MV78200: 1.0 x2, 2 x4,  x4,   x2,  x2,  x4
 */

#define MV78XX0_UNITID_DDR		MVSOC_UNITID_DDR
#define MV78XX0_UNITID_DEVBUS		MVSOC_UNITID_DEVBUS
#define MV78XX0_UNITID_LMB		MVSOC_UNITID_LMB
#define MV78XX0_UNITID_GBE23		0x3	/* Gigabit Ethernet registers */
#define MV78XX0_UNITID_PEX		MVSOC_UNITID_PEX
#define MV78XX0_UNITID_USB		0x5	/* USB registers */
#define MV78XX0_UNITID_IDMA		0x6	/* IDMA registers */
#define MV78XX0_UNITID_XOR		0x6	/* XOR registers */
#define MV78XX0_UNITID_GBE01		0x7	/* Gigabit Ethernet registers */
#define MV78XX0_UNITID_CRYPT		0x9	/* Cryptographic Engine reg */
#define MV78XX0_UNITID_SA		0x9	/* Security Accelelerator reg */
#define MV78XX0_UNITID_SATA		0xa	/* SATA registers */
#define MV78XX0_UNITID_TDM		0xb	/* TDM registers */

#define MV78XX0_ATTR_DEVICE_CS0		0x3e
#define MV78XX0_ATTR_DEVICE_CS1		0x3d
#define MV78XX0_ATTR_DEVICE_CS2		0x3b
#define MV78XX0_ATTR_DEVICE_CS2		0x37
#define MV78XX0_ATTR_BOOT_CS		0x2f
#define MV78XX0_ATTR_SPI		0x1f
#define MV78XX0_ATTR_PEX_IO		0xe0	/* PCIe x4 Port 0.0 */
#define MV78XX0_ATTR_PEX_MEM		0xe8
#define MV78XX0_ATTR_PEX1_IO		0xe0	/* PCIe x1 Port 1 */
#define MV78XX0_ATTR_PEX1_MEM		0xe8
#define MV78XX0_ATTR_PEX_0_IO		0xe0	/* PCIe x1 Port 0.0 */
#define MV78XX0_ATTR_PEX_0_MEM		0xe8
#define MV78XX0_ATTR_PEX_1_IO		0xd0	/* PCIe x1 Port 0.1 */
#define MV78XX0_ATTR_PEX_1_MEM		0xd8
#define MV78XX0_ATTR_PEX_2_IO		0xb0	/* PCIe x1 Port 0.2 */
#define MV78XX0_ATTR_PEX_2_MEM		0xb8
#define MV78XX0_ATTR_PEX_3_IO		0x70	/* PCIe x1 Port 0.3 */
#define MV78XX0_ATTR_PEX_3_MEM		0x78

#define MV78XX0_IRQ_ERRSUM		0	/* Sum of Main Intr Err Cause */
#define MV78XX0_IRQ_SPI			1	/* SPI */
#define MV78XX0_IRQ_TWSI0		2	/* TWSI0 */
#define MV78XX0_IRQ_TWSI1		3	/* TWSI1 */
#define MV78XX0_IRQ_IDMA0		4	/* IDMA Channel0 completion */
#define MV78XX0_IRQ_IDMA1		5	/* IDMA Channel1 completion */
#define MV78XX0_IRQ_IDMA2		6	/* IDMA Channel2 completion */
#define MV78XX0_IRQ_IDMA3		7	/* IDMA Channel3 completion */
#define MV78XX0_IRQ_TIMER0		8	/* Timer0 */
#define MV78XX0_IRQ_TIMER1		9	/* Timer1 */
#define MV78XX0_IRQ_TIMER2		10	/* Timer2 */
#define MV78XX0_IRQ_TIMER3		11	/* Timer3 */
#define MV78XX0_IRQ_UART0		12	/* UART0 */
#define MV78XX0_IRQ_UART1		13	/* UART1 */
#define MV78XX0_IRQ_UART2		14	/* UART2 */
#define MV78XX0_IRQ_UART3		15	/* UART3 */
#define MV78XX0_IRQ_USB0		16	/* USB0 */
#define MV78XX0_IRQ_USB1		17	/* USB1 */
#define MV78XX0_IRQ_USB2		18	/* USB2 */
#define MV78XX0_IRQ_CRYPTO		19	/* Crypto engine completion */
#define MV78XX0_IRQ_XOR0		22	/* XOR engine 0 completion */
#define MV78XX0_IRQ_XOR1		23	/* XOR engine 1 completion */
#define MV78XX0_IRQ_SATA		26	/* SATA ports */
#define MV78XX0_IRQ_TDMI_INT		27	/* TDM */
#define MV78XX0_IRQ_PEX00INTA		32	/* PCIe Port0.0 INTA/B/C/D */
#define MV78XX0_IRQ_PEX01INTA		33	/* PCIe Port0.1 INTA/B/C/D */
#define MV78XX0_IRQ_PEX02INTA		34	/* PCIe Port0.2 INTA/B/C/D */
#define MV78XX0_IRQ_PEX03INTA		35	/* PCIe Port0.3 INTA/B/C/D */
#define MV78XX0_IRQ_PEX10INTA		36	/* PCIe Port1.0 INTA/B/C/D */
#define MV78XX0_IRQ_PEX11INTA		37	/* PCIe Port1.1 INTA/B/C/D */
#define MV78XX0_IRQ_PEX12INTA		38	/* PCIe Port1.2 INTA/B/C/D */
#define MV78XX0_IRQ_PEX13INTA		39	/* PCIe Port1.3 INTA/B/C/D */
#define MV78XX0_IRQ_GE00SUM		40	/* Gigabit Ethernet Port0 sum */
#define MV78XX0_IRQ_GE00RX		41	/* Gigabit Ethernet Port0 Rx */
#define MV78XX0_IRQ_GE00TX		42	/* Gigabit Ethernet Port0 Tx */
#define MV78XX0_IRQ_GE00MISC		43	/* Gigabit Ethernet Port0 misc*/
#define MV78XX0_IRQ_GE01SUM		44	/* Gigabit Ethernet Port1 sum */
#define MV78XX0_IRQ_GE01RX		45	/* Gigabit Ethernet Port1 Rx */
#define MV78XX0_IRQ_GE01TX		46	/* Gigabit Ethernet Port1 Tx */
#define MV78XX0_IRQ_GE01MISC		47	/* Gigabit Ethernet Port1 misc*/
#define MV78XX0_IRQ_GE10SUM		48	/* Gigabit Ethernet Port2 sum */
#define MV78XX0_IRQ_GE10RX		49	/* Gigabit Ethernet Port2 Rx */
#define MV78XX0_IRQ_GE10TX		50	/* Gigabit Ethernet Port2 Tx */
#define MV78XX0_IRQ_GE10MISC		51	/* Gigabit Ethernet Port2 misc*/
#define MV78XX0_IRQ_GE11SUM		52	/* Gigabit Ethernet Port3 sum */
#define MV78XX0_IRQ_GE11RX		53	/* Gigabit Ethernet Port3 Rx */
#define MV78XX0_IRQ_GE11TX		54	/* Gigabit Ethernet Port3 Tx */
#define MV78XX0_IRQ_GE11MISC		55	/* Gigabit Ethernet Port3 misc*/
#define MV78XX0_IRQ_GPIO0_7		56	/* GPIO[7:0] */
#define MV78XX0_IRQ_GPIO8_15		57	/* GPIO[15:8] */
#define MV78XX0_IRQ_GPIO16_23		58	/* GPIO[23:16] */
#define MV78XX0_IRQ_GPIO24_31		59	/* GPIO[31:24] */
#define MV78XX0_IRQ_DB_IN		60 /*Summary of Inbound Doorbell Cause*/
#define MV78XX0_IRQ_DB_OUT		61/*Summary of Outbound Doorbell Cause*/


/*
 * Physical address of integrated peripherals
 */

#undef UNITID2PHYS
#define UNITID2PHYS(uid)	((MV78XX0_UNITID_ ## uid) << 16)

/*
 * General Purpose Port Registers
 */
#define MV78XX0_GPP_BASE		(MVSOC_DEVBUS_BASE + 0x0100)
#define MV78XX0_GPP_SIZE		  0x100

/*
 * UART Interface Registers
 */
					/* NS16550 compatible */
#define MV78XX0_COM2_BASE		(MVSOC_DEVBUS_BASE + 0x2200)
#define MV78XX0_COM3_BASE		(MVSOC_DEVBUS_BASE + 0x2300)

/*
 * Mbus-L to Mbus Bridge Registers
 */
/* CPU Address Map Registers */
#define MV78XX0_MLMB_NWINDOW		14
#define MV78XX0_MLMB_NREMAP		8

/* Main Interrupt Controller Registers */
#define MV78XX0_MLMB_MICLR		  0x200	/*Main Interrupt Cause Low reg*/
#define MV78XX0_MLMB_MIRQIMLR		  0x204	/* Main IRQ Interrupt Mask */
#define MV78XX0_MLMB_MFIQIMLR		  0x208	/* Main FIQ Interrupt Mask */
#define MV78XX0_MLMB_EIMLR		  0x20c	/* Endpoint Interrupt Mask */
#define MV78XX0_MLMB_MICHR		  0x210	/* Main Intr Cause High reg */
#define MV78XX0_MLMB_MIRQIMHR		  0x214 /*Main IRQ Interrupt High Mask*/
#define MV78XX0_MLMB_MFIQIMHR		  0x218 /*Main FIQ Interrupt High Mask*/
#define MV78XX0_MLMB_EIMHR		  0x21c	/*Endpoint Interrupt High Mask*/

/* CPU Timers Registers */
/*   see oriontmrreg.h */


/*
 * PCI Express Interface Registers
 */
#define MV78XX0_PEX_BASE	(UNITID2PHYS(PEX))	/* 0x40000 */

/*
 * USB 2.0 Interface Registers
 */
#define MV78XX0_USB_BASE	(UNITID2PHYS(USB))	/* 0x50000 */

/*
 * IDMA Controller and XOR Engine Registers
 */
#define MV78XX0_IDMAC_BASE	(UNITID2PHYS(IDMA))	/* 0x60000 */

/*
 * Gigabit Ethernet Registers
 */
#define MV78XX0_GBE01_BASE	(UNITID2PHYS(GBE01))	/* 0x70000 */
#define MV78XX0_GBE23_BASE	(UNITID2PHYS(GBE23))	/* 0x30000 */

/*
 * Cryptographic Engine and Security Accelerator Registers
 */
#define MV78XX0_CESA_BASE	(UNITID2PHYS(CRYPT))	/* 0x90000 */

/*
 * Serial-ATA Host Controller (SATAHC) Registers
 */
#define MV78XX0_SATAHC_BASE	(UNITID2PHYS(SATA))	/* 0xa0000 */

/*
 * Time Division Multiplexing (TDM) Unit Registers
 */
#define MV78XX0_TDM_BASE	(UNITID2PHYS(TDM))	/* 0xb0000 */

#endif	/* _MV78XX0REG_H_ */
