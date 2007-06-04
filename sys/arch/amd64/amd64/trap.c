/*	$NetBSD: trap.c,v 1.32 2007/06/04 23:15:01 xtraeme Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.32 2007/06/04 23:15:01 xtraeme Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/pool.h>


#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/fpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>
#ifdef DDB
#include <machine/db_machdep.h>
#endif

#include "isa.h"

#ifdef KGDB
#include <sys/kgdb.h>
#endif

void trap(struct trapframe *);
#if defined(I386_CPU)
int trapwrite(unsigned);
#endif

const char *trap_type[] = {
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
	"machine check",			/* 18 T_MCA */
	"SSE FP exception",			/* 19 T_XMM */
	"reserved trap",			/* 20 T_RESERVED */
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

#ifdef DEBUG
int	trapdebug = 0;
#endif

#define	IDTVEC(name)	__CONCAT(X, name)

#undef TRAP_SIGDEBUG

#ifdef TRAP_SIGDEBUG
static void frame_dump(struct trapframe *);
#endif

/*
 * trap(frame):
 *	Exception, fault, and trap interface to BSD kernel. This
 * common code is called from assembly language IDT gate entry
 * routines that prepare a suitable stack frame, and restore this
 * frame after the exception has been processed. Note that the
 * effect is as if the arguments were passed call by reference.
 */
/*ARGSUSED*/
void
trap(struct trapframe *frame)
{
	struct lwp *l = curlwp;
	struct proc *p;
	int type = (int)frame->tf_trapno;
	struct pcb *pcb;
	extern char fusuintrfailure[],
		    resume_iret[];
#if defined(COMPAT_10) || defined(COMPAT_IBCS2)
	extern char IDTVEC(oosyscall)[];
#endif
#if 0
	extern char resume_pop_ds[], resume_pop_es[];
#endif
	struct trapframe *vframe;
	void *resume;
	void *onfault;
	int error;
	uint64_t cr2;
	ksiginfo_t ksi;

	uvmexp.traps++;

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
#ifdef DEBUG
	if (trapdebug) {
		printf("trap %d code %lx eip %lx cs %lx rflags %lx cr2 %lx "
		       "cpl %x\n",
		    type, frame->tf_err, frame->tf_rip, frame->tf_cs,
		    frame->tf_rflags, rcr2(), curcpu()->ci_ilevel);
		printf("curlwp %p\n", curlwp);
		if (curlwp)
			printf("pid %d lid %d\n", l->l_proc->p_pid, l->l_lid);
	}
#endif

	if (!KERNELMODE(frame->tf_cs, frame->tf_rflags)) {
		type |= T_USER;
		l->l_md.md_regs = frame;
		LWP_CACHE_CREDS(l, p);
	}

	switch (type) {

	default:
	we_re_toast:
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
#ifdef DDB
		if (kdb_trap(type, 0, frame))
			return;
#endif
		if (frame->tf_trapno < trap_types)
			printf("fatal %s", trap_type[frame->tf_trapno]);
		else
			printf("unknown trap %ld", (u_long)frame->tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %d code %lx rip %lx cs %lx rflags %lx cr2 "
		       " %lx cpl %x rsp %lx\n",
		    type, frame->tf_err, (u_long)frame->tf_rip, frame->tf_cs,
		    frame->tf_rflags, rcr2(), curcpu()->ci_ilevel, frame->tf_rsp);

		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_TSSFLT:
		if (p == NULL)
			goto we_re_toast;
		/* Check for copyin/copyout fault. */
		if (pcb->pcb_onfault != 0) {
copyefault:
			error = EFAULT;
copyfault:
			frame->tf_rip = (u_int64_t)pcb->pcb_onfault;
			frame->tf_rax = error;
			return;
		}

		/*
		 * Check for failure during return to user mode.
		 *
		 * XXXfvdl check for rex prefix?
		 *
		 * We do this by looking at the instruction we faulted on.  The
		 * specific instructions we recognize only happen when
		 * returning from a trap, syscall, or interrupt.
		 *
		 * XXX
		 * The heuristic used here will currently fail for the case of
		 * one of the 2 pop instructions faulting when returning from a
		 * a fast interrupt.  This should not be possible.  It can be
		 * fixed by rearranging the trap frame so that the stack format
		 * at this point is the same as on exit from a `slow'
		 * interrupt.
		 */
		switch (*(u_char *)frame->tf_rip) {
		case 0xcf:	/* iret */
			vframe = (void *)((u_int64_t)&frame->tf_rsp - 44);
			resume = resume_iret;
			break;
/*
 * XXXfvdl these are illegal in long mode (not in compat mode, though)
 * and we do not take back the descriptors from the signal context anyway,
 * but may do so later for USER_LDT, in which case we need to intercept
 * other instructions (movl %eax, %Xs).
 */
#if 0
		case 0x1f:	/* popl %ds */
			vframe = (void *)((u_int64_t)&frame->tf_rsp - 4);
			resume = resume_pop_ds;
			break;
		case 0x07:	/* popl %es */
			vframe = (void *)((u_int64_t)&frame->tf_rsp - 0);
			resume = resume_pop_es;
			break;
#endif
		default:
			goto we_re_toast;
		}
		if (KERNELMODE(vframe->tf_cs, vframe->tf_rflags))
			goto we_re_toast;

		frame->tf_rip = (u_int64_t)resume;
		return;

	case T_PROTFLT|T_USER:		/* protection fault */
	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_ALIGNFLT|T_USER:
	case T_NMI|T_USER:
#ifdef TRAP_SIGDEBUG
		printf("pid %d (%s): BUS (%x) at rip %lx addr %lx\n",
		    p->p_pid, p->p_comm, type, frame->tf_rip, rcr2());
		frame_dump(frame);
#endif
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_SEGNPFLT|T_USER:
		case T_STKFLT|T_USER:
			ksi.ksi_code = BUS_ADRERR;
			break;
		case T_TSSFLT|T_USER:
		case T_NMI|T_USER:
			ksi.ksi_code = BUS_OBJERR;
			break;
		case T_ALIGNFLT|T_USER:
			ksi.ksi_code = BUS_ADRALN;
			break;
		default:
			break;
		}
		goto trapsignal;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
#ifdef TRAP_SIGDEBUG
		printf("pid %d (%s): ILL at rip %lx addr %lx\n",
		    p->p_pid, p->p_comm, frame->tf_rip, rcr2());
		frame_dump(frame);
#endif
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_PRIVINFLT|T_USER:
			ksi.ksi_code = ILL_PRVOPC;
			break;
		case T_FPOPFLT|T_USER:
			ksi.ksi_code = ILL_COPROC;
			break;
		default:
			break;
		}
		goto trapsignal;

	case T_ASTFLT|T_USER:		/* Allow process switch */
		uvmexp.softs++;
		if (l->l_flag & LP_OWEUPC) {
			p->p_flag &= ~LP_OWEUPC;
			KERNEL_LOCK(1, l);
			ADDUPROF(l);
			KERNEL_UNLOCK_LAST(l);
		}
		/* Allow a forced task switch. */
		if (curcpu()->ci_want_resched) {
			curcpu()->ci_want_resched = 0;
			preempt();
		}
		goto out;

