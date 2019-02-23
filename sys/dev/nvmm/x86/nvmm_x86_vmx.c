/*	$NetBSD: nvmm_x86_vmx.c,v 1.14 2019/02/23 12:27:00 maxv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nvmm_x86_vmx.c,v 1.14 2019/02/23 12:27:00 maxv Exp $");

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

int _vmx_vmxon(paddr_t *pa);
int _vmx_vmxoff(void);
int _vmx_invept(uint64_t op, void *desc);
int _vmx_invvpid(uint64_t op, void *desc);
int _vmx_vmread(uint64_t op, uint64_t *val);
int _vmx_vmwrite(uint64_t op, uint64_t val);
int _vmx_vmptrld(paddr_t *pa);
int _vmx_vmptrst(paddr_t *pa);
int _vmx_vmclear(paddr_t *pa);
int vmx_vmlaunch(uint64_t *gprs);
int vmx_vmresume(uint64_t *gprs);

#define vmx_vmxon(a) \
	if (__predict_false(_vmx_vmxon(a) != 0)) { \
		panic("%s: VMXON failed", __func__); \
	}
#define vmx_vmxoff() \
	if (__predict_false(_vmx_vmxoff() != 0)) { \
		panic("%s: VMXOFF failed", __func__); \
	}
#define vmx_invept(a, b) \
	if (__predict_false(_vmx_invept(a, b) != 0)) { \
		panic("%s: INVEPT failed", __func__); \
	}
#define vmx_invvpid(a, b) \
	if (__predict_false(_vmx_invvpid(a, b) != 0)) { \
		panic("%s: INVVPID failed", __func__); \
	}
#define vmx_vmread(a, b) \
	if (__predict_false(_vmx_vmread(a, b) != 0)) { \
		panic("%s: VMREAD failed", __func__); \
	}
#define vmx_vmwrite(a, b) \
	if (__predict_false(_vmx_vmwrite(a, b) != 0)) { \
		panic("%s: VMWRITE failed", __func__); \
	}
#define vmx_vmptrld(a) \
	if (__predict_false(_vmx_vmptrld(a) != 0)) { \
		panic("%s: VMPTRLD failed", __func__); \
	}
#define vmx_vmptrst(a) \
	if (__predict_false(_vmx_vmptrst(a) != 0)) { \
		panic("%s: VMPTRST failed", __func__); \
	}
#define vmx_vmclear(a) \
	if (__predict_false(_vmx_vmclear(a) != 0)) { \
		panic("%s: VMCLEAR failed", __func__); \
	}

#define MSR_IA32_FEATURE_CONTROL	0x003A
#define		IA32_FEATURE_CONTROL_LOCK	__BIT(0)
#define		IA32_FEATURE_CONTROL_IN_SMX	__BIT(1)
#define		IA32_FEATURE_CONTROL_OUT_SMX	__BIT(2)

#define MSR_IA32_VMX_BASIC		0x0480
#define		IA32_VMX_BASIC_IDENT		__BITS(30,0)
#define		IA32_VMX_BASIC_DATA_SIZE	__BITS(44,32)
#define		IA32_VMX_BASIC_MEM_WIDTH	__BIT(48)
#define		IA32_VMX_BASIC_DUAL		__BIT(49)
#define		IA32_VMX_BASIC_MEM_TYPE		__BITS(53,50)
#define			MEM_TYPE_UC		0
#define			MEM_TYPE_WB		6
#define		IA32_VMX_BASIC_IO_REPORT	__BIT(54)
#define		IA32_VMX_BASIC_TRUE_CTLS	__BIT(55)

#define MSR_IA32_VMX_PINBASED_CTLS		0x0481
#define MSR_IA32_VMX_PROCBASED_CTLS		0x0482
#define MSR_IA32_VMX_EXIT_CTLS			0x0483
#define MSR_IA32_VMX_ENTRY_CTLS			0x0484
#define MSR_IA32_VMX_PROCBASED_CTLS2		0x048B

#define MSR_IA32_VMX_TRUE_PINBASED_CTLS		0x048D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS	0x048E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS		0x048F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS		0x0490

#define MSR_IA32_VMX_CR0_FIXED0			0x0486
#define MSR_IA32_VMX_CR0_FIXED1			0x0487
#define MSR_IA32_VMX_CR4_FIXED0			0x0488
#define MSR_IA32_VMX_CR4_FIXED1			0x0489

#define MSR_IA32_VMX_EPT_VPID_CAP	0x048C
#define		IA32_VMX_EPT_VPID_WALKLENGTH_4		__BIT(6)
#define		IA32_VMX_EPT_VPID_UC			__BIT(8)
#define		IA32_VMX_EPT_VPID_WB			__BIT(14)
#define		IA32_VMX_EPT_VPID_INVEPT		__BIT(20)
#define		IA32_VMX_EPT_VPID_FLAGS_AD		__BIT(21)
#define		IA32_VMX_EPT_VPID_INVEPT_CONTEXT	__BIT(25)
#define		IA32_VMX_EPT_VPID_INVEPT_ALL		__BIT(26)
#define		IA32_VMX_EPT_VPID_INVVPID		__BIT(32)
#define		IA32_VMX_EPT_VPID_INVVPID_ADDR		__BIT(40)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT	__BIT(41)
#define		IA32_VMX_EPT_VPID_INVVPID_ALL		__BIT(42)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT_NOG	__BIT(43)

/* -------------------------------------------------------------------------- */

