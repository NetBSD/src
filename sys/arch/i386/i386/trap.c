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
 *	$Id: trap.c,v 1.21 1993/12/20 17:16:47 ws Exp $
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
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>

#include "npx.h"

struct	sysent sysent[];
int	nsysent;
unsigned rcr2();
extern int cpl;

/*
 * Define the code needed before returning to user mode, for
 * trap and syscall.
 */
static inline void
userret(p, pc, syst)
	struct proc *p;
	int pc;
	struct timeval syst;
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
		(void) splclock();
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
	if (p->p_stats->p_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &p->p_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks) {
#ifdef PROFTIMER
			extern int profscale;
			addupc(pc, &p->p_stats->p_prof, ticks * profscale);
#else
			addupc(pc, &p->p_stats->p_prof, ticks);
#endif
		}
	}

	curpri = p->p_pri;
}

char *trapstr[] = {
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
};

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
	register struct proc *p;
	register struct pcb *pcb;
	int type, code;
	struct timeval syst;
#ifdef notyet
	extern char fusubail[];
#endif

	cnt.v_trap++;

#ifdef DEBUG
	printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
		frame.tf_trapno, frame.tf_err, frame.tf_eip, frame.tf_cs,
		frame.tf_eflags, rcr2(), cpl);
	printf("curproc %x\n", curproc);
