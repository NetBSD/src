/* $NetBSD: trap.c,v 1.49 2023/07/16 21:36:40 riastradh Exp $ */

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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: trap.c,v 1.49 2023/07/16 21:36:40 riastradh Exp $");

#include "opt_arm_intr_impl.h"
#include "opt_compat_netbsd32.h"
#include "opt_dtrace.h"

#include <sys/param.h>
#include <sys/kauth.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#ifdef KDB
#include <sys/kdb.h>
#endif
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>
#include <sys/xcall.h>

#ifdef ARM_INTR_IMPL
#include ARM_INTR_IMPL
#else
#error ARM_INTR_IMPL not defined
#endif

#ifndef ARM_IRQ_HANDLER
#error ARM_IRQ_HANDLER not defined
#endif

#include <arm/cpufunc.h>

#include <aarch64/userret.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/locore.h>

#include <arm/cpufunc.h>

#ifdef KDB
#include <machine/db_machdep.h>
#endif
#ifdef DDB
#include <ddb/db_output.h>
#include <machine/db_machdep.h>
#endif
#ifdef KDTRACE_HOOKS
#include <sys/dtrace_bsd.h>
#endif

#ifdef DDB
int sigill_debug = 0;
#endif

#ifdef KDTRACE_HOOKS
dtrace_doubletrap_func_t	dtrace_doubletrap_func = NULL;
dtrace_trap_func_t		dtrace_trap_func = NULL;
int (*dtrace_invop_jump_addr)(struct trapframe *);
#endif

enum emul_arm_result {
	EMUL_ARM_SUCCESS = 0,
	EMUL_ARM_UNKNOWN,
	EMUL_ARM_FAULT,
};

const char * const trap_names[] = {
	[ESR_EC_UNKNOWN]	= "Unknown Reason (Illegal Instruction)",
	[ESR_EC_SERROR]		= "SError Interrupt",
	[ESR_EC_WFX]		= "WFI or WFE instruction execution",
	[ESR_EC_ILL_STATE]	= "Illegal Execution State",

	[ESR_EC_BTE_A64]	= "Branch Target Exception",

	[ESR_EC_SYS_REG]	= "MSR/MRS/SYS instruction",
	[ESR_EC_SVC_A64]	= "SVC Instruction Execution",
	[ESR_EC_HVC_A64]	= "HVC Instruction Execution",
	[ESR_EC_SMC_A64]	= "SMC Instruction Execution",

	[ESR_EC_INSN_ABT_EL0]	= "Instruction Abort (EL0)",
	[ESR_EC_INSN_ABT_EL1]	= "Instruction Abort (EL1)",
	[ESR_EC_DATA_ABT_EL0]	= "Data Abort (EL0)",
	[ESR_EC_DATA_ABT_EL1]	= "Data Abort (EL1)",

	[ESR_EC_PC_ALIGNMENT]	= "Misaligned PC",
	[ESR_EC_SP_ALIGNMENT]	= "Misaligned SP",

	[ESR_EC_FP_ACCESS]	= "Access to SIMD/FP Registers",
	[ESR_EC_FP_TRAP_A64]	= "FP Exception",

	[ESR_EC_BRKPNT_EL0]	= "Breakpoint Exception (EL0)",
	[ESR_EC_BRKPNT_EL1]	= "Breakpoint Exception (EL1)",
	[ESR_EC_SW_STEP_EL0]	= "Software Step (EL0)",
	[ESR_EC_SW_STEP_EL1]	= "Software Step (EL1)",
	[ESR_EC_WTCHPNT_EL0]	= "Watchpoint (EL0)",
	[ESR_EC_WTCHPNT_EL1]	= "Watchpoint (EL1)",
	[ESR_EC_BKPT_INSN_A64]	= "BKPT Instruction Execution",

	[ESR_EC_CP15_RT]	= "A32: MCR/MRC access to CP15",
	[ESR_EC_CP15_RRT]	= "A32: MCRR/MRRC access to CP15",
	[ESR_EC_CP14_RT]	= "A32: MCR/MRC access to CP14",
	[ESR_EC_CP14_DT]	= "A32: LDC/STC access to CP14",
	[ESR_EC_CP14_RRT]	= "A32: MRRC access to CP14",
	[ESR_EC_SVC_A32]	= "A32: SVC Instruction Execution",
	[ESR_EC_HVC_A32]	= "A32: HVC Instruction Execution",
	[ESR_EC_SMC_A32]	= "A32: SMC Instruction Execution",
	[ESR_EC_FPID]		= "A32: MCR/MRC access to CP10",
	[ESR_EC_FP_TRAP_A32]	= "A32: FP Exception",
	[ESR_EC_BKPT_INSN_A32]	= "A32: BKPT Instruction Execution",
	[ESR_EC_VECTOR_CATCH]	= "A32: Vector Catch Exception"
};

