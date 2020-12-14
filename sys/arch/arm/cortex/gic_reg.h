/*	$NetBSD: gic_reg.h,v 1.11.10.1 2020/12/14 14:37:48 thorpej Exp $	*/
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
#define	GICD_IGRPMODRn(n)	(0xD00+4*(n)) // Interrupt Group Modifier Registers
#define	GICD_NSACRn(n)		(0xE00+4*(n)) // Non-secure Access Control Registers, optional
#define	GICD_SGIR		0xF00 // Software Generated Interrupt Register
#define	GICD_CPENDSGIR(n)	(0xF10+4*(n)) // SGI Clear-Pending Registers
#define	GICD_SPENDSGIR(n)	(0xF20+4*(n)) // SGI Set-Pending Registers
#define	GICD_IROUTER(n)		(0x6000+8*(n)) // Interrupt Routing Registers

#define	GICD_CTRL_RWP			__BIT(31)	// GICv3
#define	GICD_CTRL_E1NWF			__BIT(7)	// GICv3
#define	GICD_CTRL_DS			__BIT(6)	// GICv3
#define	GICD_CTRL_ARE_NS		__BIT(4)	// GICv3
#define	GICD_CTRL_EnableGrp1S		__BIT(2)	// Secure only
#define	GICD_CTRL_EnableGrp1A		__BIT(1)	// GICv3
#define	GICD_CTRL_Enable		__BIT(0)

#define	GICD_TYPER_No1N			__BIT(25)	// GICv3
#define	GICD_TYPER_A3V			__BIT(24)	// GICv3
#define	GICD_TYPER_IDbits		__BITS(23,19)	// GICv3
#define	GICD_TYPER_DVIS			__BIT(18)	// GICv3
#define	GICD_TYPER_LPIS			__BIT(17)	// GICv3
#define	GICD_TYPER_MBIS			__BIT(16)	// GICv3
#define	GICD_TYPER_LSPI			__BITS(15,11)
#define	GICD_TYPER_SecurityExtn		__BIT(10)
#define	GICD_TYPER_CPUNumber		__BITS(7,5)
#define	GICD_TYPER_ITLinesNumber	__BITS(4,0)	// 32*(N+1)
#define	GICD_TYPER_LINES(n)		MIN(32*(__SHIFTOUT((n), GICD_TYPER_ITLinesNumber) + 1), 1020)

#define	GICD_IIDR_ProductID		__BITS(31,24)
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

#define	GICD_IROUTER_Aff3		__BITS(39,32)
#define	GICD_IROUTER_Interrupt_Routing_mode __BIT(31)
#define	GICD_IROUTER_Aff2		__BITS(23,16)
#define	GICD_IROUTER_Aff1		__BITS(15,8)
#define	GICD_IROUTER_Aff0		__BITS(7,0)

#define	GICR_CTLR		0x0000	// Redistributor Control Register
#define	GICR_IIDR		0x0004	// Implementor Identification Register
#define	GICR_TYPER		0x0008	// Redistributor Type Register
#define	GICR_STATUSR		0x0010	// Error Reporting Status Register, optional
#define	GICR_WAKER		0x0014	// Redistributor Wake Register
#define	GICR_SETLPIR		0x0040	// Set LPI Pending Register
#define	GICR_CLRLPIR		0x0048	// Clear LPI Pending Register
#define	GICR_PROPBASER		0x0070	// Redistributor Properties Base Address Register
#define	GICR_PENDBASER		0x0078	// Redistributor LPI Pending Table Base Address Register
#define	GICR_INVLPIR		0x00A0	// Redistributor Invalidate LPI Register
#define	GICR_INVALLR		0x00B0	// Redistributor Invalidate All Register
#define	GICR_SYNCR		0x00C0	// Redistributor Synchronize Register

#define	GICR_IGROUPR0		0x10080	// Interrupt Group Register 0
#define	GICR_ISENABLER0		0x10100	// Interrupt Set-Enable Register 0
#define	GICR_ICENABLER0		0x10180	// Interrupt Clear-Enable Register 0
#define	GICR_ISPENDR0		0x10200	// Interrupt Set-Pend Register 0
#define	GICR_ICPENDR0		0x10280	// Interrupt Clear-Pend Register 0
#define	GICR_ISACTIVER0		0x10300	// Interrupt Set-Active Register 0
#define	GICR_ICACTIVER0		0x10380	// Interrupt Clear-Active Register 0
#define	GICR_IPRIORITYRn(n)	(0x10400+4*(n)) // Interrupt Priority Registers
#define	GICR_ICFGRn(n)		(0x10C00+4*(n)) // SGI (0) / PPI (1) Configuration Register
#define	GICR_IGRPMODR0		0x10D00	// Interrupt Group Modifier Register 0
#define	GICR_NSACR		0x10E00	// Non-Secure Access Control Register

