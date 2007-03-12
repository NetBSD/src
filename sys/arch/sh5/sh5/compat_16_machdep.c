/*	$NetBSD: compat_16_machdep.c,v 1.3.2.1 2007/03/12 05:50:16 rmind Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.3.2.1 2007/03/12 05:50:16 rmind Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/ras.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#ifdef COMPAT_16
/*
 * Send a signal to process
 */
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);

void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *returnmask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_regs;
	struct sigacts *ps = p->p_sigacts;
	int sig = ksi->ksi_signo, onstack, fsize, rndfsize;
	struct sigcontext *scp, ksc;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	fsize = sizeof(ksc);
	rndfsize = ((fsize + 15) / 16) * 16;

	if (onstack)
		scp = (struct sigcontext *)((void *)p->p_sigctx.ps_sigstk.ss_sp
		    + p->p_sigctx.ps_sigstk.ss_size);
	else
		scp = (struct sigcontext *)(intptr_t)tf->tf_caller.r15;
	scp = (struct sigcontext *)((void *)scp - rndfsize);

	process_read_regs(l, &ksc.sc_regs);
	ksc.sc_regs.r_regs[24] = 0xACEBABE5ULL;	/* magic number */

	/* Save FP state if necessary */
	if ((l->l_md.md_flags & MDP_FPSAVED) == 0) {
		l->l_md.md_flags |= sh5_fpsave(tf->tf_state.sf_usr,
		    &l->l_addr->u_pcb);
	}
	if ((l->l_md.md_flags & MDP_FPUSED) != 0)
		process_read_fpregs(l, &ksc.sc_fpregs);
	else {
		/* Always copy the FPSCR */
		ksc.sc_fpregs.r_fpscr =
		    l->l_addr->u_pcb.pcb_ctx.sf_fpregs.fpscr;
	}
	ksc.sc_fpstate = l->l_md.md_flags & (MDP_FPUSED | MDP_FPSAVED);

	/* Save signal stack */
	ksc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask */
	ksc.sc_mask = *returnmask;

	if (copyout(&ksc, (void *)scp, fsize) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Set up the registers to directly invoke the signal handler.  The
	 * signal trampoline is then used to return from the signal.  Note
	 * the trampoline version numbers are coordinated with machine-
	 * dependent code in libc.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 1:
		tf->tf_caller.r18 =
		    (register_t)(intptr_t)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	tf->tf_state.sf_spc = (register_t)(intptr_t)catcher;
	tf->tf_caller.r2 = ksi->ksi_signo;
	tf->tf_caller.r3 = ksi->ksi_code;
	tf->tf_caller.r4 = (register_t)(intptr_t)scp;
	tf->tf_caller.r15 = (register_t)(intptr_t)scp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

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
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, ksc;
	struct trapframe *tf;
	struct proc *p;
	int i;

	p = l->l_proc;
	tf = l->l_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (ALIGN(scp) != (intptr_t)scp)
		return (EINVAL);

	if (copyin((void *)scp, &ksc, sizeof(ksc)) != 0)
		return (EFAULT);

	if (ksc.sc_regs.r_regs[24] != 0xACEBABE5ULL)	/* magic number */
		return (EINVAL);

	/*
	 * Validate the branch target registers. If we don't, we risk
	 * a kernel-mode exception when trying to restore invalid
	 * values to them just before returning to user-mode.
	 */
	for (i = 0; i < 8; i++) {
		if (!SH5_EFF_IS_VALID(ksc.sc_regs.r_tr[i]) ||
		    (ksc.sc_regs.r_tr[i] & 0x3) == 0x3)
			return (EINVAL);
	}

	/*
	 * Ditto for the PC
	 */
	if (!SH5_EFF_IS_VALID(ksc.sc_regs.r_pc) ||
	    (ksc.sc_regs.r_pc & 0x3) == 0x3)
		return (EINVAL);

	/* Restore register context. */
	process_write_regs(l, &ksc.sc_regs);
	if ((ksc.sc_fpstate & MDP_FPSAVED) != 0) {
		process_write_fpregs(l, &ksc.sc_fpregs);
		sh5_fprestore(tf->tf_state.sf_usr, &l->l_addr->u_pcb);
	} else {
		/* Always restore FPSCR */
		l->l_addr->u_pcb.pcb_ctx.sf_fpregs.fpscr =
		    ksc.sc_fpregs.r_fpscr;
	}
	l->l_md.md_flags = ksc.sc_fpstate & MDP_FPUSED;

	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &ksc.sc_mask, 0);

	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */
