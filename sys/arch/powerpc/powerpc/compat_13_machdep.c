/*	$NetBSD: compat_13_machdep.c,v 1.22 2022/03/13 17:50:55 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.22 2022/03/13 17:50:55 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <powerpc/psl.h>

int
compat_13_sys_sigreturn(struct lwp *l,
    const struct compat_13_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct sigcontext13 sc;
	int error;
	sigset_t mask;

	/*
	 * The trampoline hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	if ((error = copyin(SCARG(uap, sigcntxp), &sc, sizeof sc)) != 0)
		return (error);

	if (!PSL_USEROK_P(sc.sc_frame.srr1))
		return (EINVAL);

	/* Restore register context. */
	memcpy(tf->tf_fixreg, sc.sc_frame.fixreg, sizeof(tf->tf_fixreg));
	tf->tf_lr   = sc.sc_frame.lr;
	tf->tf_cr   = sc.sc_frame.cr;
	tf->tf_xer  = sc.sc_frame.xer;
	tf->tf_ctr  = sc.sc_frame.ctr;
	tf->tf_srr0 = sc.sc_frame.srr0;
	tf->tf_srr1 = sc.sc_frame.srr1;
#ifdef PPC_OEA
	tf->tf_vrsave = sc.sc_frame.vrsave;
	tf->tf_mq = sc.sc_frame.mq;
#endif

	mutex_enter(p->p_lock);
	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	native_sigset13_to_sigset(&sc.sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
