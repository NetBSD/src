/*	$NetBSD: gic_reg.h,v 1.3.4.2 2015/09/22 12:05:37 skrll Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

/*
 * ARM Generic Interrupt Controller Register Definitions
 *
 * These registers are accessible through a dedicated internal bus.
 * All accesses must be done in a little-endian manner.
 * The base address of the pages containing these registers is defined
 * by the pins PERIPHBASE[31:13] which can be obtained by doing a
 *	MRC p15,4,<Rd>,c15,c0,0; Read Configuration Base Address Register
 *	(except cortex-A9 uniprocessor)
 *
 * GIC is used by all Cortex-A cores except the A8.
 */

#ifndef _ARM_CORTEX_GICREG_H_
#define	_ARM_CORTEX_GICREG_H_

#define	GICC_BASE	0x0100	// Offset in PeriphBase

#define	GICC_CTRL	0x0000	// CPU Interface Control Register
#define	GICC_PMR	0x0004	// Interrupt Priority Mask Register
#define	GICC_BPR	0x0008	// Binary Point Register
#define	GICC_IAR	0x000C	// Interrupt Acknowledge Register
#define	GICC_EOIR	0x0010	// End Of Interrupt Register (WO)
#define	GICC_RPR	0x0014	// Running Priority Register
#define	GICC_HPPIR	0x0018	// Highest Priority Pending Interrupt Registers
#define	GICC_ABPR	0x001C	// Aliased Binary Point Register
#define	GICC_AIAR	0x0020	// Aliased Interrupt Acknowledge Register
#define	GICC_AEOIR	0x0024	// Aliased End Of Interrupt Register
#define	GICC_AHPPIR	0x0028	// Aliased Highest Priority Pending Interrupt Registers
#define	GICC_APR0	0x00D0	// Active Priorites Register 0
#define	GICC_APR1	0x00D4	// Active Priorites Register 1
#define	GICC_APR2	0x00D8	// Active Priorites Register 2
#define	GICC_APR3	0x00DC	// Active Priorites Register 3
#define	GICC_NSAPR0	0x00E0	// Non-secure Active Priorities Register 0
#define	GICC_NSAPR1	0x00E4	// Non-secure Active Priorities Register 1
#define	GICC_NSAPR2	0x00E8	// Non-secure Active Priorities Register 2
#define	GICC_NSAPR3	0x00EC	// Non-secure Active Priorities Register 3
#define	GICC_IIDR	0x00FC	// CPU Interface Identification Register
#define	GICC_DIR	0x1000	// Deactivate Interrupt Register


#define	GICC_CTRL_V1_Enable		__BIT(0) // GICv1

#define	GICC_CTRL_V2_EnableGrp0		__BIT(0) // GICv2 !Secure | Secure
#define	GICC_CTRL_V2_EnableGrp1		__BIT(1) // GICv2 !Secure | Secure

#define	GICC_CTRL_V2S_EOImodeNS		__BIT(9) // GICv2 Secure
#define	GICC_CTRL_V2S_IRQBypDisGrp1	__BIT(6) // GICv2 Secure
#define	GICC_CTRL_V2S_FIQBypDisGrp1	__BIT(4) // GICv2 Secure

#define	GICC_CTRL_V2NS_EOImodeNS	__BIT(10) // GICv2 !Secure
#define	GICC_CTRL_V2NS_EOImodeS		__BIT(9) // GICv2 !Secure
#define	GICC_CTRL_V2NS_IRQBypDisGrp1	__BIT(8) // GICv2 !Secure
#define	GICC_CTRL_V2NS_FIQBypDisGrp1	__BIT(7) // GICv2 !Secure
#define	GICC_CTRL_V2NS_IRQBypDisGrp0	__BIT(6) // GICv2 !Secure
#define	GICC_CTRL_V2NS_FIQBypDisGrp0	__BIT(5) // GICv2 !Secure
#define	GICC_CTRL_V2NS_CPBR		__BIT(4) // GICv2 !Secure
#define	GICC_CTRL_V2NS_FIQEn		__BIT(3) // GICv2 !Secure
#define	GICC_CTRL_V2NS_AckCtl		__BIT(2) // GICv2 !Secure

