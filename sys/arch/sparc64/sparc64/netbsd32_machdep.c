/*	$NetBSD: netbsd32_machdep.c,v 1.3.2.3 2001/03/12 13:29:34 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/map.h>

#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/netbsd32_machdep.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
netbsd32_setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack; /* XXX */
{
	register struct trapframe64 *tf = p->p_md.md_tf;
	register struct fpstate64 *fs;
	register int64_t tstate;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/* Mark this as a 32-bit emulation */
	p->p_flag |= P_32;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: address of PS_STRINGS (used by crt0)
	 *	%tpc,%tnpc: entry point of program
	 */
	tstate = ((PSTATE_USER32)<<TSTATE_PSTATE_SHIFT) 
		| (tf->tf_tstate & TSTATE_CWP);
	if ((fs = p->p_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		if (p == fpproc) {
			savefpstate(fs);
			fpproc = NULL;
		}
		free((void *)fs, M_SUBPROC);
		p->p_md.md_fpstate = NULL;
	}
	bzero((caddr_t)tf, sizeof *tf);
	tf->tf_tstate = tstate;
	tf->tf_global[1] = (u_int)(u_long)p->p_psstr;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;

	stack -= sizeof(struct rwindow32);
	tf->tf_out[6] = stack;
	tf->tf_out[7] = NULL;
}

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

void
netbsd32_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct sparc32_sigframe *fp;
	register struct trapframe64 *tf;
	register int addr, onstack; 
	struct rwindow32 *kwin, *oldsp, *newsp;
	struct sparc32_sigframe sf;
	extern char netbsd32_sigcode[], netbsd32_esigcode[];
#define	szsigcode	(netbsd32_esigcode - netbsd32_sigcode)

	tf = p->p_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (onstack) {
		fp = (struct sparc32_sigframe *)((char *)p->p_sigctx.ps_sigstk.ss_sp +
					p->p_sigctx.ps_sigstk.ss_size);
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
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
	sf.sf_code = (u_int)code;
#ifdef COMPAT_SUNOS
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
	sf.sf_sc.sc_tstate = TSTATECCR_TO_PSR(tf->tf_tstate); /* XXX */
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
	newsp = (struct rwindow32 *)((long)fp - sizeof(struct rwindow32));
	write_user_windows();
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK))
	    printf("sendsig: saving sf to %p, setting stack pointer %p to %p\n",
		   fp, &(((struct rwindow32 *)newsp)->rw_in[6]), oldsp);
#endif
	kwin = (struct rwindow32 *)(((caddr_t)tf)-CCFSZ);
	if (rwindow_save(p) || 
	    copyout((caddr_t)&sf, (caddr_t)fp, sizeof sf) || 
	    suword(&(((struct rwindow32 *)newsp)->rw_in[6]), (u_long)oldsp)) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig: window save or copyout error\n");
		printf("sendsig: stack was trashed trying to send sig %d, sending SIGILL\n", sig);
		if (sigdebug & SDB_DDB) Debugger();
#endif
		sigexit(p, SIGILL);
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
	tf->tf_out[6] = (u_int64_t)(u_int)(u_long)newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sendsig: about to return to catcher %p thru %p\n", 
		       catcher, addr);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
}

#undef DEBUG