/* 16-bit control fields */
#define VMCS_VPID				0x00000000
#define VMCS_PIR_VECTOR				0x00000002
#define VMCS_EPTP_INDEX				0x00000004
/* 16-bit guest-state fields */
#define VMCS_GUEST_ES_SELECTOR			0x00000800
#define VMCS_GUEST_CS_SELECTOR			0x00000802
#define VMCS_GUEST_SS_SELECTOR			0x00000804
#define VMCS_GUEST_DS_SELECTOR			0x00000806
#define VMCS_GUEST_FS_SELECTOR			0x00000808
#define VMCS_GUEST_GS_SELECTOR			0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR		0x0000080C
#define VMCS_GUEST_TR_SELECTOR			0x0000080E
#define VMCS_GUEST_INTR_STATUS			0x00000810
#define VMCS_PML_INDEX				0x00000812
/* 16-bit host-state fields */
#define VMCS_HOST_ES_SELECTOR			0x00000C00
#define VMCS_HOST_CS_SELECTOR			0x00000C02
#define VMCS_HOST_SS_SELECTOR			0x00000C04
#define VMCS_HOST_DS_SELECTOR			0x00000C06
#define VMCS_HOST_FS_SELECTOR			0x00000C08
#define VMCS_HOST_GS_SELECTOR			0x00000C0A
#define VMCS_HOST_TR_SELECTOR			0x00000C0C
/* 64-bit control fields */
#define VMCS_IO_BITMAP_A			0x00002000
#define VMCS_IO_BITMAP_B			0x00002002
#define VMCS_MSR_BITMAP				0x00002004
#define VMCS_EXIT_MSR_STORE_ADDRESS		0x00002006
#define VMCS_EXIT_MSR_LOAD_ADDRESS		0x00002008
#define VMCS_ENTRY_MSR_LOAD_ADDRESS		0x0000200A
#define VMCS_EXECUTIVE_VMCS			0x0000200C
#define VMCS_PML_ADDRESS			0x0000200E
#define VMCS_TSC_OFFSET				0x00002010
#define VMCS_VIRTUAL_APIC			0x00002012
#define VMCS_APIC_ACCESS			0x00002014
#define VMCS_PIR_DESC				0x00002016
#define VMCS_VM_CONTROL				0x00002018
#define VMCS_EPTP				0x0000201A
#define		EPTP_TYPE			__BITS(2,0)
#define			EPTP_TYPE_UC		0
#define			EPTP_TYPE_WB		6
#define		EPTP_WALKLEN			__BITS(5,3)
#define		EPTP_FLAGS_AD			__BIT(6)
#define		EPTP_PHYSADDR			__BITS(63,12)
#define VMCS_EOI_EXIT0				0x0000201C
#define VMCS_EOI_EXIT1				0x0000201E
#define VMCS_EOI_EXIT2				0x00002020
#define VMCS_EOI_EXIT3				0x00002022
#define VMCS_EPTP_LIST				0x00002024
#define VMCS_VMREAD_BITMAP			0x00002026
#define VMCS_VMWRITE_BITMAP			0x00002028
#define VMCS_VIRTUAL_EXCEPTION			0x0000202A
#define VMCS_XSS_EXIT_BITMAP			0x0000202C
#define VMCS_ENCLS_EXIT_BITMAP			0x0000202E
#define VMCS_TSC_MULTIPLIER			0x00002032
/* 64-bit read-only fields */
#define VMCS_GUEST_PHYSICAL_ADDRESS		0x00002400
/* 64-bit guest-state fields */
#define VMCS_LINK_POINTER			0x00002800
#define VMCS_GUEST_IA32_DEBUGCTL		0x00002802
#define VMCS_GUEST_IA32_PAT			0x00002804
#define VMCS_GUEST_IA32_EFER			0x00002806
#define VMCS_GUEST_IA32_PERF_GLOBAL_CTRL	0x00002808
#define VMCS_GUEST_PDPTE0			0x0000280A
#define VMCS_GUEST_PDPTE1			0x0000280C
#define VMCS_GUEST_PDPTE2			0x0000280E
#define VMCS_GUEST_PDPTE3			0x00002810
#define VMCS_GUEST_BNDCFGS			0x00002812
/* 64-bit host-state fields */
#define VMCS_HOST_IA32_PAT			0x00002C00
#define VMCS_HOST_IA32_EFER			0x00002C02
#define VMCS_HOST_IA32_PERF_GLOBAL_CTRL		0x00002C04
/* 32-bit control fields */
#define VMCS_PINBASED_CTLS			0x00004000
#define		PIN_CTLS_INT_EXITING		__BIT(0)
#define		PIN_CTLS_NMI_EXITING		__BIT(3)
#define		PIN_CTLS_VIRTUAL_NMIS		__BIT(5)
#define		PIN_CTLS_ACTIVATE_PREEMPT_TIMER	__BIT(6)
#define		PIN_CTLS_PROCESS_POSTEd_INTS	__BIT(7)
#define VMCS_PROCBASED_CTLS			0x00004002
#define		PROC_CTLS_INT_WINDOW_EXITING	__BIT(2)
#define		PROC_CTLS_USE_TSC_OFFSETTING	__BIT(3)
#define		PROC_CTLS_HLT_EXITING		__BIT(7)
#define		PROC_CTLS_INVLPG_EXITING	__BIT(9)
#define		PROC_CTLS_MWAIT_EXITING		__BIT(10)
#define		PROC_CTLS_RDPMC_EXITING		__BIT(11)
#define		PROC_CTLS_RDTSC_EXITING		__BIT(12)
#define		PROC_CTLS_RCR3_EXITING		__BIT(15)
#define		PROC_CTLS_LCR3_EXITING		__BIT(16)
#define		PROC_CTLS_RCR8_EXITING		__BIT(19)
#define		PROC_CTLS_LCR8_EXITING		__BIT(20)
#define		PROC_CTLS_USE_TPR_SHADOW	__BIT(21)
#define		PROC_CTLS_NMI_WINDOW_EXITING	__BIT(22)
#define		PROC_CTLS_DR_EXITING		__BIT(23)
#define		PROC_CTLS_UNCOND_IO_EXITING	__BIT(24)
#define		PROC_CTLS_USE_IO_BITMAPS	__BIT(25)
#define		PROC_CTLS_MONITOR_TRAP_FLAG	__BIT(27)
#define		PROC_CTLS_USE_MSR_BITMAPS	__BIT(28)
#define		PROC_CTLS_MONITOR_EXITING	__BIT(29)
#define		PROC_CTLS_PAUSE_EXITING		__BIT(30)
#define		PROC_CTLS_ACTIVATE_CTLS2	__BIT(31)
#define VMCS_EXCEPTION_BITMAP			0x00004004
#define VMCS_PF_ERROR_MASK			0x00004006
#define VMCS_PF_ERROR_MATCH			0x00004008
#define VMCS_CR3_TARGET_COUNT			0x0000400A
#define VMCS_EXIT_CTLS				0x0000400C
#define		EXIT_CTLS_SAVE_DEBUG_CONTROLS	__BIT(2)
#define		EXIT_CTLS_HOST_LONG_MODE	__BIT(9)
#define		EXIT_CTLS_LOAD_PERFGLOBALCTRL	__BIT(12)
#define		EXIT_CTLS_ACK_INTERRUPT		__BIT(15)
#define		EXIT_CTLS_SAVE_PAT		__BIT(18)
#define		EXIT_CTLS_LOAD_PAT		__BIT(19)
#define		EXIT_CTLS_SAVE_EFER		__BIT(20)
#define		EXIT_CTLS_LOAD_EFER		__BIT(21)
#define		EXIT_CTLS_SAVE_PREEMPT_TIMER	__BIT(22)
#define		EXIT_CTLS_CLEAR_BNDCFGS		__BIT(23)
#define		EXIT_CTLS_CONCEAL_PT		__BIT(24)
#define VMCS_EXIT_MSR_STORE_COUNT		0x0000400E
#define VMCS_EXIT_MSR_LOAD_COUNT		0x00004010
#define VMCS_ENTRY_CTLS				0x00004012
#define		ENTRY_CTLS_LOAD_DEBUG_CONTROLS	__BIT(2)
#define		ENTRY_CTLS_LONG_MODE		__BIT(9)
#define		ENTRY_CTLS_SMM			__BIT(10)
#define		ENTRY_CTLS_DISABLE_DUAL		__BIT(11)
#define		ENTRY_CTLS_LOAD_PERFGLOBALCTRL	__BIT(13)
#define		ENTRY_CTLS_LOAD_PAT		__BIT(14)
#define		ENTRY_CTLS_LOAD_EFER		__BIT(15)
#define		ENTRY_CTLS_LOAD_BNDCFGS		__BIT(16)
#define		ENTRY_CTLS_CONCEAL_PT		__BIT(17)
#define VMCS_ENTRY_MSR_LOAD_COUNT		0x00004014
#define VMCS_ENTRY_INTR_INFO			0x00004016
#define		INTR_INFO_VECTOR		__BITS(7,0)
#define		INTR_INFO_TYPE_EXT_INT		(0 << 8)
#define		INTR_INFO_TYPE_NMI		(2 << 8)
#define		INTR_INFO_TYPE_HW_EXC		(3 << 8)
#define		INTR_INFO_TYPE_SW_INT		(4 << 8)
#define		INTR_INFO_TYPE_PRIV_SW_EXC	(5 << 8)
#define		INTR_INFO_TYPE_SW_EXC		(6 << 8)
#define		INTR_INFO_TYPE_OTHER		(7 << 8)
#define		INTR_INFO_ERROR			__BIT(11)
#define		INTR_INFO_VALID			__BIT(31)
#define VMCS_ENTRY_EXCEPTION_ERROR		0x00004018
#define VMCS_ENTRY_INST_LENGTH			0x0000401A
#define VMCS_TPR_THRESHOLD			0x0000401C
#define VMCS_PROCBASED_CTLS2			0x0000401E
#define		PROC_CTLS2_VIRT_APIC_ACCESSES	__BIT(0)
#define		PROC_CTLS2_ENABLE_EPT		__BIT(1)
#define		PROC_CTLS2_DESC_TABLE_EXITING	__BIT(2)
#define		PROC_CTLS2_ENABLE_RDTSCP	__BIT(3)
#define		PROC_CTLS2_VIRT_X2APIC		__BIT(4)
#define		PROC_CTLS2_ENABLE_VPID		__BIT(5)
#define		PROC_CTLS2_WBINVD_EXITING	__BIT(6)
#define		PROC_CTLS2_UNRESTRICTED_GUEST	__BIT(7)
#define		PROC_CTLS2_APIC_REG_VIRT	__BIT(8)
#define		PROC_CTLS2_VIRT_INT_DELIVERY	__BIT(9)
#define		PROC_CTLS2_PAUSE_LOOP_EXITING	__BIT(10)
#define		PROC_CTLS2_RDRAND_EXITING	__BIT(11)
#define		PROC_CTLS2_INVPCID_ENABLE	__BIT(12)
#define		PROC_CTLS2_VMFUNC_ENABLE	__BIT(13)
#define		PROC_CTLS2_VMCS_SHADOWING	__BIT(14)
#define		PROC_CTLS2_ENCLS_EXITING	__BIT(15)
#define		PROC_CTLS2_RDSEED_EXITING	__BIT(16)
#define		PROC_CTLS2_PML_ENABLE		__BIT(17)
#define		PROC_CTLS2_EPT_VIOLATION	__BIT(18)
#define		PROC_CTLS2_CONCEAL_VMX_FROM_PT	__BIT(19)
#define		PROC_CTLS2_XSAVES_ENABLE	__BIT(20)
#define		PROC_CTLS2_MODE_BASED_EXEC_EPT	__BIT(22)
#define		PROC_CTLS2_USE_TSC_SCALING	__BIT(25)
#define VMCS_PLE_GAP				0x00004020
#define VMCS_PLE_WINDOW				0x00004022
/* 32-bit read-only data fields */
#define VMCS_INSTRUCTION_ERROR			0x00004400
#define VMCS_EXIT_REASON			0x00004402
#define VMCS_EXIT_INTR_INFO			0x00004404
#define VMCS_EXIT_INTR_ERRCODE			0x00004406
#define VMCS_IDT_VECTORING_INFO			0x00004408
#define VMCS_IDT_VECTORING_ERROR		0x0000440A
#define VMCS_EXIT_INSTRUCTION_LENGTH		0x0000440C
#define VMCS_EXIT_INSTRUCTION_INFO		0x0000440E
/* 32-bit guest-state fields */
#define VMCS_GUEST_ES_LIMIT			0x00004800
#define VMCS_GUEST_CS_LIMIT			0x00004802
#define VMCS_GUEST_SS_LIMIT			0x00004804
#define VMCS_GUEST_DS_LIMIT			0x00004806
#define VMCS_GUEST_FS_LIMIT			0x00004808
#define VMCS_GUEST_GS_LIMIT			0x0000480A
#define VMCS_GUEST_LDTR_LIMIT			0x0000480C
#define VMCS_GUEST_TR_LIMIT			0x0000480E
#define VMCS_GUEST_GDTR_LIMIT			0x00004810
#define VMCS_GUEST_IDTR_LIMIT			0x00004812
#define VMCS_GUEST_ES_ACCESS_RIGHTS		0x00004814
#define VMCS_GUEST_CS_ACCESS_RIGHTS		0x00004816
#define VMCS_GUEST_SS_ACCESS_RIGHTS		0x00004818
#define VMCS_GUEST_DS_ACCESS_RIGHTS		0x0000481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS		0x0000481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS		0x0000481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS		0x00004820
#define VMCS_GUEST_TR_ACCESS_RIGHTS		0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY		0x00004824
#define		INT_STATE_STI			__BIT(0)
#define		INT_STATE_MOVSS			__BIT(1)
#define		INT_STATE_SMI			__BIT(2)
#define		INT_STATE_NMI			__BIT(3)
#define		INT_STATE_ENCLAVE		__BIT(4)
#define VMCS_GUEST_ACTIVITY			0x00004826
#define VMCS_GUEST_SMBASE			0x00004828
#define VMCS_GUEST_IA32_SYSENTER_CS		0x0000482A
#define VMCS_PREEMPTION_TIMER_VALUE		0x0000482E
/* 32-bit host state fields */
#define VMCS_HOST_IA32_SYSENTER_CS		0x00004C00
/* Natural-Width control fields */
#define VMCS_CR0_MASK				0x00006000
#define VMCS_CR4_MASK				0x00006002
#define VMCS_CR0_SHADOW				0x00006004
#define VMCS_CR4_SHADOW				0x00006006
#define VMCS_CR3_TARGET0			0x00006008
#define VMCS_CR3_TARGET1			0x0000600A
#define VMCS_CR3_TARGET2			0x0000600C
#define VMCS_CR3_TARGET3			0x0000600E
/* Natural-Width read-only fields */
#define VMCS_EXIT_QUALIFICATION			0x00006400
#define VMCS_IO_RCX				0x00006402
#define VMCS_IO_RSI				0x00006404
#define VMCS_IO_RDI				0x00006406
#define VMCS_IO_RIP				0x00006408
#define VMCS_GUEST_LINEAR_ADDRESS		0x0000640A
/* Natural-Width guest-state fields */
#define VMCS_GUEST_CR0				0x00006800
#define VMCS_GUEST_CR3				0x00006802
#define VMCS_GUEST_CR4				0x00006804
#define VMCS_GUEST_ES_BASE			0x00006806
#define VMCS_GUEST_CS_BASE			0x00006808
#define VMCS_GUEST_SS_BASE			0x0000680A
#define VMCS_GUEST_DS_BASE			0x0000680C
#define VMCS_GUEST_FS_BASE			0x0000680E
#define VMCS_GUEST_GS_BASE			0x00006810
#define VMCS_GUEST_LDTR_BASE			0x00006812
#define VMCS_GUEST_TR_BASE			0x00006814
#define VMCS_GUEST_GDTR_BASE			0x00006816
#define VMCS_GUEST_IDTR_BASE			0x00006818
#define VMCS_GUEST_DR7				0x0000681A
#define VMCS_GUEST_RSP				0x0000681C
#define VMCS_GUEST_RIP				0x0000681E
#define VMCS_GUEST_RFLAGS			0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS	0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP		0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP		0x00006826
/* Natural-Width host-state fields */
#define VMCS_HOST_CR0				0x00006C00
#define VMCS_HOST_CR3				0x00006C02
#define VMCS_HOST_CR4				0x00006C04
#define VMCS_HOST_FS_BASE			0x00006C06
#define VMCS_HOST_GS_BASE			0x00006C08
#define VMCS_HOST_TR_BASE			0x00006C0A
#define VMCS_HOST_GDTR_BASE			0x00006C0C
#define VMCS_HOST_IDTR_BASE			0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP		0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP		0x00006C12
#define VMCS_HOST_RSP				0x00006C14
#define VMCS_HOST_RIP				0x00006c16

/* VMX basic exit reasons. */
#define VMCS_EXITCODE_EXC_NMI			0
#define VMCS_EXITCODE_EXT_INT			1
#define VMCS_EXITCODE_SHUTDOWN			2
#define VMCS_EXITCODE_INIT			3
#define VMCS_EXITCODE_SIPI			4
#define VMCS_EXITCODE_SMI			5
#define VMCS_EXITCODE_OTHER_SMI			6
#define VMCS_EXITCODE_INT_WINDOW		7
#define VMCS_EXITCODE_NMI_WINDOW		8
#define VMCS_EXITCODE_TASK_SWITCH		9
#define VMCS_EXITCODE_CPUID			10
#define VMCS_EXITCODE_GETSEC			11
#define VMCS_EXITCODE_HLT			12
#define VMCS_EXITCODE_INVD			13
#define VMCS_EXITCODE_INVLPG			14
#define VMCS_EXITCODE_RDPMC			15
#define VMCS_EXITCODE_RDTSC			16
#define VMCS_EXITCODE_RSM			17
#define VMCS_EXITCODE_VMCALL			18
#define VMCS_EXITCODE_VMCLEAR			19
#define VMCS_EXITCODE_VMLAUNCH			20
#define VMCS_EXITCODE_VMPTRLD			21
#define VMCS_EXITCODE_VMPTRST			22
#define VMCS_EXITCODE_VMREAD			23
#define VMCS_EXITCODE_VMRESUME			24
#define VMCS_EXITCODE_VMWRITE			25
#define VMCS_EXITCODE_VMXOFF			26
#define VMCS_EXITCODE_VMXON			27
#define VMCS_EXITCODE_CR			28
#define VMCS_EXITCODE_DR			29
#define VMCS_EXITCODE_IO			30
#define VMCS_EXITCODE_RDMSR			31
#define VMCS_EXITCODE_WRMSR			32
#define VMCS_EXITCODE_FAIL_GUEST_INVALID	33
#define VMCS_EXITCODE_FAIL_MSR_INVALID		34
#define VMCS_EXITCODE_MWAIT			36
#define VMCS_EXITCODE_TRAP_FLAG			37
#define VMCS_EXITCODE_MONITOR			39
#define VMCS_EXITCODE_PAUSE			40
#define VMCS_EXITCODE_FAIL_MACHINE_CHECK	41
#define VMCS_EXITCODE_TPR_BELOW			43
#define VMCS_EXITCODE_APIC_ACCESS		44
#define VMCS_EXITCODE_VEOI			45
#define VMCS_EXITCODE_GDTR_IDTR			46
#define VMCS_EXITCODE_LDTR_TR			47
#define VMCS_EXITCODE_EPT_VIOLATION		48
#define VMCS_EXITCODE_EPT_MISCONFIG		49
#define VMCS_EXITCODE_INVEPT			50
#define VMCS_EXITCODE_RDTSCP			51
#define VMCS_EXITCODE_PREEMPT_TIMEOUT		52
#define VMCS_EXITCODE_INVVPID			53
#define VMCS_EXITCODE_WBINVD			54
#define VMCS_EXITCODE_XSETBV			55
#define VMCS_EXITCODE_APIC_WRITE		56
#define VMCS_EXITCODE_RDRAND			57
#define VMCS_EXITCODE_INVPCID			58
#define VMCS_EXITCODE_VMFUNC			59
#define VMCS_EXITCODE_ENCLS			60
#define VMCS_EXITCODE_RDSEED			61
#define VMCS_EXITCODE_PAGE_LOG_FULL		62
#define VMCS_EXITCODE_XSAVES			63
#define VMCS_EXITCODE_XRSTORS			64

/* -------------------------------------------------------------------------- */

#define VMX_MSRLIST_STAR		0
#define VMX_MSRLIST_LSTAR		1
#define VMX_MSRLIST_CSTAR		2
#define VMX_MSRLIST_SFMASK		3
#define VMX_MSRLIST_KERNELGSBASE	4
#define VMX_MSRLIST_EXIT_NMSR		5
#define VMX_MSRLIST_L1DFLUSH		5

/* On entry, we may do +1 to include L1DFLUSH. */
static size_t vmx_msrlist_entry_nmsr __read_mostly = VMX_MSRLIST_EXIT_NMSR;

struct vmxon {
	uint32_t ident;
#define VMXON_IDENT_REVISION	__BITS(30,0)

	uint8_t data[PAGE_SIZE - 4];
} __packed;

CTASSERT(sizeof(struct vmxon) == PAGE_SIZE);

struct vmxoncpu {
	vaddr_t va;
	paddr_t pa;
};

static struct vmxoncpu vmxoncpu[MAXCPUS];

