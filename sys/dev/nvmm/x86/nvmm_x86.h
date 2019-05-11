/*	$NetBSD: nvmm_x86.h,v 1.15 2019/05/11 07:31:56 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef _NVMM_X86_H_
#define _NVMM_X86_H_

/* --------------------------------------------------------------------- */

#ifndef ASM_NVMM

struct nvmm_exit_memory {
	int prot;
	gpaddr_t gpa;
	uint8_t inst_len;
	uint8_t inst_bytes[15];
};

enum nvmm_exit_io_type {
	NVMM_EXIT_IO_IN,
	NVMM_EXIT_IO_OUT
};

struct nvmm_exit_io {
	enum nvmm_exit_io_type type;
	uint16_t port;
	int seg;
	uint8_t address_size;
	uint8_t operand_size;
	bool rep;
	bool str;
	uint64_t npc;
};

enum nvmm_exit_msr_type {
	NVMM_EXIT_MSR_RDMSR,
	NVMM_EXIT_MSR_WRMSR
};

struct nvmm_exit_msr {
	enum nvmm_exit_msr_type type;
	uint64_t msr;
	uint64_t val;
	uint64_t npc;
};

struct nvmm_exit_insn {
	uint64_t npc;
};

struct nvmm_exit_invalid {
	uint64_t hwcode;
};

union nvmm_exit_md {
	struct nvmm_exit_memory mem;
	struct nvmm_exit_io io;
	struct nvmm_exit_msr msr;
	struct nvmm_exit_insn insn;
	struct nvmm_exit_invalid inv;
};

#define NVMM_EXIT_MONITOR	0x0000000000001000ULL
#define NVMM_EXIT_MWAIT		0x0000000000001001ULL
#define NVMM_EXIT_MWAIT_COND	0x0000000000001002ULL

struct nvmm_cap_md {
	uint64_t xcr0_mask;
	uint64_t mxcsr_mask;
	uint64_t conf_cpuid_maxops;
	uint64_t rsvd[5];
};

#endif

/* --------------------------------------------------------------------- */

/* Segments. */
#define NVMM_X64_SEG_ES			0
#define NVMM_X64_SEG_CS			1
#define NVMM_X64_SEG_SS			2
#define NVMM_X64_SEG_DS			3
#define NVMM_X64_SEG_FS			4
#define NVMM_X64_SEG_GS			5
#define NVMM_X64_SEG_GDT		6
#define NVMM_X64_SEG_IDT		7
#define NVMM_X64_SEG_LDT		8
#define NVMM_X64_SEG_TR			9
#define NVMM_X64_NSEG			10

/* General Purpose Registers. */
#define NVMM_X64_GPR_RAX		0
#define NVMM_X64_GPR_RCX		1
#define NVMM_X64_GPR_RDX		2
#define NVMM_X64_GPR_RBX		3
#define NVMM_X64_GPR_RSP		4
#define NVMM_X64_GPR_RBP		5
#define NVMM_X64_GPR_RSI		6
#define NVMM_X64_GPR_RDI		7
#define NVMM_X64_GPR_R8			8
#define NVMM_X64_GPR_R9			9
#define NVMM_X64_GPR_R10		10
#define NVMM_X64_GPR_R11		11
#define NVMM_X64_GPR_R12		12
#define NVMM_X64_GPR_R13		13
#define NVMM_X64_GPR_R14		14
#define NVMM_X64_GPR_R15		15
#define NVMM_X64_GPR_RIP		16
#define NVMM_X64_GPR_RFLAGS		17
#define NVMM_X64_NGPR			18

/* Control Registers. */
#define NVMM_X64_CR_CR0			0
#define NVMM_X64_CR_CR2			1
#define NVMM_X64_CR_CR3			2
#define NVMM_X64_CR_CR4			3
#define NVMM_X64_CR_CR8			4
#define NVMM_X64_CR_XCR0		5
#define NVMM_X64_NCR			6

/* Debug Registers. */
#define NVMM_X64_DR_DR0			0
#define NVMM_X64_DR_DR1			1
#define NVMM_X64_DR_DR2			2
#define NVMM_X64_DR_DR3			3
#define NVMM_X64_DR_DR6			4
#define NVMM_X64_DR_DR7			5
#define NVMM_X64_NDR			6

