/*	$NetBSD: trap.c,v 1.23 2001/03/15 06:10:48 chs Exp $	*/

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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

#define RECURSE_TLB_HANDLER

/*
 * SH3 Trap and System call handling
 *
 * T.Horiuchi 1998.06.8
 */

#include "opt_ddb.h"
#include "opt_syscall_debug.h"
#include "opt_ktrace.h"

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
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <sh3/trapreg.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#ifdef DDB
#include <machine/db_machdep.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

extern int cpu_debug_mode;

static __inline void userret __P((struct proc *, int, u_quad_t));
void trap __P((int, int, int, int, struct trapframe));
int trapwrite __P((unsigned));
void syscall __P((struct trapframe *));
void
tlb_handler __P((
	    int p1, int p2, int p3, int p4, /* These four param is dummy */
	    struct trapframe frame));

/*
 * Define the code needed before returning to user mode, for
 * trap and syscall.
 */
static __inline void
userret(p, pc, oticks)
	register struct proc *p;
	int pc;
	u_quad_t oticks;
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If profiling, charge recent system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, pc, (int)(p->p_sticks - oticks) * psratio);
	}

	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}

char	*trap_type[] = {
	"power-on",				/* 0x000 T_POWERON */
	"manual reset",				/* 0x020 T_RESET */
	"TLB miss/invalid (load)",		/* 0x040 T_TLBMISSR */
	"TLB miss/invalid (store)",		/* 0x060 T_TLBMISSW */
	"initial page write",			/* 0x080 T_INITPAGEWR */
	"TLB protection violation (load)",	/* 0x0a0 T_TLBPRIVR */
	"TLB protection violation (store)",	/* 0x0c0 T_TLBPRIVW */
	"address error (load)",			/* 0x0e0 T_ADDRESSERRR */
	"address error (store)",		/* 0x100 T_ADDRESSERRW */
	"unknown trap (0x120)",			/* 0x120 */
	"unknown trap (0x140)",			/* 0x140 */
	"unconditional trap (TRAPA)",		/* 0x160 T_TRAP */
	"reserved instruction code exception",	/* 0x180 T_INVALIDISN */
	"illegal slot instruction exception",	/* 0x1a0 T_INVALIDSLOT */
	"nonmaskable interrupt",		/* 0x1c0 T_NMI */
	"user break point trap",		/* 0x1e0 T_USERBREAK */
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

#define	DEBUG 1
#ifdef DEBUG
int	trapdebug = 1;
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
trap(p1, p2, p3, p4, frame)
     int p1, p2, p3, p4; /* dummy param */
     struct trapframe frame;
{
	register struct proc *p = curproc;
	int type = frame.tf_trapno;
	u_quad_t sticks;
	struct pcb *pcb = NULL;
	int resume;
	vaddr_t va;

	if (p == NULL)
		goto we_re_toast;
	uvmexp.traps++;

#ifdef TRAPDEBUG
	if (trapdebug) {
		printf("trap %x spc %x ssr %x \n",
			   frame.tf_trapno, frame.tf_spc, frame.tf_ssr);
		printf("curproc %p\n", curproc);
	}
#endif

#if 1
	if (!KERNELMODE(frame.tf_r15)) {
#else
	if (!KERNELMODE(frame.tf_spc, frame.tf_ssr)) {
#endif
		type |= T_USER;
		sticks = p->p_sticks;
		p->p_md.md_regs = &frame;
	}
	else
		sticks = 0;

	switch (type) {

	default:
	we_re_toast:
#ifdef DDB
		if (kdb_trap(type, 0, &frame))
			return;
#endif
		if (frame.tf_trapno >> 5 < trap_types)
			printf("fatal %s", trap_type[frame.tf_trapno >> 5]);
		else
			printf("unknown trap %x", frame.tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %x spc %x ssr %x \n",
			   type, frame.tf_spc, frame.tf_ssr);

		panic("trap");
		/*NOTREACHED*/

	case T_TRAP|T_USER:
		if (SHREG_TRA == (0x000000c3 << 2)) {
			trapsignal(p, SIGTRAP, type &~ T_USER);
			break;
		} else {
			syscall(&frame);
			return;
		}

	case T_INITPAGEWR:
	case T_INITPAGEWR|T_USER:
		va = (vaddr_t)SHREG_TEA;
		pmap_emulate_reference(p, va, type & T_USER, 1);
		return;

	case T_TRAP:
		goto we_re_toast;

	case T_ADDRESSERRR:
	case T_ADDRESSERRW:
	case T_INVALIDSLOT:
		/* Check for copyin/copyout fault. */
		pcb = &p->p_addr->u_pcb;
		if (pcb->pcb_onfault != 0) {
#ifdef	TODO
		copyfault:
#endif
			printf("copyin/copyout fault\n");
			frame.tf_spc = (int)pcb->pcb_onfault;
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
		switch (*(u_char *)frame.tf_spc) {
#ifdef TODO
		case 0xcf:	/* iret */
			vframe = (void *)((int)&frame.tf_esp - 44);
			resume = (int)resume_iret;
			break;
#endif
		default:
			goto we_re_toast;
		}
		frame.tf_spc = resume;
		return;

	case T_ADDRESSERRR|T_USER:		/* protection fault */
	case T_ADDRESSERRW|T_USER:
	case T_INVALIDSLOT|T_USER:
		printf("trap type %x spc %x ssr %x (%s)\n",
			   type, frame.tf_spc, frame.tf_ssr, p->p_comm);
		trapsignal(p, SIGBUS, type &~ T_USER);
		goto out;

	case T_INVALIDISN|T_USER:	/* invalid instruction fault */
		trapsignal(p, SIGILL, type &~ T_USER);
		goto out;

	case T_ASTFLT :
		printf("AST fault\n");
	        return;

	case T_ASTFLT|T_USER:		/* Allow process switch */
	/* printf("ASTU fault\n"); */
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		goto out;
#ifdef	TODO
	case T_PAGEFLT:			/* allow page faults in kernel mode */
		if (p == 0)
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
		/* FALLTHROUGH */

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddrt_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss, v;

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
				rv = KERN_FAILURE;
				goto nogo;
			}
		}

		/* Fault the original page in. */
		rv = uvm_fault(map, va, 0, ftype);
		if (rv == 0) {
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;

			if (type == T_PAGEFLT)
				return;
			goto out;
		}

	nogo:
		if (type == T_PAGEFLT) {
			if (pcb->pcb_onfault != 0)
				goto copyfault;
			printf("uvm_fault(%p, 0x%lx, 0, %d) -> %x\n",
				   map, va, ftype, rv);
			goto we_re_toast;
		}
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(p, SIGKILL, T_PAGEFLT);
		} else
			trapsignal(p, SIGSEGV, T_PAGEFLT);
		break;
	}

#endif /* TODO */

	case T_USERBREAK|T_USER:		/* bpt instruction fault */
		trapsignal(p, SIGTRAP, type &~ T_USER);
		break;
	}

	if ((type & T_USER) == 0)
		return;
 out:
	userret(p, frame.tf_spc, sticks);
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
/*ARGSUSED*/
void
syscall(frame)
	struct trapframe *frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error, opc, nsys;
	size_t argsize;
	register_t code, args[8], rval[2], ocode;
	u_quad_t sticks;

	uvmexp.syscalls++;