struct vmcs {
	uint32_t ident;
#define VMCS_IDENT_REVISION	__BITS(30,0)
#define VMCS_IDENT_SHADOW	__BIT(31)

	uint32_t abort;
	uint8_t data[PAGE_SIZE - 8];
} __packed;

CTASSERT(sizeof(struct vmcs) == PAGE_SIZE);

struct msr_entry {
	uint32_t msr;
	uint32_t rsvd;
	uint64_t val;
} __packed;

struct ept_desc {
	uint64_t eptp;
	uint64_t mbz;
} __packed;

struct vpid_desc {
	uint64_t vpid;
	uint64_t addr;
} __packed;

#define VPID_MAX	0xFFFF

/* Make sure we never run out of VPIDs. */
CTASSERT(VPID_MAX-1 >= NVMM_MAX_MACHINES * NVMM_MAX_VCPUS);

static uint64_t vmx_tlb_flush_op __read_mostly;
static uint64_t vmx_ept_flush_op __read_mostly;
static uint64_t vmx_eptp_type __read_mostly;

static uint64_t vmx_pinbased_ctls __read_mostly;
static uint64_t vmx_procbased_ctls __read_mostly;
static uint64_t vmx_procbased_ctls2 __read_mostly;
static uint64_t vmx_entry_ctls __read_mostly;
static uint64_t vmx_exit_ctls __read_mostly;

static uint64_t vmx_cr0_fixed0 __read_mostly;
static uint64_t vmx_cr0_fixed1 __read_mostly;
static uint64_t vmx_cr4_fixed0 __read_mostly;
static uint64_t vmx_cr4_fixed1 __read_mostly;

extern bool pmap_ept_has_ad;

#define VMX_PINBASED_CTLS_ONE	\
	(PIN_CTLS_INT_EXITING| \
	 PIN_CTLS_NMI_EXITING| \
	 PIN_CTLS_VIRTUAL_NMIS)

#define VMX_PINBASED_CTLS_ZERO	0

#define VMX_PROCBASED_CTLS_ONE	\
	(PROC_CTLS_USE_TSC_OFFSETTING| \
	 PROC_CTLS_HLT_EXITING| \
	 PROC_CTLS_MWAIT_EXITING | \
	 PROC_CTLS_RDPMC_EXITING | \
	 PROC_CTLS_RCR8_EXITING | \
	 PROC_CTLS_LCR8_EXITING | \
	 PROC_CTLS_UNCOND_IO_EXITING | /* no I/O bitmap */ \
	 PROC_CTLS_USE_MSR_BITMAPS | \
	 PROC_CTLS_MONITOR_EXITING | \
	 PROC_CTLS_ACTIVATE_CTLS2)

#define VMX_PROCBASED_CTLS_ZERO	\
	(PROC_CTLS_RCR3_EXITING| \
	 PROC_CTLS_LCR3_EXITING)

#define VMX_PROCBASED_CTLS2_ONE	\
	(PROC_CTLS2_ENABLE_EPT| \
	 PROC_CTLS2_ENABLE_VPID| \
	 PROC_CTLS2_UNRESTRICTED_GUEST)

#define VMX_PROCBASED_CTLS2_ZERO	0

#define VMX_ENTRY_CTLS_ONE	\
	(ENTRY_CTLS_LOAD_DEBUG_CONTROLS| \
	 ENTRY_CTLS_LOAD_EFER| \
	 ENTRY_CTLS_LOAD_PAT)

#define VMX_ENTRY_CTLS_ZERO	\
	(ENTRY_CTLS_SMM| \
	 ENTRY_CTLS_DISABLE_DUAL)

#define VMX_EXIT_CTLS_ONE	\
	(EXIT_CTLS_SAVE_DEBUG_CONTROLS| \
	 EXIT_CTLS_HOST_LONG_MODE| \
	 EXIT_CTLS_SAVE_PAT| \
	 EXIT_CTLS_LOAD_PAT| \
	 EXIT_CTLS_SAVE_EFER| \
	 EXIT_CTLS_LOAD_EFER)

#define VMX_EXIT_CTLS_ZERO	0

static uint8_t *vmx_asidmap __read_mostly;
static uint32_t vmx_maxasid __read_mostly;
static kmutex_t vmx_asidlock __cacheline_aligned;

#define VMX_XCR0_MASK_DEFAULT	(XCR0_X87|XCR0_SSE)
static uint64_t vmx_xcr0_mask __read_mostly;

#define VMX_NCPUIDS	32

#define VMCS_NPAGES	1
#define VMCS_SIZE	(VMCS_NPAGES * PAGE_SIZE)

#define MSRBM_NPAGES	1
#define MSRBM_SIZE	(MSRBM_NPAGES * PAGE_SIZE)

#define EFER_TLB_FLUSH \
	(EFER_NXE|EFER_LMA|EFER_LME)
#define CR0_TLB_FLUSH \
	(CR0_PG|CR0_WP|CR0_CD|CR0_NW)
#define CR4_TLB_FLUSH \
	(CR4_PGE|CR4_PAE|CR4_PSE)

/* -------------------------------------------------------------------------- */

struct vmx_machdata {
	bool cpuidpresent[VMX_NCPUIDS];
	struct nvmm_x86_conf_cpuid cpuid[VMX_NCPUIDS];
	volatile uint64_t mach_htlb_gen;
};

static const size_t vmx_conf_sizes[NVMM_X86_NCONF] = {
	[NVMM_X86_CONF_CPUID] = sizeof(struct nvmm_x86_conf_cpuid)
};

struct vmx_cpudata {
	/* General */
	uint64_t asid;
	bool gtlb_want_flush;
	uint64_t vcpu_htlb_gen;
	kcpuset_t *htlb_want_flush;

	/* VMCS */
	struct vmcs *vmcs;
	paddr_t vmcs_pa;
	size_t vmcs_refcnt;

	/* MSR bitmap */
	uint8_t *msrbm;
	paddr_t msrbm_pa;

	/* Host state */
	uint64_t hxcr0;
	uint64_t star;
	uint64_t lstar;
	uint64_t cstar;
	uint64_t sfmask;
	uint64_t kernelgsbase;
	bool ts_set;
	struct xsave_header hfpu __aligned(64);

	/* Event state */
	bool int_window_exit;
	bool nmi_window_exit;

	/* Guest state */
	struct msr_entry *gmsr;
	paddr_t gmsr_pa;
	uint64_t gmsr_misc_enable;
	uint64_t gcr2;
	uint64_t gcr8;
	uint64_t gxcr0;
	uint64_t gprs[NVMM_X64_NGPR];
	uint64_t drs[NVMM_X64_NDR];
	uint64_t tsc_offset;
	struct xsave_header gfpu __aligned(64);
};

static const struct {
	uint64_t selector;
	uint64_t attrib;
	uint64_t limit;
	uint64_t base;
} vmx_guest_segs[NVMM_X64_NSEG] = {
	[NVMM_X64_SEG_ES] = {
		VMCS_GUEST_ES_SELECTOR,
		VMCS_GUEST_ES_ACCESS_RIGHTS,
		VMCS_GUEST_ES_LIMIT,
		VMCS_GUEST_ES_BASE
	},
	[NVMM_X64_SEG_CS] = {
		VMCS_GUEST_CS_SELECTOR,
		VMCS_GUEST_CS_ACCESS_RIGHTS,
		VMCS_GUEST_CS_LIMIT,
		VMCS_GUEST_CS_BASE
	},
	[NVMM_X64_SEG_SS] = {
		VMCS_GUEST_SS_SELECTOR,
		VMCS_GUEST_SS_ACCESS_RIGHTS,
		VMCS_GUEST_SS_LIMIT,
		VMCS_GUEST_SS_BASE
	},
	[NVMM_X64_SEG_DS] = {
		VMCS_GUEST_DS_SELECTOR,
		VMCS_GUEST_DS_ACCESS_RIGHTS,
		VMCS_GUEST_DS_LIMIT,
		VMCS_GUEST_DS_BASE
	},
	[NVMM_X64_SEG_FS] = {
		VMCS_GUEST_FS_SELECTOR,
		VMCS_GUEST_FS_ACCESS_RIGHTS,
		VMCS_GUEST_FS_LIMIT,
		VMCS_GUEST_FS_BASE
	},
	[NVMM_X64_SEG_GS] = {
		VMCS_GUEST_GS_SELECTOR,
		VMCS_GUEST_GS_ACCESS_RIGHTS,
		VMCS_GUEST_GS_LIMIT,
		VMCS_GUEST_GS_BASE
	},
	[NVMM_X64_SEG_GDT] = {
		0, /* doesn't exist */
		0, /* doesn't exist */
		VMCS_GUEST_GDTR_LIMIT,
		VMCS_GUEST_GDTR_BASE
	},
	[NVMM_X64_SEG_IDT] = {
		0, /* doesn't exist */
		0, /* doesn't exist */
		VMCS_GUEST_IDTR_LIMIT,
		VMCS_GUEST_IDTR_BASE
	},
	[NVMM_X64_SEG_LDT] = {
		VMCS_GUEST_LDTR_SELECTOR,
		VMCS_GUEST_LDTR_ACCESS_RIGHTS,
		VMCS_GUEST_LDTR_LIMIT,
		VMCS_GUEST_LDTR_BASE
	},
	[NVMM_X64_SEG_TR] = {
		VMCS_GUEST_TR_SELECTOR,
		VMCS_GUEST_TR_ACCESS_RIGHTS,
		VMCS_GUEST_TR_LIMIT,
		VMCS_GUEST_TR_BASE
	}
};

/* -------------------------------------------------------------------------- */

static uint64_t
vmx_get_revision(void)
{
	uint64_t msr;

	msr = rdmsr(MSR_IA32_VMX_BASIC);
	msr &= IA32_VMX_BASIC_IDENT;

	return msr;
}

static void
vmx_vmcs_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	paddr_t oldpa __diagused;

	cpudata->vmcs_refcnt++;
	if (cpudata->vmcs_refcnt > 1) {
#ifdef DIAGNOSTIC
		KASSERT(kpreempt_disabled());
		vmx_vmptrst(&oldpa);
		KASSERT(oldpa == cpudata->vmcs_pa);
#endif
		return;
	}

	kpreempt_disable();

#ifdef DIAGNOSTIC
	vmx_vmptrst(&oldpa);
	KASSERT(oldpa == 0xFFFFFFFFFFFFFFFF);
#endif

	vmx_vmptrld(&cpudata->vmcs_pa);
}

static void
vmx_vmcs_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	paddr_t oldpa __diagused;

	KASSERT(kpreempt_disabled());
	KASSERT(cpudata->vmcs_refcnt > 0);
	cpudata->vmcs_refcnt--;

	if (cpudata->vmcs_refcnt > 0) {
#ifdef DIAGNOSTIC
		vmx_vmptrst(&oldpa);
		KASSERT(oldpa == cpudata->vmcs_pa);
#endif
		return;
	}

	vmx_vmclear(&cpudata->vmcs_pa);
	kpreempt_enable();
}

/* -------------------------------------------------------------------------- */

static void
vmx_event_waitexit_enable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t ctls1;

	vmx_vmread(VMCS_PROCBASED_CTLS, &ctls1);

	if (nmi) {
		// XXX INT_STATE_NMI?
		ctls1 |= PROC_CTLS_NMI_WINDOW_EXITING;
		cpudata->nmi_window_exit = true;
	} else {
		ctls1 |= PROC_CTLS_INT_WINDOW_EXITING;
		cpudata->int_window_exit = true;
	}

	vmx_vmwrite(VMCS_PROCBASED_CTLS, ctls1);
}

static void
vmx_event_waitexit_disable(struct nvmm_cpu *vcpu, bool nmi)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t ctls1;

	vmx_vmread(VMCS_PROCBASED_CTLS, &ctls1);

	if (nmi) {
		ctls1 &= ~PROC_CTLS_NMI_WINDOW_EXITING;
		cpudata->nmi_window_exit = false;
	} else {
		ctls1 &= ~PROC_CTLS_INT_WINDOW_EXITING;
		cpudata->int_window_exit = false;
	}

	vmx_vmwrite(VMCS_PROCBASED_CTLS, ctls1);
}

static inline int
vmx_event_has_error(uint64_t vector)
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
vmx_vcpu_inject(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_event *event)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	int type = 0, err = 0, ret = 0;
	uint64_t info, intstate, rflags;

	if (event->vector >= 256) {
		return EINVAL;
	}

	vmx_vmcs_enter(vcpu);

	switch (event->type) {
	case NVMM_EVENT_INTERRUPT_HW:
		type = INTR_INFO_TYPE_EXT_INT;
		if (event->vector == 2) {
			type = INTR_INFO_TYPE_NMI;
		}
		vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY, &intstate);
		if (type == INTR_INFO_TYPE_NMI) {
			if (cpudata->nmi_window_exit) {
				ret = EAGAIN;
				goto out;
			}
			vmx_event_waitexit_enable(vcpu, true);
		} else {
			vmx_vmread(VMCS_GUEST_RFLAGS, &rflags);
			if ((rflags & PSL_I) == 0 ||
			    (intstate & (INT_STATE_STI|INT_STATE_MOVSS)) != 0) {
				vmx_event_waitexit_enable(vcpu, false);
				ret = EAGAIN;
				goto out;
			}
		}
		err = 0;
		break;
	case NVMM_EVENT_INTERRUPT_SW:
		ret = EINVAL;
		goto out;
	case NVMM_EVENT_EXCEPTION:
		if (event->vector == 2 || event->vector >= 32) {
			ret = EINVAL;
			goto out;
		}
		if (event->vector == 3 || event->vector == 0) {
			ret = EINVAL;
			goto out;
		}
		type = INTR_INFO_TYPE_HW_EXC;
		err = vmx_event_has_error(event->vector);
		break;
	default:
		ret = EAGAIN;
		goto out;
	}

	info =
	    __SHIFTIN(event->vector, INTR_INFO_VECTOR) |
	    type |
	    __SHIFTIN(err, INTR_INFO_ERROR) |
	    __SHIFTIN(1, INTR_INFO_VALID);
	vmx_vmwrite(VMCS_ENTRY_INTR_INFO, info);
	vmx_vmwrite(VMCS_ENTRY_EXCEPTION_ERROR, event->u.error);

