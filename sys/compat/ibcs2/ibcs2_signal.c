/*	$NetBSD: ibcs2_signal.c,v 1.12 2000/12/22 22:58:57 jdolecek Exp $	*/

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

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_util.h>

#define	ibcs2_sigmask(n)	(1 << ((n) - 1))
#define ibcs2_sigemptyset(s)	memset((s), 0, sizeof(*(s)))
#define ibcs2_sigismember(s, n)	(*(s) & ibcs2_sigmask(n))
#define ibcs2_sigaddset(s, n)	(*(s) |= ibcs2_sigmask(n))

int native_to_ibcs2_sig[NSIG] = {
	0,			/* 0 */
	IBCS2_SIGHUP,		/* 1 */
	IBCS2_SIGINT,		/* 2 */
	IBCS2_SIGQUIT,		/* 3 */
	IBCS2_SIGILL,		/* 4 */
	IBCS2_SIGTRAP,		/* 5 */
	IBCS2_SIGABRT,		/* 6 */
	IBCS2_SIGEMT,		/* 7 */
	IBCS2_SIGFPE,		/* 8 */
	IBCS2_SIGKILL,		/* 9 */
	IBCS2_SIGBUS,		/* 10 */
	IBCS2_SIGSEGV,		/* 11 */
	IBCS2_SIGSYS,		/* 12 */
	IBCS2_SIGPIPE,		/* 13 */
	IBCS2_SIGALRM,		/* 14 */
	IBCS2_SIGTERM,		/* 15 */
	0,			/* 16 - SIGURG */
	IBCS2_SIGSTOP,		/* 17 */
	IBCS2_SIGTSTP,		/* 18 */
	IBCS2_SIGCONT,		/* 19 */
	IBCS2_SIGCLD,		/* 20 */
	IBCS2_SIGTTIN,		/* 21 */
	IBCS2_SIGTTOU,		/* 22 */
	IBCS2_SIGPOLL,		/* 23 */
	IBCS2_SIGXCPU,		/* 24 */
	IBCS2_SIGXFSZ,		/* 25 */
	IBCS2_SIGVTALRM,	/* 26 */
	IBCS2_SIGPROF,		/* 27 */
	IBCS2_SIGWINCH,		/* 28 */
	0,			/* 29 - SIGINFO */
	IBCS2_SIGUSR1,		/* 30 */
	IBCS2_SIGUSR2,		/* 31 */
	IBCS2_SIGPWR,		/* 32 */
};

int ibcs2_to_native_sig[IBCS2_NSIG] = {
	0,			/* 0 */
	SIGHUP,			/* 1 */
	SIGINT,			/* 2 */
	SIGQUIT,		/* 3 */
	SIGILL,			/* 4 */
	SIGTRAP,		/* 5 */
	SIGABRT,		/* 6 */
	SIGEMT,			/* 7 */
	SIGFPE,			/* 8 */
	SIGKILL,		/* 9 */
	SIGBUS,			/* 10 */
	SIGSEGV,		/* 11 */
	SIGSYS,			/* 12 */
	SIGPIPE,		/* 13 */
	SIGALRM,		/* 14 */
	SIGTERM,		/* 15 */
	SIGUSR1,		/* 16 */
	SIGUSR2,		/* 17 */
	SIGCHLD,		/* 18 */
	SIGPWR,			/* 19 */
	SIGWINCH,		/* 20 */
	0,			/* 21 - SIGPHONE */
	SIGIO,			/* 22 */
	SIGSTOP,		/* 23 */
	SIGTSTP,		/* 24 */
	SIGCONT,		/* 25 */
	SIGTTIN,		/* 26 */
	SIGTTOU,		/* 27 */
	SIGVTALRM,		/* 28 */
	SIGPROF,		/* 29 */
	SIGXCPU,		/* 30 */
	SIGXFSZ,		/* 31 */
};

void ibcs2_to_native_sigaction __P((const struct ibcs2_sigaction *, struct sigaction *));
void native_to_ibcs2_sigaction __P((const struct sigaction *, struct ibcs2_sigaction *));
void ibcs2_to_native_sigaltstack __P((const struct ibcs2_sigaltstack *, struct sigaltstack *));
void native_to_ibcs2_sigaltstack __P((const struct sigaltstack *, struct ibcs2_sigaltstack *));

