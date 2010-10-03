/*	$NetBSD: mvsocreg.h,v 1.1 2010/10/03 05:49:24 kiyohara Exp $	*/
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

#ifndef _MVSOCREG_H_
#define _MVSOCREG_H_

#define MVSOC_UNITID_MASK		0xf
#define MVSOC_UNITID_DDR		0x0	/* DDR registers */
#define MVSOC_UNITID_DEVBUS		0x1	/* Device Bus registers */
#define MVSOC_UNITID_MLMB		0x2	/* Mbus-L to Mbus Bridge reg */
#define MVSOC_UNITID_PEX		0x4	/* PCI Express Interface reg */


/*
 * Physical address of integrated peripherals
 */

#define UNITID2PHYS(uid)	((MVSOC_UNITID_ ## uid) << 16)

/*
 * DDR SDRAM Controller Registers
 */
#define MVSOC_DDR_BASE		(UNITID2PHYS(DDR))	/* 0x00000 */

/* DDR SDRAM Contriller Address Decode Registers */
#define MVSOC_DSC_BASE			0x01500	/* DDR SDRAM Ctrl Addr Reg */
#define MVSOC_DSC_NCS			4
#define MVSOC_DSC_CSBAR(x)		((x) * 8)
#define MVSOC_DSC_CSBAR_BASE_MASK	0xff000000
#define MVSOC_DSC_CSSR(x)		((x) * 8 + 4)
#define MVSOC_DSC_CSSR_WINEN		0x00000001
#define MVSOC_DSC_CSSR_SIZE_MASK	0xff000000


/*
 * Device Bus
 */
#define MVSOC_DEVBUS_BASE	(UNITID2PHYS(DEVBUS))	/* 0x10000 */

/*
 * General Purpose Port Registers
 */
#define MVSOC_GPP_BASE			(MVSOC_DEVBUS_BASE + 0x0100)

/*
 * Two-Wire Serial Interface Registers
 */
#define MVSOC_TWSI_BASE			(MVSOC_DEVBUS_BASE + 0x1000)

/*
 * UART Interface Registers
 */
					/* NS16550 compatible */
#define MVSOC_COM0_BASE			(MVSOC_DEVBUS_BASE + 0x2000)
#define MVSOC_COM1_BASE			(MVSOC_DEVBUS_BASE + 0x2100)

/*
 * Mbus-L to Mbus Bridge Registers
 */
#define MVSOC_MLMB_BASE		(UNITID2PHYS(MLMB))	/* 0x20000 */

/* CPU Address Map Registers */
#define MVSOC_MLMB_WCR(w)		  (((w) << 4) + 0x0)
#define MVSOC_MLMB_WCR_WINEN			(1 << 0)
#define MVSOC_MLMB_WCR_TARGET(t)		(((t) & 0xf) << 4)
#define MVSOC_MLMB_WCR_ATTR(a)			(((a) & 0xff) << 8)
#define MVSOC_MLMB_WCR_SIZE_MASK		0xffff0000
#define MVSOC_MLMB_WCR_SIZE(s)		  (((s) - 1) & MVSOC_MLMB_WCR_SIZE_MASK)
#define MVSOC_MLMB_WBR(w)		  (((w) << 4) + 0x4)
#define MVSOC_MLMB_WBR_BASE_MASK		0xffff0000
#define MVSOC_MLMB_WRLR(w)		  (((w) << 4) + 0x8)
#define MVSOC_MLMB_WRLR_REMAP_MASK		0xffff0000
#define MVSOC_MLMB_WRHR(w)		  (((w) << 4) + 0xc)
#define MVSOC_MLMB_IRBAR		  0x080 /* Internal regs Base Address */
#define MVSOC_MLMB_IRBAR_BASE_MASK	0xfff00000

/* CPU Control and Status Registers */
#define MVSOC_MLMB_CPUCR		  0x100	/* CPU Configuration Register */
#define MVSOC_MLMB_CPUCSR		  0x104	/* CPU Control/Status Register*/
#define MVSOC_MLMB_RSTOUTNMASKR		  0x108 /* RSTOUTn Mask Register */
#define MVSOC_MLMB_SSRR			  0x10c	/* System Soft Reset Register */
#define MVSOC_MLMB_MLMBICR		  0x110	/*Mb-L to Mb Bridge Intr Cause*/
#define MVSOC_MLMB_MLMBIMR		  0x114	/*Mb-L to Mb Bridge Intr Mask */

#define MVSOC_MLMB_L2CFG		  0x128	/* L2 Cache Config */

#define MVSOC_TMR_BASE			(MVSOC_MLMB_BASE + 0x0300)

/* CPU Doorbell Registers */
#define MVSOC_MLMB_H2CDR		  0x400	/* Host-to-CPU Doorbell */
#define MVSOC_MLMB_H2CDMR		  0x404	/* Host-to-CPU Doorbell Mask */
#define MVSOC_MLMB_C2HDR		  0x408	/* CPU-to-Host Doorbell */
#define MVSOC_MLMB_C2HDMR		  0x40c	/* CPU-to-Host Doorbell Mask */

/* Local to System Bridge Interrupt {Cause,Mask} Register bits */
#define MVSOC_MLMB_MLMBI_CPUSELFINT		0
#define MVSOC_MLMB_MLMBI_CPUTIMER0INTREQ	1
#define MVSOC_MLMB_MLMBI_CPUTIMER1INTREQ	2
#define MVSOC_MLMB_MLMBI_CPUWDTIMERINTREQ	3
#define MVSOC_MLMB_MLMBI_ACCESSERR		4
#define MVSOC_MLMB_MLMBI_BIT64ERR		5

#define MVSOC_MLMB_MLMBI_NIRQ			6

/*
 * PCI-Express Interface Registers
 */
#define MVSOC_PEX_BASE		(UNITID2PHYS(PEX))	/* 0x40000 */

#endif	/* _MVSOCREG_H_ */
