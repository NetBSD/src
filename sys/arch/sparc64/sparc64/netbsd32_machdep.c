/*	$NetBSD: netbsd32_machdep.c,v 1.72.10.2 2008/01/09 01:49:08 matt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep.c,v 1.72.10.2 2008/01/09 01:49:08 matt Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "firm_events.h"
#endif

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/ucontext.h>
#include <sys/ioctl.h>

#include <dev/sun/event_var.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_exec.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#include <compat/sys/siginfo.h>
#include <compat/sys/ucontext.h>

#ifndef SUN4U
#define SUN4U	/* see .../sparc/include/frame.h for the reason */
#endif
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/vuid_event.h>
#include <machine/netbsd32_machdep.h>
#include <machine/userret.h>

/* Provide a the name of the architecture we're emulating */
const char	machine32[] = "sparc";	
const char	machine_arch32[] = "sparc";	

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
netbsd32_setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct proc *p = l->l_proc;
	struct trapframe64 *tf = l->l_md.md_tf;
	struct fpstate64 *fs;
	int64_t tstate;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/* Mark this as a 32-bit emulation */
	p->p_flag |= PK_32;

	netbsd32_adjust_limits(p);

	/* Setup the ev_out32 hook */
#if NFIRM_EVENTS > 0
	if (ev_out32_hook == NULL)
		ev_out32_hook = ev_out32;
#endif

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: address of p->p_psstr (used by crt0)
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
		free((void *)fs, M_SUBPROC);
		l->l_md.md_fpstate = NULL;
	}
	memset(tf, 0, sizeof *tf);
	tf->tf_tstate = tstate;
	tf->tf_global[1] = (u_int)(u_long)p->p_psstr;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;

	stack -= sizeof(struct rwindow32);
	tf->tf_out[6] = stack;
	tf->tf_out[7] = 0;
}

#ifdef COMPAT_16
/*
 * NB: since this is a 32-bit address world, sf_scp and sf_sc
 *	can't be a pointer since those are 64-bits wide.
 */
struct sparc32_sigframe {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	u_int	sf_scp;			/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	netbsd32_sigcontext sf_sc;	/* actual sigcontext */
};

#undef DEBUG
#ifdef DEBUG
extern int sigdebug;
#endif

static void
netbsd32_sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sparc32_sigframe *fp;
	struct trapframe64 *tf;
	int addr, onstack, error;
	struct rwindow32 *kwin, *oldsp, *newsp;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sparc32_sigframe sf;
	extern char netbsd32_sigcode[], netbsd32_esigcode[];
#define	szsigcode	(netbsd32_esigcode - netbsd32_sigcode)

	tf = l->l_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (onstack) {
		fp = (struct sparc32_sigframe *)((char *)l->l_sigstk.ss_sp +
					l->l_sigstk.ss_size);
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sparc32_sigframe *)oldsp;
	fp = (struct sparc32_sigframe *)((u_long)(fp - 1) & ~7);

#ifdef DEBUG
	sigpid = p->p_pid;
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sendsig: %s[%d] sig %d newusp %p scp %p oldsp %p\n",
		    p->p_comm, p->p_pid, sig, fp, &fp->sf_sc, oldsp);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	/*
	 * Now set up the signal frame.  We build it in kernel space
	 * and then copy it out.  We probably ought to just build it
	 * directly in user space....
	 */
	sf.sf_signo = sig;
	sf.sf_code = (u_int)ksi->ksi_trap;
#if defined(COMPAT_SUNOS) || defined(LKM)
	sf.sf_scp = (u_long)&fp->sf_sc;
#endif
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = onstack;
	sf.sf_sc.sc_mask = *mask;
	sf.sf_sc.sc_sp = (u_long)oldsp;
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
	mutex_exit(&p->p_smutex);
	newsp = (struct rwindow32 *)((long)fp - sizeof(struct rwindow32));
	write_user_windows();
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK))
	    printf("sendsig: saving sf to %p, setting stack pointer %p to %p\n",
		   fp, &(((struct rwindow32 *)newsp)->rw_in[6]), oldsp);
#endif
	kwin = (struct rwindow32 *)((char *)tf - CCFSZ);
	error = (rwindow_save(l) || 
	    copyout((void *)&sf, (void *)fp, sizeof sf) || 
	    suword(&(((struct rwindow32 *)newsp)->rw_in[6]), (u_long)oldsp));
	mutex_enter(&p->p_smutex);
	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		mutex_exit(&p->p_smutex);
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig: window save or copyout error\n");
		printf("sendsig: stack was trashed trying to send sig %d, sending SIGILL\n", sig);
		if (sigdebug & SDB_DDB) Debugger();
		mutex_enter(&p->p_smutex);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sendsig: %s[%d] sig %d scp %p\n",
		       p->p_comm, p->p_pid, sig, &fp->sf_sc);
	}
