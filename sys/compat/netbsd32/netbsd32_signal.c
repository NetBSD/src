/*	$NetBSD: netbsd32_signal.c,v 1.44 2017/12/06 19:15:27 christos Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_signal.c,v 1.44 2017/12/06 19:15:27 christos Exp $");

#if defined(_KERNEL_OPT) 
#include "opt_ktrace.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signalvar.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/wait.h>
#include <sys/dirent.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#include <compat/sys/siginfo.h>
#include <compat/sys/ucontext.h>
#include <compat/common/compat_sigaltstack.h>

int
netbsd32_sigaction(struct lwp *l, const struct netbsd32_sigaction_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
	} */
	struct sigaction nsa, osa;
	struct netbsd32_sigaction13 *sa32p, sa32;
	int error;

	if (SCARG_P32(uap, nsa)) {
		sa32p = SCARG_P32(uap, nsa);
		if (copyin(sa32p, &sa32, sizeof(sa32)))
			return EFAULT;
		nsa.sa_handler = (void *)NETBSD32PTR64(sa32.netbsd32_sa_handler);
		memset(&nsa.sa_mask, 0, sizeof(nsa.sa_mask));
		nsa.sa_mask.__bits[0] = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(l, SCARG(uap, signum),
			   SCARG_P32(uap, nsa) ? &nsa : 0,
			   SCARG_P32(uap, osa) ? &osa : 0,
			   NULL, 0);

	if (error)
		return (error);

	if (SCARG_P32(uap, osa)) {
		NETBSD32PTR32(sa32.netbsd32_sa_handler, osa.sa_handler);
		sa32.netbsd32_sa_mask = osa.sa_mask.__bits[0];
		sa32.netbsd32_sa_flags = osa.sa_flags;
		sa32p = SCARG_P32(uap, osa);
		if (copyout(&sa32, sa32p, sizeof(sa32)))
			return EFAULT;
	}

	return (0);
}

int
netbsd32___sigaltstack14(struct lwp *l, const struct netbsd32___sigaltstack14_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_sigaltstackp_t) nss;
		syscallarg(netbsd32_sigaltstackp_t) oss;
	} */
	compat_sigaltstack(uap, netbsd32_sigaltstack, SS_ONSTACK, SS_DISABLE);
}

/* ARGSUSED */
int
netbsd32___sigaction14(struct lwp *l, const struct netbsd32___sigaction14_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const struct sigaction *) nsa;
		syscallarg(struct sigaction *) osa;
	} */
	struct netbsd32_sigaction sa32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG_P32(uap, nsa)) {
		error = copyin(SCARG_P32(uap, nsa), &sa32, sizeof(sa32));
		if (error)
			return (error);
		nsa.sa_handler = NETBSD32PTR64(sa32.netbsd32_sa_handler);
		nsa.sa_mask = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(l, SCARG(uap, signum),
		    SCARG_P32(uap, nsa) ? &nsa : 0,
		    SCARG_P32(uap, osa) ? &osa : 0,
		    NULL, 0);
	if (error)
		return (error);
	if (SCARG_P32(uap, osa)) {
		NETBSD32PTR32(sa32.netbsd32_sa_handler, osa.sa_handler);
		sa32.netbsd32_sa_mask = osa.sa_mask;
		sa32.netbsd32_sa_flags = osa.sa_flags;
		error = copyout(&sa32, SCARG_P32(uap, osa), sizeof(sa32));
		if (error)
			return (error);
	}
	return (0);
}

/* ARGSUSED */
int
netbsd32___sigaction_sigtramp(struct lwp *l, const struct netbsd32___sigaction_sigtramp_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
		syscallarg(netbsd32_voidp) tramp;
		syscallarg(int) vers;
	} */
	struct netbsd32_sigaction sa32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG_P32(uap, nsa)) {
		error = copyin(SCARG_P32(uap, nsa), &sa32, sizeof(sa32));
		if (error)
			return (error);
		nsa.sa_handler = NETBSD32PTR64(sa32.netbsd32_sa_handler);
		nsa.sa_mask = sa32.netbsd32_sa_mask;
		nsa.sa_flags = sa32.netbsd32_sa_flags;
	}
	error = sigaction1(l, SCARG(uap, signum),
	    SCARG_P32(uap, nsa) ? &nsa : 0,
	    SCARG_P32(uap, osa) ? &osa : 0,
	    SCARG_P32(uap, tramp), SCARG(uap, vers));
	if (error)
		return (error);
	if (SCARG_P32(uap, osa)) {
		NETBSD32PTR32(sa32.netbsd32_sa_handler, osa.sa_handler);
		sa32.netbsd32_sa_mask = osa.sa_mask;
		sa32.netbsd32_sa_flags = osa.sa_flags;
		error = copyout(&sa32, SCARG_P32(uap, osa), sizeof(sa32));
		if (error)
			return (error);
	}
	return (0);
}

