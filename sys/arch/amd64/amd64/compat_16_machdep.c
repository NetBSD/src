/*	$NetBSD: compat_16_machdep.c,v 1.1 2003/10/06 22:53:47 fvdl Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.1 2003/10/06 22:53:47 fvdl Exp $");

#include "opt_vm86.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/pmap.h>
#include <machine/vmparam.h>

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */

int compat_16_sys___sigreturn14(struct lwp *, void *, register_t *);

int
compat_16_sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext *scp, context;
	struct trapframe *tf;
	uint64_t rflags;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context,
	    sizeof(struct sigcontext) - sizeof(struct fxsave64)) != 0)
		return EFAULT;

	/* Restore register context. */
	tf = l->l_md.md_regs;
	/*
	 * Check for security violations.  If we're returning to
	 * protected mode, the CPU will validate the segment registers
	 * automatically and generate a trap on violations.  We handle
	 * the trap, rather than doing all of the checking here.
	 */
	rflags = context.sc_mcontext.__gregs[_REG_RFL];
	if (((rflags ^ tf->tf_rflags) & PSL_USERSTATIC) != 0 ||
	    !USERMODE(context.sc_mcontext.__gregs[_REG_CS], rflags))
		return EINVAL;

	memcpy(tf, &context.sc_mcontext.__gregs, sizeof (*tf));

	/* Restore (possibly fixed up) FP state and force it to be reloaded */
	if (l->l_md.md_flags & MDP_USEDFPU) {
		fpusave_lwp(l, 0);
		if (context.sc_fpstate != NULL && copyin(context.sc_fpstate,
		    &l->l_addr->u_pcb.pcb_savefpu.fp_fxsave,
		    sizeof (struct fxsave64)) != 0)
			return EFAULT;
	}

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return EJUSTRETURN;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf;
	struct sigframe_sigcontext *fp, frame;
	int onstack;
	size_t tocopy;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = l->l_md.md_regs;

	fp = getframe(l, sig, &onstack);
	fp--;

	if (l->l_md.md_flags & MDP_USEDFPU) {
		fpusave_lwp(l, 1);
		frame.sf_sc.sc_fpstate =
		    (struct fxsave64 *)&fp->sf_sc.sc_mcontext.__fpregs;
		memcpy(&frame.sf_sc.sc_mcontext.__fpregs,
		    &l->l_addr->u_pcb.pcb_savefpu.fp_fxsave,
		    sizeof (struct fxsave64));
		tocopy = sizeof (struct sigframe_sigcontext);
	} else {
		frame.sf_sc.sc_fpstate = NULL;
		tocopy = sizeof (struct sigframe_sigcontext) -
		    sizeof (fp->sf_sc.sc_mcontext.__fpregs);
	}

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		frame.sf_ra = (uint64_t) p->p_sigctx.ps_sigcode;
		break;

	case 1:
		frame.sf_ra = (uint64_t) ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		printf("osendsig: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	}

	/* Save register context. */
	memcpy(&frame.sf_sc.sc_mcontext.__gregs, tf, sizeof (*tf));

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	if (copyout(&frame, fp, tocopy) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, catcher, fp);

	tf->tf_rdi = sig;
	tf->tf_rsi = ksi->ksi_trap;
	tf->tf_rdx = (int64_t) &fp->sf_sc;


	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}
