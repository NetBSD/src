/*	$NetBSD: trap.c,v 1.39 2002/03/24 18:04:41 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc. All rights reserved.
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
#include "opt_kgdb.h"
#include "opt_syscall_debug.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <sh3/cpu.h>
#include <sh3/exception.h>
#include <sh3/userret.h>
#include <sh3/trap.h>
#include <sh3/mmu.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif
#ifdef KGDB
#include <sys/kgdb.h>
#endif

const char *trap_type[] = {
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
const int trap_types = sizeof trap_type / sizeof trap_type[0];

int trapdebug = 1;

void ast(struct trapframe *);
void tlb_handler(u_int32_t, u_int32_t, struct trapframe *);
void trap(u_int32_t, u_int32_t, struct trapframe *);
void syscall(struct trapframe *);

/*
 * void trap(u_int32_t ssr, u_int32_t spc, struct trapframe *tf):
 *	ssr ... SR when exception occur.
 *	spc ... PC when exception occur.
 *	tf  ... full user context.
 */
void
trap(u_int32_t ssr, u_int32_t spc, struct trapframe *tf)
{
	register struct proc *p = curproc;
	int type = tf->tf_trapno;
	struct pcb *pcb = NULL;
	vaddr_t va;

	if (p == NULL)
		goto we_re_toast;
	uvmexp.traps++;

#ifdef TRAPDEBUG
	if (trapdebug > 1) {
		int expevt = tf->tf_trapno;
		printf("%s: ", curproc->p_comm);
		if ((expevt >> 5) < trap_types)
			printf("%s, ", trap_type[expevt >> 5]);
		else
			printf("EXPEVT 0x%03x, ", expevt);
		printf("spc 0x%x, ssr 0x%x, frame=%p\n",
		    tf->tf_spc, tf->tf_ssr, tf);
	}
#endif /* TRAPDEBUG */

	if (!KERNELMODE(tf->tf_ssr)) {
		type |= T_USER;
		p->p_md.md_regs = tf;
	}

	switch (type) {

	default:
	we_re_toast:
#ifdef DDB
		if (kdb_trap(type, 0, tf))
			return;
#endif
#ifdef KGDB
		if (kgdb_trap(T_USERBREAK, tf))
			return;
#endif
		if (tf->tf_trapno >> 5 < trap_types)
			printf("fatal %s", trap_type[tf->tf_trapno >> 5]);
		else
			printf("unknown trap %x", tf->tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %x spc %x ssr %x \n",
			   type, tf->tf_spc, tf->tf_ssr);

		panic("trap");
		/*NOTREACHED*/

	case T_TRAP|T_USER:
		if (_reg_read_4(SH_(TRA)) == (0x000000c3 << 2)) {
			trapsignal(p, SIGTRAP, type &~ T_USER);
			break;
		} else {
			syscall(tf);
			return;
		}

	case T_INITPAGEWR:
	case T_INITPAGEWR|T_USER:
		va = (vaddr_t)SH_TEA;
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
			printf("copyin/copyout fault\n");
			tf->tf_spc = (int)pcb->pcb_onfault;
			return;
		}
		goto we_re_toast; /* XXX notyet */

	case T_ADDRESSERRR|T_USER:		/* protection fault */
	case T_ADDRESSERRW|T_USER:
	case T_INVALIDSLOT|T_USER:
		printf("trap type %x spc %x ssr %x (%s)\n",
		    type, tf->tf_spc, tf->tf_ssr, p->p_comm);
		trapsignal(p, SIGBUS, type &~ T_USER);
		goto out;

	case T_INVALIDISN|T_USER:	/* invalid instruction fault */
		trapsignal(p, SIGILL, type &~ T_USER);
		goto out;

	case T_USERBREAK|T_USER:		/* bpt instruction fault */
		trapsignal(p, SIGTRAP, type &~ T_USER);
		break;
	}

	if ((type & T_USER) == 0)
		return;
 out:
	userret(p);
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
syscall(struct trapframe *frame)
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error, opc, nsys;
	size_t argsize;
	register_t code, args[8], rval[2], ocode;

	uvmexp.syscalls++;
	if (KERNELMODE(frame->tf_ssr))
		panic("syscall");
	p = curproc;
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

#ifdef SYSCALL_DEBUG
	if (trapdebug > 1)
		scdebug_call(p, code, args);
#endif
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

#ifdef SYSCALL_DEBUG
	if (trapdebug > 1)
		scdebug_ret(p, code, error, rval);
