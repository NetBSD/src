/*	$NetBSD: linux_signal.c,v 1.31.2.6 2002/05/29 21:32:44 nathanw Exp $	*/
/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
/*
 * heavily from: svr4_signal.c,v 1.7 1995/01/09 01:04:21 christos Exp
 */

/*
 *   Functions in multiarch:
 *	linux_sys_signal	: linux_sig_notalpha.c
 *	linux_sys_siggetmask	: linux_sig_notalpha.c
 *	linux_sys_sigsetmask	: linux_sig_notalpha.c
 *	linux_sys_pause		: linux_sig_notalpha.c
 *	linux_sys_sigaction	: linux_sigaction.c
 *
 */

/*
 *   Unimplemented:
 *	linux_sys_rt_sigtimedwait	: sigsuspend w/timeout.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_signal.c,v 1.31.2.6 2002/05/29 21:32:44 nathanw Exp $");

#define COMPAT_LINUX 1

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/common/linux_util.h>

#include <compat/linux/linux_syscallargs.h>

/* Locally used defines (in bsd<->linux conversion functions): */
#define	linux_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define	linux_sigismember(s, n)	((s)->sig[((n) - 1) / LINUX__NSIG_BPW]	\
					& (1 << ((n) - 1) % LINUX__NSIG_BPW))
#define	linux_sigaddset(s, n)	((s)->sig[((n) - 1) / LINUX__NSIG_BPW]	\
					|= (1 << ((n) - 1) % LINUX__NSIG_BPW))

#ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
#else
#define DPRINTF(a)
#endif

extern const int native_to_linux_signo[];
extern const int linux_to_native_signo[];

/*
 * Convert between Linux and BSD signal sets.
 */
#if LINUX__NSIG_WORDS > 1
void
linux_old_extra_to_native_sigset(bss, lss, extra)
	sigset_t *bss;
	const linux_old_sigset_t *lss;
	const unsigned long *extra;
{
	linux_sigset_t lsnew;

	/* convert old sigset to new sigset */
	linux_sigemptyset(&lsnew);
	lsnew.sig[0] = *lss;
	if (extra)
		memcpy(&lsnew.sig[1], extra,
		    sizeof(linux_sigset_t) - sizeof(linux_old_sigset_t));

	linux_to_native_sigset(bss, &lsnew);
}

void
native_to_linux_old_extra_sigset(lss, extra, bss)
	linux_old_sigset_t *lss;
	unsigned long *extra;
	const sigset_t *bss;
{
	linux_sigset_t lsnew;

	native_to_linux_sigset(&lsnew, bss);

	/* convert new sigset to old sigset */
	*lss = lsnew.sig[0];
	if (extra)
		memcpy(extra, &lsnew.sig[1],
		    sizeof(linux_sigset_t) - sizeof(linux_old_sigset_t));
}
#endif