#define	GICR_CTLR_UWP			__BIT(31)
#define	GICR_CTLR_DPG1S			__BIT(26)
#define	GICR_CTLR_DPG1NS		__BIT(25)
#define	GICR_CTLR_DPG0			__BIT(24)
#define	GICR_CTLR_RWP			__BIT(3)
#define	GICR_CTLR_Enable_LPIs		__BIT(0)

#define	GICR_TYPER_Affinity_Value	__BITS(63,32)
#define	GICR_TYPER_Affinity_Value_Aff3	__BITS(63,56)
#define	GICR_TYPER_Affinity_Value_Aff2	__BITS(55,48)
#define	GICR_TYPER_Affinity_Value_Aff1	__BITS(47,40)
#define	GICR_TYPER_Affinity_Value_Aff0	__BITS(39,32)
#define	GICR_TYPER_CommonLPIAff		__BITS(25,24)
#define	GICR_TYPER_Processor_Number	__BITS(23,8)
#define	GICR_TYPER_DPGS			__BIT(5)
#define	GICR_TYPER_Last			__BIT(4)
#define	GICR_TYPER_DirectLPI		__BIT(3)
#define	GICR_TYPER_VLPIS		__BIT(1)
#define	GICR_TYPER_PLPIS		__BIT(0)

#define	GICR_WAKER_ChildrenAsleep	__BIT(2)
#define	GICR_WAKER_ProcessorSleep	__BIT(1)

#define	GICR_PROPBASER_OuterCache	__BITS(58,56)
#define	GICR_PROPBASER_Physical_Address	__BITS(51,12)
#define	GICR_PROPBASER_Shareability	__BITS(11,10)
#define	GICR_PROPBASER_InnerCache	__BITS(9,7)
#define	GICR_PROPBASER_IDbits		__BITS(4,0)

#define	GICR_PENDBASER_PTZ		__BIT(62)
#define	GICR_PENDBASER_OuterCache	__BITS(58,56)
#define	GICR_PENDBASER_Physical_Address	__BITS(51,16)
#define	GICR_PENDBASER_Shareability	__BITS(11,10)
#define	GICR_PENDBASER_InnerCache	__BITS(9,7)

#define	GICR_Shareability_NS		0	// Non-shareable
#define	GICR_Shareability_IS		1	// Inner Shareable
#define	GICR_Shareability_OS		2	// Outer Shareable

#define	GICR_Cache_DEVICE_nGnRnE	0	// Device-nGnRnE
#define	GICR_Cache_NORMAL_NC		1	// Non-cacheable
#define	GICR_Cache_NORMAL_RA_WT		2	// Cacheable Read-allocate, Write-through
#define	GICR_Cache_NORMAL_RA_WB		3	// Cacheable Read-allocate, Write-back
#define	GICR_Cache_NORMAL_WA_WT		4	// Cacheable Write-allocate, Write-through
#define	GICR_Cache_NORMAL_WA_WB		5	// Cacheable Write-allocate, Write-back
#define	GICR_Cache_NORMAL_RA_WA_WT	6	// Cacheable Read-allocate, Write-allocate, Write-through
#define	GICR_Cache_NORMAL_RA_WA_WB	7	// Cacheable Read-allocate, Write-allocate, Write-back

/*
 * GICv3 Locality-specific Peripheral Interrupts
 */

#define	GIC_LPI_BASE			0x2000	// Base LPI INTID

#define	GIC_LPICONF_Priority		__BITS(7,2)
#define	GIC_LPICONF_Res1		__BIT(1)
#define	GIC_LPICONF_Enable		__BIT(0)

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

/*
 * GICv2m (MSI)
 */

#define GIC_MSI_TYPER		0x0008
#define GIC_MSI_SETSPI		0x0040
#define GIC_MSI_PIDR2		0x0fe8
#define GIC_MSI_IIDR		0x0ffc

#define GIC_MSI_TYPER_BASE	__BITS(25,16)	// Starting SPI of MSIs
#define GIC_MSI_TYPER_NUMBER	__BITS(9,0)	// Count of MSIs

/*
 * GICv3 Interrupt Translation Service (ITS)
 */

#define	GITS_CTLR		0x00000		// ITS control register
#define	GITS_IIDR		0x00004		// ITS Identification register
#define	GITS_TYPER		0x00008		// ITS Type register
#define	GITS_CBASER		0x00080		// ITS Command Queue Descriptor
#define	GITS_CWRITER		0x00088		// ITS Write register
#define	GITS_CREADR		0x00090		// ITS Read register
#define	GITS_BASERn(n)		(0x00100+8*(n))	// ITS Translation Table Descriptors
#define	GITS_PIDR2		0x0FFE8		// ITS Peripheral ID2 Register
#define	GITS_TRANSLATER		0x10040		// ITS Translation register

#define	GITS_CTLR_Quiescent		__BIT(31)
#define	GITS_CTLR_ITS_Number		__BITS(7,4)
#define	GITS_CTLR_ImDe			__BIT(1)
#define	GITS_CTLR_Enabled		__BIT(0)

