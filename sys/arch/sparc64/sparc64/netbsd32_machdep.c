/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
sparc32_setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack; /* XXX */
{
	register struct trapframe *tf = p->p_md.md_tf;
	register struct fpstate *fs;
	register int64_t tstate;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%tstate: (retain icc and xcc and cwp bits)
	 *	%g1: address of PS_STRINGS (used by crt0)
	 *	%tpc,%tnpc: entry point of program
	 */
	tstate = ((PSTATE_USER)<<TSTATE_PSTATE_SHIFT) 
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
	tf->tf_global[1] = (int)PS_STRINGS;
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
struct netbsd32_sigframe {
	int	sf_signo;		/* signal number */
	int	sf_code;		/* code */
	u_int	sf_scp;			/* SunOS user addr of sigcontext */
	int	sf_addr;		/* SunOS compat, always 0 for now */
	struct	sparc32_sigcontext sf_sc;	/* actual sigcontext */
};

#undef DEBUG
#ifdef DEBUG
extern int sigdebug;
#endif

void
netbsd32_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigacts *psp = p->p_sigacts;
	register struct sparc32_sigframe *fp;
	register struct trapframe *tf;
	register int addr, onstack; 
	struct rwindow32 *kwin, *oldsp, *newsp;
	struct sparc32_sigframe sf;
	extern char sigcode[], esigcode[];
#define	szsigcode	(esigcode - sigcode)

	tf = p->p_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	/* Do we need to jump onto the signal stack? */
	onstack =
	    (psp->ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (psp->ps_sigact[sig].sa_flags & SA_ONSTACK) != 0;
	if (onstack) {
		fp = (struct sparc32_sigframe *)(psp->ps_sigstk.ss_sp +
					 psp->ps_sigstk.ss_size);
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sparc32_sigframe *)oldsp;
	fp = (struct sparc32_sigframe *)((long)(fp - 1) & ~7);

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
	sf.sf_code = code;
#ifdef COMPAT_SUNOS
	sf.sf_scp = (u_long)&fp->sf_sc;
#endif
	sf.sf_addr = 0;			/* XXX */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	sf.sf_sc.sc_onstack = onstack;
	sf.sf_sc.sc_mask = mask;
	sf.sf_sc.sc_sp = (long)oldsp;
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
	newsp = (struct rwindow32 *)((long)fp - sizeof(struct rwindow32));
	write_user_windows();
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK))
	    printf("sendsig: saving sf to %p, setting stack pointer %p to %p\n",
		   fp, &(((union rwindow *)newsp)->v8.rw_in[6]), oldsp);
#endif
	kwin = (struct rwindow32 *)(((caddr_t)tf)-CCFSZ);
	if (rwindow_save(p) || 
	    copyout((caddr_t)&sf, (caddr_t)fp, sizeof sf) || 
	    suword(&(((union rwindow *)newsp)->v8.rw_in[6]), (u_long)oldsp)) {
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
	addr = (long)PS_STRINGS - szsigcode;
	tf->tf_global[1] = (long)catcher;
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (u_int64_t)(u_int)(u_long)newsp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sendsig: about to return to catcher %p thru %p\n", 
		       catcher, addr);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
}

#undef DEBUG
int
compat_sparc32_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sigreturn_args /* {
		syscallarg(struct sparc32_sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sparc32_sigcontext *scp;
	struct sparc32_sigcontext sc;
	register struct trapframe *tf;
	struct rwindow32 *rwstack, *kstack;
	sigset_t mask;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p)) {
#ifdef DEBUG
		printf("sigreturn: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		sigexit(p, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct sparc32_sigcontext *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((caddr_t)scp, &sc, sizeof sc) != 0))
#ifdef DEBUG
	{
		printf("sigreturn: copyin failed\n");
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
		printf("sigreturn: pc %p or npc %p invalid\n", sc.sc_pc, sc.sc_npc);
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
	rwstack = (struct rwindow32 *)tf->tf_out[6];
	kstack = (struct rwindow32 *)(((caddr_t)tf)-CCFSZ);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sys_sigreturn: return trapframe pc=%p sp=%p tstate=%x\n",
		       (int)tf->tf_pc, (int)tf->tf_out[6], (int)tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask */
	native_sigset13_to_sigset(&scp->sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);
	return (EJUSTRETURN);
}