void
linux_to_native_sigset(bss, lss)
	sigset_t *bss;
	const linux_sigset_t *lss;
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < LINUX__NSIG; i++) {
		if (linux_sigismember(lss, i)) {
			newsig = linux_to_native_signo[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}

void
native_to_linux_sigset(lss, bss)
	linux_sigset_t *lss;
	const sigset_t *bss;
{
	int i, newsig;

	linux_sigemptyset(lss);
	for (i = 1; i < NSIG; i++) {
		if (sigismember(bss, i)) {
			newsig = native_to_linux_signo[i];
			if (newsig)
				linux_sigaddset(lss, newsig);
		}
	}
}

unsigned int
native_to_linux_sigflags(bsf)
	const int bsf;
{
	unsigned int lsf = 0;
	if ((bsf & SA_NOCLDSTOP) != 0)
		lsf |= LINUX_SA_NOCLDSTOP;
	if ((bsf & SA_NOCLDWAIT) != 0)
		lsf |= LINUX_SA_NOCLDWAIT;
	if ((bsf & SA_ONSTACK) != 0)
		lsf |= LINUX_SA_ONSTACK;
	if ((bsf & SA_RESTART) != 0)
		lsf |= LINUX_SA_RESTART;
	if ((bsf & SA_NODEFER) != 0)
		lsf |= LINUX_SA_NOMASK;
	if ((bsf & SA_RESETHAND) != 0)
		lsf |= LINUX_SA_ONESHOT;
	if ((bsf & SA_SIGINFO) != 0)
		lsf |= LINUX_SA_SIGINFO;
	return lsf;
}

int
linux_to_native_sigflags(lsf)
	const unsigned long lsf;
{
	int bsf = 0;
	if ((lsf & LINUX_SA_NOCLDSTOP) != 0)
		bsf |= SA_NOCLDSTOP;
	if ((lsf & LINUX_SA_NOCLDWAIT) != 0)
		bsf |= SA_NOCLDWAIT;
	if ((lsf & LINUX_SA_ONSTACK) != 0)
		bsf |= SA_ONSTACK;
	if ((lsf & LINUX_SA_RESTART) != 0)
		bsf |= SA_RESTART;
	if ((lsf & LINUX_SA_ONESHOT) != 0)
		bsf |= SA_RESETHAND;
	if ((lsf & LINUX_SA_NOMASK) != 0)
		bsf |= SA_NODEFER;
	if ((lsf & LINUX_SA_SIGINFO) != 0)
		bsf |= SA_SIGINFO;
	if ((lsf & ~LINUX_SA_ALLBITS) != 0)
		DPRINTF(("linux_old_to_native_sigflags: "
		    "%lx extra bits ignored\n", lsf));
	return bsf;
}

/*
 * Convert between Linux and BSD sigaction structures. Linux sometimes
 * has one extra field (sa_restorer) which we don't support.
 */
void
linux_old_to_native_sigaction(bsa, lsa)
	struct sigaction *bsa;
	const struct linux_old_sigaction *lsa;
{
	bsa->sa_handler = lsa->sa_handler;
	linux_old_to_native_sigset(&bsa->sa_mask, &lsa->sa_mask);
	bsa->sa_flags = linux_to_native_sigflags(lsa->sa_flags);
#ifndef __alpha__
/*
 * XXX: On the alpha sa_restorer is elsewhere.
 */
	if (lsa->sa_restorer != NULL)
		DPRINTF(("linux_old_to_native_sigaction: "
		    "sa_restorer ignored\n"));
#endif
}

void
native_to_linux_old_sigaction(lsa, bsa)
	struct linux_old_sigaction *lsa;
	const struct sigaction *bsa;
{
	lsa->sa_handler = bsa->sa_handler;
	native_to_linux_old_sigset(&lsa->sa_mask, &bsa->sa_mask);
	lsa->sa_flags = native_to_linux_sigflags(bsa->sa_flags);
#ifndef __alpha__
	lsa->sa_restorer = NULL;
#endif
}

/* ...and the new sigaction conversion funcs. */
void
linux_to_native_sigaction(bsa, lsa)
	struct sigaction *bsa;
	const struct linux_sigaction *lsa;
{
	bsa->sa_handler = lsa->sa_handler;
	linux_to_native_sigset(&bsa->sa_mask, &lsa->sa_mask);
	bsa->sa_flags = linux_to_native_sigflags(lsa->sa_flags);
#ifndef __alpha__
	if (lsa->sa_restorer != 0)
		DPRINTF(("linux_to_native_sigaction: sa_restorer ignored\n"));
#endif
}

void
native_to_linux_sigaction(lsa, bsa)
	struct linux_sigaction *lsa;
	const struct sigaction *bsa;
{
	lsa->sa_handler = bsa->sa_handler;
	native_to_linux_sigset(&lsa->sa_mask, &bsa->sa_mask);
	lsa->sa_flags = native_to_linux_sigflags(bsa->sa_flags);
#ifndef __alpha__
	lsa->sa_restorer = NULL;
#endif
}

/* ----------------------------------------------------------------------- */

/*
 * The Linux sigaction() system call. Do the usual conversions,
 * and just call sigaction(). Some flags and values are silently
 * ignored (see above).
 */
int
linux_sys_rt_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct linux_sigaction *) nsa;
		syscallarg(struct linux_sigaction *) osa;
		syscallarg(size_t) sigsetsize;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct linux_sigaction nlsa, olsa;
	struct sigaction nbsa, obsa;
	int error, sig;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nlsa, sizeof(nlsa));
		if (error)
			return (error);
		linux_to_native_sigaction(&nbsa, &nlsa);
	}
	sig = SCARG(uap, signum);
	if (sig < 0 || sig >= LINUX__NSIG)
		return (EINVAL);
	if (sig > 0 && !linux_to_native_signo[sig]) {
		/* Pretend that we did something useful for unknown signals. */
		obsa.sa_handler = SIG_IGN;
		sigemptyset(&obsa.sa_mask);
		obsa.sa_flags = 0;
	} else {
		error = sigaction1(p, linux_to_native_signo[sig],
		    SCARG(uap, nsa) ? &nbsa : NULL, SCARG(uap, osa) ? &obsa : NULL);
		if (error)
			return (error);
	}
	if (SCARG(uap, osa)) {
		native_to_linux_sigaction(&olsa, &obsa);
		error = copyout(&olsa, SCARG(uap, osa), sizeof(olsa));
		if (error)
			return (error);
	}
	return (0);
}

