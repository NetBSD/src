/*	$NetBSD: ibcs2_machdep.c,v 1.13.4.3 2001/08/24 00:08:29 nathanw Exp $	*/

/*-
 * Copyright (c) 1997, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <sys/signalvar.h>
#include <sys/signal.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/npx.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/ibcs2_machdep.h>

#ifdef VM86
#include <machine/vm86.h>
#endif

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_sysi86.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>

void
ibcs2_setregs(l, epp, stack)
	struct lwp *l;
	struct exec_package *epp;
	u_long stack;
{
	register struct pcb *pcb = &l->l_addr->u_pcb;
	register struct trapframe *tf;

	setregs(l, epp, stack);
	if (i386_use_fxsave)
		pcb->pcb_savefpu.sv_xmm.sv_env.en_cw = __iBCS2_NPXCW__;
	else
		pcb->pcb_savefpu.sv_87.sv_env.en_cw = __iBCS2_NPXCW__;
	tf = l->p_md.md_regs;
	tf->tf_eax = 0x2000000;		/* XXX base of heap */
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
ibcs2_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	/* XXX Need SCO sigframe format. */
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	int onstack;

	tf = l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
		    p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_esp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = native_to_ibcs2_sig[sig];
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		frame.sf_sc.sc_eflags = get_vflags(l);
		(*p->p_emul->e_syscall_intern)(p);
	} else
#endif
	{
		frame.sf_sc.sc_gs = tf->tf_gs;
		frame.sf_sc.sc_fs = tf->tf_fs;
		frame.sf_sc.sc_es = tf->tf_es;
		frame.sf_sc.sc_ds = tf->tf_ds;
		frame.sf_sc.sc_eflags = tf->tf_eflags;
	}
	frame.sf_sc.sc_edi = tf->tf_edi;
	frame.sf_sc.sc_esi = tf->tf_esi;
	frame.sf_sc.sc_ebp = tf->tf_ebp;
	frame.sf_sc.sc_ebx = tf->tf_ebx;
	frame.sf_sc.sc_edx = tf->tf_edx;
	frame.sf_sc.sc_ecx = tf->tf_ecx;
	frame.sf_sc.sc_eax = tf->tf_eax;
	frame.sf_sc.sc_eip = tf->tf_eip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp = tf->tf_esp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
	frame.sf_sc.sc_err = tf->tf_err;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);	
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)p->p_sigctx.ps_sigcode;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

int
ibcs2_sys_sysmachine(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_sysmachine_args /* {
		syscallarg(int) cmd;
		syscallarg(int) arg;
	} */ *uap = v;
	int val, error;

	switch (SCARG(uap, cmd)) {
	case IBCS2_SI86FPHW:
		val = IBCS2_FP_NO;
#ifdef MATH_EMULATE
		val = IBCS2_FP_SW;
#else
		val = IBCS2_FP_387;		/* a real coprocessor */
#endif
		if ((error = copyout((caddr_t)&val, (caddr_t)SCARG(uap, arg),
				     sizeof(val))))
			return error;
		break;

	case IBCS2_SI86STIME:		/* XXX - not used much, if at all */
	case IBCS2_SI86SETNAME:
		return EINVAL;

	case IBCS2_SI86PHYSMEM:
                *retval = ctob(physmem);
		break;

	case IBCS2_SI86GETFEATURES:	/* XXX structure def? */
		break;

	default:
		return EINVAL;
	}
	return 0;
}
