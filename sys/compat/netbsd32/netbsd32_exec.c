/*	$NetBSD: netbsd32_exec.c,v 1.4 1998/08/30 15:32:19 eeh Exp $	*/
/*	from: NetBSD: exec_aout.c,v 1.15 1996/09/26 23:34:46 cgd Exp */

/*
 * Copyright (c) 1998 Matthew R. Green.
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
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
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <vm/vm.h>

#include <compat/sparc32/sparc32.h>
#include <compat/sparc32/sparc32_exec.h>
#include <compat/sparc32/sparc32_syscall.h>

#include <machine/frame.h>

extern struct sysent sparc32_sysent[];
#ifdef SYSCALL_DEBUG
extern char *sparc32_syscallnames[];
#endif
extern char sigcode[], esigcode[];
const char sparc32_emul_path[] = "/emul/sparc32";
void sparc32_sendsig __P((sig_t, int, int, u_long));
void sparc32_setregs __P((struct proc *, struct exec_package *, u_long));
static int sparc32_exec_aout_prep_zmagic __P((struct proc *, struct exec_package *));
static int sparc32_exec_aout_prep_nmagic __P((struct proc *, struct exec_package *));
static int sparc32_exec_aout_prep_omagic __P((struct proc *, struct exec_package *));

struct emul emul_sparc32 = {
	"sparc32",
	NULL,
	sparc32_sendsig,	/* XXX needs to be written */
	sparc32_SYS_syscall,
	sparc32_SYS_MAXSYSCALL,
	sparc32_sysent,
#ifdef SYSCALL_DEBUG
	sparc32_syscallnames,
#else
	NULL,
#endif
	0,
	copyargs,
	sparc32_setregs,	/* XXX needs to be written?? */
	sigcode,
	esigcode,
};

/*
 * exec_sparc32_makecmds(): Check if it's an sparc32 a.out format
 * executable.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in sparc32 a.out format.  Check 'standard' magic
 * numbers for this architecture.
 *
 * This function, in the former case, or the hook, in the latter, is
 * responsible for creating a set of vmcmds which can be used to build
 * the process's vm space and inserting them into the exec package.
 */

int
exec_sparc32_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	sparc32_u_long midmag, magic;
	u_short mid;
	int error;
	struct sparc32_exec *execp = epp->ep_hdr;

	if (epp->ep_hdrvalid < sizeof(struct sparc32_exec))
		return ENOEXEC;

	midmag = (sparc32_u_long)ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0x3ff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_MACHINE << 16) | ZMAGIC:
		error = sparc32_exec_aout_prep_zmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | NMAGIC:
		error = sparc32_exec_aout_prep_nmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | OMAGIC:
		error = sparc32_exec_aout_prep_omagic(p, epp);
		break;
	default:
		/* Invalid magic */
		error = EINVAL;
		break;
	}

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * sparc32_exec_aout_prep_zmagic(): Prepare a 'native' ZMAGIC binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
sparc32_exec_aout_prep_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, 0, VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * sparc32_exec_aout_prep_nmagic(): Prepare a 'native' NMAGIC binary's exec package
 */

int
sparc32_exec_aout_prep_nmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = roundup(epp->ep_taddr + execp->a_text, __LDPGSZ);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text + sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * sparc32_exec_aout_prep_omagic(): Prepare a 'native' OMAGIC binary's exec package
 */

int
sparc32_exec_aout_prep_omagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;
	long dsize, bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->a_text + execp->a_data, epp->ep_taddr, epp->ep_vp,
	    sizeof(struct exec), VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page. 
	 */
	dsize = epp->ep_dsize + execp->a_text - roundup(execp->a_text, NBPG);
	epp->ep_dsize = (dsize > 0) ? dsize : 0;
	return exec_aout_setup_stack(p, epp);
}

/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */
/*
 * the rest is pretty much verbatum from sys/arch/sparc/sparc64/machdep.c
 */
/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */

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
struct sparc32_sigframe {
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
sparc32_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigacts *psp = p->p_sigacts;
	register struct sparc32_sigframe *fp;
	register struct trapframe *tf;
	register int addr, oonstack; 
	struct rwindow32 *kwin, *oldsp, *newsp;
	struct sparc32_sigframe sf;
	extern char sigcode[], esigcode[];
#define	szsigcode	(esigcode - sigcode)

	tf = p->p_md.md_tf;
	/* Need to attempt to zero extend this 32-bit pointer */
	oldsp = (struct rwindow32 *)(u_long)(u_int)tf->tf_out[6];
	oonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;
	/*
	 * Compute new user stack addresses, subtract off
	 * one signal frame, and align.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
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
	sf.sf_sc.sc_onstack = oonstack;
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
	    suword(&oldsp->rw_in[0], tf->tf_in[0]) || suword(&oldsp->rw_in[1], tf->tf_in[1]) ||
	    suword(&oldsp->rw_in[2], tf->tf_in[2]) || suword(&oldsp->rw_in[3], tf->tf_in[3]) ||
	    suword(&oldsp->rw_in[4], tf->tf_in[4]) || suword(&oldsp->rw_in[5], tf->tf_in[5]) ||
	    suword(&oldsp->rw_in[6], tf->tf_in[6]) || suword(&oldsp->rw_in[7], tf->tf_in[7]) ||
	    suword(&oldsp->rw_local[0], (int)tf->tf_local[0]) || suword(&oldsp->rw_local[1], (int)tf->tf_local[1]) ||
	    suword(&oldsp->rw_local[2], (int)tf->tf_local[2]) || suword(&oldsp->rw_local[3], (int)tf->tf_local[3]) ||
	    suword(&oldsp->rw_local[4], (int)tf->tf_local[4]) || suword(&oldsp->rw_local[5], (int)tf->tf_local[5]) ||
	    suword(&oldsp->rw_local[6], (int)tf->tf_local[6]) || suword(&oldsp->rw_local[7], (int)tf->tf_local[7]) ||
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
#ifdef COMPAT_SUNOS
	if (psp->ps_usertramp & sigmask(sig)) {
		addr = (long)catcher;	/* user does his own trampolining */
	} else
#endif
	{
		addr = (long)PS_STRINGS - szsigcode;
		tf->tf_global[1] = (long)catcher;
	}
	tf->tf_pc = addr;
	tf->tf_npc = addr + 4;
	tf->tf_out[6] = (u_int64_t)(u_int)(u_long)newsp;
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid) {
		printf("sendsig: about to return to catcher %p thru %p\n", 
		       catcher, addr);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
}

