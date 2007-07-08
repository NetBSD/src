/*	$NetBSD: sig_machdep.c,v 1.30 2007/07/08 10:19:23 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.30 2007/07/08 10:19:23 pooka Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ppcarch.h"
#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/ucontext.h>
#include <sys/user.h>

#include <powerpc/fpu.h>
#include <powerpc/altivec.h>

/*
 * Send a signal to process.
 */
void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = trapframe(l);
	struct sigaltstack *ss = &l->l_sigstk;
	const struct sigact_sigdesc *sd =
	    &p->p_sigacts->sa_sigdesc[ksi->ksi_signo];
	ucontext_t uc;
	vaddr_t sp, sip, ucp;
	int onstack, error;

	if (sd->sd_vers < 2) {
#ifdef COMPAT_16
		sendsig_sigcontext(ksi->ksi_signo, mask, KSI_TRAPCODE(ksi));
		return;
#else
		goto nosupport;
#endif
	}

	/* Do we need to jump onto the signal stack? */
	onstack = (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (sd->sd_sigact.sa_flags & SA_ONSTACK) != 0;

	/* Find top of stack.  */
	sp = (onstack ? (vaddr_t)ss->ss_sp + ss->ss_size : tf->fixreg[1]);
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
	mutex_exit(&p->p_smutex);
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);

	/*
	 * Copy the siginfo and ucontext onto the user's stack.
	 */
	error = (copyout(&ksi->ksi_info, (void *)sip, sizeof(ksi->ksi_info)) != 0 ||
	    copyout(&uc, (void *)ucp, sizeof(uc)) != 0);
	mutex_enter(&p->p_smutex);

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
		tf->fixreg[1]  = (register_t)sp - CALLFRAMELEN;
		tf->fixreg[3]  = (register_t)ksi->ksi_signo;
		tf->fixreg[4]  = (register_t)sip;
		tf->fixreg[5]  = (register_t)ucp;
		/* Preserve ucp across call to signal function */
		tf->fixreg[30] = (register_t)ucp;
		tf->lr         = (register_t)sd->sd_tramp;
		tf->srr0       = (register_t)sd->sd_sigact.sa_handler;
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
	const struct trapframe *tf = trapframe(l);
	__greg_t *gr = mcp->__gregs;
#if defined(PPC_HAVE_FPU) || defined(ALTIVEC)
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Save GPR context. */
	(void)memcpy(gr, &tf->fixreg, 32 * sizeof (gr[0])); /* GR0-31 */
	gr[_REG_CR]  = tf->cr;
	gr[_REG_LR]  = tf->lr;
	gr[_REG_PC]  = tf->srr0;
	gr[_REG_MSR] = tf->srr1 & PSL_USERSRR1;
#ifdef PPC_HAVE_FPU
	gr[_REG_MSR] |= pcb->pcb_flags & (PCB_FE0|PCB_FE1);
#endif
#ifdef ALTIVEC
	gr[_REG_MSR] |= pcb->pcb_flags & PCB_ALTIVEC ? PSL_VEC : 0;
#endif
	gr[_REG_CTR] = tf->ctr;
	gr[_REG_XER] = tf->xer;
#ifdef PPC_OEA
	gr[_REG_MQ]  = tf->tf_xtra[TF_MQ];
#else
	gr[_REG_MQ]  = 0;
#endif
	*flagp |= _UC_CPU;

#ifdef PPC_HAVE_FPU
	/* Save FPR context, if any. */
	if ((pcb->pcb_flags & PCB_FPU) != 0) {
		/* If we're the FPU owner, dump its context to the PCB first. */
		if (pcb->pcb_fpcpu)
			save_fpu_lwp(l, FPU_SAVE);
		(void)memcpy(mcp->__fpregs.__fpu_regs, pcb->pcb_fpu.fpreg,
		    sizeof (mcp->__fpregs.__fpu_regs));
		mcp->__fpregs.__fpu_fpscr =
		    ((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD];
		mcp->__fpregs.__fpu_valid = 1;
		*flagp |= _UC_FPU;
	} else
#endif
		memset(&mcp->__fpregs, 0, sizeof(mcp->__fpregs));

#ifdef ALTIVEC
	/* Save AltiVec context, if any. */
	if ((pcb->pcb_flags & PCB_ALTIVEC) != 0) {
		/*
		 * If we're the AltiVec owner, dump its context
		 * to the PCB first.
		 */
		if (pcb->pcb_veccpu)
			save_vec_lwp(l, ALTIVEC_SAVE);
		(void)memcpy(mcp->__vrf.__vrs, pcb->pcb_vr.vreg,
		    sizeof (mcp->__vrf.__vrs));
		mcp->__vrf.__vscr = pcb->pcb_vr.vscr;
		mcp->__vrf.__vrsave = pcb->pcb_vr.vrsave;
		*flagp |= _UC_POWERPC_VEC;
	} else
#endif
		memset(&mcp->__vrf, 0, sizeof (mcp->__vrf));
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = trapframe(l);
	const __greg_t *gr = mcp->__gregs;
#ifdef PPC_HAVE_FPU
	struct pcb *pcb = &l->l_addr->u_pcb;
#endif

	/* Restore GPR context, if any. */
	if (flags & _UC_CPU) {
#ifdef PPC_HAVE_FPU
		/*
		 * Always save the FP exception mode in the PCB.
		 */
		pcb->pcb_flags &= ~(PCB_FE0|PCB_FE1);
		pcb->pcb_flags |= gr[_REG_MSR] & (PCB_FE0|PCB_FE1);
#endif

		(void)memcpy(&tf->fixreg, gr, 32 * sizeof (gr[0]));
		tf->cr   = gr[_REG_CR];
		tf->lr   = gr[_REG_LR];
		tf->srr0 = gr[_REG_PC];
		/*
		 * Accept all user-settable bits without complaint;
		 * userland should not need to know the machine-specific
		 * MSR value.
		 */
		tf->srr1 = (gr[_REG_MSR] & PSL_USERMOD) | PSL_USERSET;
		tf->ctr  = gr[_REG_CTR];
		tf->xer  = gr[_REG_XER];
#ifdef PPC_OEA
		tf->tf_xtra[TF_MQ] = gr[_REG_MQ];
#endif
	}

#ifdef PPC_HAVE_FPU /* Restore FPR context, if any. */
	if ((flags & _UC_FPU) && mcp->__fpregs.__fpu_valid != 0) {
		/* we don't need to save the state, just drop it */
		save_fpu_lwp(l, FPU_DISCARD);
		(void)memcpy(&pcb->pcb_fpu.fpreg, &mcp->__fpregs.__fpu_regs,
		    sizeof (pcb->pcb_fpu.fpreg));
		((int *)&pcb->pcb_fpu.fpscr)[_QUAD_LOWWORD] = 
		    mcp->__fpregs.__fpu_fpscr;
	}
#endif

#ifdef ALTIVEC
	/* Restore AltiVec context, if any. */
	if (flags & _UC_POWERPC_VEC) {
		/* we don't need to save the state, just drop it */
		save_vec_lwp(l, ALTIVEC_DISCARD);
		(void)memcpy(pcb->pcb_vr.vreg, &mcp->__vrf.__vrs,
		    sizeof (pcb->pcb_vr.vreg));
		pcb->pcb_vr.vscr = mcp->__vrf.__vscr;
		pcb->pcb_vr.vrsave = mcp->__vrf.__vrsave;
	}
#endif

	return (0);
}