#endif
	/*
	 * Arrange to continue execution at the code copied out in exec().
	 * It needs the function to call in %g1, and a new stack pointer.
	 */
	addr = (long)p->p_psstr - szsigcode;
	tf->tf_global[1] = (long)catcher;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (uint64_t)(u_int)(u_long)newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		mutex_exit(&p->p_smutex);
		printf("sendsig: about to return to catcher %p thru %p\n", 
		       catcher, addr);
		if (sigdebug & SDB_DDB) Debugger();
		mutex_enter(&p->p_smutex);
	}
#endif
}
#endif

struct sparc32_sigframe_siginfo {
	siginfo32_t sf_si;
	ucontext32_t sf_uc;
};

static void
netbsd32_sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack;
	int sig = ksi->ksi_signo;
	ucontext32_t uc;
	struct sparc32_sigframe_siginfo *fp;
	netbsd32_intptr_t catcher;
	struct trapframe64 *tf = l->l_md.md_tf;
	struct rwindow32 *oldsp, *newsp;
	int ucsz, error;

	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32*)(u_long)(u_int)tf->tf_out[6];
	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sparc32_sigframe_siginfo *)
		    ((char *)l->l_sigstk.ss_sp +
					  l->l_sigstk.ss_size);
	else
		fp = (struct sparc32_sigframe_siginfo *)oldsp;
	fp = (struct sparc32_sigframe_siginfo*)((u_long)(fp - 1) & ~7);
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	uc.uc_flags = _UC_SIGMASK |
		((l->l_sigstk.ss_flags & SS_ONSTACK)
			? _UC_SETSTACK : _UC_CLRSTACK);
	uc.uc_sigmask = *mask;
	uc.uc_link = (uint32_t)(uintptr_t)l->l_ctxlink;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));

	sendsig_reset(l, sig);

	/*
	 * Now copy the stack contents out to user space.
	 * We need to make sure that when we start the signal handler,
	 * its %i6 (%fp), which is loaded from the newly allocated stack area,
	 * joins seamlessly with the frame it was in when the signal occurred,
	 * so that the debugger and _longjmp code can back up through it.
	 * Since we're calling the handler directly, allocate a full size
	 * C stack frame.
	 */
	mutex_exit(&p->p_smutex);
	cpu_getmcontext32(l, &uc.uc_mcontext, &uc.uc_flags);
	ucsz = (int)(intptr_t)&uc.__uc_pad - (int)(intptr_t)&uc;
	newsp = (struct rwindow32*)((intptr_t)fp - sizeof(struct frame32));
	error = (copyout(&ksi->ksi_info, &fp->sf_si, sizeof ksi->ksi_info) ||
	    copyout(&uc, &fp->sf_uc, ucsz) ||
	    suword(&newsp->rw_in[6], (intptr_t)oldsp));
	mutex_enter(&p->p_smutex);

	if (error) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	switch (ps->sa_sigdesc[sig].sd_vers) {
	default:
		/* Unsupported trampoline version; kill the process. */
		sigexit(l, SIGILL);
	case 2:
		/*
		 * Arrange to continue execution at the user's handler.
		 * It needs a new stack pointer, a return address and
		 * three arguments: (signo, siginfo *, ucontext *).
		 */
		catcher = (intptr_t)SIGACTION(p, sig).sa_handler;
		tf->tf_pc = catcher;
		tf->tf_npc = catcher + 4;
		tf->tf_out[0] = sig;
		tf->tf_out[1] = (intptr_t)&fp->sf_si;
		tf->tf_out[2] = (intptr_t)&fp->sf_uc;
		tf->tf_out[6] = (intptr_t)newsp;
		tf->tf_out[7] = (intptr_t)ps->sa_sigdesc[sig].sd_tramp - 8;
		break;
	}

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
netbsd32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
#ifdef COMPAT_16
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		netbsd32_sendsig_sigcontext(ksi, mask);
	else
#endif
		netbsd32_sendsig_siginfo(ksi, mask);
}

#undef DEBUG

#ifdef COMPAT_13
int
compat_13_netbsd32_sigreturn(struct lwp *l, const struct compat_13_netbsd32_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct netbsd32_sigcontext13 *) sigcntxp;
	} */
	struct netbsd32_sigcontext13 *scp;
	struct netbsd32_sigcontext13 sc;
	struct trapframe64 *tf;
	struct proc *p = l->l_proc;
	sigset_t mask;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