out:
	vmx_vmcs_leave(vcpu);
	return ret;
}

static void
vmx_inject_ud(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct nvmm_event event;
	int ret __diagused;

	event.type = NVMM_EVENT_EXCEPTION;
	event.vector = 6;
	event.u.error = 0;

	ret = vmx_vcpu_inject(mach, vcpu, &event);
	KASSERT(ret == 0);
}

static void
vmx_inject_gp(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct nvmm_event event;
	int ret __diagused;

	event.type = NVMM_EVENT_EXCEPTION;
	event.vector = 13;
	event.u.error = 0;

	ret = vmx_vcpu_inject(mach, vcpu, &event);
	KASSERT(ret == 0);
}

static inline void
vmx_inkernel_advance(void)
{
	uint64_t rip, inslen, intstate;

	/*
	 * Maybe we should also apply single-stepping and debug exceptions.
	 * Matters for guest-ring3, because it can execute 'cpuid' under a
	 * debugger.
	 */
	vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH, &inslen);
	vmx_vmread(VMCS_GUEST_RIP, &rip);
	vmx_vmwrite(VMCS_GUEST_RIP, rip + inslen);
	vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY, &intstate);
	vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY,
	    intstate & ~(INT_STATE_STI|INT_STATE_MOVSS));
}

static void
vmx_inkernel_handle_cpuid(struct nvmm_cpu *vcpu, uint64_t eax, uint64_t ecx)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t cr4;

	switch (eax) {
	case 0x00000001:
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~CPUID_LOCAL_APIC_ID;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= __SHIFTIN(vcpu->cpuid,
		    CPUID_LOCAL_APIC_ID);
		cpudata->gprs[NVMM_X64_GPR_RCX] &=
		    ~(CPUID2_VMX|CPUID2_SMX|CPUID2_EST|CPUID2_TM2|CPUID2_PDCM|
		      CPUID2_PCID|CPUID2_DEADLINE);
		cpudata->gprs[NVMM_X64_GPR_RDX] &=
		    ~(CPUID_DS|CPUID_ACPI|CPUID_TM);

		/* CPUID2_OSXSAVE depends on CR4. */
		vmx_vmread(VMCS_GUEST_CR4, &cr4);
		if (!(cr4 & CR4_OSXSAVE)) {
			cpudata->gprs[NVMM_X64_GPR_RCX] &= ~CPUID2_OSXSAVE;
		}
		break;
	case 0x00000005:
	case 0x00000006:
		cpudata->gprs[NVMM_X64_GPR_RAX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RBX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RCX] = 0;
		cpudata->gprs[NVMM_X64_GPR_RDX] = 0;
		break;
	case 0x00000007:
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~CPUID_SEF_INVPCID;
		cpudata->gprs[NVMM_X64_GPR_RDX] &=
		    ~(CPUID_SEF_IBRS|CPUID_SEF_STIBP|CPUID_SEF_L1D_FLUSH|
		      CPUID_SEF_SSBD);
		break;
	case 0x0000000D:
		if (vmx_xcr0_mask == 0) {
			break;
		}
		switch (ecx) {
		case 0:
			cpudata->gprs[NVMM_X64_GPR_RAX] = vmx_xcr0_mask & 0xFFFFFFFF;
			if (cpudata->gxcr0 & XCR0_SSE) {
				cpudata->gprs[NVMM_X64_GPR_RBX] = sizeof(struct fxsave);
			} else {
				cpudata->gprs[NVMM_X64_GPR_RBX] = sizeof(struct save87);
			}
			cpudata->gprs[NVMM_X64_GPR_RBX] += 64; /* XSAVE header */
			cpudata->gprs[NVMM_X64_GPR_RCX] = sizeof(struct fxsave);
			cpudata->gprs[NVMM_X64_GPR_RDX] = vmx_xcr0_mask >> 32;
			break;
		case 1:
			cpudata->gprs[NVMM_X64_GPR_RAX] &= ~CPUID_PES1_XSAVES;
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
		cpudata->gprs[NVMM_X64_GPR_RDX] &= ~CPUID_RDTSCP;
		break;
	default:
		break;
	}
}

static void
vmx_exit_cpuid(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmx_machdata *machdata = mach->machdata;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct nvmm_x86_conf_cpuid *cpuid;
	uint64_t eax, ecx;
	u_int descs[4];
	size_t i;

	eax = cpudata->gprs[NVMM_X64_GPR_RAX];
	ecx = cpudata->gprs[NVMM_X64_GPR_RCX];
	x86_cpuid2(eax, ecx, descs);

	cpudata->gprs[NVMM_X64_GPR_RAX] = descs[0];
	cpudata->gprs[NVMM_X64_GPR_RBX] = descs[1];
	cpudata->gprs[NVMM_X64_GPR_RCX] = descs[2];
	cpudata->gprs[NVMM_X64_GPR_RDX] = descs[3];

	for (i = 0; i < VMX_NCPUIDS; i++) {
		cpuid = &machdata->cpuid[i];
		if (!machdata->cpuidpresent[i]) {
			continue;
		}
		if (cpuid->leaf != eax) {
			continue;
		}

		/* del */
		cpudata->gprs[NVMM_X64_GPR_RAX] &= ~cpuid->del.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] &= ~cpuid->del.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] &= ~cpuid->del.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] &= ~cpuid->del.edx;

		/* set */
		cpudata->gprs[NVMM_X64_GPR_RAX] |= cpuid->set.eax;
		cpudata->gprs[NVMM_X64_GPR_RBX] |= cpuid->set.ebx;
		cpudata->gprs[NVMM_X64_GPR_RCX] |= cpuid->set.ecx;
		cpudata->gprs[NVMM_X64_GPR_RDX] |= cpuid->set.edx;

		break;
	}

	/* Overwrite non-tunable leaves. */
	vmx_inkernel_handle_cpuid(vcpu, eax, ecx);

	vmx_inkernel_advance();
	exit->reason = NVMM_EXIT_NONE;
}

static void
vmx_exit_hlt(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t rflags;

	if (cpudata->int_window_exit) {
		vmx_vmread(VMCS_GUEST_RFLAGS, &rflags);
		if (rflags & PSL_I) {
			vmx_event_waitexit_disable(vcpu, false);
		}
	}

	vmx_inkernel_advance();
	exit->reason = NVMM_EXIT_HALTED;
}

#define VMX_QUAL_CR_NUM		__BITS(3,0)
#define VMX_QUAL_CR_TYPE	__BITS(5,4)
#define		CR_TYPE_WRITE	0
#define		CR_TYPE_READ	1
#define		CR_TYPE_CLTS	2
#define		CR_TYPE_LMSW	3
#define VMX_QUAL_CR_LMSW_OPMEM	__BIT(6)
#define VMX_QUAL_CR_GPR		__BITS(11,8)
#define VMX_QUAL_CR_LMSW_SRC	__BIT(31,16)

static inline int
vmx_check_cr(uint64_t crval, uint64_t fixed0, uint64_t fixed1)
{
	/* Bits set to 1 in fixed0 are fixed to 1. */
	if ((crval & fixed0) != fixed0) {
		return -1;
	}
	/* Bits set to 0 in fixed1 are fixed to 0. */
	if (crval & ~fixed1) {
		return -1;
	}
	return 0;
}

static int
vmx_inkernel_handle_cr0(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr, cr0;
	uint64_t efer, ctls1;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type != CR_TYPE_WRITE) {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	KASSERT(gpr < 16);

	if (gpr == NVMM_X64_GPR_RSP) {
		vmx_vmread(VMCS_GUEST_RSP, &gpr);
	} else {
		gpr = cpudata->gprs[gpr];
	}

	cr0 = gpr | CR0_NE | CR0_ET;
	cr0 &= ~(CR0_NW|CR0_CD);

	if (vmx_check_cr(cr0, vmx_cr0_fixed0, vmx_cr0_fixed1) == -1) {
		return -1;
	}

	/*
	 * XXX Handle 32bit PAE paging, need to set PDPTEs, fetched manually
	 * from CR3.
	 */

	if (cr0 & CR0_PG) {
		vmx_vmread(VMCS_ENTRY_CTLS, &ctls1);
		vmx_vmread(VMCS_GUEST_IA32_EFER, &efer);
		if (efer & EFER_LME) {
			ctls1 |= ENTRY_CTLS_LONG_MODE;
			efer |= EFER_LMA;
		} else {
			ctls1 &= ~ENTRY_CTLS_LONG_MODE;
			efer &= ~EFER_LMA;
		}
		vmx_vmwrite(VMCS_GUEST_IA32_EFER, efer);
		vmx_vmwrite(VMCS_ENTRY_CTLS, ctls1);
	}

	vmx_vmwrite(VMCS_GUEST_CR0, cr0);
	vmx_inkernel_advance();
	return 0;
}

static int
vmx_inkernel_handle_cr4(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr, cr4;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type != CR_TYPE_WRITE) {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	KASSERT(gpr < 16);

	if (gpr == NVMM_X64_GPR_RSP) {
		vmx_vmread(VMCS_GUEST_RSP, &gpr);
	} else {
		gpr = cpudata->gprs[gpr];
	}

	cr4 = gpr | CR4_VMXE;

	if (vmx_check_cr(cr4, vmx_cr4_fixed0, vmx_cr4_fixed1) == -1) {
		return -1;
	}

	vmx_vmwrite(VMCS_GUEST_CR4, cr4);
	vmx_inkernel_advance();
	return 0;
}

static int
vmx_inkernel_handle_cr8(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    uint64_t qual)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t type, gpr;
	bool write;

	type = __SHIFTOUT(qual, VMX_QUAL_CR_TYPE);
	if (type == CR_TYPE_WRITE) {
		write = true;
	} else if (type == CR_TYPE_READ) {
		write = false;
	} else {
		return -1;
	}

	gpr = __SHIFTOUT(qual, VMX_QUAL_CR_GPR);
	KASSERT(gpr < 16);

	if (write) {
		if (gpr == NVMM_X64_GPR_RSP) {
			vmx_vmread(VMCS_GUEST_RSP, &cpudata->gcr8);
		} else {
			cpudata->gcr8 = cpudata->gprs[gpr];
		}
	} else {
		if (gpr == NVMM_X64_GPR_RSP) {
			vmx_vmwrite(VMCS_GUEST_RSP, cpudata->gcr8);
		} else {
			cpudata->gprs[gpr] = cpudata->gcr8;
		}
	}

	vmx_inkernel_advance();
	return 0;
}

static void
vmx_exit_cr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	uint64_t qual;
	int ret;

	vmx_vmread(VMCS_EXIT_QUALIFICATION, &qual);

	switch (__SHIFTOUT(qual, VMX_QUAL_CR_NUM)) {
	case 0:
		ret = vmx_inkernel_handle_cr0(mach, vcpu, qual);
		break;
	case 4:
		ret = vmx_inkernel_handle_cr4(mach, vcpu, qual);
		break;
	case 8:
		ret = vmx_inkernel_handle_cr8(mach, vcpu, qual);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret == -1) {
		vmx_inject_gp(mach, vcpu);
	}

	exit->reason = NVMM_EXIT_NONE;
}

#define VMX_QUAL_IO_SIZE	__BITS(2,0)
#define		IO_SIZE_8	0
#define		IO_SIZE_16	1
#define		IO_SIZE_32	3
#define VMX_QUAL_IO_IN		__BIT(3)
#define VMX_QUAL_IO_STR		__BIT(4)
#define VMX_QUAL_IO_REP		__BIT(5)
#define VMX_QUAL_IO_DX		__BIT(6)
#define VMX_QUAL_IO_PORT	__BITS(31,16)

#define VMX_INFO_IO_ADRSIZE	__BITS(9,7)
#define		IO_ADRSIZE_16	0
#define		IO_ADRSIZE_32	1
#define		IO_ADRSIZE_64	2
#define VMX_INFO_IO_SEG		__BITS(17,15)

static const int seg_to_nvmm[] = {
	[0] = NVMM_X64_SEG_ES,
	[1] = NVMM_X64_SEG_CS,
	[2] = NVMM_X64_SEG_SS,
	[3] = NVMM_X64_SEG_DS,
	[4] = NVMM_X64_SEG_FS,
	[5] = NVMM_X64_SEG_GS
};

