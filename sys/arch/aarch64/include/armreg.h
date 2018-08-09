/* $NetBSD: armreg.h,v 1.16 2018/08/09 10:27:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _AARCH64_ARMREG_H_
#define _AARCH64_ARMREG_H_

#include <arm/cputypes.h>
#include <sys/types.h>

#define AARCH64REG_READ_INLINE2(regname, regdesc)		\
static __inline uint64_t					\
reg_##regname##_read(void)					\
{								\
	uint64_t __rv;						\
	__asm __volatile("mrs %0, " #regdesc : "=r"(__rv));	\
	return __rv;						\
}

#define AARCH64REG_WRITE_INLINE2(regname, regdesc)		\
static __inline void						\
reg_##regname##_write(uint64_t __val)				\
{								\
	__asm __volatile("msr " #regdesc ", %0" :: "r"(__val));	\
}

#define AARCH64REG_WRITEIMM_INLINE2(regname, regdesc)		\
static __inline void						\
reg_##regname##_write(uint64_t __val)				\
{								\
	__asm __volatile("msr " #regdesc ", %0" :: "n"(__val));	\
}

#define AARCH64REG_READ_INLINE(regname)				\
	AARCH64REG_READ_INLINE2(regname, regname)

#define AARCH64REG_WRITE_INLINE(regname)			\
	AARCH64REG_WRITE_INLINE2(regname, regname)

#define AARCH64REG_WRITEIMM_INLINE(regname)			\
	AARCH64REG_WRITEIMM_INLINE2(regname, regname)

#define AARCH64REG_READWRITE_INLINE2(regname, regdesc)		\
	AARCH64REG_READ_INLINE2(regname, regdesc)		\
	AARCH64REG_WRITE_INLINE2(regname, regdesc)

/*
 * System registers available at EL0 (user)
 */
AARCH64REG_READ_INLINE(ctr_el0)		// Cache Type Register

#define	CTR_EL0_CWG_LINE	__BITS(27,24)	// Cacheback Writeback Granule
#define	CTR_EL0_ERG_LINE	__BITS(23,20)	// Exclusives Reservation Granule
#define	CTR_EL0_DMIN_LINE	__BITS(19,16)	// Dcache MIN LINE size (log2 - 2)
#define	CTR_EL0_L1IP_MASK	__BITS(15,14)
#define	 CTR_EL0_L1IP_AIVIVT	1		//  ASID-tagged Virtual Index, Virtual Tag
#define	 CTR_EL0_L1IP_VIPT	2		//  Virtual Index, Physical Tag
#define	 CTR_EL0_L1IP_PIPT	3		//  Physical Index, Physical Tag
#define	CTR_EL0_IMIN_LINE	__BITS(3,0)	// Icache MIN LINE size (log2 - 2)

AARCH64REG_READ_INLINE(dczid_el0)	// Data Cache Zero ID Register

#define	DCZID_DZP		__BIT(4)	// Data Zero Prohibited
#define	DCZID_BS		__BITS(3,0)	// Block Size (log2 - 2)

AARCH64REG_READ_INLINE(fpcr)		// Floating Point Control Register
AARCH64REG_WRITE_INLINE(fpcr)

#define	FPCR_AHP		__BIT(26)	// Alternative Half Precision
#define	FPCR_DN			__BIT(25)	// Default Nan Control
#define	FPCR_FZ			__BIT(24)	// Flush-To-Zero
#define	FPCR_RMODE		__BITS(23,22)	// Rounding Mode
#define	 FPCR_RN		0		//  Round Nearest
#define	 FPCR_RP		1		//  Round towards Plus infinity
#define	 FPCR_RM		2		//  Round towards Minus infinity
#define	 FPCR_RZ		3		//  Round towards Zero
#define	FPCR_STRIDE		__BITS(21,20)
#define	FPCR_LEN		__BITS(18,16)
#define	FPCR_IDE		__BIT(15)	// Input Denormal Exception enable
#define	FPCR_IXE		__BIT(12)	// IneXact Exception enable
#define	FPCR_UFE		__BIT(11)	// UnderFlow Exception enable
#define	FPCR_OFE		__BIT(10)	// OverFlow Exception enable
#define	FPCR_DZE		__BIT(9)	// Divide by Zero Exception enable
#define	FPCR_IOE		__BIT(8)	// Invalid Operation Exception enable
#define	FPCR_ESUM		0x1F00

AARCH64REG_READ_INLINE(fpsr)		// Floating Point Status Register
AARCH64REG_WRITE_INLINE(fpsr)

#define	FPSR_N32		__BIT(31)	// AARCH32 Negative
#define	FPSR_Z32		__BIT(30)	// AARCH32 Zero
#define	FPSR_C32		__BIT(29)	// AARCH32 Carry
#define	FPSR_V32		__BIT(28)	// AARCH32 Overflow
#define	FPSR_QC			__BIT(27)	// SIMD Saturation
#define	FPSR_IDC		__BIT(7)	// Input Denormal Cumulative status
#define	FPSR_IXC		__BIT(4)	// IneXact Cumulative status
#define	FPSR_UFC		__BIT(3)	// UnderFlow Cumulative status
#define	FPSR_OFC		__BIT(2)	// OverFlow Cumulative status
#define	FPSR_DZC		__BIT(1)	// Divide by Zero Cumulative status
#define	FPSR_IOC		__BIT(0)	// Invalid Operation Cumulative status
#define	FPSR_CSUM		0x1F

AARCH64REG_READ_INLINE(nzcv)		// condition codes
AARCH64REG_WRITE_INLINE(nzcv)

#define	NZCV_N			__BIT(31)	// Negative
#define	NZCV_Z			__BIT(30)	// Zero
#define	NZCV_C			__BIT(29)	// Carry
#define	NZCV_V			__BIT(28)	// Overflow

AARCH64REG_READ_INLINE(tpidr_el0)	// Thread Pointer ID Register (RW)
AARCH64REG_WRITE_INLINE(tpidr_el0)

AARCH64REG_READ_INLINE(tpidrro_el0)	// Thread Pointer ID Register (RO)

/*
 * From here on, these can only be accessed at EL1 (kernel)
 */

/*
 * These are readonly registers
 */
AARCH64REG_READ_INLINE(aidr_el1)

AARCH64REG_READ_INLINE2(cbar_el1, s3_1_c15_c3_0)	// Cortex-A57

#define	CBAR_PA			__BITS(47,18)

AARCH64REG_READ_INLINE(ccsidr_el1)

#define	CCSIDR_WT		__BIT(31)	// Write-through supported
#define	CCSIDR_WB		__BIT(30)	// Write-back supported
#define	CCSIDR_RA		__BIT(29)	// Read-allocation supported
#define	CCSIDR_WA		__BIT(28)	// Write-allocation supported
#define	CCSIDR_NUMSET		__BITS(27,13)	// (Number of sets in cache) - 1
#define	CCSIDR_ASSOC		__BITS(12,3)	// (Associativity of cache) - 1
#define	CCSIDR_LINESIZE 	__BITS(2,0)	// Number of bytes in cache line

AARCH64REG_READ_INLINE(clidr_el1)

#define	CLIDR_LOUU		__BITS(29,27)	// Level of Unification Uniprocessor
#define	CLIDR_LOC		__BITS(26,24)	// Level of Coherency
#define	CLIDR_LOUIS		__BITS(23,21)	// Level of Unification InnerShareable*/
#define	CLIDR_CTYPE7		__BITS(20,18)	// Cache Type field for level7
#define	CLIDR_CTYPE6		__BITS(17,15)	// Cache Type field for level6
#define	CLIDR_CTYPE5		__BITS(14,12)	// Cache Type field for level5
#define	CLIDR_CTYPE4		__BITS(11,9)	// Cache Type field for level4
#define	CLIDR_CTYPE3		__BITS(8,6)	// Cache Type field for level3
#define	CLIDR_CTYPE2		__BITS(5,3)	// Cache Type field for level2
#define	CLIDR_CTYPE1		__BITS(2,0)	// Cache Type field for level1
#define	 CLIDR_TYPE_NOCACHE	 0		//  No cache
#define	 CLIDR_TYPE_ICACHE	 1		//  Instruction cache only
#define	 CLIDR_TYPE_DCACHE	 2		//  Data cache only
#define	 CLIDR_TYPE_IDCACHE	 3		//  Separate inst and data caches
#define	 CLIDR_TYPE_UNIFIEDCACHE 4		//  Unified cache

