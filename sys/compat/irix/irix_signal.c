/*	$NetBSD: irix_signal.c,v 1.13 2002/04/13 10:53:00 manu Exp $ */

/*-
 * Copyright (c) 1994, 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: irix_signal.c,v 1.13 2002/04/13 10:53:00 manu Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/ptrace.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/wait.h>
#include <sys/user.h>

#include <machine/regnum.h>

#include <compat/common/compat_util.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_wait.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>

#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscallargs.h>

extern const int native_to_svr4_signo[];
extern const int svr4_to_native_signo[];

static int irix_setinfo __P((struct proc *, int, irix_irix5_siginfo_t *));
static void irix_set_ucontext __P((struct irix_ucontext*, sigset_t *, 
    int, struct proc *));
static void irix_set_sigcontext __P((struct irix_sigcontext*, sigset_t *, 
    int, struct proc *));
static void irix_get_ucontext __P((struct irix_ucontext*, struct proc *));
static void irix_get_sigcontext __P((struct irix_sigcontext*, struct proc *));

#define irix_sigmask(n)	 (1 << (((n) - 1) & 31))
#define irix_sigword(n)	 (((n) - 1) >> 5) 
#define irix_sigemptyset(s)     memset((s), 0, sizeof(*(s)))
#define irix_sigismember(s, n)  ((s)->bits[irix_sigword(n)] & irix_sigmask(n))
#define irix_sigaddset(s, n)    ((s)->bits[irix_sigword(n)] |= irix_sigmask(n))

/* 
 * This is ripped from svr4_setinfo. See irix_sys_waitsys...
 */
static int
irix_setinfo(p, st, s)
	struct proc *p;
	int st;
	irix_irix5_siginfo_t *s;
{
	irix_irix5_siginfo_t i;
	int sig;

	memset(&i, 0, sizeof(i));

	i.isi_signo = SVR4_SIGCHLD;
	i.isi_errno = 0; /* XXX? */

	if (p) {
		i.isi_pid = p->p_pid;
		if (p->p_stat == SZOMB) {
			i.isi_stime = p->p_ru->ru_stime.tv_sec;
			i.isi_utime = p->p_ru->ru_utime.tv_sec;
		}
		else {
			i.isi_stime = p->p_stats->p_ru.ru_stime.tv_sec;
			i.isi_utime = p->p_stats->p_ru.ru_utime.tv_sec;
		}
	}

	if (WIFEXITED(st)) {
		i.isi_status = WEXITSTATUS(st);
		i.isi_code = SVR4_CLD_EXITED;
	} else if (WIFSTOPPED(st)) {
		sig = WSTOPSIG(st);
		if (sig >= 0 && sig < NSIG)
			i.isi_status = native_to_svr4_signo[sig];

		if (i.isi_status == SVR4_SIGCONT)
			i.isi_code = SVR4_CLD_CONTINUED;
		else
			i.isi_code = SVR4_CLD_STOPPED;
	} else {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			i.isi_status = native_to_svr4_signo[sig];

		if (WCOREDUMP(st))
			i.isi_code = SVR4_CLD_DUMPED;
		else
			i.isi_code = SVR4_CLD_KILLED;
	}

	return copyout(&i, s, sizeof(i));
}


void
native_to_irix_sigset(bss, sss)
	 const sigset_t *bss;
	 irix_sigset_t *sss;
{
	 int i, newsig;

	 irix_sigemptyset(sss);
	 for (i = 1; i < NSIG; i++) {
		 if (sigismember(bss, i)) {
			 newsig = native_to_svr4_signo[i];
			 if (newsig)
			 	irix_sigaddset(sss, newsig);
		 }
	 }
}