#endif
	userret(p);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif
}

void
child_return(void *arg)
{
	struct proc *p = arg;
	struct trapframe *tf = p->p_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_ssr |= PSL_TBIT; /* This indicates no error. */

	userret(p);
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
tlb_handler(u_int32_t ssr, u_int32_t spc, struct trapframe *frame)
{
	int pde_index, sig;
	unsigned long *pde;
	unsigned long pte;
	unsigned long *pd_top;
	int pte_index;
	struct proc *p;
	struct vmspace *vm;
	struct vm_map *map;
	int rv = 0;
	vm_prot_t ftype;
	extern struct vm_map *kernel_map;
	vaddr_t	va_save;
	unsigned long pteh_save;
	int exptype, user;
#ifdef RECURSE_TLB_HANDLER
	int reentrant = 0;
#endif
	vaddr_t va;

	uvmexp.traps++;

	va = (vaddr_t)SH_TEA;
	pde_index = pdei(va);
	pd_top = SH_MMU_PD_TOP();
	pde = SH_MMU_PDE(pd_top, pde_index);
	exptype = _reg_read_4(SH_(EXPEVT));

	if (((u_long)pde & PG_V) != 0 && exptype != T_TLBPRIVW) {
		(u_long)pde &= ~PGOFSET;	/* zero attribute bits */
		pte_index = ptei(va);
		pte = (u_int32_t)SH_MMU_PDE(pde, pte_index);
		if ((pte & PG_V) != 0) {
			SH_MMU_PTE_SETUP(va, pte);
			__asm __volatile ("ldtlb; nop");
			return;
		}
	}
#ifdef TRAPDEBUG
	if (trapdebug > 1) {
		printf("tlb_handler: 0x%08lx 0x%03x (%s) %s pagedir=%p "
		    "spc=0x%08x\n", va, exptype,
		    curproc ? curproc->p_comm : "idle",
		    KERNELMODE(frame->tf_ssr) ? "kernel" : "user", pd_top, spc);
	}
#endif

	user = !KERNELMODE(ssr);

	pteh_save = SH_PTEH;
	va_save = va;
	p = curproc;
	if (p == NULL) {
		printf("no curproc\n");
		goto dopanic;
	} else {
		if (user)
			p->p_md.md_regs = frame;
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
	if (((PSL_BL | PSL_IMASK) & ssr) == 0)
		reentrant = 1;
#endif
	/* Fault the original page in. */
#ifdef RECURSE_TLB_HANDLER
	if (reentrant)
		_cpu_intr_resume(0);
#endif

	rv = uvm_fault(map, va, 0, ftype);
#ifdef RECURSE_TLB_HANDLER
	if (reentrant)
		_cpu_intr_suspend();
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
		SH_PTEH = pteh_save;
		pde_index = pdei(va);
		pd_top = SH_MMU_PD_TOP();
		pde = SH_MMU_PDE(pd_top, pde_index);

		if (((u_long)pde & PG_V) != 0) {
			(u_long)pde &= ~PGOFSET;
			pte_index = ptei(va);
			pte = pde[pte_index];
			if ((pte & PG_V) != 0)
				SH_MMU_PTE_SETUP(va, pte);
		}
		__asm __volatile("ldtlb; nop");

		if (user)
			userret(p);
		return;
	}

	if (user == 0) {
		/* Check for copyin/copyout fault. */
		if (p != NULL && p->p_addr->u_pcb.pcb_onfault != 0) {
			frame->tf_spc = (int) p->p_addr->u_pcb.pcb_onfault;
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
		    "exptype 0x%x (%s)\n", va, spc, exptype,
		    p->p_comm);
	}
#endif
	trapsignal(p, sig, T_TLBINVALIDR);
	userret(p);
	return;

 dopanic:
	printf("tlb_handler: va 0x%lx, spc 0x%x, exptype 0x%x (%s)\n",
	    va, spc, exptype, p != NULL ? p->p_comm : "no curproc");
#if defined(DDB)
	Debugger();
#else
	panic("tlb_handler");
#endif
}

void
ast(struct trapframe *tf)
{
	struct proc *p = curproc;
	int sig;

	if (KERNELMODE(tf->tf_ssr))
		return;

	while (p->p_md.md_astpending) {
		uvmexp.softs++;
		p->p_md.md_astpending = 0;

		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}

		/* Take pending signals. */
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);

		if (want_resched) {
			/*
			 * We are being preempted.
			 */
			preempt(NULL);
		}

		userret(p);
	}
}