const char *
eclass_trapname(uint32_t eclass)
{
	static char trapnamebuf[sizeof("Unknown trap 0x????????")];

	if (eclass >= __arraycount(trap_names) || trap_names[eclass] == NULL) {
		snprintf(trapnamebuf, sizeof(trapnamebuf),
		    "Unknown trap %#02x", eclass);
		return trapnamebuf;
	}
	return trap_names[eclass];
}

void
userret(struct lwp *l)
{
	mi_userret(l);
}

void
trap_doast(struct trapframe *tf)
{
	struct lwp * const l = curlwp;

	/*
	 * allow to have a chance of context switch just prior to user
	 * exception return.
	 */
#ifdef __HAVE_PREEMPTION
	kpreempt_disable();
#endif
	struct cpu_info * const ci = curcpu();

	ci->ci_data.cpu_ntrap++;

	KDASSERT(ci->ci_cpl == IPL_NONE);
#ifdef __HAVE_PREEMPTION
	kpreempt_enable();
#endif

	if (l->l_pflag & LP_OWEUPC) {
		l->l_pflag &= ~LP_OWEUPC;
		ADDUPROF(l);
	}

	userret(l);
}

void
trap_el1h_sync(struct trapframe *tf)
{
	const uint32_t esr = tf->tf_esr;
	const uint32_t eclass = __SHIFTOUT(esr, ESR_EC); /* exception class */

	/* re-enable traps and interrupts */
	if (!(tf->tf_spsr & SPSR_I))
		daif_enable(DAIF_D|DAIF_A|DAIF_I|DAIF_F);
	else
		daif_enable(DAIF_D|DAIF_A);

#ifdef KDTRACE_HOOKS
	if (dtrace_trap_func != NULL && (*dtrace_trap_func)(tf, eclass))
		return;
#endif

	switch (eclass) {
	case ESR_EC_INSN_ABT_EL1:
	case ESR_EC_DATA_ABT_EL1:
		data_abort_handler(tf, eclass);
		break;

	case ESR_EC_BKPT_INSN_A64:
#ifdef KDTRACE_HOOKS
		if (__SHIFTOUT(esr, ESR_ISS) == 0x40d &&
		    dtrace_invop_jump_addr != 0) {
			(*dtrace_invop_jump_addr)(tf);
			break;
		}
		/* FALLTHROUGH */
#endif
	case ESR_EC_BRKPNT_EL1:
	case ESR_EC_SW_STEP_EL1:
	case ESR_EC_WTCHPNT_EL1:
#ifdef DDB
		if (eclass == ESR_EC_BRKPNT_EL1)
			kdb_trap(DB_TRAP_BREAKPOINT, tf);
		else if (eclass == ESR_EC_BKPT_INSN_A64)
			kdb_trap(DB_TRAP_BKPT_INSN, tf);
		else if (eclass == ESR_EC_WTCHPNT_EL1)
			kdb_trap(DB_TRAP_WATCHPOINT, tf);
		else if (eclass == ESR_EC_SW_STEP_EL1)
			kdb_trap(DB_TRAP_SW_STEP, tf);
		else
			kdb_trap(DB_TRAP_UNKNOWN, tf);
#else
		panic("No debugger in kernel");
#endif
		break;

	case ESR_EC_FP_ACCESS:
		if ((curlwp->l_flag & (LW_SYSTEM|LW_SYSTEM_FPU)) ==
		    (LW_SYSTEM|LW_SYSTEM_FPU)) {
			fpu_load(curlwp);
			break;
		}
		/*FALLTHROUGH*/
	case ESR_EC_FP_TRAP_A64:
	case ESR_EC_PC_ALIGNMENT:
	case ESR_EC_SP_ALIGNMENT:
	case ESR_EC_ILL_STATE:
	case ESR_EC_BTE_A64:
	default:
		panic("Trap: fatal %s: pc=%016" PRIx64 " sp=%016" PRIx64
		    " esr=%08x", eclass_trapname(eclass), tf->tf_pc, tf->tf_sp,
		    esr);
		break;
	}
}

/*
 * There are some systems with different cache line sizes for each cpu.
 * Userland programs can be preempted between CPUs at any time, so in such
 * a system, the minimum cache line size must be visible to userland.
 */