void    
irix_to_native_sigset(sss, bss)
	const irix_sigset_t *sss;
	sigset_t *bss;
{       
	int i, newsig;
	
	sigemptyset(bss);
	for (i = 1; i < SVR4_NSIG; i++) {
		if (irix_sigismember(sss, i)) {
			newsig = svr4_to_native_signo[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}

void
irix_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	void *fp;
	void *sp;
	struct frame *f;
	int onstack;
	int error;
	struct irix_sigframe sf;
 
	f = (struct frame *)p->p_md.md_regs;
#ifdef DEBUG_IRIX
	printf("irix_sendsig()\n");
	printf("catcher = %p, sig = %d, code = 0x%lx\n",
	    (void *)catcher, sig, code);
	printf("irix_sendsig(): starting [PC=%p SP=%p SR=0x%08lx]\n",
	    (void *)f->f_regs[PC], (void *)f->f_regs[SP], f->f_regs[SR]);
#endif /* DEBUG_IRIX */

	/*
	 * Do we need to jump onto the signal stack?
	 */
	onstack = 
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 
		&& (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0; 
#ifdef DEBUG_IRIX
	if (onstack)
		printf("irix_sendsig: using signal stack\n");
#endif
	/*
	 * Allocate space for the signal handler context.
	 */
	if (onstack)
		sp = (void *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp
		    + p->p_sigctx.ps_sigstk.ss_size);
	else
		/* cast for _MIPS_BSD_API == _MIPS_BSD_API_LP32_64CLEAN case */
		sp = (void *)(u_int32_t)f->f_regs[SP];

	/*
	 * Build the signal frame
	 */
	bzero(&sf, sizeof(sf));
	if (SIGACTION(p, sig).sa_flags & SA_SIGINFO)
		irix_set_ucontext(&sf.isf_ctx.iuc, mask, code, p);
	else
		irix_set_sigcontext(&sf.isf_ctx.isc, mask, code, p);
	
	sf.isf_signo = native_to_svr4_signo[sig];

	/*
	 * XXX On, IRIX, the sigframe holds a pointer to 
	 * errno in userspace. This is used by the signal
	 * trampoline. No idea how to emulate this for now...
	 */
	/* sf.isf_uep = NULL; */
	/* sf.isf_errno = 0 */

	/*
	 * Compute the new stack address after copying sigframe (hold by sp),
	 * and the address of the sigcontext/ucontext (hold by fp)
	 */
	sp = (void *)((unsigned long)sp - sizeof(sf));
	sp = (void *)((unsigned long)sp & ~0xfUL); /* 16 bytes alignement */
	fp = (void *)((unsigned long)sp + sizeof(sf) - sizeof(sf.isf_ctx));

	
	/* 
	 * If SA_SIGINFO is used, then ucp is used, and scp = NULL, 
	 * if it is not used, then scp is used, and ucp = scp
	 */
	if (SIGACTION(p, sig).sa_flags & SA_SIGINFO) {
		sf.isf_ucp = (struct irix_ucontext *)fp;
		sf.isf_scp = NULL;
	} else {
		sf.isf_ucp = (struct irix_ucontext *)fp;
		sf.isf_scp = (struct irix_sigcontext *)fp;
	}

	/*
	 * Install the sigframe onto the stack
	 */
	error = copyout(&sf, sp, sizeof(sf));
	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG_IRIX
		printf("irix_sendsig: stack trashed\n");
#endif /* DEBUG_IRIX */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/* 
	 * Set up signal handler arguments. 
	 */
	f->f_regs[A0] = native_to_svr4_signo[sig];	/* signo/signo */
	f->f_regs[A1] = 0;			/* code/siginfo */
	f->f_regs[A2] = (unsigned long)fp;	/* sigcontext/ucontext */

#ifdef DEBUG_IRIX
	printf("sigcontext is at %p, stack pointer at %p\n", fp, sp);
#endif /* DEBUG_IRIX */

	/* 
	 * Set up the registers to return to sigcode. 
	 */
	f->f_regs[RA] = (unsigned long)p->p_sigctx.ps_sigcode;
	f->f_regs[SP] = (unsigned long)sp;
	f->f_regs[T9] = (unsigned long)catcher;
	f->f_regs[A3] = (unsigned long)catcher;
	f->f_regs[PC] = (unsigned long)catcher;

	/* 
	 * Remember that we're now on the signal stack. 
	 */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG_IRIX
	printf("returning from irix_sendsig()\n");
#endif
	return;
}

static void 
irix_set_sigcontext (scp, mask, code, p)
	struct irix_sigcontext *scp;
	sigset_t *mask;
	int code;
	struct proc *p;
{
	int i;
	struct frame *f;

#ifdef DEBUG_IRIX
	printf("irix_set_sigcontext()\n");
#endif
	f = (struct frame *)p->p_md.md_regs;
	/*
	 * Build stack frame for signal trampoline.
	 */
	native_to_irix_sigset(mask, &scp->isc_sigset);
	for (i = 1; i < 32; i++) { /* save gpr1 - gpr31 */
		scp->isc_regs[i] = f->f_regs[i];
	}
	scp->isc_regs[0] = 0;
	scp->isc_fp_rounded_result = 0;
	scp->isc_regmask = ~0x1UL;
	scp->isc_mdhi = f->f_regs[MULHI];
	scp->isc_mdlo = f->f_regs[MULLO];
	scp->isc_pc = f->f_regs[PC];

	/* 
	 * Save the floating-pointstate, if necessary, then copy it. 
	 */
#ifndef SOFTFLOAT
	scp->isc_ownedfp = p->p_md.md_flags & MDP_FPUSED;
	if (scp->isc_ownedfp) {
		/* if FPU has current state, save it first */
		if (p == fpcurproc)
			savefpregs(p);
		(void)memcpy(&scp->isc_fpregs, &p->p_addr->u_pcb.pcb_fpregs,
		    sizeof(scp->isc_fpregs));
		scp->isc_fpc_csr = p->p_addr->u_pcb.pcb_fpregs.r_regs[32];
	}
#else
	(void)memcpy(&scp->isc_fpregs, &p->p_addr->u_pcb.pcb_fpregs,
	    sizeof(scp->isc_fpregs));
#endif 
	/*
	 * Save signal stack
	 */
	scp->isc_ssflags = 
	    (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK) ? IRIX_SS_ONSTACK : 0;

	return;
}

void 
irix_set_ucontext(ucp, mask, code, p)
	struct irix_ucontext *ucp;
	sigset_t *mask;
	int code;
	struct proc *p;
{
	struct frame *f;

#ifdef DEBUG_IRIX
	printf("irix_set_ucontext()\n");
#endif
	f = (struct frame *)p->p_md.md_regs;
	/*
	 * Build stack frame for signal trampoline.
	 */
	native_to_irix_sigset(mask, &ucp->iuc_sigmask);
	memcpy(&ucp->iuc_mcontext.svr4___gregs, 
	    &f->f_regs, 32 * sizeof(mips_reg_t));
	/* Theses registers have different order on NetBSD and IRIX */
	ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_MDLO] = f->f_regs[MULLO];
	ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_MDHI] = f->f_regs[MULHI];
	ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_EPC] = f->f_regs[PC];

	ucp->iuc_flags = IRIX_UC_SIGMASK | IRIX_UC_MCONTEXT;

	/* 
	 * Save the floating-pointstate, if necessary, then copy it. 
	 */
#ifndef SOFTFLOAT
	if (p->p_md.md_flags & MDP_FPUSED) {
		/* if FPU has current state, save it first */
		if (p == fpcurproc)
			savefpregs(p);
		(void)memcpy(&ucp->iuc_mcontext.svr4___fpregs, 
		    &p->p_addr->u_pcb.pcb_fpregs, 
		    sizeof(ucp->iuc_mcontext.svr4___fpregs));
		ucp->iuc_mcontext.svr4___fpregs.svr4___fp_csr = 
		    p->p_addr->u_pcb.pcb_fpregs.r_regs[32];
	}
#else
	(void)memcpy(&ucp->iuc_mcontext.svr4___fpregs, 
	    &p->p_addr->u_pcb.pcb_fpregs, 
	    sizeof(ucp->iuc_mcontext.svr4___fpregs));
#endif 
	/* XXX Save signal stack */
	return;
}

