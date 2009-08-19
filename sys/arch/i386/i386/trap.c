/*	$NetBSD: trap.c,v 1.235.4.3 2009/08/19 18:46:19 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2005, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)trap.c	7.4 (Berkeley) 5/13/91
 */

/*
 * 386 Trap and System call handling
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.235.4.3 2009/08/19 18:46:19 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_vm86.h"
#include "opt_kvm86.h"
#include "opt_kstack_dr0.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/ras.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/cpu.h>
#include <sys/ucontext.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>
#ifdef DDB
#include <machine/db_machdep.h>
#endif

#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>
#endif

#include <x86/nmi.h>

#include "isa.h"

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "npx.h"

static inline int xmm_si_code(struct lwp *);
void trap(struct trapframe *);
void trap_tss(struct i386tss *, int, int);

#ifdef KVM86
#include <machine/kvm86.h>
#define KVM86MODE (kvm86_incall)
#else
#define KVM86MODE (0)
#endif

const char * const trap_type[] = {
	"privileged instruction fault",		/*  0 T_PRIVINFLT */
	"breakpoint trap",			/*  1 T_BPTFLT */
	"arithmetic trap",			/*  2 T_ARITHTRAP */
	"asynchronous system trap",		/*  3 T_ASTFLT */
	"protection fault",			/*  4 T_PROTFLT */
	"trace trap",				/*  5 T_TRCTRAP */
	"page fault",				/*  6 T_PAGEFLT */
	"alignment fault",			/*  7 T_ALIGNFLT */
	"integer divide fault",			/*  8 T_DIVIDE */
	"non-maskable interrupt",		/*  9 T_NMI */
	"overflow trap",			/* 10 T_OFLOW */
	"bounds check fault",			/* 11 T_BOUND */
	"FPU not available fault",		/* 12 T_DNA */
	"double fault",				/* 13 T_DOUBLEFLT */
	"FPU operand fetch fault",		/* 14 T_FPOPFLT */
	"invalid TSS fault",			/* 15 T_TSSFLT */
	"segment not present fault",		/* 16 T_SEGNPFLT */
	"stack fault",				/* 17 T_STKFLT */
	"machine check fault",			/* 18 T_MCA */
	"SSE FP exception",			/* 19 T_XMM */
	"reserved trap",			/* 20 T_RESERVED */
};
int	trap_types = __arraycount(trap_type);

#ifdef DEBUG
int	trapdebug = 0;
#endif

#define	IDTVEC(name)	__CONCAT(X, name)

void
trap_tss(struct i386tss *tss, int trapno, int code)
{
	struct trapframe tf;

	tf.tf_gs = tss->tss_gs;
	tf.tf_fs = tss->tss_fs;
	tf.tf_es = tss->__tss_es;
	tf.tf_ds = tss->__tss_ds;
	tf.tf_edi = tss->__tss_edi;
	tf.tf_esi = tss->__tss_esi;
	tf.tf_ebp = tss->tss_ebp;
	tf.tf_ebx = tss->__tss_ebx;
	tf.tf_edx = tss->__tss_edx;
	tf.tf_ecx = tss->__tss_ecx;
	tf.tf_eax = tss->__tss_eax;
	tf.tf_trapno = trapno;
	tf.tf_err = code | TC_TSS;
	tf.tf_eip = tss->__tss_eip;
	tf.tf_cs = tss->__tss_cs;
	tf.tf_eflags = tss->__tss_eflags;
	tf.tf_esp = tss->tss_esp;
	tf.tf_ss = tss->__tss_ss;
	trap(&tf);
}

