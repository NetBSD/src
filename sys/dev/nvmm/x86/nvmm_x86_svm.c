/*	$NetBSD: nvmm_x86_svm.c,v 1.31 2019/02/23 12:27:00 maxv Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nvmm_x86_svm.c,v 1.31 2019/02/23 12:27:00 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include <x86/cputypes.h>
#include <x86/specialreg.h>
#include <x86/pmap.h>
#include <x86/dbregs.h>
#include <x86/cpu_counter.h>
#include <machine/cpuvar.h>

#include <dev/nvmm/nvmm.h>
#include <dev/nvmm/nvmm_internal.h>
#include <dev/nvmm/x86/nvmm_x86.h>

int svm_vmrun(paddr_t, uint64_t *);

#define	MSR_VM_HSAVE_PA	0xC0010117

/* -------------------------------------------------------------------------- */

#define VMCB_EXITCODE_CR0_READ		0x0000
#define VMCB_EXITCODE_CR1_READ		0x0001
#define VMCB_EXITCODE_CR2_READ		0x0002
#define VMCB_EXITCODE_CR3_READ		0x0003
#define VMCB_EXITCODE_CR4_READ		0x0004
#define VMCB_EXITCODE_CR5_READ		0x0005
#define VMCB_EXITCODE_CR6_READ		0x0006
#define VMCB_EXITCODE_CR7_READ		0x0007
#define VMCB_EXITCODE_CR8_READ		0x0008
#define VMCB_EXITCODE_CR9_READ		0x0009
#define VMCB_EXITCODE_CR10_READ		0x000A
#define VMCB_EXITCODE_CR11_READ		0x000B
#define VMCB_EXITCODE_CR12_READ		0x000C
#define VMCB_EXITCODE_CR13_READ		0x000D
#define VMCB_EXITCODE_CR14_READ		0x000E
#define VMCB_EXITCODE_CR15_READ		0x000F
#define VMCB_EXITCODE_CR0_WRITE		0x0010
#define VMCB_EXITCODE_CR1_WRITE		0x0011
#define VMCB_EXITCODE_CR2_WRITE		0x0012
#define VMCB_EXITCODE_CR3_WRITE		0x0013
#define VMCB_EXITCODE_CR4_WRITE		0x0014
#define VMCB_EXITCODE_CR5_WRITE		0x0015
#define VMCB_EXITCODE_CR6_WRITE		0x0016
#define VMCB_EXITCODE_CR7_WRITE		0x0017
#define VMCB_EXITCODE_CR8_WRITE		0x0018
#define VMCB_EXITCODE_CR9_WRITE		0x0019
#define VMCB_EXITCODE_CR10_WRITE	0x001A
#define VMCB_EXITCODE_CR11_WRITE	0x001B
#define VMCB_EXITCODE_CR12_WRITE	0x001C
#define VMCB_EXITCODE_CR13_WRITE	0x001D
#define VMCB_EXITCODE_CR14_WRITE	0x001E
#define VMCB_EXITCODE_CR15_WRITE	0x001F
#define VMCB_EXITCODE_DR0_READ		0x0020
#define VMCB_EXITCODE_DR1_READ		0x0021
#define VMCB_EXITCODE_DR2_READ		0x0022
#define VMCB_EXITCODE_DR3_READ		0x0023
#define VMCB_EXITCODE_DR4_READ		0x0024
#define VMCB_EXITCODE_DR5_READ		0x0025
#define VMCB_EXITCODE_DR6_READ		0x0026
#define VMCB_EXITCODE_DR7_READ		0x0027
#define VMCB_EXITCODE_DR8_READ		0x0028
#define VMCB_EXITCODE_DR9_READ		0x0029
#define VMCB_EXITCODE_DR10_READ		0x002A
#define VMCB_EXITCODE_DR11_READ		0x002B
#define VMCB_EXITCODE_DR12_READ		0x002C
#define VMCB_EXITCODE_DR13_READ		0x002D
#define VMCB_EXITCODE_DR14_READ		0x002E
#define VMCB_EXITCODE_DR15_READ		0x002F
#define VMCB_EXITCODE_DR0_WRITE		0x0030
#define VMCB_EXITCODE_DR1_WRITE		0x0031
#define VMCB_EXITCODE_DR2_WRITE		0x0032
#define VMCB_EXITCODE_DR3_WRITE		0x0033
#define VMCB_EXITCODE_DR4_WRITE		0x0034
#define VMCB_EXITCODE_DR5_WRITE		0x0035
#define VMCB_EXITCODE_DR6_WRITE		0x0036
#define VMCB_EXITCODE_DR7_WRITE		0x0037
#define VMCB_EXITCODE_DR8_WRITE		0x0038
#define VMCB_EXITCODE_DR9_WRITE		0x0039
#define VMCB_EXITCODE_DR10_WRITE	0x003A
#define VMCB_EXITCODE_DR11_WRITE	0x003B
#define VMCB_EXITCODE_DR12_WRITE	0x003C
#define VMCB_EXITCODE_DR13_WRITE	0x003D
#define VMCB_EXITCODE_DR14_WRITE	0x003E
#define VMCB_EXITCODE_DR15_WRITE	0x003F
#define VMCB_EXITCODE_EXCP0		0x0040
#define VMCB_EXITCODE_EXCP1		0x0041
#define VMCB_EXITCODE_EXCP2		0x0042
#define VMCB_EXITCODE_EXCP3		0x0043
#define VMCB_EXITCODE_EXCP4		0x0044
#define VMCB_EXITCODE_EXCP5		0x0045
#define VMCB_EXITCODE_EXCP6		0x0046
#define VMCB_EXITCODE_EXCP7		0x0047
#define VMCB_EXITCODE_EXCP8		0x0048
#define VMCB_EXITCODE_EXCP9		0x0049
#define VMCB_EXITCODE_EXCP10		0x004A
#define VMCB_EXITCODE_EXCP11		0x004B
#define VMCB_EXITCODE_EXCP12		0x004C
#define VMCB_EXITCODE_EXCP13		0x004D
#define VMCB_EXITCODE_EXCP14		0x004E
#define VMCB_EXITCODE_EXCP15		0x004F
#define VMCB_EXITCODE_EXCP16		0x0050
#define VMCB_EXITCODE_EXCP17		0x0051
#define VMCB_EXITCODE_EXCP18		0x0052
#define VMCB_EXITCODE_EXCP19		0x0053
#define VMCB_EXITCODE_EXCP20		0x0054
#define VMCB_EXITCODE_EXCP21		0x0055
#define VMCB_EXITCODE_EXCP22		0x0056
#define VMCB_EXITCODE_EXCP23		0x0057
#define VMCB_EXITCODE_EXCP24		0x0058
#define VMCB_EXITCODE_EXCP25		0x0059
#define VMCB_EXITCODE_EXCP26		0x005A
#define VMCB_EXITCODE_EXCP27		0x005B
#define VMCB_EXITCODE_EXCP28		0x005C
#define VMCB_EXITCODE_EXCP29		0x005D
#define VMCB_EXITCODE_EXCP30		0x005E
#define VMCB_EXITCODE_EXCP31		0x005F
#define VMCB_EXITCODE_INTR		0x0060
#define VMCB_EXITCODE_NMI		0x0061
#define VMCB_EXITCODE_SMI		0x0062
#define VMCB_EXITCODE_INIT		0x0063
#define VMCB_EXITCODE_VINTR		0x0064
#define VMCB_EXITCODE_CR0_SEL_WRITE	0x0065
#define VMCB_EXITCODE_IDTR_READ		0x0066
#define VMCB_EXITCODE_GDTR_READ		0x0067
#define VMCB_EXITCODE_LDTR_READ		0x0068
#define VMCB_EXITCODE_TR_READ		0x0069
#define VMCB_EXITCODE_IDTR_WRITE	0x006A
#define VMCB_EXITCODE_GDTR_WRITE	0x006B
#define VMCB_EXITCODE_LDTR_WRITE	0x006C
#define VMCB_EXITCODE_TR_WRITE		0x006D
#define VMCB_EXITCODE_RDTSC		0x006E
#define VMCB_EXITCODE_RDPMC		0x006F
#define VMCB_EXITCODE_PUSHF		0x0070
#define VMCB_EXITCODE_POPF		0x0071
#define VMCB_EXITCODE_CPUID		0x0072
#define VMCB_EXITCODE_RSM		0x0073
#define VMCB_EXITCODE_IRET		0x0074
#define VMCB_EXITCODE_SWINT		0x0075
#define VMCB_EXITCODE_INVD		0x0076
#define VMCB_EXITCODE_PAUSE		0x0077
#define VMCB_EXITCODE_HLT		0x0078
#define VMCB_EXITCODE_INVLPG		0x0079
#define VMCB_EXITCODE_INVLPGA		0x007A
#define VMCB_EXITCODE_IOIO		0x007B
#define VMCB_EXITCODE_MSR		0x007C
#define VMCB_EXITCODE_TASK_SWITCH	0x007D
#define VMCB_EXITCODE_FERR_FREEZE	0x007E
#define VMCB_EXITCODE_SHUTDOWN		0x007F
#define VMCB_EXITCODE_VMRUN		0x0080
#define VMCB_EXITCODE_VMMCALL		0x0081
#define VMCB_EXITCODE_VMLOAD		0x0082
#define VMCB_EXITCODE_VMSAVE		0x0083
#define VMCB_EXITCODE_STGI		0x0084
#define VMCB_EXITCODE_CLGI		0x0085
#define VMCB_EXITCODE_SKINIT		0x0086
#define VMCB_EXITCODE_RDTSCP		0x0087
#define VMCB_EXITCODE_ICEBP		0x0088
#define VMCB_EXITCODE_WBINVD		0x0089
#define VMCB_EXITCODE_MONITOR		0x008A
#define VMCB_EXITCODE_MWAIT		0x008B
#define VMCB_EXITCODE_MWAIT_CONDITIONAL	0x008C
#define VMCB_EXITCODE_XSETBV		0x008D
#define VMCB_EXITCODE_EFER_WRITE_TRAP	0x008F
#define VMCB_EXITCODE_CR0_WRITE_TRAP	0x0090
#define VMCB_EXITCODE_CR1_WRITE_TRAP	0x0091
#define VMCB_EXITCODE_CR2_WRITE_TRAP	0x0092
#define VMCB_EXITCODE_CR3_WRITE_TRAP	0x0093
#define VMCB_EXITCODE_CR4_WRITE_TRAP	0x0094
#define VMCB_EXITCODE_CR5_WRITE_TRAP	0x0095
#define VMCB_EXITCODE_CR6_WRITE_TRAP	0x0096
#define VMCB_EXITCODE_CR7_WRITE_TRAP	0x0097
#define VMCB_EXITCODE_CR8_WRITE_TRAP	0x0098
#define VMCB_EXITCODE_CR9_WRITE_TRAP	0x0099
#define VMCB_EXITCODE_CR10_WRITE_TRAP	0x009A
#define VMCB_EXITCODE_CR11_WRITE_TRAP	0x009B
#define VMCB_EXITCODE_CR12_WRITE_TRAP	0x009C
#define VMCB_EXITCODE_CR13_WRITE_TRAP	0x009D
#define VMCB_EXITCODE_CR14_WRITE_TRAP	0x009E
#define VMCB_EXITCODE_CR15_WRITE_TRAP	0x009F
#define VMCB_EXITCODE_NPF		0x0400
#define VMCB_EXITCODE_AVIC_INCOMP_IPI	0x0401
#define VMCB_EXITCODE_AVIC_NOACCEL	0x0402
#define VMCB_EXITCODE_VMGEXIT		0x0403
#define VMCB_EXITCODE_INVALID		-1