AARCH64REG_READ_INLINE(currentel)
AARCH64REG_READ_INLINE(id_aa64afr0_el1)
AARCH64REG_READ_INLINE(id_aa64afr1_el1)
AARCH64REG_READ_INLINE(id_aa64dfr0_el1)

#define	ID_AA64DFR0_EL1_CTX_CMPS	__BITS(31,28)
#define	ID_AA64DFR0_EL1_WRPS		__BITS(20,23)
#define	ID_AA64DFR0_EL1_BRPS		__BITS(12,15)
#define	ID_AA64DFR0_EL1_PMUVER		__BITS(8,11)
#define	 ID_AA64DFR0_EL1_PMUVER_NONE	 0
#define	 ID_AA64DFR0_EL1_PMUVER_V3	 1
#define	 ID_AA64DFR0_EL1_PMUVER_NOV3	 2
#define	ID_AA64DFR0_EL1_TRACEVER	__BITS(4,7)
#define	 ID_AA64DFR0_EL1_TRACEVER_NONE	 0
#define	 ID_AA64DFR0_EL1_TRACEVER_IMPL	 1
#define	ID_AA64DFR0_EL1_DEBUGVER	__BITS(0,3)
#define	 ID_AA64DFR0_EL1_DEBUGVER_V8A	 6

AARCH64REG_READ_INLINE(id_aa64dfr1_el1)

AARCH64REG_READ_INLINE(id_aa64isar0_el1)

#define	ID_AA64ISAR0_EL1_CRC32		__BITS(19,16)
#define	 ID_AA64ISAR0_EL1_CRC32_NONE	 0
#define	 ID_AA64ISAR0_EL1_CRC32_CRC32X	 1
#define	ID_AA64ISAR0_EL1_SHA2		__BITS(15,12)
#define	 ID_AA64ISAR0_EL1_SHA2_NONE	 0
#define	 ID_AA64ISAR0_EL1_SHA2_SHA256HSU 1
#define	ID_AA64ISAR0_EL1_SHA1		__BITS(11,8)
#define	 ID_AA64ISAR0_EL1_SHA1_NONE	 0
#define	 ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU 1
#define	ID_AA64ISAR0_EL1_AES		__BITS(7,4)
#define	 ID_AA64ISAR0_EL1_AES_NONE	 0
#define	 ID_AA64ISAR0_EL1_AES_AES	 1
#define	 ID_AA64ISAR0_EL1_AES_PMUL	 2

AARCH64REG_READ_INLINE(id_aa64isar1_el1)
AARCH64REG_READ_INLINE(id_aa64mmfr0_el1)

#define	ID_AA64MMFR0_EL1_TGRAN4		__BITS(31,28)
#define	 ID_AA64MMFR0_EL1_TGRAN4_4KB	 0
#define	 ID_AA64MMFR0_EL1_TGRAN4_NONE	 15
#define	ID_AA64MMFR0_EL1_TGRAN64	__BITS(24,27)
#define	 ID_AA64MMFR0_EL1_TGRAN64_64KB	 0
#define	 ID_AA64MMFR0_EL1_TGRAN64_NONE	 15
#define	ID_AA64MMFR0_EL1_TGRAN16	__BITS(20,23)
#define	 ID_AA64MMFR0_EL1_TGRAN16_NONE	 0
#define	 ID_AA64MMFR0_EL1_TGRAN16_16KB	 1
#define	ID_AA64MMFR0_EL1_BIGENDEL0	__BITS(16,19)
#define	 ID_AA64MMFR0_EL1_BIGENDEL0_NONE 0
#define	 ID_AA64MMFR0_EL1_BIGENDEL0_MIX	 1
#define	ID_AA64MMFR0_EL1_SNSMEM		__BITS(12,15)
#define	 ID_AA64MMFR0_EL1_SNSMEM_NONE	 0
#define	 ID_AA64MMFR0_EL1_SNSMEM_SNSMEM	 1
#define	ID_AA64MMFR0_EL1_BIGEND		__BITS(8,11)
#define	 ID_AA64MMFR0_EL1_BIGEND_NONE	 0
#define	 ID_AA64MMFR0_EL1_BIGEND_MIX	 1
#define	ID_AA64MMFR0_EL1_ASIDBITS	__BITS(4,7)
#define	 ID_AA64MMFR0_EL1_ASIDBITS_8BIT	 0
#define	 ID_AA64MMFR0_EL1_ASIDBITS_16BIT 2
#define	ID_AA64MMFR0_EL1_PARANGE	__BITS(0,3)
#define	 ID_AA64MMFR0_EL1_PARANGE_4G	 0
#define	 ID_AA64MMFR0_EL1_PARANGE_64G	 1
#define	 ID_AA64MMFR0_EL1_PARANGE_1T	 2
#define	 ID_AA64MMFR0_EL1_PARANGE_4T	 3
#define	 ID_AA64MMFR0_EL1_PARANGE_16T	 4
#define	 ID_AA64MMFR0_EL1_PARANGE_256T	 5

AARCH64REG_READ_INLINE(id_aa64mmfr1_el1)
AARCH64REG_READ_INLINE(id_aa64pfr0_el1)
AARCH64REG_READ_INLINE(id_aa64pfr1_el1)
AARCH64REG_READ_INLINE(id_pfr1_el1)
AARCH64REG_READ_INLINE(isr_el1)
AARCH64REG_READ_INLINE(midr_el1)
AARCH64REG_READ_INLINE(mpidr_el1)

#define	MPIDR_AFF3		__BITS(32,39)
#define	MPIDR_U	 		__BIT(30)		// 1 = Uni-Processor System
#define	MPIDR_MT		__BIT(24)		// 1 = SMT(AFF0 is logical)
#define	MPIDR_AFF2		__BITS(16,23)
#define	MPIDR_AFF1		__BITS(8,15)
#define	MPIDR_AFF0		__BITS(0,7)

AARCH64REG_READ_INLINE(mvfr0_el1)

#define	MVFR0_FPROUND		__BITS(31,28)
#define	 MVFR0_FPROUND_NEAREST	 0
#define	 MVFR0_FPROUND_ALL	 1
#define	MVFR0_FPSHVEC		__BITS(27,24)
#define	 MVFR0_FPSHVEC_NONE	 0
#define	 MVFR0_FPSHVEC_SHVEC	 1
#define	MVFR0_FPSQRT		__BITS(23,20)
#define	 MVFR0_FPSQRT_NONE	 0
#define	 MVFR0_FPSQRT_VSQRT	 1
#define	MVFR0_FPDIVIDE		__BITS(19,16)
#define	 MVFR0_FPDIVIDE_NONE	 0
#define	 MVFR0_FPDIVIDE_VDIV	 1
#define	MVFR0_FPTRAP		__BITS(15,12)
#define	 MVFR0_FPTRAP_NONE	 0
#define	 MVFR0_FPTRAP_TRAP	 1
#define	MVFR0_FPDP		__BITS(11,8)
#define	 MVFR0_FPDP_NONE	 0
#define	 MVFR0_FPDP_VFPV2	 1
#define	 MVFR0_FPDP_VFPV3	 2
#define	MVFR0_FPSP		__BITS(7,4)
#define	 MVFR0_FPSP_NONE	 0
#define	 MVFR0_FPSP_VFPV2	 1
#define	 MVFR0_FPSP_VFPV3	 2
#define	MVFR0_SIMDREG		__BITS(3,0)
#define	 MVFR0_SIMDREG_NONE	 0
#define	 MVFR0_SIMDREG_16x64	 1
#define	 MVFR0_SIMDREG_32x64	 2

AARCH64REG_READ_INLINE(mvfr1_el1)