static void
vmx_exit_io(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	uint64_t qual, info, inslen, rip;

	vmx_vmread(VMCS_EXIT_QUALIFICATION, &qual);
	vmx_vmread(VMCS_EXIT_INSTRUCTION_INFO, &info);

	exit->reason = NVMM_EXIT_IO;

	if (qual & VMX_QUAL_IO_IN) {
		exit->u.io.type = NVMM_EXIT_IO_IN;
	} else {
		exit->u.io.type = NVMM_EXIT_IO_OUT;
	}

	exit->u.io.port = __SHIFTOUT(qual, VMX_QUAL_IO_PORT);

	KASSERT(__SHIFTOUT(info, VMX_INFO_IO_SEG) < 6);
	exit->u.io.seg = seg_to_nvmm[__SHIFTOUT(info, VMX_INFO_IO_SEG)];

	if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_64) {
		exit->u.io.address_size = 8;
	} else if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_32) {
		exit->u.io.address_size = 4;
	} else if (__SHIFTOUT(info, VMX_INFO_IO_ADRSIZE) == IO_ADRSIZE_16) {
		exit->u.io.address_size = 2;
	}

	if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_32) {
		exit->u.io.operand_size = 4;
	} else if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_16) {
		exit->u.io.operand_size = 2;
	} else if (__SHIFTOUT(qual, VMX_QUAL_IO_SIZE) == IO_SIZE_8) {
		exit->u.io.operand_size = 1;
	}

	exit->u.io.rep = (qual & VMX_QUAL_IO_REP) != 0;
	exit->u.io.str = (qual & VMX_QUAL_IO_STR) != 0;

	if ((exit->u.io.type == NVMM_EXIT_IO_IN) && exit->u.io.str) {
		exit->u.io.seg = NVMM_X64_SEG_ES;
	}

	vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH, &inslen);
	vmx_vmread(VMCS_GUEST_RIP, &rip);
	exit->u.io.npc = rip + inslen;
}

static const uint64_t msr_ignore_list[] = {
	MSR_BIOS_SIGN,
	MSR_IA32_PLATFORM_ID
};

static bool
vmx_inkernel_handle_msr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t val;
	size_t i;

	switch (exit->u.msr.type) {
	case NVMM_EXIT_MSR_RDMSR:
		if (exit->u.msr.msr == MSR_CR_PAT) {
			vmx_vmread(VMCS_GUEST_IA32_PAT, &val);
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		if (exit->u.msr.msr == MSR_MISC_ENABLE) {
			val = cpudata->gmsr_misc_enable;
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		for (i = 0; i < __arraycount(msr_ignore_list); i++) {
			if (msr_ignore_list[i] != exit->u.msr.msr)
				continue;
			val = 0;
			cpudata->gprs[NVMM_X64_GPR_RAX] = (val & 0xFFFFFFFF);
			cpudata->gprs[NVMM_X64_GPR_RDX] = (val >> 32);
			goto handled;
		}
		break;
	case NVMM_EXIT_MSR_WRMSR:
		if (exit->u.msr.msr == MSR_TSC) {
			cpudata->tsc_offset = exit->u.msr.val - cpu_counter();
			vmx_vmwrite(VMCS_TSC_OFFSET, cpudata->tsc_offset +
			    curcpu()->ci_data.cpu_cc_skew);
			goto handled;
		}
		if (exit->u.msr.msr == MSR_CR_PAT) {
			vmx_vmwrite(VMCS_GUEST_IA32_PAT, exit->u.msr.val);
			goto handled;
		}
		if (exit->u.msr.msr == MSR_MISC_ENABLE) {
			/* Don't care. */
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
	vmx_inkernel_advance();
	return true;
}

static void
vmx_exit_msr(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit, bool rdmsr)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t inslen, rip;

	if (rdmsr) {
		exit->u.msr.type = NVMM_EXIT_MSR_RDMSR;
	} else {
		exit->u.msr.type = NVMM_EXIT_MSR_WRMSR;
	}

	exit->u.msr.msr = (cpudata->gprs[NVMM_X64_GPR_RCX] & 0xFFFFFFFF);

	if (rdmsr) {
		exit->u.msr.val = 0;
	} else {
		uint64_t rdx, rax;
		rdx = cpudata->gprs[NVMM_X64_GPR_RDX];
		rax = cpudata->gprs[NVMM_X64_GPR_RAX];
		exit->u.msr.val = (rdx << 32) | (rax & 0xFFFFFFFF);
	}

	if (vmx_inkernel_handle_msr(mach, vcpu, exit)) {
		exit->reason = NVMM_EXIT_NONE;
		return;
	}

	exit->reason = NVMM_EXIT_MSR;
	vmx_vmread(VMCS_EXIT_INSTRUCTION_LENGTH, &inslen);
	vmx_vmread(VMCS_GUEST_RIP, &rip);
	exit->u.msr.npc = rip + inslen;
}

static void
vmx_exit_xsetbv(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint16_t val;

	exit->reason = NVMM_EXIT_NONE;

	val = (cpudata->gprs[NVMM_X64_GPR_RDX] << 32) |
	    (cpudata->gprs[NVMM_X64_GPR_RAX] & 0xFFFFFFFF);

	if (__predict_false(cpudata->gprs[NVMM_X64_GPR_RCX] != 0)) {
		goto error;
	} else if (__predict_false((val & ~vmx_xcr0_mask) != 0)) {
		goto error;
	} else if (__predict_false((val & XCR0_X87) == 0)) {
		goto error;
	}

	cpudata->gxcr0 = val;

	vmx_inkernel_advance();
	return;

error:
	vmx_inject_gp(mach, vcpu);
}

#define VMX_EPT_VIOLATION_READ		__BIT(0)
#define VMX_EPT_VIOLATION_WRITE		__BIT(1)
#define VMX_EPT_VIOLATION_EXECUTE	__BIT(2)

static void
vmx_exit_epf(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	uint64_t perm;
	gpaddr_t gpa;

	vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &gpa);

	exit->reason = NVMM_EXIT_MEMORY;
	vmx_vmread(VMCS_EXIT_QUALIFICATION, &perm);
	if (perm & VMX_EPT_VIOLATION_WRITE)
		exit->u.mem.perm = NVMM_EXIT_MEMORY_WRITE;
	else if (perm & VMX_EPT_VIOLATION_EXECUTE)
		exit->u.mem.perm = NVMM_EXIT_MEMORY_EXEC;
	else
		exit->u.mem.perm = NVMM_EXIT_MEMORY_READ;
	exit->u.mem.gpa = gpa;
	exit->u.mem.inst_len = 0;
}

/* -------------------------------------------------------------------------- */

static void
vmx_vcpu_guest_fpu_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	cpudata->ts_set = (rcr0() & CR0_TS) != 0;

	fpu_area_save(&cpudata->hfpu, vmx_xcr0_mask);
	fpu_area_restore(&cpudata->gfpu, vmx_xcr0_mask);

	if (vmx_xcr0_mask != 0) {
		cpudata->hxcr0 = rdxcr(0);
		wrxcr(0, cpudata->gxcr0);
	}
}

static void
vmx_vcpu_guest_fpu_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	if (vmx_xcr0_mask != 0) {
		cpudata->gxcr0 = rdxcr(0);
		wrxcr(0, cpudata->hxcr0);
	}

	fpu_area_save(&cpudata->gfpu, vmx_xcr0_mask);
	fpu_area_restore(&cpudata->hfpu, vmx_xcr0_mask);

	if (cpudata->ts_set) {
		stts();
	}
}

static void
vmx_vcpu_guest_dbregs_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	x86_dbregs_save(curlwp);

	ldr7(0);

	ldr0(cpudata->drs[NVMM_X64_DR_DR0]);
	ldr1(cpudata->drs[NVMM_X64_DR_DR1]);
	ldr2(cpudata->drs[NVMM_X64_DR_DR2]);
	ldr3(cpudata->drs[NVMM_X64_DR_DR3]);
	ldr6(cpudata->drs[NVMM_X64_DR_DR6]);
}

static void
vmx_vcpu_guest_dbregs_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	cpudata->drs[NVMM_X64_DR_DR0] = rdr0();
	cpudata->drs[NVMM_X64_DR_DR1] = rdr1();
	cpudata->drs[NVMM_X64_DR_DR2] = rdr2();
	cpudata->drs[NVMM_X64_DR_DR3] = rdr3();
	cpudata->drs[NVMM_X64_DR_DR6] = rdr6();

	x86_dbregs_restore(curlwp);
}

static void
vmx_vcpu_guest_misc_enter(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	/* This gets restored automatically by the CPU. */
	vmx_vmwrite(VMCS_HOST_FS_BASE, rdmsr(MSR_FSBASE));
	vmx_vmwrite(VMCS_HOST_CR3, rcr3());
	vmx_vmwrite(VMCS_HOST_CR4, rcr4());

	/* Note: MSR_LSTAR is not static, because of SVS. */
	cpudata->lstar = rdmsr(MSR_LSTAR);
	cpudata->kernelgsbase = rdmsr(MSR_KERNELGSBASE);
}

static void
vmx_vcpu_guest_misc_leave(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	wrmsr(MSR_STAR, cpudata->star);
	wrmsr(MSR_LSTAR, cpudata->lstar);
	wrmsr(MSR_CSTAR, cpudata->cstar);
	wrmsr(MSR_SFMASK, cpudata->sfmask);
	wrmsr(MSR_KERNELGSBASE, cpudata->kernelgsbase);
}

/* -------------------------------------------------------------------------- */

#define VMX_INVVPID_ADDRESS		0
#define VMX_INVVPID_CONTEXT		1
#define VMX_INVVPID_ALL			2
#define VMX_INVVPID_CONTEXT_NOGLOBAL	3

#define VMX_INVEPT_CONTEXT		1
#define VMX_INVEPT_ALL			2

static inline void
vmx_gtlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	if (vcpu->hcpu_last != hcpu) {
		cpudata->gtlb_want_flush = true;
	}
}

static inline void
vmx_htlb_catchup(struct nvmm_cpu *vcpu, int hcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct ept_desc ept_desc;

	if (__predict_true(!kcpuset_isset(cpudata->htlb_want_flush, hcpu))) {
		return;
	}

	vmx_vmread(VMCS_EPTP, &ept_desc.eptp);
	ept_desc.mbz = 0;
	vmx_invept(vmx_ept_flush_op, &ept_desc);
	kcpuset_clear(cpudata->htlb_want_flush, hcpu);
}

static inline uint64_t
vmx_htlb_flush(struct vmx_machdata *machdata, struct vmx_cpudata *cpudata)
{
	struct ept_desc ept_desc;
	uint64_t machgen;

	machgen = machdata->mach_htlb_gen;
	if (__predict_true(machgen == cpudata->vcpu_htlb_gen)) {
		return machgen;
	}

	kcpuset_copy(cpudata->htlb_want_flush, kcpuset_running);

	vmx_vmread(VMCS_EPTP, &ept_desc.eptp);
	ept_desc.mbz = 0;
	vmx_invept(vmx_ept_flush_op, &ept_desc);

	return machgen;
}

static inline void
vmx_htlb_flush_ack(struct vmx_cpudata *cpudata, uint64_t machgen)
{
	cpudata->vcpu_htlb_gen = machgen;
	kcpuset_clear(cpudata->htlb_want_flush, cpu_number());
}

static int
vmx_vcpu_run(struct nvmm_machine *mach, struct nvmm_cpu *vcpu,
    struct nvmm_exit *exit)
{
	struct vmx_machdata *machdata = mach->machdata;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct vpid_desc vpid_desc;
	struct cpu_info *ci;
	uint64_t exitcode;
	uint64_t intstate;
	uint64_t machgen;
	int hcpu, s, ret;
	bool launched = false;

	vmx_vmcs_enter(vcpu);
	ci = curcpu();
	hcpu = cpu_number();

	vmx_gtlb_catchup(vcpu, hcpu);
	vmx_htlb_catchup(vcpu, hcpu);

	if (vcpu->hcpu_last != hcpu) {
		vmx_vmwrite(VMCS_HOST_TR_SELECTOR, ci->ci_tss_sel);
		vmx_vmwrite(VMCS_HOST_TR_BASE, (uint64_t)ci->ci_tss);
		vmx_vmwrite(VMCS_HOST_GDTR_BASE, (uint64_t)ci->ci_gdt);
		vmx_vmwrite(VMCS_HOST_GS_BASE, rdmsr(MSR_GSBASE));
		vmx_vmwrite(VMCS_TSC_OFFSET, cpudata->tsc_offset +
		    curcpu()->ci_data.cpu_cc_skew);
		vcpu->hcpu_last = hcpu;
	}

	vmx_vcpu_guest_dbregs_enter(vcpu);
	vmx_vcpu_guest_misc_enter(vcpu);

