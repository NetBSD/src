/* $NetBSD: trap.c,v 1.17.4.1 2019/08/07 10:19:54 martin Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: trap.c,v 1.17.4.1 2019/08/07 10:19:54 martin Exp $");

#include "opt_arm_intr_impl.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/kauth.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#ifdef KDB
#include <sys/kdb.h>
#endif
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>

#ifdef ARM_INTR_IMPL
#include ARM_INTR_IMPL
#else
#error ARM_INTR_IMPL not defined
#endif

#ifndef ARM_IRQ_HANDLER
#error ARM_IRQ_HANDLER not defined
#endif

#include <aarch64/userret.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/locore.h>

#ifdef KDB
#include <machine/db_machdep.h>
#endif
#ifdef DDB
#include <ddb/db_output.h>
#include <machine/db_machdep.h>
#endif

#ifdef DDB
int sigill_debug = 0;
#endif

const char * const trap_names[] = {
	[ESR_EC_UNKNOWN]	= "Unknown Reason (Illegal Instruction)",
	[ESR_EC_SERROR]		= "SError Interrupt",
	[ESR_EC_WFX]		= "WFI or WFE instruction execution",
	[ESR_EC_ILL_STATE]	= "Illegal Execution State",

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
	const int want_resched = ci->ci_want_resched;
#ifdef __HAVE_PREEMPTION
	kpreempt_enable();
#endif

	if (l->l_pflag & LP_OWEUPC) {
		l->l_pflag &= ~LP_OWEUPC;
		ADDUPROF(l);
	}

	/* Allow a forced task switch. */
	if (want_resched)
		preempt();
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

	switch (eclass) {
	case ESR_EC_INSN_ABT_EL1:
	case ESR_EC_DATA_ABT_EL1:
		data_abort_handler(tf, eclass);
		break;

	case ESR_EC_BRKPNT_EL1:
	case ESR_EC_SW_STEP_EL1:
	case ESR_EC_WTCHPNT_EL1:
	case ESR_EC_BKPT_INSN_A64:
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
	case ESR_EC_FP_TRAP_A64:
	case ESR_EC_PC_ALIGNMENT:
	case ESR_EC_SP_ALIGNMENT:
	case ESR_EC_ILL_STATE:
	default:
		panic("Trap: fatal %s: pc=%016" PRIx64 " sp=%016" PRIx64
		    " esr=%08x", eclass_trapname(eclass), tf->tf_pc, tf->tf_sp,
		    esr);
		break;
	}
}

void
trap_el0_sync(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	const uint32_t esr = tf->tf_esr;
	const uint32_t eclass = __SHIFTOUT(esr, ESR_EC); /* exception class */

	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);
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

	default:
	case ESR_EC_UNKNOWN:
#ifdef DDB
		if (sigill_debug) {
			/* show illegal instruction */
			printf("TRAP: pid %d (%s), uid %d: %s:"
			    " esr=0x%lx: pc=0x%lx: %s\n",
			    curlwp->l_proc->p_pid, curlwp->l_proc->p_comm,
			    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1,
			    eclass_trapname(eclass), tf->tf_esr, tf->tf_pc,
			    strdisasm(tf->tf_pc));
		}
#endif
		/* illegal or not implemented instruction */
		do_trapsignal(l, SIGILL, ILL_ILLTRP, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	}
}

void
interrupt(struct trapframe *tf)
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

	/* enable traps */
	daif_enable(DAIF_D|DAIF_A);

	ci->ci_intr_depth++;
	ARM_IRQ_HANDLER(tf);
	ci->ci_intr_depth--;

	cpu_dosoftints();
}

void
trap_el0_32sync(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	const uint32_t esr = tf->tf_esr;
	const uint32_t eclass = __SHIFTOUT(esr, ESR_EC); /* exception class */

	/* disable trace */
	reg_mdscr_el1_write(reg_mdscr_el1_read() & ~MDSCR_SS);
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

	case ESR_EC_CP15_RT:
	case ESR_EC_CP15_RRT:
	case ESR_EC_CP14_RT:
	case ESR_EC_CP14_DT:
	case ESR_EC_CP14_RRT:
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
			    strdisasm_aarch32(tf->tf_pc));
		}
#endif
		/* illegal or not implemented instruction */
		do_trapsignal(l, SIGILL, ILL_ILLTRP, (void *)tf->tf_pc, esr);
		userret(l);
		break;
	}
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
bad_trap_panic(trap_el1h_error)
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

void do_trapsignal1(
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
