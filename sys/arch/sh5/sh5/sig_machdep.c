/*	$NetBSD: sig_machdep.c,v 1.21.2.1 2007/03/12 05:50:17 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.21.2.1 2007/03/12 05:50:17 rmind Exp $");

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

/*
 * Send a signal to process
 */
static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *returnmask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_regs;
	struct sigacts *ps = p->p_sigacts;
	struct sigframe_siginfo *ssp, kss;
	int onstack, fsize, sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 2:
		break;
	default:
		/* Don't know about this trampoline version; kill it. */
		sigexit(l, SIGILL);
		/*NOTREACHED*/
	}

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	fsize = sizeof(kss);
	if (onstack)
		ssp = (struct sigframe_siginfo *)
		    ((void *)p->p_sigctx.ps_sigstk.ss_sp
		    + p->p_sigctx.ps_sigstk.ss_size);
	else
		ssp = (struct sigframe_siginfo *)(intptr_t)tf->tf_caller.r15;
	ssp = (struct sigframe_siginfo *)((void *)ssp - ((fsize + 15) & ~15));

	/* Build stack frame for signal trampoline. */
	kss.sf_si._info = ksi->ksi_info;
	kss.sf_uc.uc_flags = _UC_SIGMASK;
	kss.sf_uc.uc_sigmask = *returnmask;
	kss.sf_uc.uc_link = NULL;
	kss.sf_uc.uc_flags |= (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	cpu_getmcontext(l, &kss.sf_uc.uc_mcontext, &kss.sf_uc.uc_flags);

	if (copyout(&kss, (void *)ssp, fsize) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Set up the registers to invoke the signal handler directly.
	 */
	tf->tf_state.sf_spc = (register_t)(intptr_t)catcher;
	tf->tf_caller.r2 = (register_t)sig;
	tf->tf_caller.r3 = (register_t)(intptr_t)&ssp->sf_si;
	tf->tf_caller.r4 = tf->tf_callee.r28 = (register_t)(intptr_t)&ssp->sf_uc;
	tf->tf_caller.r15 = (register_t)(intptr_t)ssp;
	tf->tf_caller.r18 = (register_t)(intptr_t)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
#ifdef COMPAT_16
	extern void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);

	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else
#endif
		sendsig_siginfo(ksi, mask);
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	process_read_regs(l, (struct reg *)(void *)gr);

	if ((ras_pc = (__greg_t)(intptr_t) ras_lookup(l->l_proc,
	    (void *)(intptr_t) gr[_REG_PC])) != -1)
		gr[_REG_PC] = ras_pc;

	*flags |= _UC_CPU;

	/* Save FP state if necessary */
	if ((l->l_md.md_flags & MDP_FPSAVED) == 0) {
		l->l_md.md_flags |= sh5_fpsave(tf->tf_state.sf_usr,
		    &l->l_addr->u_pcb);
	}
	if ((l->l_md.md_flags & MDP_FPUSED) != 0) {
		process_read_fpregs(l, (struct fpreg *)(void *)&mcp->__fpregs);
		*flags |= _UC_FPU;
	} else {
		/* Always copy the FPSCR */
		mcp->__fpregs.__fp_scr =
		    l->l_addr->u_pcb.pcb_ctx.sf_fpregs.fpscr;
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;
	int i;

	if (flags & _UC_CPU) {
		/*
		 * Validate the branch target registers. If we don't, we risk
		 * a kernel-mode exception when trying to restore invalid
		 * values to them just before returning to user-mode.
		 */
		for (i = 0; i < 8; i++) {
			if (!SH5_EFF_IS_VALID(gr[_REG_TR(i)]) ||
			    (gr[_REG_TR(i)] & 0x3) == 0x3)
				return (EINVAL);
		}

		/*
		 * Ditto for the PC
		 */
		if (!SH5_EFF_IS_VALID(gr[_REG_PC]) ||
		    (gr[_REG_PC] & 0x3) == 0x3)
			return (EINVAL);

		/* Restore register context. */
		process_write_regs(l, (const struct reg *)(const void *)mcp);
	}

	if (flags & _UC_FPU) {
		process_write_fpregs(l,
		    (const struct fpreg *)(const void *)&mcp->__fpregs);
		sh5_fprestore(tf->tf_state.sf_usr, &l->l_addr->u_pcb);
	} else {
		/* Always restore FPSCR */
		l->l_addr->u_pcb.pcb_ctx.sf_fpregs.fpscr =
		    mcp->__fpregs.__fp_scr;
		l->l_md.md_flags &= ~(MDP_FPUSED | MDP_FPSAVED);
	}

	if (flags & _UC_SETSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	return (0);
}
