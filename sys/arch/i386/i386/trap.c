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
 *	from: @(#)trap.c	7.4 (Berkeley) 5/13/91
 *	$Id: trap.c,v 1.14.2.12 1993/11/08 20:21:14 mycroft Exp $
 */

/*
 * 386 Trap and System call handling
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/vmmeter.h>

#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/reg.h>
#include <machine/trap.h>

#include "npx.h"
#include "fpe.h"

struct	sysent sysent[];
int	nsysent;

/*
 * Define the code needed before returning to user mode, for
 * trap, and syscall.
 */
static inline void
userret(p, pc, oticks)
	struct proc *p;
	int pc;
	u_quad_t oticks;
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		psig(sig);
	p->p_pri = p->p_usrpri;
	if (want_resched) {
		/*
		 * Since we are curproc, a clock interrupt could
		 * change our priority without changing run queues
		 * (the running process is not kept on a run queue).
		 * If this happened after we setrq ourselves but
		 * before we swtch()'ed, we might not be on the queue
		 * indicated by our priority.
		 */
		(void) splstatclock();
		setrq(p);
		p->p_stats->p_ru.ru_nivcsw++;
		swtch();
		(void) spl0();
		while ((sig = CURSIG(p)) != 0)
			psig(sig);
	}

	/*
	 * If profiling, charge recent system time to the trapped pc.
	 */
	if (p->p_flag & SPROFIL)
		addupc_task(p, pc, (int)(p->p_sticks - oticks));

	curpri = p->p_pri;
}

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
	register int i;
	register struct proc *p = curproc;
	u_quad_t sticks;
	int ucode, type, code, eva;
	extern char fusubail[];

	cnt.v_trap++;

#ifdef DEBUG
	printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
	       frame.tf_trapno, frame.tf_err, frame.tf_eip, frame.tf_cs,
	       frame.tf_eflags, rcr2(), cpl);
	printf("curpcb %x curproc %x\n", curpcb, curproc);
#endif

	frame.tf_eflags &= ~PSL_NT;	/* clear nested trap XXX */
	type = frame.tf_trapno;

	if (curpcb == 0)
		goto we_re_toast;

#ifdef DDB
	if (curpcb->pcb_onfault)
		if (type == T_BPTFLT || type == T_TRCTRAP)
			if (kdb_trap (type, 0, &frame))
				return;
#endif

	if (curproc == 0)
		goto we_re_toast;

	if (curpcb->pcb_onfault && type != T_PAGEFLT) {
	    copyfault:
		frame.tf_eip = (int)curpcb->pcb_onfault;
		return;
	}

	sticks = p->p_sticks;

	if (ISPL(frame.tf_cs) == SEL_UPL) {
		type |= T_USER;
		p->p_regs = (int *)&frame;
	}

	ucode = 0;
	eva = rcr2();
	code = frame.tf_err;

	switch (type) {

	    default:
	    we_re_toast:
#ifdef KDB /* XXX KGDB? */
		if (kdb_trap(&psl))
			return;
#endif
#ifdef DDB
		if (kdb_trap(type, 0, &frame))
			return;
#endif

		printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
		       type, code, frame.tf_eip, frame.tf_cs, frame.tf_eflags, eva, cpl);
		panic("trap");
		/*NOTREACHED*/

	    case T_SEGNPFLT|T_USER:
	    case T_STKFLT|T_USER:
	    case T_PROTFLT|T_USER:		/* protection fault */
	    case T_ALIGNFLT|T_USER:
		ucode = code + BUS_SEGM_FAULT ;
		i = SIGBUS;
		break;

	    case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	    case T_RESADFLT|T_USER:		/* reserved addressing fault */
	    case T_RESOPFLT|T_USER:		/* reserved operand fault */
	    case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		ucode = type &~ T_USER;
		i = SIGILL;
		break;

	    case T_ASTFLT|T_USER:		/* Allow process switch */
		cnt.v_soft++;
		if (p->p_flag & SOWEUPC) {
			p->p_flag &= ~SOWEUPC;
			ADDUPROF(p);
		}
		goto out;

	    case T_DNA|T_USER:
