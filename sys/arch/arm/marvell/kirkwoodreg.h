/*	$NetBSD: kirkwoodreg.h,v 1.4.2.1 2014/08/20 00:02:47 tls Exp $	*/
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

#ifndef _KIRKWOODREG_H_
#define _KIRKWOODREG_H_

#include <arm/marvell/mvsocreg.h>
#include <dev/marvell/mvgbereg.h>

/*
 *                MHz TCLK  GbE SATA TDMI Audio MTS GPIO TSens TWSI
 * 6180:     600/800, 166,  x1,   -,   -,    o,  -,  30,    -,  x1
 * 6190:         600, 166, *x2,  x1,   -,    -,  -,  36,    -,  x1
 * 6192:         800, 166,  x2,  x2,   o,    o,  o,  36,    -,  x1
 * 6281: 1.0/1.2/1.5, 200,  x2,  x2,   o,    o,  o,  50,    -,  x1
 * 6282:       ?-2.0, 200,  x2,  x2,   o,    o,  o,  50,    o,  x2
 *
 *  * GbE x1 + 100BT x1
 */

#define KIRKWOOD_UNITID_DDR		MVSOC_UNITID_DDR
#define KIRKWOOD_UNITID_DEVBUS		MVSOC_UNITID_DEVBUS
#define KIRKWOOD_UNITID_MLMB		MVSOC_UNITID_MLMB
#define KIRKWOOD_UNITID_CRYPT		0x3	/* Cryptographic Engine reg */
#define KIRKWOOD_UNITID_SA		0x3	/* Security Accelelerator reg */
#define KIRKWOOD_UNITID_PEX		MVSOC_UNITID_PEX
#define KIRKWOOD_UNITID_USB		0x5	/* USB registers */
#define KIRKWOOD_UNITID_IDMA		0x6	/* IDMA registers */
#define KIRKWOOD_UNITID_XOR		0x6	/* XOR registers */
#define KIRKWOOD_UNITID_GBE		0x7	/* Gigabit Ethernet registers */
#define KIRKWOOD_UNITID_SATA		0x8	/* SATA registers */
#define KIRKWOOD_UNITID_SDIO		0x9	/* SDIO registers */
#define KIRKWOOD_UNITID_AUDIO		0xa	/* Audio registers */
#define KIRKWOOD_UNITID_MTS		0xb	/* MPEG Transport Stream reg */
#define KIRKWOOD_UNITID_TDM		0xd	/* TDM registers */

#define KIRKWOOD_ATTR_NAND		0x2f
#define KIRKWOOD_ATTR_SPI		0x1e
#define KIRKWOOD_ATTR_BOOTROM		0x1d
#define KIRKWOOD_ATTR_PEX_MEM		0xe8
#define KIRKWOOD_ATTR_PEX_IO		0xe0
#define KIRKWOOD_ATTR_PEX1_MEM		0xd8
#define KIRKWOOD_ATTR_PEX1_IO		0xd0
#define KIRKWOOD_ATTR_CRYPT		0x00