static inline int
xmm_si_code(struct lwp *l)
{
	uint32_t mxcsr, mask;

	if (!i386_use_fxsave) {
#ifdef DIAGNOSTIC
		panic("SSE FP Exception, but no SSE");
#endif
		return 0;
	}
	mxcsr = l->l_addr->u_pcb.pcb_savefpu.sv_xmm.sv_env.en_mxcsr;

	/*
         * Since we only have a single status and control register,
	 * we use the exception mask bits to mask disabled exceptions
	 */
	mask = ~((mxcsr & __INITIAL_MXCSR__) >> 7) & 0xff;
        switch (mask & mxcsr) {
	case EN_SW_INVOP:
		return FPE_FLTINV;
	case EN_SW_DENORM:
	case EN_SW_PRECLOSS:
		return FPE_FLTRES;
	case EN_SW_ZERODIV:
		return FPE_FLTDIV;
	case EN_SW_OVERFLOW:
		return FPE_FLTOVF;
	case EN_SW_UNDERFLOW:
		return FPE_FLTUND;
	case EN_SW_DATACHAIN:
		return FPE_FLTSUB;
	case 0:
	default:
		return 0;
	}
}

static void *
onfault_handler(const struct pcb *pcb, const struct trapframe *tf)
{
	struct onfault_table {
		uintptr_t start;
		uintptr_t end;
		void *handler;
	};
	extern const struct onfault_table onfault_table[];
	const struct onfault_table *p;
	uintptr_t pc;

	if (pcb->pcb_onfault != NULL) {
		return pcb->pcb_onfault;
	}

	pc = tf->tf_eip;
	for (p = onfault_table; p->start; p++) {
		if (p->start <= pc && pc < p->end) {
			return p->handler;
		}
	}
	return NULL;
}

/*
 * trap(frame): exception, fault, and trap interface to BSD kernel.
 *
 * This common code is called from assembly language IDT gate entry routines
 * that prepare a suitable stack frame, and restore this frame after the
 * exception has been processed. Note that the effect is as if the arguments
 * were passed call by reference.
 */
void
trap(struct trapframe *frame)
{
	struct lwp *l = curlwp;
	struct proc *p;
	struct pcb *pcb;
	extern char fusubail[], kcopy_fault[], trapreturn[], IDTVEC(osyscall)[];
	struct trapframe *vframe;
	ksiginfo_t ksi;
	void *onfault;
	int type, error;
	uint32_t cr2;
	bool pfail;

	if (__predict_true(l != NULL)) {
		pcb = &l->l_addr->u_pcb;
		p = l->l_proc;
	} else {
		/*
		 * this can happen eg. on break points in early on boot.
		 */
		pcb = NULL;
		p = NULL;
	}
	type = frame->tf_trapno;

#ifdef DEBUG
	if (trapdebug) {
		printf("trap %d code %x eip %x cs %x eflags %x cr2 %lx cpl %x\n",
		    type, frame->tf_err, frame->tf_eip, frame->tf_cs,
		    frame->tf_eflags, rcr2(), curcpu()->ci_ilevel);
		printf("curlwp %p%s", curlwp, curlwp ? " " : "\n");
		if (curlwp)
			printf("pid %d lid %d\n", l->l_proc->p_pid, l->l_lid);
	}
#endif
	if (type != T_NMI && !KVM86MODE &&
	    !KERNELMODE(frame->tf_cs, frame->tf_eflags)) {
		type |= T_USER;
		l->l_md.md_regs = frame;
		pcb->pcb_cr2 = 0;
		LWP_CACHE_CREDS(l, p);
	}

	switch (type) {

	case T_ASTFLT:
		if (KVM86MODE) {
			break;
		}
		/*FALLTHROUGH*/

	default:
	we_re_toast:
#ifdef KSTACK_CHECK_DR0
		if (type == T_TRCTRAP) {
			u_int mask, dr6 = rdr6();

			mask = 1 << 0; /* dr0 */
			if (dr6 & mask) {
				panic("trap on DR0: maybe kernel stack overflow\n");
#if 0
				dr6 &= ~mask;
				ldr6(dr6);
				return;
#endif
			}
		}
#endif
		if (frame->tf_trapno < trap_types)
			printf("fatal %s", trap_type[frame->tf_trapno]);
		else
			printf("unknown trap %d", frame->tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %d code %x eip %x cs %x eflags %x cr2 %lx ilevel %x\n",
		    type, frame->tf_err, frame->tf_eip, frame->tf_cs,
		    frame->tf_eflags, (long)rcr2(), curcpu()->ci_ilevel);
#ifdef DDB
		if (kdb_trap(type, 0, frame))
			return;
#endif
#ifdef KGDB
		if (kgdb_trap(type, frame))
			return;
		else {
			/*
			 * If this is a breakpoint, don't panic
			 * if we're not connected.
			 */
			if (type == T_BPTFLT) {
				printf("kgdb: ignored %s\n", trap_type[type]);
				return;
			}
		}
#endif
		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:
#ifdef KVM86
		if (KVM86MODE) {
			kvm86_gpfault(frame);
			return;
		}
#endif
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_TSSFLT:
		if (p == NULL)
			goto we_re_toast;
		/* Check for copyin/copyout fault. */
		onfault = onfault_handler(pcb, frame);
		if (onfault != NULL) {
copyefault:
			error = EFAULT;
copyfault:
			frame->tf_eip = (uintptr_t)onfault;
			frame->tf_eax = error;
			return;
		}

		/*
		 * Check for failure during return to user mode.
		 *
		 * We do this by looking at the instruction we faulted on.
		 * The specific instructions we recognize only happen when
		 * returning from a trap, syscall, or interrupt.
		 *
		 * At this point, there are (at least) two trap frames on
		 * the kernel stack; we presume here that we faulted while
		 * loading our registers out of the outer one.
		 */
		switch (*(u_char *)frame->tf_eip) {
		case 0xcf:	/* iret */
			vframe = (void *)((int)&frame->tf_esp -
			    offsetof(struct trapframe, tf_eip));
			break;
		case 0x8e:
			switch (*(uint32_t *)frame->tf_eip) {
			case 0x0c245c8e:	/* movl 0xc(%esp,1),%ds */
			case 0x0824448e:	/* movl 0x8(%esp,1),%es */
			case 0x0424648e:	/* movl 0x4(%esp,1),%fs */
			case 0x00246c8e:	/* movl 0x0(%esp,1),%gs */
				break;
			default:
				goto we_re_toast;
			}
			vframe = (void *)(int)&frame->tf_esp;
			break;
		default:
			goto we_re_toast;
		}
		if (KERNELMODE(vframe->tf_cs, vframe->tf_eflags))
			goto we_re_toast;

		/*
		 * Arrange to signal the thread, which will reset its
		 * registers in the outer frame.  This also allows us to
		 * capture the invalid register state in sigcontext,
		 * packaged up with the signal delivery.  We restart
		 * on return at 'trapreturn', acting as if nothing
		 * happened, restarting the return to user with our new
		 * set of registers.
		 *
		 * Clear PSL_NT.  It can be set by userland because setting
		 * it isn't a privileged operation.
		 *
		 * Set PSL_I.  Otherwise, if SIGSEGV is ignored, we'll
		 * continue to generate traps infinitely with
		 * interrupts disabled.
		 */
		frame->tf_ds = GSEL(GDATA_SEL, SEL_KPL);
		frame->tf_es = GSEL(GDATA_SEL, SEL_KPL);
		frame->tf_gs = GSEL(GDATA_SEL, SEL_KPL);
		frame->tf_fs = GSEL(GCPU_SEL, SEL_KPL);
		frame->tf_eip = (uintptr_t)trapreturn;
		frame->tf_eflags = (frame->tf_eflags & ~PSL_NT) | PSL_I;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_addr = (void *)rcr2();
		ksi.ksi_code = SEGV_ACCERR;
		ksi.ksi_trap = type & ~T_USER;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		return;

	case T_PROTFLT|T_USER:		/* protection fault */
	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_ALIGNFLT|T_USER:
		KSI_INIT_TRAP(&ksi);

		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_SEGNPFLT|T_USER:
		case T_STKFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case T_TSSFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_OBJERR;
			break;
		case T_ALIGNFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRALN;
			break;
		case T_PROTFLT|T_USER:
#ifdef VM86
			if (frame->tf_eflags & PSL_VM) {
				vm86_gpfault(l, type & ~T_USER);
				goto out;
			}
#endif
			/*
			 * If pmap_exec_fixup does something,
			 * let's retry the trap.
			 */
			if (pmap_exec_fixup(&p->p_vmspace->vm_map, frame,
			    &l->l_addr->u_pcb)) {
				goto out;
			}
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			break;
		default:
			KASSERT(0);
			break;
		}
		goto trapsignal;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_PRIVINFLT|T_USER:
			ksi.ksi_code = ILL_PRVOPC;
			break;
		case T_FPOPFLT|T_USER:
			ksi.ksi_code = ILL_COPROC;
			break;
		default:
			ksi.ksi_code = 0;
			break;
		}
		goto trapsignal;

	case T_ASTFLT|T_USER:
		/* Allow process switch. */
		uvmexp.softs++;
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
		/* Allow a forced task switch. */
		if (curcpu()->ci_want_resched) {
			preempt();
		}
		goto out;

	case T_DNA|T_USER: {
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGKILL;
		ksi.ksi_addr = (void *)frame->tf_eip;
		printf("pid %d killed due to lack of floating point\n",
		    p->p_pid);
		goto trapsignal;
	}

	case T_XMM|T_USER:
	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
	case T_ARITHTRAP|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_addr = (void *)frame->tf_eip;
		switch (type) {
		case T_XMM|T_USER:
			ksi.ksi_code = xmm_si_code(l);
			break;
		case T_BOUND|T_USER:
		case T_OFLOW|T_USER:
			ksi.ksi_code = FPE_FLTOVF;
			break;
		case T_DIVIDE|T_USER:
			ksi.ksi_code = FPE_FLTDIV;
			break;
		case T_ARITHTRAP|T_USER:
			ksi.ksi_code = FPE_INTOVF;
			break;
		default:
			ksi.ksi_code = 0;
			break;
		}
		goto trapsignal;

	case T_PAGEFLT:
		/* Allow page faults in kernel mode. */
		if (__predict_false(l == NULL))
			goto we_re_toast;

		/*
		 * fusubail is used by [fs]uswintr() to prevent page faulting
		 * from inside the profiling interrupt.
		 */
		onfault = pcb->pcb_onfault;
		if (onfault == fusubail) {
			goto copyefault;
		}
		if (cpu_intr_p() || (l->l_pflag & LP_INTR) != 0) {
			goto we_re_toast;
		}

		cr2 = rcr2();
		goto faultcommon;

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register struct vm_map *map;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

		cr2 = rcr2();
		if (l->l_flag & LW_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)cr2;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}