int      
irix_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{       
	struct irix_sys_sigreturn_args /* {
		syscallarg(struct irix_sigcontext *) scp;
		syscallarg(struct irix_ucontext *) ucp;
		syscallarg(int) signo;
	} */ *uap = v;
	void *usf;
	struct irix_sigframe ksf;
	int error;

#ifdef DEBUG_IRIX
	printf("irix_sys_sigreturn()\n");
	printf("scp = %p, ucp = %p, sig = %d\n", 
	    (void *)SCARG(uap, scp), (void *)SCARG(uap, ucp),
	    SCARG(uap, signo));
#endif /* DEBUG_IRIX */

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	usf = (void *)SCARG(uap, scp);
	if (usf == NULL) {
		usf = (void *)SCARG(uap, ucp);

		if ((error = copyin(usf, &ksf.isf_ctx.iuc, 
		    sizeof(ksf.isf_ctx))) != 0)
			return error;

		irix_get_ucontext(&ksf.isf_ctx.iuc, p);
	} else {
		if ((error = copyin(usf, &ksf.isf_ctx.isc, 
		    sizeof(ksf.isf_ctx))) != 0)
			return error;

		irix_get_sigcontext(&ksf.isf_ctx.isc, p);
	}

#ifdef DEBUG_IRIX
	printf("irix_sys_sigreturn(): returning [PC=%p SP=%p SR=0x%08lx]\n",
	    (void *)((struct frame *)(p->p_md.md_regs))->f_regs[PC], 
	    (void *)((struct frame *)(p->p_md.md_regs))->f_regs[SP], 
	    ((struct frame *)(p->p_md.md_regs))->f_regs[SR]);
#endif

	return EJUSTRETURN;
}

