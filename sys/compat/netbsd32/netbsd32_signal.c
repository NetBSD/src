/*	$NetBSD: netbsd32_signal.c,v 1.21.2.3 2007/07/15 13:27:14 ad Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_signal.c,v 1.21.2.3 2007/07/15 13:27:14 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signalvar.h>
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

#ifdef unused
static void netbsd32_si32_to_si(siginfo_t *, const siginfo32_t *);
#endif


int
netbsd32_sigaction(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
	} */ *uap = v;
	struct sigaction nsa, osa;
	struct netbsd32_sigaction *sa32p, sa32;
	int error;

	if (SCARG_P32(uap, nsa)) {
		sa32p = SCARG_P32(uap, nsa);
		if (copyin(sa32p, &sa32, sizeof(sa32)))
			return EFAULT;
		nsa.sa_handler = (void *)NETBSD32PTR64(sa32.netbsd32_sa_handler);
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
		sa32p = SCARG_P32(uap, osa);
		if (copyout(&sa32, sa32p, sizeof(sa32)))
			return EFAULT;
	}

	return (0);
}

int
netbsd32___sigaltstack14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaltstack14_args /* {
		syscallarg(const netbsd32_sigaltstackp_t) nss;
		syscallarg(netbsd32_sigaltstackp_t) oss;
	} */ *uap = v;
	compat_sigaltstack(uap, netbsd32_sigaltstack, SS_ONSTACK, SS_DISABLE);
}

/* ARGSUSED */
int
netbsd32___sigaction14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaction14_args /* {
		syscallarg(int) signum;
		syscallarg(const struct sigaction *) nsa;
		syscallarg(struct sigaction *) osa;
	} */ *uap = v;
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
netbsd32___sigaction_sigtramp(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaction_sigtramp_args /* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
		syscallarg(netbsd32_voidp) tramp;
		syscallarg(int) vers;
	} */ *uap = v;
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

#ifdef unused
static void
netbsd32_si32_to_si(siginfo_t *si, const siginfo32_t *si32)
{
	memset(si, 0, sizeof (*si));
	si->si_signo = si32->si_signo;
	si->si_code = si32->si_code;
	si->si_errno = si32->si_errno;

	switch (si32->si_signo) {
	case SIGILL:
	case SIGBUS:
	case SIGSEGV:
	case SIGFPE:
	case SIGTRAP:
		si->si_addr = NETBSD32PTR64(si32->si_addr);
		si->si_trap = si32->si_trap;
		break;
	case SIGALRM:
	case SIGVTALRM:
	case SIGPROF:
		si->si_pid = si32->si_pid;
		si->si_uid = si32->si_uid;
		/*
		 * XXX sival_ptr is currently unused.
		 */
		si->si_value.sival_int = si32->si_value.sival_int;
		break;
	case SIGCHLD:
		si->si_pid = si32->si_pid;
		si->si_uid = si32->si_uid;
		si->si_utime = si32->si_utime;
		si->si_stime = si32->si_stime;
		break;
	case SIGURG:
	case SIGIO:
		si->si_band = si32->si_band;
		si->si_fd = si32->si_fd;
		break;
	}
}
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
		si32->si_pid = si->si_pid;
		si32->si_uid = si->si_uid;
		/*
		 * XXX sival_ptr is currently unused.
		 */
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

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

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
	mutex_exit(&p->p_smutex);
	cpu_getmcontext32(l, &ucp->uc_mcontext, &ucp->uc_flags);
	mutex_enter(&p->p_smutex);
}

/* ARGSUSED */
int
netbsd32_getcontext(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_getcontext_args /* {
		syscallarg(netbsd32_ucontextp) ucp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	ucontext32_t uc;

	mutex_enter(&p->p_smutex);
	getucontext32(l, &uc);
	mutex_exit(&p->p_smutex);

	return copyout(&uc, SCARG_P32(uap, ucp), sizeof (ucontext32_t));
}

int
setucontext32(struct lwp *l, const ucontext32_t *ucp)
{
	struct proc *p = l->l_proc;
	int error;

	LOCK_ASSERT(mutex_owned(&p->p_smutex));

	if ((ucp->uc_flags & _UC_SIGMASK) != 0) {
		error = sigprocmask1(l, SIG_SETMASK, &ucp->uc_sigmask, NULL);
		if (error != 0)
			return error;
	}

	mutex_exit(&p->p_smutex);
	error = cpu_setmcontext32(l, &ucp->uc_mcontext, ucp->uc_flags);
	mutex_enter(&p->p_smutex);
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
netbsd32_setcontext(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_setcontext_args /* {
		syscallarg(netbsd32_ucontextp) ucp;
	} */ *uap = v;
	ucontext32_t uc;
	int error;
	struct proc *p = l->l_proc;

	error = copyin(SCARG_P32(uap, ucp), &uc, sizeof (uc));
	if (error)
		return (error);
	if (!(uc.uc_flags & _UC_CPU))
		return (EINVAL);
	mutex_enter(&p->p_smutex);
	error = setucontext32(l, &uc);
	mutex_exit(&p->p_smutex);
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
netbsd32___sigtimedwait(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32___sigtimedwait_args /* {
		syscallarg(netbsd32_sigsetp_t) set;
		syscallarg(netbsd32_siginfop_t) info;
		syscallarg(netbsd32_timespecp_t) timeout;
	} */ *uap = v;
	struct sys___sigtimedwait_args ua;

	NETBSD32TOP_UAP(set, const sigset_t);
	NETBSD32TOP_UAP(info, siginfo_t);
	NETBSD32TOP_UAP(timeout, struct timespec);

	return __sigtimedwait1(l, &ua, retval, netbsd32_sigtimedwait_put_info,
	    netbsd32_sigtimedwait_fetch_timeout,
	    netbsd32_sigtimedwait_put_timeout);
}