#define	MVFR1_SIMDFMAC		__BITS(31,28)
#define	 MVFR1_SIMDFMAC_NONE	 0
#define	 MVFR1_SIMDFMAC_FMAC	 1
#define	MVFR1_FPHP		__BITS(27,24)
#define	 MVFR1_FPHP_NONE	 0
#define	 MVFR1_FPHP_HALF_SINGLE	 1
#define	 MVFR1_FPHP_HALF_DOUBLE	 2
#define	MVFR1_SIMDHP		__BITS(23,20)
#define	 MVFR1_SIMDHP_NONE	 0
#define	 MVFR1_SIMDHP_HALF	 1
#define	MVFR1_SIMDSP		__BITS(19,16)
#define	 MVFR1_SIMDSP_NONE	 0
#define	 MVFR1_SIMDSP_SINGLE	 1
#define	MVFR1_SIMDINT		 __BITS(15,12)
#define	 MVFR1_SIMDINT_NONE	 0
#define	 MVFR1_SIMDINT_INTEGER	 1
#define	MVFR1_SIMDLS		__BITS(11,8)
#define	 MVFR1_SIMDLS_NONE	 0
#define	 MVFR1_SIMDLS_LOADSTORE	 1
#define	MVFR1_FPDNAN		__BITS(7,4)
#define	 MVFR1_FPDNAN_NONE	 0
#define	 MVFR1_FPDNAN_NAN	 1
#define	MVFR1_FPFTZ		__BITS(3,0)
#define	 MVFR1_FPFTZ_NONE	 0
#define	 MVFR1_FPFTZ_DENORMAL	 1

AARCH64REG_READ_INLINE(mvfr2_el1)

#define	MVFR2_FPMISC		__BITS(7,4)
#define	 MVFR2_FPMISC_NONE	 0
#define	 MVFR2_FPMISC_SEL	 1
#define	 MVFR2_FPMISC_DROUND	 2
#define	 MVFR2_FPMISC_ROUNDINT	 3
#define	 MVFR2_FPMISC_MAXMIN	 4
#define	MVFR2_SIMDMISC		__BITS(3,0)
#define	 MVFR2_SIMDMISC_NONE	 0
#define	 MVFR2_SIMDMISC_DROUND	 1
#define	 MVFR2_SIMDMISC_ROUNDINT 2
#define	 MVFR2_SIMDMISC_MAXMIN	 3

AARCH64REG_READ_INLINE(revidr_el1)

/*
 * These are read/write registers
 */
AARCH64REG_READ_INLINE(cpacr_el1)	// Coprocessor Access Control Regiser
AARCH64REG_WRITE_INLINE(cpacr_el1)

#define	CPACR_TTA		__BIT(28)	 // System Register Access Traps
#define	CPACR_FPEN		__BITS(21,20)
#define  CPACR_FPEN_NONE	 __SHIFTIN(0, CPACR_FPEN)
#define	 CPACR_FPEN_EL1		 __SHIFTIN(1, CPACR_FPEN)
#define	 CPACR_FPEN_NONE_2	 __SHIFTIN(2, CPACR_FPEN)
#define	 CPACR_FPEN_ALL		 __SHIFTIN(3, CPACR_FPEN)

AARCH64REG_READ_INLINE(csselr_el1)	// Cache Size Selection Register
AARCH64REG_WRITE_INLINE(csselr_el1)

#define	CSSELR_LEVEL		__BITS(3,1)	// Cache level of required cache
#define	CSSELR_IND		__BIT(0)	// Instruction not Data bit

AARCH64REG_READ_INLINE(daif)		// Debug Async Irq Fiq mask register
AARCH64REG_WRITE_INLINE(daif)
AARCH64REG_WRITEIMM_INLINE(daifclr)
AARCH64REG_WRITEIMM_INLINE(daifset)

#define	DAIF_D			__BIT(9)	// Debug Exception Mask
#define	DAIF_A			__BIT(8)	// SError Abort Mask
#define	DAIF_I			__BIT(7)	// IRQ Mask
#define	DAIF_F			__BIT(6)	// FIQ Mask
#define	DAIF_SETCLR_SHIFT	6		// for daifset/daifclr #imm shift

AARCH64REG_READ_INLINE(elr_el1)		// Exception Link Register
AARCH64REG_WRITE_INLINE(elr_el1)

AARCH64REG_READ_INLINE(esr_el1)		// Exception Symdrone Register
AARCH64REG_WRITE_INLINE(esr_el1)

#define	ESR_EC			__BITS(31,26) // Exception Cause
#define	 ESR_EC_UNKNOWN		 0x00	// AXX: Unknown Reason
#define	 ESR_EC_WFX		 0x01	// AXX: WFI or WFE instruction execution
#define	 ESR_EC_CP15_RT		 0x03	// A32: MCR/MRC access to CP15 !EC=0
#define	 ESR_EC_CP15_RRT	 0x04	// A32: MCRR/MRRC access to CP15 !EC=0
#define	 ESR_EC_CP14_RT		 0x05	// A32: MCR/MRC access to CP14
#define	 ESR_EC_CP14_DT		 0x06	// A32: LDC/STC access to CP14
#define	 ESR_EC_FP_ACCESS	 0x07	// AXX: Access to SIMD/FP Registers
#define	 ESR_EC_FPID		 0x08	// A32: MCR/MRC access to CP10 !EC=7
#define	 ESR_EC_CP14_RRT	 0x0c	// A32: MRRC access to CP14
#define	 ESR_EC_ILL_STATE	 0x0e	// AXX: Illegal Execution State
#define	 ESR_EC_SVC_A32		 0x11	// A32: SVC Instruction Execution
#define	 ESR_EC_HVC_A32		 0x12	// A32: HVC Instruction Execution
#define	 ESR_EC_SMC_A32		 0x13	// A32: SMC Instruction Execution
#define	 ESR_EC_SVC_A64		 0x15	// A64: SVC Instruction Execution
#define	 ESR_EC_HVC_A64		 0x16	// A64: HVC Instruction Execution
#define	 ESR_EC_SMC_A64		 0x17	// A64: SMC Instruction Execution
#define	 ESR_EC_SYS_REG		 0x18	// A64: MSR/MRS/SYS instruction (!EC0/1/7)
#define	 ESR_EC_INSN_ABT_EL0	 0x20	// AXX: Instruction Abort (EL0)
#define	 ESR_EC_INSN_ABT_EL1	 0x21	// AXX: Instruction Abort (EL1)
#define	 ESR_EC_PC_ALIGNMENT	 0x22	// AXX: Misaligned PC
#define	 ESR_EC_DATA_ABT_EL0	 0x24	// AXX: Data Abort (EL0)
#define	 ESR_EC_DATA_ABT_EL1	 0x25	// AXX: Data Abort (EL1)
#define	 ESR_EC_SP_ALIGNMENT 	 0x26	// AXX: Misaligned SP
#define	 ESR_EC_FP_TRAP_A32	 0x28	// A32: FP Exception
#define	 ESR_EC_FP_TRAP_A64	 0x2c	// A64: FP Exception
#define	 ESR_EC_SERROR	 	 0x2f	// AXX: SError Interrupt
#define	 ESR_EC_BRKPNT_EL0	 0x30	// AXX: Breakpoint Exception (EL0)
#define	 ESR_EC_BRKPNT_EL1	 0x31	// AXX: Breakpoint Exception (EL1)
#define	 ESR_EC_SW_STEP_EL0	 0x32	// AXX: Software Step (EL0)
#define	 ESR_EC_SW_STEP_EL1	 0x33	// AXX: Software Step (EL1)
#define	 ESR_EC_WTCHPNT_EL0	 0x34	// AXX: Watchpoint (EL0)
#define	 ESR_EC_WTCHPNT_EL1	 0x35	// AXX: Watchpoint (EL1)
#define	 ESR_EC_BKPT_INSN_A32	 0x38	// A32: BKPT Instruction Execution
#define	 ESR_EC_VECTOR_CATCH	 0x3a	// A32: Vector Catch Exception
#define	 ESR_EC_BKPT_INSN_A64	 0x3c	// A64: BKPT Instruction Execution
#define	ESR_IL			__BIT(25)	// Instruction Length (1=32-bit)
#define	ESR_ISS			__BITS(24,0)	// Instruction Specific Syndrome
#define	ESR_ISS_CV		__BIT(24)	// common
#define	ESR_ISS_COND		__BITS(23,20)	// common
#define	ESR_ISS_WFX_TRAP_INSN	__BIT(0)	// for ESR_EC_WFX
#define	ESR_ISS_MRC_OPC2	__BITS(19,17)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MRC_OPC1	__BITS(16,14)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MRC_CRN		__BITS(13,10)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MRC_RT		__BITS(9,5)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MRC_CRM		__BITS(4,1)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MRC_DIRECTION	__BIT(0)	// for ESR_EC_CP15_RT
#define	ESR_ISS_MCRR_OPC1	__BITS(19,16)	// for ESR_EC_CP15_RRT
#define	ESR_ISS_MCRR_RT2	__BITS(14,10)	// for ESR_EC_CP15_RRT
#define	ESR_ISS_MCRR_RT		__BITS(9,5)	// for ESR_EC_CP15_RRT
#define	ESR_ISS_MCRR_CRM	__BITS(4,1)	// for ESR_EC_CP15_RRT
#define	ESR_ISS_MCRR_DIRECTION	__BIT(0)	// for ESR_EC_CP15_RRT
#define	ESR_ISS_HVC_IMM16	__BITS(15,0)	// for ESR_EC_{SVC,HVC}
// ...
#define	ESR_ISS_INSNABORT_EA	__BIT(9)	// for ESC_RC_INSN_ABT_EL[01]
#define	ESR_ISS_INSNABORT_S1PTW	__BIT(7)	// for ESC_RC_INSN_ABT_EL[01]
#define	ESR_ISS_INSNABORT_IFSC	__BITS(0,5)	// for ESC_RC_INSN_ABT_EL[01]
#define	ESR_ISS_DATAABORT_ISV	__BIT(24)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_SAS	__BITS(23,22)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_SSE	__BIT(21)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_SRT	__BITS(19,16)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_SF	__BIT(15)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_AR	__BIT(14)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_EA	__BIT(9)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_CM	__BIT(8)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_S1PTW	__BIT(7)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_WnR	__BIT(6)	// for ESC_RC_DATA_ABT_EL[01]
#define	ESR_ISS_DATAABORT_DFSC	__BITS(0,5)	// for ESC_RC_DATA_ABT_EL[01]

