/* $NetBSD: svr4_signal.c,v 1.1 1994/10/24 17:37:44 deraadt Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <sys/syscallargs.h>
#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_signal.h>

static int
svr4_signumcvt(sn)
    int sn;
{
    switch (sn) {
    case SVR4_SIGHUP:	return SIGHUP;
    case SVR4_SIGINT:	return SIGINT;
    case SVR4_SIGQUIT:	return SIGQUIT;
    case SVR4_SIGILL:	return SIGILL;
    case SVR4_SIGTRAP:	return SIGTRAP;
    case SVR4_SIGABRT:	return SIGABRT;
    case SVR4_SIGEMT:	return SIGEMT;
    case SVR4_SIGFPE:	return SIGFPE;
    case SVR4_SIGKILL:	return SIGKILL;
    case SVR4_SIGBUS:	return SIGBUS;
    case SVR4_SIGSEGV:	return SIGSEGV;
    case SVR4_SIGSYS:	return SIGSYS;
    case SVR4_SIGPIPE:	return SIGPIPE;
    case SVR4_SIGALRM:	return SIGALRM;
    case SVR4_SIGTERM:	return SIGTERM;
    case SVR4_SIGUSR1:	return SIGUSR1;
    case SVR4_SIGUSR2:	return SIGUSR2;
    case SVR4_SIGCHLD:	return SIGCHLD;
    case SVR4_SIGPWR:	return NSIG;
    case SVR4_SIGWINCH:	return SIGWINCH;
    case SVR4_SIGURG:	return SIGURG;
    case SVR4_SIGIO:	return SIGIO;
    case SVR4_SIGSTOP:	return SIGSTOP;
    case SVR4_SIGTSTP:	return SIGTSTP;
    case SVR4_SIGCONT:	return SIGCONT;
    case SVR4_SIGTTIN:	return SIGTTIN;
    case SVR4_SIGTTOU:	return SIGTTOU;
    case SVR4_SIGVTALRM:return SIGVTALRM;
    case SVR4_SIGPROF:	return SIGPROF;
    case SVR4_SIGXCPU:	return SIGXCPU;
    case SVR4_SIGXFSZ:	return SIGXFSZ;
    default:
	return NSIG;
    }
}


/*
 * Stolen from the ibcs2 one
 */
int
svr4_signal(p, uap, retval)
    register struct proc *p;
    register struct svr4_signal_args *uap;
    register_t *retval;
{
    int nsig = svr4_signumcvt(SVR4_SIGNO(SCARG(uap, signum)));
    int error;

    if (nsig == NSIG) {
	if (SVR4_SIGCALL(SCARG(uap, signum)) == SVR4_SIGNAL_MASK
	    || SVR4_SIGCALL(SCARG(uap, signum)) == SVR4_SIGDEFER_MASK)
		*retval = (int)SVR4_SIG_ERR;
	return EINVAL;
    }

    stackgap_init();
    
    switch (SVR4_SIGCALL(SCARG(uap, signum))) {
    /*
     * sigset is identical to signal() except that SIG_HOLD is allowed as
     * an action and we don't set the bit in the ibcs_sigflags field.
     */
    case SVR4_SIGDEFER_MASK:
	if (SCARG(uap, handler) == SVR4_SIG_HOLD) {
	    struct sigprocmask_args sa;

	    SCARG(&sa, how) = SIG_BLOCK;
	    SCARG(&sa, mask) = sigmask(nsig);
	    return sigprocmask(p, &sa, retval);
	}
	/*FALLTHROUGH*/

    case SVR4_SIGNAL_MASK:
	{
	    struct sigaction_args sa_args;
	    struct sigaction *sap = stackgap_alloc(sizeof(struct sigaction));
	    struct sigaction *osap = stackgap_alloc(sizeof(struct sigaction));
	    struct sigaction sa;
	    SCARG(&sa_args, signum) = nsig;
	    SCARG(&sa_args, nsa) = sap;
	    SCARG(&sa_args, osa) = osap;
	    sa.sa_handler = SCARG(uap, handler);
	    sa.sa_mask = (sigset_t)0;
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
		DPRINTF(("signal: sigaction failed: %d\n", error));
		*retval = (int)SVR4_SIG_ERR;
		return error;
	    }

	    error = copyin(osap, &sa, sizeof(sa));
	    if (error)
		return error;

	    *retval = (int)sa.sa_handler;
	    if (SVR4_SIGCALL(SCARG(uap, signum)) == SVR4_SIGNAL_MASK)
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
	    struct sigaction *sap = stackgap_alloc(sizeof(struct sigaction));
	    struct sigaction sa;
	    struct sigaction_args sa_args;
	    SCARG(&sa_args, signum) = nsig;
	    SCARG(&sa_args, nsa) = sap;
	    SCARG(&sa_args, osa) = NULL;
	    sa.sa_handler = SIG_IGN;
	    sa.sa_mask = (sigset_t)0;
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
	    SCARG(&sa, mask) = p->p_sigmask &~ sigmask(nsig);
	    return sigsuspend(p, &sa, retval);
	}
	    
    default:
	return ENOSYS;
    }
}
