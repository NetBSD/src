/*	$NetBSD: trap.c,v 1.134.2.11 2001/01/07 22:12:45 sommerfeld Exp $	*/

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
 * 386 Trap and System call handling
 */

#include "opt_ddb.h"
#include "opt_math_emulate.h"
#include "opt_vm86.h"
#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
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

#include "isa.h"

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "npx.h"

void trap __P((struct trapframe));
#if defined(I386_CPU)
int trapwrite __P((unsigned));
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
	"reserved trap",			/* 18 T_RESERVED */
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

#ifdef DEBUG
int	trapdebug = 0;
#endif

#define	IDTVEC(name)	__CONCAT(X, name)

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
trap(frame)
	struct trapframe frame;
{
	register struct proc *p = curproc;
	int type = frame.tf_trapno;
	struct pcb *pcb = NULL;
	extern char fusubail[],
		    resume_iret[], resume_pop_ds[], resume_pop_es[],
		    IDTVEC(osyscall)[];
	struct trapframe *vframe;
	int resume;

	uvmexp.traps++;

#ifdef DEBUG
	if (trapdebug) {
		printf("trap %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
		    frame.tf_trapno, frame.tf_err, frame.tf_eip, frame.tf_cs,
		    frame.tf_eflags, rcr2(), lapic_tpr);
		printf("curproc %p\n", curproc);
	}
#endif

	if (!KERNELMODE(frame.tf_cs, frame.tf_eflags)) {
		type |= T_USER;
		p->p_md.md_regs = &frame;
	}

	switch (type) {

	default:
	we_re_toast:
#ifdef KGDB
		if (kgdb_trap(type, &frame))
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
		if (kdb_trap(type, 0, &frame))
			return;
#endif
		if (frame.tf_trapno < trap_types)
			printf("fatal %s", trap_type[frame.tf_trapno]);
		else
			printf("unknown trap %d", frame.tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
		    type, frame.tf_err, frame.tf_eip, frame.tf_cs, frame.tf_eflags, rcr2(), lapic_tpr);

		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_TSSFLT:
		/* Check for copyin/copyout fault. */
		pcb = &p->p_addr->u_pcb;
		if (pcb->pcb_onfault != 0) {
		copyfault:
			frame.tf_eip = (int)pcb->pcb_onfault;
			return;
		}

		/*
		 * Check for failure during return to user mode.
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
		switch (*(u_char *)frame.tf_eip) {
		case 0xcf:	/* iret */
			vframe = (void *)((int)&frame.tf_esp - 44);
			resume = (int)resume_iret;
			break;
		case 0x1f:	/* popl %ds */
			vframe = (void *)((int)&frame.tf_esp - 4);
			resume = (int)resume_pop_ds;
			break;
		case 0x07:	/* popl %es */
			vframe = (void *)((int)&frame.tf_esp - 0);
			resume = (int)resume_pop_es;
			break;
		default:
			goto we_re_toast;
		}
		if (KERNELMODE(vframe->tf_cs, vframe->tf_eflags))
			goto we_re_toast;

		frame.tf_eip = resume;
		return;

	case T_PROTFLT|T_USER:		/* protection fault */
#ifdef VM86
		if (frame.tf_eflags & PSL_VM) {
			vm86_gpfault(p, type & ~T_USER);
			goto out;
		}
#endif
	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_ALIGNFLT|T_USER:
	case T_NMI|T_USER:
		KERNEL_PROC_LOCK(p);
		trapsignal(p, SIGBUS, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);		
		goto out;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		KERNEL_PROC_LOCK(p);
		trapsignal(p, SIGILL, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);
		goto out;

	case T_ASTFLT|T_USER:		/* Allow process switch */
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		/* Allow a forced task switch. */
		if (want_resched)
			preempt(NULL);
		goto out;

	case T_DNA|T_USER: {
#ifdef MATH_EMULATE
		int rv;
		if ((rv = math_emulate(&frame)) == 0) {
			if (frame.tf_eflags & PSL_T)
				goto trace;
			return;
		}
		KERNEL_PROC_LOCK(p);
		trapsignal(p, rv, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);
		goto out;
#else
		printf("pid %d killed due to lack of floating point\n",
		    p->p_pid);
		KERNEL_PROC_LOCK(p);		
		trapsignal(p, SIGKILL, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);		
		goto out;
#endif
	}

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
		KERNEL_PROC_LOCK(p);		
		trapsignal(p, SIGFPE, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);		
		goto out;

	case T_ARITHTRAP|T_USER:
		KERNEL_PROC_LOCK(p);		
		trapsignal(p, SIGFPE, frame.tf_err);
		KERNEL_PROC_UNLOCK(p);		
		goto out;

	case T_PAGEFLT:			/* allow page faults in kernel mode */
		if (p == 0)
			goto we_re_toast;

#ifdef LOCKDEBUG
		/* If we page-fault while in scheduler, we're doomed. */
		if (simple_lock_held(&sched_lock))
			goto we_re_toast;
#endif		
		/*
		 * process doing kernel-mode page fault must have
		 * been running with big lock held
		 */
		if ((p->p_flag & P_BIGLOCK) == 0)
			goto we_re_toast;

