/*	$NetBSD: svr4_signal.c,v 1.8 1995/02/01 01:37:34 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
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
#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>

static int  svr4_to_bsd_signum __P((int sn));
static void bsd_to_svr4_sigaction __P((const struct sigaction *bsa, 
				       struct svr4_sigaction *ssa));
static void svr4_to_bsd_sigaction __P((const struct svr4_sigaction *ssa,
				       struct sigaction *bsa));
static int  svr4_to_bsd_sigaction_flags __P((int flgs));
static int  bsd_to_svr4_sigaction_flags __P((int flgs));
static int  svr4_sigismember __P((const svr4_sigset_t *ss, int sn));
static int  svr4_sigaddset __P((svr4_sigset_t *ss, int sn));

#define SVR4_SSN (sizeof(ss->bits[0]) * 8)

#define sigemptyset(bs)	bzero((bs), sizeof(*(bs)))
#define sigaddset(bs, n)	*(bs) |= sigmask(n)
#define sigismember(bs, n)	(*(bs) & sigmask(n)) != 0
#define svr4_sigemptyset(ss)	bzero((ss), sizeof(*(ss)))

static int
svr4_sigismember(ss, sn)
	const svr4_sigset_t *ss;
	int sn;
{
	int i = sn / SVR4_SSN;
	if (sn >= SVR4_NSIG || sn < 0)
		return 0;
	return (ss->bits[i] & (1 << ((sn - (i * SVR4_SSN)) - 1))) != 0;

}


static int
svr4_sigaddset(ss, sn)
	svr4_sigset_t *ss;
	int sn;
{
	int i = sn / SVR4_SSN;
	if (sn >= SVR4_NSIG || sn < 0)
		return EINVAL;
	ss->bits[i] |= (1 << ((sn - (i * SVR4_SSN)) - 1));
	return 0;
}


static int
svr4_to_bsd_signum(sn)
	int	sn;
{
	switch (sn) {
	case SVR4_SIGHUP:
		return SIGHUP;
	case SVR4_SIGINT:
		return SIGINT;
	case SVR4_SIGQUIT:
		return SIGQUIT;
	case SVR4_SIGILL:
		return SIGILL;
	case SVR4_SIGTRAP:
		return SIGTRAP;
	case SVR4_SIGABRT:
		return SIGABRT;
	case SVR4_SIGEMT:
		return SIGEMT;
	case SVR4_SIGFPE:
		return SIGFPE;
	case SVR4_SIGKILL:
		return SIGKILL;
	case SVR4_SIGBUS:
		return SIGBUS;
	case SVR4_SIGSEGV:
		return SIGSEGV;
	case SVR4_SIGSYS:
		return SIGSYS;
	case SVR4_SIGPIPE:
		return SIGPIPE;
	case SVR4_SIGALRM:
		return SIGALRM;
	case SVR4_SIGTERM:
		return SIGTERM;
	case SVR4_SIGUSR1:
		return SIGUSR1;
	case SVR4_SIGUSR2:
		return SIGUSR2;
	case SVR4_SIGCHLD:
		return SIGCHLD;
	case SVR4_SIGPWR:
		return NSIG;
	case SVR4_SIGWINCH:
		return SIGWINCH;
	case SVR4_SIGURG:
		return SIGURG;
	case SVR4_SIGIO:
		return SIGIO;
	case SVR4_SIGSTOP:
		return SIGSTOP;
	case SVR4_SIGTSTP:
		return SIGTSTP;
	case SVR4_SIGCONT:
		return SIGCONT;
	case SVR4_SIGTTIN:
		return SIGTTIN;
	case SVR4_SIGTTOU:
		return SIGTTOU;
	case SVR4_SIGVTALRM:
		return SIGVTALRM;
	case SVR4_SIGPROF:
		return SIGPROF;
	case SVR4_SIGXCPU:
		return SIGXCPU;
	case SVR4_SIGXFSZ:
		return SIGXFSZ;
	default:
		return NSIG;
	}
}

int
bsd_to_svr4_signum(sn)
	int	sn;
{
	switch (sn) {
	case SIGHUP:
		return SVR4_SIGHUP;
	case SIGINT:
		return SVR4_SIGINT;
	case SIGQUIT:
		return SVR4_SIGQUIT;
	case SIGILL:
		return SVR4_SIGILL;
	case SIGTRAP:
		return SVR4_SIGTRAP;
	case SIGABRT:
		return SVR4_SIGABRT;
	case SIGEMT:
		return SVR4_SIGEMT;
	case SIGFPE:
		return SVR4_SIGFPE;
	case SIGKILL:
		return SVR4_SIGKILL;
	case SIGBUS:
		return SVR4_SIGBUS;
	case SIGSEGV:
		return SVR4_SIGSEGV;
	case SIGSYS:
		return SVR4_SIGSYS;
	case SIGPIPE:
		return SVR4_SIGPIPE;
	case SIGALRM:
		return SVR4_SIGALRM;
	case SIGTERM:
		return SVR4_SIGTERM;
	case SIGURG:
		return SVR4_SIGURG;
	case SIGSTOP:
		return SVR4_SIGSTOP;
	case SIGTSTP:
		return SVR4_SIGTSTP;
	case SIGCONT:
		return SVR4_SIGCONT;
	case SIGCHLD:
		return SVR4_SIGCHLD;
	case SIGTTIN:
		return SVR4_SIGTTIN;
	case SIGTTOU:
		return SVR4_SIGTTOU;
	case SIGIO:
		return SVR4_SIGIO;
	case SIGXCPU:
		return SVR4_SIGXCPU;
	case SIGXFSZ:
		return SVR4_SIGXFSZ;
	case SIGVTALRM:
		return SVR4_SIGVTALRM;
	case SIGWINCH:
		return SVR4_SIGWINCH;
	case SIGINFO:
		return SVR4_NSIG;
	case SIGUSR1:
		return SVR4_SIGUSR1;
	case SIGUSR2:
		return SVR4_SIGUSR2;
	default:
		return SVR4_NSIG;
	}
}

void
svr4_to_bsd_sigset_t(ss, bs)
	const svr4_sigset_t 	*ss;
	sigset_t 		*bs;
{
	int i;
	sigemptyset(bs);
	for (i = 0; i < SVR4_NSIG; i++)
		if (svr4_sigismember(ss, i))
			sigaddset(bs, svr4_to_bsd_signum(i));
}


void
bsd_to_svr4_sigset_t(bs, ss)
	const sigset_t 	*bs;
	svr4_sigset_t 	*ss;
{
	int i;
	svr4_sigemptyset(ss);
	for (i = 0; i < NSIG; i++)
		if (sigismember(bs, i))
			svr4_sigaddset(ss, bsd_to_svr4_signum(i));
}

/*
 * XXX: Only a subset of the flags is currently implemented.
 */
