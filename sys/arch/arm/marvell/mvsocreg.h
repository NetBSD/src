/*	$NetBSD: mvsocreg.h,v 1.2.16.4 2017/12/03 11:35:54 jdolecek Exp $	*/
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


#define MVSOC_INTERREGS_SIZE		0x00100000	/* 1 MB */


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
#define MVSOC_GPP_BASE		(MVSOC_DEVBUS_BASE + 0x0100)

/*
 * Two-Wire Serial Interface Registers
 */
#define MVSOC_TWSI_BASE		(MVSOC_DEVBUS_BASE + 0x1000)

/*
 * UART Interface Registers
 */
				/* NS16550 compatible */
#define MVSOC_COM0_BASE		(MVSOC_DEVBUS_BASE + 0x2000)
#define MVSOC_COM1_BASE		(MVSOC_DEVBUS_BASE + 0x2100)

/*
 * Mbus-L to Mbus Bridge Registers
 */
#define MVSOC_MLMB_BASE		(UNITID2PHYS(MLMB))	/* 0x20000 */

/* CPU Address Map Registers */
#define MVSOC_MLMB_WCR(w)		  ((w) < 8 ? ((w) << 4) + 0x0 :\
						     (((w) - 8) << 3) + 0x90)
#define MVSOC_MLMB_WCR_WINEN			(1 << 0)
#define MVSOC_MLMB_WCR_SYNC			(1 << 1) /* sync barrier */
#define MVSOC_MLMB_WCR_TARGET(t)		(((t) & 0xf) << 4)
#define MVSOC_MLMB_WCR_GET_TARGET(reg)		(((reg) >> 4) & 0xf)
#define MVSOC_MLMB_WCR_ATTR(a)			(((a) & 0xff) << 8)
#define MVSOC_MLMB_WCR_GET_ATTR(reg)		(((reg) >> 8) & 0xff)
#define MVSOC_MLMB_WCR_SIZE_MASK		0xffff0000
#define MVSOC_MLMB_WCR_SIZE(s)		  (((s) - 1) & MVSOC_MLMB_WCR_SIZE_MASK)
#define MVSOC_MLMB_WCR_GET_SIZE(reg) \
    (((reg) & MVSOC_MLMB_WCR_SIZE_MASK) + (1 << 16))
#define MVSOC_MLMB_WBR(w)		  ((w) < 8 ? ((w) << 4) + 0x4 :\
						     (((w) - 8) << 3) + 0x94)
#define MVSOC_MLMB_WBR_BASE_MASK		0xffff0000
#define MVSOC_MLMB_WBR_GET_BASE(reg)		(reg & MVSOC_MLMB_WBR_BASE_MASK)
#define MVSOC_MLMB_WRLR(w)		  (((w) << 4) + 0x8)
#define MVSOC_MLMB_WRLR_REMAP_MASK		0xffff0000
#define MVSOC_MLMB_WRLR_GET_REMAP(reg) \
    (reg & MVSOC_MLMB_WRLR_REMAP_MASK)
#define MVSOC_MLMB_WRHR(w)		  (((w) << 4) + 0xc)
#define MVSOC_MLMB_IRBAR		  0x080 /* Internal regs Base Address */
#define MVSOC_MLMB_IRBAR_BASE_MASK		0xfff00000

/* CPU Control and Status Registers */
#define MVSOC_MLMB_CPUCR		  0x100	/* CPU Configuration Register */
#define MVSOC_MLMB_CPUCSR		  0x104	/* CPU Control/Status Register*/
#define MVSOC_MLMB_RSTOUTNMASKR		  0x108 /* RSTOUTn Mask Register */
#define MVSOC_MLMB_RSTOUTNMASKR_SOFTRSTOUTEN    (1 << 2)
#define MVSOC_MLMB_RSTOUTNMASKR_WDRSTOUTEN      (1 << 1)
#define MVSOC_MLMB_RSTOUTNMASKR_PEXRSTOUTEN     (1 << 0)
#define MVSOC_MLMB_SSRR			  0x10c	/* System Soft Reset Register */
#define MVSOC_MLMB_SSRR_SYSTEMSOFTRST           (1 << 0)
#define MVSOC_MLMB_MLMBICR		  0x110	/*Mb-L to Mb Bridge Intr Cause*/
#define MVSOC_MLMB_MLMBIMR		  0x114	/*Mb-L to Mb Bridge Intr Mask */