#ifdef DEBUG
		printf("compat_13_netbsd32_sigreturn: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		mutex_enter(&p->p_smutex);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_netbsd32_sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct netbsd32_sigcontext13 *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
	{
#ifdef DEBUG
		printf("compat_13_netbsd32_sigreturn: copyin failed\n");
		Debugger();
#endif
		return (EINVAL);
	}
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((sc.sc_pc | sc.sc_npc) & 3) != 0)
#ifdef DEBUG
	{
		printf("compat_13_netbsd32_sigreturn: pc %p or npc %p invalid\n", sc.sc_pc, sc.sc_npc);
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(sc.sc_psr);
	tf->tf_pc = (int64_t)sc.sc_pc;
	tf->tf_npc = (int64_t)sc.sc_npc;
	tf->tf_global[1] = (int64_t)sc.sc_g1;
	tf->tf_out[0] = (int64_t)sc.sc_o0;
	tf->tf_out[6] = (int64_t)sc.sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_netbsd32_sys_sigreturn: return trapframe pc=%p sp=%p tstate=%x\n",
		       (int)tf->tf_pc, (int)tf->tf_out[6], (int)tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	mutex_enter(&p->p_smutex);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask */
	native_sigset13_to_sigset((sigset13_t *)&scp->sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(&p->p_smutex);

	return (EJUSTRETURN);
}
#endif

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above),
 * and return to the given trap frame (if there is one).
 * Check carefully to make sure that the user has not
 * modified the state to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
