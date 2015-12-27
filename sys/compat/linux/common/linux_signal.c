/*	$NetBSD: linux_signal.c,v 1.76.2.1 2015/12/27 12:09:47 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_signal.c,v 1.76.2.1 2015/12/27 12:09:47 skrll Exp $");

#define COMPAT_LINUX 1

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
#include <sys/wait.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/common/linux_sigevent.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_sched.h>

#include <compat/linux/linux_syscallargs.h>

/* Locally used defines (in bsd<->linux conversion functions): */
#define	linux_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define	linux_sigismember(s, n)	((s)->sig[((n) - 1) / LINUX__NSIG_BPW]	\
					& (1L << ((n) - 1) % LINUX__NSIG_BPW))
#define	linux_sigaddset(s, n)	((s)->sig[((n) - 1) / LINUX__NSIG_BPW]	\
					|= (1L << ((n) - 1) % LINUX__NSIG_BPW))

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
linux_old_extra_to_native_sigset(sigset_t *bss, const linux_old_sigset_t *lss, const unsigned long *extra)
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
native_to_linux_old_extra_sigset(linux_old_sigset_t *lss, unsigned long *extra, const sigset_t *bss)
{
	linux_sigset_t lsnew;

	native_to_linux_sigset(&lsnew, bss);

	/* convert new sigset to old sigset */
	*lss = lsnew.sig[0];
	if (extra)
		memcpy(extra, &lsnew.sig[1],
		    sizeof(linux_sigset_t) - sizeof(linux_old_sigset_t));
}
#endif /* LINUX__NSIG_WORDS > 1 */

void
linux_to_native_sigset(sigset_t *bss, const linux_sigset_t *lss)
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
native_to_linux_sigset(linux_sigset_t *lss, const sigset_t *bss)
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

void
native_to_linux_siginfo(linux_siginfo_t *lsi, const struct _ksiginfo *ksi)
{
	memset(lsi, 0, sizeof(*lsi));

	lsi->lsi_signo = native_to_linux_signo[ksi->_signo];
	lsi->lsi_errno = native_to_linux_errno[ksi->_errno];
	lsi->lsi_code = native_to_linux_si_code(ksi->_code);

	switch (ksi->_code) {
	case SI_NOINFO:
		break;

	case SI_USER:
		lsi->lsi_pid = ksi->_reason._rt._pid;
		lsi->lsi_uid = ksi->_reason._rt._uid;
		if (lsi->lsi_signo == LINUX_SIGALRM ||
		    lsi->lsi_signo >= LINUX_SIGRTMIN)
			lsi->lsi_value.sival_ptr =
			    ksi->_reason._rt._value.sival_ptr;
		break;

	case SI_TIMER:
	case SI_QUEUE:
		lsi->lsi_uid = ksi->_reason._rt._uid;
		lsi->lsi_uid = ksi->_reason._rt._uid;
		lsi->lsi_value.sival_ptr = ksi->_reason._rt._value.sival_ptr;
		break;

	case SI_ASYNCIO:
	case SI_MESGQ:
		lsi->lsi_value.sival_ptr = ksi->_reason._rt._value.sival_ptr;
		break;

	default:
		switch (ksi->_signo) {
		case SIGCHLD:
			lsi->lsi_uid = ksi->_reason._child._uid;
			lsi->lsi_pid = ksi->_reason._child._pid;
			lsi->lsi_status = native_to_linux_si_status(
			    ksi->_code, ksi->_reason._child._status);
			lsi->lsi_utime = ksi->_reason._child._utime;
			lsi->lsi_stime = ksi->_reason._child._stime;
			break;

		case SIGILL:
		case SIGFPE:
		case SIGSEGV:
		case SIGBUS:
		case SIGTRAP:
			lsi->lsi_addr = ksi->_reason._fault._addr;
			break;

		case SIGIO:
			lsi->lsi_fd = ksi->_reason._poll._fd;
			lsi->lsi_band = ksi->_reason._poll._band;
			break;
		default:
			break;
		}
	}
}

unsigned int
native_to_linux_sigflags(const int bsf)
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
linux_to_native_sigflags(const unsigned long lsf)
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
	if ((lsf & ~LINUX_SA_ALLBITS) != 0) {
		DPRINTF(("linux_old_to_native_sigflags: "
		    "%lx extra bits ignored\n", lsf));
	}
	return bsf;
}

/*
 * Convert between Linux and BSD sigaction structures.
 */
