/*	$NetBSD: ibcs2_signal.c,v 1.25.8.1 2007/07/11 20:04:01 mjf Exp $	*/

/*
 * Copyright (c) 1995 Scott Bartram
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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_signal.c,v 1.25.8.1 2007/07/11 20:04:01 mjf Exp $");

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

#include <sys/syscallargs.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_util.h>
#include <compat/common/compat_sigaltstack.h>

#define	ibcs2_sigmask(n)	(1 << ((n) - 1))
#define ibcs2_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define ibcs2_sigismember(s, n)	(*(s) & ibcs2_sigmask(n))
#define ibcs2_sigaddset(s, n)	(*(s) |= ibcs2_sigmask(n))

extern const int native_to_ibcs2_signo[];
extern const int ibcs2_to_native_signo[];

void ibcs2_to_native_sigaction __P((const struct ibcs2_sigaction *, struct sigaction *));
void native_to_ibcs2_sigaction __P((const struct sigaction *, struct ibcs2_sigaction *));

void
ibcs2_to_native_sigset(iss, bss)
	const ibcs2_sigset_t *iss;
	sigset_t *bss;
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < IBCS2_NSIG; i++) {
		if (ibcs2_sigismember(iss, i)) {
			newsig = ibcs2_to_native_signo[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}

void
native_to_ibcs2_sigset(bss, iss)
	const sigset_t *bss;
	ibcs2_sigset_t *iss;
{
	int i, newsig;

	ibcs2_sigemptyset(iss);
	for (i = 1; i < NSIG; i++) {
		if (sigismember(bss, i)) {
			newsig = native_to_ibcs2_signo[i];
			if (newsig)
				ibcs2_sigaddset(iss, newsig);
		}
	}
}

void
ibcs2_to_native_sigaction(isa, bsa)
	const struct ibcs2_sigaction *isa;
	struct sigaction *bsa;
{

	bsa->sa_handler = isa->ibcs2_sa_handler;
	ibcs2_to_native_sigset(&isa->ibcs2_sa_mask, &bsa->sa_mask);
	bsa->sa_flags = 0;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_NOCLDSTOP) != 0)
		bsa->sa_flags |= SA_NOCLDSTOP;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_RESETHAND) != 0)
		bsa->sa_flags |= SA_RESETHAND;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_RESTART) != 0)
		bsa->sa_flags |= SA_RESTART;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_SIGINFO) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: SA_SIGINFO ignored\n");
	if ((isa->ibcs2_sa_flags & IBCS2_SA_NODEFER) != 0)
		bsa->sa_flags |= SA_NODEFER;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_ONSTACK) != 0)
		bsa->sa_flags |= SA_ONSTACK;
	if ((isa->ibcs2_sa_flags & IBCS2_SA_NOCLDWAIT) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: SA_NOCLDWAIT ignored\n");
	if ((isa->ibcs2_sa_flags & ~IBCS2_SA_ALLBITS) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: extra bits ignored\n");
}

void
native_to_ibcs2_sigaction(bsa, isa)
	const struct sigaction *bsa;
	struct ibcs2_sigaction *isa;
{

	isa->ibcs2_sa_handler = bsa->sa_handler;
	native_to_ibcs2_sigset(&bsa->sa_mask, &isa->ibcs2_sa_mask);
	isa->ibcs2_sa_flags = 0;
	if ((bsa->sa_flags & SA_NOCLDSTOP) != 0)
		isa->ibcs2_sa_flags |= IBCS2_SA_NOCLDSTOP;
	if ((bsa->sa_flags & SA_RESETHAND) != 0)
		isa->ibcs2_sa_flags |= IBCS2_SA_RESETHAND;
	if ((bsa->sa_flags & SA_RESTART) != 0)
		isa->ibcs2_sa_flags |= IBCS2_SA_RESTART;
	if ((bsa->sa_flags & SA_NODEFER) != 0)
		isa->ibcs2_sa_flags |= IBCS2_SA_NODEFER;
	if ((bsa->sa_flags & SA_ONSTACK) != 0)
		isa->ibcs2_sa_flags |= IBCS2_SA_ONSTACK;
}


int
ibcs2_sys_sigaction(struct lwp *l, void *v, register_t *retval)
{
	struct ibcs2_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct ibcs2_sigaction *) nsa;
		syscallarg(struct ibcs2_sigaction *) osa;
	} */ *uap = v;
	struct ibcs2_sigaction nisa, oisa;
	struct sigaction nbsa, obsa;
	int error, signum = SCARG(uap, signum);

	if (signum < 0 || signum >= IBCS2_NSIG)
		return EINVAL;
	signum = ibcs2_to_native_signo[signum];

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nisa, sizeof(nisa));
		if (error)
			return (error);
		ibcs2_to_native_sigaction(&nisa, &nbsa);
	}
	error = sigaction1(l, signum,
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		native_to_ibcs2_sigaction(&obsa, &oisa);
		error = copyout(&oisa, SCARG(uap, osa), sizeof(oisa));
		if (error)
			return (error);
	}
	return (0);
}