#ifdef COMPAT_13
int
compat_13_netbsd32_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_netbsd32_sigreturn_args /* {
		syscallarg(struct netbsd32_sigcontext13 *) sigcntxp;
	} */ *uap = v;
	struct netbsd32_sigcontext13 *scp;
	struct netbsd32_sigcontext13 sc;
	register struct trapframe64 *tf;
	struct rwindow32 *rwstack, *kstack;
	sigset_t mask;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p)) {
#ifdef DEBUG
		printf("compat_13_netbsd32_sigreturn: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		sigexit(p, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_netbsd32_sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct netbsd32_sigcontext13 *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((caddr_t)scp, &sc, sizeof sc) != 0))
#ifdef DEBUG
	{
		printf("compat_13_netbsd32_sigreturn: copyin failed\n");
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	tf = p->p_md.md_tf;
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
	rwstack = (struct rwindow32 *)(u_long)tf->tf_out[6];
	kstack = (struct rwindow32 *)(((caddr_t)tf)-CCFSZ);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("compat_13_netbsd32_sys_sigreturn: return trapframe pc=%p sp=%p tstate=%x\n",
		       (int)tf->tf_pc, (int)tf->tf_out[6], (int)tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	native_sigset13_to_sigset(&scp->sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);
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
netbsd32___sigreturn14(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct netbsd32_sigcontext sc, *scp;
	register struct trapframe64 *tf;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p)) {
#ifdef DEBUG
		printf("netbsd32_sigreturn14: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		sigexit(p, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("netbsd32_sigreturn14: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct netbsd32_sigcontext *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((caddr_t)scp, &sc, sizeof sc) != 0))
#ifdef DEBUG
	{
		printf("netbsd32_sigreturn14: copyin failed: scp=%p\n", scp);
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	scp = &sc;

	tf = p->p_md.md_tf;
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
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | (scp->sc_tstate & TSTATE_CCR);
	tf->tf_pc = (int64_t)scp->sc_pc;
	tf->tf_npc = (int64_t)scp->sc_npc;
	tf->tf_global[1] = (int64_t)scp->sc_g1;
	tf->tf_out[0] = (int64_t)scp->sc_o0;
	tf->tf_out[6] = (int64_t)scp->sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("netbsd32_sigreturn14: return trapframe pc=%p sp=%p tstate=%llx\n",
		       (vaddr_t)tf->tf_pc, (vaddr_t)tf->tf_out[6], tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif

	/* Restore signal stack. */
	if (sc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &sc.sc_mask, 0);

	return (EJUSTRETURN);
}

#if 0
/* Unfortunately we need to convert v9 trapframe to v8 regs */
int
netbsd32_process_read_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct reg32* regp = (struct reg32*)regs;
	struct trapframe64* tf = p->p_md.md_tf;
	int i;

	/* 
	 * Um, we should only do this conversion for 32-bit emulation
	 * or when running 32-bit mode.  We really need to pass in a
	 * 32-bit emulation flag!
	 */

	regp->r_psr = TSTATECCR_TO_PSR(tf->tf_tstate);
	regp->r_pc = tf->tf_pc;
	regp->r_npc = tf->tf_npc;
	regp->r_y = tf->tf_y;
	for (i = 0; i < 8; i++) {
		regp->r_global[i] = tf->tf_global[i];
		regp->r_out[i] = tf->tf_out[i];
	}
	/* We should also write out the ins and locals.  See signal stuff */
	return (0);
}

int
netbsd32_process_write_regs(p, regs)
	struct proc *p;
	struct reg *regs;
{
	struct reg32* regp = (struct reg32*)regs;
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

int
netbsd32_process_read_fpregs(p, regs)
struct proc	*p;
struct fpreg	*regs;
{
	extern struct fpstate64	initfpstate;
	struct fpstate64	*statep = &initfpstate;
	struct fpreg32		*regp = (struct fpreg32 *)regs;
	int i;

	/* NOTE: struct fpreg == struct fpstate */
	if (p->p_md.md_fpstate)
		statep = p->p_md.md_fpstate;
	for (i=0; i<32; i++)
		regp->fr_regs[i] = statep->fs_regs[i];
	regp->fr_fsr = statep->fs_fsr;
	regp->fr_qsize = statep->fs_qsize;
	for (i=0; i<statep->fs_qsize; i++)
		regp->fr_queue[i] = statep->fs_queue[i];

	return 0;
}

int
netbsd32_process_write_fpregs(p, regs)
struct proc	*p;
struct fpreg	*regs;
{
	extern struct fpstate	initfpstate;
	struct fpstate64	*statep = &initfpstate;
	struct fpreg32		*regp = (struct fpreg32 *)regs;
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
cpu_coredump32(p, vp, cred, chdr)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
	struct core32 *chdr;
{
	int i, error;
	struct md_coredump32 md_core;
	struct coreseg32 cseg;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN(sizeof(cseg));
	chdr->c_cpusize = sizeof(md_core);

	/* Fake a v8 trapframe */
	md_core.md_tf.tf_psr = TSTATECCR_TO_PSR(p->p_md.md_tf->tf_tstate);
	md_core.md_tf.tf_pc = p->p_md.md_tf->tf_pc;
	md_core.md_tf.tf_npc = p->p_md.md_tf->tf_npc;
	md_core.md_tf.tf_y = p->p_md.md_tf->tf_y;
	for (i=0; i<8; i++) {
		md_core.md_tf.tf_global[i] = p->p_md.md_tf->tf_global[i];
		md_core.md_tf.tf_out[i] = p->p_md.md_tf->tf_out[i];
	}

	if (p->p_md.md_fpstate) {
		if (p == fpproc)
			savefpstate(p->p_md.md_fpstate);
		/* Copy individual fields */
		for (i=0; i<32; i++)
			md_core.md_fpstate.fs_regs[i] = 
				p->p_md.md_fpstate->fs_regs[i];
		md_core.md_fpstate.fs_fsr = p->p_md.md_fpstate->fs_fsr;
		i = md_core.md_fpstate.fs_qsize = p->p_md.md_fpstate->fs_qsize;
		/* Should always be zero */
		while (i--)
			md_core.md_fpstate.fs_queue[i] = 
				p->p_md.md_fpstate->fs_queue[i];
	} else
		bzero((caddr_t)&md_core.md_fpstate, 
		      sizeof(md_core.md_fpstate));

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (!error)
		chdr->c_nseg++;

	return error;
}