static void
irix_get_ucontext(ucp, p)
	struct irix_ucontext *ucp;
	struct proc *p;
{
	struct frame *f;
	sigset_t mask;

	/* Restore the register context. */
	f = (struct frame *)p->p_md.md_regs;

	if (ucp->iuc_flags & IRIX_UC_CPU) {
		(void)memcpy(&f->f_regs, &ucp->iuc_mcontext.svr4___gregs, 
		    32 * sizeof(mips_reg_t));
		/* Theses registers have different order on NetBSD and IRIX */
		f->f_regs[MULLO] = 
		    ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_MDLO];
		f->f_regs[MULHI] = 
		    ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_MDHI];
		f->f_regs[PC] = 
		    ucp->iuc_mcontext.svr4___gregs[IRIX_CTX_EPC];
	}

	if (ucp->iuc_flags & IRIX_UC_MAU) { 
#ifndef SOFTFLOAT
		/* Disable the FPU to fault in FP registers. */
		f->f_regs[SR] &= ~MIPS_SR_COP_1_BIT;
		if (p == fpcurproc) 
			fpcurproc = (struct proc *)0;
		(void)memcpy(&p->p_addr->u_pcb.pcb_fpregs, 
		    &ucp->iuc_mcontext.svr4___fpregs,
		    sizeof(p->p_addr->u_pcb.pcb_fpregs));
		p->p_addr->u_pcb.pcb_fpregs.r_regs[32] = 
		     ucp->iuc_mcontext.svr4___fpregs.svr4___fp_csr;
#else	
		(void)memcpy(&p->p_addr->u_pcb.pcb_fpregs, 
		    &ucp->iuc_mcontext.svr4___fpregs,
		    sizeof(p->p_addr->u_pcb.pcb_fpregs));
#endif
	}

	if (ucp->iuc_flags & IRIX_UC_STACK)
		; /* XXX Restore signal stack. */

	if (ucp->iuc_flags & IRIX_UC_SIGMASK) {
		/* Restore signal mask. */
		irix_to_native_sigset(&ucp->iuc_sigmask, &mask);
		(void)sigprocmask1(p, SIG_SETMASK, &mask, 0);
	}

	return;
}

static void
irix_get_sigcontext(scp, p) 
	struct irix_sigcontext *scp;
	struct proc *p;
{
	int i;
	struct frame *f;
	sigset_t mask;

	/* Restore the register context. */
	f = (struct frame *)p->p_md.md_regs;