faultcommon:
		vm = p->p_vmspace;
		if (__predict_false(vm == NULL)) {
			goto we_re_toast;
		}
		pcb->pcb_cr2 = cr2;
		va = trunc_page((vaddr_t)cr2);
		/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= KERNBASE)
			map = kernel_map;
		else
			map = &vm->vm_map;
		if (frame->tf_err & PGEX_W)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %lx\n", va);
			goto we_re_toast;
		}
#endif
		/* Fault the original page in. */
		onfault = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		error = uvm_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;
		if (error == 0) {
			if (map != kernel_map && (void *)va >= vm->vm_maxsaddr)
				uvm_grow(p, va);

			pfail = false;
			while (type == T_PAGEFLT) {
				/*
				 * we need to switch pmap now if we're in
				 * the middle of copyin/out.
				 *
				 * but we don't need to do so for kcopy as
				 * it never touch userspace.
 				 */
				kpreempt_disable();
				if (curcpu()->ci_want_pmapload) {
					onfault = onfault_handler(pcb, frame);
					if (onfault != kcopy_fault) {
						pmap_load();
					}
				}
				/*
				 * We need to keep the pmap loaded and
				 * so avoid being preempted until back
				 * into the copy functions.  Disable
				 * interrupts at the hardware level before
				 * re-enabling preemption.  Interrupts
				 * will be re-enabled by 'iret' when
				 * returning back out of the trap stub.
				 * They'll only be re-enabled when the
				 * program counter is once again in
				 * the copy functions, and so visible
				 * to cpu_kpreempt_exit().
				 */
#ifndef XEN
				x86_disable_intr();
#endif
				l->l_nopreempt--;
				if (l->l_nopreempt > 0 || !l->l_dopreempt ||
				    pfail) {
					return;
				}
#ifndef XEN
				x86_enable_intr();
#endif
				/*
				 * If preemption fails for some reason,
				 * don't retry it.  The conditions won't
				 * change under our nose.
				 */
				pfail = kpreempt(0);
			}
			l->l_pflag &= ~LP_SA_PAGEFAULT;
			goto out;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)cr2;
		if (error == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			error = EFAULT;
		} else {
			ksi.ksi_code = SEGV_MAPERR;
		}

		if (type == T_PAGEFLT) {
			onfault = onfault_handler(pcb, frame);
			if (onfault != NULL)
				goto copyfault;
			printf("uvm_fault(%p, %#lx, %d) -> %#x\n",
			    map, va, ftype, error);
			goto we_re_toast;
		}
		if (error == ENOMEM) {
			ksi.ksi_signo = SIGKILL;
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       l->l_cred ?
			       kauth_cred_geteuid(l->l_cred) : -1);
		} else {
			ksi.ksi_signo = SIGSEGV;
		}
		(*p->p_emul->e_trapsignal)(l, &ksi);
		l->l_pflag &= ~LP_SA_PAGEFAULT;
		break;
	}

	case T_TRCTRAP:
		/* Check whether they single-stepped into a lcall. */
		if (frame->tf_eip == (int)IDTVEC(osyscall))
			return;
		if (frame->tf_eip == (int)IDTVEC(osyscall) + 1) {
			frame->tf_eflags &= ~PSL_T;
			return;
		}
		goto we_re_toast;

	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
		/*
		 * Don't go single-stepping into a RAS.
		 */
		if (p->p_raslist == NULL ||
		    (ras_lookup(p, (void *)frame->tf_eip) == (void *)-1)) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_trap = type & ~T_USER;
			if (type == (T_BPTFLT|T_USER))
				ksi.ksi_code = TRAP_BRKPT;
			else
				ksi.ksi_code = TRAP_TRACE;
			ksi.ksi_addr = (void *)frame->tf_eip;
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
		break;

	case T_NMI:
