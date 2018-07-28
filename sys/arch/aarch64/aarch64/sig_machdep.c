/* $NetBSD: sig_machdep.c,v 1.1.28.2 2018/07/28 04:37:25 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: sig_machdep.c,v 1.1.28.2 2018/07/28 04:37:25 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>

#include <aarch64/frame.h>

void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct sigaltstack * const ss = &l->l_sigstk;
	const struct sigact_sigdesc * const sd =
	    &p->p_sigacts->sa_sigdesc[ksi->ksi_signo];

	const uintptr_t handler = (uintptr_t) sd->sd_sigact.sa_handler;
	const bool onstack_p = (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (sd->sd_sigact.sa_flags & SA_ONSTACK) != 0;

	vaddr_t sp;

	sp = onstack_p ? ((vaddr_t)ss->ss_sp + ss->ss_size) & -16 : tf->tf_sp;

	sp -= sizeof(ucontext_t);
	const vaddr_t ucp = sp;

	sp -= roundup(sizeof(siginfo_t), 16);
	const vaddr_t sip = sp;

	ucontext_t uc;
	memset(&uc, 0, sizeof(uc));
	uc.uc_flags = _UC_SIGMASK;
	uc.uc_sigmask = *mask;
	uc.uc_link = l->l_ctxlink;
	sendsig_reset(l, ksi->ksi_signo);
	mutex_exit(p->p_lock);
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);

	/*
	 * Copy the siginfo and ucontext onto the user's stack.
	 */
	int error = copyout(&ksi->ksi_info, (void *)sip, sizeof(ksi->ksi_info));
	if (error == 0) {
		error = copyout(&uc, (void *)ucp, sizeof(uc));
	}

	mutex_enter(p->p_lock);

	if (error || sd->sd_vers != 2) {
		/*
		 * Thread has trashed its stack.  Blow it away.
		 */
		if (error == 0) {
			printf("pid %d.%d(%s): %p(sig %d): bad version %d\n",
			    p->p_pid, l->l_lid, p->p_comm, __func__,
			    ksi->ksi_signo, sd->sd_vers);
		}
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_reg[0] = ksi->ksi_signo;
	tf->tf_reg[1] = sip;
	tf->tf_reg[2] = ucp;
	tf->tf_reg[28] = ucp;	/* put in a callee saved register */

	tf->tf_sp = sp;
	tf->tf_lr = (uint64_t)sd->sd_tramp;
	tf->tf_pc = handler;

	/*
	 * Remember if we'ere now on the signal stack.
	 */
	if (onstack_p)
		ss->ss_flags |= SS_ONSTACK;
}