void
netbsd32_ksi32_to_ksi(struct _ksiginfo *si, const struct __ksiginfo32 *si32)
{
	memset(si, 0, sizeof (*si));
	si->_signo = si32->_signo;
	si->_code = si32->_code;
	si->_errno = si32->_errno;

	switch (si32->_signo) {
	case SIGILL:
	case SIGBUS:
	case SIGSEGV:
	case SIGFPE:
	case SIGTRAP:
		si->_reason._fault._addr =
		    NETBSD32IPTR64(si32->_reason._fault._addr);
		si->_reason._fault._trap = si32->_reason._fault._trap;
		break;
	case SIGALRM:
	case SIGVTALRM:
	case SIGPROF:
	default:	/* see sigqueue() and kill1() */
		si->_reason._rt._pid = si32->_reason._rt._pid;
		si->_reason._rt._uid = si32->_reason._rt._uid;
		si->_reason._rt._value.sival_int =
		    si32->_reason._rt._value.sival_int;
		break;
	case SIGCHLD:
		si->_reason._child._pid = si32->_reason._child._pid;
		si->_reason._child._uid = si32->_reason._child._uid;
		si->_reason._child._utime = si32->_reason._child._utime;
		si->_reason._child._stime = si32->_reason._child._stime;
		break;
	case SIGURG:
	case SIGIO:
		si->_reason._poll._band = si32->_reason._poll._band;
		si->_reason._poll._fd = si32->_reason._poll._fd;
		break;
	}
}

#ifdef notyet
#ifdef KTRACE
static void
netbsd32_ksi_to_ksi32(struct __ksiginfo32 *si32, const struct _ksiginfo *si)
{
	memset(si32, 0, sizeof (*si32));
	si32->_signo = si->_signo;
	si32->_code = si->_code;
	si32->_errno = si->_errno;

	switch (si->_signo) {
	case SIGILL:
	case SIGBUS:
	case SIGSEGV:
	case SIGFPE:
	case SIGTRAP:
		si32->_reason._fault._addr =
		    NETBSD32PTR32I(si->_reason._fault._addr);
		si32->_reason._fault._trap = si->_reason._fault._trap;
		break;
	case SIGALRM:
	case SIGVTALRM:
	case SIGPROF:
	default:	/* see sigqueue() and kill1() */
		si32->_reason._rt._pid = si->_reason._rt._pid;
		si32->_reason._rt._uid = si->_reason._rt._uid;
		si32->_reason._rt._value.sival_int =
		    si->_reason._rt._value.sival_int;
		break;
	case SIGCHLD:
		si32->_reason._child._pid = si->_reason._child._pid;
		si32->_reason._child._uid = si->_reason._child._uid;
		si32->_reason._child._utime = si->_reason._child._utime;
		si32->_reason._child._stime = si->_reason._child._stime;
		break;
	case SIGURG:
	case SIGIO:
		si32->_reason._poll._band = si->_reason._poll._band;
		si32->_reason._poll._fd = si->_reason._poll._fd;
		break;
	}
}
#endif
#endif

