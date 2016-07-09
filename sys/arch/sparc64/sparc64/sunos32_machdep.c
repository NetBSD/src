/*	$NetBSD: sunos32_machdep.c,v 1.32.32.1 2016/07/09 20:24:57 skrll Exp $	*/
/* from: NetBSD: sunos_machdep.c,v 1.14 2001/01/29 01:37:56 mrg Exp 	*/

/*
 * Copyright (c) 1995, 2001 Matthew R. Green
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
__KERNEL_RCSID(0, "$NetBSD: sunos32_machdep.c,v 1.32.32.1 2016/07/09 20:24:57 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "firm_events.h"
#endif

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/select.h>

#include <sys/syscallargs.h>
#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscallargs.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_syscallargs.h>
#include <compat/sunos32/sunos32_exec.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/vuid_event.h>
#include <machine/reg.h>

#include <dev/sun/event_var.h>

#ifdef DEBUG
#include <sparc64/sparc64/sigdebug.h>
#endif

struct sunos32_sigcontext {
	uint32_t	sc_onstack;		/* sigstack state to restore */
	uint32_t	sc_mask;		/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	uint32_t	sc_sp;			/* %sp to restore */
	uint32_t	sc_pc;			/* pc to restore */
	uint32_t	sc_npc;			/* npc to restore */
	uint32_t	sc_psr;			/* pstate to restore */
	uint32_t	sc_g1;			/* %g1 to restore */
	uint32_t	sc_o0;			/* %o0 to restore */
};

struct sunos32_sigframe {
	uint32_t	sf_signo;		/* signal number */
	uint32_t	sf_code;		/* code */
	uint32_t	sf_scp;			/* SunOS user addr of sigcontext */
	uint32_t	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sunos32_sigcontext sf_sc;	/* actual sigcontext */
};

#if NFIRM_EVENTS > 0
static int ev_out32(struct firm_event *, int, struct uio *);
#endif

/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
sunos32_setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
	/* stack:  XXX */
{
	struct trapframe64 *tf = l->l_md.md_tf;
	struct fpstate64 *fs;
	int64_t tstate;
	struct proc *p = l->l_proc;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/* Mark this as a 32-bit emulation */
	mutex_enter(p->p_lock);
	p->p_flag |= PK_32;
	mutex_exit(p->p_lock);

	/* Setup the ev_out32 hook */
#if NFIRM_EVENTS > 0
	if (ev_out32_hook == NULL)
		ev_out32_hook = ev_out32;
#endif

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: p->p_psstrp (used by crt0)
	 *	%tpc,%tnpc: entry point of program
	 */
	tstate = ((PSTATE_USER32)<<TSTATE_PSTATE_SHIFT) 
		| (tf->tf_tstate & TSTATE_CWP);
	if ((fs = l->l_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		if (l == fplwp) {
			savefpstate(fs);
			fplwp = NULL;
		}
		pool_cache_put(fpstate_cache, fs);
		l->l_md.md_fpstate = NULL;
	}
	memset(tf, 0, sizeof *tf);
	tf->tf_tstate = tstate;
	tf->tf_global[1] = (u_int)p->p_psstrp;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;

	stack -= sizeof(struct rwindow32);
	tf->tf_out[6] = stack;
	tf->tf_out[7] = 0;
}

void
sunos32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;	/* XXX */
	struct proc *p = l->l_proc;
	struct sunos32_sigframe *fp;
	struct trapframe64 *tf;
	struct rwindow32 *oldsp, *newsp;
	struct sunos32_sigframe sf;
	struct sunos32_sigcontext *scp;
	uint32_t addr, oldsp32;
	int onstack, error;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = l->l_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	oldsp32 = (uint32_t)(u_long)oldsp;

	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	if (onstack)
		fp = (struct sunos32_sigframe *)
		     ((char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size);
	else
		fp = (struct sunos32_sigframe *)oldsp;

	fp = (struct sunos32_sigframe *)((u_long)(fp - 1) & ~7);

#ifdef DEBUG
	sigpid = p->p_pid;
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		mutex_exit(p->p_lock);
		printf("sunos32_sendsig: %s[%d] sig %d newusp %p scp %p oldsp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc, oldsp);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
		mutex_enter(p->p_lock);
	}
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = (uint32_t)ksi->ksi_trap;
	scp = &fp->sf_sc;
	if ((u_long)scp >= 0x100000000)
		printf("sunos32_sendsig: sf_scp overflow %p > 0x100000000\n", scp);
	sf.sf_scp = (uint32_t)(u_long)scp;
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;
	native_sigset_to_sigset13(mask, &sf.sf_sc.sc_mask);
	sf.sf_sc.sc_sp = (u_int)(u_long)oldsp;
	sf.sf_sc.sc_pc = tf->tf_pc;
	sf.sf_sc.sc_npc = tf->tf_npc;
	sf.sf_sc.sc_psr = TSTATECCR_TO_PSR(tf->tf_tstate); /* XXX */
	sf.sf_sc.sc_g1 = tf->tf_global[1];
	sf.sf_sc.sc_o0 = tf->tf_out[0];

	/*
	 * Put the stack in a consistent state before we whack away
	 * at it.  Note that write_user_windows may just dump the
	 * registers into the pcb; we need them in the process's memory.
	 * We also need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 */
	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);
	newsp = (struct rwindow32 *)((long)fp - sizeof(struct rwindow32));
	write_user_windows();
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK))
	    printf("sunos32_sendsig: saving sf to %p, setting stack pointer %p to %p\n",
		   fp, &(((struct rwindow32 *)newsp)->rw_in[6]), oldsp);