#define CTR_EL0_USR_MASK	\
	(CTR_EL0_DIC | CTR_EL0_IDC | CTR_EL0_DMIN_LINE | CTR_EL0_IMIN_LINE)
uint64_t ctr_el0_usr __read_mostly;

static void
configure_cpu_traps0(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	uint64_t sctlr;
	uint64_t ctr_el0_raw = reg_ctr_el0_read();

#ifdef DEBUG_FORCE_TRAP_CTR_EL0
	goto need_ctr_trap;
#endif

	if ((__SHIFTOUT(ctr_el0_raw, CTR_EL0_DMIN_LINE) >
	     __SHIFTOUT(ctr_el0_usr, CTR_EL0_DMIN_LINE)) ||
	    (__SHIFTOUT(ctr_el0_raw, CTR_EL0_IMIN_LINE) >
	     __SHIFTOUT(ctr_el0_usr, CTR_EL0_IMIN_LINE)))
		goto need_ctr_trap;

	if ((__SHIFTOUT(ctr_el0_raw, CTR_EL0_DIC) == 1 &&
	     __SHIFTOUT(ctr_el0_usr, CTR_EL0_DIC) == 0) ||
	    (__SHIFTOUT(ctr_el0_raw, CTR_EL0_IDC) == 1 &&
	     __SHIFTOUT(ctr_el0_usr, CTR_EL0_IDC) == 0))
		goto need_ctr_trap;

#if 0 /* XXX: To do or not to do */
	/*
	 * IDC==0, but (LoC==0 || LoUIS==LoUU==0)?
	 * Would it be better to show IDC=1 to userland?
	 */
	if (__SHIFTOUT(ctr_el0_raw, CTR_EL0_IDC) == 0 &&
	    __SHIFTOUT(ctr_el0_usr, CTR_EL0_IDC) == 1)
		goto need_ctr_trap;
#endif

	return;

 need_ctr_trap:
	evcnt_attach_dynamic(&ci->ci_uct_trap, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "ctr_el0 trap");

	/* trap CTR_EL0 access from EL0 on this cpu */
	sctlr = reg_sctlr_el1_read();
	sctlr &= ~SCTLR_UCT;
	reg_sctlr_el1_write(sctlr);
}

void
configure_cpu_traps(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	uint64_t where;

	/* remember minimum cache line size out of all CPUs */
	for (CPU_INFO_FOREACH(cii, ci)) {
		uint64_t ctr_el0_cpu = ci->ci_id.ac_ctr;
		uint64_t clidr = ci->ci_id.ac_clidr;

		if (__SHIFTOUT(clidr, CLIDR_LOC) == 0 ||
		    (__SHIFTOUT(clidr, CLIDR_LOUIS) == 0 &&
		     __SHIFTOUT(clidr, CLIDR_LOUU) == 0)) {
			/* this means the same as IDC=1 */
			ctr_el0_cpu |= CTR_EL0_IDC;
		}

		/*
		 * if DIC==1, there is no need to icache sync. however,
		 * to calculate the minimum cacheline, in this case
		 * ICacheLine is treated as the maximum.
		 */
		if (__SHIFTOUT(ctr_el0_cpu, CTR_EL0_DIC) == 1)
			ctr_el0_cpu |= CTR_EL0_IMIN_LINE;

		/* Neoverse N1 erratum 1542419 */
		if (CPU_ID_NEOVERSEN1_P(ci->ci_id.ac_midr) &&
		    __SHIFTOUT(ctr_el0_cpu, CTR_EL0_DIC) == 1)
			ctr_el0_cpu &= ~CTR_EL0_DIC;

		if (cii == 0) {
			ctr_el0_usr = ctr_el0_cpu;
			continue;
		}

		/* keep minimum cache line size, and worst DIC/IDC */
		ctr_el0_usr &= (ctr_el0_cpu & CTR_EL0_DIC) | ~CTR_EL0_DIC;
		ctr_el0_usr &= (ctr_el0_cpu & CTR_EL0_IDC) | ~CTR_EL0_IDC;
		if (__SHIFTOUT(ctr_el0_cpu, CTR_EL0_DMIN_LINE) <
		    __SHIFTOUT(ctr_el0_usr, CTR_EL0_DMIN_LINE)) {
			ctr_el0_usr &= ~CTR_EL0_DMIN_LINE;
			ctr_el0_usr |= ctr_el0_cpu & CTR_EL0_DMIN_LINE;
		}
		if ((ctr_el0_cpu & CTR_EL0_DIC) == 0 &&
		    (__SHIFTOUT(ctr_el0_cpu, CTR_EL0_IMIN_LINE) <
		    __SHIFTOUT(ctr_el0_usr, CTR_EL0_IMIN_LINE))) {
			ctr_el0_usr &= ~CTR_EL0_IMIN_LINE;
			ctr_el0_usr |= ctr_el0_cpu & CTR_EL0_IMIN_LINE;
		}
	}

	where = xc_broadcast(0, configure_cpu_traps0, NULL, NULL);
	xc_wait(where);
}

