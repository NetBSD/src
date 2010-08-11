/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#define	 FRR_NIRQ(n)		(((n) >> 16) & 0x7ff)	/*  intr sources - 1 */
#define	 FRR_NCPU(n)		(((n) >>  8) & 0x01f)	/*  cpus - 1 */
#define	 FRR_VID(n)		(((n) >>  0) & 0x0ff)	/*  version id */
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
#define	OPENPIC_GTCCR(n)	(0x1100 + 0x40 * (n))
#define	 GTCCR_TOG		0x80000000
#define	 GTCCR_COUNT		0x7fffffff
#define	OPENPIC_GTBCR(n)	(0x1110 + 0x40 * (n))
#define	 GTBCR_CI		0x80000000		/* Count Inhibit */
#define	 GTBCR_BASECNT		0x7fffffff		/* Base Count */
#define	OPENPIC_GTVPR(n)	(0x1120 + 0x40 * (n))
#define	OPENPIC_GTDR(n)		(0x1130 + 0x40 * (n))
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
#define	OPENPIC_MER(cpu		(0x1500 + 0x1000 * (cpu))
#defien	 MER_MSG(n)		(1 << (n))
#define	OPENPIC_MSR(cpu)	(0x1510 + 0x1000 * (cpu))
#defien	 MSR_MSG(n)		(1 << (n))

#define	OPENPIC_MSIR(n)		(0x1600 + 0x10 * (n))
#define	OPENPIC_MSISR		0x1720
#defien	 MSIR_SR(n)		(1 << (n))
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

#define	MPC8548_EXTERNALSOURCES	12
#define	MPC8548_INTERNALSOURCES	48
#define	MPC8548_INTERNALBITMAP	{ 0x9effffff, 0x000000f3 }
#define	MPC8548_IPISOURCES	4
#define	MPC8548_TIMERSOURCES	4
#define	MPC8548_MISOURCES	4
#define	MPC8548_MSISOURCES	8
#define	MPC8548_NCPUS		1
#define	MPC8548_SOURCES					\
	(MPC8548_EXTERNALSOURCES			\
	 + MPC8548_INTERNALSOURCES			\
	 + MPC8548_MSISOURCES				\
	 + MPC8548_NCPUS*(MPC8548_IPISOURCES		\
			  + MPC8548_TIMERSOURCES	\
			  + MPC8548_MISOURCES))

#define	MPC8572_EXTERNALSOURCES	12
#define	MPC8572_INTERNALSOURCES	64
#define	MPC8572_INTERNALBITMAP	{ 0xdeffffff, 0xf8ff93f3 }
#define	MPC8572_IPISOURCES	4
#define	MPC8572_TIMERSOURCES	4
#define	MPC8572_MISOURCES	4
#define	MPC8572_MSISOURCES	8
#define	MPC8572_SOURCES					\
	(MPC8572_EXTERNALSOURCES			\
	 + MPC8572_INTERNALSOURCES			\
	 + MPC8572_MSISOURCES				\
	 + MPC8572_NCPUS*(MPC8572_IPISOURCES		\
			  + MPC8572_TIMERSOURCES	\
			  + MPC8572_MISOURCES))

/*
 * Per-CPU Registers
 */
#define	OPENPIC_IPIDRn(cpu, n)	(0x20040 + 0x1000 * (cpu) + 0x10 * (n))
#define	OPENPIC_CTPRn(cpu)	(0x20080 + 0x1000 * (cpu))
#define	OPENPIC_WHOAMIn(cpu)	(0x20090 + 0x1000 * (cpu))
#define	OPENPIC_IACKn(cpu)	(0x200a0 + 0x1000 * (cpu))
#define	OPENPIC_EOIn(cpu)	(0x200b0 + 0x1000 * (cpu))

#define	ISOURCE_L2		0
#define	ISOURCE_ECM		1
#define	ISOURCE_DRAM		2
#define	ISOURCE_LBC		3
#define	ISOURCE_DMA_CHAN1	4
#define	ISOURCE_DMA_CHAN2	5
#define	ISOURCE_DMA_CHAN3	6
#define	ISOURCE_DMA_CHAN4	7
#define	ISOURCE_PCIEX_P3	8	/* MPC8572 */
#define	ISOURCE_PCIEX_P2	9	/* MPC8572 */
#define	ISOURCE_PCI1		8	/* MPC8548 */
#define	ISOURCE_PCI2		9	/* MPC8548 */
#define	ISOURCE_PCIEX		10
#define	ISOURCE_11		11
#define	ISOURCE_12		12
#define	ISOURCE_ETSEC1_TX	13
#define	ISOURCE_ETSEC1_RX	14
#define	ISOURCE_ETSEC3_TX	15
#define	ISOURCE_ETSEC3_RX	16
#define	ISOURCE_ETSEC3_ERR	17
#define	ISOURCE_ETSEC1_ERR	18
#define	ISOURCE_ETSEC2_TX	19
#define	ISOURCE_ETSEC2_RX	20
#define	ISOURCE_ETSEC4_TX	21
#define	ISOURCE_ETSEC4_RX	22
#define	ISOURCE_ETSEC4_ERR	23
#define	ISOURCE_ETSEC2_ERR	24
#define	ISOURCE_FEC		25	/* MPC8572 */
#define	ISOURCE_DUART		26
#define	ISOURCE_I2C		27
#define	ISOURCE_PERFMON		28
#define	ISOURCE_SECURITY1	29
#define	ISOURCE_30		30
#define	ISOURCE_MPC8572_GPIO	31	/* MPC8572 */
#define	ISOURCE_SRIO_EWPU	32
#define	ISOURCE_SRIO_ODBELL	33
#define	ISOURCE_SRIO_IDBELL	34
#define	ISOURCE_35		35
#define	ISOURCE_36		36
#define	ISOURCE_SRIO_OMU1	37
#define	ISOURCE_SRIO_IMU1	38
#define	ISOURCE_SRIO_OMU2	39
#define	ISOURCE_SRIO_IMU2	40
#define	ISOURCE_PME_GENERAL	41	/* MPC8572 */
#define	ISOURCE_SECURITY2	42	/* MPC8572 */
#define	ISOURCE_43		43
#define	ISOURCE_44		44
#define	ISOURCE_TLU1		45	/* MPC8572 */
#define	ISOURCE_46		46
#define	ISOURCE_47		47
#define	ISOURCE_PME_CHAN1	48	/* MPC8572 */
#define	ISOURCE_PME_CHAN2	49	/* MPC8572 */
#define	ISOURCE_PME_CHAN3	50	/* MPC8572 */
#define	ISOURCE_PME_CHAN4	51	/* MPC8572 */
#define	ISOURCE_ETSEC1_PTP	52	/* MPC8572 */
#define	ISOURCE_ETSEC2_PTP	53	/* MPC8572 */
#define	ISOURCE_ETSEC3_PTP	54	/* MPC8572 */
#define	ISOURCE_ETSEC4_PTP	55	/* MPC8572 */
#define	ISOURCE_56		56
#define	ISOURCE_57		57
#define	ISOURCE_58		58
#define	ISOURCE_TLU2		59	/* MPC8572 */
#define	ISOURCE_DMA2_CHAN1	60	/* MPC8572 */
#define	ISOURCE_DMA2_CHAN2	61	/* MPC8572 */
#define	ISOURCE_DMA2_CHAN3	62	/* MPC8572 */
#define	ISOURCE_DMA2_CHAN4	63	/* MPC8572 */

#endif /* _POWERPC_BOOKE_OPENPICREG_H_ */