void
netbsd32_si_to_si32(siginfo32_t *si32, const siginfo_t *si)
{
	memset(si32, 0, sizeof (*si32));
	si32->si_signo = si->si_signo;
	si32->si_code = si->si_code;
	si32->si_errno = si->si_errno;

	switch (si32->si_signo) {
	case 0:	/* SA */
		si32->si_value.sival_int = si->si_value.sival_int;
		break;
	case SIGILL:
	case SIGBUS:
	case SIGSEGV:
	case SIGFPE:
	case SIGTRAP:
		si32->si_addr = (uint32_t)(uintptr_t)si->si_addr;
		si32->si_trap = si->si_trap;
		break;
	case SIGALRM:
	case SIGVTALRM:
	case SIGPROF:
	default:
		si32->si_pid = si->si_pid;
		si32->si_uid = si->si_uid;
		si32->si_value.sival_int = si->si_value.sival_int;
		break;
	case SIGCHLD:
		si32->si_pid = si->si_pid;
		si32->si_uid = si->si_uid;
		si32->si_status = si->si_status;
		si32->si_utime = si->si_utime;
		si32->si_stime = si->si_stime;
		break;
	case SIGURG:
	case SIGIO:
		si32->si_band = si->si_band;
		si32->si_fd = si->si_fd;
		break;
	}
}

void
getucontext32(struct lwp *l, ucontext32_t *ucp)
{
	struct proc *p = l->l_proc;

	KASSERT(mutex_owned(p->p_lock));

	ucp->uc_flags = 0;
	ucp->uc_link = (uint32_t)(intptr_t)l->l_ctxlink;
	ucp->uc_sigmask = l->l_sigmask;
	ucp->uc_flags |= _UC_SIGMASK;

	/*
	 * The (unsupplied) definition of the `current execution stack'
	 * in the System V Interface Definition appears to allow returning
	 * the main context stack.
	 */
	if ((l->l_sigstk.ss_flags & SS_ONSTACK) == 0) {
		ucp->uc_stack.ss_sp = USRSTACK32;
		ucp->uc_stack.ss_size = ctob(p->p_vmspace->vm_ssize);
		ucp->uc_stack.ss_flags = 0;	/* XXX, def. is Very Fishy */
	} else {
		/* Simply copy alternate signal execution stack. */
		ucp->uc_stack.ss_sp =
		    (uint32_t)(intptr_t)l->l_sigstk.ss_sp;
		ucp->uc_stack.ss_size = l->l_sigstk.ss_size;
		ucp->uc_stack.ss_flags = l->l_sigstk.ss_flags;
	}
	ucp->uc_flags |= _UC_STACK;
	mutex_exit(p->p_lock);
	cpu_getmcontext32(l, &ucp->uc_mcontext, &ucp->uc_flags);
	mutex_enter(p->p_lock);
}

int
netbsd32_getcontext(struct lwp *l, const struct netbsd32_getcontext_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_ucontextp) ucp;
	} */
	struct proc *p = l->l_proc;
	ucontext32_t uc;

	memset(&uc, 0, sizeof(uc));

	mutex_enter(p->p_lock);
	getucontext32(l, &uc);
	mutex_exit(p->p_lock);

	return copyout(&uc, SCARG_P32(uap, ucp), sizeof (ucontext32_t));
}

int
setucontext32(struct lwp *l, const ucontext32_t *ucp)
{
	struct proc *p = l->l_proc;
	int error;

	KASSERT(mutex_owned(p->p_lock));

	if ((ucp->uc_flags & _UC_SIGMASK) != 0) {
		error = sigprocmask1(l, SIG_SETMASK, &ucp->uc_sigmask, NULL);
		if (error != 0)
			return error;
	}

	mutex_exit(p->p_lock);
	error = cpu_setmcontext32(l, &ucp->uc_mcontext, ucp->uc_flags);
	mutex_enter(p->p_lock);
	if (error != 0)
		return (error);

	l->l_ctxlink = (void *)(intptr_t)ucp->uc_link;

	/*
	 * If there was stack information, update whether or not we are
	 * still running on an alternate signal stack.
	 */
	if ((ucp->uc_flags & _UC_STACK) != 0) {
		if (ucp->uc_stack.ss_flags & SS_ONSTACK)
			l->l_sigstk.ss_flags |= SS_ONSTACK;
		else
			l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	}

	return 0;
}