/* -------------------------------------------------------------------------- */

struct vmcb_ctrl {
	uint32_t intercept_cr;
#define VMCB_CTRL_INTERCEPT_RCR(x)	__BIT( 0 + x)
#define VMCB_CTRL_INTERCEPT_WCR(x)	__BIT(16 + x)

	uint32_t intercept_dr;
#define VMCB_CTRL_INTERCEPT_RDR(x)	__BIT( 0 + x)
#define VMCB_CTRL_INTERCEPT_WDR(x)	__BIT(16 + x)

	uint32_t intercept_vec;
#define VMCB_CTRL_INTERCEPT_VEC(x)	__BIT(x)

	uint32_t intercept_misc1;
#define VMCB_CTRL_INTERCEPT_INTR	__BIT(0)
#define VMCB_CTRL_INTERCEPT_NMI		__BIT(1)
#define VMCB_CTRL_INTERCEPT_SMI		__BIT(2)
#define VMCB_CTRL_INTERCEPT_INIT	__BIT(3)
#define VMCB_CTRL_INTERCEPT_VINTR	__BIT(4)
#define VMCB_CTRL_INTERCEPT_CR0_SPEC	__BIT(5)
#define VMCB_CTRL_INTERCEPT_RIDTR	__BIT(6)
#define VMCB_CTRL_INTERCEPT_RGDTR	__BIT(7)
#define VMCB_CTRL_INTERCEPT_RLDTR	__BIT(8)
#define VMCB_CTRL_INTERCEPT_RTR		__BIT(9)
#define VMCB_CTRL_INTERCEPT_WIDTR	__BIT(10)
#define VMCB_CTRL_INTERCEPT_WGDTR	__BIT(11)
#define VMCB_CTRL_INTERCEPT_WLDTR	__BIT(12)
#define VMCB_CTRL_INTERCEPT_WTR		__BIT(13)
#define VMCB_CTRL_INTERCEPT_RDTSC	__BIT(14)
#define VMCB_CTRL_INTERCEPT_RDPMC	__BIT(15)
#define VMCB_CTRL_INTERCEPT_PUSHF	__BIT(16)
#define VMCB_CTRL_INTERCEPT_POPF	__BIT(17)
#define VMCB_CTRL_INTERCEPT_CPUID	__BIT(18)
#define VMCB_CTRL_INTERCEPT_RSM		__BIT(19)
#define VMCB_CTRL_INTERCEPT_IRET	__BIT(20)
#define VMCB_CTRL_INTERCEPT_INTN	__BIT(21)
#define VMCB_CTRL_INTERCEPT_INVD	__BIT(22)
#define VMCB_CTRL_INTERCEPT_PAUSE	__BIT(23)
#define VMCB_CTRL_INTERCEPT_HLT		__BIT(24)
#define VMCB_CTRL_INTERCEPT_INVLPG	__BIT(25)
#define VMCB_CTRL_INTERCEPT_INVLPGA	__BIT(26)
#define VMCB_CTRL_INTERCEPT_IOIO_PROT	__BIT(27)
#define VMCB_CTRL_INTERCEPT_MSR_PROT	__BIT(28)
#define VMCB_CTRL_INTERCEPT_TASKSW	__BIT(29)
#define VMCB_CTRL_INTERCEPT_FERR_FREEZE	__BIT(30)
#define VMCB_CTRL_INTERCEPT_SHUTDOWN	__BIT(31)

	uint32_t intercept_misc2;
#define VMCB_CTRL_INTERCEPT_VMRUN	__BIT(0)
#define VMCB_CTRL_INTERCEPT_VMMCALL	__BIT(1)
#define VMCB_CTRL_INTERCEPT_VMLOAD	__BIT(2)
#define VMCB_CTRL_INTERCEPT_VMSAVE	__BIT(3)
#define VMCB_CTRL_INTERCEPT_STGI	__BIT(4)
#define VMCB_CTRL_INTERCEPT_CLGI	__BIT(5)
#define VMCB_CTRL_INTERCEPT_SKINIT	__BIT(6)
#define VMCB_CTRL_INTERCEPT_RDTSCP	__BIT(7)
#define VMCB_CTRL_INTERCEPT_ICEBP	__BIT(8)
#define VMCB_CTRL_INTERCEPT_WBINVD	__BIT(9)
#define VMCB_CTRL_INTERCEPT_MONITOR	__BIT(10)
#define VMCB_CTRL_INTERCEPT_MWAIT	__BIT(12)
#define VMCB_CTRL_INTERCEPT_XSETBV	__BIT(13)
#define VMCB_CTRL_INTERCEPT_EFER_SPEC	__BIT(15)
#define VMCB_CTRL_INTERCEPT_WCR_SPEC(x)	__BIT(16 + x)

	uint8_t  rsvd1[40];
	uint16_t pause_filt_thresh;
	uint16_t pause_filt_cnt;
	uint64_t iopm_base_pa;
	uint64_t msrpm_base_pa;
	uint64_t tsc_offset;
	uint32_t guest_asid;

	uint32_t tlb_ctrl;
#define VMCB_CTRL_TLB_CTRL_FLUSH_ALL			0x01
#define VMCB_CTRL_TLB_CTRL_FLUSH_GUEST			0x03
#define VMCB_CTRL_TLB_CTRL_FLUSH_GUEST_NONGLOBAL	0x07

	uint64_t v;
#define VMCB_CTRL_V_TPR			__BITS(7,0)
#define VMCB_CTRL_V_IRQ			__BIT(8)
#define VMCB_CTRL_V_VGIF		__BIT(9)
#define VMCB_CTRL_V_INTR_PRIO		__BITS(19,16)
#define VMCB_CTRL_V_IGN_TPR		__BIT(20)
#define VMCB_CTRL_V_INTR_MASKING	__BIT(24)
#define VMCB_CTRL_V_GUEST_VGIF		__BIT(25)
#define VMCB_CTRL_V_AVIC_EN		__BIT(31)
#define VMCB_CTRL_V_INTR_VECTOR		__BITS(39,32)

	uint64_t intr;
#define VMCB_CTRL_INTR_SHADOW		__BIT(0)

	uint64_t exitcode;
	uint64_t exitinfo1;
	uint64_t exitinfo2;

	uint64_t exitintinfo;
#define VMCB_CTRL_EXITINTINFO_VECTOR	__BITS(7,0)
#define VMCB_CTRL_EXITINTINFO_TYPE	__BITS(10,8)
#define VMCB_CTRL_EXITINTINFO_EV	__BIT(11)
#define VMCB_CTRL_EXITINTINFO_V		__BIT(31)
#define VMCB_CTRL_EXITINTINFO_ERRORCODE	__BITS(63,32)

	uint64_t enable1;
#define VMCB_CTRL_ENABLE_NP		__BIT(0)
#define VMCB_CTRL_ENABLE_SEV		__BIT(1)
#define VMCB_CTRL_ENABLE_ES_SEV		__BIT(2)

	uint64_t avic;
#define VMCB_CTRL_AVIC_APIC_BAR		__BITS(51,0)

	uint64_t ghcb;

	uint64_t eventinj;
#define VMCB_CTRL_EVENTINJ_VECTOR	__BITS(7,0)
#define VMCB_CTRL_EVENTINJ_TYPE		__BITS(10,8)
#define VMCB_CTRL_EVENTINJ_EV		__BIT(11)
#define VMCB_CTRL_EVENTINJ_V		__BIT(31)
#define VMCB_CTRL_EVENTINJ_ERRORCODE	__BITS(63,32)

	uint64_t n_cr3;

	uint64_t enable2;
#define VMCB_CTRL_ENABLE_LBR		__BIT(0)
#define VMCB_CTRL_ENABLE_VVMSAVE	__BIT(1)

	uint32_t vmcb_clean;
#define VMCB_CTRL_VMCB_CLEAN_I		__BIT(0)
#define VMCB_CTRL_VMCB_CLEAN_IOPM	__BIT(1)
#define VMCB_CTRL_VMCB_CLEAN_ASID	__BIT(2)
#define VMCB_CTRL_VMCB_CLEAN_TPR	__BIT(3)
#define VMCB_CTRL_VMCB_CLEAN_NP		__BIT(4)
#define VMCB_CTRL_VMCB_CLEAN_CR		__BIT(5)
#define VMCB_CTRL_VMCB_CLEAN_DR		__BIT(6)
#define VMCB_CTRL_VMCB_CLEAN_DT		__BIT(7)
#define VMCB_CTRL_VMCB_CLEAN_SEG	__BIT(8)
#define VMCB_CTRL_VMCB_CLEAN_CR2	__BIT(9)
#define VMCB_CTRL_VMCB_CLEAN_LBR	__BIT(10)
#define VMCB_CTRL_VMCB_CLEAN_AVIC	__BIT(11)

	uint32_t rsvd2;
	uint64_t nrip;
	uint8_t	inst_len;
	uint8_t	inst_bytes[15];
	uint64_t avic_abpp;
	uint64_t rsvd3;
	uint64_t avic_ltp;

	uint64_t avic_phys;
#define VMCB_CTRL_AVIC_PHYS_TABLE_PTR	__BITS(51,12)
#define VMCB_CTRL_AVIC_PHYS_MAX_INDEX	__BITS(7,0)

	uint64_t rsvd4;
	uint64_t vmcb_ptr;

	uint8_t	pad[752];
} __packed;

CTASSERT(sizeof(struct vmcb_ctrl) == 1024);

struct vmcb_segment {
	uint16_t selector;
	uint16_t attrib;	/* hidden */
	uint32_t limit;		/* hidden */
	uint64_t base;		/* hidden */
} __packed;

CTASSERT(sizeof(struct vmcb_segment) == 16);