/* MSRs. */
#define NVMM_X64_MSR_EFER		0
#define NVMM_X64_MSR_STAR		1
#define NVMM_X64_MSR_LSTAR		2
#define NVMM_X64_MSR_CSTAR		3
#define NVMM_X64_MSR_SFMASK		4
#define NVMM_X64_MSR_KERNELGSBASE	5
#define NVMM_X64_MSR_SYSENTER_CS	6
#define NVMM_X64_MSR_SYSENTER_ESP	7
#define NVMM_X64_MSR_SYSENTER_EIP	8
#define NVMM_X64_MSR_PAT		9
#define NVMM_X64_MSR_TSC		10
#define NVMM_X64_NMSR			11

#ifndef ASM_NVMM

#include <sys/types.h>
#include <x86/cpu_extended_state.h>

struct nvmm_x64_state_seg {
	uint16_t selector;
	struct {		/* hidden */
		uint16_t type:4;
		uint16_t s:1;
		uint16_t dpl:2;
		uint16_t p:1;
		uint16_t avl:1;
		uint16_t l:1;
		uint16_t def:1;
		uint16_t g:1;
		uint16_t rsvd:4;
	} attrib;
	uint32_t limit;		/* hidden */
	uint64_t base;		/* hidden */
};

struct nvmm_x64_state_intr {
	uint64_t int_shadow:1;
	uint64_t int_window_exiting:1;
	uint64_t nmi_window_exiting:1;
	uint64_t evt_pending:1;
	uint64_t rsvd:60;
};

/* VM exit state indexes. */
#define NVMM_X64_EXITSTATE_CR8			0
#define NVMM_X64_EXITSTATE_RFLAGS		1
#define NVMM_X64_EXITSTATE_INT_SHADOW		2
#define NVMM_X64_EXITSTATE_INT_WINDOW_EXIT	3
#define NVMM_X64_EXITSTATE_NMI_WINDOW_EXIT	4
#define NVMM_X64_EXITSTATE_EVT_PENDING		5

/* Flags. */
#define NVMM_X64_STATE_SEGS	0x01
#define NVMM_X64_STATE_GPRS	0x02
#define NVMM_X64_STATE_CRS	0x04
#define NVMM_X64_STATE_DRS	0x08
#define NVMM_X64_STATE_MSRS	0x10
#define NVMM_X64_STATE_INTR	0x20
#define NVMM_X64_STATE_FPU	0x40
#define NVMM_X64_STATE_ALL	\
	(NVMM_X64_STATE_SEGS | NVMM_X64_STATE_GPRS | NVMM_X64_STATE_CRS | \
	 NVMM_X64_STATE_DRS | NVMM_X64_STATE_MSRS | NVMM_X64_STATE_INTR | \
	 NVMM_X64_STATE_FPU)

struct nvmm_x64_state {
	struct nvmm_x64_state_seg segs[NVMM_X64_NSEG];
	uint64_t gprs[NVMM_X64_NGPR];
	uint64_t crs[NVMM_X64_NCR];
	uint64_t drs[NVMM_X64_NDR];
	uint64_t msrs[NVMM_X64_NMSR];
	struct nvmm_x64_state_intr intr;
	struct fxsave fpu;
};

#define nvmm_vcpu_state nvmm_x64_state

#define NVMM_MACH_CONF_X86_CPUID	NVMM_MACH_CONF_MD_BEGIN
#define NVMM_X86_NCONF			1

struct nvmm_mach_conf_x86_cpuid {
	uint32_t leaf;
	struct {
		uint32_t eax;
		uint32_t ebx;
		uint32_t ecx;
		uint32_t edx;
	} set;
	struct {
		uint32_t eax;
		uint32_t ebx;
		uint32_t ecx;
		uint32_t edx;
	} del;
};

#ifdef _KERNEL
struct nvmm_x86_cpuid_mask {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};
extern const struct nvmm_x64_state nvmm_x86_reset_state;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000001;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_00000007;
extern const struct nvmm_x86_cpuid_mask nvmm_cpuid_80000001;
bool nvmm_x86_pat_validate(uint64_t);
#endif

#endif /* ASM_NVMM */

#endif /* _NVMM_X86_H_ */
