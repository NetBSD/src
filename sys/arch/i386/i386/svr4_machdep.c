/*	$NetBSD: svr4_machdep.c,v 1.83.2.1 2007/05/27 12:27:32 ad Exp $	 */

/*-
 * Copyright (c) 1994, 2000 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_machdep.c,v 1.83.2.1 2007/05/27 12:27:32 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/vm86.h>
#include <machine/vmparam.h>
#include <machine/svr4_machdep.h>

static void svr4_getsiginfo(union svr4_siginfo *, int, u_long, void *);
void svr4_fasttrap(struct trapframe);

#ifdef DEBUG_SVR4
static void svr4_printmcontext(const char *, svr4_mcontext_t *);


static void
svr4_printmcontext(fun, mc)
	const char *fun;
	svr4_mcontext_t *mc;
{
	svr4_greg_t *r = mc->greg;

	uprintf("%s at %p\n", fun, mc);

	uprintf("Regs: ");
	uprintf("GS = 0x%x ", r[SVR4_X86_GS]);
	uprintf("FS = 0x%x ",  r[SVR4_X86_FS]);
	uprintf("ES = 0x%x ", r[SVR4_X86_ES]);
	uprintf("DS = 0x%x ",   r[SVR4_X86_DS]);
	uprintf("EDI = 0x%x ",  r[SVR4_X86_EDI]);
	uprintf("ESI = 0x%x ",  r[SVR4_X86_ESI]);
	uprintf("EBP = 0x%x ",  r[SVR4_X86_EBP]);
	uprintf("ESP = 0x%x ",  r[SVR4_X86_ESP]);
	uprintf("EBX = 0x%x ",  r[SVR4_X86_EBX]);
	uprintf("EDX = 0x%x ",  r[SVR4_X86_EDX]);
	uprintf("ECX = 0x%x ",  r[SVR4_X86_ECX]);
	uprintf("EAX = 0x%x ",  r[SVR4_X86_EAX]);
	uprintf("TRAPNO = 0x%x ",  r[SVR4_X86_TRAPNO]);
	uprintf("ERR = 0x%x ",  r[SVR4_X86_ERR]);
	uprintf("EIP = 0x%x ",  r[SVR4_X86_EIP]);
	uprintf("CS = 0x%x ",  r[SVR4_X86_CS]);
	uprintf("EFL = 0x%x ",  r[SVR4_X86_EFL]);
	uprintf("UESP = 0x%x ",  r[SVR4_X86_UESP]);
	uprintf("SS = 0x%x ",  r[SVR4_X86_SS]);
	uprintf("\n");
}
#endif

void
svr4_setregs(l, epp, stack)
	struct lwp *l;
	struct exec_package *epp;
	u_long stack;
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf = l->l_md.md_regs;

	setregs(l, epp, stack);
	if (i386_use_fxsave)
		pcb->pcb_savefpu.sv_xmm.sv_env.en_cw = __SVR4_NPXCW__;
	else
		pcb->pcb_savefpu.sv_87.sv_env.en_cw = __SVR4_NPXCW__;
	tf->tf_cs = GSEL(LUCODEBIG_SEL, SEL_UPL);
}

void *
svr4_getmcontext(l, mc, flags)
	struct lwp *l;
	svr4_mcontext_t *mc;
	u_long *flags;
{
	struct trapframe *tf = l->l_md.md_regs;
	svr4_greg_t *r = mc->greg;

	/* Save context. */
	tf = l->l_md.md_regs;
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		r[SVR4_X86_GS] = tf->tf_vm86_gs;
		r[SVR4_X86_FS] = tf->tf_vm86_fs;
		r[SVR4_X86_ES] = tf->tf_vm86_es;
		r[SVR4_X86_DS] = tf->tf_vm86_ds;
		r[SVR4_X86_EFL] = get_vflags(l);
	} else