struct vmcb_state {
	struct   vmcb_segment es;
	struct   vmcb_segment cs;
	struct   vmcb_segment ss;
	struct   vmcb_segment ds;
	struct   vmcb_segment fs;
	struct   vmcb_segment gs;
	struct   vmcb_segment gdt;
	struct   vmcb_segment ldt;
	struct   vmcb_segment idt;
	struct   vmcb_segment tr;
	uint8_t	 rsvd1[43];
	uint8_t	 cpl;
	uint8_t  rsvd2[4];
	uint64_t efer;
	uint8_t	 rsvd3[112];
	uint64_t cr4;
	uint64_t cr3;
	uint64_t cr0;
	uint64_t dr7;
	uint64_t dr6;
	uint64_t rflags;
	uint64_t rip;
	uint8_t	 rsvd4[88];
	uint64_t rsp;
	uint8_t	 rsvd5[24];
	uint64_t rax;
	uint64_t star;
	uint64_t lstar;
	uint64_t cstar;
	uint64_t sfmask;
	uint64_t kernelgsbase;
	uint64_t sysenter_cs;
	uint64_t sysenter_esp;
	uint64_t sysenter_eip;
	uint64_t cr2;
	uint8_t	 rsvd6[32];
	uint64_t g_pat;
	uint64_t dbgctl;
	uint64_t br_from;
	uint64_t br_to;
	uint64_t int_from;
	uint64_t int_to;
	uint8_t	 pad[2408];
} __packed;

CTASSERT(sizeof(struct vmcb_state) == 0xC00);

struct vmcb {
	struct vmcb_ctrl ctrl;
	struct vmcb_state state;
} __packed;

CTASSERT(sizeof(struct vmcb) == PAGE_SIZE);
CTASSERT(offsetof(struct vmcb, state) == 0x400);

/* -------------------------------------------------------------------------- */

struct svm_hsave {
	paddr_t pa;
};

static struct svm_hsave hsave[MAXCPUS];

static uint8_t *svm_asidmap __read_mostly;
static uint32_t svm_maxasid __read_mostly;
static kmutex_t svm_asidlock __cacheline_aligned;

static bool svm_decode_assist __read_mostly;
static uint32_t svm_ctrl_tlb_flush __read_mostly;

#define SVM_XCR0_MASK_DEFAULT	(XCR0_X87|XCR0_SSE)
static uint64_t svm_xcr0_mask __read_mostly;

#define SVM_NCPUIDS	32

#define VMCB_NPAGES	1

#define MSRBM_NPAGES	2
#define MSRBM_SIZE	(MSRBM_NPAGES * PAGE_SIZE)

#define IOBM_NPAGES	3
#define IOBM_SIZE	(IOBM_NPAGES * PAGE_SIZE)

/* Does not include EFER_LMSLE. */
#define EFER_VALID \
	(EFER_SCE|EFER_LME|EFER_LMA|EFER_NXE|EFER_SVME|EFER_FFXSR|EFER_TCE)

#define EFER_TLB_FLUSH \
	(EFER_NXE|EFER_LMA|EFER_LME)
#define CR0_TLB_FLUSH \
	(CR0_PG|CR0_WP|CR0_CD|CR0_NW)
#define CR4_TLB_FLUSH \
	(CR4_PGE|CR4_PAE|CR4_PSE)

/* -------------------------------------------------------------------------- */

struct svm_machdata {
	bool cpuidpresent[SVM_NCPUIDS];
	struct nvmm_x86_conf_cpuid cpuid[SVM_NCPUIDS];
	volatile uint64_t mach_htlb_gen;
};

static const size_t svm_conf_sizes[NVMM_X86_NCONF] = {
	[NVMM_X86_CONF_CPUID] = sizeof(struct nvmm_x86_conf_cpuid)
};

struct svm_cpudata {
	/* General */
	bool shared_asid;
	bool gtlb_want_flush;
	uint64_t vcpu_htlb_gen;

	/* VMCB */
	struct vmcb *vmcb;
	paddr_t vmcb_pa;

	/* I/O bitmap */
	uint8_t *iobm;
	paddr_t iobm_pa;

	/* MSR bitmap */
	uint8_t *msrbm;
	paddr_t msrbm_pa;

	/* Host state */
	uint64_t hxcr0;
	uint64_t star;
	uint64_t lstar;
	uint64_t cstar;
	uint64_t sfmask;
	uint64_t fsbase;
	uint64_t kernelgsbase;
	bool ts_set;
	struct xsave_header hfpu __aligned(64);

	/* Event state */
	bool int_window_exit;
	bool nmi_window_exit;

	/* Guest state */
	uint64_t gxcr0;
	uint64_t gprs[NVMM_X64_NGPR];
	uint64_t drs[NVMM_X64_NDR];
	uint64_t tsc_offset;
	struct xsave_header gfpu __aligned(64);
};

static void
svm_vmcb_cache_default(struct vmcb *vmcb)
{
	vmcb->ctrl.vmcb_clean =
	    VMCB_CTRL_VMCB_CLEAN_I |
	    VMCB_CTRL_VMCB_CLEAN_IOPM |
	    VMCB_CTRL_VMCB_CLEAN_ASID |
	    VMCB_CTRL_VMCB_CLEAN_TPR |
	    VMCB_CTRL_VMCB_CLEAN_NP |
	    VMCB_CTRL_VMCB_CLEAN_CR |
	    VMCB_CTRL_VMCB_CLEAN_DR |
	    VMCB_CTRL_VMCB_CLEAN_DT |
	    VMCB_CTRL_VMCB_CLEAN_SEG |
	    VMCB_CTRL_VMCB_CLEAN_CR2 |
	    VMCB_CTRL_VMCB_CLEAN_LBR |
	    VMCB_CTRL_VMCB_CLEAN_AVIC;
}

static void
svm_vmcb_cache_update(struct vmcb *vmcb, uint64_t flags)
{
	if (flags & NVMM_X64_STATE_SEGS) {
		vmcb->ctrl.vmcb_clean &=
		    ~(VMCB_CTRL_VMCB_CLEAN_SEG | VMCB_CTRL_VMCB_CLEAN_DT);
	}
	if (flags & NVMM_X64_STATE_CRS) {
		vmcb->ctrl.vmcb_clean &=
		    ~(VMCB_CTRL_VMCB_CLEAN_CR | VMCB_CTRL_VMCB_CLEAN_CR2 |
		      VMCB_CTRL_VMCB_CLEAN_TPR);
	}
	if (flags & NVMM_X64_STATE_DRS) {
		vmcb->ctrl.vmcb_clean &= ~VMCB_CTRL_VMCB_CLEAN_DR;
	}
	if (flags & NVMM_X64_STATE_MSRS) {
		/* CR for EFER, NP for PAT. */
		vmcb->ctrl.vmcb_clean &=
		    ~(VMCB_CTRL_VMCB_CLEAN_CR | VMCB_CTRL_VMCB_CLEAN_NP);
	}
}

static inline void
svm_vmcb_cache_flush(struct vmcb *vmcb, uint64_t flags)
{
	vmcb->ctrl.vmcb_clean &= ~flags;
}

static inline void
svm_vmcb_cache_flush_all(struct vmcb *vmcb)
{
	vmcb->ctrl.vmcb_clean = 0;
}

#define SVM_EVENT_TYPE_HW_INT	0
#define SVM_EVENT_TYPE_NMI	2
#define SVM_EVENT_TYPE_EXC	3
#define SVM_EVENT_TYPE_SW_INT	4

static void
svm_event_waitexit_enable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;

	if (nmi) {
		vmcb->ctrl.intercept_misc1 |= VMCB_CTRL_INTERCEPT_IRET;
		cpudata->nmi_window_exit = true;
	} else {
		vmcb->ctrl.intercept_misc1 |= VMCB_CTRL_INTERCEPT_VINTR;
		vmcb->ctrl.v |= (VMCB_CTRL_V_IRQ | VMCB_CTRL_V_IGN_TPR);
		svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_TPR);
		cpudata->int_window_exit = true;
	}

	svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_I);
}

static void
svm_event_waitexit_disable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;

	if (nmi) {
		vmcb->ctrl.intercept_misc1 &= ~VMCB_CTRL_INTERCEPT_IRET;
		cpudata->nmi_window_exit = false;
	} else {
		vmcb->ctrl.intercept_misc1 &= ~VMCB_CTRL_INTERCEPT_VINTR;
		vmcb->ctrl.v &= ~(VMCB_CTRL_V_IRQ | VMCB_CTRL_V_IGN_TPR);
		svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_TPR);
		cpudata->int_window_exit = false;
	}

	svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_I);
}

static inline int
svm_event_has_error(uint64_t vector)
{
	switch (vector) {
	case 8:		/* #DF */
	case 10:	/* #TS */
	case 11:	/* #NP */
	case 12:	/* #SS */
	case 13:	/* #GP */
	case 14:	/* #PF */
	case 17:	/* #AC */
	case 30:	/* #SX */
		return 1;
	default:
		return 0;
	}
}

static int
svm_vcpu_inject(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_event *event)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	int type = 0, err = 0;

	if (event->vector >= 256) {
		return EINVAL;
	}

	switch (event->type) {
	case NVMM_EVENT_INTERRUPT_HW:
		type = SVM_EVENT_TYPE_HW_INT;
		if (event->vector == 2) {
			type = SVM_EVENT_TYPE_NMI;
		}
		if (type == SVM_EVENT_TYPE_NMI) {
			if (cpudata->nmi_window_exit) {
				return EAGAIN;
			}
			svm_event_waitexit_enable(vcpu, true);
		} else {
			if (((vmcb->state.rflags & PSL_I) == 0) ||
			    ((vmcb->ctrl.intr & VMCB_CTRL_INTR_SHADOW) != 0)) {
				svm_event_waitexit_enable(vcpu, false);
				return EAGAIN;
			}
		}
		err = 0;
		break;
	case NVMM_EVENT_INTERRUPT_SW:
		return EINVAL;
	case NVMM_EVENT_EXCEPTION:
		type = SVM_EVENT_TYPE_EXC;
		if (event->vector == 2 || event->vector >= 32)
			return EINVAL;
		if (event->vector == 3 || event->vector == 0)
			return EINVAL;
		err = svm_event_has_error(event->vector);
		break;
	default:
		return EINVAL;
	}

	vmcb->ctrl.eventinj =
	    __SHIFTIN(event->vector, VMCB_CTRL_EVENTINJ_VECTOR) |
	    __SHIFTIN(type, VMCB_CTRL_EVENTINJ_TYPE) |
	    __SHIFTIN(err, VMCB_CTRL_EVENTINJ_EV) |
	    __SHIFTIN(1, VMCB_CTRL_EVENTINJ_V) |
	    __SHIFTIN(event->u.error, VMCB_CTRL_EVENTINJ_ERRORCODE);

	return 0;
}