#define	ESR_ISS_FSC_ADDRESS_SIZE_FAULT_0		0x00
#define	ESR_ISS_FSC_ADDRESS_SIZE_FAULT_1		0x01
#define	ESR_ISS_FSC_ADDRESS_SIZE_FAULT_2		0x02
#define	ESR_ISS_FSC_ADDRESS_SIZE_FAULT_3		0x03
#define	ESR_ISS_FSC_TRANSLATION_FAULT_0			0x04
#define	ESR_ISS_FSC_TRANSLATION_FAULT_1			0x05
#define	ESR_ISS_FSC_TRANSLATION_FAULT_2			0x06
#define	ESR_ISS_FSC_TRANSLATION_FAULT_3			0x07
#define	ESR_ISS_FSC_ACCESS_FAULT_0			0x08
#define	ESR_ISS_FSC_ACCESS_FAULT_1			0x09
#define	ESR_ISS_FSC_ACCESS_FAULT_2			0x0a
#define	ESR_ISS_FSC_ACCESS_FAULT_3			0x0b
#define	ESR_ISS_FSC_PERM_FAULT_0			0x0c
#define	ESR_ISS_FSC_PERM_FAULT_1			0x0d
#define	ESR_ISS_FSC_PERM_FAULT_2			0x0e
#define	ESR_ISS_FSC_PERM_FAULT_3			0x0f
#define	ESR_ISS_FSC_SYNC_EXTERNAL_ABORT			0x10
#define	ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_0	0x14
#define	ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_1	0x15
#define	ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_2	0x16
#define	ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_3	0x17
#define	ESR_ISS_FSC_SYNC_PARITY_ERROR			0x18
#define	ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_0	0x1c
#define	ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_1	0x1d
#define	ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_2	0x1e
#define	ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_3	0x1f
#define	ESR_ISS_FSC_ALIGNMENT_FAULT			0x21
#define	ESR_ISS_FSC_TLB_CONFLICT_FAULT			0x30
#define	ESR_ISS_FSC_LOCKDOWN_ABORT			0x34
#define	ESR_ISS_FSC_UNSUPPORTED_EXCLUSIVE		0x35
#define	ESR_ISS_FSC_FIRST_LEVEL_DOMAIN_FAULT		0x3d
#define	ESR_ISS_FSC_SECOND_LEVEL_DOMAIN_FAULT		0x3e


AARCH64REG_READ_INLINE(far_el1)		// Fault Address Register
AARCH64REG_WRITE_INLINE(far_el1)

AARCH64REG_READ_INLINE2(l2ctlr_el1, s3_1_c11_c0_2)  // Cortex-A53,57,72,73
AARCH64REG_WRITE_INLINE2(l2ctlr_el1, s3_1_c11_c0_2) // Cortex-A53,57,72,73

#define	L2CTLR_NUMOFCORE	__BITS(25,24)	// Number of cores
#define	L2CTLR_CPUCACHEPROT	__BIT(22)	// CPU Cache Protection
#define	L2CTLR_SCUL2CACHEPROT	__BIT(21)	// SCU-L2 Cache Protection
#define	L2CTLR_L2_INPUT_LATENCY	__BIT(5)	// L2 Data RAM input latency
#define	L2CTLR_L2_OUTPUT_LATENCY __BIT(0)	// L2 Data RAM output latency

AARCH64REG_READ_INLINE(mair_el1) // Memory Attribute Indirection Register
AARCH64REG_WRITE_INLINE(mair_el1)

#define	MAIR_ATTR0		 __BITS(7,0)
#define	MAIR_ATTR1		 __BITS(15,8)
#define	MAIR_ATTR2		 __BITS(23,16)
#define	MAIR_ATTR3		 __BITS(31,24)
#define	MAIR_ATTR4		 __BITS(39,32)
#define	MAIR_ATTR5		 __BITS(47,40)
#define	MAIR_ATTR6		 __BITS(55,48)
#define	MAIR_ATTR7		 __BITS(63,56)
#define	MAIR_DEVICE_nGnRnE	 0x00	// NoGathering,NoReordering,NoEarlyWriteAck.
#define	MAIR_NORMAL_NC		 0x44
#define	MAIR_NORMAL_WT		 0xbb
#define	MAIR_NORMAL_WB		 0xff

AARCH64REG_READ_INLINE(par_el1)		// Physical Address Register
AARCH64REG_WRITE_INLINE(par_el1)

#define	PAR_ATTR		__BITS(63,56)	// F=0 memory attributes
#define	PAR_PA			__BITS(47,12)	// F=0 physical address
#define	PAR_NS			__BIT(9)	// F=0 non-secure
#define	PAR_S			__BIT(9)	// F=1 failure stage
#define	PAR_SHA			__BITS(8,7)	// F=0 shareability attribute
#define	 PAR_SHA_NONE		 0
#define	 PAR_SHA_OUTER		 2
#define	 PAR_SHA_INNER		 3
#define	PAR_PTW			__BIT(8)	// F=1 partial table walk
#define	PAR_FST			__BITS(6,1)	// F=1 fault status code
#define	PAR_F			__BIT(0)	// translation failed

AARCH64REG_READ_INLINE(rmr_el1)		// Reset Management Register
AARCH64REG_WRITE_INLINE(rmr_el1)

AARCH64REG_READ_INLINE(rvbar_el1)	// Reset Vector Base Address Register
AARCH64REG_WRITE_INLINE(rvbar_el1)

AARCH64REG_READ_INLINE(sctlr_el1)	// System Control Register
AARCH64REG_WRITE_INLINE(sctlr_el1)

#define	SCTLR_RES0		0xc8222400	// Reserved ARMv8.0, write 0
#define	SCTLR_RES1		0x30d00800	// Reserved ARMv8.0, write 1
#define	SCTLR_M			__BIT(0)
#define	SCTLR_A			__BIT(1)
#define	SCTLR_C			__BIT(2)
#define	SCTLR_SA		__BIT(3)
#define	SCTLR_SA0		__BIT(4)
#define	SCTLR_CP15BEN		__BIT(5)
#define	SCTLR_THEE		__BIT(6)
#define	SCTLR_ITD		__BIT(7)
#define	SCTLR_SED		__BIT(8)
#define	SCTLR_UMA		__BIT(9)
#define	SCTLR_I			__BIT(12)
#define	SCTLR_DZE		__BIT(14)
#define	SCTLR_UCT		__BIT(15)
#define	SCTLR_nTWI		__BIT(16)
#define	SCTLR_nTWE		__BIT(18)
#define	SCTLR_WXN		__BIT(19)
#define	SCTLR_IESB		__BIT(21)
#define	SCTLR_SPAN		__BIT(23)
#define	SCTLR_EOE		__BIT(24)
#define	SCTLR_EE		__BIT(25)
#define	SCTLR_UCI		__BIT(26)
#define	SCTLR_nTLSMD		__BIT(28)
#define	SCTLR_LSMAOE		__BIT(29)