#endif
	{
		r[SVR4_X86_GS] = tf->tf_gs;
		r[SVR4_X86_FS] = tf->tf_fs;
		r[SVR4_X86_ES] = tf->tf_es;
		r[SVR4_X86_DS] = tf->tf_ds;
		r[SVR4_X86_EFL] = tf->tf_eflags;
	}
	r[SVR4_X86_EDI] = tf->tf_edi;
	r[SVR4_X86_ESI] = tf->tf_esi;
	r[SVR4_X86_EBP] = tf->tf_ebp;
	r[SVR4_X86_ESP] = tf->tf_esp;
	r[SVR4_X86_EBX] = tf->tf_ebx;
	r[SVR4_X86_EDX] = tf->tf_edx;
	r[SVR4_X86_ECX] = tf->tf_ecx;
	r[SVR4_X86_EAX] = tf->tf_eax;
	r[SVR4_X86_TRAPNO] = tf->tf_trapno;
	r[SVR4_X86_ERR] = tf->tf_err;
	r[SVR4_X86_EIP] = tf->tf_eip;
	r[SVR4_X86_CS] = tf->tf_cs;
	r[SVR4_X86_UESP] = tf->tf_esp;
	r[SVR4_X86_SS] = tf->tf_ss;

	*flags |= SVR4_UC_CPU;
#ifdef DEBUG_SVR4
	svr4_printmcontext("getmcontext", mc);
#endif  
	return (void *) tf->tf_esp;

}


/*
 * Set to mcontext specified.
 * has been taken. 
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
svr4_setmcontext(l, mc, flags)
	struct lwp *l;
	svr4_mcontext_t *mc;
	u_long flags;
{
	struct trapframe *tf;
	svr4_greg_t *r = mc->greg;
#ifdef VM86
	struct proc *p = l->l_proc;
#endif

#ifdef DEBUG_SVR4
	svr4_printmcontext("setmcontext", mc);
#endif  
	/*
	 * XXX: What to do with floating point stuff?
	 */
	if ((flags & SVR4_UC_CPU) == 0)
		return 0;

	/* Restore context. */
	tf = l->l_md.md_regs;
#ifdef VM86
	if (r[SVR4_X86_EFL] & PSL_VM) {
		void syscall_vm86(struct trapframe *);

		tf->tf_vm86_gs = r[SVR4_X86_GS];
		tf->tf_vm86_fs = r[SVR4_X86_FS];
		tf->tf_vm86_es = r[SVR4_X86_ES];
		tf->tf_vm86_ds = r[SVR4_X86_DS];
		set_vflags(l, r[SVR4_X86_EFL]);
		p->p_md.md_syscall = syscall_vm86;
	} else
#endif
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((r[SVR4_X86_EFL] ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(r[SVR4_X86_CS], r[SVR4_X86_EFL]))
			return (EINVAL);

		tf->tf_fs = r[SVR4_X86_FS];
		tf->tf_gs = r[SVR4_X86_GS];
		tf->tf_es = r[SVR4_X86_ES];
		tf->tf_ds = r[SVR4_X86_DS];
#ifdef VM86
		if (tf->tf_eflags & PSL_VM)
			(*p->p_emul->e_syscall_intern)(p);
#endif
		tf->tf_eflags = r[SVR4_X86_EFL];
	}
	tf->tf_edi = r[SVR4_X86_EDI];
	tf->tf_esi = r[SVR4_X86_ESI];
	tf->tf_ebp = r[SVR4_X86_EBP];
	tf->tf_ebx = r[SVR4_X86_EBX];
	tf->tf_edx = r[SVR4_X86_EDX];
	tf->tf_ecx = r[SVR4_X86_ECX];
	tf->tf_eax = r[SVR4_X86_EAX];
	tf->tf_eip = r[SVR4_X86_EIP];
	tf->tf_cs = r[SVR4_X86_CS];
	tf->tf_ss = r[SVR4_X86_SS];
	tf->tf_esp = r[SVR4_X86_UESP];

	return 0;
}


