/*	$NetBSD: irix_signal.c,v 1.5 2002/02/17 22:49:54 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_signal.c,v 1.5 2002/02/17 22:49:54 manu Exp $");

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

#include <machine/regnum.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_wait.h>

#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscallargs.h>

extern const int native_to_svr4_sig[];
extern const int svr4_to_native_sig[];

static int irix_setinfo __P((struct proc *, int, irix_irix5_siginfo_t *));

#define irix_sigmask(n)	 (1 << (((n) - 1) & 31))
#define irix_sigword(n)	 (((n) - 1) >> 5) 
#define irix_sigemptyset(s)     memset((s), 0, sizeof(*(s)))
#define irix_sigismember(s, n)  ((s)->bits[irix_sigword(n)] & irix_sigmask(n))
#define irix_sigaddset(s, n)    ((s)->bits[irix_sigword(n)] |= irix_sigmask(n))

/* 
 * This is ripped from svr4_setinfo. See irix_sys_waitsys...
 */
int
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
			i.isi_status = native_to_svr4_sig[sig];

		if (i.isi_status == SVR4_SIGCONT)
			i.isi_code = SVR4_CLD_CONTINUED;
		else
			i.isi_code = SVR4_CLD_STOPPED;
	} else {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			i.isi_status = native_to_svr4_sig[sig];

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
			 newsig = native_to_svr4_sig[i];
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
			newsig = svr4_to_native_sig[i];
			if (newsig)
				sigaddset(bss, newsig);
		}
	}
}

void
irix_sendsig(catcher, sig, mask, code)  /* XXX Check me */
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct irix_sigcontext *fp;
	struct frame *f;
	int i,onstack;
	struct irix_sigcontext sc;
 
#ifdef DEBUG_IRIX
	printf("irix_sendsig()\n");
	printf("catcher = %p, sig = %d, code = 0x%lx\n",
	    (void *)catcher, sig, code);
#endif /* DEBUG_IRIX */
	f = (struct frame *)p->p_md.md_regs;

	/*
	 * Do we need to jump onto the signal stack?
	 */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * not sure it works yet.
	 */
	onstack=0;

	/*
	 * Allocate space for the signal handler context.
	 */
	if (onstack)
		fp = (struct irix_sigcontext *)
		    ((caddr_t)p->p_sigctx.ps_sigstk.ss_sp
		    + p->p_sigctx.ps_sigstk.ss_size);
	else
		/* cast for _MIPS_BSD_API == _MIPS_BSD_API_LP32_64CLEAN case */
		fp = (struct irix_sigcontext *)(u_int32_t)f->f_regs[SP];

	/*
	 * Build stack frame for signal trampoline.
	 */
	memset(&sc, 0, sizeof sc);

	native_to_irix_sigset(mask, &sc.isc_sigset);
	for (i = 1; i < 32; i++) { /* save gpr1 - gpr31 */
		sc.isc_regs[i] = f->f_regs[i];
	}
	sc.isc_regs[0] = 0;
	sc.isc_fp_rounded_result = 0;
	sc.isc_regmask = ~0x1UL;
	sc.isc_mdhi = f->f_regs[MULHI];
	sc.isc_mdlo = f->f_regs[MULLO];
	sc.isc_pc = f->f_regs[PC];
	sc.isc_ownedfp = 0;
	sc.isc_ssflags = 0;

	/*
	 * Save signal stack.  XXX broken
	 */
	/* kregs.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK; */

	/*
	 * Install the sigframe onto the stack
	 */
	fp = (struct irix_sigcontext *)((unsigned long)fp
	    - sizeof(struct irix_sigcontext));
	fp = (struct irix_sigcontext *)((unsigned long)fp 
	    & ~0xfUL);		/* 16 bytes alignement */
	if (copyout(&sc, fp, sizeof(sc)) != 0) {
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

	/* Set up the registers to return to sigcode. */
	f->f_regs[A0] = native_to_svr4_sig[sig];
	f->f_regs[A1] = 0;
	f->f_regs[A2] = (unsigned long)fp;

#ifdef DEBUG_IRIX
	printf("sigcontext is at %p\n", fp);
#endif /* DEBUG_IRIX */

	f->f_regs[RA] = (unsigned long)p->p_sigctx.ps_sigcode;
	f->f_regs[SP] = (unsigned long)fp;
	f->f_regs[T9] = (unsigned long)catcher;
	f->f_regs[A3] = (unsigned long)catcher;
	f->f_regs[PC] = (unsigned long)catcher;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG_IRIX
	printf("returning from irix_sendsig()\n");
#endif
	return;
}

int      
irix_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{       
	struct irix_sys_sigreturn_args /* {
		syscallarg(struct irix_sigreturna) isr;
	} */ *uap = v;
	struct irix_sigcontext *sc, ksc;
	struct frame *f; 
	sigset_t mask;
	int i, error;

#ifdef DEBUG_IRIX
	printf("irix_sys_sigreturn()\n");
	printf("scp = %p, ucp = %p, sig = %d (%p)\n", 
	    (void *)SCARG(uap, isr).scp, (void *)SCARG(uap, isr).ucp, 
	    SCARG(uap, isr).signo, (void *)SCARG(uap, isr).signo);
#endif /* DEBUG_IRIX */

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	sc = SCARG(uap, isr).scp;
	if (sc == NULL)
		sc = SCARG(uap, isr).ucp;

	if ((error = copyin(sc, &ksc, sizeof(ksc))) != 0)
		return (error);

	/* Restore the register context. */
	f = (struct frame *)p->p_md.md_regs;
	for (i = 1; i < 32; i++) /* restore gpr1 to gpr31 */
		f->f_regs[i] = ksc.isc_regs[i];
	f->f_regs[MULLO] = ksc.isc_mdlo;
	f->f_regs[MULHI] = ksc.isc_mdhi;
	f->f_regs[PC] = ksc.isc_pc;

	/* Restore signal stack. */
	p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	irix_to_native_sigset((irix_sigset_t *)&ksc.isc_sigset, &mask);
	(void)sigprocmask1(p, SIG_SETMASK, &mask, 0);

#ifdef DEBUG_IRIX
	printf("irix_sys_sigreturn(): returning\n");
#endif
	return (EJUSTRETURN);
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
	printf("waitsys(%d, %d, %p, %x)\n", 
		 SCARG(uap, type), SCARG(uap, pid),
		 SCARG(uap, info), SCARG(uap, options));
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
			printf("found %d\n", q->p_pid);
#endif
			if ((error = irix_setinfo(q, q->p_xstat,
						  SCARG(uap, info))) != 0)
				return error;


			if ((SCARG(uap, options) & SVR4_WNOWAIT)) {
#ifdef DEBUG_IRIX
				printf(("Don't wait\n"));
#endif
				return 0;
			}
			if (SCARG(uap, ru) &&
			    (error = copyout((caddr_t)p->p_ru,
			    (caddr_t)SCARG(uap, ru),
			    sizeof(struct rusage))))
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