// current EL stack pointer
static __inline uint64_t
reg_sp_read(void)
{
	uint64_t __rv;
	__asm __volatile ("mov %0, sp" : "=r"(__rv));
	return __rv;
}

AARCH64REG_READ_INLINE(sp_el0)		// EL0 Stack Pointer
AARCH64REG_WRITE_INLINE(sp_el0)

AARCH64REG_READ_INLINE(spsel)		// Stack Pointer Select
AARCH64REG_WRITE_INLINE(spsel)

#define	SPSEL_SP		__BIT(0);	// use SP_EL0 at all exception levels

AARCH64REG_READ_INLINE(spsr_el1)	// Saved Program Status Register
AARCH64REG_WRITE_INLINE(spsr_el1)

#define	SPSR_NZCV 		__BITS(31,28)	// mask of N Z C V
#define	 SPSR_N	 		__BIT(31)	// Negative
#define	 SPSR_Z	 		__BIT(30)	// Zero
#define	 SPSR_C	 		__BIT(29)	// Carry
#define	 SPSR_V	 		__BIT(28)	// oVerflow
#define	SPSR_A32_Q 		__BIT(27)	// A32: Overflow
#define	SPSR_A32_J 		__BIT(24)	// A32: Jazelle Mode
#define	SPSR_A32_IT1 		__BIT(23)	// A32: IT[1]
#define	SPSR_A32_IT0 		__BIT(22)	// A32: IT[0]
#define	SPSR_SS	 		__BIT(21)	// Software Step
#define	SPSR_IL	 		__BIT(20)	// Instruction Length
#define	SPSR_GE	 		__BITS(19,16)	// A32: SIMD GE
#define	SPSR_IT7 		__BIT(15)	// A32: IT[7]
#define	SPSR_IT6 		__BIT(14)	// A32: IT[6]
#define	SPSR_IT5 		__BIT(13)	// A32: IT[5]
#define	SPSR_IT4 		__BIT(12)	// A32: IT[4]
#define	SPSR_IT3 		__BIT(11)	// A32: IT[3]
#define	SPSR_IT2 		__BIT(10)	// A32: IT[2]
#define	SPSR_A64_D 		__BIT(9)	// A64: Debug Exception Mask
#define	SPSR_A32_E 		__BIT(9)	// A32: BE Endian Mode
#define	SPSR_A	 		__BIT(8)	// Async abort (SError) Mask
#define	SPSR_I	 		__BIT(7)	// IRQ Mask
#define	SPSR_F	 		__BIT(6)	// FIQ Mask
#define	SPSR_A32_T 		__BIT(5)	// A32 Thumb Mode
#define	SPSR_M	 		__BITS(4,0)	// Execution State
#define	 SPSR_M_EL3H 		 0x0d
#define	 SPSR_M_EL3T 		 0x0c
#define	 SPSR_M_EL2H 		 0x09
#define	 SPSR_M_EL2T 		 0x08
#define	 SPSR_M_EL1H 		 0x05
#define	 SPSR_M_EL1T 		 0x04
#define	 SPSR_M_EL0T 		 0x00
#define	 SPSR_M_SYS32		 0x1f
#define	 SPSR_M_UND32		 0x1b
#define	 SPSR_M_ABT32		 0x17
#define	 SPSR_M_SVC32		 0x13
#define	 SPSR_M_IRQ32		 0x12
#define	 SPSR_M_FIQ32		 0x11
#define	 SPSR_M_USR32		 0x10

AARCH64REG_READ_INLINE(tcr_el1)		// Translation Control Register
AARCH64REG_WRITE_INLINE(tcr_el1)

#define TCR_PAGE_SIZE1(tcr)	(1L << ((1L << __SHIFTOUT(tcr, TCR_TG1)) + 8))

AARCH64REG_READ_INLINE(tpidr_el1)	// Thread ID Register (EL1)
AARCH64REG_WRITE_INLINE(tpidr_el1)

AARCH64REG_WRITE_INLINE(tpidrro_el0)	// Thread ID Register (RO for EL0)

AARCH64REG_READ_INLINE(ttbr0_el1) // Translation Table Base Register 0 EL1
AARCH64REG_WRITE_INLINE(ttbr0_el1)

AARCH64REG_READ_INLINE(ttbr1_el1) // Translation Table Base Register 1 EL1
AARCH64REG_WRITE_INLINE(ttbr1_el1)

AARCH64REG_READ_INLINE(vbar_el1)	// Vector Base Address Register
AARCH64REG_WRITE_INLINE(vbar_el1)

/*
 * From here on, these are DEBUG registers
 */
AARCH64REG_READ_INLINE(dbgbcr0_el1) // Debug Breakpoint Control Register 0
AARCH64REG_WRITE_INLINE(dbgbcr0_el1)
AARCH64REG_READ_INLINE(dbgbcr1_el1) // Debug Breakpoint Control Register 1
AARCH64REG_WRITE_INLINE(dbgbcr1_el1)
AARCH64REG_READ_INLINE(dbgbcr2_el1) // Debug Breakpoint Control Register 2
AARCH64REG_WRITE_INLINE(dbgbcr2_el1)
AARCH64REG_READ_INLINE(dbgbcr3_el1) // Debug Breakpoint Control Register 3
AARCH64REG_WRITE_INLINE(dbgbcr3_el1)
AARCH64REG_READ_INLINE(dbgbcr4_el1) // Debug Breakpoint Control Register 4
AARCH64REG_WRITE_INLINE(dbgbcr4_el1)
AARCH64REG_READ_INLINE(dbgbcr5_el1) // Debug Breakpoint Control Register 5
AARCH64REG_WRITE_INLINE(dbgbcr5_el1)
AARCH64REG_READ_INLINE(dbgbcr6_el1) // Debug Breakpoint Control Register 6
AARCH64REG_WRITE_INLINE(dbgbcr6_el1)
AARCH64REG_READ_INLINE(dbgbcr7_el1) // Debug Breakpoint Control Register 7
AARCH64REG_WRITE_INLINE(dbgbcr7_el1)
AARCH64REG_READ_INLINE(dbgbcr8_el1) // Debug Breakpoint Control Register 8
AARCH64REG_WRITE_INLINE(dbgbcr8_el1)
AARCH64REG_READ_INLINE(dbgbcr9_el1) // Debug Breakpoint Control Register 9
AARCH64REG_WRITE_INLINE(dbgbcr9_el1)
AARCH64REG_READ_INLINE(dbgbcr10_el1) // Debug Breakpoint Control Register 10
AARCH64REG_WRITE_INLINE(dbgbcr10_el1)
AARCH64REG_READ_INLINE(dbgbcr11_el1) // Debug Breakpoint Control Register 11
AARCH64REG_WRITE_INLINE(dbgbcr11_el1)
AARCH64REG_READ_INLINE(dbgbcr12_el1) // Debug Breakpoint Control Register 12
AARCH64REG_WRITE_INLINE(dbgbcr12_el1)
AARCH64REG_READ_INLINE(dbgbcr13_el1) // Debug Breakpoint Control Register 13
AARCH64REG_WRITE_INLINE(dbgbcr13_el1)
AARCH64REG_READ_INLINE(dbgbcr14_el1) // Debug Breakpoint Control Register 14
AARCH64REG_WRITE_INLINE(dbgbcr14_el1)
AARCH64REG_READ_INLINE(dbgbcr15_el1) // Debug Breakpoint Control Register 15
AARCH64REG_WRITE_INLINE(dbgbcr15_el1)

#define	DBGBCR_BT		 __BITS(23,20)
#define	DBGBCR_LBN		 __BITS(19,16)
#define	DBGBCR_SSC		 __BITS(15,14)
#define	DBGBCR_HMC		 __BIT(13)
#define	DBGBCR_BAS		 __BITS(8,5)
#define	DBGBCR_PMC		 __BITS(2,1)
#define	DBGBCR_E		 __BIT(0)