compat_16_netbsd32___sigreturn14(struct lwp *l, const struct compat_16_netbsd32___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct netbsd32_sigcontext sc, *scp;
	struct trapframe64 *tf;
	struct proc *p = l->l_proc;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
#ifdef DEBUG
		printf("netbsd32_sigreturn14: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		mutex_enter(&p->p_smutex);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("netbsd32_sigreturn14: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct netbsd32_sigcontext *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
	{
#ifdef DEBUG
		printf("netbsd32_sigreturn14: copyin failed: scp=%p\n", scp);
		Debugger();
#endif
		return (EINVAL);
	}
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((sc.sc_pc | sc.sc_npc) & 3) != 0 || (sc.sc_pc == 0) || (sc.sc_npc == 0))
#ifdef DEBUG
	{
		printf("netbsd32_sigreturn14: pc %p or npc %p invalid\n", sc.sc_pc, sc.sc_npc);
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(sc.sc_psr);
	tf->tf_pc = (int64_t)sc.sc_pc;
	tf->tf_npc = (int64_t)sc.sc_npc;
	tf->tf_global[1] = (int64_t)sc.sc_g1;
	tf->tf_out[0] = (int64_t)sc.sc_o0;
	tf->tf_out[6] = (int64_t)sc.sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("netbsd32_sigreturn14: return trapframe pc=%p sp=%p tstate=%llx\n",
		       (vaddr_t)tf->tf_pc, (vaddr_t)tf->tf_out[6], tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif

	/* Restore signal stack. */
	mutex_enter(&p->p_smutex);
	if (sc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &sc.sc_mask, 0);
	mutex_exit(&p->p_smutex);

	return (EJUSTRETURN);
}

/* Unfortunately we need to convert v9 trapframe to v8 regs */
int
netbsd32_process_read_regs(struct lwp *l, struct reg32 *regs)
{
	struct trapframe64* tf = l->l_md.md_tf;
	int i;

	/* 
	 * Um, we should only do this conversion for 32-bit emulation
	 * or when running 32-bit mode.  We really need to pass in a
	 * 32-bit emulation flag!
	 */

	regs->r_psr = TSTATECCR_TO_PSR(tf->tf_tstate);
	regs->r_pc = tf->tf_pc;
	regs->r_npc = tf->tf_npc;
	regs->r_y = tf->tf_y;
	for (i = 0; i < 8; i++) {
		regs->r_global[i] = tf->tf_global[i];
		regs->r_out[i] = tf->tf_out[i];
	}
	/* We should also write out the ins and locals.  See signal stuff */
	return (0);
}

#if 0
int
netbsd32_process_write_regs(struct proc *p, const struct reg *regs)
{
	const struct reg32* regp = (const struct reg32*)regs;
	struct trapframe64* tf = p->p_md.md_tf;
	int i;

	tf->tf_pc = regp->r_pc;
	tf->tf_npc = regp->r_npc;
	tf->tf_y = regp->r_pc;
	for (i = 0; i < 8; i++) {
		tf->tf_global[i] = regp->r_global[i];
		tf->tf_out[i] = regp->r_out[i];
	}
	/* We should also read in the ins and locals.  See signal stuff */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(regp->r_psr);
	return (0);
}
#endif

int
netbsd32_process_read_fpregs(struct lwp *l, struct fpreg32 *regs)
{
	extern struct fpstate64	initfpstate;
	struct fpstate64	*statep = &initfpstate;
	int i;

	if (l->l_md.md_fpstate)
		statep = l->l_md.md_fpstate;
	for (i=0; i<32; i++)
		regs->fr_regs[i] = statep->fs_regs[i];

	return 0;
}

#if 0
int
netbsd32_process_write_fpregs(struct proc *p, const struct fpreg *regs)
{
	extern struct fpstate	initfpstate;
	struct fpstate64	*statep = &initfpstate;
	const struct fpreg32	*regp = (const struct fpreg32 *)regs;
	int i;

	/* NOTE: struct fpreg == struct fpstate */
	if (p->p_md.md_fpstate)
		statep = p->p_md.md_fpstate;
	for (i=0; i<32; i++)
		statep->fs_regs[i] = regp->fr_regs[i];
	statep->fs_fsr = regp->fr_fsr;
	statep->fs_qsize = regp->fr_qsize;
	for (i=0; i<regp->fr_qsize; i++)
		statep->fs_queue[i] = regp->fr_queue[i];

	return 0;
}
#endif

/*
 * 32-bit version of cpu_coredump.
 */
int
cpu_coredump32(struct lwp *l, void *iocookie, struct core32 *chdr)
{
	int i, error;
	struct md_coredump32 md_core;
	struct coreseg32 cseg;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core);
		chdr->c_nseg++;
		return 0;
	}

	/* Fake a v8 trapframe */
	md_core.md_tf.tf_psr = TSTATECCR_TO_PSR(l->l_md.md_tf->tf_tstate);
	md_core.md_tf.tf_pc = l->l_md.md_tf->tf_pc;
	md_core.md_tf.tf_npc = l->l_md.md_tf->tf_npc;
	md_core.md_tf.tf_y = l->l_md.md_tf->tf_y;
	for (i=0; i<8; i++) {
		md_core.md_tf.tf_global[i] = l->l_md.md_tf->tf_global[i];
		md_core.md_tf.tf_out[i] = l->l_md.md_tf->tf_out[i];
	}

	if (l->l_md.md_fpstate) {
		if (l == fplwp) {
			savefpstate(l->l_md.md_fpstate);
			fplwp = NULL;
		}
		/* Copy individual fields */
		for (i=0; i<32; i++)
			md_core.md_fpstate.fs_regs[i] = 
				l->l_md.md_fpstate->fs_regs[i];
		md_core.md_fpstate.fs_fsr = l->l_md.md_fpstate->fs_fsr;
		i = md_core.md_fpstate.fs_qsize = l->l_md.md_fpstate->fs_qsize;
		/* Should always be zero */
		while (i--)
			md_core.md_fpstate.fs_queue[i] = 
				l->l_md.md_fpstate->fs_queue[i];
	} else
		memset(&md_core.md_fpstate, 0,
		      sizeof(md_core.md_fpstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &md_core,
	    sizeof(md_core));
}

void netbsd32_cpu_getmcontext(struct lwp *, mcontext_t  *, unsigned int *);

void
netbsd32_cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	/* netbsd32_mcontext_t XXX */mcontext_t  *mcp;
	unsigned int *flags;
{
#if 0
/* XXX */
	greg32_t *gr = mcp->__gregs;
	const struct trapframe64 *tf = l->l_md.md_tf;

	/* First ensure consistent stack state (see sendsig). */ /* XXX? */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(&l->l_proc->p_smutex);
		sigexit(l, SIGILL);
	}

	/* For now: Erase any random indicators for optional state. */
	(void)memset(mcp, 0, sizeof (*mcp));

	/* Save general register context. */
	gr[_REG_PSR] = TSTATECCR_TO_PSR(tf->tf_tstate);
	gr[_REG_PC]  = tf->tf_pc;
	gr[_REG_nPC] = tf->tf_npc;
	gr[_REG_Y]   = tf->tf_y;
	gr[_REG_G1]  = tf->tf_global[1];
	gr[_REG_G2]  = tf->tf_global[2];
	gr[_REG_G3]  = tf->tf_global[3];
	gr[_REG_G4]  = tf->tf_global[4];
	gr[_REG_G5]  = tf->tf_global[5];
	gr[_REG_G6]  = tf->tf_global[6];
	gr[_REG_G7]  = tf->tf_global[7];
	gr[_REG_O0]  = tf->tf_out[0];
	gr[_REG_O1]  = tf->tf_out[1];
	gr[_REG_O2]  = tf->tf_out[2];
	gr[_REG_O3]  = tf->tf_out[3];
	gr[_REG_O4]  = tf->tf_out[4];
	gr[_REG_O5]  = tf->tf_out[5];
	gr[_REG_O6]  = tf->tf_out[6];
	gr[_REG_O7]  = tf->tf_out[7];
	*flags |= _UC_CPU;

	mcp->__gwins = 0;


	/* Save FP register context, if any. */
	if (l->l_md.md_fpstate != NULL) {
		struct fpstate fs, *fsp;
		netbsd32_fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If our FP context is currently held in the FPU, take a
		 * private snapshot - lazy FPU context switching can deal
		 * with it later when it becomes necessary.
		 * Otherwise, get it from the process's save area.
		 */
		if (p == fplwp) {
			fsp = &fs;
			savefpstate(fsp);
		} else {
			fsp = l->l_md.md_fpstate;
		}
		memcpy(&fpr->__fpu_fr, fsp->fs_regs, sizeof (fpr->__fpu_fr));
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_fsr = fs.fs_fsr;
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
		mcp->__fpregs.__fpu_q_entrysize =
		    sizeof (struct netbsd32_fq);
		mcp->__fpregs.__fpu_en = 1;
		*flags |= _UC_FPU;
	} else {
		mcp->__fpregs.__fpu_en = 0;
	}

	mcp->__xrs.__xrs_id = 0;	/* Solaris extension? */
#endif
}