void
linux_old_to_native_sigaction(struct sigaction *bsa, const struct linux_old_sigaction *lsa)
{
	bsa->sa_handler = lsa->linux_sa_handler;
	linux_old_to_native_sigset(&bsa->sa_mask, &lsa->linux_sa_mask);
	bsa->sa_flags = linux_to_native_sigflags(lsa->linux_sa_flags);
}

void
native_to_linux_old_sigaction(struct linux_old_sigaction *lsa, const struct sigaction *bsa)
{
	lsa->linux_sa_handler = bsa->sa_handler;
	native_to_linux_old_sigset(&lsa->linux_sa_mask, &bsa->sa_mask);
	lsa->linux_sa_flags = native_to_linux_sigflags(bsa->sa_flags);
#ifndef __alpha__
	lsa->linux_sa_restorer = NULL;
#endif
}

/* ...and the new sigaction conversion funcs. */
void
linux_to_native_sigaction(struct sigaction *bsa, const struct linux_sigaction *lsa)
{
	bsa->sa_handler = lsa->linux_sa_handler;
	linux_to_native_sigset(&bsa->sa_mask, &lsa->linux_sa_mask);
	bsa->sa_flags = linux_to_native_sigflags(lsa->linux_sa_flags);
}

void
native_to_linux_sigaction(struct linux_sigaction *lsa, const struct sigaction *bsa)
{
	lsa->linux_sa_handler = bsa->sa_handler;
	native_to_linux_sigset(&lsa->linux_sa_mask, &bsa->sa_mask);
	lsa->linux_sa_flags = native_to_linux_sigflags(bsa->sa_flags);
#ifndef __alpha__
	lsa->linux_sa_restorer = NULL;
#endif
}

/* ----------------------------------------------------------------------- */

/*
 * The Linux sigaction() system call. Do the usual conversions,
 * and just call sigaction(). Some flags and values are silently
 * ignored (see above).
 */
int
linux_sys_rt_sigaction(struct lwp *l, const struct linux_sys_rt_sigaction_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const struct linux_sigaction *) nsa;
		syscallarg(struct linux_sigaction *) osa;
		syscallarg(size_t) sigsetsize;
	} */
	struct linux_sigaction nlsa, olsa;
	struct sigaction nbsa, obsa;
	int error, sig;
	void *tramp = NULL;
	int vers = 0;
#ifdef LINUX_SA_RESTORER
	struct sigacts *ps = l->l_proc->p_sigacts;
#endif

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
#ifdef LINUX_SA_RESTORER
		if ((nlsa.linux_sa_flags & LINUX_SA_RESTORER) &&
		    (tramp = nlsa.linux_sa_restorer) != NULL)
				vers = 2;
#endif

		error = sigaction1(l, linux_to_native_signo[sig],
		    SCARG(uap, nsa) ? &nbsa : NULL,
		    SCARG(uap, osa) ? &obsa : NULL,
		    tramp, vers);
		if (error)
			return (error);
	}
	if (SCARG(uap, osa)) {
		native_to_linux_sigaction(&olsa, &obsa);

#ifdef LINUX_SA_RESTORER
		if (ps->sa_sigdesc[sig].sd_vers != 0) {
			olsa.linux_sa_restorer = ps->sa_sigdesc[sig].sd_tramp;
			olsa.linux_sa_flags |= LINUX_SA_RESTORER;
		}
#endif

		error = copyout(&olsa, SCARG(uap, osa), sizeof(olsa));
		if (error)
			return (error);
	}
	return (0);
}

int
linux_sigprocmask1(struct lwp *l, int how, const linux_old_sigset_t *set, linux_old_sigset_t *oset)
{
	struct proc *p = l->l_proc;
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
	mutex_enter(p->p_lock);
	error = sigprocmask1(l, how,
	    set ? &nbss : NULL, oset ? &obss : NULL);
	mutex_exit(p->p_lock);
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
linux_sys_rt_sigprocmask(struct lwp *l, const struct linux_sys_rt_sigprocmask_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) how;
		syscallarg(const linux_sigset_t *) set;
		syscallarg(linux_sigset_t *) oset;
		syscallarg(size_t) sigsetsize;
	} */
	linux_sigset_t nlss, olss, *oset;
	const linux_sigset_t *set;
	struct proc *p = l->l_proc;
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
	mutex_enter(p->p_lock);
	error = sigprocmask1(l, how,
	    set ? &nbss : NULL, oset ? &obss : NULL);
	mutex_exit(p->p_lock);
	if (!error && oset) {
		native_to_linux_sigset(&olss, &obss);
		error = copyout(&olss, oset, sizeof(olss));
	}
	return (error);
}