#endif
	error = (rwindow_save(l) || copyout((void *)&sf, (void *)fp, sizeof sf) || 
	    copyout((void *)&oldsp32, &(((struct rwindow32 *)newsp)->rw_in[6]), sizeof oldsp32));
	mutex_enter(p->p_lock);
	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		mutex_exit(p->p_lock);
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sunos32_sendsig: window save or copyout error\n");
		printf("sunos32_sendsig: stack was trashed trying to send sig %d, sending SIGILL\n", sig);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
		mutex_enter(p->p_lock);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW)) {
		printf("sunos32_sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
	}
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	addr = (uint32_t)(u_long)catcher;	/* user does his own trampolining */
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (uint64_t)(u_int)(u_long)newsp;
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		mutex_exit(p->p_lock);
		printf("sunos32_sendsig: about to return to catcher %p thru %p\n", 
		       catcher, (void *)(u_long)addr);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
		mutex_enter(p->p_lock);
	}
#endif
}

int
sunos32_sys_sigreturn(struct lwp *l, const struct sunos32_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_sigcontextp_t) sigcntxp;
	} */
	struct proc *p = l->l_proc;
	struct sunos32_sigcontext sc, *scp;
	sigset_t mask;
	struct trapframe64 *tf;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sunos32_sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, (void *)(u_long)SCARG(uap, sigcntxp));
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	scp = (struct sunos32_sigcontext *)(u_long)SCARG(uap, sigcntxp);
	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
		return (EFAULT);
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((scp->sc_pc | scp->sc_npc) & 3) != 0 || scp->sc_pc == 0 || scp->sc_npc == 0)
	{
#ifdef DEBUG
		printf("sunos32_sigreturn: pc %x or npc %x invalid\n", scp->sc_pc, scp->sc_npc);
#ifdef DDB
		Debugger();
#endif
#endif
		return (EINVAL);
	}
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(scp->sc_psr);
	tf->tf_pc = scp->sc_pc;
	tf->tf_npc = scp->sc_npc;
	tf->tf_global[1] = scp->sc_g1;
	tf->tf_out[0] = scp->sc_o0;
	tf->tf_out[6] = scp->sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sunos32_sigreturn: return trapframe pc=%p sp=%p tstate=%llx\n",
		       (void *)(u_long)tf->tf_pc, (void *)(u_long)tf->tf_out[6], (unsigned long long)tf->tf_tstate);
#ifdef DDB
		if (sigdebug & SDB_DDB) Debugger();
#endif
	}
#endif

	mutex_enter(p->p_lock);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask */
	native_sigset13_to_sigset(&scp->sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}

#if NFIRM_EVENTS > 0
/*
 * Write out a series of 32-bit firm_events.
 */
static int
ev_out32(struct firm_event *e, int n, struct uio *uio)
{
	struct firm_event32 e32;
	int error = 0;

	while (n-- && error == 0) {
		e32.id = e->id;
		e32.value = e->value;
		e32.time.tv_sec = e->time.tv_sec;
		e32.time.tv_usec = e->time.tv_usec;
		error = uiomove((void *)&e32, sizeof(e32), uio);
		e++;
	}
	return (error);
}
#endif