void
ibcs2_to_native_sigset(iss, bss)
	const ibcs2_sigset_t *iss;
	sigset_t *bss;
{
	int i, newsig;

	sigemptyset(bss);
	for (i = 1; i < IBCS2_NSIG; i++) {
		if (ibcs2_sigismember(iss, i)) {
			newsig = ibcs2_to_native_sig[i];
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
			newsig = native_to_ibcs2_sig[i];
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

	bsa->sa_handler = isa->sa_handler;
	ibcs2_to_native_sigset(&isa->sa_mask, &bsa->sa_mask);
	bsa->sa_flags = 0;
	if ((isa->sa_flags & IBCS2_SA_NOCLDSTOP) != 0)
		bsa->sa_flags |= SA_NOCLDSTOP;
	if ((isa->sa_flags & IBCS2_SA_RESETHAND) != 0)
		bsa->sa_flags |= SA_RESETHAND;
	if ((isa->sa_flags & IBCS2_SA_RESTART) != 0)
		bsa->sa_flags |= SA_RESTART;
	if ((isa->sa_flags & IBCS2_SA_SIGINFO) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: SA_SIGINFO ignored\n");
	if ((isa->sa_flags & IBCS2_SA_NODEFER) != 0)
		bsa->sa_flags |= SA_NODEFER;
	if ((isa->sa_flags & IBCS2_SA_ONSTACK) != 0)
		bsa->sa_flags |= SA_ONSTACK;
	if ((isa->sa_flags & IBCS2_SA_NOCLDWAIT) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: SA_NOCLDWAIT ignored\n");
	if ((isa->sa_flags & ~IBCS2_SA_ALLBITS) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaction: extra bits ignored\n");
}

void
native_to_ibcs2_sigaction(bsa, isa)
	const struct sigaction *bsa;
	struct ibcs2_sigaction *isa;
{

	isa->sa_handler = bsa->sa_handler;
	native_to_ibcs2_sigset(&bsa->sa_mask, &isa->sa_mask);
	isa->sa_flags = 0;
	if ((bsa->sa_flags & SA_NOCLDSTOP) != 0)
		isa->sa_flags |= IBCS2_SA_NOCLDSTOP;
	if ((bsa->sa_flags & SA_RESETHAND) != 0)
		isa->sa_flags |= IBCS2_SA_RESETHAND;
	if ((bsa->sa_flags & SA_RESTART) != 0)
		isa->sa_flags |= IBCS2_SA_RESTART;
	if ((bsa->sa_flags & SA_NODEFER) != 0)
		isa->sa_flags |= IBCS2_SA_NODEFER;
	if ((bsa->sa_flags & SA_ONSTACK) != 0)
		isa->sa_flags |= IBCS2_SA_ONSTACK;
}

void
ibcs2_to_native_sigaltstack(sss, bss)
	const struct ibcs2_sigaltstack *sss;
	struct sigaltstack *bss;
{

	bss->ss_sp = sss->ss_sp;
	bss->ss_size = sss->ss_size;
	bss->ss_flags = 0;
	if ((sss->ss_flags & IBCS2_SS_DISABLE) != 0)
		bss->ss_flags |= SS_DISABLE;
	if ((sss->ss_flags & IBCS2_SS_ONSTACK) != 0)
		bss->ss_flags |= SS_ONSTACK;
	if ((sss->ss_flags & ~IBCS2_SS_ALLBITS) != 0)
/*XXX*/		printf("ibcs2_to_native_sigaltstack: extra bits ignored\n");
}

void
native_to_ibcs2_sigaltstack(bss, sss)
	const struct sigaltstack *bss;
	struct ibcs2_sigaltstack *sss;
{

	sss->ss_sp = bss->ss_sp;
	sss->ss_size = bss->ss_size;
	sss->ss_flags = 0;
	if ((bss->ss_flags & SS_DISABLE) != 0)
		sss->ss_flags |= IBCS2_SS_DISABLE;
	if ((bss->ss_flags & SS_ONSTACK) != 0)
		sss->ss_flags |= IBCS2_SS_ONSTACK;
}

int
ibcs2_sys_sigaction(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const struct ibcs2_sigaction *) nsa;
		syscallarg(struct ibcs2_sigaction *) osa;
	} */ *uap = v;
	struct ibcs2_sigaction nisa, oisa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nisa, sizeof(nisa));
		if (error)
			return (error);
		ibcs2_to_native_sigaction(&nisa, &nbsa);
	}
	error = sigaction1(p, ibcs2_to_native_sig[SCARG(uap, signum)],
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0);
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
ibcs2_sys_sigaltstack(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sigaltstack_args /* {
		syscallarg(const struct ibcs2_sigaltstack *) nss;
		syscallarg(struct ibcs2_sigaltstack *) oss;
	} */ *uap = v;
	struct ibcs2_sigaltstack nsss, osss;
	struct sigaltstack nbss, obss;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin(SCARG(uap, nss), &nsss, sizeof(nsss));
		if (error)
			return (error);
		ibcs2_to_native_sigaltstack(&nsss, &nbss);
	}
	error = sigaltstack1(p,
	    SCARG(uap, nss) ? &nbss : 0, SCARG(uap, oss) ? &obss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		native_to_ibcs2_sigaltstack(&obss, &osss);
		error = copyout(&osss, SCARG(uap, oss), sizeof(osss));
		if (error)
			return (error);
	}
	return (0);
}

int
ibcs2_sys_sigsys(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sigsys_args /* {
		syscallarg(int) sig;
		syscallarg(ibcs2_sig_t) fp;
	} */ *uap = v;
	int signum = ibcs2_to_native_sig[IBCS2_SIGNO(SCARG(uap, sig))];
	struct sigaction nbsa, obsa;
	sigset_t ss;
	int error;

	if (signum <= 0 || signum >= IBCS2_NSIG)
		return (EINVAL);
	
	switch (IBCS2_SIGCALL(SCARG(uap, sig))) {
	case IBCS2_SIGSET_MASK:
		if (SCARG(uap, fp) == IBCS2_SIG_HOLD)
			goto sighold;
		/* FALLTHROUGH */

	case IBCS2_SIGNAL_MASK:
		nbsa.sa_handler = (sig_t)SCARG(uap, fp);
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		error = sigaction1(p, signum, &nbsa, &obsa);
		if (error)
			return (error);
		*retval = (int)obsa.sa_handler;
		return (0);
		
	case IBCS2_SIGHOLD_MASK:
	sighold:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		return (sigprocmask1(p, SIG_BLOCK, &ss, 0));
		
	case IBCS2_SIGRELSE_MASK:
		sigemptyset(&ss);
		sigaddset(&ss, signum);
		return (sigprocmask1(p, SIG_UNBLOCK, &ss, 0));
		
	case IBCS2_SIGIGNORE_MASK:
		nbsa.sa_handler = SIG_IGN;
		sigemptyset(&nbsa.sa_mask);
		nbsa.sa_flags = 0;
		return (sigaction1(p, signum, &nbsa, 0));
		
	case IBCS2_SIGPAUSE_MASK:
		ss = p->p_sigctx.ps_sigmask;
		sigdelset(&ss, signum);
		return (sigsuspend1(p, &ss));
		
	default:
		return (ENOSYS);
	}
}

int
ibcs2_sys_sigprocmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
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
	error = sigprocmask1(p, how,
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
ibcs2_sys_sigpending(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sigpending_args /* {
		syscallarg(ibcs2_sigset_t *) set;
	} */ *uap = v;
	sigset_t bss;
	ibcs2_sigset_t iss;

	sigpending1(p, &bss);
	native_to_ibcs2_sigset(&bss, &iss);
	return (copyout(&iss, SCARG(uap, set), sizeof(iss)));
}

int
ibcs2_sys_sigsuspend(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
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

	return (sigsuspend1(p, SCARG(uap, set) ? &bss : 0));
}

int
ibcs2_sys_pause(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return (sigsuspend1(p, 0));
}

int
ibcs2_sys_kill(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signo;
	} */ *uap = v;
	struct sys_kill_args ka;

	SCARG(&ka, pid) = SCARG(uap, pid);
	SCARG(&ka, signum) = ibcs2_to_native_sig[SCARG(uap, signo)];
	return sys_kill(p, &ka, retval);
}
