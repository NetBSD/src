/*	$NetBSD: nvmm_x86.c,v 1.7 2019/05/15 04:39:52 maxv Exp $	*/

/*
 * Copyright (c) 2018-2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nvmm_x86.c,v 1.7 2019/05/15 04:39:52 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include <x86/cputypes.h>
#include <x86/specialreg.h>
#include <x86/pmap.h>

#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_internal.h>
#include <dev/nvmm/x86/nvmm_x86.h>

/*
 * Code shared between x86-SVM and x86-VMX.
 */

const struct nvmm_x64_state nvmm_x86_reset_state = {
	.segs = {
		[NVMM_X64_SEG_ES] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_CS] = {
			.selector = 0xF000,
			.base = 0xFFFF0000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_SS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_DS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_FS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_GS] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 3,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_GDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 2,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_IDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = 2,
				.s = 1,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_LDT] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = SDT_SYSLDT,
				.s = 0,
				.p = 1,
			}
		},
		[NVMM_X64_SEG_TR] = {
			.selector = 0x0000,
			.base = 0x00000000,
			.limit = 0xFFFF,
			.attrib = {
				.type = SDT_SYS286BSY,
				.s = 0,
				.p = 1,
			}
		},
	},

	.gprs = {
		[NVMM_X64_GPR_RAX] = 0x00000000,
		[NVMM_X64_GPR_RCX] = 0x00000000,
		[NVMM_X64_GPR_RDX] = 0x00000600,
		[NVMM_X64_GPR_RBX] = 0x00000000,
		[NVMM_X64_GPR_RSP] = 0x00000000,
		[NVMM_X64_GPR_RBP] = 0x00000000,
		[NVMM_X64_GPR_RSI] = 0x00000000,
		[NVMM_X64_GPR_RDI] = 0x00000000,
		[NVMM_X64_GPR_R8] = 0x00000000,
		[NVMM_X64_GPR_R9] = 0x00000000,
		[NVMM_X64_GPR_R10] = 0x00000000,
		[NVMM_X64_GPR_R11] = 0x00000000,
		[NVMM_X64_GPR_R12] = 0x00000000,
		[NVMM_X64_GPR_R13] = 0x00000000,
		[NVMM_X64_GPR_R14] = 0x00000000,
		[NVMM_X64_GPR_R15] = 0x00000000,
		[NVMM_X64_GPR_RIP] = 0x0000FFF0,
		[NVMM_X64_GPR_RFLAGS] = 0x00000002,
	},

	.crs = {
		[NVMM_X64_CR_CR0] = 0x60000010,
		[NVMM_X64_CR_CR2] = 0x00000000,
		[NVMM_X64_CR_CR3] = 0x00000000,
		[NVMM_X64_CR_CR4] = 0x00000000,
		[NVMM_X64_CR_CR8] = 0x00000000,
		[NVMM_X64_CR_XCR0] = 0x00000001,
	},

	.drs = {
		[NVMM_X64_DR_DR0] = 0x00000000,
		[NVMM_X64_DR_DR1] = 0x00000000,
		[NVMM_X64_DR_DR2] = 0x00000000,
		[NVMM_X64_DR_DR3] = 0x00000000,
		[NVMM_X64_DR_DR6] = 0xFFFF0FF0,
		[NVMM_X64_DR_DR7] = 0x00000400,
	},

	.msrs = {
		[NVMM_X64_MSR_EFER] = 0x00000000,
		[NVMM_X64_MSR_STAR] = 0x00000000,
		[NVMM_X64_MSR_LSTAR] = 0x00000000,
		[NVMM_X64_MSR_CSTAR] = 0x00000000,
		[NVMM_X64_MSR_SFMASK] = 0x00000000,
		[NVMM_X64_MSR_KERNELGSBASE] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_CS] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_ESP] = 0x00000000,
		[NVMM_X64_MSR_SYSENTER_EIP] = 0x00000000,
		[NVMM_X64_MSR_PAT] =
		    PATENTRY(0, PAT_WB) | PATENTRY(1, PAT_WT) |
		    PATENTRY(2, PAT_UCMINUS) | PATENTRY(3, PAT_UC) |
		    PATENTRY(4, PAT_WB) | PATENTRY(5, PAT_WT) |
		    PATENTRY(6, PAT_UCMINUS) | PATENTRY(7, PAT_UC),
		[NVMM_X64_MSR_TSC] = 0,
	},

	.intr = {
		.int_shadow = 0,
		.int_window_exiting = 0,
		.nmi_window_exiting = 0,
		.evt_pending = 0,
	},

	.fpu = {
		.fx_cw = 0x0040,
		.fx_sw = 0x0000,
		.fx_tw = 0x55,
		.fx_zero = 0x55,
		.fx_mxcsr = 0x1F80,
	}
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000001 = {
	.eax = ~0,
	.ebx = ~0,
	.ecx =
	    /* Excluded: MONITOR, VMX, SMX, EST, TM2, PDCM, PCID, X2APIC,
	     * DEADLINE, RAZ. */
	    CPUID2_SSE3 | CPUID2_PCLMUL |
	    CPUID2_DTES64 | CPUID2_DS_CPL |
	    CPUID2_SSSE3 | CPUID2_CID |
	    CPUID2_SDBG | CPUID2_FMA |
	    CPUID2_CX16 | CPUID2_xTPR |
	    CPUID2_DCA | CPUID2_SSE41 |
	    CPUID2_SSE42 | CPUID2_MOVBE |
	    CPUID2_POPCNT | CPUID2_AES |
	    CPUID2_XSAVE | CPUID2_OSXSAVE |
	    CPUID2_F16C | CPUID2_RDRAND,
	.edx =
	    /* Excluded: MCE, MTRR, MCA, DS, ACPI, TM. */
	    CPUID_FPU | CPUID_VME |
	    CPUID_DE | CPUID_PSE |
	    CPUID_TSC | CPUID_MSR |
	    CPUID_PAE | CPUID_CX8 |
	    CPUID_APIC | CPUID_B10 |	
	    CPUID_SEP | CPUID_PGE |
	    CPUID_CMOV | CPUID_PAT |
	    CPUID_PSE36 | CPUID_PN |
	    CPUID_CFLUSH | CPUID_B20 |
	    CPUID_MMX | CPUID_FXSR |
	    CPUID_SSE | CPUID_SSE2 |
	    CPUID_SS | CPUID_HTT |
	    CPUID_IA64 | CPUID_SBF
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000007 = {
	.eax = ~0,
	.ebx =
	    /* Excluded: TSC_ADJUST, AVX2, INVPCID, AVX512*, PT, SHA. */
	    CPUID_SEF_FSGSBASE |
	    CPUID_SEF_SGX | CPUID_SEF_BMI1 |
	    CPUID_SEF_HLE | CPUID_SEF_FDPEXONLY |
	    CPUID_SEF_SMEP | CPUID_SEF_BMI2 |
	    CPUID_SEF_ERMS | CPUID_SEF_RTM |
	    CPUID_SEF_QM | CPUID_SEF_FPUCSDS |
	    CPUID_SEF_PQE | CPUID_SEF_RDSEED |
	    CPUID_SEF_ADX | CPUID_SEF_SMAP |
	    CPUID_SEF_CLFLUSHOPT | CPUID_SEF_CLWB,
	.ecx =
	    /* Excluded: AVX512*, MAWAU, RDPID. */
	    CPUID_SEF_PREFETCHWT1 | CPUID_SEF_UMIP |
	    CPUID_SEF_PKU | CPUID_SEF_OSPKE |
	    CPUID_SEF_WAITPKG | CPUID_SEF_GFNI |
	    CPUID_SEF_VAES | CPUID_SEF_VPCLMULQDQ |
	    CPUID_SEF_CLDEMOTE | CPUID_SEF_MOVDIRI |
	    CPUID_SEF_MOVDIR64B | CPUID_SEF_SGXLC,
	.edx =
	    /* Excluded: all except MD_CLEAR and ARCH_CAP. */
	    CPUID_SEF_MD_CLEAR | CPUID_SEF_ARCH_CAP
};

const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000001 = {
	.eax = ~0,
	.ebx = ~0,
	.ecx =
	    /* Excluded: SVM, EAPIC, OSVW. */
	    CPUID_LAHF | CPUID_CMPLEGACY |
	    CPUID_ALTMOVCR0 | CPUID_LZCNT |
	    CPUID_SSE4A | CPUID_MISALIGNSSE |
	    CPUID_3DNOWPF | CPUID_IBS |
	    CPUID_XOP | CPUID_SKINIT |
	    CPUID_WDT | CPUID_LWP |
	    CPUID_FMA4 | CPUID_TCE |
	    CPUID_NODEID | CPUID_TBM |
	    CPUID_TOPOEXT | CPUID_PCEC |
	    CPUID_PCENB | CPUID_SPM |
	    CPUID_DBE | CPUID_PTSC |
	    CPUID_L2IPERFC | CPUID_MWAITX,
	.edx =
	    /* Excluded: RDTSCP. */
	    CPUID_SYSCALL | CPUID_MPC |
	    CPUID_XD | CPUID_MMXX |
	    CPUID_MMX | CPUID_FXSR |
	    CPUID_FFXSR | CPUID_P1GB |
	    CPUID_EM64T | CPUID_3DNOW2 |
	    CPUID_3DNOW
};

bool
nvmm_x86_pat_validate(uint64_t val)
{
	uint8_t *pat = (uint8_t *)&val;
	size_t i;

	for (i = 0; i < 8; i++) {
		if (__predict_false(pat[i] & ~__BITS(2,0)))
			return false;
		if (__predict_false(pat[i] == 2 || pat[i] == 3))
			return false;
	}

	return true;
}
