/*	$NetBSD: openpicreg.h,v 1.6.2.1 2017/12/03 11:36:37 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_OPENPICREG_H_
#define _POWERPC_BOOKE_OPENPICREG_H_

/*
 * Common definition of VPR registers (IPIVPR, GTVPR, ...)
 */
#define VPR_MSK			0x80000000		/* Mask */
#define VPR_A			0x40000000		/* Activity */
#define VPR_P			0x00800000		/* Polatity */
#define VPR_P_HIGH		0x00800000		/* Active High */
#define VPR_S			0x00400000		/* Sense */
#define VPR_S_LEVEL		0x00400000		/* Level Sensitive */
#define VPR_PRIORITY		0x000f0000		/* Priority */
#define VPR_PRIORITY_GET(n)	(((n) >> 16) & 0x000f)
#define VPR_PRIORITY_MAKE(n)	(((n) & 0x000f) << 16)
#define VPR_VECTOR		0x0000ffff		/* Vector */
#define VPR_VECTOR_GET(n)	(((n) >>  0) & 0xffff)
#define VPR_VECTOR_MAKE(n)	(((n) & 0xffff) <<  0)

#define VPR_LEVEL_LOW		(VPR_S_LEVEL)
#define VPR_LEVEL_HIGH		(VPR_S_LEVEL | VPR_P_HIGH)

/*
 * Common definition of DR registers (IPIVPR, GTVPR, ...)
 */
#define	 DR_EP			0x80000000		/* external signal */
#define	 DR_CI(n)		(1 << (30 - (n)))	/* critical intr cpu n */
#define	 DR_P(n)		(1 << (n))		/* intr cpu n */


#define	OPENPIC_BRR1		0x0000			/* Block Revision 1 */
#define   BRR1_IPID(n)		(((n) >> 16) & 0xffff)
#define   BRR1_IPMJ(n)		(((n) >>  8) & 0x00ff)
#define   BRR1_IPMN(n)		(((n) >>  0) & 0x00ff)
#define	OPENPIC_BRR2		0x0010			/* Block Revision 2 */
#define   BRR2_IPINT0(n)	(((n) >> 16) & 0xff)
#define   BRR2_IPCFG0(n)	(((n) >>  0) & 0xff)

#define	OPENPIC_IPIDR(n)	(0x0040 + 0x10 * (n))

#define	OPENPIC_CTPR		0x0080
#define	OPENPIC_WHOAMI		0x0090
#define	OPENPIC_IACK		0x00a0
#define	OPENPIC_EOI		0x00b0

#define	OPENPIC_FRR		0x1000			/* Feature Reporting */
#define	 FRR_NIRQ_GET(n)	(((n) >> 16) & 0x7ff)	/*  intr sources - 1 */
#define	 FRR_NCPU_GET(n)	(((n) >>  8) & 0x01f)	/*  cpus - 1 */
#define	 FRR_VID_GET(n)		(((n) >>  0) & 0x0ff)	/*  version id */
#define	OPENPIC_GCR		0x1020			/* Global Configuration */
#define	 GCR_RST		0x80000000		/* Reset */
#define  GCR_M			0x20000000		/* Mixed Mode */
#define	OPENPIC_VIR		0x1080			/* Vendor Identification */
#define	OPENPIC_PIR		0x1090			/* Processor Initialization */

#define	OPENPIC_IPIVPR(n)	(0x10a0 + 0x10 * (n))
#define	OPENPIC_SVR		0x10e0
#define  SVR_VECTOR		0x0000ffff		/* Vector */
#define  SVR_VECTOR_GET(n)	(((n) >>  0) & 0xffff)
#define  SVR_VECTOR_MAKE(n)	(((n) & 0xffff) <<  0)