int
linux_sigprocmask1(p, how, set, oset)
	struct proc *p;
	int how;
	const linux_old_sigset_t *set;
	linux_old_sigset_t *oset;
{
	linux_old_sigset_t nlss, olss;
	sigset_t nbss, obss;
	int error;

	switch (how) {
	case LINUX_SIG_BLOCK:
		how = SIG_BLOCK;
		break;
	case LINUX_SIG_UNBLOCK:
		how = SIG_UNBLOCK;
		break;
	case LINUX_SIG_SETMASK:
		how = SIG_SETMASK;
		break;
	default:
		return (EINVAL);
	}

	if (set) {
		error = copyin(set, &nlss, sizeof(nlss));
		if (error)
			return (error);
		linux_old_to_native_sigset(&nbss, &nlss);
	}
	error = sigprocmask1(p, how,
	    set ? &nbss : NULL, oset ? &obss : NULL);
	if (error)
		return (error); 
	if (oset) {
		native_to_linux_old_sigset(&olss, &obss);
		error = copyout(&olss, oset, sizeof(olss));
		if (error)
			return (error);
	}       
	return (error);
}

int
linux_sys_rt_sigprocmask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(const linux_sigset_t *) set;
		syscallarg(linux_sigset_t *) oset;
		syscallarg(size_t) sigsetsize;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	linux_sigset_t nlss, olss, *oset;
	const linux_sigset_t *set;
	sigset_t nbss, obss;
	int error, how;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	switch (SCARG(uap, how)) {
	case LINUX_SIG_BLOCK:
		how = SIG_BLOCK;
		break;
	case LINUX_SIG_UNBLOCK:
		how = SIG_UNBLOCK;
		break;
	case LINUX_SIG_SETMASK:
		how = SIG_SETMASK;
		break;
	default:
		return (EINVAL);
	}

	set = SCARG(uap, set);
	oset = SCARG(uap, oset);

	if (set) {
		error = copyin(set, &nlss, sizeof(nlss));
		if (error)
			return (error);
		linux_to_native_sigset(&nbss, &nlss);
	}
	error = sigprocmask1(p, how,
	    set ? &nbss : NULL, oset ? &obss : NULL);
	if (!error && oset) {
		native_to_linux_sigset(&olss, &obss);
		error = copyout(&olss, oset, sizeof(olss));
	}       
	return (error);
}