static int
bsd_to_svr4_sigaction_flags(flgs)
	int flgs;
{
	int rv = 0;

#define SETFLAG(_a)	if (flgs & __CONCAT(SA_,_a)) rv |= __CONCAT(SVR4_SA_,_a)
	SETFLAG(ONSTACK);
	SETFLAG(RESTART);
	SETFLAG(NOCLDSTOP);
	return rv;
#undef SETFLAG
}


static int
svr4_to_bsd_sigaction_flags(flgs)
	int flgs;
{
	int rv = 0;

#define SETFLAG(_a)	if (flgs & __CONCAT(SVR4_SA_,_a)) rv |= __CONCAT(SA_,_a)
	SETFLAG(ONSTACK);
	SETFLAG(RESTART);
	SETFLAG(NOCLDSTOP);
	return rv;
#undef SETFLAG
}


static void
bsd_to_svr4_sigaction(bsa, ssa)
	const struct sigaction *bsa;
	struct svr4_sigaction *ssa;
{
	ssa->sa_handler = bsa->sa_handler;
	bsd_to_svr4_sigset_t(&bsa->sa_mask, &ssa->sa_mask);
	ssa->sa_flags = bsd_to_svr4_sigaction_flags(bsa->sa_flags);
}


static void
svr4_to_bsd_sigaction(ssa, bsa)
	const struct svr4_sigaction *ssa;
	struct sigaction *bsa;
{
	bsa->sa_handler = ssa->sa_handler;
	svr4_to_bsd_sigset_t(&ssa->sa_mask, &bsa->sa_mask);
	bsa->sa_flags = svr4_to_bsd_sigaction_flags(ssa->sa_flags);
}