static enum emul_arm_result
emul_aarch64_insn(struct trapframe *tf)
{
	uint32_t insn;

	if (ufetch_32((uint32_t *)tf->tf_pc, &insn)) {
		tf->tf_far = reg_far_el1_read();
		return EMUL_ARM_FAULT;
	}

	LE32TOH(insn);
	if ((insn & 0xffffffe0) == 0xd53b0020) {
		/* mrs x?,ctr_el0 */
		unsigned int Xt = insn & 31;
		if (Xt != 31) {	/* !xzr */
			uint64_t ctr_el0 = reg_ctr_el0_read();
			ctr_el0 &= ~CTR_EL0_USR_MASK;
			ctr_el0 |= (ctr_el0_usr & CTR_EL0_USR_MASK);
			tf->tf_reg[Xt] = ctr_el0;
		}
		curcpu()->ci_uct_trap.ev_count++;

	} else {
		return EMUL_ARM_UNKNOWN;
	}

	tf->tf_pc += 4;
	return EMUL_ARM_SUCCESS;
}

void
trap_el0_sync(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	const uint32_t esr = tf->tf_esr;
	const uint32_t eclass = __SHIFTOUT(esr, ESR_EC); /* exception class */

#ifdef DDB
	/* disable trace, and enable hardware breakpoint/watchpoint */
	reg_mdscr_el1_write(
	    (reg_mdscr_el1_read() & ~MDSCR_SS) | MDSCR_KDE);
#else
	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);