#define KIRKWOOD_IRQ_HIGH		0	/* High interrupt */
#define KIRKWOOD_IRQ_BRIDGE		1	/* Mbus-L to Mbus Bridge */
#define KIRKWOOD_IRQ_H2CPUDB		2	/* Doorbell interrupt */
#define KIRKWOOD_IRQ_CPU2HDB		3	/* Doorbell interrupt */
#define KIRKWOOD_IRQ_XOR0CHAN0		5	/* Xor 0 Channel0 */
#define KIRKWOOD_IRQ_XOR0CHAN1		6	/* Xor 0 Channel1 */
#define KIRKWOOD_IRQ_XOR1CHAN0		7	/* Xor 1 Channel0 */
#define KIRKWOOD_IRQ_XOR1CHAN1		8	/* Xor 1 Channel1 */
#define KIRKWOOD_IRQ_PEX0INT		9	/* PCI Express port0 INT A-D */
#define KIRKWOOD_IRQ_PEX1INT		10	/* PCI Express port1 INT A-D */
#define KIRKWOOD_IRQ_GBE0SUM		11	/* GbE0 summary */
#define KIRKWOOD_IRQ_GBE0RX		12	/* GbE0 receive interrupt */
#define KIRKWOOD_IRQ_GBE0TX		13	/* GbE0 transmit interrupt */
#define KIRKWOOD_IRQ_GBE0MISC		14	/* GbE0 miscellaneous intr */
#define KIRKWOOD_IRQ_GBE1SUM		15	/* GbE1 summary */
#define KIRKWOOD_IRQ_GBE1RX		16	/* GbE1 receive interrupt */
#define KIRKWOOD_IRQ_GBE1TX		17	/* GbE1 transmit interrupt */
#define KIRKWOOD_IRQ_GBE1MISC		18	/* GbE1 miscellaneous intr */
#define KIRKWOOD_IRQ_USB0CNT		19	/* USB0 controller interrupt */
#define KIRKWOOD_IRQ_SATA		21	/*Sata ports interrupt summary*/
#define KIRKWOOD_IRQ_SECURITYINT	22	/* Security engine completion */
#define KIRKWOOD_IRQ_SPIINT		23	/* SPI Interrupt */
#define KIRKWOOD_IRQ_AUDIOINT		24	/* Audio interrupt */
#define KIRKWOOD_IRQ_TS0INT		26	/* TS0 Interrupt */
#define KIRKWOOD_IRQ_SDIOINT		28	/* SDIO Interrupt */
#define KIRKWOOD_IRQ_TWSI		29	/* TWSI interrupt */
#define KIRKWOOD_IRQ_AVBINT		30	/* AVB Interrupt */
#define KIRKWOOD_IRQ_TDMINT		31	/* TDM Interrupt */
#define KIRKWOOD_IRQ_TWSI1		32	/* TWSI1 interrupt */
#define KIRKWOOD_IRQ_UART0INT		33	/* UART0 */
#define KIRKWOOD_IRQ_UART1INT		34	/* UART1 */
#define KIRKWOOD_IRQ_GPIOLO7_0		35	/* GPIO Low[7:0] */
#define KIRKWOOD_IRQ_GPIOLO8_15		36	/* GPIO Low[15:8] */
#define KIRKWOOD_IRQ_GPIOLO16_23	37	/* GPIO Low[23:16] */
#define KIRKWOOD_IRQ_GPIOLO24_31	38	/* GPIO Low[31:24] */
#define KIRKWOOD_IRQ_GPIOHI7_0		39	/* GPIO High[7:0] */
#define KIRKWOOD_IRQ_GPIOHI8_15		40	/* GPIO High[15:8] */
#define KIRKWOOD_IRQ_GPIOHI16_23	41	/* GPIO High[23:16] */
#define KIRKWOOD_IRQ_XOR0ERR		42	/* XOR0 error */
#define KIRKWOOD_IRQ_XOR1ERR		43	/* XOR1 error */
#define KIRKWOOD_IRQ_PEX0ERR		44	/* PCI Express0 error */
#define KIRKWOOD_IRQ_PEX1ERR		45	/* PCI Express1 error */
#define KIRKWOOD_IRQ_GBE0ERR		46	/* GbE port0 error */
#define KIRKWOOD_IRQ_GBE1ERR		47	/* GbE port1 error */
#define KIRKWOOD_IRQ_USBERR		48	/* USB error */
#define KIRKWOOD_IRQ_SECURITYERR	49	/* Cryptographic engine error */
#define KIRKWOOD_IRQ_AUDIOERR		50	/* Audio error */
#define KIRKWOOD_IRQ_RTCINT		53	/* Real time clock interrupt */


/*
 * Physical address of integrated peripherals
 */

#define KIRKWOOD_UNITID2PHYS(uid)	((KIRKWOOD_UNITID_ ## uid) << 16)

/*
 * Pin Multiplexing Interface Registers
 */
#define KIRKWOOD_MPP_BASE		(MVSOC_DEVBUS_BASE + 0x0000)
#define KIRKWOOD_MPP_MPPC0R		   0x00
#define KIRKWOOD_MPP_MPPC1R		   0x04
#define KIRKWOOD_MPP_MPPC2R		   0x08
#define KIRKWOOD_MPP_MPPC3R		   0x0c
#define KIRKWOOD_MPP_MPPC4R		   0x10
#define KIRKWOOD_MPP_MPPC5R		   0x14
#define KIRKWOOD_MPP_MPPC6R		   0x18
#define KIRKWOOD_MPP_SAMPLE_AT_RESET	   0x30

/*
 * Miscellaneous Registers
 */
#define KIRKWOOD_MISC_BASE		(MVSOC_DEVBUS_BASE + 0x0000)
#define KIRKWOOD_MISC_DEVICEID		   0x34
#define KIRKWOOD_MISC_CLOCKCONTROL	   0x4c
#define KIRKWOOD_MISC_SYSRSTLC		   0x50	/* SYSRSTn Length Counter */
#define KIRKWOOD_MISC_AGC		   0x7c	/* Analog Group Configuration */
#define KIRKWOOD_MISC_SSCGC		   0xd8	/* SSCG Configuration */
#define KIRKWOOD_MISC_PTPCC		   0xdc	/* PTP Clock Configuration */
#define KIRKWOOD_MISC_IOC0		   0xe0	/* IO Configuration 0 */