static void
svm_inject_ud(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct nvmm_event event;
	int ret __diagused;

	event.type = NVMM_EVENT_EXCEPTION;
	event.vector = 6;
	event.u.error = 0;

	ret = svm_vcpu_inject(mach, vcpu, &event);
	KASSERT(ret == 0);
}

static void
svm_inject_gp(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct nvmm_event event;
	int ret __diagused;

	event.type = NVMM_EVENT_EXCEPTION;
	event.vector = 13;
	event.u.error = 0;

	ret = svm_vcpu_inject(mach, vcpu, &event);
	KASSERT(ret == 0);
}

static inline void
svm_inkernel_advance(struct vmcb *vmcb)
{
	/*
	 * Maybe we should also apply single-stepping and debug exceptions.
	 * Matters for guest-ring3, because it can execute 'cpuid' under a
	 * debugger.
	 */
	vmcb->state.rip = vmcb->ctrl.nrip;
	vmcb->ctrl.intr &= ~VMCB_CTRL_INTR_SHADOW;
}

static void
svm_inkernel_handle_cpuid(struct nvmm_cpu *vcpu, uint64_t eax, uint64_t ecx)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	uint64_t cr4;

	switch (eax) {
	case 0x00000001:
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~CPUID_LOCAL_APIC_ID;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= __SHIFTIN(vcpu->cpuid,
		    CPUID_LOCAL_APIC_ID);

		/* CPUID2_OSXSAVE depends on CR4. */
		cr4 = cpudata->vmcb->state.cr4;
		if (!(cr4 & CR4_OSXSAVE)) {
			cpudata->gprs[NVMM_X64_GPR_RCX] &= ~CPUID2_OSXSAVE;
		}
		break;
	case 0x0000000D:
		if (svm_xcr0_mask == 0) {
			break;
		}
		switch (ecx) {
		case 0:
			cpudata->vmcb->state.rax = svm_xcr0_mask & 0xFFFFFFFF;
			if (cpudata->gxcr0 & XCR0_SSE) {
				cpudata->gprs[NVMM_X64_GPR_RBX] = sizeof(struct fxsave);
			} else {
				cpudata->gprs[NVMM_X64_GPR_RBX] = sizeof(struct save87);
			}
			cpudata->gprs[NVMM_X64_GPR_RBX] += 64; /* XSAVE header */
			cpudata->gprs[NVMM_X64_GPR_RCX] = sizeof(struct fxsave);
			cpudata->gprs[NVMM_X64_GPR_RDX] = svm_xcr0_mask >> 32;
			break;
		case 1:
			cpudata->vmcb->state.rax &= ~CPUID_PES1_XSAVES;
			break;
		}
		break;
	case 0x40000000:
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RBX], "___ ", 4);
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RCX], "NVMM", 4);
		memcpy(&cpudata->gprs[NVMM_X64_GPR_RDX], " ___", 4);
		break;
	case 0x80000001:
		cpudata->gprs[NVMM_X64_GPR_RCX] &= ~CPUID_SVM;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= ~CPUID_RDTSCP;
		break;
	default:
		break;
	}
}

static void
svm_exit_cpuid(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_machdata *machdata = mach->machdata;
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct nvmm_x86_conf_cpuid *cpuid;
	uint64_t eax, ecx;
	u_int descs[4];
	size_t i;

	eax = cpudata->vmcb->state.rax;
	ecx = cpudata->gprs[NVMM_X64_GPR_RCX];
	x86_cpuid2(eax, ecx, descs);

	cpudata->vmcb->state.rax = descs[0];
	cpudata->gprs[NVMM_X64_GPR_RBX] = descs[1];
	cpudata->gprs[NVMM_X64_GPR_RCX] = descs[2];
	cpudata->gprs[NVMM_X64_GPR_RDX] = descs[3];

	for (i = 0; i < SVM_NCPUIDS; i++) {
		cpuid = &machdata->cpuid[i];
		if (!machdata->cpuidpresent[i]) {
			continue;
		}
		if (cpuid->leaf != eax) {
			continue;
		}

		/* del */
		cpudata->vmcb->state.rax &= ~cpuid->del.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~cpuid->del.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= ~cpuid->del.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= ~cpuid->del.edx;

		/* set */
		cpudata->vmcb->state.rax |= cpuid->set.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= cpuid->set.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] |= cpuid->set.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] |= cpuid->set.edx;

		break;
	}

	/* Overwrite non-tunable leaves. */
	svm_inkernel_handle_cpuid(vcpu, eax, ecx);

	svm_inkernel_advance(cpudata->vmcb);
	exit->reason = NVMM_EXIT_NONE;
}

static void
svm_exit_hlt(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;

	if (cpudata->int_window_exit && (vmcb->state.rflags & PSL_I)) {
		svm_event_waitexit_disable(vcpu, false);
	}

	svm_inkernel_advance(cpudata->vmcb);
	exit->reason = NVMM_EXIT_HALTED;
}

#define SVM_EXIT_IO_PORT	__BITS(31,16)
#define SVM_EXIT_IO_SEG		__BITS(12,10)
#define SVM_EXIT_IO_A64		__BIT(9)
#define SVM_EXIT_IO_A32		__BIT(8)
#define SVM_EXIT_IO_A16		__BIT(7)
#define SVM_EXIT_IO_SZ32	__BIT(6)
#define SVM_EXIT_IO_SZ16	__BIT(5)
#define SVM_EXIT_IO_SZ8		__BIT(4)
#define SVM_EXIT_IO_REP		__BIT(3)
#define SVM_EXIT_IO_STR		__BIT(2)
#define SVM_EXIT_IO_IN		__BIT(0)

static const int seg_to_nvmm[] = {
	[0] = NVMM_X64_SEG_ES,
	[1] = NVMM_X64_SEG_CS,
	[2] = NVMM_X64_SEG_SS,
	[3] = NVMM_X64_SEG_DS,
	[4] = NVMM_X64_SEG_FS,
	[5] = NVMM_X64_SEG_GS
};

static void
svm_exit_io(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	uint64_t info = cpudata->vmcb->ctrl.exitinfo1;
	uint64_t nextpc = cpudata->vmcb->ctrl.exitinfo2;

	exit->reason = NVMM_EXIT_IO;

	if (info & SVM_EXIT_IO_IN) {
		exit->u.io.type = NVMM_EXIT_IO_IN;
	} else {
		exit->u.io.type = NVMM_EXIT_IO_OUT;
	}

	exit->u.io.port = __SHIFTOUT(info, SVM_EXIT_IO_PORT);

	if (svm_decode_assist) {
		KASSERT(__SHIFTOUT(info, SVM_EXIT_IO_SEG) < 6);
		exit->u.io.seg = seg_to_nvmm[__SHIFTOUT(info, SVM_EXIT_IO_SEG)];
	} else {
		exit->u.io.seg = -1;
	}

	if (info & SVM_EXIT_IO_A64) {
		exit->u.io.address_size = 8;
	} else if (info & SVM_EXIT_IO_A32) {
		exit->u.io.address_size = 4;
	} else if (info & SVM_EXIT_IO_A16) {
		exit->u.io.address_size = 2;
	}

	if (info & SVM_EXIT_IO_SZ32) {
		exit->u.io.operand_size = 4;
	} else if (info & SVM_EXIT_IO_SZ16) {
		exit->u.io.operand_size = 2;
	} else if (info & SVM_EXIT_IO_SZ8) {
		exit->u.io.operand_size = 1;
	}

	exit->u.io.rep = (info & SVM_EXIT_IO_REP) != 0;
	exit->u.io.str = (info & SVM_EXIT_IO_STR) != 0;
	exit->u.io.npc = nextpc;
}

static const uint64_t msr_ignore_list[] = {
	0xc0010055, /* MSR_CMPHALT */
	MSR_DE_CFG,
	MSR_IC_CFG,
	MSR_UCODE_AMD_PATCHLEVEL
};

static bool
svm_inkernel_handle_msr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	uint64_t val;
	size_t i;

	switch (exit->u.msr.type) {
	case NVMM_EXIT_MSR_RDMSR:
		if (exit->u.msr.msr == MSR_NB_CFG) {
			val = NB_CFG_INITAPICCPUIDLO;
			vmcb->state.rax = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		for (i = 0; i < __arraycount(msr_ignore_list); i++) {
			if (msr_ignore_list[i] != exit->u.msr.msr)
				continue;
			val = 0;
			vmcb->state.rax = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		break;
	case NVMM_EXIT_MSR_WRMSR:
		if (exit->u.msr.msr == MSR_EFER) {
			if (__predict_false(exit->u.msr.val & ~EFER_VALID)) {
				goto error;
			}
			if ((vmcb->state.efer ^ exit->u.msr.val) &
			     EFER_TLB_FLUSH) {
				cpudata->gtlb_want_flush = true;
			}
			vmcb->state.efer = exit->u.msr.val | EFER_SVME;
			svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_CR);
			goto handled;
		}
		if (exit->u.msr.msr == MSR_TSC) {
			cpudata->tsc_offset = exit->u.msr.val - cpu_counter();
			vmcb->ctrl.tsc_offset = cpudata->tsc_offset +
			    curcpu()->ci_data.cpu_cc_skew;
			svm_vmcb_cache_flush(vmcb, VMCB_CTRL_VMCB_CLEAN_I);
			goto handled;
		}
		for (i = 0; i < __arraycount(msr_ignore_list); i++) {
			if (msr_ignore_list[i] != exit->u.msr.msr)
				continue;
			goto handled;
		}
		break;
	}

	return false;

handled:
	svm_inkernel_advance(cpudata->vmcb);
	return true;

error:
	svm_inject_gp(mach, vcpu);
	return true;
}

static void
svm_exit_msr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	uint64_t info = cpudata->vmcb->ctrl.exitinfo1;

	if (info == 0) {
		exit->u.msr.type = NVMM_EXIT_MSR_RDMSR;
	} else {
		exit->u.msr.type = NVMM_EXIT_MSR_WRMSR;
	}

	exit->u.msr.msr = (cpudata->gprs[NVMM_X64_GPR_RCX] & 0xFFFFFFFF);

	if (info == 1) {
		uint64_t rdx, rax;
		rdx = cpudata->gprs[NVMM_X64_GPR_RDX];
		rax = cpudata->vmcb->state.rax;
		exit->u.msr.val = (rdx << 32) | (rax & 0xFFFFFFFF);
	} else {
		exit->u.msr.val = 0;
	}

	if (svm_inkernel_handle_msr(mach, vcpu, exit)) {
		exit->reason = NVMM_EXIT_NONE;
		return;
	}

	exit->reason = NVMM_EXIT_MSR;
	exit->u.msr.npc = cpudata->vmcb->ctrl.nrip;
}