#endif
	/* enable traps and interrupts */
	daif_enable(DAIF_D|DAIF_A|DAIF_I|DAIF_F);

	switch (eclass) {
	case ESR_EC_INSN_ABT_EL0:
	case ESR_EC_DATA_ABT_EL0:
		data_abort_handler(tf, eclass);
		userret(l);
		break;

	case ESR_EC_SVC_A64:
		(*l->l_proc->p_md.md_syscall)(tf);
		break;
	case ESR_EC_FP_ACCESS:
		fpu_load(l);
		userret(l);
		break;
	case ESR_EC_FP_TRAP_A64:
		do_trapsignal(l, SIGFPE, FPE_FLTUND, NULL, esr); /* XXX */
		userret(l);
		break;

	case ESR_EC_PC_ALIGNMENT:
		do_trapsignal(l, SIGBUS, BUS_ADRALN, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	case ESR_EC_SP_ALIGNMENT:
		do_trapsignal(l, SIGBUS, BUS_ADRALN, (void *)tf->tf_sp, esr);
		userret(l);
		break;

	case ESR_EC_BKPT_INSN_A64:
	case ESR_EC_BRKPNT_EL0:
	case ESR_EC_WTCHPNT_EL0:
		do_trapsignal(l, SIGTRAP, TRAP_BRKPT, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	case ESR_EC_SW_STEP_EL0:
		/* disable trace, and send trace trap */
		tf->tf_spsr &= ~SPSR_SS;
		do_trapsignal(l, SIGTRAP, TRAP_TRACE, (void *)tf->tf_pc, esr);
		userret(l);
		break;

	case ESR_EC_SYS_REG:
		switch (emul_aarch64_insn(tf)) {
		case EMUL_ARM_SUCCESS:
			break;
		case EMUL_ARM_UNKNOWN:
			goto unknown;
		case EMUL_ARM_FAULT:
			do_trapsignal(l, SIGSEGV, SEGV_MAPERR,
			    (void *)tf->tf_far, esr);
			break;
		}
		userret(l);
		break;

	default:
	case ESR_EC_UNKNOWN:
 unknown:
#ifdef DDB
		if (sigill_debug) {
			/* show illegal instruction */
			printf("TRAP: pid %d (%s), uid %d: %s:"
			    " esr=0x%lx: pc=0x%lx: %s\n",
			    curlwp->l_proc->p_pid, curlwp->l_proc->p_comm,
			    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1,
			    eclass_trapname(eclass), tf->tf_esr, tf->tf_pc,
			    strdisasm(tf->tf_pc, tf->tf_spsr));
		}
#endif
		/* illegal or not implemented instruction */
		do_trapsignal(l, SIGILL, ILL_ILLTRP, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	}
}

void
cpu_irq(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();

#ifdef STACKCHECKS
	struct lwp *l = curlwp;
	void *sp = (void *)reg_sp_read();
	if (l->l_addr >= sp) {
		panic("lwp/interrupt stack overflow detected."
		    " lwp=%p, sp=%p, l_addr=%p", l, sp, l->l_addr);
	}
#endif

#ifdef DDB
	/* disable trace, and enable hardware breakpoint/watchpoint */
	reg_mdscr_el1_write(
	    (reg_mdscr_el1_read() & ~MDSCR_SS) | MDSCR_KDE);
#else
	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);
#endif

	/*
	 * Prevent preemption once we enable traps, until we have
	 * finished running hard and soft interrupt handlers.  This
	 * guarantees ci = curcpu() remains stable and we don't
	 * accidentally try to run its pending soft interrupts on
	 * another CPU.
	 */
	kpreempt_disable();

	/* enable traps */
	daif_enable(DAIF_D|DAIF_A);

	/* run hard interrupt handlers */
	ci->ci_intr_depth++;
	ARM_IRQ_HANDLER(tf);
	ci->ci_intr_depth--;

	/* run soft interrupt handlers */
	cpu_dosoftints();

	/* all done, preempt as you please */
	kpreempt_enable();
}

void
cpu_fiq(struct trapframe *tf)
{
	struct cpu_info * const ci = curcpu();

#ifdef STACKCHECKS
	struct lwp *l = curlwp;
	void *sp = (void *)reg_sp_read();
	if (l->l_addr >= sp) {
		panic("lwp/interrupt stack overflow detected."
		    " lwp=%p, sp=%p, l_addr=%p", l, sp, l->l_addr);
	}
#endif

	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);

	/*
	 * Prevent preemption once we enable traps, until we have
	 * finished running hard and soft interrupt handlers.  This
	 * guarantees ci = curcpu() remains stable and we don't
	 * accidentally try to run its pending soft interrupts on
	 * another CPU.
	 */
	kpreempt_disable();

	/* enable traps */
	daif_enable(DAIF_D|DAIF_A);

	/* run hard interrupt handlers */
	ci->ci_intr_depth++;
	ARM_FIQ_HANDLER(tf);
	ci->ci_intr_depth--;

	/* run soft interrupt handlers */
	cpu_dosoftints();

	/* all done, preempt as you please */
	kpreempt_enable();
}

#ifdef COMPAT_NETBSD32

/*
 * 32-bit length Thumb instruction. See ARMv7 DDI0406A A6.3.
 */
#define THUMB_32BIT(hi) (((hi) & 0xe000) == 0xe000 && ((hi) & 0x1800))

int
fetch_arm_insn(uint64_t pc, uint64_t spsr, uint32_t *insn)
{

	/*
	 * Instructions are stored in little endian for BE8,
	 * only a valid binary format for ILP32EB. Therefore,
	 * we need byte-swapping before decoding on aarch64eb.
	 */

	/* THUMB? */
	if (spsr & SPSR_A32_T) {
		uint16_t *p = (uint16_t *)(pc & ~1UL); /* XXX */
		uint16_t hi, lo;

		if (ufetch_16(p, &hi))
			return -1;
		LE16TOH(hi);

		if (!THUMB_32BIT(hi)) {
			/* 16-bit Thumb instruction */
			*insn = hi;
			return 2;
		}

		/* 32-bit Thumb instruction */
		if (ufetch_16(p + 1, &lo))
			return -1;
		LE16TOH(lo);

		*insn = ((uint32_t)hi << 16) | lo;
		return 4;
	}

	if (ufetch_32((uint32_t *)pc, insn))
		return -1;
	LE32TOH(*insn);

	return 4;
}

