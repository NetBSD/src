/*	$NetBSD: svr4_machdep.c,v 1.1 1995/01/08 21:22:19 christos Exp $	 */

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
#include <sys/user.h>
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

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>

extern int _ucodesel, _udatasel;
extern int check_selectors __P((u_short, u_short, u_short, u_short));

void
svr4_getcontext(p, uc, mask, oonstack)
	struct proc *p;
	struct svr4_ucontext *uc;
	int mask, oonstack;
{
	struct trapframe *tf = (struct trapframe *)p->p_md.md_regs;
	struct sigacts *psp = p->p_sigacts;
	svr4_greg_t* r = uc->uc_mcontext.greg;
	svr4_stack_t *s = &uc->uc_stack;
	struct sigaltstack *sf = &psp->ps_sigstk;

	bzero(uc, sizeof(struct svr4_ucontext));

	/*
	 * Set the general purpose registers
	 */
	r[SVR4_X86_GS] = 0;
	r[SVR4_X86_FS] = 0;
	r[SVR4_X86_ES] = tf->tf_es;
	r[SVR4_X86_DS] = tf->tf_ds;
	r[SVR4_X86_EDI] = tf->tf_edi;
	r[SVR4_X86_ESI] = tf->tf_esi;
	r[SVR4_X86_EBP] = tf->tf_ebp;
	r[SVR4_X86_ESP] = tf->tf_esp;
	r[SVR4_X86_EBX] = tf->tf_ebx;
	r[SVR4_X86_EDX] = tf->tf_edx;
	r[SVR4_X86_ECX] = tf->tf_ecx;
	r[SVR4_X86_EAX] = tf->tf_eax;
	r[SVR4_X86_TRAPNO] = 0;
	r[SVR4_X86_ERR] = 0;
	r[SVR4_X86_EIP] = tf->tf_eip;
	r[SVR4_X86_CS] = tf->tf_cs;
	r[SVR4_X86_EFL] = tf->tf_eflags;
	r[SVR4_X86_UESP] = 0;
	r[SVR4_X86_SS] = tf->tf_ss;

	/*
	 * Set the signal stack
	 */
	s->ss_sp  = sf->ss_base;
	s->ss_size = sf->ss_size;
	s->ss_flags = 0;

	if (oonstack)
	    s->ss_flags |= SVR4_SS_ONSTACK;

	if (sf->ss_flags & SA_DISABLE)
	    s->ss_flags |= SVR4_SS_DISABLE;

	/*
	 * Set the signal mask
	 */
	bsd_to_svr4_sigset_t(&mask, &uc->uc_sigmask);

	/*
	 * Set the flags
	 */
	uc->uc_flags = SVR4_UC_ALL;
}


/*
 * Set to ucontext specified.
 * has been taken.  Reset signal mask and
 * stack state from context.
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
svr4_setcontext(p, uc)
	struct proc *p;
	struct svr4_ucontext *uc;
{
	struct sigcontext *scp, context;
	register struct trapframe *tf;
	svr4_greg_t* r = uc->uc_mcontext.greg;
	svr4_stack_t *s = &uc->uc_stack;
	int eflags, mask;

	/*
	 * XXX:
	 * Should we check the value of flags to determine what to restore?
	 * What to do with uc_link?
	 * Should we restore the sigaltstack struct completely?
	 * What to do with floating point stuff?
	 * Should we bother with the rest of the registers that we
	 * set to 0 right now?
	 */

	tf = (struct trapframe *)p->p_md.md_regs;

	eflags = r[SVR4_X86_EFL];
	if ((eflags & PSL_USERCLR) != 0 ||
	    (eflags & PSL_USERSET) != PSL_USERSET ||
	    (eflags & PSL_IOPL) > (tf->tf_eflags & PSL_IOPL))
		return EINVAL;

	/*
	 * Sanity check the user's selectors and error if they are suspect.
	 * We assume that swtch() has loaded the correct LDT descriptor, so
	 * we can just use the `verr' instruction.  We further assume that
	 * none of the segments we wish to protect are conforming.  (If they
	 * were, this check wouldn't help much anyway.)
	 */
	if (check_selectors(r[SVR4_X86_CS], r[SVR4_X86_SS], r[SVR4_X86_DS],
			    r[SVR4_X86_ES])) {
		trapsignal(p, SIGBUS, T_PROTFLT);
		return EINVAL;
	}

	/*
	 * restore signal stack
	 */
	if (s->ss_flags & SVR4_SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;

	/*
	 * restore signal mask
	 */
	svr4_to_bsd_sigset_t(&uc->uc_sigmask, &mask);
	p->p_sigmask = mask & ~sigcantmask;

	/*
	 * Restore register context.
	 */
	tf->tf_es = r[SVR4_X86_ES];
	tf->tf_ds = r[SVR4_X86_DS];
	tf->tf_edi = r[SVR4_X86_EDI];
	tf->tf_esi = r[SVR4_X86_ESI];
	tf->tf_ebp = r[SVR4_X86_EBP];
	tf->tf_ebx = r[SVR4_X86_EBX];
	tf->tf_edx = r[SVR4_X86_EDX];
	tf->tf_ecx = r[SVR4_X86_ECX];
	tf->tf_eax = r[SVR4_X86_EAX];
	tf->tf_eip = r[SVR4_X86_EIP];
	tf->tf_cs = r[SVR4_X86_CS];
	tf->tf_eflags = eflags;
	tf->tf_ss = r[SVR4_X86_SS];
	tf->tf_esp = r[SVR4_X86_ESP];

	return EJUSTRETURN;
}


/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine. After the handler is
 * done svr4 will call setcontext for us
 * with the user context we just set up, and we
 * will return to the user pc, psl.
 */
void
svr4_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct trapframe *tf;
	struct svr4_sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];


	tf = (struct trapframe *)p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct svr4_sigframe *)(psp->ps_sigstk.ss_base +
		    psp->ps_sigstk.ss_size - sizeof(struct svr4_sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else {
		fp = (struct svr4_sigframe *)tf->tf_esp - 1;
	}

	/* 
	 * Build the argument list for the signal handler.
	 */
	frame.sf_signum = bsd_to_svr4_signum(sig);
	frame.sf_code = code;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_handler = catcher;
	svr4_getcontext(p, &frame.sf_uc, mask, oonstack);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_esp = (int)fp;
	tf->tf_eip = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
	tf->tf_eflags &= ~PSL_VM;
	tf->tf_cs = _ucodesel;
	tf->tf_ds = _udatasel;
	tf->tf_es = _udatasel;
	tf->tf_ss = _udatasel;
}
