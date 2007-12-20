/*	$NetBSD: osf1_signal.c,v 1.34 2007/12/20 23:03:03 dsl Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_signal.c,v 1.34 2007/12/20 23:03:03 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_signal.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/common/compat_sigaltstack.h>
#include <compat/osf1/osf1_cvt.h>

#if 0
int
osf1_sys_kill(struct lwp *l, const struct osf1_sys_kill_args *uap, register_t *retval)
{
	struct sys_kill_args ka;

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = osf1_to_native_signo[SCARG(uap, signum)];
	return sys_kill(l, &ka, retval);
}
#endif

int
osf1_sys_sigaction(struct lwp *l, const struct osf1_sys_sigaction_args *uap, register_t *retval)
{
	struct osf1_sigaction *nosa, *oosa, tmposa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	nosa = SCARG(uap, nsa);
	oosa = SCARG(uap, osa);

	if (nosa != NULL) {
		if ((error = copyin(nosa, &tmposa, sizeof(tmposa))) != 0)
			return error;
		osf1_cvt_sigaction_to_native(&tmposa, &nbsa);
	}

	if ((error = sigaction1(l,
				osf1_to_native_signo[SCARG(uap, signum)],
				(nosa ? &nbsa : NULL),
				(oosa ? &obsa : NULL),
				NULL, 0)) != 0)
		return error;

	if (oosa != NULL) {
		osf1_cvt_sigaction_from_native(&obsa, &tmposa);
		if ((error = copyout(&tmposa, oosa, sizeof(tmposa))) != 0)
			return error;
	}

	return 0;
}

int
osf1_sys_sigaltstack(struct lwp *l, const struct osf1_sys_sigaltstack_args *uap, register_t *retval)
{
	/* We silently ignore OSF1_SS_NOMASK and OSF1_SS_UCONTEXT */
	compat_sigaltstack(uap, osf1_sigaltstack,
	    OSF1_SS_ONSTACK, OSF1_SS_DISABLE);
}

#if 0
int
osf1_sys_signal(struct lwp *l, const struct osf1_sys_signal_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	int signum;
	int error;

	if (SCARG(uap, signum) < 0 || SCARG(uap, signum) > OSF1_NSIG)
		return EINVAL;
	signum = osf1_to_native_signo[OSF1_SIGNO(SCARG(uap, signum))];
	if (signum <= 0 || signum >= OSF1_NSIG) {
		if (OSF1_SIGCALL(SCARG(uap, signum)) == OSF1_SIGNAL_MASK ||
		    OSF1_SIGCALL(SCARG(uap, signum)) == OSF1_SIGDEFER_MASK)
			*retval = (int)OSF1_SIG_ERR;
		return EINVAL;
	}

	switch (OSF1_SIGCALL(SCARG(uap, signum))) {
	case OSF1_SIGDEFER_MASK:
		/*
		 * sigset is identical to signal() except
		 * that SIG_HOLD is allowed as
		 * an action.
		 */
		if (SCARG(uap, handler) == OSF1_SIG_HOLD) {
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}
		/* FALLTHROUGH */

	case OSF1_SIGNAL_MASK:
		{
			struct sys_sigaction_args sa_args;
			struct sigaction nbsa, obsa;

			nbsa.sa_handler = SCARG(uap, handler);
			sigemptyset(&nbsa.sa_mask);
			nbsa.sa_flags = 0;
#if 0
			if (signum != SIGALRM)
				nbsa.sa_flags = SA_RESTART;
#endif
			error = sigaction1(l, signum, &nbsa, &obsa, ?, ?);
			if (error != 0) {
				DPRINTF(("signal: sigaction failed: %d\n",
					 error));
				*retval = (int)OSF1_SIG_ERR;
				return error;
			}
			*retval = (int)obsa.sa_handler;
			return 0;
		}

	case OSF1_SIGHOLD_MASK:
		{
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}

	case OSF1_SIGRELSE_MASK:
		{
			struct sys_sigprocmask_args sa;

			SCARG(&sa, how) = SIG_UNBLOCK;
			SCARG(&sa, mask) = sigmask(signum);
			return sys_sigprocmask(l, &sa, retval);
		}

	case OSF1_SIGIGNORE_MASK:
		{
			struct sys_sigaction_args sa_args;
			struct sigaction bsa;

			bsa.sa_handler = SIG_IGN;
			sigemptyset(&bsa.sa_mask);
			bsa.sa_flags = 0;
			if ((error = sigaction1(l, &bsa, NULL, ?, ?)) != 0) {
				DPRINTF(("sigignore: sigaction failed\n"));
				return error;
			}
			return 0;
		}

	case OSF1_SIGPAUSE_MASK:
		{
			struct sys_sigsuspend_args sa;

			SCARG(&sa, mask) = p->p_sigmask & ~sigmask(signum);
			return sys_sigsuspend(l, &sa, retval);
		}

	default:
		return ENOSYS;
	}
}

int
osf1_sys_sigpending(struct lwp *l, const struct osf1_sys_sigpending_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	sigset_t bss;
	osf1_sigset_t oss;

	bss = p->p_siglist & p->p_sigmask;
	osf1_cvt_sigset_from_native(&bss, &oss);

	return copyout(&oss, SCARG(uap, mask), sizeof(oss));
}

int
osf1_sys_sigprocmask(struct lwp *l, const struct osf1_sys_sigprocmask_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	osf1_sigset_t oss;
	sigset_t bss;
	int error = 0;

	if (SCARG(uap, oset) != NULL) {
		/* Fix the return value first if needed */
		osf1_cvt_sigset_from_native(&p->p_sigmask, &oss);
		if ((error = copyout(&oss, SCARG(uap, oset), sizeof(oss))) != 0)
			return error;
	}

	if (SCARG(uap, set) == NULL)
		/* Just examine */
		return 0;

	if ((error = copyin(SCARG(uap, set), &oss, sizeof(oss))) != 0)
		return error;

	osf1_cvt_sigset_to_native(&oss, &bss);

	mutex_enter(&p->p_smutex);

	switch (SCARG(uap, how)) {
	case OSF1_SIG_BLOCK:
		*l->l_sigmask |= bss & ~sigcantmask;
		break;

	case OSF1_SIG_UNBLOCK:
		*l->l_sigmask &= ~bss;
		lwp_lock(l);
		l->l_flag |= LW_PENDSIG;
		lwp_unlock(l);
		break;

	case OSF1_SIG_SETMASK:
		*l->l_sigmask = bss & ~sigcantmask;
		lwp_lock(l);
		l->l_flag |= LW_PENDSIG;
		lwp_unlock(l);
		break;

	default:
		error = EINVAL;
		break;
	}

	mutex_exit(&p->p_smutex);

	return error;
}

int
osf1_sys_sigsuspend(struct lwp *l, const struct osf1_sys_sigsuspend_args *uap, register_t *retval)
{
	osf1_sigset_t oss;
	sigset_t bss;
	struct sys_sigsuspend_args sa;
	int error;

	if ((error = copyin(SCARG(uap, ss), &oss, sizeof(oss))) != 0)
		return error;

	osf1_cvt_sigset_to_native(&oss, &bss);

	SCARG(&sa, mask) = bss;
	return sys_sigsuspend(l, &sa, retval);
}
#endif