		pcb = &p->p_addr->u_pcb;
		/*
		 * fusubail is used by [fs]uswintr() to prevent page faulting
		 * from inside the profiling interrupt.
		 */
		if (pcb->pcb_onfault == fusubail)
			goto copyfault;
#if 0
		/* XXX - check only applies to 386's and 486's with WP off */
		if (frame.tf_err & PGEX_P)
			goto we_re_toast;
#endif
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		goto faultcommon;

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss;

		KERNEL_PROC_LOCK(p);
	faultcommon:
		vm = p->p_vmspace;
		if (vm == NULL)
			goto we_re_toast;
		va = trunc_page((vaddr_t)rcr2());
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
		if (frame.tf_err & PGEX_W)
			ftype = VM_PROT_READ | VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %lx\n", va);
			goto we_re_toast;
		}
#endif

		nss = 0;
		if ((caddr_t)va >= vm->vm_maxsaddr
		    && (caddr_t)va < (caddr_t)VM_MAXUSER_ADDRESS
		    && map != kernel_map) {
			nss = btoc(USRSTACK-(unsigned)va);
			if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
				/*
				 * We used to fail here. However, it may
				 * just have been an mmap()ed page low
				 * in the stack, which is legal. If it
				 * wasn't, uvm_fault() will fail below.
				 *
				 * Set nss to 0, since this case is not
				 * a "stack extension".
				 */
				nss = 0;
			}
		}

		/* Fault the original page in. */
		rv = uvm_fault(map, va, 0, ftype);
		if (rv == KERN_SUCCESS) {
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;

			if (type == T_PAGEFLT) {
				KERNEL_UNLOCK();
				return;
			}
			KERNEL_PROC_UNLOCK(p);
			goto out;
		}

		if (type == T_PAGEFLT) {
			if (pcb->pcb_onfault != 0) {
				KERNEL_UNLOCK();
				goto copyfault;
			}
			printf("uvm_fault(%p, 0x%lx, 0, %d) -> %x\n",
			    map, va, ftype, rv);
			goto we_re_toast;
		}

		if (rv == KERN_RESOURCE_SHORTAGE) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(p, SIGKILL, T_PAGEFLT);
		} else {
			trapsignal(p, SIGSEGV, T_PAGEFLT);
		}
		if (type == T_PAGEFLT)
			KERNEL_UNLOCK();
		else
			KERNEL_PROC_UNLOCK(p);
		break;
	}

	case T_TRCTRAP:
		/* Check whether they single-stepped into a lcall. */
		if (frame.tf_eip == (int)IDTVEC(osyscall))
			return;
		if (frame.tf_eip == (int)IDTVEC(osyscall) + 1) {
			frame.tf_eflags &= ~PSL_T;
			return;
		}
		goto we_re_toast;

	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
#ifdef MATH_EMULATE
	trace:
#endif
		KERNEL_PROC_LOCK(p);
		trapsignal(p, SIGTRAP, type &~ T_USER);
		KERNEL_PROC_UNLOCK(p);
		break;

#if	NISA > 0 || NMCA > 0
	case T_NMI:
#if defined(KGDB) || defined(DDB)
		/* NMI can be hooked up to a pushbutton for debugging */
		printf ("NMI ... going to debugger\n");
#ifdef KGDB

		if (kgdb_trap(type, &frame))
			return;
#endif
#ifdef DDB
		if (kdb_trap(type, 0, &frame))
			return;
#endif
#endif /* KGDB || DDB */
		/* machine/parity/power fail/"kitchen sink" faults */

#if NMCA > 0
		/* mca_nmi() takes care to call isa_nmi() if appropriate */
		if (mca_nmi() != 0)
			goto we_re_toast;
		else
			return;
#else /* NISA > 0 */
		if (isa_nmi() != 0)
			goto we_re_toast;
		else
			return;
#endif /* NMCA > 0 */
#endif /* NISA > 0 || NMCA > 0 */
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(p);
}

#if defined(I386_CPU)

#ifdef MULTIPROCESSOR
/* XXX XXX XXX */
#endif
/*
 * Compensate for 386 brain damage (missing URKR)
 */
int
trapwrite(addr)
	unsigned addr;
{
	vaddr_t va;
	unsigned nss;
	struct proc *p;
	struct vmspace *vm;

	va = trunc_page((vaddr_t)addr);
	if (va >= VM_MAXUSER_ADDRESS)
		return 1;

	nss = 0;
	p = curproc;
	vm = p->p_vmspace;
	if ((caddr_t)va >= vm->vm_maxsaddr) {
		nss = btoc(USRSTACK-(unsigned)va);
		if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur))
			nss = 0;
	}

	if (uvm_fault(&vm->vm_map, va, 0, VM_PROT_READ | VM_PROT_WRITE)
	    != KERN_SUCCESS)
		return 1;

	if (nss > vm->vm_ssize)
		vm->vm_ssize = nss;

	return 0;
}
#endif /* I386_CPU */