#define	GICC_PMR_PRIORITY		__BITS(7,0)
#define	GICC_PMR_PRIORITIES		256
#define	GICC_PMR_NS_PRIORITIES		128
#define	GICC_PMR_NONSECURE		0x80
#define	GICC_PMR_8_LEVELS		0x1f
#define	GICC_PMR_16_LEVELS		0x0f
#define	GICC_PMR_32_LEVELS		0x07
#define	GICC_PMR_64_LEVELS		0x03
#define	GICC_PMR_128_LEVELS		0x01

#define	GICC_IAR_CPUID			__BITS(12,10)
#define	GICC_IAR_IRQ			__BITS(9,0)
#define	GICC_IAR_IRQ_SPURIOUS		1023
#define	GICC_IAR_IRQ_SSPURIOUS		1022	// Secure

#define	GICC_EOIR_CPUID			__BITS(12,10)
#define	GICC_EOIR_InterruptID		__BITS(9,0)

#define	GICC_HPPIR_CPUID		__BITS(12,10)
#define	GICC_HPPIR_PendIntID		__BITS(9,0)

#define	GICC_IIDR_ProductID		__BITS(31,20)
#define	GICC_IIDR_ArchVersion		__BITS(19,16)
#define	GICC_IIDR_Revision		__BITS(15,12)
#define	GICC_IIDR_Implementer		__BITS(11,0)

#define	GICC_DIR_CPUID			__BITS(12,10)
#define	GICC_DIR_InterruptID		__BITS(9,0)

#define	GICD_BASE		0x1000 // Offset from PeriphBase

#define	GICD_CTRL		0x000 // Distributor Control Register
#define	GICD_TYPER		0x004 // Interrupt Controller Type Register
#define	GICD_IIDR		0x008 // Distributor Implementer Identification Register
#define	GICD_IGROUPRn(n)	(0x080+4*(n)) // Interrupt Group Registers
#define	GICD_ISENABLERn(n)	(0x100+4*(n)) // Interrupt Set-Enable Registers
#define	GICD_ICENABLERn(n)	(0x180+4*(n)) // Interrupt Clear-Enable Registers
#define	GICD_ISPENDRn(n)	(0x200+4*(n)) // Interrupt Set-Pending Registers
#define	GICD_ICPENDRn(n)	(0x280+4*(n)) // Interrupt Clear-Pending Registers
#define	GICD_ISACTIVERn(n)	(0x300+4*(n)) // GICv2 Interrupt Set-Active Registers
#define	GICD_ICACTIVERn(n)	(0x380+4*(n)) // Interrupt Clear-Active Registers
#define	GICD_IPRIORITYRn(n)	(0x400+4*(n)) // Interrupt Priority Registers

#define	GICD_ITARGETSRn(n)	(0x800+4*(n)) // Interrupt Processor Targets Registers
#define	GICD_ICFGRn(n)		(0xC00+4*(n)) // Interrupt Configuration Registers
#define	GICD_NSACRn(n)		(0xE00+4*(n)) // Non-secure Access Control Registers, optional
#define	GICD_SGIR		0xF00 // Software Generated Interrupt Register
#define	GICD_CPENDSGIR(n)	(0xF10+4*(n)) // SGI Clear-Pending Registers
#define	GICD_SPENDSGIR(n)	(0xF20+4*(n)) // SGI Set-Pending Registers

#define	GICD_CTRL_Enable		__BIT(0)

#define	GICD_TYPER_LSPI			__BITS(15,11)
#define	GICD_TYPER_SecurityExtn		__BIT(10)
#define	GICD_TYPER_CPUNumber		__BITS(7,5)
#define	GICD_TYPER_ITLinesNumber	__BITS(4,0)	// 32*(N+1)
#define	GICD_TYPER_LINES(n)		MIN(32*(__SHIFTOUT((n), GICD_TYPER_ITLinesNumber) + 1), 1020)