static void
svm_exit_npf(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	gpaddr_t gpa = cpudata->vmcb->ctrl.exitinfo2;

	exit->reason = NVMM_EXIT_MEMORY;
	if (cpudata->vmcb->ctrl.exitinfo1 & PGEX_W)
		exit->u.mem.perm = NVMM_EXIT_MEMORY_WRITE;
	else if (cpudata->vmcb->ctrl.exitinfo1 & PGEX_X)
		exit->u.mem.perm = NVMM_EXIT_MEMORY_EXEC;
	else
		exit->u.mem.perm = NVMM_EXIT_MEMORY_READ;
	exit->u.mem.gpa = gpa;
	exit->u.mem.inst_len = cpudata->vmcb->ctrl.inst_len;
	memcpy(exit->u.mem.inst_bytes, cpudata->vmcb->ctrl.inst_bytes,
	    sizeof(exit->u.mem.inst_bytes));
}

static void
svm_exit_insn(struct vmcb *vmcb, struct nvmm_exit *exit, uint64_t reason)
{
	exit->u.insn.npc = vmcb->ctrl.nrip;
	exit->reason = reason;
}

static void
svm_exit_xsetbv(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	uint64_t val;

	exit->reason = NVMM_EXIT_NONE;

	val = (cpudata->gprs[NVMM_X64_GPR_RDX] << 32) |
	    (vmcb->state.rax & 0xFFFFFFFF);

	if (__predict_false(cpudata->gprs[NVMM_X64_GPR_RCX] != 0)) {
		goto error;
	} else if (__predict_false(vmcb->state.cpl != 0)) {
		goto error;
	} else if (__predict_false((val & ~svm_xcr0_mask) != 0)) {
		goto error;
	} else if (__predict_false((val & XCR0_X87) == 0)) {
		goto error;
	}

	cpudata->gxcr0 = val;

	svm_inkernel_advance(cpudata->vmcb);
	return;

error:
	svm_inject_gp(mach, vcpu);
}

/* -------------------------------------------------------------------------- */

static void
svm_vcpu_guest_fpu_enter(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	cpudata->ts_set = (rcr0() & CR0_TS) != 0;

	fpu_area_save(&cpudata->hfpu, svm_xcr0_mask);
	fpu_area_restore(&cpudata->gfpu, svm_xcr0_mask);

	if (svm_xcr0_mask != 0) {
		cpudata->hxcr0 = rdxcr(0);
		wrxcr(0, cpudata->gxcr0);
	}
}

static void
svm_vcpu_guest_fpu_leave(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	if (svm_xcr0_mask != 0) {
		cpudata->gxcr0 = rdxcr(0);
		wrxcr(0, cpudata->hxcr0);
	}

	fpu_area_save(&cpudata->gfpu, svm_xcr0_mask);
	fpu_area_restore(&cpudata->hfpu, svm_xcr0_mask);

	if (cpudata->ts_set) {
		stts();
	}
}

static void
svm_vcpu_guest_dbregs_enter(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	x86_dbregs_save(curlwp);

	ldr7(0);

	ldr0(cpudata->drs[NVMM_X64_DR_DR0]);
	ldr1(cpudata->drs[NVMM_X64_DR_DR1]);
	ldr2(cpudata->drs[NVMM_X64_DR_DR2]);
	ldr3(cpudata->drs[NVMM_X64_DR_DR3]);
}

static void
svm_vcpu_guest_dbregs_leave(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	cpudata->drs[NVMM_X64_DR_DR0] = rdr0();
	cpudata->drs[NVMM_X64_DR_DR1] = rdr1();
	cpudata->drs[NVMM_X64_DR_DR2] = rdr2();
	cpudata->drs[NVMM_X64_DR_DR3] = rdr3();

	x86_dbregs_restore(curlwp);
}

static void
svm_vcpu_guest_misc_enter(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	cpudata->fsbase = rdmsr(MSR_FSBASE);
	cpudata->kernelgsbase = rdmsr(MSR_KERNELGSBASE);
}

static void
svm_vcpu_guest_misc_leave(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	wrmsr(MSR_STAR, cpudata->star);
	wrmsr(MSR_LSTAR, cpudata->lstar);
	wrmsr(MSR_CSTAR, cpudata->cstar);
	wrmsr(MSR_SFMASK, cpudata->sfmask);
	wrmsr(MSR_FSBASE, cpudata->fsbase);
	wrmsr(MSR_KERNELGSBASE, cpudata->kernelgsbase);
}

/* -------------------------------------------------------------------------- */

static inline void
svm_gtlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	if (vcpu->hcpu_last != hcpu || cpudata->shared_asid) {
		cpudata->gtlb_want_flush = true;
	}
}

static inline void
svm_htlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	/*
	 * Nothing to do. If an hTLB flush was needed, either the VCPU was
	 * executing on this hCPU and the hTLB already got flushed, or it
	 * was executing on another hCPU in which case the catchup is done
	 * in svm_gtlb_catchup().
	 */
}

static inline uint64_t
svm_htlb_flush(struct svm_machdata *machdata, struct svm_cpudata *cpudata)
{
	struct vmcb *vmcb = cpudata->vmcb;
	uint64_t machgen;

	machgen = machdata->mach_htlb_gen;
	if (__predict_true(machgen == cpudata->vcpu_htlb_gen)) {
		return machgen;
	}

	vmcb->ctrl.tlb_ctrl = svm_ctrl_tlb_flush;
	return machgen;
}

static inline void
svm_htlb_flush_ack(struct svm_cpudata *cpudata, uint64_t machgen)
{
	struct vmcb *vmcb = cpudata->vmcb;

	if (__predict_true(vmcb->ctrl.exitcode != VMCB_EXITCODE_INVALID)) {
		cpudata->vcpu_htlb_gen = machgen;
	}
}