	for (i = 1; i < 32; i++) /* restore gpr1 to gpr31 */
		f->f_regs[i] = scp->isc_regs[i];
	f->f_regs[MULLO] = scp->isc_mdlo;
	f->f_regs[MULHI] = scp->isc_mdhi;
	f->f_regs[PC] = scp->isc_pc;

#ifndef SOFTFLOAT
	if (scp->isc_ownedfp) {
		/* Disable the FPU to fault in FP registers. */
		f->f_regs[SR] &= ~MIPS_SR_COP_1_BIT;
		if (p == fpcurproc) 
			fpcurproc = (struct proc *)0;
		(void)memcpy(&p->p_addr->u_pcb.pcb_fpregs, &scp->isc_fpregs,
		    sizeof(scp->isc_fpregs));
		p->p_addr->u_pcb.pcb_fpregs.r_regs[32] = scp->isc_fpc_csr;
	}
#else	
	(void)memcpy(&p->p_addr->u_pcb.pcb_fpregs, &scp->isc_fpregs,
	    sizeof(p->p_addr->u_pcb.pcb_fpregs));
#endif

	/* Restore signal stack. */
	if (scp->isc_ssflags & IRIX_SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;
		

	/* Restore signal mask. */
	irix_to_native_sigset(&scp->isc_sigset, &mask);
	(void)sigprocmask1(p, SIG_SETMASK, &mask, 0);

	return;
}


int
irix_sys_sginap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_sginap_args /* {
		syscallarg(long) ticks;
	} */ *uap = v;
	int rticks = SCARG(uap, ticks);
	struct timeval tvb, tve, tvd;
	long long delta;
	int dontcare;

	if (rticks != 0)
		microtime(&tvb);

	if ((tsleep(&dontcare, PCATCH, 0, rticks) != 0) && (rticks != 0)) {
		microtime(&tve);
		timersub(&tve, &tvb, &tvd);
		delta = ((tvd.tv_sec * 1000000) + tvd.tv_usec); /* XXX */
		*retval = (register_t)(rticks - (delta / tick));
	}

	return 0;
}

/*
 * XXX Untested. Expect bugs and security problems here 
 */
int 
irix_sys_getcontext(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_getcontext_args /* {
		syscallarg(struct irix_ucontext *) ucp;
	} */ *uap = v;
	struct frame *f;
	struct irix_ucontext kucp;
	int i, error;

	f = (struct frame *)p->p_md.md_regs;

	kucp.iuc_flags = IRIX_UC_ALL;
	kucp.iuc_link = NULL;		/* XXX */
	native_to_irix_sigset(&p->p_sigctx.ps_sigmask, &kucp.iuc_sigmask);
	kucp.iuc_stack.ss_sp = p->p_sigctx.ps_sigstk.ss_sp;
	kucp.iuc_stack.ss_size = p->p_sigctx.ps_sigstk.ss_size;
	kucp.iuc_stack.ss_flags = 0;
	if (p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK)
		kucp.iuc_stack.ss_flags &= IRIX_SS_ONSTACK;
	if (p->p_sigctx.ps_sigstk.ss_flags & SS_DISABLE)
		kucp.iuc_stack.ss_flags &= IRIX_SS_DISABLE;

	for (i = 0; i < 36; i++) /* Is order correct? */
		kucp.iuc_mcontext.svr4___gregs[i] = f->f_regs[i];
	for (i = 0; i < 32; i++) 
		kucp.iuc_mcontext.svr4___fpregs.svr4___fp_r.svr4___fp_regs[i] 
		    = 0; /* XXX where are FP registers? */
	for (i = 0; i < 47; i++)
		kucp.iuc_filler[i] = 0;	/* XXX */
	kucp.iuc_triggersave = 0;	/* XXX */

	error = copyout(&kucp, SCARG(uap, ucp), sizeof(kucp));

	return error;
}

/*
 * XXX Untested. Expect bugs and security problems here 
 */