#define	GICD_IIDR_ProductId		__BITS(31,24)
#define	GICD_IIDR_Variant		__BITS(19,16)
#define	GICD_IIDR_Revision		__BITS(15,12)
#define	GICD_IIDR_Implementer		__BITS(11,0)

/*
 * This register is byte-accessible but always little-endian.
 */
#define	GICD_IPRIORITYR_Byte3		__BITS(31,24)
#define	GICD_IPRIORITYR_Byte1		__BITS(23,16)
#define	GICD_IPRIORITYR_Byte2		__BITS(15,8)
#define	GICD_IPRIORITYR_Byte0		__BITS(7,0)

/*
 * This register is byte-accessible but always little-endian.
 */
#define	GICD_ITARGETSR_Byte3		__BITS(31,24)
#define	GICD_ITARGETSR_Byte1		__BITS(23,16)
#define	GICD_ITARGETSR_Byte2		__BITS(15,8)
#define	GICD_ITARGETSR_Byte0		__BITS(7,0)

#define	GICD_SGIR_TargetListFilter	__BITS(25,24)
#define	GICD_SGIR_TargetListFilter_List	__SHIFTIN(0, GICD_SGIR_TargetListFilter)
#define	GICD_SGIR_TargetListFilter_NotMe __SHIFTIN(1, GICD_SGIR_TargetListFilter)
#define	GICD_SGIR_TargetListFilter_Me	__SHIFTIN(2, GICD_SGIR_TargetListFilter)
#define	GICD_SGIR_TargetList		__BITS(23,16)
#define	GICD_SGIR_NSATT			__BIT(15)
#define	GICD_SGIR_SGIINTID		__BITS(3,0)

/*
 * GICv1 names
 */
#define	GICv1_ICDDCR		GICD_CTLR
#define	GICv1_ICDICTR		GICD_TYPER
#define	GICv1_ICDIIDR		GICD_IIDR
#define	GICv1_ICDISRn(n)	GICD_IGROUPRn(n)
#define	GICv1_ICDABRn(n)	GICD_ISACTIVERn(n)
#define	GICv1_ICDISERn(n)	GICD_ISENABLERn(n)
#define	GICv1_ICDICERn(n)	GICD_ICENABLERn(n)
#define	GICv1_ICDISPRn(n)	GICD_ISPENDRn(n)
#define	GICv1_ICDICPRn(n)	GICD_ICPENDRn(n)
#define	GICv1_ICDIPRn(n)	GICD_IPRIORITYRn(n)
#define	GICv1_ICDIPTRn(n)	GICD_ITARGETSRn(n)
#define	GICv1_ICDICRn(n)	GICD_ICFGRn(n)
#define	GICv1_ICDSGIR		GICD_SGIR

#define	GICv1_ICCICR		GICC_CTLR
#define	GICv1_ICCPMR		GICC_PMR
#define	GICv1_ICCBPR		GICC_BPR
#define	GICv1_ICCIAR		GICC_IAR
#define	GICv1_ICCEOIR		GICC_EOIR
#define	GICv1_ICCRPR		GICC_RPR
#define	GICv1_ICCABPR		GICC_ABPR
#define	GICv1_ICCHPIR		GICC_HPPIR
#define	GICv1_ICCIIDR		GICC_IIDR

/* GICv2m (MSI) */

#define GIC_MSI_TYPER		0x0008
#define GIC_MSI_SETSPI		0x0040
#define GIC_MSI_PIDR2		0x0fe8
#define GIC_MSI_IIDR		0x0ffc

#define GIC_MSI_TYPER_BASE	__BITS(25,16)	// Starting SPI of MSIs
#define GIC_MSI_TYPER_NUMBER	__BITS(9,0)	// Count of MSIs

#endif /* !_ARM_CORTEX_GICREG_H_ */