#define	OPENPIC_TFRR		0x10f0
#define	OPENPIC_GTCCR(cpu, n)	(0x1100 + 0x40 * (n) + 0x1000 * (cpu))
#define	 GTCCR_TOG		0x80000000
#define	 GTCCR_COUNT		0x7fffffff
#define	OPENPIC_GTBCR(cpu, n)	(0x1110 + 0x40 * (n) + 0x1000 * (cpu))
#define	 GTBCR_CI		0x80000000		/* Count Inhibit */
#define	 GTBCR_BASECNT		0x7fffffff		/* Base Count */
#define	OPENPIC_GTVPR(cpu, n)	(0x1120 + 0x40 * (n) + 0x1000 * (cpu))
#define	OPENPIC_GTDR(cpu, n)	(0x1130 + 0x40 * (n) + 0x1000 * (cpu))
#define	OPENPIC_TCR		0x1300
#define	 TCR_ROVR(n)		(1 << (24 + (n)))	/* timer n rollover */
#define	 TCR_RTM		0x00010000		/* real time source */
#define	 TCR_CLKR		0x00000300		/* clock ratio */
#define	 TCR_CLKR_64		0x00000300		/* divide by .. */
#define	 TCR_CLKR_32		0x00000200		/* divide by .. */
#define	 TCR_CLKR_16		0x00000100		/* divide by .. */
#define	 TCR_CLKR_8		0x00000000		/* divide by .. */
#define	 TCR_CASC		0x00000007		/* cascase timers */
#define	 TCR_CASC_0123		0x00000007
#define	 TCR_CASC_123		0x00000006
#define	 TCR_CASC_01_23		0x00000005
#define	 TCR_CASC_23		0x00000004
#define	 TCR_CASC_012		0x00000003
#define	 TCR_CASC_12		0x00000002
#define	 TCR_CASC_01		0x00000001
#define	 TCR_CASC_OFF		0x00000000

#define	OPENPIC_ERQSR		0x1308			/* ext. intr summary */
#define	  ERQSR_A(n)		(1 << (31 - (n)))	/* intr <n> active */
#define	OPENPIC_IRQSR0		0x1310			/* irq out summary 0 */
#define	  IRSR0_MSI_A(n)	(1 << (31 - (n)))	/* msg sig intr <n> */
#define	  IRSR0_MSG_A(n)	(1 << (20 - ((n) ^ 4))) /* shared msg intr */ 
#define	  IRSR0_EXT_A(n)	(1 << (11 - (n)))	/* ext int <n> active */
#define	OPENPIC_IRQSR1		0x1320			/* irq out summary 1 */
#define	  IRQSR1_A(n)		(1 << (31 - ((n) -  0))) /* intr <n> active */
#define	OPENPIC_IRQSR2		0x1324			/* irq out summary 2 */
#define	  IRQSR2_A(n)		(1 << (31 - ((n) - 32))) /* intr <n> active */
#define	OPENPIC_CISR0		0x1330
#define	OPENPIC_CISR1		0x1340
#define	  CISR1_A(n)		(1 << (31 - ((n) -  0))) /* intr <n> active */
#define	OPENPIC_CISR2		0x1344
#define	  CISR2_A(n)		(1 << (31 - ((n) - 32))) /* intr <n> active */

/*
 * Performance Monitor Mask Registers
 */
#define	OPENPIC_PMMR0(n)	(0x1350 + 0x20 * (n))
#define  PMMR0_MShl(n)		(1 << (31 - (n)))
#define  PMMR0_IPI(n)		(1 << (24 - (n)))
#define  PMMR0_TIMER(n)		(1 << (20 - (n)))
#define  PMMR0_MSG(n)		(1 << (16 - ((n) & 7)))
#define  PMMR0_EXT(n)		(1 << (12 - (n)))
#define	OPENPIC_PMMR1(n)	(0x1360 + 0x20 * (n))
#define	  PMMR1_INT(n)		(1 << (31 - ((n) -  0))) /* intr <n> active */
#define	OPENPIC_PMMR2(n)	(0x1364 + 0x20 * (n))
#define	  PMMR2_INT(n)		(1 << (31 - ((n) - 32))) /* intr <n> active */

/*
 * Message Registers
 */
#define	OPENPIC_MSGR(cpu, n)	(0x1400 + 0x1000 * (cpu) + 0x10 * (n))
#define	OPENPIC_MER(cpu)	(0x1500 + 0x1000 * (cpu))
#define	 MER_MSG(n)		(1 << (n))
#define	OPENPIC_MSR(cpu)	(0x1510 + 0x1000 * (cpu))
#define	 MSR_MSG(n)		(1 << (n))

#define	OPENPIC_MSIR(n)		(0x1600 + 0x10 * (n))
#define	OPENPIC_MSISR		0x1720
#define	 MSIR_SR(n)		(1 << (n))
#define	OPENPIC_MSIIR		0x1740
#define	 MSIIR_BIT(srs, ibs)	(((srs) << 29) | ((ibs) << 24))

