/*	$NetBSD: sig_machdep.c,v 1.23.46.1 2019/01/27 18:43:09 martin Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
	
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.23.46.1 2019/01/27 18:43:09 martin Exp $"); 

#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/locore.h>

void *	
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
 
	/* Do we need to jump onto the signal stack? */
	*onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (*onstack)
		return (char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size;
	return (void *)(intptr_t)tf->tf_regs[_R_SP];
}

struct sigframe_siginfo {
	siginfo_t sf_si;
	ucontext_t sf_uc;
};

/*
 * Send a signal to process.
 */
void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct sigacts * const sa = p->p_sigacts;
	struct trapframe * const tf = l->l_md.md_utf;
	int onstack, error;
	const int signo = ksi->ksi_signo;
	struct sigframe_siginfo *sf = getframe(l, signo, &onstack);
	struct sigframe_siginfo ksf;
	const sig_t catcher = SIGACTION(p, signo).sa_handler;

	sf--;

	memset(&ksf, 0, sizeof(ksf));
	ksf.sf_si._info = ksi->ksi_info;
	ksf.sf_uc.uc_flags = _UC_SIGMASK
	    | (l->l_sigstk.ss_flags & SS_ONSTACK ? _UC_SETSTACK : _UC_CLRSTACK);
	ksf.sf_uc.uc_sigmask = *mask;
	ksf.sf_uc.uc_link = l->l_ctxlink;
	sendsig_reset(l, signo);

	mutex_exit(p->p_lock);
	cpu_getmcontext(l, &ksf.sf_uc.uc_mcontext, &ksf.sf_uc.uc_flags);
	error = copyout(&ksf, sf, sizeof(ksf));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Set up the registers to directly invoke the signal
	 * handler.  The return address will be set up to point
	 * to the signal trampoline to bounce us back.
	 */
	tf->tf_regs[_R_A0] = signo;
	tf->tf_regs[_R_A1] = (intptr_t)&sf->sf_si;
	tf->tf_regs[_R_A2] = (intptr_t)&sf->sf_uc;

	tf->tf_regs[_R_PC] = (intptr_t)catcher;
	tf->tf_regs[_R_T9] = (intptr_t)catcher;
	tf->tf_regs[_R_SP] = (intptr_t)sf;
	tf->tf_regs[_R_RA] = (intptr_t)sa->sa_sigdesc[signo].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}