int netbsd32_cpu_setmcontext(struct lwp *, mcontext_t *, unsigned int);

int
netbsd32_cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	/* XXX const netbsd32_*/mcontext_t *mcp;
	unsigned int flags;
{
#ifdef NOT_YET
/* XXX */
	greg32_t *gr = mcp->__gregs;
	struct trapframe64 *tf = l->l_md.md_tf;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p)) {
		mutex_enter(&l->l_proc->p_smutex);
		sigexit(p, SIGILL);
	}

	if ((flags & _UC_CPU) != 0) {
		/*
	 	 * Only the icc bits in the psr are used, so it need not be
	 	 * verified.  pc and npc must be multiples of 4.  This is all
	 	 * that is required; if it holds, just do it.
		 */
		if (((gr[_REG_PC] | gr[_REG_nPC]) & 3) != 0 ||
		    gr[_REG_PC] == 0 || gr[_REG_nPC] == 0)
			return (EINVAL);

		/* Restore general register context. */
		/* take only tstate CCR (and ASI) fields */
		tf->tf_tstate = (tf->tf_tstate & ~TSTATE_CCR) |
		    PSRCC_TO_TSTATE(gr[_REG_PSR]);
		tf->tf_pc        = (uint64_t)gr[_REG_PC];
		tf->tf_npc       = (uint64_t)gr[_REG_nPC];
		tf->tf_y         = (uint64_t)gr[_REG_Y];
		tf->tf_global[1] = (uint64_t)gr[_REG_G1];
		tf->tf_global[2] = (uint64_t)gr[_REG_G2];
		tf->tf_global[3] = (uint64_t)gr[_REG_G3];
		tf->tf_global[4] = (uint64_t)gr[_REG_G4];
		tf->tf_global[5] = (uint64_t)gr[_REG_G5];
		tf->tf_global[6] = (uint64_t)gr[_REG_G6];
		tf->tf_global[7] = (uint64_t)gr[_REG_G7];
		tf->tf_out[0]    = (uint64_t)gr[_REG_O0];
		tf->tf_out[1]    = (uint64_t)gr[_REG_O1];
		tf->tf_out[2]    = (uint64_t)gr[_REG_O2];
		tf->tf_out[3]    = (uint64_t)gr[_REG_O3];
		tf->tf_out[4]    = (uint64_t)gr[_REG_O4];
		tf->tf_out[5]    = (uint64_t)gr[_REG_O5];
		tf->tf_out[6]    = (uint64_t)gr[_REG_O6];
		tf->tf_out[7]    = (uint64_t)gr[_REG_O7];
		/* %asi restored above; %fprs not yet supported. */

		/* XXX mcp->__gwins */
	}

	/* Restore FP register context, if any. */
	if ((flags & _UC_FPU) != 0 && mcp->__fpregs.__fpu_en != 0) {
		struct fpstate *fsp;
		const netbsd32_fpregset_t *fpr = &mcp->__fpregs;
		int reload = 0;

		/*
		 * If we're the current FPU owner, simply reload it from
		 * the supplied context.  Otherwise, store it into the
		 * process' FPU save area (which is used to restore from
		 * by lazy FPU context switching); allocate it if necessary.
		 */
		/*
		 * XXX Should we really activate the supplied FPU context
		 * XXX immediately or just fault it in later?
		 */
		if ((fsp = l->l_md.md_fpstate) == NULL) {
			fsp = malloc(sizeof (*fsp), M_SUBPROC, M_WAITOK);
			l->l_md.md_fpstate = fsp;
		} else if (p == fplwp) {
			/* Drop the live context on the floor. */
			savefpstate(fsp);
			reload = 1;
		}
		/* Note: sizeof fpr->__fpu_fr <= sizeof fsp->fs_regs. */
		memcpy(fsp->fs_regs, fpr->__fpu_fr, sizeof (fpr->__fpu_fr));
		fsp->fs_fsr = fpr->__fpu_fsr;	/* don't care about fcc1-3 */
		fsp->fs_qsize = 0;

#if 0
		/* Need more info! */
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
#endif

		/* Reload context again, if necessary. */
		if (reload)
			loadfpstate(fsp);
	}

	/* XXX mcp->__xrs */
	/* XXX mcp->__asrs */
#endif
	return (0);
}

#if NFIRM_EVENTS > 0
/*
 * Write out a series of 32-bit firm_events.
 */
int
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

/*
 * ioctl code
 */

#include <dev/sun/fbio.h>
#include <machine/openpromio.h>

/* from arch/sparc/include/fbio.h */
#if 0
/* unused */
#define	FBIOGINFO	_IOR('F', 2, struct fbinfo)
#endif