#endif

	frame.tf_eflags &= ~PSL_NT;	/* clear nested trap XXX */
	type = frame.tf_trapno;

	if ((p = curproc) == 0)
		p = &proc0;
	/* can't use curpcb, as it might be NULL; and we have p in a register
	   anyway */
	pcb = &p->p_addr->u_pcb;

	if (pcb == 0)
		goto we_re_toast;

	/* fusubail is used by [fs]uswintr to avoid page faulting */
	if (pcb->pcb_onfault &&
	    (type != T_PAGEFLT /*|| pcb->pcb_onfault == fusubail*/)) {
	    copyfault:
		frame.tf_eip = (int)pcb->pcb_onfault;
		return;
	}

	syst = p->p_stime;

	if (ISPL(frame.tf_cs) != SEL_KPL) {
		type |= T_USER;
		p->p_regs = (int *)&frame;
	}

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

		if (frame.tf_trapno < (sizeof(trapstr) / sizeof(trapstr[0])) &&
		    trapstr[frame.tf_trapno])
			printf("fatal %s", trapstr[frame.tf_trapno]);
		else
			printf("unknown trap %d", frame.tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x cpl %x\n",
			type, code, frame.tf_eip, frame.tf_cs, frame.tf_eflags, rcr2(), cpl);

		panic("trap");
		/*NOTREACHED*/

	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_PROTFLT|T_USER:		/* protection fault */
	case T_ALIGNFLT|T_USER:
		trapsignal(p, SIGBUS, type &~ T_USER);
		break;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		trapsignal(p, SIGILL, type &~ T_USER);
		break;

	case T_ASTFLT|T_USER:		/* Allow process switch */
		cnt.v_soft++;
		if ((p->p_flag & SOWEUPC) && p->p_stats->p_prof.pr_scale) {
			addupc(frame.tf_eip, &p->p_stats->p_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		goto out;

	case T_DNA|T_USER: {
		int rv;
#if NNPX > 0
		/* if a transparent fault (due to context switch "late") */
		if (npxdna())
			return;
#endif
#ifdef MATH_EMULATE
		rv = math_emulate(&frame);
		if (rv == 0) {
			if (frame.tf_eflags & PSL_T)
				goto trace;
			return;
		}
#else
		printf("pid %d killed due to lack of floating point\n",
			p->p_pid);
		rv = SIGKILL;
#endif
		trapsignal(p, rv, type &~ T_USER);
		break;
	}

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
		trapsignal(p, SIGFPE, type &~ T_USER);
		break;

	case T_ARITHTRAP|T_USER:
		trapsignal(p, SIGFPE, code);
		break;

	case T_PAGEFLT:			/* allow page faults in kernel mode */
#if 0
		/* XXX - check only applies to 386's and 486's with WP off */
		if (code & PGEX_P)
			goto we_re_toast;
#endif

		/*FALLTHROUGH*/
	case T_PAGEFLT|T_USER: {		/* page fault */
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss, v;

		va = trunc_page((vm_offset_t)rcr2());
		/*
		 * It is only a kernel address space fault iff:
		 *      1. (type & T_USER) == 0  and
		 *      2. pcb_onfault not set or
		 *      3. pcb_onfault set but supervisor space fault
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
				vm_map_pageable(map, va, round_page(va+1),
						FALSE);
			if (type == T_PAGEFLT)
				return;
			goto out;
		}
	nogo:
		if (type == T_PAGEFLT) {
			if (pcb->pcb_onfault)
				goto copyfault;
			printf("vm_fault(%x, %x, %x, 0) -> %x\n",
			       map, va, ftype, rv);
			goto we_re_toast;
		}
		trapsignal(p, (rv == KERN_PROTECTION_FAILURE)
			      ? SIGBUS : SIGSEGV, T_PAGEFLT);
		break;
	    }

#ifndef DDB
	/* XXX need to deal with this when DDB is present, too */
	case T_TRCTRAP: /* kernel trace trap -- someone single stepping lcall's */
			/* syscall has to turn off the trace bit itself */
		return;
#endif
	
	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
	trace:
		frame.tf_eflags &= ~PSL_T;
		trapsignal(p, SIGTRAP, type &~ T_USER);
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

	if ((type & T_USER) == 0)
		return;
out:
	userret(p, frame.tf_eip, syst);
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
	register caddr_t params;
	register int i;
	register struct sysent *callp;
	register struct proc *p = curproc;
	struct timeval syst;
	int error, opc;
	int args[8], rval[2];
	int code;

	if (ISPL(frame.tf_cs) != SEL_UPL)
		panic("syscall");

	cnt.v_syscall++;

	syst = p->p_stime;
	code = frame.tf_eax;
	p->p_regs = (int *)&frame;
	params = (caddr_t)frame.tf_esp + sizeof(int);

	/*
	 * Reconstruct pc, assuming lcall $X,y is 7 bytes, as it is always.
	 */
	opc = frame.tf_eip - 7;
	if (code == 0) {			/* indir */
		code = fuword(params);
		params += sizeof(int);
	}
	if (code <= 0 || code >= nsysent)
		callp = &sysent[0];		/* indir (illegal) */
	else
		callp = &sysent[code];
	if ((i = callp->sy_narg * sizeof(int)) &&
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
	else if (error != EJUSTRETURN) {
		if (error) {
			frame.tf_eax = error;
			frame.tf_eflags |= PSL_C;	/* carry bit */
		} else {
			frame.tf_eax = rval[0];
			frame.tf_edx = rval[1];
			frame.tf_eflags &= ~PSL_C;	/* carry bit */
		}
	}
	/* else if (error == EJUSTRETURN) */
		/* nothing to do */
done:
	/*
	 * Reinitialize proc pointer `p' as it may be different
	 * if this is a child returning from fork syscall.
	 */
	p = curproc;
	userret(p, frame.tf_eip, syst);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, code, error, rval[0]);
#endif
#ifdef	DIAGNOSTIC
{ extern int _udatasel, _ucodesel;
	if (frame.tf_ss != _udatasel)
		printf("ss %x call %d\n", frame.tf_ss, code);
	if ((frame.tf_cs&0xffff) != _ucodesel)
		printf("cs %x call %d\n", frame.tf_cs, code);
	if (frame.tf_eip > VM_MAXUSER_ADDRESS) {
		printf("eip %x call %d\n", frame.tf_eip, code);
		frame.tf_eip = 0;
	}
}
#endif
}