/*
 * Interrupt Source Configuration Registers
 */
#define	OPENPIC_EIVPR(n)	(0x10000 + 0x20 * (n))
#define	OPENPIC_EIDR(n)		(0x10010 + 0x20 * (n))
#define	OPENPIC_IIVPR(n)	(0x10200 + 0x20 * (n))
#define	OPENPIC_IIDR(n)		(0x10210 + 0x20 * (n))
#define	OPENPIC_MIVPR(n)	(0x11600 + 0x20 * (n))
#define	OPENPIC_MIDR(n)		(0x11610 + 0x20 * (n))
#define	OPENPIC_MSIVPR(n)	(0x11c00 + 0x20 * (n))
#define	OPENPIC_MSIDR(n)	(0x11c10 + 0x20 * (n))

#define	MPC8536_EXTERNALSOURCES	12
#define	MPC8536_ONCHIPSOURCES	64
#define	MPC8536_ONCHIPBITMAP	{ 0xfe07ffff, 0x05501c00 }
#define	MPC8536_IPISOURCES	8
#define	MPC8536_TIMERSOURCES	8
#define	MPC8536_MISOURCES	4
#define	MPC8536_MSIGROUPSOURCES	8
#define	MPC8536_NCPUS		1
#define	MPC8536_SOURCES		/* 104 */		\
	(MPC8536_EXTERNALSOURCES			\
	 + MPC8536_ONCHIPSOURCES			\
	 + MPC8536_MSIGROUPSOURCES			\
	 + MPC8536_NCPUS*(MPC8536_IPISOURCES		\
			  + MPC8536_TIMERSOURCES	\
			  + MPC8536_MISOURCES))

#define	MPC8544_EXTERNALSOURCES	12
#define	MPC8544_ONCHIPSOURCES	48
#define	MPC8544_ONCHIPBITMAP	{ 0x3c07efff, 0x00000000 }
#define	MPC8544_IPISOURCES	4
#define	MPC8544_TIMERSOURCES	4
#define	MPC8544_MISOURCES	4
#define	MPC8544_MSIGROUPSOURCES	8
#define	MPC8544_NCPUS		1
#define	MPC8544_SOURCES		/* 80 */		\
	(MPC8544_EXTERNALSOURCES			\
	 + MPC8544_ONCHIPSOURCES			\
	 + MPC8544_MSIGROUPSOURCES			\
	 + MPC8544_NCPUS*(MPC8544_IPISOURCES		\
			  + MPC8544_TIMERSOURCES	\
			  + MPC8544_MISOURCES))

#define	MPC8548_EXTERNALSOURCES	12
#define	MPC8548_ONCHIPSOURCES	48
#define	MPC8548_ONCHIPBITMAP	{ 0x3dffffff, 0x000000f3 }
#define	MPC8548_IPISOURCES	4
#define	MPC8548_TIMERSOURCES	4
#define	MPC8548_MISOURCES	4
#define	MPC8548_MSIGROUPSOURCES	8
#define	MPC8548_NCPUS		1
#define	MPC8548_SOURCES		/* 80 */		\
	(MPC8548_EXTERNALSOURCES			\
	 + MPC8548_ONCHIPSOURCES			\
	 + MPC8548_MSIGROUPSOURCES			\
	 + MPC8548_NCPUS*(MPC8548_IPISOURCES		\
			  + MPC8548_TIMERSOURCES	\
			  + MPC8548_MISOURCES))

#define	MPC8555_EXTERNALSOURCES	12
#define	MPC8555_ONCHIPSOURCES	32
#define	MPC8555_ONCHIPBITMAP	{ 0x7d1c63ff, 0 }
#define	MPC8555_IPISOURCES	4
#define	MPC8555_TIMERSOURCES	4
#define	MPC8555_MISOURCES	4
#define	MPC8555_MSIGROUPSOURCES	0
#define	MPC8555_NCPUS		1
#define	MPC8555_SOURCES		/* 56 */		\
	(MPC8555_EXTERNALSOURCES			\
	 + MPC8555_ONCHIPSOURCES			\
	 + MPC8555_MSIGROUPSOURCES			\
	 + MPC8555_NCPUS*(MPC8555_IPISOURCES		\
			  + MPC8555_TIMERSOURCES	\
			  + MPC8555_MISOURCES))