#define	GITS_IIDR_ProductID		__BITS(31,24)
#define	GITS_IIDR_Variant		__BITS(19,16)
#define	GITS_IIDR_Revision		__BITS(15,12)
#define	GITS_IIDR_Implementor		__BITS(11,0)

#define	GITS_TYPER_VMOVP		__BIT(37)
#define	GITS_TYPER_CIL			__BIT(36)
#define	GITS_TYPER_CIDbits		__BITS(35,32)
#define	GITS_TYPER_HCC			__BITS(31,24)
#define	GITS_TYPER_PTA			__BIT(19)
#define	GITS_TYPER_SEIS			__BIT(18)
#define	GITS_TYPER_Devbits		__BITS(17,13)
#define	GITS_TYPER_ID_bits		__BITS(12,8)
#define	GITS_TYPER_ITT_entry_size	__BITS(7,4)
#define	GITS_TYPER_CCT			__BIT(2)
#define	GITS_TYPER_Virtual		__BIT(1)
#define	GITS_TYPER_Physical		__BIT(0)

#define	GITS_CBASER_Valid		__BIT(63)
#define	GITS_CBASER_InnerCache		__BITS(61,59)
#define	GITS_CBASER_OuterCache		__BITS(55,53)
#define	GITS_CBASER_Physical_Address	__BITS(51,12)
#define	GITS_CBASER_Shareability	__BITS(11,10)
#define	GITS_CBASER_Size		__BITS(7,0)

#define	GITS_CWRITER_Offset		__BITS(19,5)
#define	GITS_CWRITER_Retry		__BIT(0)

#define	GITS_CREADR_Offset		__BITS(19,5)
#define	GITS_CREADR_Stalled		__BIT(0)

#define	GITS_BASER_Valid		__BIT(63)
#define	GITS_BASER_Indirect		__BIT(62)
#define	GITS_BASER_InnerCache		__BITS(61,59)
#define	GITS_BASER_Type			__BITS(58,56)
#define	GITS_BASER_OuterCache		__BITS(55,53)
#define	GITS_BASER_Entry_Size		__BITS(52,48)
#define	GITS_BASER_Physical_Address	__BITS(47,12)
#define	GITS_BASER_Shareability		__BITS(11,10)
#define	GITS_BASER_Page_Size		__BITS(9,8)
#define	GITS_BASER_Size			__BITS(7,0)

#define	GITS_Shareability_NS		0	// Non-shareable
#define	GITS_Shareability_IS		1	// Inner Shareable
#define	GITS_Shareability_OS		2	// Outer Shareable

#define	GITS_Cache_DEVICE_nGnRnE	0	// Device-nGnRnE
#define	GITS_Cache_NORMAL_NC		1	// Non-cacheable
#define	GITS_Cache_NORMAL_RA_WT		2	// Cacheable Read-allocate, Write-through
#define	GITS_Cache_NORMAL_RA_WB		3	// Cacheable Read-allocate, Write-back
#define	GITS_Cache_NORMAL_WA_WT		4	// Cacheable Write-allocate, Write-through
#define	GITS_Cache_NORMAL_WA_WB		5	// Cacheable Write-allocate, Write-back
#define	GITS_Cache_NORMAL_RA_WA_WT	6	// Cacheable Read-allocate, Write-allocate, Write-through
#define	GITS_Cache_NORMAL_RA_WA_WB	7	// Cacheable Read-allocate, Write-allocate, Write-back

#define	GITS_Type_Unimplemented		0	// Unimplemented
#define	GITS_Type_Devices		1	// Devices table
#define	GITS_Type_vPEs			2	// vPEs table
#define	GITS_Type_InterruptCollections	4	// Interrupt collections table

#define	GITS_Page_Size_4KB		0
#define	GITS_Page_Size_16KB		1
#define	GITS_Page_Size_64KB		2

struct gicv3_its_command {
	uint64_t	dw[4];
};

#define	GITS_CMD_MOVI			0x01
#define	GITS_CMD_INT			0x03
#define	GITS_CMD_CLEAR			0x04
#define	GITS_CMD_SYNC			0x05
#define	GITS_CMD_MAPD			0x08
#define	GITS_CMD_MAPC			0x09
#define	GITS_CMD_MAPTI			0x0A
#define	GITS_CMD_MAPI			0x0B
#define	GITS_CMD_INV			0x0C
#define	GITS_CMD_INVALL			0x0D
#define	GITS_CMD_MOVALL			0x0E
#define	GITS_CMD_DISCARD		0x0F
#define	GITS_CMD_VMOVI			0x21
#define	GITS_CMD_VMOVP			0x22
#define	GITS_CMD_VSYNC			0x25
#define	GITS_CMD_VMAPP			0x29
#define	GITS_CMD_VMAPTI			0x2A
#define	GITS_CMD_VMAPI			0x2B
#define	GITS_CMD_VINVALL		0x2D

#endif /* !_ARM_CORTEX_GICREG_H_ */