int
linux_sys_rt_sigpending(struct lwp *l, const struct linux_sys_rt_sigpending_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_sigset_t *) set;
		syscallarg(size_t) sigsetsize;
	} */
	sigset_t bss;
	linux_sigset_t lss;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	sigpending1(l, &bss);
	native_to_linux_sigset(&lss, &bss);
	return copyout(&lss, SCARG(uap, set), sizeof(lss));
}

#ifndef __amd64__
int
linux_sys_sigpending(struct lwp *l, const struct linux_sys_sigpending_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_old_sigset_t *) mask;
	} */
	sigset_t bss;
	linux_old_sigset_t lss;

	sigpending1(l, &bss);
	native_to_linux_old_sigset(&lss, &bss);
	return copyout(&lss, SCARG(uap, set), sizeof(lss));
}

int
linux_sys_sigsuspend(struct lwp *l, const struct linux_sys_sigsuspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) restart;
		syscallarg(int) oldmask;
		syscallarg(int) mask;
	} */
	linux_old_sigset_t lss;
	sigset_t bss;

	lss = SCARG(uap, mask);
	linux_old_to_native_sigset(&bss, &lss);
	return (sigsuspend1(l, &bss));
}
#endif /* __amd64__ */

int
linux_sys_rt_sigsuspend(struct lwp *l, const struct linux_sys_rt_sigsuspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_sigset_t *) unewset;
		syscallarg(size_t) sigsetsize;
	} */
	linux_sigset_t lss;
	sigset_t bss;
	int error;

	if (SCARG(uap, sigsetsize) != sizeof(linux_sigset_t))
		return (EINVAL);

	error = copyin(SCARG(uap, unewset), &lss, sizeof(linux_sigset_t));
	if (error)
		return (error);

	linux_to_native_sigset(&bss, &lss);

	return (sigsuspend1(l, &bss));
}

static int
fetchss(const void *u, void *s, size_t len)
{
	int error;
	linux_sigset_t lss;
	
	if ((error = copyin(u, &lss, sizeof(lss))) != 0)
		return error;

	linux_to_native_sigset(s, &lss);
	return 0;
}

static int
fetchts(const void *u, void *s, size_t len)
{
	int error;
	struct linux_timespec lts;
	
	if ((error = copyin(u, &lts, sizeof(lts))) != 0)
		return error;

	linux_to_native_timespec(s, &lts);
	return 0;
}

static int
fakestorets(const void *u, void *s, size_t len)
{
	/* Do nothing, sigtimedwait does not alter timeout like ours */
	return 0;
}

static int
storeinfo(const void *s, void *u, size_t len)
{
	struct linux_siginfo lsi;

	native_to_linux_siginfo(&lsi, &((const siginfo_t *)s)->_info);
	return copyout(&lsi, u, sizeof(lsi));
}

int
linux_sys_rt_sigtimedwait(struct lwp *l,
    const struct linux_sys_rt_sigtimedwait_args *uap, register_t *retval)
{
	/* {
		syscallarg(const linux_sigset_t *) set;
		syscallarg(linux_siginfo_t *) info);
		syscallarg(const struct linux_timespec *) timeout;
	} */

	return sigtimedwait1(l, (const struct sys_____sigtimedwait50_args *)uap,
	    retval, fetchss, storeinfo, fetchts, fakestorets);
}

/*
 * Once more: only a signal conversion is needed.
 * Note: also used as sys_rt_queueinfo.  The info field is ignored.
 */
int
linux_sys_rt_queueinfo(struct lwp *l, const struct linux_sys_rt_queueinfo_args *uap, register_t *retval)
{
	/*
		syscallarg(int) pid;
		syscallarg(int) signum;
		syscallarg(linix_siginfo_t *) uinfo;
	*/
	int error;
	linux_siginfo_t info;

	error = copyin(SCARG(uap, uinfo), &info, sizeof(info));
	if (error)
		return error;
	if (info.lsi_code >= 0)
		return EPERM;

	/* XXX To really implement this we need to	*/
	/* XXX keep a list of queued signals somewhere.	*/
	return (linux_sys_kill(l, (const void *)uap, retval));
}