/*
 * Real-Time Clock Unit Registers
 */
#define KIRKWOOD_RTC_BASE		(MVSOC_DEVBUS_BASE + 0x0300)

/*
 * Serial Peripheral Interface Registers
 */
#define KIRKWOOD_SPI_BASE		(MVSOC_DEVBUS_BASE + 0x0600)

/*
 * Mbus-L to Mbus Bridge Registers
 */
/* CPU Address Map Registers */
#define KIRKWOOD_MLMB_NWINDOW		8
#define KIRKWOOD_MLMB_NREMAP		4

/* Main Interrupt Controller Registers */
#define KIRKWOOD_MLMB_MICLR		  0x200	/*Main Interrupt Cause Low reg*/
#define KIRKWOOD_MLMB_MIRQIMLR		  0x204	/*Main IRQ Interrupt Low Mask*/
#define KIRKWOOD_MLMB_MFIQIMLR		  0x208	/*Main FIQ Interrupt Low Mask*/
#define KIRKWOOD_MLMB_EIMLR		  0x20c	/*Endpoint Interrupt Low Mask*/
#define KIRKWOOD_MLMB_MICHR		  0x210	/*Main Intr Cause High reg*/
#define KIRKWOOD_MLMB_MIRQIMHR		  0x214	/*Main IRQ Interrupt High Mask*/
#define KIRKWOOD_MLMB_MFIQIMHR		  0x218	/*Main FIQ Interrupt High Mask*/
#define KIRKWOOD_MLMB_EIMHR		  0x21c	/*Endpoint Interrupt High Mask*/


/*
 * Kirkwood Thermal Sensor(6282 only)
 */
#define KIRKWOOD_TS_BASE	(MVSOC_DEVBUS_BASE + 0x0078)	/* XXXX: ??? */

/*
 * Two-Wire Serial Interface Registers
 */
#define KIRKWOOD_TWSI1_BASE	(MVSOC_TWSI_BASE + 0x0100)

/*
 * PCI-Express Interface Registers
 */
#define KIRKWOOD_PEX1_BASE	(MVSOC_PEX_BASE + 0x4000)

/*
 * Cryptographic Engine and Security Accelerator Registers
 */								/* 0x3d000 */
#define KIRKWOOD_CESA_BASE	(KIRKWOOD_UNITID2PHYS(CRYPT) + 0xd000)

/*
 * USB 2.0 Interface Registers
 */
#define KIRKWOOD_USB_BASE	(KIRKWOOD_UNITID2PHYS(USB))	/* 0x50000 */

/*
 * IDMA Controller and XOR Engine Registers
 */
#define KIRKWOOD_IDMAC_BASE	(KIRKWOOD_UNITID2PHYS(IDMA))	/* 0x60000 */

/*
 * Gigabit Ethernet Registers
 */
#define KIRKWOOD_GBE0_BASE	(KIRKWOOD_UNITID2PHYS(GBE))	/* 0x70000 */
#define KIRKWOOD_GBE1_BASE	(KIRKWOOD_GBE0_BASE + MVGBE_SIZE)

/*
 * Serial-ATA Host Controller (SATAHC) Registers
 */
#define KIRKWOOD_SATAHC_BASE	(KIRKWOOD_UNITID2PHYS(SATA))	/* 0x80000 */

/*
 * Secure Digital Input/Output (SDIO) Interface Registers
 */
#define KIRKWOOD_SDIO_BASE	(KIRKWOOD_UNITID2PHYS(SDIO))	/* 0x90000 */

/*
 * Audio (I2S/S/PDIF) Interface Registers
 */
#define KIRKWOOD_AUDIO_BASE	(KIRKWOOD_UNITID2PHYS(AUDIO))	/* 0xa0000 */

/*
 * MPEG-2 Transport Stream (TS) Interface Registers
 */
#define KIRKWOOD_MTS_BASE	(KIRKWOOD_UNITID2PHYS(MTS))	/* 0xb0000 */

/*
 * Time Division Multiplexing (TDM) Unit Registers
 */
#define KIRKWOOD_TDM_BASE	(KIRKWOOD_UNITID2PHYS(TDM))	/* 0xd0000 */

#endif	/* _KIRKWOODREG_H_ */