	while (1) {
		if (cpudata->gtlb_want_flush) {
			vpid_desc.vpid = cpudata->asid;
			vpid_desc.addr = 0;
			vmx_invvpid(vmx_tlb_flush_op, &vpid_desc);
			cpudata->gtlb_want_flush = false;
		}

		s = splhigh();
		machgen = vmx_htlb_flush(machdata, cpudata);
		vmx_vcpu_guest_fpu_enter(vcpu);
		lcr2(cpudata->gcr2);
		if (launched) {
			ret = vmx_vmresume(cpudata->gprs);
		} else {
			ret = vmx_vmlaunch(cpudata->gprs);
		}
		cpudata->gcr2 = rcr2();
		vmx_vcpu_guest_fpu_leave(vcpu);
		vmx_htlb_flush_ack(cpudata, machgen);
		splx(s);

		if (__predict_false(ret != 0)) {
			exit->reason = NVMM_EXIT_INVALID;
			break;
		}

		launched = true;

		vmx_vmread(VMCS_EXIT_REASON, &exitcode);
		exitcode &= __BITS(15,0);

		switch (exitcode) {
		case VMCS_EXITCODE_EXT_INT:
			exit->reason = NVMM_EXIT_NONE;
			break;
		case VMCS_EXITCODE_CPUID:
			vmx_exit_cpuid(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_HLT:
			vmx_exit_hlt(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_CR:
			vmx_exit_cr(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_IO:
			vmx_exit_io(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_RDMSR:
			vmx_exit_msr(mach, vcpu, exit, true);
			break;
		case VMCS_EXITCODE_WRMSR:
			vmx_exit_msr(mach, vcpu, exit, false);
			break;
		case VMCS_EXITCODE_SHUTDOWN:
			exit->reason = NVMM_EXIT_SHUTDOWN;
			break;
		case VMCS_EXITCODE_MONITOR:
			exit->reason = NVMM_EXIT_MONITOR;
			break;
		case VMCS_EXITCODE_MWAIT:
			exit->reason = NVMM_EXIT_MWAIT;
			break;
		case VMCS_EXITCODE_XSETBV:
			vmx_exit_xsetbv(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_RDPMC:
		case VMCS_EXITCODE_RDTSCP:
		case VMCS_EXITCODE_INVVPID:
		case VMCS_EXITCODE_INVEPT:
		case VMCS_EXITCODE_VMCALL:
		case VMCS_EXITCODE_VMCLEAR:
		case VMCS_EXITCODE_VMLAUNCH:
		case VMCS_EXITCODE_VMPTRLD:
		case VMCS_EXITCODE_VMPTRST:
		case VMCS_EXITCODE_VMREAD:
		case VMCS_EXITCODE_VMRESUME:
		case VMCS_EXITCODE_VMWRITE:
		case VMCS_EXITCODE_VMXOFF:
		case VMCS_EXITCODE_VMXON:
			vmx_inject_ud(mach, vcpu);
			exit->reason = NVMM_EXIT_NONE;
			break;
		case VMCS_EXITCODE_EPT_VIOLATION:
			vmx_exit_epf(mach, vcpu, exit);
			break;
		case VMCS_EXITCODE_INT_WINDOW:
			vmx_event_waitexit_disable(vcpu, false);
			exit->reason = NVMM_EXIT_INT_READY;
			break;
		case VMCS_EXITCODE_NMI_WINDOW:
			vmx_event_waitexit_disable(vcpu, true);
			exit->reason = NVMM_EXIT_NMI_READY;
			break;
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

	vmx_vcpu_guest_misc_leave(vcpu);
	vmx_vcpu_guest_dbregs_leave(vcpu);

	exit->exitstate[NVMM_X64_EXITSTATE_CR8] = cpudata->gcr8;
	vmx_vmread(VMCS_GUEST_RFLAGS,
	    &exit->exitstate[NVMM_X64_EXITSTATE_RFLAGS]);
	vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY, &intstate);
	exit->exitstate[NVMM_X64_EXITSTATE_INT_SHADOW] =
	    (intstate & (INT_STATE_STI|INT_STATE_MOVSS)) != 0;
	exit->exitstate[NVMM_X64_EXITSTATE_INT_WINDOW_EXIT] =
	    cpudata->int_window_exit;
	exit->exitstate[NVMM_X64_EXITSTATE_NMI_WINDOW_EXIT] =
	    cpudata->nmi_window_exit;

	vmx_vmcs_leave(vcpu);

	return 0;
}

/* -------------------------------------------------------------------------- */

static int
vmx_memalloc(paddr_t *pa, vaddr_t *va, size_t npages)
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
vmx_memfree(paddr_t pa, vaddr_t va, size_t npages)
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

static void
vmx_vcpu_msr_allow(uint8_t *bitmap, uint64_t msr, bool read, bool write)
{
	uint64_t byte;
	uint8_t bitoff;

	if (msr < 0x00002000) {
		/* Range 1 */
		byte = ((msr - 0x00000000) / 8) + 0;
	} else if (msr >= 0xC0000000 && msr < 0xC0002000) {
		/* Range 2 */
		byte = ((msr - 0xC0000000) / 8) + 1024;
	} else {
		panic("%s: wrong range", __func__);
	}

	bitoff = (msr & 0x7);

	if (read) {
		bitmap[byte] &= ~__BIT(bitoff);
	}
	if (write) {
		bitmap[2048 + byte] &= ~__BIT(bitoff);
	}
}

#define VMX_SEG_ATTRIB_TYPE		__BITS(4,0)
#define VMX_SEG_ATTRIB_DPL		__BITS(6,5)
#define VMX_SEG_ATTRIB_P		__BIT(7)
#define VMX_SEG_ATTRIB_AVL		__BIT(12)
#define VMX_SEG_ATTRIB_LONG		__BIT(13)
#define VMX_SEG_ATTRIB_DEF32		__BIT(14)
#define VMX_SEG_ATTRIB_GRAN		__BIT(15)
#define VMX_SEG_ATTRIB_UNUSABLE		__BIT(16)

static void
vmx_vcpu_setstate_seg(const struct nvmm_x64_state_seg *segs, int idx)
{
	uint64_t attrib;

	attrib =
	    __SHIFTIN(segs[idx].attrib.type, VMX_SEG_ATTRIB_TYPE) |
	    __SHIFTIN(segs[idx].attrib.dpl, VMX_SEG_ATTRIB_DPL) |
	    __SHIFTIN(segs[idx].attrib.p, VMX_SEG_ATTRIB_P) |
	    __SHIFTIN(segs[idx].attrib.avl, VMX_SEG_ATTRIB_AVL) |
	    __SHIFTIN(segs[idx].attrib.lng, VMX_SEG_ATTRIB_LONG) |
	    __SHIFTIN(segs[idx].attrib.def32, VMX_SEG_ATTRIB_DEF32) |
	    __SHIFTIN(segs[idx].attrib.gran, VMX_SEG_ATTRIB_GRAN) |
	    (!segs[idx].attrib.p ? VMX_SEG_ATTRIB_UNUSABLE : 0);

	if (idx != NVMM_X64_SEG_GDT && idx != NVMM_X64_SEG_IDT) {
		vmx_vmwrite(vmx_guest_segs[idx].selector, segs[idx].selector);
		vmx_vmwrite(vmx_guest_segs[idx].attrib, attrib);
	}
	vmx_vmwrite(vmx_guest_segs[idx].limit, segs[idx].limit);
	vmx_vmwrite(vmx_guest_segs[idx].base, segs[idx].base);
}

static void
vmx_vcpu_getstate_seg(struct nvmm_x64_state_seg *segs, int idx)
{
	uint64_t attrib = 0;

	if (idx != NVMM_X64_SEG_GDT && idx != NVMM_X64_SEG_IDT) {
		vmx_vmread(vmx_guest_segs[idx].selector, &segs[idx].selector);
		vmx_vmread(vmx_guest_segs[idx].attrib, &attrib);
	}
	vmx_vmread(vmx_guest_segs[idx].limit, &segs[idx].limit);
	vmx_vmread(vmx_guest_segs[idx].base, &segs[idx].base);

	segs[idx].attrib.type = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_TYPE);
	segs[idx].attrib.dpl = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_DPL);
	segs[idx].attrib.p = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_P);
	segs[idx].attrib.avl = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_AVL);
	segs[idx].attrib.lng = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_LONG);
	segs[idx].attrib.def32 = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_DEF32);
	segs[idx].attrib.gran = __SHIFTOUT(attrib, VMX_SEG_ATTRIB_GRAN);
	if (attrib & VMX_SEG_ATTRIB_UNUSABLE) {
		segs[idx].attrib.p = 0;
	}
}

static inline bool
vmx_state_tlb_flush(const struct nvmm_x64_state *state, uint64_t flags)
{
	uint64_t cr0, cr3, cr4, efer;

	if (flags & NVMM_X64_STATE_CRS) {
		vmx_vmread(VMCS_GUEST_CR0, &cr0);
		if ((cr0 ^ state->crs[NVMM_X64_CR_CR0]) & CR0_TLB_FLUSH) {
			return true;
		}
		vmx_vmread(VMCS_GUEST_CR3, &cr3);
		if (cr3 != state->crs[NVMM_X64_CR_CR3]) {
			return true;
		}
		vmx_vmread(VMCS_GUEST_CR4, &cr4);
		if ((cr4 ^ state->crs[NVMM_X64_CR_CR4]) & CR4_TLB_FLUSH) {
			return true;
		}
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		vmx_vmread(VMCS_GUEST_IA32_EFER, &efer);
		if ((efer ^
		     state->msrs[NVMM_X64_MSR_EFER]) & EFER_TLB_FLUSH) {
			return true;
		}
	}

	return false;
}

static void
vmx_vcpu_setstate(struct nvmm_cpu *vcpu, const void *data, uint64_t flags)
{
	const struct nvmm_x64_state *state = data;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct fxsave *fpustate;
	uint64_t ctls1, intstate;

	vmx_vmcs_enter(vcpu);

	if (vmx_state_tlb_flush(state, flags)) {
		cpudata->gtlb_want_flush = true;
	}

	if (flags & NVMM_X64_STATE_SEGS) {
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_CS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_DS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_ES);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_FS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_GS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_SS);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_GDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_IDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_LDT);
		vmx_vcpu_setstate_seg(state->segs, NVMM_X64_SEG_TR);
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(cpudata->gprs, state->gprs, sizeof(state->gprs));

		vmx_vmwrite(VMCS_GUEST_RIP, state->gprs[NVMM_X64_GPR_RIP]);
		vmx_vmwrite(VMCS_GUEST_RSP, state->gprs[NVMM_X64_GPR_RSP]);
		vmx_vmwrite(VMCS_GUEST_RFLAGS, state->gprs[NVMM_X64_GPR_RFLAGS]);
	}

	if (flags & NVMM_X64_STATE_CRS) {
		/*
		 * CR0_NE and CR4_VMXE are mandatory.
		 */
		vmx_vmwrite(VMCS_GUEST_CR0,
		    state->crs[NVMM_X64_CR_CR0] | CR0_NE);
		cpudata->gcr2 = state->crs[NVMM_X64_CR_CR2];
		vmx_vmwrite(VMCS_GUEST_CR3, state->crs[NVMM_X64_CR_CR3]); // XXX PDPTE?
		vmx_vmwrite(VMCS_GUEST_CR4,
		    state->crs[NVMM_X64_CR_CR4] | CR4_VMXE);
		cpudata->gcr8 = state->crs[NVMM_X64_CR_CR8];

		if (vmx_xcr0_mask != 0) {
			/* Clear illegal XCR0 bits, set mandatory X87 bit. */
			cpudata->gxcr0 = state->crs[NVMM_X64_CR_XCR0];
			cpudata->gxcr0 &= vmx_xcr0_mask;
			cpudata->gxcr0 |= XCR0_X87;
		}
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(cpudata->drs, state->drs, sizeof(state->drs));

		cpudata->drs[NVMM_X64_DR_DR6] &= 0xFFFFFFFF;
		vmx_vmwrite(VMCS_GUEST_DR7, cpudata->drs[NVMM_X64_DR_DR7]);
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		cpudata->gmsr[VMX_MSRLIST_STAR].val =
		    state->msrs[NVMM_X64_MSR_STAR];
		cpudata->gmsr[VMX_MSRLIST_LSTAR].val =
		    state->msrs[NVMM_X64_MSR_LSTAR];
		cpudata->gmsr[VMX_MSRLIST_CSTAR].val =
		    state->msrs[NVMM_X64_MSR_CSTAR];
		cpudata->gmsr[VMX_MSRLIST_SFMASK].val =
		    state->msrs[NVMM_X64_MSR_SFMASK];
		cpudata->gmsr[VMX_MSRLIST_KERNELGSBASE].val =
		    state->msrs[NVMM_X64_MSR_KERNELGSBASE];

		vmx_vmwrite(VMCS_GUEST_IA32_EFER,
		    state->msrs[NVMM_X64_MSR_EFER]);
		vmx_vmwrite(VMCS_GUEST_IA32_PAT,
		    state->msrs[NVMM_X64_MSR_PAT]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_CS,
		    state->msrs[NVMM_X64_MSR_SYSENTER_CS]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_ESP,
		    state->msrs[NVMM_X64_MSR_SYSENTER_ESP]);
		vmx_vmwrite(VMCS_GUEST_IA32_SYSENTER_EIP,
		    state->msrs[NVMM_X64_MSR_SYSENTER_EIP]);

		/* ENTRY_CTLS_LONG_MODE must match EFER_LMA. */
		vmx_vmread(VMCS_ENTRY_CTLS, &ctls1);
		if (state->msrs[NVMM_X64_MSR_EFER] & EFER_LMA) {
			ctls1 |= ENTRY_CTLS_LONG_MODE;
		} else {
			ctls1 &= ~ENTRY_CTLS_LONG_MODE;
		}
		vmx_vmwrite(VMCS_ENTRY_CTLS, ctls1);
	}

	if (flags & NVMM_X64_STATE_MISC) {
		vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY, &intstate);
		intstate &= ~(INT_STATE_STI|INT_STATE_MOVSS);
		if (state->misc[NVMM_X64_MISC_INT_SHADOW]) {
			intstate |= INT_STATE_MOVSS;
		}
		vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY, intstate);

		if (state->misc[NVMM_X64_MISC_INT_WINDOW_EXIT]) {
			vmx_event_waitexit_enable(vcpu, false);
		} else {
			vmx_event_waitexit_disable(vcpu, false);
		}

		if (state->misc[NVMM_X64_MISC_NMI_WINDOW_EXIT]) {
			vmx_event_waitexit_enable(vcpu, true);
		} else {
			vmx_event_waitexit_disable(vcpu, true);
		}
	}

	CTASSERT(sizeof(cpudata->gfpu.xsh_fxsave) == sizeof(state->fpu));
	if (flags & NVMM_X64_STATE_FPU) {
		memcpy(cpudata->gfpu.xsh_fxsave, &state->fpu,
		    sizeof(state->fpu));

		fpustate = (struct fxsave *)cpudata->gfpu.xsh_fxsave;
		fpustate->fx_mxcsr_mask &= x86_fpu_mxcsr_mask;
		fpustate->fx_mxcsr &= fpustate->fx_mxcsr_mask;

		if (vmx_xcr0_mask != 0) {
			/* Reset XSTATE_BV, to force a reload. */
			cpudata->gfpu.xsh_xstate_bv = vmx_xcr0_mask;
		}
	}

	vmx_vmcs_leave(vcpu);
}