/* ARGSUSED */
int
netbsd32_setcontext(struct lwp *l, const struct netbsd32_setcontext_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_ucontextp) ucp;
	} */
	ucontext32_t uc;
	int error;
	struct proc *p = l->l_proc;

	error = copyin(SCARG_P32(uap, ucp), &uc, sizeof (uc));
	if (error)
		return (error);
	if (!(uc.uc_flags & _UC_CPU))
		return (EINVAL);
	mutex_enter(p->p_lock);
	error = setucontext32(l, &uc);
	mutex_exit(p->p_lock);
	if (error)
		return (error);

	return (EJUSTRETURN);
}

static int
netbsd32_sigtimedwait_put_info(const void *src, void *dst, size_t size)
{
	const siginfo_t *info = src;
	siginfo32_t info32;

	netbsd32_si_to_si32(&info32, info);

	return copyout(&info32, dst, sizeof(info32));
}

static int
netbsd32_sigtimedwait_fetch_timeout(const void *src, void *dst, size_t size)
{
	struct timespec *ts = dst;
	struct netbsd32_timespec ts32;
	int error;

	error = copyin(src, &ts32, sizeof(ts32));
	if (error)
		return error;

	netbsd32_to_timespec(&ts32, ts);
	return 0;
}

static int
netbsd32_sigtimedwait_put_timeout(const void *src, void *dst, size_t size)
{
	const struct timespec *ts = src;
	struct netbsd32_timespec ts32;

	netbsd32_from_timespec(ts, &ts32);

	return copyout(&ts32, dst, sizeof(ts32));
}

int
netbsd32_____sigtimedwait50(struct lwp *l, const struct netbsd32_____sigtimedwait50_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_sigsetp_t) set;
		syscallarg(netbsd32_siginfop_t) info;
		syscallarg(netbsd32_timespec50p_t) timeout;
	} */
	struct sys_____sigtimedwait50_args ua;

	NETBSD32TOP_UAP(set, const sigset_t);
	NETBSD32TOP_UAP(info, siginfo_t);
	NETBSD32TOP_UAP(timeout, struct timespec);

	return sigtimedwait1(l, &ua, retval,
	    copyin,
	    netbsd32_sigtimedwait_put_info,
	    netbsd32_sigtimedwait_fetch_timeout,
	    netbsd32_sigtimedwait_put_timeout);
}

int
netbsd32_sigqueueinfo(struct lwp *l,
    const struct netbsd32_sigqueueinfo_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(const netbsd32_siginfop_t) info;
	} */
	struct __ksiginfo32 ksi32;
	ksiginfo_t ksi;
	int error;

	if ((error = copyin(SCARG_P32(uap, info), &ksi32,
	    sizeof(ksi32))) != 0)
		return error;

	KSI_INIT(&ksi);
	netbsd32_ksi32_to_ksi(&ksi.ksi_info, &ksi32);

	return kill1(l, SCARG(uap, pid), &ksi, retval);
}

struct netbsd32_ktr_psig {
	int			signo;
	netbsd32_pointer_t	action;
	sigset_t		mask;
	int			code;
	/* and optional siginfo_t */
};

#ifdef notyet
#ifdef KTRACE
void
netbsd32_ktrpsig(int sig, sig_t action, const sigset_t *mask,
	 const ksiginfo_t *ksi)
{
	struct ktrace_entry *kte;
	lwp_t *l = curlwp;
	struct {
		struct netbsd32_ktr_psig	kp;
		siginfo32_t			si;
	} *kbuf;

	if (!KTRPOINT(l->l_proc, KTR_PSIG))
		return;

	if (ktealloc(&kte, (void *)&kbuf, l, KTR_PSIG, sizeof(*kbuf)))
		return;

	kbuf->kp.signo = (char)sig;
	NETBSD32PTR32(kbuf->kp.action, action);
	kbuf->kp.mask = *mask;

	if (ksi) {
		kbuf->kp.code = KSI_TRAPCODE(ksi);
		(void)memset(&kbuf->si, 0, sizeof(kbuf->si));
		netbsd32_ksi_to_ksi32(&kbuf->si._info, &ksi->ksi_info);
		ktesethdrlen(kte, sizeof(*kbuf));
	} else {
		kbuf->kp.code = 0;
		ktesethdrlen(kte, sizeof(struct netbsd32_ktr_psig));
	}

	ktraddentry(l, kte, KTA_WAITOK);
}
#endif
#endif