static int
svm_vcpu_run(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct svm_machdata *machdata = mach->machdata;
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	uint64_t machgen;
	int hcpu, s;

	kpreempt_disable();
	hcpu = cpu_number();

	svm_gtlb_catchup(vcpu, hcpu);
	svm_htlb_catchup(vcpu, hcpu);

	if (vcpu->hcpu_last != hcpu) {
		vmcb->ctrl.tsc_offset = cpudata->tsc_offset +
		    curcpu()->ci_data.cpu_cc_skew;
		svm_vmcb_cache_flush_all(vmcb);
	}

	svm_vcpu_guest_dbregs_enter(vcpu);
	svm_vcpu_guest_misc_enter(vcpu);

	while (1) {
		if (cpudata->gtlb_want_flush) {
			vmcb->ctrl.tlb_ctrl = svm_ctrl_tlb_flush;
		} else {
			vmcb->ctrl.tlb_ctrl = 0;
		}

		s = splhigh();
		machgen = svm_htlb_flush(machdata, cpudata);
		svm_vcpu_guest_fpu_enter(vcpu);
		svm_vmrun(cpudata->vmcb_pa, cpudata->gprs);
		svm_vcpu_guest_fpu_leave(vcpu);
		svm_htlb_flush_ack(cpudata, machgen);
		splx(s);

		svm_vmcb_cache_default(vmcb);

		if (vmcb->ctrl.exitcode != VMCB_EXITCODE_INVALID) {
			cpudata->gtlb_want_flush = false;
			vcpu->hcpu_last = hcpu;
		}

		switch (vmcb->ctrl.exitcode) {
		case VMCB_EXITCODE_INTR:
		case VMCB_EXITCODE_NMI:
			exit->reason = NVMM_EXIT_NONE;
			break;
		case VMCB_EXITCODE_VINTR:
			svm_event_waitexit_disable(vcpu, false);
			exit->reason = NVMM_EXIT_INT_READY;
			break;
		case VMCB_EXITCODE_IRET:
			svm_event_waitexit_disable(vcpu, true);
			exit->reason = NVMM_EXIT_NMI_READY;
			break;
		case VMCB_EXITCODE_CPUID:
			svm_exit_cpuid(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_HLT:
			svm_exit_hlt(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_IOIO:
			svm_exit_io(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_MSR:
			svm_exit_msr(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_SHUTDOWN:
			exit->reason = NVMM_EXIT_SHUTDOWN;
			break;
		case VMCB_EXITCODE_RDPMC:
		case VMCB_EXITCODE_RSM:
		case VMCB_EXITCODE_INVLPGA:
		case VMCB_EXITCODE_VMRUN:
		case VMCB_EXITCODE_VMMCALL:
		case VMCB_EXITCODE_VMLOAD:
		case VMCB_EXITCODE_VMSAVE:
		case VMCB_EXITCODE_STGI:
		case VMCB_EXITCODE_CLGI:
		case VMCB_EXITCODE_SKINIT:
		case VMCB_EXITCODE_RDTSCP:
			svm_inject_ud(mach, vcpu);
			exit->reason = NVMM_EXIT_NONE;
			break;
		case VMCB_EXITCODE_MONITOR:
			svm_exit_insn(vmcb, exit, NVMM_EXIT_MONITOR);
			break;
		case VMCB_EXITCODE_MWAIT:
			svm_exit_insn(vmcb, exit, NVMM_EXIT_MWAIT);
			break;
		case VMCB_EXITCODE_MWAIT_CONDITIONAL:
			svm_exit_insn(vmcb, exit, NVMM_EXIT_MWAIT_COND);
			break;
		case VMCB_EXITCODE_XSETBV:
			svm_exit_xsetbv(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_NPF:
			svm_exit_npf(mach, vcpu, exit);
			break;
		case VMCB_EXITCODE_FERR_FREEZE: /* ? */
		default:
			exit->reason = NVMM_EXIT_INVALID;
			break;
		}

		/* If no reason to return to userland, keep rolling. */
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD) {
			break;
		}
		if (curcpu()->ci_data.cpu_softints != 0) {
			break;
		}
		if (curlwp->l_flag & LW_USERRET) {
			break;
		}
		if (exit->reason != NVMM_EXIT_NONE) {
			break;
		}
	}

	svm_vcpu_guest_misc_leave(vcpu);
	svm_vcpu_guest_dbregs_leave(vcpu);

	kpreempt_enable();

	exit->exitstate[NVMM_X64_EXITSTATE_CR8] = __SHIFTOUT(vmcb->ctrl.v,
	    VMCB_CTRL_V_TPR);
	exit->exitstate[NVMM_X64_EXITSTATE_RFLAGS] = vmcb->state.rflags;

	exit->exitstate[NVMM_X64_EXITSTATE_INT_SHADOW] =
	    ((vmcb->ctrl.intr & VMCB_CTRL_INTR_SHADOW) != 0);
	exit->exitstate[NVMM_X64_EXITSTATE_INT_WINDOW_EXIT] =
	    cpudata->int_window_exit;
	exit->exitstate[NVMM_X64_EXITSTATE_NMI_WINDOW_EXIT] =
	    cpudata->nmi_window_exit;

	return 0;
}

/* -------------------------------------------------------------------------- */

static int
svm_memalloc(paddr_t *pa, vaddr_t *va, size_t npages)
{
	struct pglist pglist;
	paddr_t _pa;
	vaddr_t _va;
	size_t i;
	int ret;

	ret = uvm_pglistalloc(npages * PAGE_SIZE, 0, ~0UL, PAGE_SIZE, 0,
	    &pglist, 1, 0);
	if (ret != 0)
		return ENOMEM;
	_pa = TAILQ_FIRST(&pglist)->phys_addr;
	_va = uvm_km_alloc(kernel_map, npages * PAGE_SIZE, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (_va == 0)
		goto error;

	for (i = 0; i < npages; i++) {
		pmap_kenter_pa(_va + i * PAGE_SIZE, _pa + i * PAGE_SIZE,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WRITE_BACK);
	}
	pmap_update(pmap_kernel());

	memset((void *)_va, 0, npages * PAGE_SIZE);

	*pa = _pa;
	*va = _va;
	return 0;

error:
	for (i = 0; i < npages; i++) {
		uvm_pagefree(PHYS_TO_VM_PAGE(_pa + i * PAGE_SIZE));
	}
	return ENOMEM;
}

static void
svm_memfree(paddr_t pa, vaddr_t va, size_t npages)
{
	size_t i;

	pmap_kremove(va, npages * PAGE_SIZE);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, va, npages * PAGE_SIZE, UVM_KMF_VAONLY);
	for (i = 0; i < npages; i++) {
		uvm_pagefree(PHYS_TO_VM_PAGE(pa + i * PAGE_SIZE));
	}
}

/* -------------------------------------------------------------------------- */

#define SVM_MSRBM_READ	__BIT(0)
#define SVM_MSRBM_WRITE	__BIT(1)

static void
svm_vcpu_msr_allow(uint8_t *bitmap, uint64_t msr, bool read, bool write)
{
	uint64_t byte;
	uint8_t bitoff;

	if (msr < 0x00002000) {
		/* Range 1 */
		byte = ((msr - 0x00000000) >> 2UL) + 0x0000;
	} else if (msr >= 0xC0000000 && msr < 0xC0002000) {
		/* Range 2 */
		byte = ((msr - 0xC0000000) >> 2UL) + 0x0800;
	} else if (msr >= 0xC0010000 && msr < 0xC0012000) {
		/* Range 3 */
		byte = ((msr - 0xC0010000) >> 2UL) + 0x1000;
	} else {
		panic("%s: wrong range", __func__);
	}

	bitoff = (msr & 0x3) << 1;

	if (read) {
		bitmap[byte] &= ~(SVM_MSRBM_READ << bitoff);
	}
	if (write) {
		bitmap[byte] &= ~(SVM_MSRBM_WRITE << bitoff);
	}
}



#define SVM_SEG_ATTRIB_TYPE		__BITS(4,0)
#define SVM_SEG_ATTRIB_DPL		__BITS(6,5)
#define SVM_SEG_ATTRIB_P		__BIT(7)
#define SVM_SEG_ATTRIB_AVL		__BIT(8)
#define SVM_SEG_ATTRIB_LONG		__BIT(9)
#define SVM_SEG_ATTRIB_DEF32		__BIT(10)
#define SVM_SEG_ATTRIB_GRAN		__BIT(11)

static void
svm_vcpu_setstate_seg(const struct nvmm_x64_state_seg *seg,
    struct vmcb_segment *vseg)
{
	vseg->selector = seg->selector;
	vseg->attrib =
	    __SHIFTIN(seg->attrib.type, SVM_SEG_ATTRIB_TYPE) |
	    __SHIFTIN(seg->attrib.dpl, SVM_SEG_ATTRIB_DPL) |
	    __SHIFTIN(seg->attrib.p, SVM_SEG_ATTRIB_P) |
	    __SHIFTIN(seg->attrib.avl, SVM_SEG_ATTRIB_AVL) |
	    __SHIFTIN(seg->attrib.lng, SVM_SEG_ATTRIB_LONG) |
	    __SHIFTIN(seg->attrib.def32, SVM_SEG_ATTRIB_DEF32) |
	    __SHIFTIN(seg->attrib.gran, SVM_SEG_ATTRIB_GRAN);
	vseg->limit = seg->limit;
	vseg->base = seg->base;
}

static void
svm_vcpu_getstate_seg(struct nvmm_x64_state_seg *seg, struct vmcb_segment *vseg)
{
	seg->selector = vseg->selector;
	seg->attrib.type = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_TYPE);
	seg->attrib.dpl = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_DPL);
	seg->attrib.p = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_P);
	seg->attrib.avl = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_AVL);
	seg->attrib.lng = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_LONG);
	seg->attrib.def32 = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_DEF32);
	seg->attrib.gran = __SHIFTOUT(vseg->attrib, SVM_SEG_ATTRIB_GRAN);
	seg->limit = vseg->limit;
	seg->base = vseg->base;
}

static inline bool
svm_state_tlb_flush(const struct vmcb *vmcb, const struct nvmm_x64_state *state,
    uint64_t flags)
{
	if (flags & NVMM_X64_STATE_CRS) {
		if ((vmcb->state.cr0 ^
		     state->crs[NVMM_X64_CR_CR0]) & CR0_TLB_FLUSH) {
			return true;
		}
		if (vmcb->state.cr3 != state->crs[NVMM_X64_CR_CR3]) {
			return true;
		}
		if ((vmcb->state.cr4 ^
		     state->crs[NVMM_X64_CR_CR4]) & CR4_TLB_FLUSH) {
			return true;
		}
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		if ((vmcb->state.efer ^
		     state->msrs[NVMM_X64_MSR_EFER]) & EFER_TLB_FLUSH) {
			return true;
		}
	}

	return false;
}

static void
svm_vcpu_setstate(struct nvmm_cpu *vcpu, const void *data, uint64_t flags)
{
	const struct nvmm_x64_state *state = data;
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	struct fxsave *fpustate;

	if (svm_state_tlb_flush(vmcb, state, flags)) {
		cpudata->gtlb_want_flush = true;
	}

	if (flags & NVMM_X64_STATE_SEGS) {
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_CS],
		    &vmcb->state.cs);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_DS],
		    &vmcb->state.ds);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_ES],
		    &vmcb->state.es);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_FS],
		    &vmcb->state.fs);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_GS],
		    &vmcb->state.gs);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_SS],
		    &vmcb->state.ss);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_GDT],
		    &vmcb->state.gdt);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_IDT],
		    &vmcb->state.idt);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_LDT],
		    &vmcb->state.ldt);
		svm_vcpu_setstate_seg(&state->segs[NVMM_X64_SEG_TR],
		    &vmcb->state.tr);

		vmcb->state.cpl = state->segs[NVMM_X64_SEG_SS].attrib.dpl;
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(cpudata->gprs, state->gprs, sizeof(state->gprs));

		vmcb->state.rip = state->gprs[NVMM_X64_GPR_RIP];
		vmcb->state.rsp = state->gprs[NVMM_X64_GPR_RSP];
		vmcb->state.rax = state->gprs[NVMM_X64_GPR_RAX];
		vmcb->state.rflags = state->gprs[NVMM_X64_GPR_RFLAGS];
	}

	if (flags & NVMM_X64_STATE_CRS) {
		vmcb->state.cr0 = state->crs[NVMM_X64_CR_CR0];
		vmcb->state.cr2 = state->crs[NVMM_X64_CR_CR2];
		vmcb->state.cr3 = state->crs[NVMM_X64_CR_CR3];
		vmcb->state.cr4 = state->crs[NVMM_X64_CR_CR4];

		vmcb->ctrl.v &= ~VMCB_CTRL_V_TPR;
		vmcb->ctrl.v |= __SHIFTIN(state->crs[NVMM_X64_CR_CR8],
		    VMCB_CTRL_V_TPR);

		if (svm_xcr0_mask != 0) {
			/* Clear illegal XCR0 bits, set mandatory X87 bit. */
			cpudata->gxcr0 = state->crs[NVMM_X64_CR_XCR0];
			cpudata->gxcr0 &= svm_xcr0_mask;
			cpudata->gxcr0 |= XCR0_X87;
		}
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(cpudata->drs, state->drs, sizeof(state->drs));

		vmcb->state.dr6 = state->drs[NVMM_X64_DR_DR6];
		vmcb->state.dr7 = state->drs[NVMM_X64_DR_DR7];
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		/*
		 * EFER_SVME is mandatory.
		 */
		vmcb->state.efer = state->msrs[NVMM_X64_MSR_EFER] | EFER_SVME;
		vmcb->state.star = state->msrs[NVMM_X64_MSR_STAR];
		vmcb->state.lstar = state->msrs[NVMM_X64_MSR_LSTAR];
		vmcb->state.cstar = state->msrs[NVMM_X64_MSR_CSTAR];
		vmcb->state.sfmask = state->msrs[NVMM_X64_MSR_SFMASK];
		vmcb->state.kernelgsbase =
		    state->msrs[NVMM_X64_MSR_KERNELGSBASE];
		vmcb->state.sysenter_cs =
		    state->msrs[NVMM_X64_MSR_SYSENTER_CS];
		vmcb->state.sysenter_esp =
		    state->msrs[NVMM_X64_MSR_SYSENTER_ESP];
		vmcb->state.sysenter_eip =
		    state->msrs[NVMM_X64_MSR_SYSENTER_EIP];
		vmcb->state.g_pat = state->msrs[NVMM_X64_MSR_PAT];
	}

	if (flags & NVMM_X64_STATE_MISC) {
		if (state->misc[NVMM_X64_MISC_INT_SHADOW]) {
			vmcb->ctrl.intr |= VMCB_CTRL_INTR_SHADOW;
		} else {
			vmcb->ctrl.intr &= ~VMCB_CTRL_INTR_SHADOW;
		}

		if (state->misc[NVMM_X64_MISC_INT_WINDOW_EXIT]) {
			svm_event_waitexit_enable(vcpu, false);
		} else {
			svm_event_waitexit_disable(vcpu, false);
		}

		if (state->misc[NVMM_X64_MISC_NMI_WINDOW_EXIT]) {
			svm_event_waitexit_enable(vcpu, true);
		} else {
			svm_event_waitexit_disable(vcpu, true);
		}
	}

	CTASSERT(sizeof(cpudata->gfpu.xsh_fxsave) == sizeof(state->fpu));
	if (flags & NVMM_X64_STATE_FPU) {
		memcpy(cpudata->gfpu.xsh_fxsave, &state->fpu,
		    sizeof(state->fpu));

		fpustate = (struct fxsave *)cpudata->gfpu.xsh_fxsave;
		fpustate->fx_mxcsr_mask &= x86_fpu_mxcsr_mask;
		fpustate->fx_mxcsr &= fpustate->fx_mxcsr_mask;

		if (svm_xcr0_mask != 0) {
			/* Reset XSTATE_BV, to force a reload. */
			cpudata->gfpu.xsh_xstate_bv = svm_xcr0_mask;
		}
	}

	svm_vmcb_cache_update(vmcb, flags);
}