AARCH64REG_READ_INLINE(dbgbvr0_el1) // Debug Breakpoint Value Register 0
AARCH64REG_WRITE_INLINE(dbgbvr0_el1)
AARCH64REG_READ_INLINE(dbgbvr1_el1) // Debug Breakpoint Value Register 1
AARCH64REG_WRITE_INLINE(dbgbvr1_el1)
AARCH64REG_READ_INLINE(dbgbvr2_el1) // Debug Breakpoint Value Register 2
AARCH64REG_WRITE_INLINE(dbgbvr2_el1)
AARCH64REG_READ_INLINE(dbgbvr3_el1) // Debug Breakpoint Value Register 3
AARCH64REG_WRITE_INLINE(dbgbvr3_el1)
AARCH64REG_READ_INLINE(dbgbvr4_el1) // Debug Breakpoint Value Register 4
AARCH64REG_WRITE_INLINE(dbgbvr4_el1)
AARCH64REG_READ_INLINE(dbgbvr5_el1) // Debug Breakpoint Value Register 5
AARCH64REG_WRITE_INLINE(dbgbvr5_el1)
AARCH64REG_READ_INLINE(dbgbvr6_el1) // Debug Breakpoint Value Register 6
AARCH64REG_WRITE_INLINE(dbgbvr6_el1)
AARCH64REG_READ_INLINE(dbgbvr7_el1) // Debug Breakpoint Value Register 7
AARCH64REG_WRITE_INLINE(dbgbvr7_el1)
AARCH64REG_READ_INLINE(dbgbvr8_el1) // Debug Breakpoint Value Register 8
AARCH64REG_WRITE_INLINE(dbgbvr8_el1)
AARCH64REG_READ_INLINE(dbgbvr9_el1) // Debug Breakpoint Value Register 9
AARCH64REG_WRITE_INLINE(dbgbvr9_el1)
AARCH64REG_READ_INLINE(dbgbvr10_el1) // Debug Breakpoint Value Register 10
AARCH64REG_WRITE_INLINE(dbgbvr10_el1)
AARCH64REG_READ_INLINE(dbgbvr11_el1) // Debug Breakpoint Value Register 11
AARCH64REG_WRITE_INLINE(dbgbvr11_el1)
AARCH64REG_READ_INLINE(dbgbvr12_el1) // Debug Breakpoint Value Register 12
AARCH64REG_WRITE_INLINE(dbgbvr12_el1)
AARCH64REG_READ_INLINE(dbgbvr13_el1) // Debug Breakpoint Value Register 13
AARCH64REG_WRITE_INLINE(dbgbvr13_el1)
AARCH64REG_READ_INLINE(dbgbvr14_el1) // Debug Breakpoint Value Register 14
AARCH64REG_WRITE_INLINE(dbgbvr14_el1)
AARCH64REG_READ_INLINE(dbgbvr15_el1) // Debug Breakpoint Value Register 15
AARCH64REG_WRITE_INLINE(dbgbvr15_el1)

AARCH64REG_READ_INLINE(dbgwcr0_el1) // Debug Watchpoint Control Register 0
AARCH64REG_WRITE_INLINE(dbgwcr0_el1)
AARCH64REG_READ_INLINE(dbgwcr1_el1) // Debug Watchpoint Control Register 1
AARCH64REG_WRITE_INLINE(dbgwcr1_el1)
AARCH64REG_READ_INLINE(dbgwcr2_el1) // Debug Watchpoint Control Register 2
AARCH64REG_WRITE_INLINE(dbgwcr2_el1)
AARCH64REG_READ_INLINE(dbgwcr3_el1) // Debug Watchpoint Control Register 3
AARCH64REG_WRITE_INLINE(dbgwcr3_el1)
AARCH64REG_READ_INLINE(dbgwcr4_el1) // Debug Watchpoint Control Register 4
AARCH64REG_WRITE_INLINE(dbgwcr4_el1)
AARCH64REG_READ_INLINE(dbgwcr5_el1) // Debug Watchpoint Control Register 5
AARCH64REG_WRITE_INLINE(dbgwcr5_el1)
AARCH64REG_READ_INLINE(dbgwcr6_el1) // Debug Watchpoint Control Register 6
AARCH64REG_WRITE_INLINE(dbgwcr6_el1)
AARCH64REG_READ_INLINE(dbgwcr7_el1) // Debug Watchpoint Control Register 7
AARCH64REG_WRITE_INLINE(dbgwcr7_el1)
AARCH64REG_READ_INLINE(dbgwcr8_el1) // Debug Watchpoint Control Register 8
AARCH64REG_WRITE_INLINE(dbgwcr8_el1)
AARCH64REG_READ_INLINE(dbgwcr9_el1) // Debug Watchpoint Control Register 9
AARCH64REG_WRITE_INLINE(dbgwcr9_el1)
AARCH64REG_READ_INLINE(dbgwcr10_el1) // Debug Watchpoint Control Register 10
AARCH64REG_WRITE_INLINE(dbgwcr10_el1)
AARCH64REG_READ_INLINE(dbgwcr11_el1) // Debug Watchpoint Control Register 11
AARCH64REG_WRITE_INLINE(dbgwcr11_el1)
AARCH64REG_READ_INLINE(dbgwcr12_el1) // Debug Watchpoint Control Register 12
AARCH64REG_WRITE_INLINE(dbgwcr12_el1)
AARCH64REG_READ_INLINE(dbgwcr13_el1) // Debug Watchpoint Control Register 13
AARCH64REG_WRITE_INLINE(dbgwcr13_el1)
AARCH64REG_READ_INLINE(dbgwcr14_el1) // Debug Watchpoint Control Register 14
AARCH64REG_WRITE_INLINE(dbgwcr14_el1)
AARCH64REG_READ_INLINE(dbgwcr15_el1) // Debug Watchpoint Control Register 15
AARCH64REG_WRITE_INLINE(dbgwcr15_el1)

#define	DBGWCR_MASK		 __BITS(28,24)
#define	DBGWCR_WT		 __BIT(20)
#define	DBGWCR_LBN		 __BITS(19,16)
#define	DBGWCR_SSC		 __BITS(15,14)
#define	DBGWCR_HMC		 __BIT(13)
#define	DBGWCR_BAS		 __BITS(12,5)
#define	DBGWCR_LSC		 __BITS(4,3)
#define	DBGWCR_PAC		 __BITS(2,1)
#define	DBGWCR_E		 __BIT(0)

AARCH64REG_READ_INLINE(dbgwvr0_el1) // Debug Watchpoint Value Register 0
AARCH64REG_WRITE_INLINE(dbgwvr0_el1)
AARCH64REG_READ_INLINE(dbgwvr1_el1) // Debug Watchpoint Value Register 1
AARCH64REG_WRITE_INLINE(dbgwvr1_el1)
AARCH64REG_READ_INLINE(dbgwvr2_el1) // Debug Watchpoint Value Register 2
AARCH64REG_WRITE_INLINE(dbgwvr2_el1)
AARCH64REG_READ_INLINE(dbgwvr3_el1) // Debug Watchpoint Value Register 3
AARCH64REG_WRITE_INLINE(dbgwvr3_el1)
AARCH64REG_READ_INLINE(dbgwvr4_el1) // Debug Watchpoint Value Register 4
AARCH64REG_WRITE_INLINE(dbgwvr4_el1)
AARCH64REG_READ_INLINE(dbgwvr5_el1) // Debug Watchpoint Value Register 5
AARCH64REG_WRITE_INLINE(dbgwvr5_el1)
AARCH64REG_READ_INLINE(dbgwvr6_el1) // Debug Watchpoint Value Register 6
AARCH64REG_WRITE_INLINE(dbgwvr6_el1)
AARCH64REG_READ_INLINE(dbgwvr7_el1) // Debug Watchpoint Value Register 7
AARCH64REG_WRITE_INLINE(dbgwvr7_el1)
AARCH64REG_READ_INLINE(dbgwvr8_el1) // Debug Watchpoint Value Register 8
AARCH64REG_WRITE_INLINE(dbgwvr8_el1)
AARCH64REG_READ_INLINE(dbgwvr9_el1) // Debug Watchpoint Value Register 9
AARCH64REG_WRITE_INLINE(dbgwvr9_el1)
AARCH64REG_READ_INLINE(dbgwvr10_el1) // Debug Watchpoint Value Register 10
AARCH64REG_WRITE_INLINE(dbgwvr10_el1)
AARCH64REG_READ_INLINE(dbgwvr11_el1) // Debug Watchpoint Value Register 11
AARCH64REG_WRITE_INLINE(dbgwvr11_el1)
AARCH64REG_READ_INLINE(dbgwvr12_el1) // Debug Watchpoint Value Register 12
AARCH64REG_WRITE_INLINE(dbgwvr12_el1)
AARCH64REG_READ_INLINE(dbgwvr13_el1) // Debug Watchpoint Value Register 13
AARCH64REG_WRITE_INLINE(dbgwvr13_el1)
AARCH64REG_READ_INLINE(dbgwvr14_el1) // Debug Watchpoint Value Register 14
AARCH64REG_WRITE_INLINE(dbgwvr14_el1)
AARCH64REG_READ_INLINE(dbgwvr15_el1) // Debug Watchpoint Value Register 15
AARCH64REG_WRITE_INLINE(dbgwvr15_el1)