void
bsd_to_svr4_sigaltstack(bs, ss)
	const struct sigaltstack *bs;
	svr4_stack_t *ss;
{
	ss->ss_sp = bs->ss_base;
	ss->ss_size = bs->ss_size;
	ss->ss_flags = 0;

	if (bs->ss_flags & SA_DISABLE)
		ss->ss_flags |= SVR4_SS_DISABLE;

	if (bs->ss_flags & SA_ONSTACK)
		ss->ss_flags |= SVR4_SS_ONSTACK;
}


void
svr4_to_bsd_sigaltstack(ss, bs)
	const svr4_stack_t *ss;
	struct sigaltstack *bs;
{
	bs->ss_base = ss->ss_sp;
	bs->ss_size = ss->ss_size;
	bs->ss_flags = 0;

	if (ss->ss_flags & SVR4_SS_DISABLE)
		bs->ss_flags |= SA_DISABLE;

	if (ss->ss_flags & SVR4_SS_ONSTACK)
		bs->ss_flags |= SA_ONSTACK;
}


/*
 * Stolen from the ibcs2 one
 */
int
svr4_signal(p, uap, retval)
	register struct proc			*p;
	register struct svr4_signal_args	*uap;
	register_t				*retval;
{
	int	nsig = svr4_to_bsd_signum(SVR4_SIGNO(SCARG(uap, signum)));
	int	error;
	caddr_t sg = stackgap_init();

	if (nsig >= SVR4_NSIG || nsig < 0) {
		if (SVR4_SIGCALL(SCARG(uap, signum)) == SVR4_SIGNAL_MASK ||
		    SVR4_SIGCALL(SCARG(uap, signum)) == SVR4_SIGDEFER_MASK)
			*retval = (int) SVR4_SIG_ERR;
		return EINVAL;
	}

	switch (SVR4_SIGCALL(SCARG(uap, signum))) {
	case SVR4_SIGDEFER_MASK:
		/*
		 * sigset is identical to signal() except
		 * that SIG_HOLD is allowed as
		 * an action and we don't set the bit
		 * in the ibcs_sigflags field.
		 */
		if (SCARG(uap, handler) == SVR4_SIG_HOLD) {
			struct sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		/* FALLTHROUGH */

	case SVR4_SIGNAL_MASK:
		{
			struct sigaction_args sa_args;
			struct sigaction *sap = stackgap_alloc(&sg,
						sizeof(struct sigaction));
			struct sigaction *osap = stackgap_alloc(&sg,
						sizeof(struct sigaction));
			struct sigaction sa;

			SCARG(&sa_args, signum) = nsig;
			SCARG(&sa_args, nsa) = sap;
			SCARG(&sa_args, osa) = osap;

			sa.sa_handler = SCARG(uap, handler);
			sa.sa_mask = (sigset_t) 0;
			sa.sa_flags = 0;
#if 0
			if (nsig != SIGALRM)
				sa.sa_flags = SA_RESTART;
#endif
			error = copyout(&sa, sap, sizeof(sa));
			if (error)
				return error;
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("signal: sigaction failed: %d\n",
					 error));
				*retval = (int) SVR4_SIG_ERR;
				return error;
			}
			error = copyin(osap, &sa, sizeof(sa));
			if (error)
				return error;

			*retval = (int) sa.sa_handler;
			if (SVR4_SIGCALL(SCARG(uap, signum)) ==
			    SVR4_SIGNAL_MASK)
				p->p_md.ibcs_sigflags |= sigmask(nsig);
			return 0;
		}

	case SVR4_SIGHOLD_MASK:
		{
			struct sigprocmask_args sa;
			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}

	case SVR4_SIGRELSE_MASK:
		{
			struct sigprocmask_args sa;
			SCARG(&sa, how) = SIG_UNBLOCK;
			SCARG(&sa, mask) = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}

	case SVR4_SIGIGNORE_MASK:
		{
			struct sigaction *sap = stackgap_alloc(&sg, 
						sizeof(struct sigaction));
			struct sigaction sa;
			struct sigaction_args sa_args;

			SCARG(&sa_args, signum) = nsig;
			SCARG(&sa_args, nsa) = sap;
			SCARG(&sa_args, osa) = NULL;
			sa.sa_handler = SIG_IGN;
			sa.sa_mask = (sigset_t) 0;
			sa.sa_flags = 0;
			error = copyout(&sa, sap, sizeof(sa));
			if (error)
				return error;
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("sigignore: sigaction failed\n"));
				return error;
			}
			return 0;
		}

	case SVR4_SIGPAUSE_MASK:
		{
			struct sigsuspend_args sa;
			SCARG(&sa, mask) = p->p_sigmask & ~sigmask(nsig);
			return sigsuspend(p, &sa, retval);
		}

	default:
		return ENOSYS;
	}
}