int 
irix_sys_setcontext(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_setcontext_args /* {
		syscallarg(struct irix_ucontext *) ucp;
	} */ *uap = v;
	struct frame *f;
	struct irix_ucontext kucp;
	int i, error;

	error = copyin(SCARG(uap, ucp), &kucp, sizeof(kucp));
	if (error)
		goto out;

	f = (struct frame *)p->p_md.md_regs;

	if (kucp.iuc_flags & IRIX_UC_SIGMASK)
		irix_to_native_sigset(&kucp.iuc_sigmask, 
		    &p->p_sigctx.ps_sigmask);

	if (kucp.iuc_flags & IRIX_UC_STACK) {
		p->p_sigctx.ps_sigstk.ss_sp = kucp.iuc_stack.ss_sp;
		p->p_sigctx.ps_sigstk.ss_size = 
		    (unsigned long)kucp.iuc_stack.ss_sp;
		p->p_sigctx.ps_sigstk.ss_flags = 0;
		if (kucp.iuc_stack.ss_flags & IRIX_SS_ONSTACK)
			p->p_sigctx.ps_sigstk.ss_flags &= SS_ONSTACK;
		if (kucp.iuc_stack.ss_flags & IRIX_SS_DISABLE)
			p->p_sigctx.ps_sigstk.ss_flags &= SS_DISABLE;
	}

	if (kucp.iuc_flags & IRIX_UC_CPU) 
		for (i = 0; i < 36; i++) /* Is register order right? */
			f->f_regs[i] = kucp.iuc_mcontext.svr4___gregs[i];

	if (kucp.iuc_flags & IRIX_UC_MAU) { /* XXX */
#ifdef DEBUG_IRIX
	printf("irix_sys_setcontext(): IRIX_UC_MAU requested\n");
#endif
	}

out:
	return error;
}


/* 
 * The following code is from svr4_sys_waitsys(), with a few lines added
 * for supporting the rusage argument which is present in the IRIX version
 * and not in the SVR4 version.
 * Both version could be merged by creating a svr4_sys_waitsys1() with the
 * rusage argument, and by calling it with NULL from svr4_sys_waitsys().
 * irix_setinfo is here because 1) svr4_setinfo is static and cannot be 
 * used here and 2) because struct irix_irix5_siginfo is quite different
 * from svr4_siginfo. In order to merge, we need to include irix_signal.h
 * from svr4_misc.c, or push the irix_irix5_siginfo into svr4_siginfo.h
 */
