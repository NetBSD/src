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
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/syscallargs.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_fcntl.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_util.h>

#define NOSIG		(-1)

static int ibcs2bsd_sigtbl[IBCS2_NSIG] = {
	NOSIG,			/* 0 */
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
	NOSIG,			/* 19 - SIGPWR */
	SIGWINCH,		/* 20 */
	NOSIG,			/* 21 */
	NOSIG,			/* 22 - SIGPOLL */
	SIGSTOP,		/* 23 */
	SIGTSTP,		/* 24 */
	SIGCONT,		/* 25 */
	SIGTTIN,		/* 26 */
	SIGTTOU,		/* 27 */
	SIGVTALRM,		/* 28 */
	SIGPROF,		/* 29 */
	NOSIG,			/* 30 */
	NOSIG,			/* 31 */
};

static int bsd2ibcs_sigtbl[NSIG] = {
	NOSIG,			/* 0 */
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
	NOSIG,			/* 16 */
	IBCS2_SIGSTOP,		/* 17 */
	IBCS2_SIGTSTP,		/* 18 */
	IBCS2_SIGCONT,		/* 19 */
	IBCS2_SIGCLD,		/* 20 */
	IBCS2_SIGTTIN,		/* 21 */
	IBCS2_SIGTTOU,		/* 22 */
	NOSIG,			/* 23 */
	NOSIG,			/* 24 */
	NOSIG,			/* 25 */
	IBCS2_SIGVTALRM,	/* 26 */
	IBCS2_SIGPROF,		/* 27 */
	IBCS2_SIGWINCH,		/* 28 */
	NOSIG,			/* 29 */
	IBCS2_SIGUSR1,		/* 30 */
	IBCS2_SIGUSR2,		/* 31 */
};

static int
ibcs2bsd_sig(sig)
	int sig;
{
	if (sig < 1 || sig >= IBCS2_NSIG)
		return NOSIG;
	else
		return ibcs2bsd_sigtbl[sig];
}

int
bsd2ibcs_sig(sig)
	int sig;
{
	if (sig < 1 || sig >= NSIG)
		return NOSIG;
	else
		return bsd2ibcs_sigtbl[sig];
}

static sigset_t
cvt_sigmask(mask, sigtbl)
	sigset_t mask;
	int sigtbl[];
{
	int i, newmask;

	for (i = 0, newmask = 0; i < NSIG; i++)
		if ((sigtbl[i] != NOSIG) && (mask & (1 << i)))
			newmask |= (1 << (sigtbl[i] - 1));
	return newmask;
}

static void
cvt_sa2isa(sap, isap)
	struct sigaction *sap;
	struct ibcs2_sigaction *isap;
{
	isap->sa_handler = sap->sa_handler;
	isap->sa_mask = cvt_sigmask(sap->sa_mask, bsd2ibcs_sigtbl);
	isap->sa_flags = (sap->sa_flags & SA_NOCLDSTOP)
		? IBCS2_SA_NOCLDSTOP : 0;
}

static void
cvt_isa2sa(isap, sap)
	struct ibcs2_sigaction *isap;
	struct sigaction *sap;
{
	sap->sa_handler = isap->sa_handler;
	sap->sa_mask = cvt_sigmask(isap->sa_mask, ibcs2bsd_sigtbl);
	sap->sa_flags = (isap->sa_flags & IBCS2_SA_NOCLDSTOP)
		? SA_NOCLDSTOP : 0;
}

int
ibcs2_sigsys(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigsys_args *uap;
	int *retval;
{
	int error;
	int nsig = ibcs2bsd_sig(IBCS2_SIGNO(SCARG(uap, sig)));

	if (nsig  == NOSIG) {
		if (IBCS2_SIGCALL(SCARG(uap, sig)) == IBCS2_SIGNAL_MASK
		    || IBCS2_SIGCALL(SCARG(uap, sig)) == IBCS2_SIGSET_MASK)
			*retval = (int)IBCS2_SIG_ERR;
		return EINVAL;
	}
	
	switch (IBCS2_SIGCALL(SCARG(uap, sig))) {
	/*
	 * sigset is identical to signal() except that SIG_HOLD is allowed as
	 * an action and we don't set the bit in the ibcs_sigflags field.
	 */
	case IBCS2_SIGSET_MASK:
		if (SCARG(uap, fp) == IBCS2_SIG_HOLD) {
			struct sigprocmask_args sa;

			SCARG(&sa, how) = SIG_BLOCK;
			SCARG(&sa, mask) = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		/* else fall through */

	case IBCS2_SIGNAL_MASK:
		{
			struct sigaction *sap, *osap;
			struct sigaction_args sa_args;
			caddr_t sg = stackgap_init();
			sap = stackgap_alloc(&sg, sizeof(*sap));
			osap = stackgap_alloc(&sg, sizeof(*sap));
			sap->sa_handler = SCARG(uap, fp);
			sap->sa_mask = (sigset_t)0;
			sap->sa_flags = 0;
#if 0
			if (SCARG(&sa_args, sig) != SIGALRM)
				sap->sa_flags = SA_RESTART;
#endif
			SCARG(&sa_args, signum) = nsig;
			SCARG(&sa_args, nsa) = sap;
			SCARG(&sa_args, osa) = osap;
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("signal: sigaction failed: %d\n",
					 error));
				*retval = (int)IBCS2_SIG_ERR;
				return error;
			}
			*retval = (int)SCARG(&sa_args, osa)->sa_handler;
			if (IBCS2_SIGCALL(SCARG(uap, sig)) == IBCS2_SIGNAL_MASK)
				p->p_md.ibcs_sigflags |= sigmask(nsig);
			return 0;
		}
		
	case IBCS2_SIGHOLD_MASK:
		{
			struct sigprocmask_args sa;

			sa.how = SIG_BLOCK;
			sa.mask = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		
	case IBCS2_SIGRELSE_MASK:
		{
			struct sigprocmask_args sa;

			sa.how = SIG_UNBLOCK;
			sa.mask = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		
	case IBCS2_SIGIGNORE_MASK:
		{
			struct sigaction *sap;
			struct sigaction_args sa_args;
			caddr_t sg = stackgap_init();

			sap = stackgap_alloc(&sg, sizeof(*sap));
			sap->sa_handler = SIG_IGN;
			sap->sa_mask = (sigset_t)0;
			sap->sa_flags = 0;
			SCARG(&sa_args, signum) = nsig;
			SCARG(&sa_args, nsa) = sap;
			SCARG(&sa_args, osa) = NULL;
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("sigignore: sigaction failed\n"));
				return error;
			}
			return 0;
		}
		
	case IBCS2_SIGPAUSE_MASK:
		{
			struct sigsuspend_args sa;
			SCARG(&sa, mask) = p->p_sigmask &~ sigmask(nsig);
			return sigsuspend(p, &sa, retval);
		}
		
	default:
		return ENOSYS;
	}
}