static void
svr4_getsiginfo(si, sig, code, addr)
	union svr4_siginfo	*si;
	int			 sig;
	u_long			 code;
	void *			 addr;
{
	si->si_signo = native_to_svr4_signo[sig];
	si->si_errno = 0;
	si->si_addr  = addr;

	switch (code) {
	case T_PRIVINFLT:
		si->si_code = SVR4_ILL_PRVOPC;
		si->si_trap = SVR4_T_PRIVINFLT;
		break;

	case T_BPTFLT:
		si->si_code = SVR4_TRAP_BRKPT;
		si->si_trap = SVR4_T_BPTFLT;
		break;

	case T_ARITHTRAP:
		si->si_code = SVR4_FPE_INTOVF;
		si->si_trap = SVR4_T_DIVIDE;
		break;

	case T_PROTFLT:
		si->si_code = SVR4_SEGV_ACCERR;
		si->si_trap = SVR4_T_PROTFLT;
		break;

	case T_TRCTRAP:
		si->si_code = SVR4_TRAP_TRACE;
		si->si_trap = SVR4_T_TRCTRAP;
		break;

	case T_PAGEFLT:
		si->si_code = SVR4_SEGV_ACCERR;
		si->si_trap = SVR4_T_PAGEFLT;
		break;

	case T_ALIGNFLT:
		si->si_code = SVR4_BUS_ADRALN;
		si->si_trap = SVR4_T_ALIGNFLT;
		break;

	case T_DIVIDE:
		si->si_code = SVR4_FPE_FLTDIV;
		si->si_trap = SVR4_T_DIVIDE;
		break;

	case T_OFLOW:
		si->si_code = SVR4_FPE_FLTOVF;
		si->si_trap = SVR4_T_DIVIDE;
		break;

	case T_BOUND:
		si->si_code = SVR4_FPE_FLTSUB;
		si->si_trap = SVR4_T_BOUND;
		break;

	case T_DNA:
		si->si_code = SVR4_FPE_FLTINV;
		si->si_trap = SVR4_T_DNA;
		break;

	case T_FPOPFLT:
		si->si_code = SVR4_FPE_FLTINV;
		si->si_trap = SVR4_T_FPOPFLT;
		break;

	case T_SEGNPFLT:
		si->si_code = SVR4_SEGV_MAPERR;
		si->si_trap = SVR4_T_SEGNPFLT;
		break;

	case T_STKFLT:
		si->si_code = SVR4_ILL_BADSTK;
		si->si_trap = SVR4_T_STKFLT;
		break;

	default:
		si->si_code = 0;
		si->si_trap = 0;
#ifdef DIAGNOSTIC
		printf("sig %d code %ld\n", sig, code);
		panic("svr4_getsiginfo");
#endif
		break;
	}
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
svr4_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	u_long code = KSI_TRAPCODE(ksi);
	int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int onstack, error;
	struct svr4_sigframe *fp = getframe(l, sig, &onstack), frame;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigaltstack *sas = &l->l_sigstk;
	struct trapframe *tf = l->l_md.md_regs;

	fp--;

	/* 
	 * Build the argument list for the signal handler.
	 * Notes:
	 * 	- we always build the whole argument list, even when we
	 *	  don't need to [when SA_SIGINFO is not set, we don't need
	 *	  to pass all sf_si and sf_uc]
	 *	- we don't pass the correct signal address [we need to
	 *	  modify many kernel files to enable that]
	 */
	svr4_getcontext(l, &frame.sf_uc);
	svr4_getsiginfo(&frame.sf_si, sig, code, (void *) tf->tf_eip);

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = frame.sf_si.si_signo;
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_handler = catcher;

#ifdef DEBUG_SVR4
	printf("sig = %d, sip %p, ucp = %p, handler = %p\n", 
	       frame.sf_signum, frame.sf_sip, frame.sf_ucp, frame.sf_handler);
#endif

	sendsig_reset(l, sig);

	mutex_exit(&p->p_smutex);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, GUCODEBIG_SEL, p->p_sigctx.ps_sigcode, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		sas->ss_flags |= SS_ONSTACK;
}

/*
 * sysi86
 */