#define	DBGWVR_MASK		 __BITS(64,3)


AARCH64REG_READ_INLINE(mdscr_el1) // Monitor Debug System Control Register
AARCH64REG_WRITE_INLINE(mdscr_el1)

AARCH64REG_WRITE_INLINE(oslar_el1)	// OS Lock Access Register

AARCH64REG_READ_INLINE(oslsr_el1)	// OS Lock Status Register

/*
 * From here on, these are PMC registers
 */

AARCH64REG_READ_INLINE(pmccfiltr_el0)
AARCH64REG_WRITE_INLINE(pmccfiltr_el0)

#define	PMCCFILTR_P		__BIT(31)	// Don't count cycles in EL1
#define	PMCCFILTR_U		__BIT(30)	// Don't count cycles in EL0
#define	PMCCFILTR_NSK		__BIT(29)	// Don't count cycles in NS EL1
#define	PMCCFILTR_NSU 		__BIT(28)	// Don't count cycles in NS EL0
#define	PMCCFILTR_NSH 		__BIT(27)	// Don't count cycles in NS EL2
#define	PMCCFILTR_M		__BIT(26)	// Don't count cycles in EL3

AARCH64REG_READ_INLINE(pmccntr_el0)

AARCH64REG_READ_INLINE(pmceid0_el0)
AARCH64REG_READ_INLINE(pmceid1_el0)

AARCH64REG_WRITE_INLINE(pmcntenclr_el0)
AARCH64REG_WRITE_INLINE(pmcntenset_el0)

AARCH64REG_READ_INLINE(pmcr_el0)
AARCH64REG_WRITE_INLINE(pmcr_el0)

#define	PMCR_IMP		__BITS(31,24)	// Implementor code
#define	PMCR_IDCODE		__BITS(23,16)	// Identification code
#define	PMCR_N			__BITS(15,11)	// Number of event counters
#define	PMCR_LC			__BIT(6)	// Long cycle counter enable
#define	PMCR_DP			__BIT(5)	// Disable cycle counter when event
						// counting is prohibited
#define	PMCR_X			__BIT(4)	// Enable export of events
#define	PMCR_D			__BIT(3)	// Clock divider
#define	PMCR_C			__BIT(2)	// Cycle counter reset
#define	PMCR_P			__BIT(1)	// Event counter reset
#define	PMCR_E			__BIT(0)	// Enable


AARCH64REG_READ_INLINE(pmevcntr1_el0)
AARCH64REG_WRITE_INLINE(pmevcntr1_el0)

AARCH64REG_READ_INLINE(pmevtyper1_el0)
AARCH64REG_WRITE_INLINE(pmevtyper1_el0)

#define	PMEVTYPER_P		__BIT(31)	// Don't count events in EL1
#define	PMEVTYPER_U		__BIT(30)	// Don't count events in EL0
#define	PMEVTYPER_NSK		__BIT(29)	// Don't count events in NS EL1
#define	PMEVTYPER_NSU		__BIT(28)	// Don't count events in NS EL0
#define	PMEVTYPER_NSH		__BIT(27)	// Count events in NS EL2
#define	PMEVTYPER_M		__BIT(26)	// Don't count events in EL3
#define	PMEVTYPER_MT		__BIT(25)	// Count events on all CPUs with same
						// aff1 level
#define	PMEVTYPER_EVTCOUNT	__BITS(15,0)	// Event to count

AARCH64REG_WRITE_INLINE(pmintenclr_el1)
AARCH64REG_WRITE_INLINE(pmintenset_el1)

AARCH64REG_WRITE_INLINE(pmovsclr_el0)
AARCH64REG_READ_INLINE(pmovsset_el0)
AARCH64REG_WRITE_INLINE(pmovsset_el0)

AARCH64REG_WRITE_INLINE(pmselr_el0)

AARCH64REG_WRITE_INLINE(pmswinc_el0)

AARCH64REG_READ_INLINE(pmuserenr_el0)
AARCH64REG_WRITE_INLINE(pmuserenr_el0)

AARCH64REG_READ_INLINE(pmxevcntr_el0)
AARCH64REG_WRITE_INLINE(pmxevcntr_el0)

AARCH64REG_READ_INLINE(pmxevtyper_el0)
AARCH64REG_WRITE_INLINE(pmxevtyper_el0)

/*
 * Generic timer registers
 */

AARCH64REG_READ_INLINE(cntfrq_el0)

AARCH64REG_READ_INLINE(cnthctl_el2)
AARCH64REG_WRITE_INLINE(cnthctl_el2)

#define	CNTHCTL_EVNTDIR		__BIT(3)
#define	CNTHCTL_EVNTEN		__BIT(2)
#define	CNTHCTL_EL1PCEN		__BIT(1)
#define	CNTHCTL_EL1PCTEN	__BIT(0)

AARCH64REG_READ_INLINE(cntkctl_el1)
AARCH64REG_WRITE_INLINE(cntkctl_el1)

#define	CNTKCTL_EL0PTEN		__BIT(9)	// EL0 access for CNTP CVAL/TVAL/CTL
#define	CNTKCTL_PL0PTEN		CNTKCTL_EL0PTEN
#define	CNTKCTL_EL0VTEN		__BIT(8)	// EL0 access for CNTV CVAL/TVAL/CTL
#define	CNTKCTL_PL0VTEN		CNTKCTL_EL0VTEN
#define	CNTKCTL_ELNTI		__BITS(7,4)
#define	CNTKCTL_EVNTDIR		__BIT(3)
#define	CNTKCTL_EVNTEN		__BIT(2)
#define	CNTKCTL_EL0VCTEN	__BIT(1)	// EL0 access for CNTVCT and CNTFRQ
#define	CNTKCTL_PL0VCTEN	CNTKCTL_EL0VCTEN
#define	CNTKCTL_EL0PCTEN	__BIT(0)	// EL0 access for CNTPCT and CNTFRQ
#define	CNTKCTL_PL0PCTEN	CNTKCTL_EL0PCTEN

AARCH64REG_READ_INLINE(cntp_ctl_el0)
AARCH64REG_WRITE_INLINE(cntp_ctl_el0)
AARCH64REG_READ_INLINE(cntp_cval_el0)
AARCH64REG_WRITE_INLINE(cntp_cval_el0)
AARCH64REG_READ_INLINE(cntp_tval_el0)
AARCH64REG_WRITE_INLINE(cntp_tval_el0)
AARCH64REG_READ_INLINE(cntpct_el0)
AARCH64REG_WRITE_INLINE(cntpct_el0)

AARCH64REG_READ_INLINE(cntps_ctl_el1)
AARCH64REG_WRITE_INLINE(cntps_ctl_el1)
AARCH64REG_READ_INLINE(cntps_cval_el1)
AARCH64REG_WRITE_INLINE(cntps_cval_el1)
AARCH64REG_READ_INLINE(cntps_tval_el1)
AARCH64REG_WRITE_INLINE(cntps_tval_el1)

AARCH64REG_READ_INLINE(cntv_ctl_el0)
AARCH64REG_WRITE_INLINE(cntv_ctl_el0)
AARCH64REG_READ_INLINE(cntv_cval_el0)
AARCH64REG_WRITE_INLINE(cntv_cval_el0)
AARCH64REG_READ_INLINE(cntv_tval_el0)
AARCH64REG_WRITE_INLINE(cntv_tval_el0)
AARCH64REG_READ_INLINE(cntvct_el0)
AARCH64REG_WRITE_INLINE(cntvct_el0)

#define	CNTCTL_ISTATUS		__BIT(2)	// Interrupt Asserted
#define	CNTCTL_IMASK		__BIT(1)	// Timer Interrupt is Masked
#define	CNTCTL_ENABLE		__BIT(0)	// Timer Enabled