int
linux_sys_rt_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigpending_args /* {
		syscallarg(linux_sigset_t *) set;
		syscallarg(size_t) sigsetsize;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	sigset_t bss;
	linux_sigset_t lss;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	sigpending1(p, &bss);
	native_to_linux_sigset(&lss, &bss);
	return copyout(&lss, SCARG(uap, set), sizeof(lss));
}

int
linux_sys_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigpending_args /* {
		syscallarg(linux_old_sigset_t *) mask;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	sigset_t bss;
	linux_old_sigset_t lss;

	sigpending1(p, &bss);
	native_to_linux_old_sigset(&lss, &bss);
	return copyout(&lss, SCARG(uap, set), sizeof(lss));
}

int
linux_sys_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigsuspend_args /* {
		syscallarg(caddr_t) restart;
		syscallarg(int) oldmask;
		syscallarg(int) mask;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	linux_old_sigset_t lss;
	sigset_t bss;

	lss = SCARG(uap, mask);
	linux_old_to_native_sigset(&bss, &lss);
	return (sigsuspend1(p, &bss));
}
int
linux_sys_rt_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigsuspend_args /* {
		syscallarg(linux_sigset_t *) unewset;
		syscallarg(size_t) sigsetsize;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	linux_sigset_t lss;
	sigset_t bss;
	int error;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	error = copyin(SCARG(uap, unewset), &lss, sizeof(linux_sigset_t));
	if (error)
		return (error);

	linux_to_native_sigset(&bss, &lss);

	return (sigsuspend1(p, &bss));
}

/*
 * Once more: only a signal conversion is needed.
 * Note: also used as sys_rt_queueinfo.  The info field is ignored.
 */
int
linux_sys_rt_queueinfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	/* XXX XAX This isn't this really int, int, siginfo_t *, is it? */
#if 0
	struct linux_sys_rt_queueinfo_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
		syscallarg(siginfo_t *) uinfo;
	} */ *uap = v;
#endif

	/* XXX To really implement this we need to	*/
	/* XXX keep a list of queued signals somewhere.	*/
	return (linux_sys_kill(l, v, retval));
}

int
linux_sys_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap = v;

	struct sys_kill_args ka;
	int sig;

	SCARG(&ka, pid) = SCARG(uap, pid);
	sig = SCARG(uap, signum);
	if (sig < 0 || sig >= LINUX__NSIG)
		return (EINVAL);
	SCARG(&ka, signum) = linux_to_native_signo[sig];
	return sys_kill(l, &ka, retval);
}

#ifdef LINUX_SS_ONSTACK
static void linux_to_native_sigaltstack __P((struct sigaltstack *,
    const struct linux_sigaltstack *));
static void native_to_linux_sigaltstack __P((struct linux_sigaltstack *,
    const struct sigaltstack *));

static void
linux_to_native_sigaltstack(bss, lss)
	struct sigaltstack *bss;
	const struct linux_sigaltstack *lss;
{
	bss->ss_sp = lss->ss_sp;
	bss->ss_size = lss->ss_size;
	if (lss->ss_flags & LINUX_SS_ONSTACK)
	    bss->ss_flags = SS_ONSTACK;
	else if (lss->ss_flags & LINUX_SS_DISABLE)
	    bss->ss_flags = SS_DISABLE;
	else
	    bss->ss_flags = 0;
}

static void
native_to_linux_sigaltstack(lss, bss)
	struct linux_sigaltstack *lss;
	const struct sigaltstack *bss;
{
	lss->ss_sp = bss->ss_sp;
	lss->ss_size = bss->ss_size;
	if (bss->ss_flags & SS_ONSTACK)
	    lss->ss_flags = LINUX_SS_ONSTACK;
	else if (bss->ss_flags & SS_DISABLE)
	    lss->ss_flags = LINUX_SS_DISABLE;
	else
	    lss->ss_flags = 0;
}

int
linux_sys_sigaltstack(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigaltstack_args /* {
		syscallarg(const struct linux_sigaltstack *) ss;
		syscallarg(struct linux_sigaltstack *) oss;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct linux_sigaltstack ss;
	struct sigaltstack nss, oss;
	int error;

	if (SCARG(uap, ss) != NULL) {
		if ((error = copyin(SCARG(uap, ss), &ss, sizeof(ss))) != 0)
			return error;
		linux_to_native_sigaltstack(&nss, &ss);
	}

	error = sigaltstack1(p,
	    SCARG(uap, ss) ? &nss : NULL, SCARG(uap, oss) ? &oss : NULL);
	if (error)
		return error;

	if (SCARG(uap, oss) != NULL) {
		native_to_linux_sigaltstack(&ss, &oss);
		if ((error = copyout(&ss, SCARG(uap, oss), sizeof(ss))) != 0)
			return error;
	}
	return 0;
}
#endif