int
ibcs2_sys_sigaltstack(struct lwp *l, void *v, register_t *retval)
{
	struct ibcs2_sys_sigaltstack_args /* {
		syscallarg(const struct ibcs2_sigaltstack *) nss;
		syscallarg(struct ibcs2_sigaltstack *) oss;
	} */ *uap = v;
	compat_sigaltstack(uap, ibcs2_sigaltstack,
	    IBCS2_SS_ONSTACK, IBCS2_SS_DISABLE);
}

int
ibcs2_sys_sigsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sigsys_args /* {
		syscallarg(int) sig;
		syscallarg(ibcs2_sig_t) fp;
	} */ *uap = v;
	struct sigaction nbsa, obsa;
	sigset_t ss;
	int error, signum = IBCS2_SIGNO(SCARG(uap, sig));

	if (signum < 0 || signum >= IBCS2_NSIG)
		return EINVAL;
	signum = ibcs2_to_native_signo[signum];

	switch (IBCS2_SIGCALL(SCARG(uap, sig))) {
	case IBCS2_SIGSET_MASK:
		if (SCARG(uap, fp) == IBCS2_SIG_HOLD)
			goto sighold;
		/* FALLTHROUGH */

	case IBCS2_SIGNAL_MASK:
		nbsa.sa_handler = (sig_t)SCARG(uap, fp);
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		error = sigaction1(l, signum, &nbsa, &obsa, NULL, 0);
		if (error)
			return (error);
		*retval = (int)obsa.sa_handler;
		return (0);

	case IBCS2_SIGHOLD_MASK:
	sighold:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		return (sigprocmask1(l, SIG_BLOCK, &ss, 0));

	case IBCS2_SIGRELSE_MASK:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		return (sigprocmask1(l, SIG_UNBLOCK, &ss, 0));

	case IBCS2_SIGIGNORE_MASK:
		nbsa.sa_handler = SIG_IGN;
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		return (sigaction1(l, signum, &nbsa, 0, NULL, 0));

	case IBCS2_SIGPAUSE_MASK:
		ss = l->l_sigmask;
		sigdelset(&ss, signum);
		return (sigsuspend1(l, &ss));

	default:
		return (ENOSYS);
	}
}

int
ibcs2_sys_sigprocmask(struct lwp *l, void *v, register_t *retval)
{
	struct ibcs2_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(const ibcs2_sigset_t *) set;
		syscallarg(ibcs2_sigset_t *) oset;
	} */ *uap = v;
	ibcs2_sigset_t niss, oiss;
	sigset_t nbss, obss;
	int how;
	int error;

	switch (SCARG(uap, how)) {
	case IBCS2_SIG_BLOCK:
		how = SIG_BLOCK;
		break;
	case IBCS2_SIG_UNBLOCK:
		how = SIG_UNBLOCK;
		break;
	case IBCS2_SIG_SETMASK:
		how = SIG_SETMASK;
		break;
	default:
		return (EINVAL);
	}

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &niss, sizeof(niss));
		if (error)
			return (error);
		ibcs2_to_native_sigset(&niss, &nbss);
	}
	error = sigprocmask1(l, how,
	    SCARG(uap, set) ? &nbss : 0, SCARG(uap, oset) ? &obss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oset)) {
		native_to_ibcs2_sigset(&obss, &oiss);
		error = copyout(&oiss, SCARG(uap, oset), sizeof(oiss));
		if (error)
			return (error);
	}
	return (0);
}

int
ibcs2_sys_sigpending(struct lwp *l, void *v, register_t *retval)
{
	struct ibcs2_sys_sigpending_args /* {
		syscallarg(ibcs2_sigset_t *) set;
	} */ *uap = v;
	sigset_t bss;
	ibcs2_sigset_t iss;

	sigpending1(l, &bss);
	native_to_ibcs2_sigset(&bss, &iss);
	return (copyout(&iss, SCARG(uap, set), sizeof(iss)));
}

int
ibcs2_sys_sigsuspend(struct lwp *l, void *v, register_t *retval)
{
	struct ibcs2_sys_sigsuspend_args /* {
		syscallarg(const ibcs2_sigset_t *) set;
	} */ *uap = v;
	ibcs2_sigset_t sss;
	sigset_t bss;
	int error;

	if (SCARG(uap, set)) {
		error = copyin(SCARG(uap, set), &sss, sizeof(sss));
		if (error)
			return (error);
		ibcs2_to_native_sigset(&sss, &bss);
	}

	return (sigsuspend1(l, SCARG(uap, set) ? &bss : 0));
}

int
ibcs2_sys_pause(struct lwp *l, void *v, register_t *retval)
{

	return (sigsuspend1(l, 0));
}

int
ibcs2_sys_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signo;
	} */ *uap = v;
	struct sys_kill_args ka;
	int signum = SCARG(uap, signo);

	if (signum < 0 || signum >= IBCS2_NSIG)
		return EINVAL;
	signum = ibcs2_to_native_signo[signum];

	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = signum;
	return sys_kill(l, &ka, retval);
}
