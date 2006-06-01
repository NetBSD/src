/*	$NetBSD: trap.c,v 1.70.6.2 2006/06/01 22:35:09 kardel Exp $	*/

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

/*-
 * Copyright (c) 1996 Matthias Pfaller. All rights reserved.
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * 532 Trap handling
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.70.6.2 2006/06/01 22:35:09 kardel Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ktrace.h"
#include "opt_ns381.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/pool.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/kauth.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/syscall.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/fpu.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>
#ifdef DDB
#include <machine/db_machdep.h>
#endif

struct lwp *fpu_lwp;		/* LWP owning the FPU. */

/* Allow turning off if ieee_handler for debugging */
int ieee_handler_disable = 0;

#if defined(MRTD)
#define __CDECL__ __attribute__ ((cdecl))
#else
#define __CDECL__
#endif

void trap(struct trapframe) __CDECL__;

const char *trap_type[] = {
	"non-vectored interrupt",		/*  0 T_NVI */
	"non-maskable interrupt",		/*  1 T_NMI */
	"abort trap",				/*  2 T_ABT */
	"coprocessor trap",			/*  3 T_SLAVE */
	"illegal operation in user mode",	/*  4 T_ILL */
	"supervisor call",			/*  5 T_SVC */
	"divide by zero",			/*  6 T_DVZ */
	"flag instruction",			/*  7 T_FLG */
	"breakpoint instruction",		/*  8 T_BPT */
	"trace trap",				/*  9 T_TRC */
	"undefined instruction",		/* 10 T_UND */
	"restartable bus error",		/* 11 T_RBE */
	"non-restartable bus error",		/* 12 T_NBE */
	"integer overflow trap",		/* 13 T_OVF */
	"debug trap",				/* 14 T_DBG */
	"reserved trap",			/* 15 T_RESERVED */
	"unused",				/* 16 unused */
	"watchpoint",				/* 17 T_WATCHPOINT */
	"asynchronous system trap"		/* 18 T_AST */
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

#ifdef DEBUG
int	trapdebug = 0;
#endif

/*
 * trap(frame):
 *	Exception, fault, and trap interface to BSD kernel. This
 * common code is called from assembly language trap vector
 * routines that prepare a suitable stack frame, and restore this
 * frame after the exception has been processed. Note that the
 * effect is as if the arguments were passed call by reference.
 */
/*ARGSUSED*/
void
trap(struct trapframe frame)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int type = frame.tf_trapno;
	u_quad_t sticks;
	struct pcb *pcb = NULL;
	extern char fubail[], subail[];
#ifdef CINVSMALL
	extern char cinvstart[], cinvend[];
#endif
	ksiginfo_t ksi;

	uvmexp.traps++;

#ifdef DEBUG
	if (trapdebug) {
		printf("trap type=%d, pc=0x%x, tear=0x%x, msr=0x%x\n",
			type, frame.tf_regs.r_pc,
			frame.tf_tear, frame.tf_msr);
		printf("curproc %p\n", curproc);
	}
#endif

	if (USERMODE(frame.tf_regs.r_psr)) {
		type |= T_USER;
		sticks = p->p_sticks;
		l->l_md.md_regs = &frame.tf_regs;
	} else
		sticks = 0;

	switch (type) {

	default:
	we_re_toast:
#if defined(DDB) || defined(KGDB)
	{
		static struct trapframe *db_frame = (struct trapframe *)
			(VM_MIN_KERNEL_ADDRESS + 0x1000 - sizeof(frame));
		static int usp, r;
		register int ret __asm("r3");
		register int s __asm("r4") = splhigh();
		register int sp __asm("r5");

		r = 0;
		ret = ((int *)&frame)[-1];
		*++db_frame = frame;
		usp = db_frame->tf_regs.r_sp;
		db_frame->tf_regs.r_sp = ((int)&frame) + sizeof(frame);
		sprd(sp, sp);
		if (sp >= (VM_MIN_KERNEL_ADDRESS + 0x1000))
			lprd(sp, VM_MIN_KERNEL_ADDRESS + 0x1000 - 4);
		else
			lprd(sp, sp - 64);
# ifdef KGDB
		r = kgdb_trap(type, db_frame);
# endif
# ifdef DDB
		if (r == 0) {
			db_active_ipl = s;
			r = kdb_trap(type, 0, db_frame);
		}
# endif
		splx(s);
		if (r) {
			sp = db_frame->tf_regs.r_sp - sizeof(frame);
			db_frame->tf_regs.r_sp = usp;
			*(struct trapframe *)sp = *db_frame--;
			lprd(fp, &((struct trapframe *)sp)->tf_regs.r_fp);
			lprd(sp, sp);
			__asm volatile("jump 0(%0)" : : "g" (ret));
		}
		db_frame--;
		lprd(sp, sp); /* Frame should be intact */
	}
#endif
		if (frame.tf_trapno < trap_types)
			printf("fatal %s", trap_type[frame.tf_trapno]);
		else
			printf("unknown trap %ld", frame.tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type=%d, pc=0x%x, tear=0x%lx, msr=0x%lx\n",
			type, frame.tf_regs.r_pc, frame.tf_tear, frame.tf_msr);

		panic("trap");
		/*NOTREACHED*/

	case T_UND | T_USER: {		/* undefined instruction fault */
		int cfg;
#ifndef NS381
		extern int _have_fpu;
		if (!_have_fpu) {
# if 0 /* XXX Glue in softfloat here */
			int rv;
			if ((rv = math_emulate(&frame)) == 0) {
				if (frame.tf_psr & PSL_T) {
					type = T_TRC | T_USER;
					goto trace;
				}
				return;
			}
# endif
		} else
#endif
		{
			sprd(cfg, cfg);
			if ((cfg & CFG_F) == 0) {
				lprd(cfg, cfg | CFG_F);
				if (fpu_lwp == l)
					return;
				pcb = &l->l_addr->u_pcb;
				if (fpu_lwp != 0)
					save_fpu_context(&fpu_lwp->l_addr->u_pcb);
				restore_fpu_context(pcb);
				fpu_lwp = l;
				return;
			}
		}
	}

	case T_ILL | T_USER:		/* privileged instruction fault */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_code = ILL_PRVOPC;
		ksi.ksi_addr = (void *)frame.tf_regs.r_pc;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		goto out;

	case T_AST | T_USER:		/* Allow process switch */
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		goto out;

	case T_OVF | T_USER:
	case T_DVZ | T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_code = ksi.ksi_trap == T_OVF ? FPE_FLTOVF : FPE_FLTDIV;
		ksi.ksi_addr = (void *)frame.tf_regs.r_pc;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		goto out;

	case T_SLAVE | T_USER: {
		int fsr, sig = SIGFPE;
		pcb = &l->l_addr->u_pcb;
		if (ieee_handler_disable == 0) {
			save_fpu_context(pcb);
			switch(ieee_handle_exception(l)) {
			case FPC_TT_NONE:
				restore_fpu_context(pcb);
				if (frame.tf_regs.r_psr &  PSL_T) {
					type = T_TRC | T_USER;
					goto trace;
				}
				return;
			case FPC_TT_ILL:
				sig = SIGILL;
				break;
			default:
				break;
			}
			restore_fpu_context(pcb);
		}
		sfsr(fsr);
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_trap = 0x80000000 | fsr;
		ksi.ksi_code = sig == SIGFPE ? FPE_FLTOVF : FPE_FLTINV;
		ksi.ksi_addr = (void *)frame.tf_regs.r_pc;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		goto out;
	}

	case T_ABT:			/* allow page faults in kernel mode */
		if ((frame.tf_msr & MSR_STT) == STT_SEQ_INS ||
		    (frame.tf_msr & MSR_STT) == STT_NSQ_INS ||
		    (p == 0))
			goto we_re_toast;
		pcb = &l->l_addr->u_pcb;
		/*
		 * {fu,su}bail is used by [fs]uswintr() to prevent page
		 * faulting from inside the profiling interrupt.
		 */
		if (pcb->pcb_onfault == fubail || pcb->pcb_onfault == subail)
			goto copyfault;
#ifdef CINVSMALL
		/*
		 * If a address translation for a cache invalidate
		 * request fails, reset the pc and return.
		 */
		if ((unsigned int)frame.tf_pc >= (unsigned int)cinvstart &&
		    (unsigned int)frame.tf_pc <  (unsigned int)cinvend) {
			frame.tf_pc = (int)cinvend;
			return;
		}
#endif
		/* FALLTHROUGH */

	case T_ABT | T_USER: {		/* page fault */
		vaddr_t va;
		struct vmspace *vm = p->p_vmspace;
		struct vm_map *map;
		int rv, sig;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

		va = trunc_page((vaddr_t)frame.tf_tear);
		/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_ABT && va >= KERNBASE)
			map = kernel_map;
		else {
			map = &vm->vm_map;
			if (l->l_flag & L_SA) {
				l->l_savp->savp_faultaddr =
				    (vaddr_t)frame.tf_tear;
				l->l_flag |= L_SA_PAGEFAULT;
			}
		}
		if ((frame.tf_msr & MSR_DDT) == DDT_WRITE ||
		    (frame.tf_msr & MSR_STT) == STT_RMW)
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
		rv = uvm_fault(map, va, ftype);
		if (rv == 0) {
			if (map != kernel_map && (caddr_t)va >= vm->vm_maxsaddr)
				uvm_grow(p, va);

			if (type == T_ABT)
				return;
			l->l_flag &= ~L_SA_PAGEFAULT;
			goto out;
		}

		if (type == T_ABT) {
			if (pcb->pcb_onfault != 0) {
			copyfault:
				frame.tf_regs.r_pc = (int)curpcb->pcb_onfault;
				return;
			}
			printf("uvm_fault(%p, 0x%lx, %d) -> %x\n",
			    map, va, ftype, rv);
			goto we_re_toast;
		}
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred ?
			       kauth_cred_geteuid(p->p_cred) : -1);
			sig = SIGKILL;
		} else {
			sig = SIGSEGV;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = sig;
		ksi.ksi_trap = T_ABT;
		ksi.ksi_code = sig == SIGKILL ? SI_NOINFO : SEGV_MAPERR;
		ksi.ksi_addr = (void *)frame.tf_tear;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		l->l_flag &= ~L_SA_PAGEFAULT;
		break;
	}

	case T_TRC | T_USER:	/* trace trap */
	case T_BPT | T_USER:	/* breakpoint instruction */
	case T_DBG | T_USER:	/* debug trap */
	trace:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_code = ksi.ksi_trap == T_TRC ? TRAP_TRACE : TRAP_BRKPT;
		ksi.ksi_addr = (void *)frame.tf_regs.r_pc;
		(*p->p_emul->e_trapsignal)(l, &ksi);
		break;

	case T_NMI:		/* non-maskable interrupt */
	case T_NMI | T_USER:
#if defined(KGDB) || defined(DDB)
		/* NMI can be hooked up to a pushbutton for debugging */
		printf ("NMI ... going to debugger\n");
# ifdef KGDB
		if (kgdb_trap(type, &frame))
			return;
# endif
# ifdef DDB
		if (kdb_trap (type, 0, &frame))
			return;
# endif
#endif
		goto we_re_toast;
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(l, frame.tf_regs.r_pc, sticks);
}

void
child_return(void *arg)
{
	struct lwp *l = arg;

	l->l_md.md_regs->r_r0 = 0;
	l->l_md.md_regs->r_psr &= ~PSL_C;

	userret(l, l->l_md.md_regs->r_pc, 0);
#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_SYSRET))
		ktrsysret(l, SYS_fork, 0, 0);
#endif
}

/*
 * Start a new LWP.
 */
void
startlwp(void *arg)
{
	int error;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	l->l_md.md_regs->r_r0 = 0;
	l->l_md.md_regs->r_psr &= ~PSL_C;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (error)
		printf("Error %d from cpu_setmcontext.", error);
#endif
	pool_put(&lwp_uc_pool, uc);

	userret(l, l->l_md.md_regs->r_pc, p->p_sticks);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	struct proc *p = l->l_proc;

	userret(l, l->l_md.md_regs->r_pc, p->p_sticks);
}
