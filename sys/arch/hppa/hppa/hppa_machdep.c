/*	$NetBSD: hppa_machdep.c,v 1.15.8.1 2009/05/13 17:17:47 jym Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: hppa_machdep.c,v 1.15.8.1 2009/05/13 17:17:47 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sa.h>
#include <sys/lwp.h>
#include <sys/savar.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/cpu.h>

#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/mcontext.h>
#include <hppa/hppa/machdep.h>

/* the following is used externally (sysctl_hw) */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/*
 * XXX fredette - much of the TLB trap handler setup should
 * probably be moved here from hp700/hp700/machdep.c, seeing
 * that there's related code already in hppa/hppa/trap.S.
 */


/*
 * Scheduler activations upcall frame.  Pushed onto user stack before
 * calling an SA upcall.
 */

struct saframe {
	/* first 4 arguments passed in registers on entry to upcallcode */
	void *		sa_arg;
	int		sa_interrupted;	/* arg3 */
	int		sa_events;	/* arg2 */
	struct sa_t **	sa_sas;		/* arg1 */
	int		sa_type;	/* arg0 */
};

/*
 * cpu_upcall:
 *
 *      Send an an upcall to userland.
 */

void
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
	   void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct saframe *sf, frame;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	uintptr_t upva;
	vaddr_t va;

	tf = (struct trapframe *)l->l_md.md_regs;

	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
	frame.sa_arg = ap;

	pmap_activate(l);
	va = HPPA_FRAME_ROUND((uintptr_t)sp + sizeof(frame) + HPPA_FRAME_SIZE);
	sf = (void *)(va - 32 - sizeof(frame));
	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work. Die. */
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Deal with the upcall function pointer being a PLABEL.
	 */

	upva = (uintptr_t)upcall;
	if (upva & 2) {
		upva &= ~3;
		if (copyin((void *)(upva + 4), &tf->tf_t4, 4)) {
			printf("copyin t4 failed\n");
			mutex_enter(p->p_lock);
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		if (copyin((void *)upva, &upcall, 4)) {
			printf("copyin upcall failed\n");
			mutex_enter(p->p_lock);
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	}

	tf->tf_iioq_head = (uintptr_t)upcall | HPPA_PC_PRIV_USER;
	tf->tf_iioq_tail = tf->tf_iioq_head + 4;

	tf->tf_sp = va;
	tf->tf_arg0 = type;
	tf->tf_arg1 = (uintptr_t)sas;
	tf->tf_arg2 = nevents;
	tf->tf_arg3 = ninterrupted;
	tf->tf_rp = 0;
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	gr[0]  = tf->tf_ipsw;
	gr[1]  = tf->tf_r1;
	gr[2]  = tf->tf_rp;
	gr[3]  = tf->tf_r3;
	gr[4]  = tf->tf_r4;
	gr[5]  = tf->tf_r5;
	gr[6]  = tf->tf_r6;
	gr[7]  = tf->tf_r7;
	gr[8]  = tf->tf_r8;
	gr[9]  = tf->tf_r9;
	gr[10] = tf->tf_r10;
	gr[11] = tf->tf_r11;
	gr[12] = tf->tf_r12;
	gr[13] = tf->tf_r13;
	gr[14] = tf->tf_r14;
	gr[15] = tf->tf_r15;
	gr[16] = tf->tf_r16;
	gr[17] = tf->tf_r17;
	gr[18] = tf->tf_r18;
	gr[19] = tf->tf_t4;
	gr[20] = tf->tf_t3;
	gr[21] = tf->tf_t2;
	gr[22] = tf->tf_t1;
	gr[23] = tf->tf_arg3;
	gr[24] = tf->tf_arg2;
	gr[25] = tf->tf_arg1;
	gr[26] = tf->tf_arg0;
	gr[27] = tf->tf_dp;
	gr[28] = tf->tf_ret0;
	gr[29] = tf->tf_ret1;
	gr[30] = tf->tf_sp;
	gr[31] = tf->tf_r31;

	gr[_REG_SAR] = tf->tf_sar;
	gr[_REG_PCSQH] = tf->tf_iisq_head;
	gr[_REG_PCSQT] = tf->tf_iisq_tail;
	gr[_REG_PCOQH] = tf->tf_iioq_head;
	gr[_REG_PCOQT] = tf->tf_iioq_tail;
	gr[_REG_SR0] = tf->tf_sr0;
	gr[_REG_SR1] = tf->tf_sr1;
	gr[_REG_SR2] = tf->tf_sr2;
	gr[_REG_SR3] = tf->tf_sr3;
	gr[_REG_SR4] = tf->tf_sr4;
#if 0
	gr[_REG_CR26] = tf->tf_cr26;
	gr[_REG_CR27] = tf->tf_cr27;
#endif

	ras_pc = (__greg_t)ras_lookup(l->l_proc,
	    (void *)(gr[_REG_PCOQH] & ~HPPA_PC_PRIV_MASK));
	if (ras_pc != -1) {
		ras_pc |= HPPA_PC_PRIV_USER;
		gr[_REG_PCOQH] = ras_pc;
		gr[_REG_PCOQT] = ras_pc + 4;
	}

	*flags |= _UC_CPU;

	if (l->l_md.md_flags & 0) {
		return;
	}

	hppa_fpu_flush(l);
	memcpy(&mcp->__fpregs, l->l_addr->u_pcb.pcb_fpregs,
	       sizeof(mcp->__fpregs));
	fdcache(HPPA_SID_KERNEL, (vaddr_t)l->l_addr->u_pcb.pcb_fpregs,
		sizeof(l->l_addr->u_pcb.pcb_fpregs));
	*flags |= _UC_FPU;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	struct proc *p = l->l_proc;
	struct pmap *pmap = p->p_vmspace->vm_map.pmap;
	const __greg_t *gr = mcp->__gregs;

	if ((flags & _UC_CPU) != 0) {

		if ((gr[_REG_PSW] & (PSW_MBS|PSW_MBZ)) != PSW_MBS) {
			return EINVAL;
		}

#if 0
		/*
		 * XXX
		 * Force the space regs and priviledge bits to
		 * the right values in the trapframe for now.
		 */

		if (gr[_REG_PCSQH] != pmap_sid(pmap, gr[_REG_PCOQH])) {
			return EINVAL;
		}

		if (gr[_REG_PCSQT] != pmap_sid(pmap, gr[_REG_PCOQT])) {
			return EINVAL;
		}

		if (gr[_REG_PCOQH] < 0xc0000020 &&
		    (gr[_REG_PCOQH] & HPPA_PC_PRIV_MASK) != HPPA_PC_PRIV_USER) {
			return EINVAL;
		}

		if (gr[_REG_PCOQT] < 0xc0000020 &&
		    (gr[_REG_PCOQT] & HPPA_PC_PRIV_MASK) != HPPA_PC_PRIV_USER) {
			return EINVAL;
		}
#endif

		tf->tf_ipsw	= gr[0];
		tf->tf_r1	= gr[1];
		tf->tf_rp	= gr[2];
		tf->tf_r3	= gr[3];
		tf->tf_r4	= gr[4];
		tf->tf_r5	= gr[5];
		tf->tf_r6	= gr[6];
		tf->tf_r7	= gr[7];
		tf->tf_r8	= gr[8];
		tf->tf_r9	= gr[9];
		tf->tf_r10	= gr[10];
		tf->tf_r11	= gr[11];
		tf->tf_r12	= gr[12];
		tf->tf_r13	= gr[13];
		tf->tf_r14	= gr[14];
		tf->tf_r15	= gr[15];
		tf->tf_r16	= gr[16];
		tf->tf_r17	= gr[17];
		tf->tf_r18	= gr[18];
		tf->tf_t4	= gr[19];
		tf->tf_t3	= gr[20];
		tf->tf_t2	= gr[21];
		tf->tf_t1	= gr[22];
		tf->tf_arg3	= gr[23];
		tf->tf_arg2	= gr[24];
		tf->tf_arg1	= gr[25];
		tf->tf_arg0	= gr[26];
		tf->tf_dp	= gr[27];
		tf->tf_ret0	= gr[28];
		tf->tf_ret1	= gr[29];
		tf->tf_sp	= gr[30];
		tf->tf_r31	= gr[31];
		tf->tf_sar	= gr[_REG_SAR];
		tf->tf_iisq_head = pmap_sid(pmap, gr[_REG_PCOQH]);
		tf->tf_iisq_tail = pmap_sid(pmap, gr[_REG_PCOQT]);

		tf->tf_iioq_head = gr[_REG_PCOQH];
		tf->tf_iioq_tail = gr[_REG_PCOQT];

		if (tf->tf_iioq_head >= 0xc0000020) {
			tf->tf_iioq_head &= ~HPPA_PC_PRIV_MASK;
		} else {
			tf->tf_iioq_head |= HPPA_PC_PRIV_USER;
		}
		if (tf->tf_iioq_tail >= 0xc0000020) {
			tf->tf_iioq_tail &= ~HPPA_PC_PRIV_MASK;
		} else {
			tf->tf_iioq_tail |= HPPA_PC_PRIV_USER;
		}

#if 0
		tf->tf_sr0	= gr[_REG_SR0];
		tf->tf_sr1	= gr[_REG_SR1];
		tf->tf_sr2	= gr[_REG_SR2];
		tf->tf_sr3	= gr[_REG_SR3];
		tf->tf_sr4	= gr[_REG_SR4];
		tf->tf_cr26	= gr[_REG_CR26];
		tf->tf_cr27	= gr[_REG_CR27];
#endif
	}

	if ((flags & _UC_FPU) != 0) {
		hppa_fpu_flush(l);
		memcpy(l->l_addr->u_pcb.pcb_fpregs, &mcp->__fpregs,
		       sizeof(mcp->__fpregs));
		fdcache(HPPA_SID_KERNEL, (vaddr_t)l->l_addr->u_pcb.pcb_fpregs,
			sizeof(l->l_addr->u_pcb.pcb_fpregs));
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return 0;
}

/*
 * Do RAS processing.
 */

void
hppa_ras(struct lwp *l)
{
	struct proc *p;
	struct trapframe *tf;
	intptr_t rasaddr;

	p = l->l_proc;
	tf = l->l_md.md_regs;
	rasaddr = (intptr_t)ras_lookup(p, (void *)tf->tf_iioq_head);
	if (rasaddr != -1) {
		rasaddr |= HPPA_PC_PRIV_USER;
		tf->tf_iioq_head = rasaddr;
		tf->tf_iioq_tail = rasaddr + 4;
	}
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	bool immed = (flags & RESCHED_IMMED) != 0;

	if (ci->ci_want_resched && !immed)
		return;
	ci->ci_want_resched = 1;
#ifdef MULTIPROCESSOR
        if (ci->ci_curlwp != ci->ci_data.cpu_idlelwp) {
		/* aston(ci->ci_curlwp); */
		setsoftast();
	}
#endif
}
