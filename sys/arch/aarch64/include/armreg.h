/* $NetBSD: armreg.h,v 1.3.2.4 2018/05/21 04:35:57 pgoyette Exp $ */

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
static uint64_t inline						\
reg_##regname##_read(void)					\
{								\
	uint64_t __rv;						\
	__asm __volatile("mrs %0, " #regdesc : "=r"(__rv));	\
	return __rv;						\
}

#define AARCH64REG_WRITE_INLINE2(regname, regdesc)		\
static void inline						\
reg_##regname##_write(uint64_t __val)				\
{								\
	__asm __volatile("msr " #regdesc ", %0" :: "r"(__val));	\
}

#define AARCH64REG_WRITEIMM_INLINE2(regname, regdesc)		\
static void inline						\
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
/*
 * System registers available at EL0 (user)
 */
AARCH64REG_READ_INLINE(ctr_el0)		// Cache Type Register

static const uintmax_t
   CTR_EL0_CWG_LINE	= __BITS(27,24), // Cacheback Writeback Granule
   CTR_EL0_ERG_LINE	= __BITS(23,20), // Exclusives Reservation Granule
   CTR_EL0_DMIN_LINE	= __BITS(19,16), // Dcache MIN LINE size (log2 - 2)
   CTR_EL0_L1IP_MASK	= __BITS(15,14),
   CTR_EL0_L1IP_AIVIVT	= 1, // ASID-tagged Virtual Index, Virtual Tag
   CTR_EL0_L1IP_VIPT	= 2, // Virtual Index, Physical Tag
   CTR_EL0_L1IP_PIPT	= 3, // Physical Index, Physical Tag
   CTR_EL0_IMIN_LINE	= __BITS(3,0); // Icache MIN LINE size (log2 - 2)

AARCH64REG_READ_INLINE(dczid_el0)	// Data Cache Zero ID Register

static const uintmax_t
    DCZID_DZP = __BIT(4),	// Data Zero Prohibited
    DCZID_BS  = __BITS(3,0);	// Block Size (log2 - 2)

AARCH64REG_READ_INLINE(fpcr)		// Floating Point Control Register
AARCH64REG_WRITE_INLINE(fpcr)

static const uintmax_t
    FPCR_AHP    = __BIT(26),	// Alternative Half Precision
    FPCR_DN     = __BIT(25),	// Default Nan Control
    FPCR_FZ     = __BIT(24),	// Flush-To-Zero
    FPCR_RMODE  = __BITS(23,22),// Rounding Mode
     FPCR_RN     = 0,		//  Round Nearest
     FPCR_RP     = 1,		//  Round towards Plus infinity
     FPCR_RM     = 2,		//  Round towards Minus infinity
     FPCR_RZ     = 3,		//  Round towards Zero
    FPCR_STRIDE = __BITS(21,20),
    FPCR_LEN    = __BITS(18,16),
    FPCR_IDE    = __BIT(15),	// Input Denormal Exception enable
    FPCR_IXE    = __BIT(12),	// IneXact Exception enable
    FPCR_UFE    = __BIT(11),	// UnderFlow Exception enable
    FPCR_OFE    = __BIT(10),	// OverFlow Exception enable
    FPCR_DZE    = __BIT(9),	// Divide by Zero Exception enable
    FPCR_IOE    = __BIT(8),	// Invalid Operation Exception enable
    FPCR_ESUM   = 0x1F00;

AARCH64REG_READ_INLINE(fpsr)		// Floating Point Status Register
AARCH64REG_WRITE_INLINE(fpsr)

static const uintmax_t
    FPSR_N32  = __BIT(31), // AARCH32 Negative
    FPSR_Z32  = __BIT(30), // AARCH32 Zero
    FPSR_C32  = __BIT(29), // AARCH32 Carry
    FPSR_V32  = __BIT(28), // AARCH32 Overflow
    FPSR_QC   = __BIT(27), // SIMD Saturation
    FPSR_IDC  = __BIT(7), // Input Denormal Cumulative status
    FPSR_IXC  = __BIT(4), // IneXact Cumulative status
    FPSR_UFC  = __BIT(3), // UnderFlow Cumulative status
    FPSR_OFC  = __BIT(2), // OverFlow Cumulative status
    FPSR_DZC  = __BIT(1), // Divide by Zero Cumulative status
    FPSR_IOC  = __BIT(0), // Invalid Operation Cumulative status
    FPSR_CSUM = 0x1F;

AARCH64REG_READ_INLINE(nzcv)		// condition codes
AARCH64REG_WRITE_INLINE(nzcv)

static const uintmax_t
    NZCV_N = __BIT(31), // Negative
    NZCV_Z = __BIT(30), // Zero
    NZCV_C = __BIT(29), // Carry
    NZCV_V = __BIT(28); // Overflow

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

AARCH64REG_READ_INLINE2(cbar_el1, s3_1_c15_c3_0)	 // Cortex-A57

static const uintmax_t CBAR_PA = __BITS(47,18);

AARCH64REG_READ_INLINE(ccsidr_el1)

static const uintmax_t
    CCSIDR_WT		= __BIT(31),	// Write-through supported
    CCSIDR_WB		= __BIT(30),	// Write-back supported
    CCSIDR_RA		= __BIT(29),	// Read-allocation supported
    CCSIDR_WA		= __BIT(28),	// Write-allocation supported
    CCSIDR_NUMSET	= __BITS(27,13),// (Number of sets in cache) - 1
    CCSIDR_ASSOC	= __BITS(12,3),	// (Associativity of cache) - 1
    CCSIDR_LINESIZE	= __BITS(2,0);	// Number of bytes in cache line

AARCH64REG_READ_INLINE(clidr_el1)

static const uintmax_t
    CLIDR_LOUU   = __BITS(29,27),	// Level of Unification Uniprocessor
    CLIDR_LOC    = __BITS(26,24),	// Level of Coherency
    CLIDR_LOUIS  = __BITS(23,21),	// Level of Unification InnerShareable*/
    CLIDR_CTYPE7 = __BITS(20,18),	// Cache Type field for level7
    CLIDR_CTYPE6 = __BITS(17,15),	// Cache Type field for level6
    CLIDR_CTYPE5 = __BITS(14,12),	// Cache Type field for level5
    CLIDR_CTYPE4 = __BITS(11,9),	// Cache Type field for level4
    CLIDR_CTYPE3 = __BITS(8,6),		// Cache Type field for level3
    CLIDR_CTYPE2 = __BITS(5,3),		// Cache Type field for level2
    CLIDR_CTYPE1 = __BITS(2,0),		// Cache Type field for level1
     CLIDR_TYPE_NOCACHE		= 0,	// No cache
     CLIDR_TYPE_ICACHE		= 1,	// Instruction cache only
     CLIDR_TYPE_DCACHE		= 2,	// Data cache only
     CLIDR_TYPE_IDCACHE		= 3,	// Separate inst and data caches
     CLIDR_TYPE_UNIFIEDCACHE	= 4;	// Unified cache

AARCH64REG_READ_INLINE(currentel)
AARCH64REG_READ_INLINE(id_aa64afr0_el1)
AARCH64REG_READ_INLINE(id_aa64afr1_el1)
AARCH64REG_READ_INLINE(id_aa64dfr0_el1)

static const uintmax_t
    ID_AA64DFR0_EL1_CTX_CMPS	= __BITS(31,28),
    ID_AA64DFR0_EL1_WRPS	= __BITS(20,23),
    ID_AA64DFR0_EL1_BRPS	= __BITS(12,15),
    ID_AA64DFR0_EL1_PMUVER	= __BITS(8,11),
     ID_AA64DFR0_EL1_PMUVER_NONE	= 0,
     ID_AA64DFR0_EL1_PMUVER_V3		= 1,
     ID_AA64DFR0_EL1_PMUVER_NOV3	= 2,
    ID_AA64DFR0_EL1_TRACEVER	= __BITS(4,7),
     ID_AA64DFR0_EL1_TRACEVER_NONE	= 0,
     ID_AA64DFR0_EL1_TRACEVER_IMPL	= 1,
    ID_AA64DFR0_EL1_DEBUGVER	= __BITS(0,3),
     ID_AA64DFR0_EL1_DEBUGVER_V8A	= 6;

AARCH64REG_READ_INLINE(id_aa64dfr1_el1)

AARCH64REG_READ_INLINE(id_aa64isar0_el1)

static const uintmax_t
    ID_AA64ISAR0_EL1_CRC32	= __BITS(19,16),
     ID_AA64ISAR0_EL1_CRC32_NONE	= 0,
     ID_AA64ISAR0_EL1_CRC32_CRC32X	= 1,
    ID_AA64ISAR0_EL1_SHA2	= __BITS(15,12),
     ID_AA64ISAR0_EL1_SHA2_NONE		= 0,
     ID_AA64ISAR0_EL1_SHA2_SHA256HSU	= 1,
    ID_AA64ISAR0_EL1_SHA1	= __BITS(11,8),
     ID_AA64ISAR0_EL1_SHA1_NONE		= 0,
     ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU	= 1,
    ID_AA64ISAR0_EL1_AES	= __BITS(7,4),
     ID_AA64ISAR0_EL1_AES_NONE		= 0,
     ID_AA64ISAR0_EL1_AES_AES		= 1,
     ID_AA64ISAR0_EL1_AES_PMUL		= 2;

AARCH64REG_READ_INLINE(id_aa64isar1_el1)
AARCH64REG_READ_INLINE(id_aa64mmfr0_el1)

static const uintmax_t
    ID_AA64MMFR0_EL1_TGRAN4	= __BITS(31,28),
     ID_AA64MMFR0_EL1_TGRAN4_4KB	= 0,
     ID_AA64MMFR0_EL1_TGRAN4_NONE	= 15,
    ID_AA64MMFR0_EL1_TGRAN64	= __BITS(24,27),
     ID_AA64MMFR0_EL1_TGRAN64_64KB	= 0,
     ID_AA64MMFR0_EL1_TGRAN64_NONE	= 15,
    ID_AA64MMFR0_EL1_TGRAN16	= __BITS(20,23),
     ID_AA64MMFR0_EL1_TGRAN16_NONE	= 0,
     ID_AA64MMFR0_EL1_TGRAN16_16KB	= 1,
    ID_AA64MMFR0_EL1_BIGENDEL0	= __BITS(16,19),
     ID_AA64MMFR0_EL1_BIGENDEL0_NONE	= 0,
     ID_AA64MMFR0_EL1_BIGENDEL0_MIX	= 1,
    ID_AA64MMFR0_EL1_SNSMEM	= __BITS(12,15),
     ID_AA64MMFR0_EL1_SNSMEM_NONE	= 0,
     ID_AA64MMFR0_EL1_SNSMEM_SNSMEM	= 1,
    ID_AA64MMFR0_EL1_BIGEND	= __BITS(8,11),
     ID_AA64MMFR0_EL1_BIGEND_NONE	= 0,
     ID_AA64MMFR0_EL1_BIGEND_MIX	= 1,
    ID_AA64MMFR0_EL1_ASIDBITS	= __BITS(4,7),
     ID_AA64MMFR0_EL1_ASIDBITS_8BIT	= 0,
     ID_AA64MMFR0_EL1_ASIDBITS_16BIT	= 2,
    ID_AA64MMFR0_EL1_PARANGE	= __BITS(0,3),
     ID_AA64MMFR0_EL1_PARANGE_4G	= 0,
     ID_AA64MMFR0_EL1_PARANGE_64G	= 1,
     ID_AA64MMFR0_EL1_PARANGE_1T	= 2,
     ID_AA64MMFR0_EL1_PARANGE_4T	= 3,
     ID_AA64MMFR0_EL1_PARANGE_16T	= 4,
     ID_AA64MMFR0_EL1_PARANGE_256T	= 5;

AARCH64REG_READ_INLINE(id_aa64mmfr1_el1)
AARCH64REG_READ_INLINE(id_aa64pfr0_el1)
AARCH64REG_READ_INLINE(id_aa64pfr1_el1)
AARCH64REG_READ_INLINE(id_pfr1_el1)
AARCH64REG_READ_INLINE(isr_el1)
AARCH64REG_READ_INLINE(midr_el1)
AARCH64REG_READ_INLINE(mpidr_el1)

static const uintmax_t
    MPIDR_AFF3		= __BITS(32,39),
    MPIDR_U		= __BIT(30),		// 1 = Uni-Processor System
    MPIDR_MT		= __BIT(24),		// 1 = SMT(AFF0 is logical)
    MPIDR_AFF2		= __BITS(16,23),
    MPIDR_AFF1		= __BITS(8,15),
    MPIDR_AFF0		= __BITS(0,7);

AARCH64REG_READ_INLINE(mvfr0_el1)

static const uintmax_t
    MVFR0_FPROUND	= __BITS(31,28),
     MVFR0_FPROUND_NEAREST	= 0,
     MVFR0_FPROUND_ALL		= 1,
    MVFR0_FPSHVEC	= __BITS(27,24),
     MVFR0_FPSHVEC_NONE		= 0,
     MVFR0_FPSHVEC_SHVEC	= 1,
    MVFR0_FPSQRT	= __BITS(23,20),
     MVFR0_FPSQRT_NONE		= 0,
     MVFR0_FPSQRT_VSQRT		= 1,
    MVFR0_FPDIVIDE	= __BITS(19,16),
     MVFR0_FPDIVIDE_NONE	= 0,
     MVFR0_FPDIVIDE_VDIV	= 1,
    MVFR0_FPTRAP	= __BITS(15,12),
     MVFR0_FPTRAP_NONE		= 0,
     MVFR0_FPTRAP_TRAP		= 1,
    MVFR0_FPDP		= __BITS(11,8),
     MVFR0_FPDP_NONE		= 0,
     MVFR0_FPDP_VFPV2		= 1,
     MVFR0_FPDP_VFPV3		= 2,
    MVFR0_FPSP		= __BITS(7,4),
     MVFR0_FPSP_NONE		= 0,
     MVFR0_FPSP_VFPV2		= 1,
     MVFR0_FPSP_VFPV3		= 2,
    MVFR0_SIMDREG	= __BITS(3,0),
     MVFR0_SIMDREG_NONE		= 0,
     MVFR0_SIMDREG_16x64	= 1,
     MVFR0_SIMDREG_32x64	= 2;

AARCH64REG_READ_INLINE(mvfr1_el1)

static const uintmax_t
    MVFR1_SIMDFMAC	= __BITS(31,28),
     MVFR1_SIMDFMAC_NONE	= 0,
     MVFR1_SIMDFMAC_FMAC	= 1,
    MVFR1_FPHP		= __BITS(27,24),
     MVFR1_FPHP_NONE		= 0,
     MVFR1_FPHP_HALF_SINGLE	= 1,
     MVFR1_FPHP_HALF_DOUBLE	= 2,
    MVFR1_SIMDHP	= __BITS(23,20),
     MVFR1_SIMDHP_NONE		= 0,
     MVFR1_SIMDHP_HALF		= 1,
    MVFR1_SIMDSP	= __BITS(19,16),
     MVFR1_SIMDSP_NONE		= 0,
     MVFR1_SIMDSP_SINGLE	= 1,
    MVFR1_SIMDINT	= __BITS(15,12),
     MVFR1_SIMDINT_NONE		= 0,
     MVFR1_SIMDINT_INTEGER	= 1,
    MVFR1_SIMDLS	= __BITS(11,8),
     MVFR1_SIMDLS_NONE		= 0,
     MVFR1_SIMDLS_LOADSTORE	= 1,
    MVFR1_FPDNAN	= __BITS(7,4),
     MVFR1_FPDNAN_NONE		= 0,
     MVFR1_FPDNAN_NAN		= 1,
    MVFR1_FPFTZ		= __BITS(3,0),
     MVFR1_FPFTZ_NONE		= 0,
     MVFR1_FPFTZ_DENORMAL	= 1;

AARCH64REG_READ_INLINE(mvfr2_el1)

static const uintmax_t
    MVFR2_FPMISC	= __BITS(7,4),
     MVFR2_FPMISC_NONE		= 0,
     MVFR2_FPMISC_SEL		= 1,
     MVFR2_FPMISC_DROUND	= 2,
     MVFR2_FPMISC_ROUNDINT	= 3,
     MVFR2_FPMISC_MAXMIN	= 4,
    MVFR2_SIMDMISC	= __BITS(3,0),
     MVFR2_SIMDMISC_NONE	= 0,
     MVFR2_SIMDMISC_DROUND	= 1,
     MVFR2_SIMDMISC_ROUNDINT	= 2,
     MVFR2_SIMDMISC_MAXMIN	= 3;

AARCH64REG_READ_INLINE(revidr_el1)

/*
 * These are read/write registers
 */
AARCH64REG_READ_INLINE(cpacr_el1)	// Coprocessor Access Control Regiser
AARCH64REG_WRITE_INLINE(cpacr_el1)

static const uintmax_t
    CPACR_TTA		= __BIT(28),	 // System Register Access Traps
    CPACR_FPEN		= __BITS(21,20),
    CPACR_FPEN_NONE	= __SHIFTIN(0, CPACR_FPEN),
    CPACR_FPEN_EL1	= __SHIFTIN(1, CPACR_FPEN),
    CPACR_FPEN_NONE_2	= __SHIFTIN(2, CPACR_FPEN),
    CPACR_FPEN_ALL	= __SHIFTIN(3, CPACR_FPEN);

AARCH64REG_READ_INLINE(csselr_el1)	// Cache Size Selection Register
AARCH64REG_WRITE_INLINE(csselr_el1)

static const uintmax_t
    CSSELR_LEVEL	= __BITS(3,1),	// Cache level of required cache
    CSSELR_IND		= __BIT(0);	// Instruction not Data bit

AARCH64REG_READ_INLINE(daif)		// Debug Async Irq Fiq mask register
AARCH64REG_WRITE_INLINE(daif)
AARCH64REG_WRITEIMM_INLINE(daifclr)
AARCH64REG_WRITEIMM_INLINE(daifset)

static const uintmax_t
    DAIF_D		= __BIT(9),	// Debug Exception Mask
    DAIF_A		= __BIT(8),	// SError Abort Mask
    DAIF_I		= __BIT(7),	// IRQ Mask
    DAIF_F		= __BIT(6),	// FIQ Mask
    DAIF_SETCLR_SHIFT	= 6;		// for daifset/daifclr #imm shift

AARCH64REG_READ_INLINE(elr_el1)		// Exception Link Register
AARCH64REG_WRITE_INLINE(elr_el1)

AARCH64REG_READ_INLINE(esr_el1)		// Exception Symdrone Register
AARCH64REG_WRITE_INLINE(esr_el1)

static const uintmax_t
    ESR_EC = 		__BITS(31,26), // Exception Cause
     ESR_EC_UNKNOWN		= 0x00,	// AXX: Unknown Reason
     ESR_EC_WFX			= 0x01,	// AXX: WFI or WFE instruction execution
     ESR_EC_CP15_RT		= 0x03,	// A32: MCR/MRC access to CP15 !EC=0
     ESR_EC_CP15_RRT		= 0x04,	// A32: MCRR/MRRC access to CP15 !EC=0
     ESR_EC_CP14_RT		= 0x05,	// A32: MCR/MRC access to CP14
     ESR_EC_CP14_DT		= 0x06,	// A32: LDC/STC access to CP14
     ESR_EC_FP_ACCESS		= 0x07,	// AXX: Access to SIMD/FP Registers
     ESR_EC_FPID		= 0x08,	// A32: MCR/MRC access to CP10 !EC=7
     ESR_EC_CP14_RRT		= 0x0c,	// A32: MRRC access to CP14
     ESR_EC_ILL_STATE		= 0x0e,	// AXX: Illegal Execution State
     ESR_EC_SVC_A32		= 0x11,	// A32: SVC Instruction Execution
     ESR_EC_HVC_A32		= 0x12,	// A32: HVC Instruction Execution
     ESR_EC_SMC_A32		= 0x13,	// A32: SMC Instruction Execution
     ESR_EC_SVC_A64		= 0x15,	// A64: SVC Instruction Execution
     ESR_EC_HVC_A64		= 0x16,	// A64: HVC Instruction Execution
     ESR_EC_SMC_A64		= 0x17,	// A64: SMC Instruction Execution
     ESR_EC_SYS_REG		= 0x18,	// A64: MSR/MRS/SYS instruction (!EC0/1/7)
     ESR_EC_INSN_ABT_EL0	= 0x20, // AXX: Instruction Abort (EL0)
     ESR_EC_INSN_ABT_EL1	= 0x21, // AXX: Instruction Abort (EL1)
     ESR_EC_PC_ALIGNMENT	= 0x22, // AXX: Misaligned PC
     ESR_EC_DATA_ABT_EL0	= 0x24, // AXX: Data Abort (EL0)
     ESR_EC_DATA_ABT_EL1	= 0x25, // AXX: Data Abort (EL1)
     ESR_EC_SP_ALIGNMENT	= 0x26, // AXX: Misaligned SP
     ESR_EC_FP_TRAP_A32		= 0x28,	// A32: FP Exception
     ESR_EC_FP_TRAP_A64		= 0x2c,	// A64: FP Exception
     ESR_EC_SERROR		= 0x2f,	// AXX: SError Interrupt
     ESR_EC_BRKPNT_EL0		= 0x30,	// AXX: Breakpoint Exception (EL0)
     ESR_EC_BRKPNT_EL1		= 0x31,	// AXX: Breakpoint Exception (EL1)
     ESR_EC_SW_STEP_EL0		= 0x32,	// AXX: Software Step (EL0)
     ESR_EC_SW_STEP_EL1		= 0x33,	// AXX: Software Step (EL1)
     ESR_EC_WTCHPNT_EL0		= 0x34,	// AXX: Watchpoint (EL0)
     ESR_EC_WTCHPNT_EL1		= 0x35,	// AXX: Watchpoint (EL1)
     ESR_EC_BKPT_INSN_A32 = 0x38,	// A32: BKPT Instruction Execution
     ESR_EC_VECTOR_CATCH = 0x3a,	// A32: Vector Catch Exception
     ESR_EC_BKPT_INSN_A64 = 0x3c,	// A64: BKPT Instruction Execution
    ESR_IL = 		__BIT(25), // Instruction Length (1=32-bit)
    ESR_ISS = 		__BITS(24,0), // Instruction Specific Syndrome
    ESR_ISS_CV =		__BIT(24),	// common
    ESR_ISS_COND =		__BITS(23,20),	// common
    ESR_ISS_WFX_TRAP_INSN =	__BIT(0),	// for ESR_EC_WFX
    ESR_ISS_MRC_OPC2 =		__BITS(19,17),	// for ESR_EC_CP15_RT
    ESR_ISS_MRC_OPC1 =		__BITS(16,14),	// for ESR_EC_CP15_RT
    ESR_ISS_MRC_CRN =		__BITS(13,10),	// for ESR_EC_CP15_RT
    ESR_ISS_MRC_RT =		__BITS(9,5),	// for ESR_EC_CP15_RT
    ESR_ISS_MRC_CRM =		__BITS(4,1),	// for ESR_EC_CP15_RT
    ESR_ISS_MRC_DIRECTION =	__BIT(0),	// for ESR_EC_CP15_RT
    ESR_ISS_MCRR_OPC1 =		__BITS(19,16),	// for ESR_EC_CP15_RRT
    ESR_ISS_MCRR_RT2 =		__BITS(14,10),	// for ESR_EC_CP15_RRT
    ESR_ISS_MCRR_RT =		__BITS(9,5),	// for ESR_EC_CP15_RRT
    ESR_ISS_MCRR_CRM =		__BITS(4,1),	// for ESR_EC_CP15_RRT
    ESR_ISS_MCRR_DIRECTION =	__BIT(0),	// for ESR_EC_CP15_RRT
    ESR_ISS_HVC_IMM16 =		__BITS(15,0),	// for ESR_EC_{SVC,HVC}
    // ...
    ESR_ISS_INSNABORT_EA =	__BIT(9),	// for ESC_RC_INSN_ABT_EL[01]
    ESR_ISS_INSNABORT_S1PTW =	__BIT(7),	// for ESC_RC_INSN_ABT_EL[01]
    ESR_ISS_INSNABORT_IFSC =	__BITS(0,5),	// for ESC_RC_INSN_ABT_EL[01]
    ESR_ISS_DATAABORT_ISV =	__BIT(24),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_SAS =	__BITS(23,22),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_SSE =	__BIT(21),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_SRT =	__BITS(19,16),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_SF =	__BIT(15),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_AR =	__BIT(14),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_EA =	__BIT(9),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_CM =	__BIT(8),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_S1PTW =	__BIT(7),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_WnR =	__BIT(6),	// for ESC_RC_DATA_ABT_EL[01]
    ESR_ISS_DATAABORT_DFSC =	__BITS(0,5);	// for ESC_RC_DATA_ABT_EL[01]

static const uintmax_t	// ESR_ISS_{INSN,DATA}ABORT_FSC
    ESR_ISS_FSC_ADDRESS_SIZE_FAULT_0		= 0x00,
    ESR_ISS_FSC_ADDRESS_SIZE_FAULT_1		= 0x01,
    ESR_ISS_FSC_ADDRESS_SIZE_FAULT_2		= 0x02,
    ESR_ISS_FSC_ADDRESS_SIZE_FAULT_3		= 0x03,
    ESR_ISS_FSC_TRANSLATION_FAULT_0		= 0x04,
    ESR_ISS_FSC_TRANSLATION_FAULT_1		= 0x05,
    ESR_ISS_FSC_TRANSLATION_FAULT_2		= 0x06,
    ESR_ISS_FSC_TRANSLATION_FAULT_3		= 0x07,
    ESR_ISS_FSC_ACCESS_FAULT_0			= 0x08,
    ESR_ISS_FSC_ACCESS_FAULT_1			= 0x09,
    ESR_ISS_FSC_ACCESS_FAULT_2			= 0x0a,
    ESR_ISS_FSC_ACCESS_FAULT_3			= 0x0b,
    ESR_ISS_FSC_PERM_FAULT_0			= 0x0c,
    ESR_ISS_FSC_PERM_FAULT_1			= 0x0d,
    ESR_ISS_FSC_PERM_FAULT_2			= 0x0e,
    ESR_ISS_FSC_PERM_FAULT_3			= 0x0f,
    ESR_ISS_FSC_SYNC_EXTERNAL_ABORT		= 0x10,
    ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_0	= 0x14,
    ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_1	= 0x15,
    ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_2	= 0x16,
    ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_3	= 0x17,
    ESR_ISS_FSC_SYNC_PARITY_ERROR		= 0x18,
    ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_0	= 0x1c,
    ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_1	= 0x1d,
    ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_2	= 0x1e,
    ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_3	= 0x1f,
    ESR_ISS_FSC_ALIGNMENT_FAULT			= 0x21,
    ESR_ISS_FSC_TLB_CONFLICT_FAULT		= 0x30,
    ESR_ISS_FSC_LOCKDOWN_ABORT			= 0x34,
    ESR_ISS_FSC_UNSUPPORTED_EXCLUSIVE		= 0x35,
    ESR_ISS_FSC_FIRST_LEVEL_DOMAIN_FAULT	= 0x3d,
    ESR_ISS_FSC_SECOND_LEVEL_DOMAIN_FAULT	= 0x3e;


AARCH64REG_READ_INLINE(far_el1)		// Fault Address Register
AARCH64REG_WRITE_INLINE(far_el1)

AARCH64REG_READ_INLINE2(l2ctlr_el1, s3_1_c11_c0_2)  // Cortex-A53,57,72,73
AARCH64REG_WRITE_INLINE2(l2ctlr_el1, s3_1_c11_c0_2) // Cortex-A53,57,72,73

static const uintmax_t
    L2CTLR_NUMOFCORE		= __BITS(25,24),// Number of cores
    L2CTLR_CPUCACHEPROT		= __BIT(22),	// CPU Cache Protection
    L2CTLR_SCUL2CACHEPROT	= __BIT(21),	// SCU-L2 Cache Protection
    L2CTLR_L2_INPUT_LATENCY	= __BIT(5),	// L2 Data RAM input latency
    L2CTLR_L2_OUTPUT_LATENCY	= __BIT(0);	// L2 Data RAM output latency

AARCH64REG_READ_INLINE(mair_el1) // Memory Attribute Indirection Register
AARCH64REG_WRITE_INLINE(mair_el1)

static const uintmax_t
    MAIR_ATTR0		= __BITS(7,0),
    MAIR_ATTR1		= __BITS(15,8),
    MAIR_ATTR2		= __BITS(23,16),
    MAIR_ATTR3		= __BITS(31,24),
    MAIR_ATTR4		= __BITS(39,32),
    MAIR_ATTR5		= __BITS(47,40),
    MAIR_ATTR6		= __BITS(55,48),
    MAIR_ATTR7		= __BITS(63,56),
    MAIR_DEVICE_nGnRnE	= 0x00,	// NoGathering,NoReordering,NoEarlyWriteAck.
    MAIR_NORMAL_NC	= 0x44,
    MAIR_NORMAL_WT	= 0xbb,
    MAIR_NORMAL_WB	= 0xff;

AARCH64REG_READ_INLINE(par_el1)		// Physical Address Register
AARCH64REG_WRITE_INLINE(par_el1)

static const uintmax_t
    PAR_ATTR		= __BITS(63,56),// F=0 memory attributes
    PAR_PA		= __BITS(47,12),// F=0 physical address
    PAR_NS		= __BIT(9),	// F=0 non-secure
    PAR_S		= __BIT(9),	// F=1 failure stage
    PAR_SHA		= __BITS(8,7),	// F=0 shareability attribute
     PAR_SHA_NONE	= 0,
     PAR_SHA_OUTER	= 2,
     PAR_SHA_INNER	= 3,
    PAR_PTW		= __BIT(8),	// F=1 partial table walk
    PAR_FST		= __BITS(6,1),	// F=1 fault status code
    PAR_F		= __BIT(0);	// translation failed

AARCH64REG_READ_INLINE(rmr_el1)		// Reset Management Register
AARCH64REG_WRITE_INLINE(rmr_el1)

AARCH64REG_READ_INLINE(rvbar_el1)	// Reset Vector Base Address Register
AARCH64REG_WRITE_INLINE(rvbar_el1)

AARCH64REG_READ_INLINE(sctlr_el1)	// System Control Register
AARCH64REG_WRITE_INLINE(sctlr_el1)

static const uintmax_t
    SCTLR_RES0		= 0xc8222400,	// Reserved ARMv8.0, write 0
    SCTLR_RES1		= 0x30d00800,	// Reserved ARMv8.0, write 1
    SCTLR_M		= __BIT(0),
    SCTLR_A		= __BIT(1),
    SCTLR_C		= __BIT(2),
    SCTLR_SA		= __BIT(3),
    SCTLR_SA0		= __BIT(4),
    SCTLR_CP15BEN	= __BIT(5),
    SCTLR_THEE		= __BIT(6),
    SCTLR_ITD		= __BIT(7),
    SCTLR_SED		= __BIT(8),
    SCTLR_UMA		= __BIT(9),
    SCTLR_I		= __BIT(12),
    SCTLR_DZE		= __BIT(14),
    SCTLR_UCT		= __BIT(15),
    SCTLR_nTWI		= __BIT(16),
    SCTLR_nTWE		= __BIT(18),
    SCTLR_WXN		= __BIT(19),
    SCTLR_IESB		= __BIT(21),
    SCTLR_SPAN		= __BIT(23),
    SCTLR_EOE		= __BIT(24),
    SCTLR_EE		= __BIT(25),
    SCTLR_UCI		= __BIT(26),
    SCTLR_nTLSMD	= __BIT(28),
    SCTLR_LSMAOE	= __BIT(29);

// current EL stack pointer
static uint64_t inline
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

static const uintmax_t
    SPSEL_SP		= __BIT(0);	// use SP_EL0 at all exception levels

AARCH64REG_READ_INLINE(spsr_el1)	// Saved Program Status Register
AARCH64REG_WRITE_INLINE(spsr_el1)

static const uintmax_t
    SPSR_NZCV	  = __BITS(31,28),	// mask of N Z C V
     SPSR_N	  = __BIT(31),		// Negative
     SPSR_Z	  = __BIT(30),		// Zero
     SPSR_C	  = __BIT(29),		// Carry
     SPSR_V	  = __BIT(28),		// oVerflow
    SPSR_A32_Q	  = __BIT(27),		// A32: Overflow
    SPSR_A32_J	  = __BIT(24),		// A32: Jazelle Mode
    SPSR_A32_IT1  = __BIT(23),		// A32: IT[1]
    SPSR_A32_IT0  = __BIT(22),		// A32: IT[0]
    SPSR_SS	  = __BIT(21),		// Software Step
    SPSR_IL	  = __BIT(20),		// Instruction Length
    SPSR_GE	  = __BITS(19,16),	// A32: SIMD GE
    SPSR_IT7	  = __BIT(15),		// A32: IT[7]
    SPSR_IT6	  = __BIT(14),		// A32: IT[6]
    SPSR_IT5	  = __BIT(13),		// A32: IT[5]
    SPSR_IT4	  = __BIT(12),		// A32: IT[4]
    SPSR_IT3	  = __BIT(11),		// A32: IT[3]
    SPSR_IT2	  = __BIT(10),		// A32: IT[2]
    SPSR_A64_D	  = __BIT(9),		// A64: Debug Exception Mask
    SPSR_A32_E	  = __BIT(9),		// A32: BE Endian Mode
    SPSR_A	  = __BIT(8),		// Async abort (SError) Mask
    SPSR_I	  = __BIT(7),		// IRQ Mask
    SPSR_F	  = __BIT(6),		// FIQ Mask
    SPSR_A32_T	  = __BIT(5),		// A32 Thumb Mode
    SPSR_M	  = __BITS(4,0),	// Execution State
     SPSR_M_EL3H  = 0x0d,
     SPSR_M_EL3T  = 0x0c,
     SPSR_M_EL2H  = 0x09,
     SPSR_M_EL2T  = 0x08,
     SPSR_M_EL1H  = 0x05,
     SPSR_M_EL1T  = 0x04,
     SPSR_M_EL0T  = 0x00,
     SPSR_M_SYS32 = 0x1f,
     SPSR_M_UND32 = 0x1b,
     SPSR_M_ABT32 = 0x17,
     SPSR_M_SVC32 = 0x13,
     SPSR_M_IRQ32 = 0x12,
     SPSR_M_FIQ32 = 0x11,
     SPSR_M_USR32 = 0x10;

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

static const uintmax_t
    DBGBCR_BT		= __BITS(23,20),
    DBGBCR_LBN		= __BITS(19,16),
    DBGBCR_SSC		= __BITS(15,14),
    DBGBCR_HMC		= __BIT(13),
    DBGBCR_BAS		= __BITS(8,5),
    DBGBCR_PMC		= __BITS(2,1),
    DBGBCR_E		= __BIT(0);

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

static const uintmax_t
    DBGWCR_MASK		= __BITS(28,24),
    DBGWCR_WT		= __BIT(20),
    DBGWCR_LBN		= __BITS(19,16),
    DBGWCR_SSC		= __BITS(15,14),
    DBGWCR_HMC		= __BIT(13),
    DBGWCR_BAS		= __BITS(12,5),
    DBGWCR_LSC		= __BITS(4,3),
    DBGWCR_PAC		= __BITS(2,1),
    DBGWCR_E		= __BIT(0);

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

static const uintmax_t
    DBGWVR_MASK		= __BITS(64,3);


AARCH64REG_READ_INLINE(mdscr_el1) // Monitor Debug System Control Register
AARCH64REG_WRITE_INLINE(mdscr_el1)

AARCH64REG_WRITE_INLINE(oslar_el1)	// OS Lock Access Register

AARCH64REG_READ_INLINE(oslsr_el1)	// OS Lock Status Register

/*
 * From here on, these are PMC registers
 */

AARCH64REG_READ_INLINE(pmccfiltr_el0)
AARCH64REG_WRITE_INLINE(pmccfiltr_el0)

static const uintmax_t
    PMCCFILTR_P	  = __BIT(31),	 // Don't count cycles in EL1
    PMCCFILTR_U	  = __BIT(30),	 // Don't count cycles in EL0
    PMCCFILTR_NSK = __BIT(29),	 // Don't count cycles in NS EL1
    PMCCFILTR_NSU = __BIT(28),	 // Don't count cycles in NS EL0
    PMCCFILTR_NSH = __BIT(27),	 // Don't count cycles in NS EL2
    PMCCFILTR_M	  = __BIT(26);	 // Don't count cycles in EL3

AARCH64REG_READ_INLINE(pmccntr_el0)

AARCH64REG_READ_INLINE(cntfrq_el0)

AARCH64REG_READ_INLINE(cnthctl_el2)
AARCH64REG_WRITE_INLINE(cnthctl_el2)

static const uintmax_t
    CNTHCTL_EVNTDIR	= __BIT(3),
    CNTHCTL_EVNTEN	= __BIT(2),
    CNTHCTL_EL1PCEN	= __BIT(1),
    CNTHCTL_EL1PCTEN	= __BIT(0);

AARCH64REG_READ_INLINE(cntkctl_el1)
AARCH64REG_WRITE_INLINE(cntkctl_el1)

static const uintmax_t
    CNTKCTL_EL0PTEN	= __BIT(9),	// EL0 access for CNTP CVAL/TVAL/CTL
    CNTKCTL_PL0PTEN	= CNTKCTL_EL0PTEN,
    CNTKCTL_EL0VTEN	= __BIT(8),	// EL0 access for CNTV CVAL/TVAL/CTL
    CNTKCTL_PL0VTEN	= CNTKCTL_EL0VTEN,
    CNTKCTL_ELNTI	= __BITS(7,4),
    CNTKCTL_EVNTDIR	= __BIT(3),
    CNTKCTL_EVNTEN	= __BIT(2),
    CNTKCTL_EL0VCTEN	= __BIT(1),	// EL0 access for CNTVCT and CNTFRQ
    CNTKCTL_PL0VCTEN	= CNTKCTL_EL0VCTEN,
    CNTKCTL_EL0PCTEN	= __BIT(0),	// EL0 access for CNTPCT and CNTFRQ
    CNTKCTL_PL0PCTEN	= CNTKCTL_EL0PCTEN;

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

static const uintmax_t
    CNTCTL_ISTATUS = __BIT(2),	// Interrupt Asserted
    CNTCTL_IMASK   = __BIT(1),	// Timer Interrupt is Masked
    CNTCTL_ENABLE  = __BIT(0);	// Timer Enabled


// ID_AA64PFR0_EL1: AArch64 Processor Feature Register 0
static const uintmax_t
    ID_AA64PFR0_EL1_GIC			= __BITS(24,27), // GIC CPU IF
    ID_AA64PFR0_EL1_GIC_SHIFT		= 24,
     ID_AA64PFR0_EL1_GIC_CPUIF_EN	= 1,
     ID_AA64PFR0_EL1_GIC_CPUIF_NONE	= 0,
    ID_AA64PFR0_EL1_ADVSIMD		= __BITS(23,20), // SIMD
     ID_AA64PFR0_EL1_ADV_SIMD_IMPL	= 0x0,
     ID_AA64PFR0_EL1_ADV_SIMD_NONE	= 0xf,
    ID_AA64PFR0_EL1_FP			= __BITS(19,16), // FP
     ID_AA64PFR0_EL1_FP_IMPL		= 0x0,
     ID_AA64PFR0_EL1_FP_NONE		= 0xf,
    ID_AA64PFR0_EL1_EL3			= __BITS(15,12), // EL3 handling
     ID_AA64PFR0_EL1_EL3_NONE		= 0,
     ID_AA64PFR0_EL1_EL3_64		= 1,
     ID_AA64PFR0_EL1_EL3_64_32		= 2,
    ID_AA64PFR0_EL1_EL2			= __BITS(11,8), // EL2 handling
     ID_AA64PFR0_EL1_EL2_NONE		= 0,
     ID_AA64PFR0_EL1_EL2_64		= 1,
     ID_AA64PFR0_EL1_EL2_64_32		= 2,
    ID_AA64PFR0_EL1_EL1			= __BITS(7,4), // EL1 handling
     ID_AA64PFR0_EL1_EL1_64		= 1,
     ID_AA64PFR0_EL1_EL1_64_32		= 2,
    ID_AA64PFR0_EL1_EL0			= __BITS(3,0), // EL0 handling
     ID_AA64PFR0_EL1_EL0_64		= 1,
     ID_AA64PFR0_EL1_EL0_64_32		= 2;

// ICC_SRE_EL1: Interrupt Controller System Register Enable register
static const uintmax_t
    ICC_SRE_EL1_SRE			= __BIT(0),
    ICC_SRE_EL1_DFB			= __BIT(1),
    ICC_SRE_EL1_DIB			= __BIT(2);

// ICC_SRE_EL2: Interrupt Controller System Register Enable register
static const uintmax_t
    ICC_SRE_EL2_SRE			= __BIT(0),
    ICC_SRE_EL2_DFB			= __BIT(1),
    ICC_SRE_EL2_DIB			= __BIT(2),
    ICC_SRE_EL2_EN			= __BIT(3);


/*
 * GENERIC TIMER REGISTER ACCESS
 */
static inline uint32_t
gtmr_cntfrq_read(void)
{

	return reg_cntfrq_el0_read();
}

static inline uint32_t
gtmr_cntk_ctl_read(void)
{

	return reg_cntkctl_el1_read();
}

static inline void
gtmr_cntk_ctl_write(uint32_t val)
{

	reg_cntkctl_el1_write(val);
}

/*
 * Counter-timer Virtual Count timer
 */
static inline uint64_t
gtmr_cntpct_read(void)
{

	return reg_cntpct_el0_read();
}

static inline uint64_t
gtmr_cntvct_read(void)
{

	return reg_cntvct_el0_read();
}

/*
 * Counter-timer Virtual Timer Control register
 */
static inline uint32_t
gtmr_cntv_ctl_read(void)
{

	return reg_cntv_ctl_el0_read();
}

static inline void
gtmr_cntv_ctl_write(uint32_t val)
{

	reg_cntv_ctl_el0_write(val);
}

static inline void
gtmr_cntp_ctl_write(uint32_t val)
{


	reg_cntp_ctl_el0_write(val);
}

/*
 * Counter-timer Virtual Timer TimerValue register
 */
static inline uint32_t
gtmr_cntv_tval_read(void)
{

	return reg_cntv_tval_el0_read();
}

static inline void
gtmr_cntv_tval_write(uint32_t val)
{

	reg_cntv_tval_el0_write(val);
}


/*
 * Counter-timer Virtual Timer CompareValue register
 */
static inline uint64_t
gtmr_cntv_cval_read(void)
{

	return reg_cntv_cval_el0_read();
}

#endif /* _AARCH64_ARMREG_H_ */