static void
svm_vcpu_getstate(struct nvmm_cpu *vcpu, void *data, uint64_t flags)
{
	struct nvmm_x64_state *state = (struct nvmm_x64_state *)data;
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;

	if (flags & NVMM_X64_STATE_SEGS) {
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_CS],
		    &vmcb->state.cs);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_DS],
		    &vmcb->state.ds);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_ES],
		    &vmcb->state.es);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_FS],
		    &vmcb->state.fs);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_GS],
		    &vmcb->state.gs);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_SS],
		    &vmcb->state.ss);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_GDT],
		    &vmcb->state.gdt);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_IDT],
		    &vmcb->state.idt);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_LDT],
		    &vmcb->state.ldt);
		svm_vcpu_getstate_seg(&state->segs[NVMM_X64_SEG_TR],
		    &vmcb->state.tr);

		state->segs[NVMM_X64_SEG_SS].attrib.dpl = vmcb->state.cpl;
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(state->gprs, cpudata->gprs, sizeof(state->gprs));

		state->gprs[NVMM_X64_GPR_RIP] = vmcb->state.rip;
		state->gprs[NVMM_X64_GPR_RSP] = vmcb->state.rsp;
		state->gprs[NVMM_X64_GPR_RAX] = vmcb->state.rax;
		state->gprs[NVMM_X64_GPR_RFLAGS] = vmcb->state.rflags;
	}

	if (flags & NVMM_X64_STATE_CRS) {
		state->crs[NVMM_X64_CR_CR0] = vmcb->state.cr0;
		state->crs[NVMM_X64_CR_CR2] = vmcb->state.cr2;
		state->crs[NVMM_X64_CR_CR3] = vmcb->state.cr3;
		state->crs[NVMM_X64_CR_CR4] = vmcb->state.cr4;
		state->crs[NVMM_X64_CR_CR8] = __SHIFTOUT(vmcb->ctrl.v,
		    VMCB_CTRL_V_TPR);
		state->crs[NVMM_X64_CR_XCR0] = cpudata->gxcr0;
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(state->drs, cpudata->drs, sizeof(state->drs));

		state->drs[NVMM_X64_DR_DR6] = vmcb->state.dr6;
		state->drs[NVMM_X64_DR_DR7] = vmcb->state.dr7;
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		state->msrs[NVMM_X64_MSR_EFER] = vmcb->state.efer;
		state->msrs[NVMM_X64_MSR_STAR] = vmcb->state.star;
		state->msrs[NVMM_X64_MSR_LSTAR] = vmcb->state.lstar;
		state->msrs[NVMM_X64_MSR_CSTAR] = vmcb->state.cstar;
		state->msrs[NVMM_X64_MSR_SFMASK] = vmcb->state.sfmask;
		state->msrs[NVMM_X64_MSR_KERNELGSBASE] =
		    vmcb->state.kernelgsbase;
		state->msrs[NVMM_X64_MSR_SYSENTER_CS] =
		    vmcb->state.sysenter_cs;
		state->msrs[NVMM_X64_MSR_SYSENTER_ESP] =
		    vmcb->state.sysenter_esp;
		state->msrs[NVMM_X64_MSR_SYSENTER_EIP] =
		    vmcb->state.sysenter_eip;
		state->msrs[NVMM_X64_MSR_PAT] = vmcb->state.g_pat;

		/* Hide SVME. */
		state->msrs[NVMM_X64_MSR_EFER] &= ~EFER_SVME;
	}

	if (flags & NVMM_X64_STATE_MISC) {
		state->misc[NVMM_X64_MISC_INT_SHADOW] =
		    (vmcb->ctrl.intr & VMCB_CTRL_INTR_SHADOW) != 0;
		state->misc[NVMM_X64_MISC_INT_WINDOW_EXIT] =
		    cpudata->int_window_exit;
		state->misc[NVMM_X64_MISC_NMI_WINDOW_EXIT] =
		    cpudata->nmi_window_exit;
	}

	CTASSERT(sizeof(cpudata->gfpu.xsh_fxsave) == sizeof(state->fpu));
	if (flags & NVMM_X64_STATE_FPU) {
		memcpy(&state->fpu, cpudata->gfpu.xsh_fxsave,
		    sizeof(state->fpu));
	}
}

/* -------------------------------------------------------------------------- */

static void
svm_asid_alloc(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	size_t i, oct, bit;

	mutex_enter(&svm_asidlock);

	for (i = 0; i < svm_maxasid; i++) {
		oct = i / 8;
		bit = i % 8;

		if (svm_asidmap[oct] & __BIT(bit)) {
			continue;
		}

		svm_asidmap[oct] |= __BIT(bit);
		vmcb->ctrl.guest_asid = i;
		mutex_exit(&svm_asidlock);
		return;
	}

	/*
	 * No free ASID. Use the last one, which is shared and requires
	 * special TLB handling.
	 */
	cpudata->shared_asid = true;
	vmcb->ctrl.guest_asid = svm_maxasid - 1;
	mutex_exit(&svm_asidlock);
}

static void
svm_asid_free(struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;
	size_t oct, bit;

	if (cpudata->shared_asid) {
		return;
	}

	oct = vmcb->ctrl.guest_asid / 8;
	bit = vmcb->ctrl.guest_asid % 8;

	mutex_enter(&svm_asidlock);
	svm_asidmap[oct] &= ~__BIT(bit);
	mutex_exit(&svm_asidlock);
}

static void
svm_vcpu_init(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;
	struct vmcb *vmcb = cpudata->vmcb;

	/* Allow reads/writes of Control Registers. */
	vmcb->ctrl.intercept_cr = 0;

	/* Allow reads/writes of Debug Registers. */
	vmcb->ctrl.intercept_dr = 0;

	/* Allow exceptions 0 to 31. */
	vmcb->ctrl.intercept_vec = 0;

	/*
	 * Allow:
	 *  - SMI [smm interrupts]
	 *  - VINTR [virtual interrupts]
	 *  - CR0_SPEC [CR0 writes changing other fields than CR0.TS or CR0.MP]
	 *  - RIDTR [reads of IDTR]
	 *  - RGDTR [reads of GDTR]
	 *  - RLDTR [reads of LDTR]
	 *  - RTR [reads of TR]
	 *  - WIDTR [writes of IDTR]
	 *  - WGDTR [writes of GDTR]
	 *  - WLDTR [writes of LDTR]
	 *  - WTR [writes of TR]
	 *  - RDTSC [rdtsc instruction]
	 *  - PUSHF [pushf instruction]
	 *  - POPF [popf instruction]
	 *  - IRET [iret instruction]
	 *  - INTN [int $n instructions]
	 *  - INVD [invd instruction]
	 *  - PAUSE [pause instruction]
	 *  - INVLPG [invplg instruction]
	 *  - TASKSW [task switches]
	 *
	 * Intercept the rest below.
	 */
	vmcb->ctrl.intercept_misc1 =
	    VMCB_CTRL_INTERCEPT_INTR |
	    VMCB_CTRL_INTERCEPT_NMI |
	    VMCB_CTRL_INTERCEPT_INIT |
	    VMCB_CTRL_INTERCEPT_RDPMC |
	    VMCB_CTRL_INTERCEPT_CPUID |
	    VMCB_CTRL_INTERCEPT_RSM |
	    VMCB_CTRL_INTERCEPT_HLT |
	    VMCB_CTRL_INTERCEPT_INVLPGA |
	    VMCB_CTRL_INTERCEPT_IOIO_PROT |
	    VMCB_CTRL_INTERCEPT_MSR_PROT |
	    VMCB_CTRL_INTERCEPT_FERR_FREEZE |
	    VMCB_CTRL_INTERCEPT_SHUTDOWN;

	/*
	 * Allow:
	 *  - ICEBP [icebp instruction]
	 *  - WBINVD [wbinvd instruction]
	 *  - WCR_SPEC(0..15) [writes of CR0-15, received after instruction]
	 *
	 * Intercept the rest below.
	 */
	vmcb->ctrl.intercept_misc2 =
	    VMCB_CTRL_INTERCEPT_VMRUN |
	    VMCB_CTRL_INTERCEPT_VMMCALL |
	    VMCB_CTRL_INTERCEPT_VMLOAD |
	    VMCB_CTRL_INTERCEPT_VMSAVE |
	    VMCB_CTRL_INTERCEPT_STGI |
	    VMCB_CTRL_INTERCEPT_CLGI |
	    VMCB_CTRL_INTERCEPT_SKINIT |
	    VMCB_CTRL_INTERCEPT_RDTSCP |
	    VMCB_CTRL_INTERCEPT_MONITOR |
	    VMCB_CTRL_INTERCEPT_MWAIT |
	    VMCB_CTRL_INTERCEPT_XSETBV;

	/* Intercept all I/O accesses. */
	memset(cpudata->iobm, 0xFF, IOBM_SIZE);
	vmcb->ctrl.iopm_base_pa = cpudata->iobm_pa;

	/* Allow direct access to certain MSRs. */
	memset(cpudata->msrbm, 0xFF, MSRBM_SIZE);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_EFER, true, false);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_STAR, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_LSTAR, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_CSTAR, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_SFMASK, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_KERNELGSBASE, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_CS, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_ESP, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_EIP, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_FSBASE, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_GSBASE, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_CR_PAT, true, true);
	svm_vcpu_msr_allow(cpudata->msrbm, MSR_TSC, true, false);
	vmcb->ctrl.msrpm_base_pa = cpudata->msrbm_pa;

	/* Generate ASID. */
	svm_asid_alloc(vcpu);

	/* Virtual TPR. */
	vmcb->ctrl.v = VMCB_CTRL_V_INTR_MASKING;

	/* Enable Nested Paging. */
	vmcb->ctrl.enable1 = VMCB_CTRL_ENABLE_NP;
	vmcb->ctrl.n_cr3 = mach->vm->vm_map.pmap->pm_pdirpa[0];

	/* Init XSAVE header. */
	cpudata->gfpu.xsh_xstate_bv = svm_xcr0_mask;
	cpudata->gfpu.xsh_xcomp_bv = 0;

	/* Set guest TSC to zero, more or less. */
	cpudata->tsc_offset = -cpu_counter();

	/* These MSRs are static. */
	cpudata->star = rdmsr(MSR_STAR);
	cpudata->lstar = rdmsr(MSR_LSTAR);
	cpudata->cstar = rdmsr(MSR_CSTAR);
	cpudata->sfmask = rdmsr(MSR_SFMASK);

	/* Install the RESET state. */
	svm_vcpu_setstate(vcpu, &nvmm_x86_reset_state, NVMM_X64_STATE_ALL);
}