#define	MPC8568_EXTERNALSOURCES	12
#define	MPC8568_ONCHIPSOURCES	48
#define	MPC8568_ONCHIPBITMAP	{ 0xfd1c65ff, 0x000b9e7 }
#define	MPC8568_IPISOURCES	4
#define	MPC8568_TIMERSOURCES	4
#define	MPC8568_MISOURCES	4
#define	MPC8568_MSIGROUPSOURCES	8
#define	MPC8568_NCPUS		1
#define	MPC8568_SOURCES		/* 80 */		\
	(MPC8568_EXTERNALSOURCES			\
	 + MPC8568_ONCHIPSOURCES			\
	 + MPC8568_MSIGROUPSOURCES			\
	 + MPC8568_NCPUS*(MPC8568_IPISOURCES		\
			  + MPC8568_TIMERSOURCES	\
			  + MPC8568_MISOURCES))

#define	MPC8572_EXTERNALSOURCES	12
#define	MPC8572_ONCHIPSOURCES	64
#define	MPC8572_ONCHIPBITMAP	{ 0xdeffffff, 0xf8ff93f3 }
#define	MPC8572_IPISOURCES	4
#define	MPC8572_TIMERSOURCES	4
#define	MPC8572_MISOURCES	4
#define	MPC8572_MSIGROUPSOURCES	8
#define	MPC8572_NCPUS		2
#define	MPC8572_SOURCES		/* 108 */		\
	(MPC8572_EXTERNALSOURCES			\
	 + MPC8572_ONCHIPSOURCES			\
	 + MPC8572_MSIGROUPSOURCES			\
	 + MPC8572_NCPUS*(MPC8572_IPISOURCES		\
			  + MPC8572_TIMERSOURCES	\
			  + MPC8572_MISOURCES))

#define	P1023_EXTERNALSOURCES	12
#define	P1023_ONCHIPSOURCES	64
#define	P1023_ONCHIPBITMAP	{ 0xbc07f5f9, 0xf0000e00 }
#define	P1023_IPISOURCES	4
#define	P1023_TIMERSOURCES	4/*8?*/
#define	P1023_MISOURCES		4/*8?*/
#define	P1023_MSIGROUPSOURCES	8
#define	P1023_NCPUS		2
#define	P1023_SOURCES		/* 116 */		\
	(P1023_EXTERNALSOURCES				\
	 + P1023_ONCHIPSOURCES				\
	 + P1023_MSIGROUPSOURCES			\
	 + P1023_NCPUS*(P1023_IPISOURCES		\
			  + P1023_TIMERSOURCES		\
			  + P1023_MISOURCES))
#define	P1017_NCPUS		1
#define	P1017_SOURCES					\
	(P1023_EXTERNALSOURCES				\
	 + P1023_ONCHIPSOURCES				\
	 + P1023_MSIGROUPSOURCES			\
	 + P1017_NCPUS*(P1023_IPISOURCES		\
			  + P1023_TIMERSOURCES		\
			  + P1023_MISOURCES))

#define	P1025_EXTERNALSOURCES	6
#define	P1025_ONCHIPSOURCES	64
#define	P1025_ONCHIPBITMAP	{ 0xbd1fffff, 0x01789c18 }
#define	P1025_IPISOURCES	4
#define	P1025_TIMERSOURCES	4
#define	P1025_MISOURCES		4
#define	P1025_MSIGROUPSOURCES	8
#define	P1025_NCPUS		2
#define	P1025_SOURCES		/* 102 */		\
	(P1025_EXTERNALSOURCES				\
	 + P1025_ONCHIPSOURCES				\
	 + P1025_MSIGROUPSOURCES			\
	 + P1025_NCPUS*(P1025_IPISOURCES		\
			  + P1025_TIMERSOURCES		\
			  + P1025_MISOURCES))
#define	P1016_NCPUS		1
#define	P1016_SOURCES					\
	(P1025_EXTERNALSOURCES				\
	 + P1025_ONCHIPSOURCES				\
	 + P1025_MSIGROUPSOURCES			\
	 + P1016_NCPUS*(P1025_IPISOURCES		\
			  + P1025_TIMERSOURCES		\
			  + P1025_MISOURCES))