#if NNPX > 0
		/* if a transparent fault (due to context switch "late") */
		if (npxdna())
			return;
#endif
#if NFPE > 0
		i = math_emulate(&frame);
		if (i == 0) {
#ifdef		TRACE_EMU
			if (frame.tf_eflags & PSL_T)
				goto trace;
#endif
			return;
		}
#else
		printf("pid %d killed due to lack of fpu and fpe\n",
		       p->p_pid);
		i = SIGKILL;
#endif
		ucode = FPE_FPU_NP_TRAP;
		break;

	    case T_BOUND|T_USER:
		ucode = FPE_SUBRNG_TRAP;
		i = SIGFPE;
		break;

	    case T_OFLOW|T_USER:
		ucode = FPE_INTOVF_TRAP;
		i = SIGFPE;
		break;

	    case T_DIVIDE|T_USER:
		ucode = FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	    case T_ARITHTRAP|T_USER:
		ucode = code;
		i = SIGFPE;
		break;

	    case T_PAGEFLT:			/* allow page faults in kernel mode */
#if 0
		/* XXX - check only applies to 386's and 486's with WP off */
		if (code & PGEX_P)
			goto we_re_toast;
#endif

		/*FALLTHROUGH*/
	    case T_PAGEFLT|T_USER: { 		/* page fault */
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss, v;

		/* fusubail is used by [fs]uswintr to avoid page faulting */
		if (curpcb->pcb_onfault == fusubail)
			goto copyfault;

		va = trunc_page((vm_offset_t)eva);
		/*
		 * It is only a kernel address space fault iff:
		 * 	1. (type & T_USER) == 0  and
		 * 	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= KERNBASE)
			map = kernel_map;
		else
			map = &vm->vm_map;
		if (code & PGEX_W)
			ftype = VM_PROT_READ | VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %x\n", va);
			goto we_re_toast;
		}
#endif

		nss = 0;
		if ((caddr_t)va >= vm->vm_maxsaddr
		    && (caddr_t)va < (caddr_t)VM_MAXUSER_ADDRESS
		    && map != kernel_map) {
			nss = clrnd(btoc(USRSTACK-(unsigned)va));
			if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
				rv = KERN_FAILURE;
				goto nogo;
			}
		}

		/* check if page table is mapped, if not, fault it first */
		if (!PTD[pdei(va)].pd_v) {
			v = trunc_page(vtopte(va));
			rv = vm_fault(map, v, ftype, FALSE);
			if (rv != KERN_SUCCESS)
				goto nogo;
			/* check if page table fault, increment wiring */
			vm_map_pageable(map, v, round_page(v+1), FALSE);
		} else
			v = 0;

		rv = vm_fault(map, va, ftype, FALSE);
		if (rv == KERN_SUCCESS) {
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
			va = trunc_page(vtopte(va));
			/* for page table, increment wiring as long as
			   not a page table fault as well */
			if (!v && map != kernel_map)
				vm_map_pageable(map, va, round_page(va+1), FALSE);
			if (type == T_PAGEFLT)
				return;
			goto out;
		}
	    nogo:
		if (type == T_PAGEFLT) {
			if (curpcb->pcb_onfault)
				goto copyfault;
			printf("vm_fault(%x, %x, %x, 0) -> %x\n",
			       map, va, ftype, rv);
			printf("  type %x, code %x\n",
			       type, code);
			goto we_re_toast;
		}
		i = (rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV;
		break;
	    }

#ifndef DDB
		/* XXX need to deal with this when DDB is present, too */
	    case T_TRCTRAP:	 /* kernel trace trap; someone single stepping lcall's */
		/* restored later from lcall frame */
		frame.tf_eflags &= ~PSL_T;
		return;
#endif

	    case T_BPTFLT|T_USER:		/* bpt instruction fault */
	    case T_TRCTRAP|T_USER:		/* trace trap */
	    trace:
		frame.tf_eflags &= ~PSL_T;
		i = SIGTRAP;
		break;

#include "isa.h"
#if	NISA > 0
	    case T_NMI:
	    case T_NMI|T_USER:
#ifdef DDB
		/* NMI can be hooked up to a pushbutton for debugging */
		printf ("NMI ... going to debugger\n");
		if (kdb_trap (type, 0, &frame))
			return;
#endif
		/* machine/parity/power fail/"kitchen sink" faults */
		if (isa_nmi() == 0)
			return;
		else
			goto we_re_toast;
#endif
	}

	trapsignal(p, i, ucode);
	if ((type & T_USER) == 0)
		return;
    out:
	userret(p, frame.tf_eip, sticks);
}