struct netbsd32_fbcmap {
	int	index;		/* first element (0 origin) */
	int	count;		/* number of elements */
	netbsd32_u_charp	red;		/* red color map elements */
	netbsd32_u_charp	green;		/* green color map elements */
	netbsd32_u_charp	blue;		/* blue color map elements */
};
#if 1
#define	FBIOPUTCMAP32	_IOW('F', 3, struct netbsd32_fbcmap)
#define	FBIOGETCMAP32	_IOW('F', 4, struct netbsd32_fbcmap)
#endif

struct netbsd32_fbcursor {
	short set;		/* what to set */
	short enable;		/* enable/disable cursor */
	struct fbcurpos pos;	/* cursor's position */
	struct fbcurpos hot;	/* cursor's hot spot */
	struct netbsd32_fbcmap cmap;	/* color map info */
	struct fbcurpos size;	/* cursor's bit map size */
	netbsd32_charp image;	/* cursor's image bits */
	netbsd32_charp mask;	/* cursor's mask bits */
};
#if 1
#define FBIOSCURSOR32	_IOW('F', 24, struct netbsd32_fbcursor)
#define FBIOGCURSOR32	_IOWR('F', 25, struct netbsd32_fbcursor)
#endif

/* from arch/sparc/include/openpromio.h */
struct netbsd32_opiocdesc {
	int	op_nodeid;		/* passed or returned node id */
	int	op_namelen;		/* length of op_name */
	netbsd32_charp op_name;		/* pointer to field name */
	int	op_buflen;		/* length of op_buf (value-result) */
	netbsd32_charp op_buf;		/* pointer to field value */
};
#if 1
#define	OPIOCGET32	_IOWR('O', 1, struct netbsd32_opiocdesc) /* get openprom field */
#define	OPIOCSET32	_IOW('O', 2, struct netbsd32_opiocdesc) /* set openprom field */
#define	OPIOCNEXTPROP32	_IOWR('O', 3, struct netbsd32_opiocdesc) /* get next property */
#endif

/* prototypes for the converters */
static inline void netbsd32_to_fbcmap(struct netbsd32_fbcmap *,
					struct fbcmap *, u_long);
static inline void netbsd32_to_fbcursor(struct netbsd32_fbcursor *,
					  struct fbcursor *, u_long);
static inline void netbsd32_to_opiocdesc(struct netbsd32_opiocdesc *,
					   struct opiocdesc *, u_long);

static inline void netbsd32_from_fbcmap(struct fbcmap *,
					  struct netbsd32_fbcmap *, u_long);
static inline void netbsd32_from_fbcursor(struct fbcursor *,
					    struct netbsd32_fbcursor *, u_long);
static inline void netbsd32_from_opiocdesc(struct opiocdesc *,
					     struct netbsd32_opiocdesc *,
					     u_long);

/* convert to/from different structures */
static inline void
netbsd32_to_fbcmap(struct netbsd32_fbcmap *s32p, struct fbcmap *p, u_long cmd)
{

	p->index = s32p->index;
	p->count = s32p->count;
	p->red = NETBSD32PTR64(s32p->red);
	p->green = NETBSD32PTR64(s32p->green);
	p->blue = NETBSD32PTR64(s32p->blue);
}

static inline void
netbsd32_to_fbcursor(struct netbsd32_fbcursor *s32p, struct fbcursor *p, u_long cmd)
{

	p->set = s32p->set;
	p->enable = s32p->enable;
	p->pos = s32p->pos;
	p->hot = s32p->hot;
	netbsd32_to_fbcmap(&s32p->cmap, &p->cmap, cmd);
	p->size = s32p->size;
	p->image = NETBSD32PTR64(s32p->image);
	p->mask = NETBSD32PTR64(s32p->mask);
}

static inline void
netbsd32_to_opiocdesc(struct netbsd32_opiocdesc *s32p, struct opiocdesc *p, u_long cmd)
{

	p->op_nodeid = s32p->op_nodeid;
	p->op_namelen = s32p->op_namelen;
	p->op_name = NETBSD32PTR64(s32p->op_name);
	p->op_buflen = s32p->op_buflen;
	p->op_buf = NETBSD32PTR64(s32p->op_buf);
}

static inline void
netbsd32_from_fbcmap(struct fbcmap *p, struct netbsd32_fbcmap *s32p, u_long cmd)
{

	s32p->index = p->index;
	s32p->count = p->count;
/* filled in */
#if 0
	s32p->red = (netbsd32_u_charp)p->red;
	s32p->green = (netbsd32_u_charp)p->green;
	s32p->blue = (netbsd32_u_charp)p->blue;
#endif
}

static inline void
netbsd32_from_fbcursor(struct fbcursor *p, struct netbsd32_fbcursor *s32p, u_long cmd)
{

	s32p->set = p->set;
	s32p->enable = p->enable;
	s32p->pos = p->pos;
	s32p->hot = p->hot;
	netbsd32_from_fbcmap(&p->cmap, &s32p->cmap, cmd);
	s32p->size = p->size;
/* filled in */
#if 0
	s32p->image = (netbsd32_charp)p->image;
	s32p->mask = (netbsd32_charp)p->mask;
#endif
}