static bool
arm_cond_match(uint32_t insn, uint64_t spsr)
{
	bool invert = (insn >> 28) & 1;
	bool match;

	switch (insn >> 29) {
	case 0:	/* EQ or NE */
		match = spsr & SPSR_Z;
		break;
	case 1:	/* CS/HI or CC/LO */
		match = spsr & SPSR_C;
		break;
	case 2:	/* MI or PL */
		match = spsr & SPSR_N;
		break;
	case 3:	/* VS or VC */
		match = spsr & SPSR_V;
		break;
	case 4:	/* HI or LS */
		match = ((spsr & (SPSR_C | SPSR_Z)) == SPSR_C);
		break;
	case 5:	/* GE or LT */
		match = (!(spsr & SPSR_N) == !(spsr & SPSR_V));
		break;
	case 6:	/* GT or LE */
		match = !(spsr & SPSR_Z) &&
		    (!(spsr & SPSR_N) == !(spsr & SPSR_V));
		break;
	case 7:	/* AL */
		match = true;
		break;
	}
	return (!match != !invert);
}

uint8_t atomic_swap_8(volatile uint8_t *, uint8_t);

static int
emul_arm_swp(uint32_t insn, struct trapframe *tf)
{
	struct faultbuf fb;
	vaddr_t vaddr;
	uint32_t val;
	int Rn, Rd, Rm, error;

	Rn = __SHIFTOUT(insn, 0x000f0000);
	Rd = __SHIFTOUT(insn, 0x0000f000);
	Rm = __SHIFTOUT(insn, 0x0000000f);

	vaddr = tf->tf_reg[Rn] & 0xffffffff;
	val = tf->tf_reg[Rm];

	/* fault if insn is swp, and unaligned access */
	if ((insn & 0x00400000) == 0 && (vaddr & 3) != 0) {
		tf->tf_far = vaddr;
		return EFAULT;
	}

	/* vaddr will always point to userspace, since it has only 32bit */
	if ((error = cpu_set_onfault(&fb)) == 0) {
		if (aarch64_pan_enabled)
			reg_pan_write(0); /* disable PAN */
		if (insn & 0x00400000) {
			/* swpb */
			val = atomic_swap_8((uint8_t *)vaddr, val);
		} else {
			/* swp */
			val = atomic_swap_32((uint32_t *)vaddr, val);
		}
		cpu_unset_onfault();
		tf->tf_reg[Rd] = val;
	} else {
		tf->tf_far = reg_far_el1_read();
	}
	if (aarch64_pan_enabled)
		reg_pan_write(1); /* enable PAN */
	return error;
}

static enum emul_arm_result
emul_thumb_insn(struct trapframe *tf, uint32_t insn, int insn_size)
{
	/* T32-16bit or 32bit instructions */
	switch (insn_size) {
	case 2:
		/* Breakpoint used by GDB */
		if (insn == 0xdefe) {
			do_trapsignal(curlwp, SIGTRAP, TRAP_BRKPT,
			    (void *)tf->tf_pc, 0);
			return EMUL_ARM_SUCCESS;
		}
		/* XXX: some T32 IT instruction deprecated should be emulated */
		break;
	case 4:
		break;
	default:
		return EMUL_ARM_FAULT;
	}
	return EMUL_ARM_UNKNOWN;
}

static enum emul_arm_result
emul_arm_insn(struct trapframe *tf)
{
	uint32_t insn;
	int insn_size;

	insn_size = fetch_arm_insn(tf->tf_pc, tf->tf_spsr, &insn);
	tf->tf_far = reg_far_el1_read();

	if (tf->tf_spsr & SPSR_A32_T)
		return emul_thumb_insn(tf, insn, insn_size);
	if (insn_size != 4)
		return EMUL_ARM_FAULT;

	/* Breakpoint used by GDB */
	if (insn == 0xe6000011 || insn == 0xe7ffdefe) {
		do_trapsignal(curlwp, SIGTRAP, TRAP_BRKPT,
		    (void *)tf->tf_pc, 0);
		return EMUL_ARM_SUCCESS;
	}

	/* Unconditional instruction extension space? */
	if ((insn & 0xf0000000) == 0xf0000000)
		goto unknown_insn;

	/* swp,swpb */
	if ((insn & 0x0fb00ff0) == 0x01000090) {
		if (arm_cond_match(insn, tf->tf_spsr)) {
			if (emul_arm_swp(insn, tf) != 0)
				return EMUL_ARM_FAULT;
		}
		goto emulated;
	}

	/*
	 * Emulate ARMv6 instructions with cache operations
	 * register (c7), that can be used in user mode.
	 */
	switch (insn & 0x0fff0fff) {
	case 0x0e070f95:
		if (arm_cond_match(insn, tf->tf_spsr)) {
			/*
			 * mcr p15, 0, <Rd>, c7, c5, 4
			 * (flush prefetch buffer)
			 */
			isb();
		}
		goto emulated;
	case 0x0e070f9a:
		if (arm_cond_match(insn, tf->tf_spsr)) {
			/*
			 * mcr p15, 0, <Rd>, c7, c10, 4
			 * (data synchronization barrier)
			 */
			dsb(sy);
		}
		goto emulated;
	case 0x0e070fba:
		if (arm_cond_match(insn, tf->tf_spsr)) {
			/*
			 * mcr p15, 0, <Rd>, c7, c10, 5
			 * (data memory barrier)
			 */
			dmb(sy);
		}
		goto emulated;
	default:
		break;
	}

 unknown_insn:
	/* unknown, or unsupported instruction */
	return EMUL_ARM_UNKNOWN;

 emulated:
	tf->tf_pc += insn_size;
	return EMUL_ARM_SUCCESS;
}
#endif /* COMPAT_NETBSD32 */