#define MVSOC_MLMB_CLKGATING		  0x11c	/* Clock Gating Control */
#define MVSOC_MLMB_CLKGATING_LNR	  (1 << 13) /* Load New Ratio */
#define MVSOC_MLMB_CLKGATING_GPH	  (1 << 12) /* Go To Power Half */
#define MVSOC_MLMB_CLKGATING_GPS	  (1 << 11) /* Go To Power Save */
#define MVSOC_MLMB_CLKGATING_CR		  (1 << 10) /* Production Realignment */
#define MVSOC_MLMB_CLKGATING_BIT(n)	  (1 << (n))

#define MVSOC_MLMB_L2CFG		  0x128	/* L2 Cache Config */

#define MVSOC_MLMB_NWIN			  4
#define MVSOC_MLMB_WINBAR(w)		  (((w) << 3) + 0x180)
#define MVSOC_MLMB_WINBAR_BASE_MASK		0xff000000
#define MVSOC_MLMB_WINCR(w)		  (((w) << 3) + 0x184)
#define MVSOC_MLMB_WINCR_EN			(1 << 0)
#define MVSOC_MLMB_WINCR_WINCS(x)		(((x) & 0x1c) >> 2)
#define MVSOC_MLMB_WINCR_SIZE_MASK		0xff000000

/* Coherent Fabric(CFU) Control and Status */
#define MVSOC_MLMB_CFU_FAB_CTRL			0x200
#define MVSOC_MLMB_CFU_FAB_CTRL_PROP_ERR	(0x1 << 8)
#define MVSOC_MLMB_CFU_FAB_CTRL_SNOOP_CPU0	(0x1 << 24)
#define MVSOC_MLMB_CFU_FAB_CTRL_SNOOP_CPU1	(0x1 << 25)
#define MVSOC_MLMB_CFU_FAB_CTRL_SNOOP_CPU2	(0x1 << 26)
#define MVSOC_MLMB_CFU_FAB_CTRL_SNOOP_CPU3	(0x1 << 27)

/* Coherent Fabiric Configuration */
#define MVSOC_MLMB_CFU_FAB_CFG			0x204

/* CFU IO Event Affinity */
#define MVSOC_MLMB_CFU_EVA			0x208

/* CFU IO Snoop Affinity */
#define MVSOC_MLMB_CFU_IOA			0x20c

/* CFU Configuration XXX: changed in ARMADA 370 */ 
#define MVSOC_MLMB_CFU_CFG			0x228
#define MVSOC_MLMB_CFU_CFG_L2_NOTIFY		(0x1 << 16)

/* CIB registers offsets */
#define MVSOC_MLMB_CIB_CTRL_CFG			0x280
#define MVSOC_MLMB_CIB_CTRL_CFG_WB_EN		(0x1 << 0)
#define MVSOC_MLMB_CIB_CTRL_CFG_STOP		(0x1 << 9)
#define MVSOC_MLMB_CIB_CTRL_CFG_IGN_SHARE	(0x2 << 10)
#define MVSOC_MLMB_CIB_CTRL_CFG_EMPTY		(0x1 << 13)

/* CIB barrier register */
#define MVSOC_MLMB_CIB_BARRIER(cpu)		(0x1810 + 0x100 * (cpu))
#define MVSOC_MLMB_CIB_BARRIER_TRIGGER		(0x1 << 0)

#define MVSOC_TMR_BASE		(MVSOC_MLMB_BASE + 0x0300)

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
#define MVSOC_MLMB_MLMBI_CPUTIMER2INTREQ	6
#define MVSOC_MLMB_MLMBI_CPUTIMER3INTREQ	7

#define MVSOC_MLMB_MLMBI_NIRQ			8

/*
 * PCI-Express Interface Registers
 */
#define MVSOC_PEX_BASE		(UNITID2PHYS(PEX))	/* 0x40000 */


/*
 * AXI's DDR Controller Registers
 *   used by Dove only ???
 */

/* DDR SDRAM Contriller Address Decode Registers */
#define MVSOC_AXI_NCS			2
#define MVSOC_AXI_MMAP1(cs)		(((cs) << 4) + 0x100)
#define MVSOC_AXI_MMAP1_STARTADDRESS(v)	((v) & 0xff800000)
#define MVSOC_AXI_MMAP1_AREALENGTH(v)	(0x10000 << (((v) & 0xf0000) >> 16))
#define MVSOC_AXI_MMAP1_ADDRESSMASK	(0x1ff << 7)
#define MVSOC_AXI_MMAP1_VALID		(1 << 0)

#endif	/* _MVSOCREG_H_ */