/*
 * Compensate for 386 brain damage (missing URKR)
 */
int
trapwrite(addr)
	unsigned addr;
{
	vm_offset_t va;
	unsigned nss;
	struct proc *p;
	struct vmspace *vm;

	va = trunc_page((vm_offset_t)addr);
	if (va >= VM_MAXUSER_ADDRESS)
		return 1;

	nss = 0;
	p = curproc;
	vm = p->p_vmspace;
	if ((caddr_t)va >= vm->vm_maxsaddr) {
		nss = clrnd(btoc(USRSTACK-(unsigned)va));
		if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur))
			return 1;
	}

	if (vm_fault(&vm->vm_map, va, VM_PROT_READ | VM_PROT_WRITE, FALSE)
	    != KERN_SUCCESS)
		return 1;

	if (nss > vm->vm_ssize)
		vm->vm_ssize = nss;

	return 0;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
/*ARGSUSED*/
void
syscall(frame)
	volatile struct trapframe frame;
{
	register int *locr0 = ((int *)&frame);
	register caddr_t params;
	register int i;
	register struct sysent *callp;
	register struct proc *p = curproc;
	u_quad_t sticks;
	int error, opc;
	int args[8], rval[2];
	int code;

	if (ISPL(frame.tf_cs) != SEL_UPL)
		panic("syscall");

	cnt.v_syscall++;

	sticks = p->p_sticks;
	code = frame.tf_eax;
	p->p_regs = (int *)&frame;
	params = (caddr_t)frame.tf_esp + sizeof(int);

	/*
	 * Reconstruct pc, assuming lcall $X,y is 7 bytes, as it is always.
	 */
	opc = frame.tf_eip - 7;
        if (code == 0) {                        /* indir */
                code = fuword(params);
                params += sizeof(int);
        }
        if (code <= 0 || code >= nsysent)
                callp = &sysent[0];             /* indir (illegal) */
        else
                callp = &sysent[code];
	if ((i = callp->sy_narg * sizeof (int)) &&
	    (error = copyin(params, (caddr_t)args, (u_int)i))) {
		frame.tf_eax = error;
		frame.tf_eflags |= PSL_C;	/* carry bit */
#ifdef KTRACE
		if (KTRPOINT(p, KTR_SYSCALL))
			ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
		goto done;
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
	rval[0] = 0;
	rval[1] = frame.tf_edx;
	error = (*callp->sy_call)(p, args, rval);
	if (error == ERESTART)
		frame.tf_eip = opc;
	else if (error != EJUSTRETURN)
		if (error) {
			frame.tf_eax = error;
			frame.tf_eflags |= PSL_C;	/* carry bit */
		} else {
			frame.tf_eax = rval[0];
			frame.tf_edx = rval[1];
			frame.tf_eflags &= ~PSL_C;	/* carry bit */
		}
	/* else if (error == EJUSTRETURN) */
		/* nothing to do */
    done:
	/*
	 * Reinitialize proc pointer `p' as it may be different
	 * if this is a child returning from fork syscall.
	 */
	p = curproc;
	userret(p, frame.tf_eip, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, code, error, rval[0]);
#endif
#ifdef	DIAGNOSTIC
{ extern int _udatasel, _ucodesel;
	if (frame.tf_ss != _udatasel)
		printf("ss %x call %d\n", frame.tf_ss, code);
	if ((frame.tf_cs & 0xffff) != _ucodesel)
		printf("cs %x call %d\n", frame.tf_cs, code);
	if (frame.tf_eip > VM_MAXUSER_ADDRESS) {
		printf("eip %x call %d\n", frame.tf_eip, code);
		frame.tf_eip = 0;
	}
}
#endif
}