void
trap_el0_32sync(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	const uint32_t esr = tf->tf_esr;
	const uint32_t eclass = __SHIFTOUT(esr, ESR_EC); /* exception class */

#ifdef DDB
	/* disable trace, and enable hardware breakpoint/watchpoint */
	reg_mdscr_el1_write(
	    (reg_mdscr_el1_read() & ~MDSCR_SS) | MDSCR_KDE);
#else
	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);
#endif
	/* enable traps and interrupts */
	daif_enable(DAIF_D|DAIF_A|DAIF_I|DAIF_F);

	switch (eclass) {
#ifdef COMPAT_NETBSD32
	case ESR_EC_INSN_ABT_EL0:
	case ESR_EC_DATA_ABT_EL0:
		data_abort_handler(tf, eclass);
		userret(l);
		break;

	case ESR_EC_SVC_A32:
		(*l->l_proc->p_md.md_syscall)(tf);
		break;

	case ESR_EC_FP_ACCESS:
		fpu_load(l);
		userret(l);
		break;

	case ESR_EC_FP_TRAP_A32:
		do_trapsignal(l, SIGFPE, FPE_FLTUND, NULL, esr); /* XXX */
		userret(l);
		break;

	case ESR_EC_PC_ALIGNMENT:
		do_trapsignal(l, SIGBUS, BUS_ADRALN, (void *)tf->tf_pc, esr);
		userret(l);
		break;

	case ESR_EC_SP_ALIGNMENT:
		do_trapsignal(l, SIGBUS, BUS_ADRALN,
		    (void *)tf->tf_reg[13], esr); /* sp is r13 on AArch32 */
		userret(l);
		break;

	case ESR_EC_BKPT_INSN_A32:
		do_trapsignal(l, SIGTRAP, TRAP_BRKPT, (void *)tf->tf_pc, esr);
		userret(l);
		break;

	case ESR_EC_UNKNOWN:
		switch (emul_arm_insn(tf)) {
		case EMUL_ARM_SUCCESS:
			break;
		case EMUL_ARM_UNKNOWN:
			goto unknown;
		case EMUL_ARM_FAULT:
			do_trapsignal(l, SIGSEGV, SEGV_MAPERR,
			    (void *)tf->tf_far, esr);
			break;
		}
		userret(l);
		break;

	case ESR_EC_CP15_RT:
	case ESR_EC_CP15_RRT:
	case ESR_EC_CP14_RT:
	case ESR_EC_CP14_DT:
	case ESR_EC_CP14_RRT:
unknown:
#endif /* COMPAT_NETBSD32 */
	default:
#ifdef DDB
		if (sigill_debug) {
			/* show illegal instruction */
			printf("TRAP: pid %d (%s), uid %d: %s:"
			    " esr=0x%lx: pc=0x%lx: %s\n",
			    curlwp->l_proc->p_pid, curlwp->l_proc->p_comm,
			    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1,
			    eclass_trapname(eclass), tf->tf_esr, tf->tf_pc,
			    strdisasm(tf->tf_pc, tf->tf_spsr));
		}
#endif
		/* illegal or not implemented instruction */
		do_trapsignal(l, SIGILL, ILL_ILLTRP, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	}
}

void
trap_el1h_error(struct trapframe *tf)
{
	/*
	 * Normally, we should panic unconditionally,
	 * but SError interrupt may occur when accessing to unmapped(?) I/O
	 * spaces. bus_space_{peek,poke}_{1,2,4,8}() should trap these case.
	 */
	struct faultbuf *fb;

	if (curcpu()->ci_intr_depth == 0) {
		fb = cpu_disable_onfault();
		if (fb != NULL) {
			cpu_jump_onfault(tf, fb, EFAULT);
			return;
		}
	}
	panic("%s", __func__);
}