#if 0 /* handled by fpudna() */
	case T_DNA|T_USER: {
		printf("pid %d killed due to lack of floating point\n",
		    p->p_pid);
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGKILL;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)frame->tf_rip;
		goto trapsignal;
	}
#endif

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)frame->tf_rip;
		switch (type ) {
		case T_BOUND|T_USER:
		case T_OFLOW|T_USER:
			ksi.ksi_code = FPE_FLTOVF;
			break;
		case T_DIVIDE|T_USER:
			ksi.ksi_code = FPE_FLTDIV;
			break;
		default:
			break;
		}
		goto trapsignal;

	case T_ARITHTRAP|T_USER:
	case T_XMM|T_USER:
		fputrap(frame);
		goto out;

	case T_PAGEFLT:			/* allow page faults in kernel mode */
		if (l == NULL)
			goto we_re_toast;
		/*
		 * fusuintrfailure is used by [fs]uswintr() to prevent
		 * page faulting from inside the profiling interrupt.
		 */
		if (pcb->pcb_onfault == fusuintrfailure)
			goto copyefault;

		cr2 = rcr2();
		KERNEL_LOCK(1, NULL);
		goto faultcommon;

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register struct vm_map *map;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

		cr2 = rcr2();
		if (p->p_emul->e_usertrap != NULL &&
		    (*p->p_emul->e_usertrap)(l, cr2, frame) != 0)
			return;
		KERNEL_LOCK(1, l);
