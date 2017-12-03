/*	$NetBSD: sig_machdep.c,v 1.43.2.1 2017/12/03 11:36:38 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.43.2.1 2017/12/03 11:36:38 jdolecek Exp $");

#include "opt_ppcarch.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/ucontext.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <powerpc/fpu.h>
#include <powerpc/altivec.h>
#include <powerpc/pcb.h>
#include <powerpc/psl.h>

/*
 * Send a signal to process.
 */
void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct sigaltstack * const ss = &l->l_sigstk;
	const struct sigact_sigdesc * const sd =
	    &p->p_sigacts->sa_sigdesc[ksi->ksi_signo];
	/* save handler before sendsig_reset trashes it! */
	const void * const handler = sd->sd_sigact.sa_handler;
	ucontext_t uc;
	vaddr_t sp, sip, ucp;
	int onstack, error;

	/* Do we need to jump onto the signal stack? */
	onstack = (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (sd->sd_sigact.sa_flags & SA_ONSTACK) != 0;

	/* Find top of stack.  */
	sp = (onstack ? (vaddr_t)ss->ss_sp + ss->ss_size : tf->tf_fixreg[1]);
	sp &= ~(CALLFRAMELEN-1);

	/* Allocate space for the ucontext.  */
	sp -= sizeof(ucontext_t);
	ucp = sp;

	/* Allocate space for the siginfo.  */
	sp -= sizeof(siginfo_t);
	sip = sp;

	sp &= ~(CALLFRAMELEN-1);

	/* Save register context. */
	uc.uc_flags = _UC_SIGMASK;
	uc.uc_sigmask = *mask;
	uc.uc_link = l->l_ctxlink;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));
	sendsig_reset(l, ksi->ksi_signo);
	mutex_exit(p->p_lock);
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);

	/*
	 * Copy the siginfo and ucontext onto the user's stack.
	 */
	error = (copyout(&ksi->ksi_info, (void *)sip, sizeof(ksi->ksi_info)) != 0 ||
	    copyout(&uc, (void *)ucp, sizeof(uc)) != 0);
	mutex_enter(p->p_lock);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.  Note the trampoline version
	 * numbers are coordinated with machine-dependent code in libc.
	 */
	switch (sd->sd_vers) {
	case 2:		/* siginfo sigtramp */
		tf->tf_fixreg[1]  = (register_t)sp - CALLFRAMELEN;
		tf->tf_fixreg[3]  = (register_t)ksi->ksi_signo;
		tf->tf_fixreg[4]  = (register_t)sip;
		tf->tf_fixreg[5]  = (register_t)ucp;
		/* Preserve ucp across call to signal function */
		tf->tf_fixreg[30] = (register_t)ucp;
		tf->tf_lr         = (register_t)sd->sd_tramp;
		tf->tf_srr0       = (register_t)handler;
		break;

	default:
		goto nosupport;
	}

	/* Remember that we're now on the signal stack. */
	if (onstack)
		ss->ss_flags |= SS_ONSTACK;
	return;

 nosupport:
	/* Don't know what trampoline version; kill it. */
	printf("sendsig_siginfo(sig %d): bad version %d\n",
	    ksi->ksi_signo, sd->sd_vers);
	sigexit(l, SIGILL);
	/* NOTREACHED */
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flagp)
{
	const struct trapframe * const tf = l->l_md.md_utf;
	__greg_t * const gr = mcp->__gregs;
#if defined(PPC_HAVE_FPU)
	struct pcb * const pcb = lwp_getpcb(l);
#endif

	/* Save GPR context. */
	(void)memcpy(gr, &tf->tf_fixreg, 32 * sizeof (gr[0])); /* GR0-31 */
	gr[_REG_CR]  = tf->tf_cr;
	gr[_REG_LR]  = tf->tf_lr;
	gr[_REG_PC]  = tf->tf_srr0;
	gr[_REG_MSR] = tf->tf_srr1 & PSL_USERSRR1;
#ifdef PPC_HAVE_FPU
	gr[_REG_MSR] |= pcb->pcb_flags & (PCB_FE0|PCB_FE1);
#endif
	gr[_REG_CTR] = tf->tf_ctr;
	gr[_REG_XER] = tf->tf_xer;
#ifdef PPC_OEA
	gr[_REG_MQ]  = tf->tf_mq;
#else
	gr[_REG_MQ]  = 0;
#endif

	*flagp |= _UC_CPU;
	*flagp |= _UC_TLSBASE;

#ifdef PPC_HAVE_FPU
	/* Save FPU context, if any. */
	if (!fpu_save_to_mcontext(l, mcp, flagp))
#endif
		memset(&mcp->__fpregs, 0, sizeof(mcp->__fpregs));

#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	/* Save vector context, if any. */
	if (!vec_save_to_mcontext(l, mcp, flagp))
#endif
		memset(&mcp->__vrf, 0, sizeof (mcp->__vrf));
}

int
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mcp)
{
	return 0;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	const __greg_t * const gr = mcp->__gregs;
	int error;

	/* Restore GPR context, if any. */
	if (flags & _UC_CPU) {
		error = cpu_mcontext_validate(l, mcp);
		if (error)
			return error;

#ifdef PPC_HAVE_FPU
		/*
		 * Always save the FP exception mode in the PCB.
		 */
		struct pcb * const pcb = lwp_getpcb(l);
		pcb->pcb_flags &= ~(PCB_FE0|PCB_FE1);
		pcb->pcb_flags |= gr[_REG_MSR] & (PCB_FE0|PCB_FE1);
#endif

		/*
		 * R2 is the TLS register so avoid updating it here.
		 */

		__greg_t save_r2 = tf->tf_fixreg[_REG_R2];
		(void)memcpy(&tf->tf_fixreg, gr, 32 * sizeof (gr[0]));
		tf->tf_fixreg[_REG_R2] = save_r2;
		tf->tf_cr   = gr[_REG_CR];
		tf->tf_lr   = gr[_REG_LR];
		tf->tf_srr0 = gr[_REG_PC];

		/*
		 * Accept all user-settable bits without complaint;
		 * userland should not need to know the machine-specific
		 * MSR value.
		 */
		tf->tf_srr1 = (gr[_REG_MSR] & PSL_USERMOD) | PSL_USERSET;
		tf->tf_ctr  = gr[_REG_CTR];
		tf->tf_xer  = gr[_REG_XER];
#ifdef PPC_OEA
		tf->tf_mq = gr[_REG_MQ];
#endif
	}

	if (flags & _UC_TLSBASE)
		lwp_setprivate(l, (void *)(uintptr_t)gr[_REG_R2]);

#ifdef PPC_HAVE_FPU
	/* Restore FPU context, if any. */
	if (flags & _UC_FPU)
		fpu_restore_from_mcontext(l, mcp);
#endif

#ifdef ALTIVEC
	/* Restore AltiVec context, if any. */
	if (flags & _UC_POWERPC_VEC)
		vec_restore_from_mcontext(l, mcp);
#endif

#ifdef PPC_HAVE_SPE
	/* Restore SPE context, if any. */
	if (flags & _UC_POWERPC_SPE)
		vec_restore_from_mcontext(l, mcp);
#endif

	return (0);
}

int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{
	struct trapframe * const tf = l->l_md.md_utf;

	tf->tf_fixreg[_REG_R2] = (register_t)addr;
	return 0;
}