#if !defined(XEN)
		if (nmi_dispatch(frame))
			return;
#if (NISA > 0 || NMCA > 0)
#if defined(KGDB) || defined(DDB)
		/* NMI can be hooked up to a pushbutton for debugging */
		printf ("NMI ... going to debugger\n");
#ifdef KGDB

		if (kgdb_trap(type, frame))
			return;
#endif
#ifdef DDB
		if (kdb_trap(type, 0, frame))
			return;
#endif
#endif /* KGDB || DDB */
		/* machine/parity/power fail/"kitchen sink" faults */

#if NMCA > 0
		/* mca_nmi() takes care to call x86_nmi() if appropriate */
		if (mca_nmi() != 0)
			goto we_re_toast;
		else
			return;
#else /* NISA > 0 */
		if (x86_nmi() != 0)
			goto we_re_toast;
		else
			return;
#endif /* NMCA > 0 */
#endif /* (NISA > 0 || NMCA > 0) */
#endif /* !defined(XEN) */
		;	/* avoid a label at end of compound statement */
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(l);
	return;
trapsignal:
	ksi.ksi_trap = type & ~T_USER;
	(*p->p_emul->e_trapsignal)(l, &ksi);
	userret(l);
}

/* 
 * startlwp: start of a new LWP.
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	lwp_t *l = curlwp;
	int error;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);
	pool_put(&lwp_uc_pool, uc);
	userret(l);
}

/*
 * XXX_SA: This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	KERNEL_UNLOCK_LAST(l);
	userret(l);
}