int
ibcs2_sigaction(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigaction_args *uap;
	int *retval;
{
	int error;
	struct sigaction_args sa;
	struct ibcs2_sigaction *isa, *oisa;
	caddr_t sg = stackgap_init();

	isa = stackgap_alloc(&sg, sizeof(*isa));
	oisa = stackgap_alloc(&sg, sizeof(*oisa));
	SCARG(&sa, signum) = ibcs2bsd_sig(SCARG(uap, sig));
	SCARG(&sa, nsa) = stackgap_alloc(&sg, sizeof(*SCARG(&sa, nsa)));
	SCARG(&sa, osa) = stackgap_alloc(&sg, sizeof(*SCARG(&sa, osa)));
	if (error = copyin((caddr_t)SCARG(uap, act), (caddr_t)isa,
			    sizeof(*isa)))
		return error;
	cvt_isa2sa(isa, SCARG(&sa, nsa));
	if (error = sigaction(p, &sa, retval))
		return error;
	cvt_sa2isa(SCARG(&sa, osa), oisa);
        return copyout((caddr_t)oisa, (caddr_t)SCARG(uap, oact), sizeof(*oisa));
}

int
ibcs2_sigprocmask(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigprocmask_args *uap;
	int *retval;
{
	int error;
	sigset_t iset;
	struct sigprocmask_args sa;

	if (SCARG(uap, set)) {
		switch (SCARG(uap, how)) {
		case IBCS2_SIG_BLOCK:
			SCARG(&sa, how) = SIG_BLOCK;
			break;
		case IBCS2_SIG_UNBLOCK:
			SCARG(&sa, how) = SIG_UNBLOCK;
			break;
		case IBCS2_SIG_SETMASK:
			SCARG(&sa, how) = SIG_SETMASK;
			break;
		default:
			return EINVAL;
		}
		if (error = copyin((caddr_t)SCARG(uap, set), (caddr_t)&iset,
				   sizeof(iset)))
			return error;
		SCARG(&sa, mask) = cvt_sigmask(iset, ibcs2bsd_sigtbl);
	} else {
		SCARG(&sa, how) = SIG_BLOCK;	/* TODO: CHECK THIS */
		SCARG(&sa, mask) = 0;
	}
	if (error = sigprocmask(p, &sa, retval))
		return error;
	if (SCARG(uap, oset) == NULL) {
		*retval = 0;
		return 0;
	}
	iset = cvt_sigmask(*retval, bsd2ibcs_sigtbl);
	*retval = 0;
	return copyout((caddr_t)&iset, (caddr_t)SCARG(uap, oset),
		       sizeof(iset));
}

int
ibcs2_sigpending(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigpending_args *uap;
	int *retval;
{
	int mask = cvt_sigmask(p->p_siglist & p->p_sigmask, bsd2ibcs_sigtbl);

	return (copyout((caddr_t)&mask, (caddr_t)SCARG(uap, mask),
			sizeof(int)));
}

int
ibcs2_sigsuspend(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigsuspend_args *uap;
	int *retval;
{
	int error;
	struct sigsuspend_args sa;

	if (error = copyin((caddr_t)SCARG(uap, mask), (caddr_t)SCARG(&sa, mask),
			   sizeof(SCARG(&sa, mask))))
		return error;
	return sigsuspend(p, &sa, retval);
}

int
ibcs2_pause(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	int error;
	struct sigsuspend_args sa;

	SCARG(&sa, mask) = p->p_sigmask;
        error = sigsuspend(p, &sa, retval);
	return error;
}

int
ibcs2_kill(p, uap, retval)
	struct proc *p;
	struct ibcs2_kill_args *uap;
	int *retval;
{

	SCARG(uap, signo) = ibcs2bsd_sig(SCARG(uap, signo));
	if (SCARG(uap, signo) == NOSIG)
		return EINVAL;
	return kill(p, uap, retval);
}