#define	P20x0_EXTERNALSOURCES	12
#define	P20x0_ONCHIPSOURCES	64
#define	P20x0_ONCHIPBITMAP	{ 0xbd1ff7ff, 0xf17005e7 }
#define	P20x0_IPISOURCES	4
#define	P20x0_TIMERSOURCES	4
#define	P20x0_MISOURCES		4
#define	P20x0_MSIGROUPSOURCES	8
#define	P2020_NCPUS		2
#define	P2020_SOURCES		/* 108 */		\
	(P20x0_EXTERNALSOURCES				\
	 + P20x0_ONCHIPSOURCES				\
	 + P20x0_MSIGROUPSOURCES			\
	 + P2020_NCPUS*(P20x0_IPISOURCES		\
			  + P20x0_TIMERSOURCES		\
			  + P20x0_MISOURCES))
#define	P2010_NCPUS		1
#define	P2010_SOURCES					\
	(P20x0_EXTERNALSOURCES				\
	 + P20x0_ONCHIPSOURCES				\
	 + P20x0_MSIGROUPSOURCES			\
	 + P2010_NCPUS*(P20x0_IPISOURCES		\
			  + P20x0_TIMERSOURCES		\
			  + P20x0_MISOURCES))

/*
 * Per-CPU Registers
 */
#define	OPENPIC_IPIDRn(cpu, n)	(0x20040 + 0x1000 * (cpu) + 0x10 * (n))
#define	OPENPIC_CTPRn(cpu)	(0x20080 + 0x1000 * (cpu))
#define	OPENPIC_WHOAMIn(cpu)	(0x20090 + 0x1000 * (cpu))
#define	OPENPIC_IACKn(cpu)	(0x200a0 + 0x1000 * (cpu))
#define	OPENPIC_EOIn(cpu)	(0x200b0 + 0x1000 * (cpu))

#define	IRQ_SPURIOUS		0xffff

