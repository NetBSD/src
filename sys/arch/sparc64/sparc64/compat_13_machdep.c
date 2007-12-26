/*	$NetBSD: compat_13_machdep.c,v 1.18.28.1 2007/12/26 19:42:47 ad Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.18.28.1 2007/12/26 19:42:47 ad Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <sys/syscallargs.h>
#include <sparc64/sparc64/sigdebug.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above),
 * and return to the given trap frame (if there is one).
 * Check carefully to make sure that the user has not
 * modified the state to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
compat_13_sys_sigreturn(struct lwp *l, const struct compat_13_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */
	struct sigcontext13 sc, *scp;
	struct trapframe64 *tf;
	struct proc *p = l->l_proc;
	sigset_t mask;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
#ifdef DEBUG
		printf("compat_13_sys_sigreturn: rwindow_save(%p) failed, sending SIGILL\n", l);
#ifdef DDB
		Debugger();
#endif
#endif
		mutex_enter(&p->p_smutex);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_sys_sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	scp = SCARG(uap, sigcntxp);
	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
#ifdef DEBUG
	{
		printf("compat_13_sys_sigreturn: copyin failed: scp=%p\n", scp);
#ifdef DDB
		Debugger();
#endif
		return (EFAULT);
	}
#else
		return (EFAULT);
#endif

	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0 || scp->sc_pc == 0 || scp->sc_npc == 0)
#ifdef DEBUG
	{
		printf("compat_13_sys_sigreturn: pc %p or npc %p invalid\n",
			(void *)scp->sc_pc, (void *)scp->sc_npc);
#ifdef DDB
		Debugger();
#endif
		return (EINVAL);
	}
#endif
		return (EINVAL);
	/* take only psr ICC field */
#ifdef __arch64__
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | scp->sc_tstate;
#else
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(scp->sc_psr);
#endif
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_sys_sigreturn: return trapframe pc=%llx sp=%llx tstate=%llx\n",
			(long long)tf->tf_pc, (long long)tf->tf_out[6],
			(long long)tf->tf_tstate);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	mutex_enter(&p->p_smutex);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	native_sigset13_to_sigset(&scp->sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(&p->p_smutex);

	return (EJUSTRETURN);
}