int 
irix_sys_waitsys(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_waitsys_args /* {
		syscallarg(int) type;
		syscallarg(int) pid;
		syscallarg(struct irix_irix5_siginfo *) info;
		syscallarg(int) options;
		syscallarg(struct rusage *) ru;
	} */ *uap = v;
	int nfound, error, s;
	struct proc *q, *t;

	switch (SCARG(uap, type)) {
	case SVR4_P_PID:	
		break;

	case SVR4_P_PGID:
		SCARG(uap, pid) = -p->p_pgid;
		break;

	case SVR4_P_ALL:
		SCARG(uap, pid) = WAIT_ANY;
		break;

	default:
		return EINVAL;
	}

#ifdef DEBUG_IRIX
	printf("waitsys(%d, %d, %p, %x, %p)\n", 
		 SCARG(uap, type), SCARG(uap, pid),
		 SCARG(uap, info), SCARG(uap, options), SCARG(uap, ru));
#endif

loop:
	nfound = 0;
	for (q = p->p_children.lh_first; q != 0; q = q->p_sibling.le_next) {
		if (SCARG(uap, pid) != WAIT_ANY &&
		    q->p_pid != SCARG(uap, pid) &&
		    q->p_pgid != -SCARG(uap, pid)) {
#ifdef DEBUG_IRIX
			printf("pid %d pgid %d != %d\n", q->p_pid,
				 q->p_pgid, SCARG(uap, pid));
#endif
			continue;
		}
		nfound++;
		if (q->p_stat == SZOMB && 
		    ((SCARG(uap, options) & (SVR4_WEXITED|SVR4_WTRAPPED)))) {
			*retval = 0;
#ifdef DEBUG_IRIX
			printf("irix_sys_wait(): found %d\n", q->p_pid);
#endif
			if ((error = irix_setinfo(q, q->p_xstat,
						  SCARG(uap, info))) != 0)
				return error;


			if ((SCARG(uap, options) & SVR4_WNOWAIT)) {
#ifdef DEBUG_IRIX
				printf(("irix_sys_wait(): Don't wait\n"));
#endif
				return 0;
			}
			if (SCARG(uap, ru) &&
			    (error = copyout(&(p->p_stats->p_ru),
			    (caddr_t)SCARG(uap, ru), sizeof(struct rusage))))
				return (error);

			/*
			 * If we got the child via ptrace(2) or procfs, and
			 * the parent is different (meaning the process was
			 * attached, rather than run as a child), then we need
			 * to give it back to the old parent, and send the
			 * parent a SIGCHLD.  The rest of the cleanup will be
			 * done when the old parent waits on the child.
			 */
			if ((q->p_flag & P_TRACED) &&
			    q->p_oppid != q->p_pptr->p_pid) {
				t = pfind(q->p_oppid);
				proc_reparent(q, t ? t : initproc);
				q->p_oppid = 0;
				q->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
				psignal(q->p_pptr, SIGCHLD);
				wakeup((caddr_t)q->p_pptr);
				return (0);
			}
			q->p_xstat = 0;
			ruadd(&p->p_stats->p_cru, q->p_ru);
			pool_put(&rusage_pool, q->p_ru);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(q);

			s = proclist_lock_write();
			LIST_REMOVE(q, p_list);	/* off zombproc */
			proclist_unlock_write(s);

			LIST_REMOVE(q, p_sibling);

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(q->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--q->p_cred->p_refcnt == 0) {
				crfree(q->p_cred->pc_ucred);
				pool_put(&pcred_pool, q->p_cred);
			}

			/*
			 * Release reference to text vnode
			 */
			if (q->p_textvp)
				vrele(q->p_textvp);

			pool_put(&proc_pool, q);
			nprocs--;
			return 0;
		}
		if (q->p_stat == SSTOP && (q->p_flag & P_WAITED) == 0 &&
		    (q->p_flag & P_TRACED ||
		     (SCARG(uap, options) & (SVR4_WSTOPPED|SVR4_WCONTINUED)))) {
#ifdef DEBUG_IRIX
			printf("jobcontrol %d\n", q->p_pid);
#endif
			if (((SCARG(uap, options) & SVR4_WNOWAIT)) == 0)
				q->p_flag |= P_WAITED;
			*retval = 0;
			return irix_setinfo(q, W_STOPCODE(q->p_xstat),
					    SCARG(uap, info));
		}
	}

	if (nfound == 0)
		return ECHILD;

	if (SCARG(uap, options) & SVR4_WNOHANG) {
		*retval = 0;
		if ((error = irix_setinfo(NULL, 0, SCARG(uap, info))) != 0)
			return error;
		return 0;
	}

	if ((error = tsleep((caddr_t)p, PWAIT | PCATCH, "svr4_wait", 0)) != 0)
		return error;
	goto loop;

}

int 
irix_sys_sigprocmask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_sigprocmask_args /* {
		syscallarg(int) how;
		syscallarg(irix_sigset_t *) set;
		syscallarg(irix_sigset_t *) oset;
	} */ *uap = v;
	struct svr4_sys_sigprocmask_args cup;
	int error;
	sigset_t *obss;
	irix_sigset_t niss, oiss;
	caddr_t sg;

	SCARG(&cup, how) = SCARG(uap, how);
	SCARG(&cup, set) = (svr4_sigset_t *)SCARG(uap, set);
	SCARG(&cup, oset) = (svr4_sigset_t *)SCARG(uap, oset);

	if (SCARG(uap, how) == IRIX_SIG_SETMASK32) {
		sg = stackgap_init(p, 0);
		if ((error = copyin(SCARG(uap, set), &niss, sizeof(niss))) != 0)
			return error;
		SCARG(&cup, set) = stackgap_alloc(p, &sg, sizeof(niss));

		obss = &p->p_sigctx.ps_sigmask;	
		native_to_irix_sigset(obss, &oiss);
		/* preserve the higher 32 bits */
		niss.bits[3] = oiss.bits[3]; 

		if ((error = copyout(&niss, (void *)SCARG(&cup, set), 
		    sizeof(niss))) != 0)
			return error;

		SCARG(&cup, how) = SVR4_SIG_SETMASK;
	}
	return svr4_sys_sigprocmask(p, &cup, retval);
}