#define	ISOURCE_L2		0
#define	ISOURCE_ERROR		0
#define	ISOURCE_ECM		1
#define	ISOURCE_ETSEC1_G1_TX	1	/* P1025 */
#define	ISOURCE_DDR		2
#define	ISOURCE_ETSEC1_G1_RX	2	/* P1025 */
#define	ISOURCE_LBC		3
#define	ISOURCE_DMA_CHAN1	4
#define	ISOURCE_DMA_CHAN2	5
#define	ISOURCE_DMA_CHAN3	6
#define	ISOURCE_DMA_CHAN4	7
#define	ISOURCE_PCIEX3_MPC8572	8	/* MPC8572/P20x0/P1025 */
#define	ISOURCE_PCI1		8	/* MPC8548/MPC8544/MPC8536/MPC8555 */
#define	ISOURCE_ETSEC1_G1_ERR	8	/* P1025 */
#define	ISOURCE_FMAN		8	/* P1023 */
#define	ISOURCE_PCI2		9	/* MPC8548 */
#define	ISOURCE_PCIEX2		9	/* MPC8544/MPC8572/MPC8536/P20x0 */
#define	ISOURCE_ETSEC3_G1_TX	9	/* P1025 */
#define	ISOURCE_PCIEX		10
#define	ISOURCE_ETSEC3_G1_RX	10	/* P1025 */
#define	ISOURCE_MDIO		10	/* P1023 */
#define	ISOURCE_PCIEX3		11	/* MPC8544/MPC8536 */
#define	ISOURCE_ETSEC3_G1_ERR	11	/* P1025 */
#define	ISOURCE_USB1		12	/* MPC8536/P20x0/P1025/P1023 */
#define	ISOURCE_ETSEC1_TX	13
#define	ISOURCE_QMAN0		13	/* P1023 */
#define	ISOURCE_ETSEC1_RX	14
#define	ISOURCE_BMAN0		14	/* P1023 */
#define	ISOURCE_ETSEC3_TX	15
#define	ISOURCE_QMAN1		15	/* P1023 */
#define	ISOURCE_ETSEC3_RX	16
#define	ISOURCE_BMAN1		16	/* P1023 */
#define	ISOURCE_ETSEC3_ERR	17
#define	ISOURCE_QMAN2		17	/* P1023 */
#define	ISOURCE_ETSEC1_ERR	18
#define	ISOURCE_BMAN2		18	/* P1023 */
#define	ISOURCE_ETSEC2_TX	19	/* !MPC8544/!MPC8536/!P1025 */
#define	ISOURCE_ETSEC2_RX	20	/* !MPC8544/!MPC8536/!P1025 */
#define	ISOURCE_ETSEC4_TX	21	/* !MPC8544/!MPC8536/!P20x0/!P1025 */
#define	ISOURCE_ETSEC4_RX	22	/* !MPC8544/!MPC8536/!P20x0/!P1025 */
#define	ISOURCE_ETSEC4_ERR	23	/* !MPC8544/!MPC8536/!P20x0/!P1025 */
#define	ISOURCE_ETSEC2_ERR	24	/* !MPC8544/!MPC8536 */
#define	ISOURCE_FEC		25	/* MPC8572 */
#define	ISOURCE_SATA2		25	/* MPC8536 */
#define	ISOURCE_DUART		26
#define	ISOURCE_I2C		27
#define	ISOURCE_PERFMON		28
#define	ISOURCE_SECURITY1	29
#define	ISOURCE_CPM		30	/* MPC8555 */
#define	ISOURCE_QEB_LOW		30	/* MPC8568 */
#define	ISOURCE_USB2		30	/* MPC8536 */
#define	ISOURCE_GPIO		31	/* MPC8572/!MPC8548 */
#define	ISOURCE_QEB_PORT	31	/* MPC8568/P1025 */
#define	ISOURCE_SRIO_EWPU	32	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_SRIO_ODBELL	33	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_SRIO_IDBELL	34	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_ETSEC2_G1_TX	35	/* P1025 */
#define	ISOURCE_ETSEC2_G1_RX	36	/* P1025 */
#define	ISOURCE_SRIO_OMU1	37	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_SRIO_IMU1	38	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_SRIO_OMU2	39	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_SRIO_IMU2	40	/* !MPC8548&!P20x0&!P1025 */
#define	ISOURCE_PME_GENERAL	41	/* MPC8572 */
#define	ISOURCE_SECURITY2_P1023	41	/* P1023 */
#define	ISOURCE_SECURITY2	42	/* MPC8572|MPC8536|P20x0|P1025 */
#define	ISOURCE_SEC_GENERAL	42	/* P1023 */
#define	ISOURCE_SPI		43	/* MPC8536|P20x0|P1025 */
#define	ISOURCE_QEB_IECC	43	/* MPC8568 */
#define	ISOURCE_USB3		44	/* MPC8536 */
#define	ISOURCE_QEB_MUECC	44	/* MPC8568|P1025 */
#define	ISOURCE_TLU1		45	/* MPC8568/MPC8572 */
#define	ISOURCE_46		46
#define	ISOURCE_QEB_HIGH	47	/* MPC8548|P1025 */
#define	ISOURCE_PME_CHAN1	48	/* MPC8572 */
#define	ISOURCE_PME_CHAN2	49	/* MPC8572 */
#define	ISOURCE_PME_CHAN3	50	/* MPC8572 */
#define	ISOURCE_PME_CHAN4	51	/* MPC8572 */
#define	ISOURCE_ETSEC2_G1_ERR	51	/* P1025 */
#define	ISOURCE_ETSEC1_PTP	52	/* MPC8572|MPC8536|P20x0|P1025 */
#define	ISOURCE_ETSEC2_PTP	53	/* MPC8572|P20x0|P1025 */
#define	ISOURCE_ETSEC3_PTP	54	/* MPC8572|MPC8536|P20x0|P1025 */
#define	ISOURCE_ETSEC4_PTP	55	/* MPC8572 */
#define	ISOURCE_ESDHC		56	/* MPC8536|P20x0|P1025 */
#define	ISOURCE_57		57
#define	ISOURCE_SATA1		58	/* MPC8536 */
#define	ISOURCE_TLU2		59	/* MPC8572 */
#define	ISOURCE_DMA2_CHAN1	60	/* MPC8572|P20x0|P1023 */
#define	ISOURCE_DMA2_CHAN2	61	/* MPC8572|P20x0|P1023 */
#define	ISOURCE_DMA2_CHAN3	62	/* MPC8572|P20x0|P1023 */
#define	ISOURCE_DMA2_CHAN4	63	/* MPC8572|P20x0|P1023 */

#endif /* _POWERPC_BOOKE_OPENPICREG_H_ */