static void
vmx_vcpu_getstate(struct nvmm_cpu *vcpu, void *data, uint64_t flags)
{
	struct nvmm_x64_state *state = (struct nvmm_x64_state *)data;
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	uint64_t intstate;

	vmx_vmcs_enter(vcpu);

	if (flags & NVMM_X64_STATE_SEGS) {
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_CS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_DS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_ES);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_FS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_GS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_SS);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_GDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_IDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_LDT);
		vmx_vcpu_getstate_seg(state->segs, NVMM_X64_SEG_TR);
	}

	CTASSERT(sizeof(cpudata->gprs) == sizeof(state->gprs));
	if (flags & NVMM_X64_STATE_GPRS) {
		memcpy(state->gprs, cpudata->gprs, sizeof(state->gprs));

		vmx_vmread(VMCS_GUEST_RIP, &state->gprs[NVMM_X64_GPR_RIP]);
		vmx_vmread(VMCS_GUEST_RSP, &state->gprs[NVMM_X64_GPR_RSP]);
		vmx_vmread(VMCS_GUEST_RFLAGS, &state->gprs[NVMM_X64_GPR_RFLAGS]);
	}

	if (flags & NVMM_X64_STATE_CRS) {
		vmx_vmread(VMCS_GUEST_CR0, &state->crs[NVMM_X64_CR_CR0]);
		state->crs[NVMM_X64_CR_CR2] = cpudata->gcr2;
		vmx_vmread(VMCS_GUEST_CR3, &state->crs[NVMM_X64_CR_CR3]);
		vmx_vmread(VMCS_GUEST_CR4, &state->crs[NVMM_X64_CR_CR4]);
		state->crs[NVMM_X64_CR_CR8] = cpudata->gcr8;
		state->crs[NVMM_X64_CR_XCR0] = cpudata->gxcr0;

		/* Hide VMXE. */
		state->crs[NVMM_X64_CR_CR4] &= ~CR4_VMXE;
	}

	CTASSERT(sizeof(cpudata->drs) == sizeof(state->drs));
	if (flags & NVMM_X64_STATE_DRS) {
		memcpy(state->drs, cpudata->drs, sizeof(state->drs));

		vmx_vmread(VMCS_GUEST_DR7, &state->drs[NVMM_X64_DR_DR7]);
	}

	if (flags & NVMM_X64_STATE_MSRS) {
		state->msrs[NVMM_X64_MSR_STAR] =
		    cpudata->gmsr[VMX_MSRLIST_STAR].val;
		state->msrs[NVMM_X64_MSR_LSTAR] =
		    cpudata->gmsr[VMX_MSRLIST_LSTAR].val;
		state->msrs[NVMM_X64_MSR_CSTAR] =
		    cpudata->gmsr[VMX_MSRLIST_CSTAR].val;
		state->msrs[NVMM_X64_MSR_SFMASK] =
		    cpudata->gmsr[VMX_MSRLIST_SFMASK].val;
		state->msrs[NVMM_X64_MSR_KERNELGSBASE] =
		    cpudata->gmsr[VMX_MSRLIST_KERNELGSBASE].val;

		vmx_vmread(VMCS_GUEST_IA32_EFER,
		    &state->msrs[NVMM_X64_MSR_EFER]);
		vmx_vmread(VMCS_GUEST_IA32_PAT,
		    &state->msrs[NVMM_X64_MSR_PAT]);
		vmx_vmread(VMCS_GUEST_IA32_SYSENTER_CS,
		    &state->msrs[NVMM_X64_MSR_SYSENTER_CS]);
		vmx_vmread(VMCS_GUEST_IA32_SYSENTER_ESP,
		    &state->msrs[NVMM_X64_MSR_SYSENTER_ESP]);
		vmx_vmread(VMCS_GUEST_IA32_SYSENTER_EIP,
		    &state->msrs[NVMM_X64_MSR_SYSENTER_EIP]);
	}

	if (flags & NVMM_X64_STATE_MISC) {
		vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY, &intstate);
		state->misc[NVMM_X64_MISC_INT_SHADOW] =
		    (intstate & (INT_STATE_STI|INT_STATE_MOVSS)) != 0;

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

	vmx_vmcs_leave(vcpu);
}

/* -------------------------------------------------------------------------- */

static void
vmx_asid_alloc(struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	size_t i, oct, bit;

	mutex_enter(&vmx_asidlock);

	for (i = 0; i < vmx_maxasid; i++) {
		oct = i / 8;
		bit = i % 8;

		if (vmx_asidmap[oct] & __BIT(bit)) {
			continue;
		}

		cpudata->asid = i;

		vmx_asidmap[oct] |= __BIT(bit);
		vmx_vmwrite(VMCS_VPID, i);
		mutex_exit(&vmx_asidlock);
		return;
	}

	mutex_exit(&vmx_asidlock);

	panic("%s: impossible", __func__);
}

static void
vmx_asid_free(struct nvmm_cpu *vcpu)
{
	size_t oct, bit;
	uint64_t asid;

	vmx_vmread(VMCS_VPID, &asid);

	oct = asid / 8;
	bit = asid % 8;

	mutex_enter(&vmx_asidlock);
	vmx_asidmap[oct] &= ~__BIT(bit);
	mutex_exit(&vmx_asidlock);
}

static void
vmx_vcpu_init(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;
	struct vmcs *vmcs = cpudata->vmcs;
	struct msr_entry *gmsr = cpudata->gmsr;
	extern uint8_t vmx_resume_rip;
	uint64_t rev, eptp;

	rev = vmx_get_revision();

	memset(vmcs, 0, VMCS_SIZE);
	vmcs->ident = __SHIFTIN(rev, VMCS_IDENT_REVISION);
	vmcs->abort = 0;

	vmx_vmcs_enter(vcpu);

	/* No link pointer. */
	vmx_vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFFFFFFFFFF);

	/* Install the CTLSs. */
	vmx_vmwrite(VMCS_PINBASED_CTLS, vmx_pinbased_ctls);
	vmx_vmwrite(VMCS_PROCBASED_CTLS, vmx_procbased_ctls);
	vmx_vmwrite(VMCS_PROCBASED_CTLS2, vmx_procbased_ctls2);
	vmx_vmwrite(VMCS_ENTRY_CTLS, vmx_entry_ctls);
	vmx_vmwrite(VMCS_EXIT_CTLS, vmx_exit_ctls);

	/* Allow direct access to certain MSRs. */
	memset(cpudata->msrbm, 0xFF, MSRBM_SIZE);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_EFER, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_STAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_LSTAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_CSTAR, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SFMASK, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_KERNELGSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_CS, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_ESP, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_SYSENTER_EIP, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_FSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_GSBASE, true, true);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_TSC, true, false);
	vmx_vcpu_msr_allow(cpudata->msrbm, MSR_IA32_ARCH_CAPABILITIES,
	    true, false);
	vmx_vmwrite(VMCS_MSR_BITMAP, (uint64_t)cpudata->msrbm_pa);

	/*
	 * List of Guest MSRs loaded on VMENTRY, saved on VMEXIT. This
	 * includes the L1D_FLUSH MSR, to mitigate L1TF.
	 */
	gmsr[VMX_MSRLIST_STAR].msr = MSR_STAR;
	gmsr[VMX_MSRLIST_STAR].val = 0;
	gmsr[VMX_MSRLIST_LSTAR].msr = MSR_LSTAR;
	gmsr[VMX_MSRLIST_LSTAR].val = 0;
	gmsr[VMX_MSRLIST_CSTAR].msr = MSR_CSTAR;
	gmsr[VMX_MSRLIST_CSTAR].val = 0;
	gmsr[VMX_MSRLIST_SFMASK].msr = MSR_SFMASK;
	gmsr[VMX_MSRLIST_SFMASK].val = 0;
	gmsr[VMX_MSRLIST_KERNELGSBASE].msr = MSR_KERNELGSBASE;
	gmsr[VMX_MSRLIST_KERNELGSBASE].val = 0;
	gmsr[VMX_MSRLIST_L1DFLUSH].msr = MSR_IA32_FLUSH_CMD;
	gmsr[VMX_MSRLIST_L1DFLUSH].val = IA32_FLUSH_CMD_L1D_FLUSH;
	vmx_vmwrite(VMCS_ENTRY_MSR_LOAD_ADDRESS, cpudata->gmsr_pa);
	vmx_vmwrite(VMCS_EXIT_MSR_STORE_ADDRESS, cpudata->gmsr_pa);
	vmx_vmwrite(VMCS_ENTRY_MSR_LOAD_COUNT, vmx_msrlist_entry_nmsr);
	vmx_vmwrite(VMCS_EXIT_MSR_STORE_COUNT, VMX_MSRLIST_EXIT_NMSR);

	/* Force CR0_NW and CR0_CD to zero, CR0_ET to one. */
	vmx_vmwrite(VMCS_CR0_MASK, CR0_NW|CR0_CD|CR0_ET);
	vmx_vmwrite(VMCS_CR0_SHADOW, CR0_ET);

	/* Force CR4_VMXE to zero. */
	vmx_vmwrite(VMCS_CR4_MASK, CR4_VMXE);

	/* Set the Host state for resuming. */
	vmx_vmwrite(VMCS_HOST_RIP, (uint64_t)&vmx_resume_rip);
	vmx_vmwrite(VMCS_HOST_CS_SELECTOR, GSEL(GCODE_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_SS_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_DS_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_ES_SELECTOR, GSEL(GDATA_SEL, SEL_KPL));
	vmx_vmwrite(VMCS_HOST_FS_SELECTOR, 0);
	vmx_vmwrite(VMCS_HOST_GS_SELECTOR, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_CS, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_ESP, 0);
	vmx_vmwrite(VMCS_HOST_IA32_SYSENTER_EIP, 0);
	vmx_vmwrite(VMCS_HOST_IDTR_BASE, (uint64_t)idt);
	vmx_vmwrite(VMCS_HOST_IA32_PAT, rdmsr(MSR_CR_PAT));
	vmx_vmwrite(VMCS_HOST_IA32_EFER, rdmsr(MSR_EFER));
	vmx_vmwrite(VMCS_HOST_CR0, rcr0());

	/* Generate ASID. */
	vmx_asid_alloc(vcpu);

	/* Enable Extended Paging, 4-Level. */
	eptp =
	    __SHIFTIN(vmx_eptp_type, EPTP_TYPE) |
	    __SHIFTIN(4-1, EPTP_WALKLEN) |
	    (pmap_ept_has_ad ? EPTP_FLAGS_AD : 0) |
	    mach->vm->vm_map.pmap->pm_pdirpa[0];
	vmx_vmwrite(VMCS_EPTP, eptp);

	/* Init IA32_MISC_ENABLE. */
	cpudata->gmsr_misc_enable = rdmsr(MSR_MISC_ENABLE);
	cpudata->gmsr_misc_enable &=
	    ~(IA32_MISC_PERFMON_EN|IA32_MISC_EISST_EN|IA32_MISC_MWAIT_EN);
	cpudata->gmsr_misc_enable |=
	    (IA32_MISC_BTS_UNAVAIL|IA32_MISC_PEBS_UNAVAIL);

	/* Init XSAVE header. */
	cpudata->gfpu.xsh_xstate_bv = vmx_xcr0_mask;
	cpudata->gfpu.xsh_xcomp_bv = 0;

	/* Set guest TSC to zero, more or less. */
	cpudata->tsc_offset = -cpu_counter();

	/* These MSRs are static. */
	cpudata->star = rdmsr(MSR_STAR);
	cpudata->cstar = rdmsr(MSR_CSTAR);
	cpudata->sfmask = rdmsr(MSR_SFMASK);

	/* Install the RESET state. */
	vmx_vcpu_setstate(vcpu, &nvmm_x86_reset_state, NVMM_X64_STATE_ALL);

	vmx_vmcs_leave(vcpu);
}