static int
svm_vcpu_create(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata;
	int error;

	/* Allocate the SVM cpudata. */
	cpudata = (struct svm_cpudata *)uvm_km_alloc(kernel_map,
	    roundup(sizeof(*cpudata), PAGE_SIZE), 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	vcpu->cpudata = cpudata;

	/* VMCB */
	error = svm_memalloc(&cpudata->vmcb_pa, (vaddr_t *)&cpudata->vmcb,
	    VMCB_NPAGES);
	if (error)
		goto error;

	/* I/O Bitmap */
	error = svm_memalloc(&cpudata->iobm_pa, (vaddr_t *)&cpudata->iobm,
	    IOBM_NPAGES);
	if (error)
		goto error;

	/* MSR Bitmap */
	error = svm_memalloc(&cpudata->msrbm_pa, (vaddr_t *)&cpudata->msrbm,
	    MSRBM_NPAGES);
	if (error)
		goto error;

	/* Init the VCPU info. */
	svm_vcpu_init(mach, vcpu);

	return 0;

error:
	if (cpudata->vmcb_pa) {
		svm_memfree(cpudata->vmcb_pa, (vaddr_t)cpudata->vmcb,
		    VMCB_NPAGES);
	}
	if (cpudata->iobm_pa) {
		svm_memfree(cpudata->iobm_pa, (vaddr_t)cpudata->iobm,
		    IOBM_NPAGES);
	}
	if (cpudata->msrbm_pa) {
		svm_memfree(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm,
		    MSRBM_NPAGES);
	}
	uvm_km_free(kernel_map, (vaddr_t)cpudata,
	    roundup(sizeof(*cpudata), PAGE_SIZE), UVM_KMF_WIRED);
	return error;
}

static void
svm_vcpu_destroy(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct svm_cpudata *cpudata = vcpu->cpudata;

	svm_asid_free(vcpu);

	svm_memfree(cpudata->vmcb_pa, (vaddr_t)cpudata->vmcb, VMCB_NPAGES);
	svm_memfree(cpudata->iobm_pa, (vaddr_t)cpudata->iobm, IOBM_NPAGES);
	svm_memfree(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm, MSRBM_NPAGES);

	uvm_km_free(kernel_map, (vaddr_t)cpudata,
	    roundup(sizeof(*cpudata), PAGE_SIZE), UVM_KMF_WIRED);
}

/* -------------------------------------------------------------------------- */

static void
svm_tlb_flush(struct pmap *pm)
{
	struct nvmm_machine *mach = pm->pm_data;
	struct svm_machdata *machdata = mach->machdata;

	atomic_inc_64(&machdata->mach_htlb_gen);

	/* Generates IPIs, which cause #VMEXITs. */
	pmap_tlb_shootdown(pmap_kernel(), -1, PG_G, TLBSHOOT_UPDATE);
}

static void
svm_machine_create(struct nvmm_machine *mach)
{
	struct svm_machdata *machdata;

	/* Fill in pmap info. */
	mach->vm->vm_map.pmap->pm_data = (void *)mach;
	mach->vm->vm_map.pmap->pm_tlb_flush = svm_tlb_flush;

	machdata = kmem_zalloc(sizeof(struct svm_machdata), KM_SLEEP);
	mach->machdata = machdata;

	/* Start with an hTLB flush everywhere. */
	machdata->mach_htlb_gen = 1;
}

static void
svm_machine_destroy(struct nvmm_machine *mach)
{
	kmem_free(mach->machdata, sizeof(struct svm_machdata));
}

static int
svm_machine_configure(struct nvmm_machine *mach, uint64_t op, void *data)
{
	struct nvmm_x86_conf_cpuid *cpuid = data;
	struct svm_machdata *machdata = (struct svm_machdata *)mach->machdata;
	size_t i;

	if (__predict_false(op != NVMM_X86_CONF_CPUID)) {
		return EINVAL;
	}

	if (__predict_false((cpuid->set.eax & cpuid->del.eax) ||
	    (cpuid->set.ebx & cpuid->del.ebx) ||
	    (cpuid->set.ecx & cpuid->del.ecx) ||
	    (cpuid->set.edx & cpuid->del.edx))) {
		return EINVAL;
	}

	/* If already here, replace. */
	for (i = 0; i < SVM_NCPUIDS; i++) {
		if (!machdata->cpuidpresent[i]) {
			continue;
		}
		if (machdata->cpuid[i].leaf == cpuid->leaf) {
			memcpy(&machdata->cpuid[i], cpuid,
			    sizeof(struct nvmm_x86_conf_cpuid));
			return 0;
		}
	}

	/* Not here, insert. */
	for (i = 0; i < SVM_NCPUIDS; i++) {
		if (!machdata->cpuidpresent[i]) {
			machdata->cpuidpresent[i] = true;
			memcpy(&machdata->cpuid[i], cpuid,
			    sizeof(struct nvmm_x86_conf_cpuid));
			return 0;
		}
	}

	return ENOBUFS;
}

/* -------------------------------------------------------------------------- */

static bool
svm_ident(void)
{
	u_int descs[4];
	uint64_t msr;

	if (cpu_vendor != CPUVENDOR_AMD) {
		return false;
	}
	if (!(cpu_feature[3] & CPUID_SVM)) {
		return false;
	}

	if (curcpu()->ci_max_ext_cpuid < 0x8000000a) {
		return false;
	}
	x86_cpuid(0x8000000a, descs);

	/* Want Nested Paging. */
	if (!(descs[3] & CPUID_AMD_SVM_NP)) {
		return false;
	}

	/* Want nRIP. */
	if (!(descs[3] & CPUID_AMD_SVM_NRIPS)) {
		return false;
	}

	svm_decode_assist = (descs[3] & CPUID_AMD_SVM_DecodeAssist) != 0;

	msr = rdmsr(MSR_VMCR);
	if ((msr & VMCR_SVMED) && (msr & VMCR_LOCK)) {
		return false;
	}

	return true;
}

static void
svm_init_asid(uint32_t maxasid)
{
	size_t i, j, allocsz;

	mutex_init(&svm_asidlock, MUTEX_DEFAULT, IPL_NONE);

	/* Arbitrarily limit. */
	maxasid = uimin(maxasid, 8192);

	svm_maxasid = maxasid;
	allocsz = roundup(maxasid, 8) / 8;
	svm_asidmap = kmem_zalloc(allocsz, KM_SLEEP);

	/* ASID 0 is reserved for the host. */
	svm_asidmap[0] |= __BIT(0);

	/* ASID n-1 is special, we share it. */
	i = (maxasid - 1) / 8;
	j = (maxasid - 1) % 8;
	svm_asidmap[i] |= __BIT(j);
}

static void
svm_change_cpu(void *arg1, void *arg2)
{
	bool enable = (bool)arg1;
	uint64_t msr;

	msr = rdmsr(MSR_VMCR);
	if (msr & VMCR_SVMED) {
		wrmsr(MSR_VMCR, msr & ~VMCR_SVMED);
	}

	if (!enable) {
		wrmsr(MSR_VM_HSAVE_PA, 0);
	}

	msr = rdmsr(MSR_EFER);
	if (enable) {
		msr |= EFER_SVME;
	} else {
		msr &= ~EFER_SVME;
	}
	wrmsr(MSR_EFER, msr);

	if (enable) {
		wrmsr(MSR_VM_HSAVE_PA, hsave[cpu_index(curcpu())].pa);
	}
}

static void
svm_init(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct vm_page *pg;
	u_int descs[4];
	uint64_t xc;

	x86_cpuid(0x8000000a, descs);

	/* The guest TLB flush command. */
	if (descs[3] & CPUID_AMD_SVM_FlushByASID) {
		svm_ctrl_tlb_flush = VMCB_CTRL_TLB_CTRL_FLUSH_GUEST;
	} else {
		svm_ctrl_tlb_flush = VMCB_CTRL_TLB_CTRL_FLUSH_ALL;
	}

	/* Init the ASID. */
	svm_init_asid(descs[1]);

	/* Init the XCR0 mask. */
	svm_xcr0_mask = SVM_XCR0_MASK_DEFAULT & x86_xsave_features;

	memset(hsave, 0, sizeof(hsave));
	for (CPU_INFO_FOREACH(cii, ci)) {
		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
		hsave[cpu_index(ci)].pa = VM_PAGE_TO_PHYS(pg);
	}

	xc = xc_broadcast(0, svm_change_cpu, (void *)true, NULL);
	xc_wait(xc);
}

static void
svm_fini_asid(void)
{
	size_t allocsz;

	allocsz = roundup(svm_maxasid, 8) / 8;
	kmem_free(svm_asidmap, allocsz);

	mutex_destroy(&svm_asidlock);
}

static void
svm_fini(void)
{
	uint64_t xc;
	size_t i;

	xc = xc_broadcast(0, svm_change_cpu, (void *)false, NULL);
	xc_wait(xc);

	for (i = 0; i < MAXCPUS; i++) {
		if (hsave[i].pa != 0)
			uvm_pagefree(PHYS_TO_VM_PAGE(hsave[i].pa));
	}

	svm_fini_asid();
}

static void
svm_capability(struct nvmm_capability *cap)
{
	cap->u.x86.xcr0_mask = svm_xcr0_mask;
	cap->u.x86.mxcsr_mask = x86_fpu_mxcsr_mask;
	cap->u.x86.conf_cpuid_maxops = SVM_NCPUIDS;
}

const struct nvmm_impl nvmm_x86_svm = {
	.ident = svm_ident,
	.init = svm_init,
	.fini = svm_fini,
	.capability = svm_capability,
	.conf_max = NVMM_X86_NCONF,
	.conf_sizes = svm_conf_sizes,
	.state_size = sizeof(struct nvmm_x64_state),
	.machine_create = svm_machine_create,
	.machine_destroy = svm_machine_destroy,
	.machine_configure = svm_machine_configure,
	.vcpu_create = svm_vcpu_create,
	.vcpu_destroy = svm_vcpu_destroy,
	.vcpu_setstate = svm_vcpu_setstate,
	.vcpu_getstate = svm_vcpu_getstate,
	.vcpu_inject = svm_vcpu_inject,
	.vcpu_run = svm_vcpu_run
};