static inline void
netbsd32_from_opiocdesc(struct opiocdesc *p, struct netbsd32_opiocdesc *s32p, u_long cmd)
{

	s32p->op_nodeid = p->op_nodeid;
	s32p->op_namelen = p->op_namelen;
	NETBSD32PTR32(s32p->op_name, p->op_name);
	s32p->op_buflen = p->op_buflen;
	NETBSD32PTR32(s32p->op_buf, p->op_buf);
}

int
netbsd32_md_ioctl(struct file *fp, netbsd32_u_long cmd, void *data32, struct lwp *l)
{
	u_int size;
	void *data, *memp = NULL;
#define STK_PARAMS	128
	u_long stkbuf[STK_PARAMS/sizeof(u_long)];
	int error;

	switch (cmd) {
	case FBIOPUTCMAP32:
		IOCTL_STRUCT_CONV_TO(FBIOPUTCMAP, fbcmap);
	case FBIOGETCMAP32:
		IOCTL_STRUCT_CONV_TO(FBIOGETCMAP, fbcmap);

	case FBIOSCURSOR32:
		IOCTL_STRUCT_CONV_TO(FBIOSCURSOR, fbcursor);
	case FBIOGCURSOR32:
		IOCTL_STRUCT_CONV_TO(FBIOGCURSOR, fbcursor);

	case OPIOCGET32:
		IOCTL_STRUCT_CONV_TO(OPIOCGET, opiocdesc);
	case OPIOCSET32:
		IOCTL_STRUCT_CONV_TO(OPIOCSET, opiocdesc);
	case OPIOCNEXTPROP32:
		IOCTL_STRUCT_CONV_TO(OPIOCNEXTPROP, opiocdesc);
	default:
		error = (*fp->f_ops->fo_ioctl)(fp, cmd, data32, l);
	}
	if (memp)
		free(memp, M_IOCTLOPS);
	return (error);
}


int
netbsd32_sysarch(struct lwp *l, const struct netbsd32_sysarch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */

	switch (SCARG(uap, op)) {
	default:
		printf("(%s) netbsd32_sysarch(%d)\n", MACHINE, SCARG(uap, op));
		return EINVAL;
	}
}


int
cpu_setmcontext32(struct lwp *l, const mcontext32_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_tf;
	const __greg32_t *gr = mcp->__gregs;
	struct proc *p = l->l_proc;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(&p->p_smutex);
		sigexit(l, SIGILL);
	}

	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
		/*
	 	 * Only the icc bits in the psr are used, so it need not be
	 	 * verified.  pc and npc must be multiples of 4.  This is all
	 	 * that is required; if it holds, just do it.
		 */
		if (((gr[_REG32_PC] | gr[_REG32_nPC]) & 3) != 0 ||
		    gr[_REG32_PC] == 0 || gr[_REG32_nPC] == 0)
			return (EINVAL);

		/* Restore general register context. */
		/* take only tstate CCR (and ASI) fields */
		tf->tf_tstate = (tf->tf_tstate & ~TSTATE_CCR) |
		    PSRCC_TO_TSTATE(gr[_REG32_PSR]);
		tf->tf_pc        = (uint64_t)gr[_REG32_PC];
		tf->tf_npc       = (uint64_t)gr[_REG32_nPC];
		tf->tf_y         = (uint64_t)gr[_REG32_Y];
		tf->tf_global[1] = (uint64_t)gr[_REG32_G1];
		tf->tf_global[2] = (uint64_t)gr[_REG32_G2];
		tf->tf_global[3] = (uint64_t)gr[_REG32_G3];
		tf->tf_global[4] = (uint64_t)gr[_REG32_G4];
		tf->tf_global[5] = (uint64_t)gr[_REG32_G5];
		tf->tf_global[6] = (uint64_t)gr[_REG32_G6];
		tf->tf_global[7] = (uint64_t)gr[_REG32_G7];
		tf->tf_out[0]    = (uint64_t)gr[_REG32_O0];
		tf->tf_out[1]    = (uint64_t)gr[_REG32_O1];
		tf->tf_out[2]    = (uint64_t)gr[_REG32_O2];
		tf->tf_out[3]    = (uint64_t)gr[_REG32_O3];
		tf->tf_out[4]    = (uint64_t)gr[_REG32_O4];
		tf->tf_out[5]    = (uint64_t)gr[_REG32_O5];
		tf->tf_out[6]    = (uint64_t)gr[_REG32_O6];
		tf->tf_out[7]    = (uint64_t)gr[_REG32_O7];
		/* %asi restored above; %fprs not yet supported. */

		/* XXX mcp->__gwins */
	}

	/* Restore floating point register context, if any. */
	if ((flags & _UC_FPU) != 0) {
#ifdef notyet
		struct fpstate64 *fsp;
		const __fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If we're the current FPU owner, simply reload it from
		 * the supplied context.  Otherwise, store it into the
		 * process' FPU save area (which is used to restore from
		 * by lazy FPU context switching); allocate it if necessary.
		 */
		if ((fsp = l->l_md.md_fpstate) == NULL) {
			fsp = malloc(sizeof (*fsp), M_SUBPROC, M_WAITOK);
			l->l_md.md_fpstate = fsp;
		} else if (l == fplwp) {
			/* Drop the live context on the floor. */
			savefpstate(fsp);
		}
		/* Note: sizeof fpr->__fpu_fr <= sizeof fsp->fs_regs. */
		memcpy(fsp->fs_regs, &fpr->__fpu_fr, sizeof (fpr->__fpu_fr));
		fsp->fs_fsr = mcp->__fpregs.__fpu_fsr;
		fsp->fs_qsize = 0;