#if 1
	if (KERNELMODE(frame->tf_r15))
#else
	if (!USERMODE(frame->tf_spc, frame->tf_ssr))
#endif
		panic("syscall");
	p = curproc;
	sticks = p->p_sticks;
	p->p_md.md_regs = frame;
	opc = frame->tf_spc;
	ocode = code = frame->tf_r0;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (caddr_t)frame->tf_r15;

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
	        code = frame->tf_r4;  /* fuword(params); */
		/* params += sizeof(int); */
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		if (callp != sysent)
			break;
		/* fuword(params + _QUAD_LOWWORD * sizeof(int)); */
#if _BYTE_ORDER == BIG_ENDIAN
		code = frame->tf_r5;
#else
		code = frame->tf_r4;
#endif
		/* params += sizeof(quad_t); */
		break;
	default:
		break;
	}
	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;
	argsize = callp->sy_argsize;

	if (ocode == SYS_syscall) {
		if (argsize) {
			args[0] = frame->tf_r5;
			args[1] = frame->tf_r6;
			args[2] = frame->tf_r7;
			if (argsize > 3 * sizeof(int)) {
				argsize -= 3 * sizeof(int);
				error = copyin(params, (caddr_t)&args[3],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}
	else if (ocode == SYS___syscall) {
		if (argsize) {
			args[0] = frame->tf_r6;
			args[1] = frame->tf_r7;
			if (argsize > 2 * sizeof(int)) {
				argsize -= 2 * sizeof(int);
				error = copyin(params, (caddr_t)&args[2],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	} else {
		if (argsize) {
			args[0] = frame->tf_r4;
			args[1] = frame->tf_r5;
			args[2] = frame->tf_r6;
			args[3] = frame->tf_r7;
			if (argsize > 4 * sizeof(int)) {
				argsize -= 4 * sizeof(int);
				error = copyin(params, (caddr_t)&args[4],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}

//#ifdef TRAP_DEBUG
#ifdef SYSCALL_DEBUG
	if (cpu_debug_mode)
		scdebug_call(p, code, args);
#endif
//#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, argsize, args);
#endif
	if (error)
		goto bad;
	rval[0] = 0;
	rval[1] = frame->tf_r1;
	error = (*callp->sy_call)(p, args, rval);
	switch (error) {
	case 0:
		/*
		 * Reinitialize proc pointer `p' as it may be different
		 * if this is a child returning from fork syscall.
		 */
		p = curproc;
		frame->tf_r0 = rval[0];
		frame->tf_r1 = rval[1];
		frame->tf_ssr |= PSL_TBIT;	/* T bit */

		break;
	case ERESTART:
		/* 2 = TRAPA instruction size */
		frame->tf_spc = opc - 2;

		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->tf_r0 = error;
		frame->tf_ssr &= ~PSL_TBIT;	/* T bit */

		break;
	}

//#ifdef TRAP_DEBUG
#ifdef SYSCALL_DEBUG
	if (cpu_debug_mode)
		scdebug_ret(p, code, error, rval);
#endif
//#endif
	userret(p, frame->tf_spc, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif
}

void
child_return(arg)
	void *arg;
{
	struct proc *p = arg;
	struct trapframe *tf = p->p_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_ssr |= PSL_TBIT; /* This indicates no error. */

	userret(p, tf->tf_spc, 0);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}

/*
 * Set TLB entry
 * This is called from tlb_miss exception handler.
 */
void
tlb_handler(p1, p2, p3, p4, frame)
	int p1, p2, p3, p4;	/* These four params are dummy */
	struct trapframe frame;
{
	vaddr_t va;
	int pde_index, sig;
	unsigned long *pde;
	unsigned long pte;
	unsigned long *pd_top;
	int pte_index;
	struct proc *p;
	struct vmspace *vm;
	vm_map_t map;
	int rv = 0;
	u_quad_t sticks = 0;
	vm_prot_t ftype;
	extern vm_map_t kernel_map;
	vaddr_t	va_save;
	unsigned long pteh_save;
	int exptype, user;
#ifdef RECURSE_TLB_HANDLER
	int reentrant = 0;
#endif

	uvmexp.traps++;

	va = (vaddr_t)SHREG_TEA;
	va = trunc_page(va);
	pde_index = pdei(va);
#ifdef SH4
	pd_top = (u_long *)(SH3_P1SEG_TO_P2SEG(SHREG_TTB));
	pde = (u_long *)((u_long)SH3_P1SEG_TO_P2SEG(pd_top[pde_index]));
#else
	pd_top = (u_long *)SHREG_TTB;
	pde = (u_long *)pd_top[pde_index];
#endif
	exptype = SHREG_EXPEVT;

#if 0 /* def SH4 */
	if (((u_long)pde & PG_V) != 0 && 
	    (incpuswitch || exptype != T_TLBPRIVW)) {
#else
	if (((u_long)pde & PG_V) != 0 && exptype != T_TLBPRIVW) {
#endif
		(u_long)pde &= ~PGOFSET;
		pte_index = ptei(va);
#ifdef SH4
		pte = SH3_P1SEG_TO_P2SEG(pde[pte_index]);
#else
		pte = pde[pte_index];
#endif

		if ((pte & PG_V) != 0) {
#ifdef	DEBUG_TLB
			if (trapdebug)
				printf("tlb_handler:va(0x%lx),pte(0x%lx)\n",
				       va, pte);
#endif

#ifdef SH4_PCMCIA
#define PTEL_VALIDBITS 0x1ffff17e
#else
#define PTEL_VALIDBITS 0x1ffffd7e
#endif
#ifdef SH4
#ifdef SH4_PCMCIA
			if (pte & PG_PCMCIA) {
				int pcmtype;
				unsigned long ptea = 0;

				pcmtype = pte & PG_PCMCIA;

				/* printf("pcmtype = %lx,pte=%lx\n",
				       pcmtype,pte); */

				if (pcmtype == PG_PCMCIA_IO) {
					ptea = 0x03;
				} else if (pcmtype == PG_PCMCIA_MEM) {
					ptea = 0x05; 
					/* ptea = 0x03; */
				} else if (pcmtype == PG_PCMCIA_ATT) {
					ptea = 0x07;
				}

				SHREG_PTEL = (pte & PTEL_VALIDBITS)
					& ~PG_N;

				SHREG_PTEA = ptea;
			} else
#endif
			{
				if ( /*1 ||*/ (va >= SH3_P1SEG_BASE)) {
					SHREG_PTEL = (pte & PTEL_VALIDBITS)
						| PG_WT;
				} else {
					SHREG_PTEL = pte & PTEL_VALIDBITS;
				}

				SHREG_PTEA = 0;
			}
#else
			SHREG_PTEL = pte & PTEL_VALIDBITS;
#endif

			return;
		}
	}
#ifdef TRAP_DEBUG
	if (trapdebug)
		printf("tlb_handler#:va(0x%lx),curproc(%p)\n", va, curproc);
#endif

	user = !KERNELMODE(frame.tf_r15);

	pteh_save = SHREG_PTEH;
	va_save = va;
	p = curproc;
	if (p == NULL) {
		rv = KERN_FAILURE;
		goto dopanic;
	} else {
		if (user) {
			sticks = p->p_sticks;
			p->p_md.md_regs = &frame;
		} else
			sticks = 0;
	}

	/*
	 * It is only a kernel address space fault iff:
	 *	1. !user and
	 *	2. pcb_onfault not set or
	 *	3. pcb_onfault set but supervisor space data fault
	 * The last can occur during an exec() copyin where the
	 * argument space is lazy-allocated.
	 */
	if (user == 0 && (va >= VM_MIN_KERNEL_ADDRESS ||
	    p->p_addr->u_pcb.pcb_onfault == 0))
		map = kernel_map;
	else {
		vm = p->p_vmspace;
		map = &vm->vm_map;
	}

	if (exptype == T_TLBMISSW || exptype == T_TLBPRIVW)
		ftype = VM_PROT_WRITE;
	else
		ftype = VM_PROT_READ;

#ifdef RECURSE_TLB_HANDLER
	if (((PSL_BL | PSL_IMASK) & frame.tf_ssr) == 0)
		reentrant = 1;
#endif
	/* Fault the original page in. */
#ifdef RECURSE_TLB_HANDLER
	if (reentrant)
		enable_ext_intr();
#endif
	rv = uvm_fault(map, va, 0, ftype);
#ifdef RECURSE_TLB_HANDLER
	if (reentrant)
		disable_ext_intr();
#endif

	/*
	 * If this was a stack access, we keep track of the
	 * maximum accessed stack size.  Also, if uvm_fault()
	 * gets a protection failure, it is due to accessing
	 * the stack region outside the current limit and
	 * we need to reflect that as an access error.
	 */
	if (map != kernel_map &&
#ifdef SH4
	    /* XXX What is this about?  --thorpej */
	    vm != NULL &&
#endif
	    va >= (vaddr_t) vm->vm_maxsaddr &&
	    va < USRSTACK) {
		if (rv == 0) {
			u_int nss;

			nss = btoc(USRSTACK - va);
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
		} else if (rv == EACCES)
			rv = EFAULT;
	}

	if (rv == 0) {
		va = va_save;
		SHREG_PTEH = pteh_save;
		pde_index = pdei(va);
		pd_top = (u_long *)SHREG_TTB;
#ifdef SH4
		pde = (u_long *)
			((u_long)SH3_P1SEG_TO_P2SEG(pd_top[pde_index]));
#else
		pde = (u_long *)pd_top[pde_index];
#endif

		if (((u_long)pde & PG_V) != 0) {
			(u_long)pde &= ~PGOFSET;
			pte_index = ptei(va);
			pte = pde[pte_index];

			if ((pte & PG_V) != 0) {
#ifdef TRAP_DEBUG
				if (trapdebug)
					printf("tlb_handler#:va(0x%lx),pte(0x%lx)\n", va, pte);
#endif

#ifdef SH4
#ifdef SH4_PCMCIA
				if (pte & PG_PCMCIA) {
					int pcmtype;
					unsigned long ptea = 0;

					pcmtype = pte & PG_PCMCIA;

					/* printf("pcmtype = %lx,pte=%lx\n",
					       pcmtype,pte); */

					if (pcmtype == PG_PCMCIA_IO) {
						ptea = 0x03;
					} else if (pcmtype == PG_PCMCIA_MEM) {
						ptea = 0x05; 
						/*ptea = 0x03; */
					} else if (pcmtype == PG_PCMCIA_ATT) {
						ptea = 0x07;
					}

					SHREG_PTEL = (pte & PTEL_VALIDBITS)
						& ~PG_N;

					SHREG_PTEA = ptea;
				} else
#endif
				{
					if ( /*1 ||*/ (va >= SH3_P1SEG_BASE)) {
						SHREG_PTEL = 
							(pte & PTEL_VALIDBITS)
								| PG_WT;
					} else {
						SHREG_PTEL = 
							(pte & PTEL_VALIDBITS);
					}
					SHREG_PTEA = 0;
				}
#else

				SHREG_PTEL = pte & PTEL_VALIDBITS;
#endif
			}
		}
		if (user)
			userret(p, frame.tf_spc, sticks);
		return;
	}

	if (user == 0) {
		/* Check for copyin/copyout fault. */
		if (p != NULL &&
		    p->p_addr->u_pcb.pcb_onfault != 0) {
			frame.tf_spc = (int) p->p_addr->u_pcb.pcb_onfault;
			return;
		}
		goto dopanic;
	}

	if (rv == ENOMEM) {
		printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
		       p->p_pid, p->p_comm,
		       p->p_cred && p->p_ucred ?
		       p->p_ucred->cr_uid : -1);
		sig = SIGKILL;
	} else
		sig = SIGSEGV;
#ifdef DEBUG
	if (trapdebug) {
		printf("tlb_handler: fatal user fault: va 0x%lx, spc 0x%x, "
		    "exptype 0x%x (%s)\n", va, frame.tf_spc, exptype,
		    p->p_comm);
	}
#endif
	trapsignal(p, sig, T_TLBINVALIDR);
	userret(p, frame.tf_spc, sticks);
	return;

 dopanic:
	printf("tlb_handler: va 0x%lx, spc 0x%x, exptype 0x%x (%s)\n",
	    va, frame.tf_spc, exptype, p != NULL ? p->p_comm : "no curproc");
#if defined(DDB)
	Debugger();
#else
	panic("tlb_handler");
#endif
}
