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

#ifndef COMPATNAME1
__RCSID("$NetBSD: sig_machdep.c,v 1.1.12.1 2019/01/27 18:43:09 martin Exp $");
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <riscv/locore.h>

#ifndef COMPATNAME1
#define COMPATNAME1(x)		x
#define COMPATNAME2(x)		x
#define COMPATTYPE(x)		__CONCAT(x,_t)
#define UCLINK_SET(uc,c)	((uc)->uc_link = (c))
#define	COPY_SIGINFO(d,s)	((d)->sf_si._info = (s)->ksi_info)

void *	
cpu_sendsig_getframe(struct lwp *l, int signo, bool *onstack)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
 
	/* Do we need to jump onto the signal stack? */
	*onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, signo).sa_flags & SA_ONSTACK) != 0;
	if (*onstack)
		return (char *)stack_align((intptr_t)l->l_sigstk.ss_sp + l->l_sigstk.ss_size);
	return (void *)(intptr_t)stack_align(tf->tf_sp);
}
#endif

#ifdef COMPATINC
#include COMPATINC
#endif

struct COMPATNAME2(sigframe_siginfo) {
	COMPATTYPE(siginfo) sf_si __aligned(__BIGGEST_ALIGNMENT__);
	COMPATTYPE(ucontext) sf_uc;
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
	const int signo = ksi->ksi_signo;
	const sig_t catcher = SIGACTION(p, signo).sa_handler;
	bool onstack;
	struct COMPATNAME2(sigframe_siginfo) *sf =
	    cpu_sendsig_getframe(l, signo, &onstack);
	struct COMPATNAME2(sigframe_siginfo) ksf;

	sf--;		// allocate sigframe

	memset(&ksf, 0, sizeof(ksf));
	COPY_SIGINFO(&ksf, ksi);
	ksf.sf_uc.uc_flags = _UC_SIGMASK
	    | (l->l_sigstk.ss_flags & SS_ONSTACK ? _UC_SETSTACK : _UC_CLRSTACK);
	ksf.sf_uc.uc_sigmask = *mask;
	UCLINK_SET(&ksf.sf_uc, l->l_ctxlink);
	memset(&ksf.sf_uc.uc_stack, 0, sizeof(ksf.sf_uc.uc_stack));
	sendsig_reset(l, signo);

	mutex_exit(p->p_lock);
	COMPATNAME2(cpu_getmcontext)(l, &ksf.sf_uc.uc_mcontext,
	     &ksf.sf_uc.uc_flags);
	int error = copyout(&ksf, sf, sizeof(ksf));
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
	tf->tf_a0 = signo;
	tf->tf_a1 = (intptr_t)&sf->sf_si;
	tf->tf_a2 = (intptr_t)&sf->sf_uc;

	tf->tf_pc = (intptr_t)catcher;
	tf->tf_sp = (intptr_t)sf;
	tf->tf_ra = (intptr_t)sa->sa_sigdesc[signo].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}