int
svr4_sys_sysarch(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_sys_sysarch_args *uap = v;
#ifdef USER_LDT
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	int error;
#endif
	*retval = 0;	/* XXX: What to do */

	switch (SCARG(uap, op)) {
	case SVR4_SYSARCH_FPHW:
		return 0;

	case SVR4_SYSARCH_DSCR:
#ifdef USER_LDT
		{
			struct x86_set_ldt_args sa, *sap;
			struct sys_sysarch_args ua;

			struct svr4_ssd ssd;
			union descriptor bsd;

			if ((error = copyin(SCARG(uap, a1), &ssd,
					    sizeof(ssd))) != 0) {
				printf("Cannot copy arg1\n");
				return error;
			}

#ifdef DEBUG
			printf("s=%x, b=%x, l=%x, a1=%x a2=%x\n",
			       ssd.selector, ssd.base, ssd.limit,
			       ssd.access1, ssd.access2);
#endif

			/* We can only set ldt's for now. */
			if (!ISLDT(ssd.selector)) {
#ifdef DEBUG
				printf("Not an ldt\n");
#endif
				return EPERM;
			}

			/* Oh, well we don't cleanup either */
			if (ssd.access1 == 0)
				return 0;

			bsd.sd.sd_lobase = ssd.base & 0xffffff;
			bsd.sd.sd_hibase = (ssd.base >> 24) & 0xff;

			bsd.sd.sd_lolimit = ssd.limit & 0xffff;
			bsd.sd.sd_hilimit = (ssd.limit >> 16) & 0xf;

			bsd.sd.sd_type = ssd.access1 & 0x1f;
			bsd.sd.sd_dpl =  (ssd.access1 >> 5) & 0x3;
			bsd.sd.sd_p = (ssd.access1 >> 7) & 0x1;

			bsd.sd.sd_xx = ssd.access2 & 0x3;
			bsd.sd.sd_def32 = (ssd.access2 >> 2) & 0x1;
			bsd.sd.sd_gran = (ssd.access2 >> 3)& 0x1;

			sa.start = IDXSEL(ssd.selector);
			sa.desc = stackgap_alloc(p, &sg,
			    sizeof(union descriptor));
			sa.num = 1;
			sap = stackgap_alloc(p, &sg,
			     sizeof(struct x86_set_ldt_args));

			if ((error = copyout(&sa, sap, sizeof(sa))) != 0)
				return error;

			SCARG(&ua, op) = X86_SET_LDT;
			SCARG(&ua, parms) = (char *) sap;

			if ((error = copyout(&bsd, sa.desc, sizeof(bsd))) != 0)
				return error;

			return sys_sysarch(l, &ua, retval);
		}
#endif

	default:
		printf("svr4_sysarch(%d), a1 %p\n", SCARG(uap, op),
		       SCARG(uap, a1));
		return 0;
	}
}

/*
 * Fast syscall gate trap...
 */
void
svr4_fasttrap(frame)
	struct trapframe frame;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct schedstate_percpu *spc;
	struct timeval tv;
	struct timespec ts;
	uint64_t tm;

	l->l_md.md_regs = &frame;

	if (p->p_emul != &emul_svr4) {
		ksiginfo_t ksi;
		memset(&ksi, 0, sizeof(ksi));
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLTRP;
		trapsignal(l, &ksi);
		return;
	}

	switch (frame.tf_eax) {
	case SVR4_TRAP_GETHRTIME:
		/*
		 * This is like gethrtime(3), returning the time expressed
		 * in nanoseconds since an arbitrary time in the past and
		 * guaranteed to be monotonically increasing, which we
		 * obtain from nanouptime(9).
		 */
		nanouptime(&ts);

		tm = ts.tv_nsec + ts.tv_sec * (uint64_t)1000000000u;
		/* XXX: dsl - I would have expected the msb in %edx */
		frame.tf_edx = tm & 0xffffffffu;
		frame.tf_eax = (tm >> 32);
		break;

	case SVR4_TRAP_GETHRVTIME:
		/*
		 * This is like gethrvtime(3). returning the LWP's virtual
		 * time expressed in nanoseconds. It is supposedly
		 * guaranteed to be monotonically increasing, but for now
		 * using the LWP's real time augmented with its current
		 * runtime is the best we can do.
		 */
		spc = &curcpu()->ci_schedstate;

		microtime(&tv);

		tm = (l->l_rtime.tv_sec + tv.tv_sec -
		    spc->spc_runtime.tv_sec) * 1000000ull;
		tm += l->l_rtime.tv_usec + tv.tv_usec;
		tm -= spc->spc_runtime.tv_usec;
		tm *= 1000u;
		/* XXX: dsl - I would have expected the msb in %edx */
		frame.tf_edx = tm & 0xffffffffu;
		frame.tf_eax = (tm >> 32);
		break;

	case SVR4_TRAP_GETHRESTIME:
		/*
		 * This is like clock_gettime(CLOCK_REALTIME, tp), returning
		 * proc's wall time. Seconds are returned in %eax, nanoseconds
		 * in %edx.
		 */
		nanotime(&ts);

		frame.tf_eax = (uint32_t) ts.tv_sec;
		frame.tf_edx = (uint32_t) ts.tv_nsec;
		break;

	default:
		uprintf("unknown svr4 fast trap %d\n",
		    frame.tf_eax);
		break;
	}
}