int
svr4_sigsuspend(p, uap, retval)
	register struct proc			*p;
	register struct svr4_sigsuspend_args	*uap;
	register_t				*retval;
{
	svr4_sigset_t ss;
	sigset_t bs;
	struct sigsuspend_args sa;
	int error;

	if ((error = copyin(SCARG(uap, ss), &ss, sizeof(ss))) != 0)
		return error;

	svr4_to_bsd_sigset_t(&ss, &bs);

	SCARG(&sa, mask) = bs;
	return sigsuspend(p, &sa, retval);
}


int
svr4_sigprocmask(p, uap, retval)
	register struct proc			*p;
	register struct svr4_sigprocmask_args	*uap;
	register_t				*retval;
{
	svr4_sigset_t ss;
	sigset_t bs;
	int error = 0;

	*retval = 0;

	if (SCARG(uap, oset) != NULL) {
		/* Fix the return value first if needed */
		bsd_to_svr4_sigset_t(&p->p_sigmask, &ss);
		if ((error = copyout(&ss, SCARG(uap, oset), sizeof(ss))) != 0)
			return error;
	}

	if (SCARG(uap, set) == NULL)
		/* Just examine */
		return 0;

	if ((error = copyin(SCARG(uap, set), &ss, sizeof(ss))) != 0)
		return error;

	svr4_to_bsd_sigset_t(&ss, &bs);

	(void) splhigh();

	switch (SCARG(uap, how)) {
	case SVR4_SIG_BLOCK:
		p->p_sigmask |= bs & ~sigcantmask;
		break;

	case SVR4_SIG_UNBLOCK:
		p->p_sigmask &= ~bs;
		break;

	case SVR4_SIG_SETMASK:
		p->p_sigmask = bs & ~sigcantmask;
		break;

	default:
		error = EINVAL;
		break;
	}

	(void) spl0();

	return error;
}


int
svr4_sigpending(p, uap, retval)
	register struct proc			*p;
	register struct svr4_sigpending_args	*uap;
	register_t				*retval;
{
	svr4_sigset_t ss;

	*retval = 0;


	switch (SCARG(uap, what)) {
	case 1:	/* sigpending */
		if (SCARG(uap, mask) == NULL)
			return 0;
		else {
			sigset_t bs = p->p_siglist & p->p_sigmask;

			bsd_to_svr4_sigset_t(&bs, &ss);
		}
		break;

	case 2:	/* sigfillset */
		{
			int i;

			svr4_sigemptyset(&ss);

			for (i = 1; i < SVR4_NSIG; i++)
				svr4_sigaddset(&ss, i);
		}
		break;

	default:
		return EINVAL;
	}
		
	return copyout(&ss, SCARG(uap, mask), sizeof(ss));
}


