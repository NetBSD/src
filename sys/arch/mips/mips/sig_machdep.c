/*	$NetBSD: sig_machdep.c,v 1.13 2007/07/08 10:19:22 pooka Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
	
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.13 2007/07/08 10:19:22 pooka Exp $"); 

#include "opt_cputype.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>

#include <mips/frame.h>
#include <mips/regnum.h>

void *	
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc *p = l->l_proc;
	struct frame *fp = l->l_md.md_regs;
 
	/* Do we need to jump onto the signal stack? */
	*onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (*onstack)
		return (char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size;
	else
		return (void *)fp->f_regs[_R_SP];
}		

struct sigframe_siginfo {
	siginfo_t sf_si;
	ucontext_t sf_uc;
};

/*
 * Send a signal to process.
 */
static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo *fp = getframe(l, sig, &onstack);
	struct frame *tf;
	ucontext_t uc;
	size_t ucsz;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = (struct frame *)l->l_md.md_regs;
	fp--;

        /* Build stack frame for signal trampoline. */
        switch (ps->sa_sigdesc[sig].sd_vers) {
        case 0:         /* handled by sendsig_sigcontext */
        case 1:         /* handled by sendsig_sigcontext */
        default:        /* unknown version */
                printf("sendsig_siginfo: bad version %d\n",
                    ps->sa_sigdesc[sig].sd_vers);
                sigexit(l, SIGILL);
        case 2:
                break;
        }

        uc.uc_flags = _UC_SIGMASK
            | ((l->l_sigstk.ss_flags & SS_ONSTACK)
            ? _UC_SETSTACK : _UC_CLRSTACK);
        uc.uc_sigmask = *mask;
        uc.uc_link = l->l_ctxlink;
        memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));
        ucsz = (char *)&uc.__uc_pad - (char *)&uc;
        sendsig_reset(l, sig);
        mutex_exit(&p->p_smutex);
        cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);
	error = copyout(&ksi->ksi_info, &fp->sf_si, sizeof(ksi->ksi_info));
	if (error == 0)
		error = copyout(&uc, &fp->sf_uc, ucsz);
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Set up the registers to directly invoke the signal
	 * handler.  The return address will be set up to point
	 * to the signal trampoline to bounce us back.
	 */
	tf->f_regs[_R_A0] = sig;
	tf->f_regs[_R_A1] = (__greg_t)&fp->sf_si;
	tf->f_regs[_R_A2] = (__greg_t)&fp->sf_uc;

	tf->f_regs[_R_PC] = (__greg_t)catcher;
	tf->f_regs[_R_T9] = (__greg_t)catcher;
	tf->f_regs[_R_SP] = (__greg_t)fp;
	tf->f_regs[_R_RA] = (__greg_t)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void    
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{               
#ifdef COMPAT_16    
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else    
#endif  
		sendsig_siginfo(ksi, mask);
}       