#define bad_trap_panic(trapfunc)	\
void					\
trapfunc(struct trapframe *tf)		\
{					\
	panic("%s", __func__);		\
}
bad_trap_panic(trap_el1t_sync)
bad_trap_panic(trap_el1t_irq)
bad_trap_panic(trap_el1t_fiq)
bad_trap_panic(trap_el1t_error)
bad_trap_panic(trap_el1h_fiq)
bad_trap_panic(trap_el0_fiq)
bad_trap_panic(trap_el0_error)
bad_trap_panic(trap_el0_32fiq)
bad_trap_panic(trap_el0_32error)

void
cpu_jump_onfault(struct trapframe *tf, const struct faultbuf *fb, int val)
{
	tf->tf_reg[19] = fb->fb_reg[FB_X19];
	tf->tf_reg[20] = fb->fb_reg[FB_X20];
	tf->tf_reg[21] = fb->fb_reg[FB_X21];
	tf->tf_reg[22] = fb->fb_reg[FB_X22];
	tf->tf_reg[23] = fb->fb_reg[FB_X23];
	tf->tf_reg[24] = fb->fb_reg[FB_X24];
	tf->tf_reg[25] = fb->fb_reg[FB_X25];
	tf->tf_reg[26] = fb->fb_reg[FB_X26];
	tf->tf_reg[27] = fb->fb_reg[FB_X27];
	tf->tf_reg[28] = fb->fb_reg[FB_X28];
	tf->tf_reg[29] = fb->fb_reg[FB_X29];
	tf->tf_sp = fb->fb_reg[FB_SP];
	tf->tf_pc = fb->fb_reg[FB_LR];
	tf->tf_reg[0] = val;
}

#ifdef TRAP_SIGDEBUG
static void
frame_dump(const struct trapframe *tf)
{
	const struct reg *r = &tf->tf_regs;

	printf("trapframe %p\n", tf);
	for (size_t i = 0; i < __arraycount(r->r_reg); i++) {
		printf(" r%.2zu %#018" PRIx64 "%c", i, r->r_reg[i],
		    " \n"[i && (i & 1) == 0]);
	}

	printf("\n");
	printf("   sp %#018" PRIx64 "    pc %#018" PRIx64 "\n",
	    r->r_sp, r->r_pc);
	printf(" spsr %#018" PRIx64 " tpidr %#018" PRIx64 "\n",
	    r->r_spsr, r->r_tpidr);
	printf("  esr %#018" PRIx64 "   far %#018" PRIx64 "\n",
	    tf->tf_esr, tf->tf_far);

	printf("\n");
	hexdump(printf, "Stack dump", tf, 256);
}

static void
sigdebug(const struct trapframe *tf, const ksiginfo_t *ksi)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	const uint32_t eclass = __SHIFTOUT(ksi->ksi_trap, ESR_EC);

	printf("pid %d.%d (%s): signal %d (trap %#x) "
	    "@pc %#" PRIx64 ", addr %p, error=%s\n",
	    p->p_pid, l->l_lid, p->p_comm, ksi->ksi_signo, ksi->ksi_trap,
	    tf->tf_regs.r_pc, ksi->ksi_addr, eclass_trapname(eclass));
	frame_dump(tf);
}
#endif

void
do_trapsignal1(
#ifdef TRAP_SIGDEBUG
    const char *func,
    size_t line,
    struct trapframe *tf,
#endif
    struct lwp *l, int signo, int code, void *addr, int trap)
{
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = signo;
	ksi.ksi_code = code;
	ksi.ksi_addr = addr;
	ksi.ksi_trap = trap;
#ifdef TRAP_SIGDEBUG
	printf("%s, %zu: ", func, line);
	sigdebug(tf, &ksi);
#endif
	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
}

bool
cpu_intr_p(void)
{
	uint64_t ncsw;
	int idepth;
	lwp_t *l;

#ifdef __HAVE_PIC_FAST_SOFTINTS
	/* XXX Copied from cpu.h.  Looks incomplete - needs fixing. */
	if (ci->ci_cpl < IPL_VM)
		return false;
#endif

	l = curlwp;
	if (__predict_false(l->l_cpu == NULL)) {
		KASSERT(l == &lwp0);
		return false;
	}
	do {
		ncsw = l->l_ncsw;
		__insn_barrier();
		idepth = l->l_cpu->ci_intr_depth;
		__insn_barrier();
	} while (__predict_false(ncsw != l->l_ncsw));

	return idepth > 0;
}