#if 0
		/* Need more info! */
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
#endif
#endif
	}
#ifdef _UC_SETSTACK
	mutex_enter(&p->p_smutex);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(&p->p_smutex);
#endif
	return (0);
}


void
cpu_getmcontext32(struct lwp *l, mcontext32_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_md.md_tf;
	__greg32_t *gr = mcp->__gregs;

	/* First ensure consistent stack state (see sendsig). */ /* XXX? */
	write_user_windows();
	if (rwindow_save(l)) {
		mutex_enter(&l->l_proc->p_smutex);
		sigexit(l, SIGILL);
	}

	/* For now: Erase any random indicators for optional state. */
	(void)memset(mcp, '0', sizeof (*mcp));

	/* Save general register context. */
	gr[_REG32_PSR] = TSTATECCR_TO_PSR(tf->tf_tstate);
	gr[_REG32_PC]  = tf->tf_pc;
	gr[_REG32_nPC] = tf->tf_npc;
	gr[_REG32_Y]   = tf->tf_y;
	gr[_REG32_G1]  = tf->tf_global[1];
	gr[_REG32_G2]  = tf->tf_global[2];
	gr[_REG32_G3]  = tf->tf_global[3];
	gr[_REG32_G4]  = tf->tf_global[4];
	gr[_REG32_G5]  = tf->tf_global[5];
	gr[_REG32_G6]  = tf->tf_global[6];
	gr[_REG32_G7]  = tf->tf_global[7];
	gr[_REG32_O0]  = tf->tf_out[0];
	gr[_REG32_O1]  = tf->tf_out[1];
	gr[_REG32_O2]  = tf->tf_out[2];
	gr[_REG32_O3]  = tf->tf_out[3];
	gr[_REG32_O4]  = tf->tf_out[4];
	gr[_REG32_O5]  = tf->tf_out[5];
	gr[_REG32_O6]  = tf->tf_out[6];
	gr[_REG32_O7]  = tf->tf_out[7];
	*flags |= _UC_CPU;

	mcp->__gwins = 0;
	mcp->__xrs.__xrs_id = 0;	/* Solaris extension? */
	*flags |= _UC_CPU;

	/* Save FP register context, if any. */
	if (l->l_md.md_fpstate != NULL) {
#ifdef notyet
		struct fpstate64 fs, *fsp;
		__fpregset_t *fpr = &mcp->__fpregs;

		/*
		 * If our FP context is currently held in the FPU, take a
		 * private snapshot - lazy FPU context switching can deal
		 * with it later when it becomes necessary.
		 * Otherwise, get it from the process's save area.
		 */
		if (l == fplwp) {
			fsp = &fs;
			savefpstate(fsp);
		} else {
			fsp = l->l_md.md_fpstate;
		}
		memcpy(&fpr->__fpu_fr, fsp->fs_regs, sizeof (fpr->__fpu_fr));
		mcp->__fpregs.__fpu_q = NULL;	/* `Need more info.' */
		mcp->__fpregs.__fpu_fsr = fs.fs_fsr;
		mcp->__fpregs.__fpu_qcnt = 0 /*fs.fs_qsize*/; /* See above */
		mcp->__fpregs.__fpu_q_entrysize =
		    (unsigned char) sizeof (*mcp->__fpregs.__fpu_q);
		mcp->__fpregs.__fpu_en = 1;
		*flags |= _UC_FPU;
#endif
	} else {
		mcp->__fpregs.__fpu_en = 0;
	}
}

void
startlwp32(void *arg)
{
	int err;
	ucontext32_t *uc = arg;
	struct lwp *l = curlwp;

	err = cpu_setmcontext32(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	KERNEL_UNLOCK_LAST(l);
	userret(l, 0, 0);
}

vaddr_t
netbsd32_vm_default_addr(struct proc *p, vaddr_t base, vsize_t size)
{
	return round_page((vaddr_t)(base) + (vsize_t)MAXDSIZ32);
}