// ID_AA64PFR0_EL1: AArch64 Processor Feature Register 0
#define	ID_AA64PFR0_EL1_GIC		__BITS(24,27) // GIC CPU IF
#define	ID_AA64PFR0_EL1_GIC_SHIFT	24
#define	 ID_AA64PFR0_EL1_GIC_CPUIF_EN	 1
#define	 ID_AA64PFR0_EL1_GIC_CPUIF_NONE	 0
#define	ID_AA64PFR0_EL1_ADVSIMD		__BITS(23,20) // SIMD
#define	 ID_AA64PFR0_EL1_ADV_SIMD_IMPL	 0x0
#define	 ID_AA64PFR0_EL1_ADV_SIMD_NONE	 0xf
#define	ID_AA64PFR0_EL1_FP		__BITS(19,16) // FP
#define	 ID_AA64PFR0_EL1_FP_IMPL	 0x0
#define	 ID_AA64PFR0_EL1_FP_NONE	 0xf
#define	ID_AA64PFR0_EL1_EL3		__BITS(15,12) // EL3 handling
#define	 ID_AA64PFR0_EL1_EL3_NONE	 0
#define	 ID_AA64PFR0_EL1_EL3_64		 1
#define	 ID_AA64PFR0_EL1_EL3_64_32	 2
#define	ID_AA64PFR0_EL1_EL2		__BITS(11,8) // EL2 handling
#define	 ID_AA64PFR0_EL1_EL2_NONE	 0
#define	 ID_AA64PFR0_EL1_EL2_64	 	 1
#define	 ID_AA64PFR0_EL1_EL2_64_32	 2
#define	ID_AA64PFR0_EL1_EL1		__BITS(7,4) // EL1 handling
#define	 ID_AA64PFR0_EL1_EL1_64	 	 1
#define	 ID_AA64PFR0_EL1_EL1_64_32	 2
#define	ID_AA64PFR0_EL1_EL0		__BITS(3,0) // EL0 handling
#define	 ID_AA64PFR0_EL1_EL0_64	 	 1
#define	 ID_AA64PFR0_EL1_EL0_64_32	 2

/*
 * GICv3 system registers
 */
AARCH64REG_READWRITE_INLINE2(icc_sre_el1, s3_0_c12_c12_5)
AARCH64REG_READWRITE_INLINE2(icc_ctlr_el1, s3_0_c12_c12_4)
AARCH64REG_READWRITE_INLINE2(icc_pmr_el1, s3_0_c4_c6_0)
AARCH64REG_READWRITE_INLINE2(icc_bpr0_el1, s3_0_c12_c8_3)
AARCH64REG_READWRITE_INLINE2(icc_bpr1_el1, s3_0_c12_c12_3)
AARCH64REG_READWRITE_INLINE2(icc_igrpen0_el1, s3_0_c12_c12_6)
AARCH64REG_READWRITE_INLINE2(icc_igrpen1_el1, s3_0_c12_c12_7)
AARCH64REG_READWRITE_INLINE2(icc_eoir0_el1, s3_0_c12_c8_1)
AARCH64REG_READWRITE_INLINE2(icc_eoir1_el1, s3_0_c12_c12_1)
AARCH64REG_READWRITE_INLINE2(icc_sgi1r_el1, s3_0_c12_c11_5)
AARCH64REG_READ_INLINE2(icc_iar1_el1, s3_0_c12_c12_0)

// ICC_SRE_EL1: Interrupt Controller System Register Enable register
#define	ICC_SRE_EL1_DIB		__BIT(2)
#define	ICC_SRE_EL1_DFB		__BIT(1)
#define	ICC_SRE_EL1_SRE		__BIT(0)

// ICC_SRE_EL2: Interrupt Controller System Register Enable register
#define	ICC_SRE_EL2_EN		__BIT(3)
#define	ICC_SRE_EL2_DIB		__BIT(2)
#define	ICC_SRE_EL2_DFB		__BIT(1)
#define	ICC_SRE_EL2_SRE		__BIT(0)

// ICC_BPR[01]_EL1: Interrupt Controller Binary Point Register 0/1
#define	ICC_BPR_EL1_BinaryPoint	__BITS(2,0)

// ICC_CTLR_EL1: Interrupt Controller Control Register
#define	ICC_CTLR_EL1_A3V	__BIT(15)
#define	ICC_CTLR_EL1_SEIS	__BIT(14)
#define	ICC_CTLR_EL1_IDbits	__BITS(13,11)
#define	ICC_CTLR_EL1_PRIbits	__BITS(10,8)
#define	ICC_CTLR_EL1_PMHE	__BIT(6)
#define	ICC_CTLR_EL1_EOImode	__BIT(1)
#define	ICC_CTLR_EL1_CBPR	__BIT(0)

// ICC_IGRPEN[01]_EL1: Interrupt Controller Interrupt Group 0/1 Enable register
#define	ICC_IGRPEN_EL1_Enable	__BIT(0)

// ICC_SGI[01]R_EL1: Interrupt Controller Software Generated Interrupt Group 0/1 Register
#define	ICC_SGIR_EL1_Aff3	__BITS(55,48)
#define	ICC_SGIR_EL1_IRM	__BIT(40)
#define	ICC_SGIR_EL1_Aff2	__BITS(39,32)
#define	ICC_SGIR_EL1_INTID	__BITS(27,24)
#define	ICC_SGIR_EL1_Aff1	__BITS(23,16)
#define	ICC_SGIR_EL1_TargetList	__BITS(15,0)
#define	ICC_SGIR_EL1_Aff	(ICC_SGIR_EL1_Aff3|ICC_SGIR_EL1_Aff2|ICC_SGIR_EL1_Aff1)

// ICC_IAR[01]_EL1: Interrupt Controller Interrupt Acknowledge Register 0/1
#define	ICC_IAR_INTID		__BITS(23,0)
#define	ICC_IAR_INTID_SPURIOUS	1023

/*
 * GICv3 REGISTER ACCESS
 */

#define	icc_sre_read		reg_icc_sre_el1_read
#define	icc_sre_write		reg_icc_sre_el1_write
#define	icc_pmr_write		reg_icc_pmr_el1_write
#define	icc_bpr0_write		reg_icc_bpr0_el1_write
#define	icc_bpr1_write		reg_icc_bpr1_el1_write
#define	icc_ctlr_read		reg_icc_ctlr_el1_read
#define	icc_ctlr_write		reg_icc_ctlr_el1_write
#define	icc_igrpen1_write	reg_icc_igrpen1_el1_write
#define	icc_sgi1r_write		reg_icc_sgi1r_el1_write
#define	icc_iar1_read		reg_icc_iar1_el1_read
#define	icc_eoi1r_write		reg_icc_eoir1_el1_write

/*
 * GENERIC TIMER REGISTER ACCESS
 */
static __inline uint32_t
gtmr_cntfrq_read(void)
{

	return reg_cntfrq_el0_read();
}

static __inline uint32_t
gtmr_cntk_ctl_read(void)
{

	return reg_cntkctl_el1_read();
}

static __inline void
gtmr_cntk_ctl_write(uint32_t val)
{

	reg_cntkctl_el1_write(val);
}

/*
 * Counter-timer Virtual Count timer
 */
static __inline uint64_t
gtmr_cntpct_read(void)
{

	return reg_cntpct_el0_read();
}

static __inline uint64_t
gtmr_cntvct_read(void)
{

	return reg_cntvct_el0_read();
}

/*
 * Counter-timer Virtual Timer Control register
 */
static __inline uint32_t
gtmr_cntv_ctl_read(void)
{

	return reg_cntv_ctl_el0_read();
}

static __inline void
gtmr_cntv_ctl_write(uint32_t val)
{

	reg_cntv_ctl_el0_write(val);
}

static __inline void
gtmr_cntp_ctl_write(uint32_t val)
{


	reg_cntp_ctl_el0_write(val);
}

/*
 * Counter-timer Virtual Timer TimerValue register
 */
static __inline uint32_t
gtmr_cntv_tval_read(void)
{

	return reg_cntv_tval_el0_read();
}

static __inline void
gtmr_cntv_tval_write(uint32_t val)
{

	reg_cntv_tval_el0_write(val);
}


/*
 * Counter-timer Virtual Timer CompareValue register
 */
static __inline uint64_t
gtmr_cntv_cval_read(void)
{

	return reg_cntv_cval_el0_read();
}

#endif /* _AARCH64_ARMREG_H_ */