faultcommon:
		vm = p->p_vmspace;
		if (vm == NULL)
			goto we_re_toast;
		va = trunc_page((vaddr_t)cr2);
		/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= VM_MIN_KERNEL_ADDRESS)
			map = kernel_map;
		else
			map = &vm->vm_map;
		if (frame->tf_err & PGEX_W)
			ftype = VM_PROT_WRITE;
		else if (frame->tf_err & PGEX_X)
			ftype = VM_PROT_EXECUTE;
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

			if (type == T_PAGEFLT) {
				KERNEL_UNLOCK_ONE(NULL);
				return;
			}
			KERNEL_UNLOCK_LAST(l);
			goto out;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		if (error == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			error = EFAULT;
		} else
			ksi.ksi_code = SEGV_MAPERR;

		if (type == T_PAGEFLT) {
			if (pcb->pcb_onfault != 0) {
				KERNEL_UNLOCK_ONE(NULL);
				goto copyfault;
			}
			printf("uvm_fault(%p, 0x%lx, %d) -> %x\n",
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
#ifdef TRAP_SIGDEBUG
			printf("pid %d (%s): SEGV at rip %lx addr %lx\n",
			    p->p_pid, p->p_comm, frame->tf_rip, va);
			frame_dump(frame);
#endif
			ksi.ksi_signo = SIGSEGV;
		}
		(*p->p_emul->e_trapsignal)(l, &ksi);
		if (type == T_PAGEFLT)
			KERNEL_UNLOCK_ONE(NULL);
		else
			KERNEL_UNLOCK_LAST(l);
		break;
	}

	case T_TRCTRAP:
#if defined(COMPAT_10) || defined(COMPAT_IBCS2)
		/* Check whether they single-stepped into a lcall. */
		if (frame->tf_rip == (int)IDTVEC(oosyscall))
			return;
		if (frame->tf_rip == (int)IDTVEC(oosyscall) + 1) {
			frame->tf_rflags &= ~PSL_T;
			return;
		}
#endif
		goto we_re_toast;

	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
#ifdef MATH_EMULATE
	trace:
#endif
		if (LIST_EMPTY(&p->p_raslist) ||
		    (ras_lookup(p, (void *)frame->tf_rip) == (void *)-1)) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_trap = type & ~T_USER;
			if (type == (T_BPTFLT|T_USER))
				ksi.ksi_code = TRAP_BRKPT;
			else
				ksi.ksi_code = TRAP_TRACE;
			KERNEL_LOCK(1, l);
			(*p->p_emul->e_trapsignal)(l, &ksi);
			KERNEL_UNLOCK_LAST(l);
		}
		break;

#if	NISA > 0
	case T_NMI:
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

		if (x86_nmi() != 0)
			goto we_re_toast;
		else
			return;
#endif /* NISA > 0 */
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(l);
	return;
trapsignal:
	KERNEL_LOCK(1, l);
	(*p->p_emul->e_trapsignal)(l, &ksi);
	KERNEL_UNLOCK_LAST(l);
	userret(l);
}

void
startlwp(void *arg)
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	pool_put(&lwp_uc_pool, uc);

	KERNEL_UNLOCK_LAST(l);

	userret(l);
}

#ifdef TRAP_SIGDEBUG
static void
frame_dump(struct trapframe *tf)
{
	printf("rip %p  rsp %p  rfl %p\n",
	    (void *)tf->tf_rip, (void *)tf->tf_rsp, (void *)tf->tf_rflags);
	printf("rdi %p  rsi %p  rdx %p\n",
	    (void *)tf->tf_rdi, (void *)tf->tf_rsi, (void *)tf->tf_rdx);
	printf("rcx %p  r8  %p  r9  %p\n",
	    (void *)tf->tf_rcx, (void *)tf->tf_r8, (void *)tf->tf_r9);
	printf("r10 %p  r11 %p  r12 %p\n",
	    (void *)tf->tf_r10, (void *)tf->tf_r11, (void *)tf->tf_r12);
	printf("r13 %p  r14 %p  r15 %p\n",
	    (void *)tf->tf_r13, (void *)tf->tf_r14, (void *)tf->tf_r15);
	printf("rbp %p  rbx %p  rax %p\n",
	    (void *)tf->tf_rbp, (void *)tf->tf_rbx, (void *)tf->tf_rax);
}
#endif