static int
vmx_vcpu_create(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata;
	int error;

	/* Allocate the VMX cpudata. */
	cpudata = (struct vmx_cpudata *)uvm_km_alloc(kernel_map,
	    roundup(sizeof(*cpudata), PAGE_SIZE), 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	vcpu->cpudata = cpudata;

	/* VMCS */
	error = vmx_memalloc(&cpudata->vmcs_pa, (vaddr_t *)&cpudata->vmcs,
	    VMCS_NPAGES);
	if (error)
		goto error;

	/* MSR Bitmap */
	error = vmx_memalloc(&cpudata->msrbm_pa, (vaddr_t *)&cpudata->msrbm,
	    MSRBM_NPAGES);
	if (error)
		goto error;

	/* Guest MSR List */
	error = vmx_memalloc(&cpudata->gmsr_pa, (vaddr_t *)&cpudata->gmsr, 1);
	if (error)
		goto error;

	kcpuset_create(&cpudata->htlb_want_flush, true);

	/* Init the VCPU info. */
	vmx_vcpu_init(mach, vcpu);

	return 0;

error:
	if (cpudata->vmcs_pa) {
		vmx_memfree(cpudata->vmcs_pa, (vaddr_t)cpudata->vmcs,
		    VMCS_NPAGES);
	}
	if (cpudata->msrbm_pa) {
		vmx_memfree(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm,
		    MSRBM_NPAGES);
	}
	if (cpudata->gmsr_pa) {
		vmx_memfree(cpudata->gmsr_pa, (vaddr_t)cpudata->gmsr, 1);
	}

	kmem_free(cpudata, sizeof(*cpudata));
	return error;
}

static void
vmx_vcpu_destroy(struct nvmm_machine *mach, struct nvmm_cpu *vcpu)
{
	struct vmx_cpudata *cpudata = vcpu->cpudata;

	vmx_vmcs_enter(vcpu);
	vmx_asid_free(vcpu);
	vmx_vmcs_leave(vcpu);

	kcpuset_destroy(cpudata->htlb_want_flush);

	vmx_memfree(cpudata->vmcs_pa, (vaddr_t)cpudata->vmcs, VMCS_NPAGES);
	vmx_memfree(cpudata->msrbm_pa, (vaddr_t)cpudata->msrbm, MSRBM_NPAGES);
	vmx_memfree(cpudata->gmsr_pa, (vaddr_t)cpudata->gmsr, 1);
	uvm_km_free(kernel_map, (vaddr_t)cpudata,
	    roundup(sizeof(*cpudata), PAGE_SIZE), UVM_KMF_WIRED);
}

/* -------------------------------------------------------------------------- */

static void
vmx_tlb_flush(struct pmap *pm)
{
	struct nvmm_machine *mach = pm->pm_data;
	struct vmx_machdata *machdata = mach->machdata;

	atomic_inc_64(&machdata->mach_htlb_gen);

	/* Generates IPIs, which cause #VMEXITs. */
	pmap_tlb_shootdown(pmap_kernel(), -1, PG_G, TLBSHOOT_UPDATE);
}

static void
vmx_machine_create(struct nvmm_machine *mach)
{
	struct pmap *pmap = mach->vm->vm_map.pmap;
	struct vmx_machdata *machdata;

	/* Convert to EPT. */
	pmap_ept_transform(pmap);

	/* Fill in pmap info. */
	pmap->pm_data = (void *)mach;
	pmap->pm_tlb_flush = vmx_tlb_flush;

	machdata = kmem_zalloc(sizeof(struct vmx_machdata), KM_SLEEP);
	mach->machdata = machdata;

	/* Start with an hTLB flush everywhere. */
	machdata->mach_htlb_gen = 1;
}

static void
vmx_machine_destroy(struct nvmm_machine *mach)
{
	struct vmx_machdata *machdata = mach->machdata;

	kmem_free(machdata, sizeof(struct vmx_machdata));
}

static int
vmx_machine_configure(struct nvmm_machine *mach, uint64_t op, void *data)
{
	struct nvmm_x86_conf_cpuid *cpuid = data;
	struct vmx_machdata *machdata = (struct vmx_machdata *)mach->machdata;
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
	for (i = 0; i < VMX_NCPUIDS; i++) {
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
	for (i = 0; i < VMX_NCPUIDS; i++) {
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

static int
vmx_init_ctls(uint64_t msr_ctls, uint64_t msr_true_ctls,
    uint64_t set_one, uint64_t set_zero, uint64_t *res)
{
	uint64_t basic, val, true_val;
	bool one_allowed, zero_allowed, has_true;
	size_t i;

	basic = rdmsr(MSR_IA32_VMX_BASIC);
	has_true = (basic & IA32_VMX_BASIC_TRUE_CTLS) != 0;

	val = rdmsr(msr_ctls);
	if (has_true) {
		true_val = rdmsr(msr_true_ctls);
	} else {
		true_val = val;
	}

#define ONE_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(32 + bitoff)) != 0)
#define ZERO_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(bitoff)) == 0)

	for (i = 0; i < 32; i++) {
		one_allowed = ONE_ALLOWED(true_val, i);
		zero_allowed = ZERO_ALLOWED(true_val, i);

		if (zero_allowed && !one_allowed) {
			if (set_one & __BIT(i))
				return -1;
			*res &= ~__BIT(i);
		} else if (one_allowed && !zero_allowed) {
			if (set_zero & __BIT(i))
				return -1;
			*res |= __BIT(i);
		} else {
			if (set_zero & __BIT(i)) {
				*res &= ~__BIT(i);
			} else if (set_one & __BIT(i)) {
				*res |= __BIT(i);
			} else if (!has_true) {
				*res &= ~__BIT(i);
			} else if (ZERO_ALLOWED(val, i)) {
				*res &= ~__BIT(i);
			} else if (ONE_ALLOWED(val, i)) {
				*res |= __BIT(i);
			} else {
				return -1;
			}
		}
	}

	return 0;
}

static bool
vmx_ident(void)
{
	uint64_t msr;
	int ret;

	if (!(cpu_feature[1] & CPUID2_VMX)) {
		return false;
	}

	msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
	if ((msr & IA32_FEATURE_CONTROL_LOCK) == 0) {
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_BASIC);
	if ((msr & IA32_VMX_BASIC_IO_REPORT) == 0) {
		return false;
	}
	if (__SHIFTOUT(msr, IA32_VMX_BASIC_MEM_TYPE) != MEM_TYPE_WB) {
		return false;
	}

	/* PG and PE are reported, even if Unrestricted Guests is supported. */
	vmx_cr0_fixed0 = rdmsr(MSR_IA32_VMX_CR0_FIXED0) & ~(CR0_PG|CR0_PE);
	vmx_cr0_fixed1 = rdmsr(MSR_IA32_VMX_CR0_FIXED1) | (CR0_PG|CR0_PE);
	ret = vmx_check_cr(rcr0(), vmx_cr0_fixed0, vmx_cr0_fixed1);
	if (ret == -1) {
		return false;
	}

	vmx_cr4_fixed0 = rdmsr(MSR_IA32_VMX_CR4_FIXED0);
	vmx_cr4_fixed1 = rdmsr(MSR_IA32_VMX_CR4_FIXED1);
	ret = vmx_check_cr(rcr4() | CR4_VMXE, vmx_cr4_fixed0, vmx_cr4_fixed1);
	if (ret == -1) {
		return false;
	}

	/* Init the CTLSs right now, and check for errors. */
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PINBASED_CTLS, MSR_IA32_VMX_TRUE_PINBASED_CTLS,
	    VMX_PINBASED_CTLS_ONE, VMX_PINBASED_CTLS_ZERO,
	    &vmx_pinbased_ctls);
	if (ret == -1) {
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS, MSR_IA32_VMX_TRUE_PROCBASED_CTLS,
	    VMX_PROCBASED_CTLS_ONE, VMX_PROCBASED_CTLS_ZERO,
	    &vmx_procbased_ctls);
	if (ret == -1) {
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS2, MSR_IA32_VMX_PROCBASED_CTLS2,
	    VMX_PROCBASED_CTLS2_ONE, VMX_PROCBASED_CTLS2_ZERO,
	    &vmx_procbased_ctls2);
	if (ret == -1) {
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_ENTRY_CTLS, MSR_IA32_VMX_TRUE_ENTRY_CTLS,
	    VMX_ENTRY_CTLS_ONE, VMX_ENTRY_CTLS_ZERO,
	    &vmx_entry_ctls);
	if (ret == -1) {
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_EXIT_CTLS, MSR_IA32_VMX_TRUE_EXIT_CTLS,
	    VMX_EXIT_CTLS_ONE, VMX_EXIT_CTLS_ZERO,
	    &vmx_exit_ctls);
	if (ret == -1) {
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((msr & IA32_VMX_EPT_VPID_WALKLENGTH_4) == 0) {
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVEPT) == 0) {
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVVPID) == 0) {
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_FLAGS_AD) != 0) {
		pmap_ept_has_ad = true;
	} else {
		pmap_ept_has_ad = false;
	}
	if (!(msr & IA32_VMX_EPT_VPID_UC) && !(msr & IA32_VMX_EPT_VPID_WB)) {
		return false;
	}

	return true;
}

static void
vmx_init_asid(uint32_t maxasid)
{
	size_t allocsz;

	mutex_init(&vmx_asidlock, MUTEX_DEFAULT, IPL_NONE);

	vmx_maxasid = maxasid;
	allocsz = roundup(maxasid, 8) / 8;
	vmx_asidmap = kmem_zalloc(allocsz, KM_SLEEP);

	/* ASID 0 is reserved for the host. */
	vmx_asidmap[0] |= __BIT(0);
}

static void
vmx_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enable = (bool)arg1;
	uint64_t cr4;

	if (!enable) {
		vmx_vmxoff();
	}

	cr4 = rcr4();
	if (enable) {
		cr4 |= CR4_VMXE;
	} else {
		cr4 &= ~CR4_VMXE;
	}
	lcr4(cr4);

	if (enable) {
		vmx_vmxon(&vmxoncpu[cpu_index(ci)].pa);
	}
}

static void
vmx_init_l1tf(void)
{
	u_int descs[4];
	uint64_t msr;

	if (cpuid_level < 7) {
		return;
	}

	x86_cpuid(7, descs);

	if (descs[3] & CPUID_SEF_ARCH_CAP) {
		msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
		if (msr & IA32_ARCH_SKIP_L1DFL_VMENTRY) {
			/* No mitigation needed. */
			return;
		}
	}

	if (descs[3] & CPUID_SEF_L1D_FLUSH) {
		/* Enable hardware mitigation. */
		vmx_msrlist_entry_nmsr += 1;
	}
}

static void
vmx_init(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	uint64_t xc, msr;
	struct vmxon *vmxon;
	uint32_t revision;
	paddr_t pa;
	vaddr_t va;
	int error;

	/* Init the ASID bitmap (VPID). */
	vmx_init_asid(VPID_MAX);

	/* Init the XCR0 mask. */
	vmx_xcr0_mask = VMX_XCR0_MASK_DEFAULT & x86_xsave_features;

	/* Init the TLB flush op, the EPT flush op and the EPTP type. */
	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((msr & IA32_VMX_EPT_VPID_INVVPID_CONTEXT) != 0) {
		vmx_tlb_flush_op = VMX_INVVPID_CONTEXT;
	} else {
		vmx_tlb_flush_op = VMX_INVVPID_ALL;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVEPT_CONTEXT) != 0) {
		vmx_ept_flush_op = VMX_INVEPT_CONTEXT;
	} else {
		vmx_ept_flush_op = VMX_INVEPT_ALL;
	}
	if ((msr & IA32_VMX_EPT_VPID_WB) != 0) {
		vmx_eptp_type = EPTP_TYPE_WB;
	} else {
		vmx_eptp_type = EPTP_TYPE_UC;
	}

	/* Init the L1TF mitigation. */
	vmx_init_l1tf();

	memset(vmxoncpu, 0, sizeof(vmxoncpu));
	revision = vmx_get_revision();

	for (CPU_INFO_FOREACH(cii, ci)) {
		error = vmx_memalloc(&pa, &va, 1);
		if (error) {
			panic("%s: out of memory", __func__);
		}
		vmxoncpu[cpu_index(ci)].pa = pa;
		vmxoncpu[cpu_index(ci)].va = va;

		vmxon = (struct vmxon *)vmxoncpu[cpu_index(ci)].va;
		vmxon->ident = __SHIFTIN(revision, VMXON_IDENT_REVISION);
	}

	xc = xc_broadcast(0, vmx_change_cpu, (void *)true, NULL);
	xc_wait(xc);
}

static void
vmx_fini_asid(void)
{
	size_t allocsz;

	allocsz = roundup(vmx_maxasid, 8) / 8;
	kmem_free(vmx_asidmap, allocsz);

	mutex_destroy(&vmx_asidlock);
}

static void
vmx_fini(void)
{
	uint64_t xc;
	size_t i;

	xc = xc_broadcast(0, vmx_change_cpu, (void *)false, NULL);
	xc_wait(xc);

	for (i = 0; i < MAXCPUS; i++) {
		if (vmxoncpu[i].pa != 0)
			vmx_memfree(vmxoncpu[i].pa, vmxoncpu[i].va, 1);
	}

	vmx_fini_asid();
}

static void
vmx_capability(struct nvmm_capability *cap)
{
	cap->u.x86.xcr0_mask = vmx_xcr0_mask;
	cap->u.x86.mxcsr_mask = x86_fpu_mxcsr_mask;
	cap->u.x86.conf_cpuid_maxops = VMX_NCPUIDS;
}

const struct nvmm_impl nvmm_x86_vmx = {
	.ident = vmx_ident,
	.init = vmx_init,
	.fini = vmx_fini,
	.capability = vmx_capability,
	.conf_max = NVMM_X86_NCONF,
	.conf_sizes = vmx_conf_sizes,
	.state_size = sizeof(struct nvmm_x64_state),
	.machine_create = vmx_machine_create,
	.machine_destroy = vmx_machine_destroy,
	.machine_configure = vmx_machine_configure,
	.vcpu_create = vmx_vcpu_create,
	.vcpu_destroy = vmx_vcpu_destroy,
	.vcpu_setstate = vmx_vcpu_setstate,
	.vcpu_getstate = vmx_vcpu_getstate,
	.vcpu_inject = vmx_vcpu_inject,
	.vcpu_run = vmx_vcpu_run
};
