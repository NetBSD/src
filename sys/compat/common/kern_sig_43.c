/*	$NetBSD: kern_sig_43.c,v 1.15 2001/05/30 11:37:22 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/core.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>

#include <sys/user.h>		/* for coredump */

void compat_43_sigmask_to_sigset __P((const int *, sigset_t *));
void compat_43_sigset_to_sigmask __P((const sigset_t *, int *));
void compat_43_sigvec_to_sigaction __P((const struct sigvec *, struct sigaction *));
void compat_43_sigaction_to_sigvec __P((const struct sigaction *, struct sigvec *));
void compat_43_sigstack_to_sigaltstack __P((const struct sigstack *, struct sigaltstack *));
void compat_43_sigaltstack_to_sigstack __P((const struct sigaltstack *, struct sigstack *));

void
compat_43_sigmask_to_sigset(sm, ss)
	const int *sm;
	sigset_t *ss;
{

	ss->__bits[0] = *sm;
	ss->__bits[1] = 0;
	ss->__bits[2] = 0;
	ss->__bits[3] = 0;
}

void
compat_43_sigset_to_sigmask(ss, sm)
	const sigset_t *ss;
	int *sm;
{

	*sm = ss->__bits[0];
}

void
compat_43_sigvec_to_sigaction(sv, sa)
	const struct sigvec *sv;
	struct sigaction *sa;
{
	sa->sa_handler = sv->sv_handler;
	compat_43_sigmask_to_sigset(&sv->sv_mask, &sa->sa_mask);
	sa->sa_flags = sv->sv_flags ^ SA_RESTART;
}

void
compat_43_sigaction_to_sigvec(sa, sv)
	const struct sigaction *sa;
	struct sigvec *sv;
{
	sv->sv_handler = sa->sa_handler;
	compat_43_sigset_to_sigmask(&sa->sa_mask, &sv->sv_mask);
	sv->sv_flags = sa->sa_flags ^ SA_RESTART;
}

void
compat_43_sigstack_to_sigaltstack(ss, sa)
	const struct sigstack *ss;
	struct sigaltstack *sa;
{
	sa->ss_sp = ss->ss_sp;
	sa->ss_size = SIGSTKSZ;	/* Use the recommended size */
	sa->ss_flags = 0;
	if (ss->ss_onstack)
		sa->ss_flags |= SS_ONSTACK;
}

void
compat_43_sigaltstack_to_sigstack(sa, ss)
	const struct sigaltstack *sa;
	struct sigstack *ss;
{
	ss->ss_sp = sa->ss_sp;
	if (sa->ss_flags & SS_ONSTACK)
		ss->ss_onstack = 1;
	else
		ss->ss_onstack = 0;
}

int
compat_43_sys_sigblock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_sigblock_args /* {
		syscallarg(int) mask;
	} */ *uap = v;
	int nsm, osm;
	sigset_t nss, oss;
	int error;

	nsm = SCARG(uap, mask);
	compat_43_sigmask_to_sigset(&nsm, &nss);
	error = sigprocmask1(p, SIG_BLOCK, &nss, &oss);
	if (error)
		return (error);
	compat_43_sigset_to_sigmask(&oss, &osm);
	*retval = osm;
	return (0);
}

int
compat_43_sys_sigsetmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_sigsetmask_args /* {
		syscallarg(int) mask;
	} */ *uap = v;
	int nsm, osm;
	sigset_t nss, oss;
	int error;

	nsm = SCARG(uap, mask);
	compat_43_sigmask_to_sigset(&nsm, &nss);
	error = sigprocmask1(p, SIG_SETMASK, &nss, &oss);
	if (error)
		return (error);
	compat_43_sigset_to_sigmask(&oss, &osm);
	*retval = osm;
	return (0);
}

/* ARGSUSED */
int
compat_43_sys_sigstack(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_sigstack_args /* {
		syscallarg(struct sigstack *) nss;
		syscallarg(struct sigstack *) oss;
	} */ *uap = v;
	struct sigstack nss, oss;
	struct sigaltstack nsa, osa;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin(SCARG(uap, nss), &nss, sizeof(nss));
		if (error)
			return (error);
		compat_43_sigstack_to_sigaltstack(&nss, &nsa);
	}
	error = sigaltstack1(p,
	    SCARG(uap, nss) ? &nsa : 0, SCARG(uap, oss) ? &osa : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		compat_43_sigaltstack_to_sigstack(&osa, &oss);
		error = copyout(&oss, SCARG(uap, oss), sizeof(oss));
		if (error)
			return (error);
	}
	return (0);
}

/*
 * Generalized interface signal handler, 4.3-compatible.
 */
/* ARGSUSED */
int
compat_43_sys_sigvec(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_sigvec_args /* {
		syscallarg(int) signum;
		syscallarg(const struct sigvec *) nsv;
		syscallarg(struct sigvec *) osv;
	} */ *uap = v;
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsv)) {
		error = copyin(SCARG(uap, nsv), &nsv, sizeof(nsv));
		if (error)
			return (error);
		compat_43_sigvec_to_sigaction(&nsv, &nsa);
	}
	error = sigaction1(p, SCARG(uap, signum),
	    SCARG(uap, nsv) ? &nsa : 0, SCARG(uap, osv) ? &osa : 0);
	if (error)
		return (error);
	if (SCARG(uap, osv)) {
		compat_43_sigaction_to_sigvec(&osa, &osv);
		error = copyout(&osv, SCARG(uap, osv), sizeof(osv));
		if (error)
			return (error);
	}
	return (0);
}


/* ARGSUSED */
int
compat_43_sys_killpg(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_43_sys_killpg_args /* {
		syscallarg(int) pgid;
		syscallarg(int) signum;
	} */ *uap = v;

#ifdef COMPAT_09
	SCARG(uap, pgid) = (short) SCARG(uap, pgid);
#endif

	if ((u_int)SCARG(uap, signum) >= NSIG)
		return (EINVAL);
	return (killpg1(p, SCARG(uap, signum), SCARG(uap, pgid), 0));
}