int
linux_sys_kill(struct lwp *l, const struct linux_sys_kill_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */

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
static void linux_to_native_sigaltstack(struct sigaltstack *,
    const struct linux_sigaltstack *);

static void
linux_to_native_sigaltstack(struct sigaltstack *bss, const struct linux_sigaltstack *lss)
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

void
native_to_linux_sigaltstack(struct linux_sigaltstack *lss, const struct sigaltstack *bss)
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
linux_sys_sigaltstack(struct lwp *l, const struct linux_sys_sigaltstack_args *uap, register_t *retval)
{
	/* {
		syscallarg(const struct linux_sigaltstack *) ss;
		syscallarg(struct linux_sigaltstack *) oss;
	} */
	struct linux_sigaltstack ss;
	struct sigaltstack nss;
	struct proc *p = l->l_proc;
	int error = 0;

	if (SCARG(uap, oss)) {
		native_to_linux_sigaltstack(&ss, &l->l_sigstk);
		if ((error = copyout(&ss, SCARG(uap, oss), sizeof(ss))) != 0)
			return error;
	}

	if (SCARG(uap, ss) != NULL) {
		if ((error = copyin(SCARG(uap, ss), &ss, sizeof(ss))) != 0)
			return error;
		linux_to_native_sigaltstack(&nss, &ss);

		mutex_enter(p->p_lock);

		if (nss.ss_flags & ~SS_ALLBITS)
			error = EINVAL;
		else if (nss.ss_flags & SS_DISABLE) {
			if (l->l_sigstk.ss_flags & SS_ONSTACK)
				error = EINVAL;
		} else if (nss.ss_size < LINUX_MINSIGSTKSZ)
			error = ENOMEM;

		if (error == 0)
			l->l_sigstk = nss;

		mutex_exit(p->p_lock);
	}

	return error;
}
#endif /* LINUX_SS_ONSTACK */

static int
linux_do_tkill(struct lwp *l, int tgid, int tid, int signum)
{
	struct proc *p;
	struct lwp *t;
	ksiginfo_t ksi;
	int error;

	if (signum < 0 || signum >= LINUX__NSIG)
		return EINVAL;
	signum = linux_to_native_signo[signum];

	if (tgid == -1) {
		tgid = tid;
	}

	KSI_INIT(&ksi);
	ksi.ksi_signo = signum;
	ksi.ksi_code = SI_LWP;
	ksi.ksi_pid = l->l_proc->p_pid;
	ksi.ksi_uid = kauth_cred_geteuid(l->l_cred);
	ksi.ksi_lid = tid;

	mutex_enter(proc_lock);
	p = proc_find(tgid);
	if (p == NULL) {
		mutex_exit(proc_lock);
		return ESRCH;
	}
	mutex_enter(p->p_lock);
	error = kauth_authorize_process(l->l_cred,
	    KAUTH_PROCESS_SIGNAL, p, KAUTH_ARG(signum), NULL, NULL);
	if ((t = lwp_find(p, ksi.ksi_lid)) == NULL)
		error = ESRCH;
	else if (signum != 0)
		kpsignal2(p, &ksi);
	mutex_exit(p->p_lock);
	mutex_exit(proc_lock);

	return error;
}

int
linux_sys_tkill(struct lwp *l, const struct linux_sys_tkill_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) tid;
		syscallarg(int) sig;
	} */

	if (SCARG(uap, tid) <= 0)
		return EINVAL;

	return linux_do_tkill(l, -1, SCARG(uap, tid), SCARG(uap, sig));
}

int
linux_sys_tgkill(struct lwp *l, const struct linux_sys_tgkill_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) tgid;
		syscallarg(int) tid;
		syscallarg(int) sig;
	} */

	if (SCARG(uap, tid) <= 0 || SCARG(uap, tgid) < -1)
		return EINVAL;

	return linux_do_tkill(l, SCARG(uap, tgid), SCARG(uap, tid), SCARG(uap, sig));
}

int
native_to_linux_si_code(int code)
{
	int si_codes[] = {
	    LINUX_SI_USER, LINUX_SI_QUEUE, LINUX_SI_TIMER, LINUX_SI_ASYNCIO,
	    LINUX_SI_MESGQ, LINUX_SI_TKILL /* SI_LWP */
	};

	if (code <= 0 && -code < __arraycount(si_codes))
		return si_codes[-code];

	return code;
}

int
native_to_linux_si_status(int code, int status)
{
	int sts;

	switch (code) {
	case CLD_CONTINUED:
		sts = LINUX_SIGCONT;
		break;
	case CLD_EXITED:
		sts = WEXITSTATUS(status);
		break;
	case CLD_STOPPED:
	case CLD_TRAPPED:
	case CLD_DUMPED:
	case CLD_KILLED:
	default:
		sts = native_to_linux_signo[WTERMSIG(status)];
		break;
	}

	return sts;
}