int
svr4_sigaction(p, uap, retval)
	register struct proc			*p;
	register struct svr4_sigaction_args	*uap;
	register_t				*retval;
{

	struct sigaction *bosa = NULL;
	struct sigaction *bnsa = NULL;
	struct sigaction bsa;
	struct sigaction_args sa_args;
	struct svr4_sigaction sa;
	caddr_t sg = stackgap_init();
	int error;
	int nsig = svr4_to_bsd_signum(SCARG(uap, signum));

	if (SCARG(uap, osa) != NULL)
		bosa = stackgap_alloc(&sg, sizeof(struct sigaction));

	if (SCARG(uap, nsa) != NULL) {
		bnsa = stackgap_alloc(&sg, sizeof(struct sigaction));
		if ((error = copyin(SCARG(uap, nsa), &sa, sizeof(sa))) != 0)
			return error;
		svr4_to_bsd_sigaction(&sa, &bsa);
		if ((error = copyout(&bsa, bnsa, sizeof(bsa))) != 0)
			return error;
	}

	SCARG(&sa_args, signum) = nsig;
	SCARG(&sa_args, nsa) = bnsa;
	SCARG(&sa_args, osa) = bosa;

	if ((error = sigaction(p, &sa_args, retval)) != 0)
		return error;

	if (SCARG(uap, osa) != NULL) {
		if ((error = copyin(bosa, &bsa, sizeof(bsa))) != 0)
			return error;
		bsd_to_svr4_sigaction(&bsa, &sa);
		if ((error = copyout(&sa, SCARG(uap, osa), sizeof(sa))) != 0)
			return error;
	}

	return 0;
}


int
svr4_kill(p, uap, retval)
	register struct proc		*p;
	register struct svr4_kill_args	*uap;
	register_t			*retval;
{
	struct kill_args ki_args;
	SCARG(&ki_args, pid) = SCARG(uap, pid);
	SCARG(&ki_args, signum) = svr4_to_bsd_signum(SCARG(uap, signum));
	return kill(p, &ki_args, retval);
}


int 
svr4_context(p, uap, retval)
	register struct proc			*p;
	register struct svr4_context_args	*uap;
	register_t				*retval;
{
	struct svr4_ucontext uc;
	int error;
	*retval = 0;

	switch (SCARG(uap, func)) {
	case 0:
		DPRINTF(("getcontext(%x)\n", SCARG(uap, uc)));
		svr4_getcontext(p, &uc, p->p_sigmask,
				p->p_sigacts->ps_sigstk.ss_flags & SA_ONSTACK);
		return copyout(&uc, SCARG(uap, uc), sizeof(uc));

	case 1: 
		DPRINTF(("setcontext(%x)\n", SCARG(uap, uc)));
		if ((error = copyin(SCARG(uap, uc), &uc, sizeof(uc))) != 0)
			return error;
		return svr4_setcontext(p, &uc);

	default:
		DPRINTF(("context(%d, %x)\n", SCARG(uap, func),
			SCARG(uap, uc)));
		return ENOSYS;
	}
	return 0;
}


int 
svr4_sigaltstack(p, uap, retval)
	register struct proc			*p;
	register struct svr4_sigaltstack_args	*uap;
	register_t				*retval;
{
	svr4_stack_t ss;
	struct sigaltstack bs;
	struct sigaltstack_args sa;
	int error;
	caddr_t sg = stackgap_init();

	if (SCARG(uap, nss)) {
		if ((error = copyin(SCARG(uap, nss), &ss, sizeof(ss))) != 0)
			return error;
		svr4_to_bsd_sigaltstack(&ss, &bs);
		SCARG(&sa, nss) = stackgap_alloc(&sg,
						 sizeof(struct sigaltstack));
		if ((error = copyout(&bs, SCARG(&sa, nss), sizeof(bs))) != 0)
			return error;
	}
	else
		SCARG(&sa, nss) = NULL;

	if (SCARG(uap, oss))
		SCARG(&sa, oss) = stackgap_alloc(&sg,
						 sizeof(struct sigaltstack));
	else
		SCARG(&sa, oss) = NULL;


	if ((error = sigaltstack(p, &sa, retval)) != 0)
		return error;

	if (SCARG(&sa, oss)) {
		if ((error = copyin(SCARG(&sa, oss), &bs, sizeof(bs))) != 0)
			return error;
		bsd_to_svr4_sigaltstack(&bs, &ss);
		if ((error = copyout(&ss, SCARG(uap, oss), sizeof(ss))) != 0)
			return error;
	}
	return 0;
}
