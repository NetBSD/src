/*	$NetBSD: exception.c,v 1.6 2003/03/09 16:14:29 tsutsui Exp $	*/

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

/*
 * SH3 Trap and System call handling
 *
 * T.Horiuchi 1998.06.8
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_syscall_debug.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/sa.h>
#include <sys/savar.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#ifdef DDB
#include <sh3/db_machdep.h>
#endif
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

#include <sh3/cpu.h>
#include <sh3/mmu.h>
#include <sh3/exception.h>
#include <sh3/userret.h>

const char *exp_type[] = {
	"--",					/* 0x000 (reset vector) */
	"--",					/* 0x020 (reset vector) */
	"TLB miss/invalid (load)",		/* 0x040 EXPEVT_TLB_MISS_LD */
	"TLB miss/invalid (store)",		/* 0x060 EXPEVT_TLB_MISS_ST */
	"initial page write",			/* 0x080 EXPEVT_TLB_MOD */
	"TLB protection violation (load)",	/* 0x0a0 EXPEVT_TLB_PROT_LD */
	"TLB protection violation (store)",	/* 0x0c0 EXPEVT_TLB_PROT_ST */
	"address error (load)",			/* 0x0e0 EXPEVT_ADDR_ERR_LD */
	"address error (store)",		/* 0x100 EXPEVT_ADDR_ERR_ST */
	"FPU",					/* 0x120 EXPEVT_FPU */
	"--",					/* 0x140 (reset vector) */
	"unconditional trap (TRAPA)",		/* 0x160 EXPEVT_TRAPA */
	"reserved instruction code exception",	/* 0x180 EXPEVT_RES_INST */
	"illegal slot instruction exception",	/* 0x1a0 EXPEVT_SLOT_INST */
	"--",					/* 0x1c0 (external interrupt) */
	"user break point trap",		/* 0x1e0 EXPEVT_BREAK */
};
const int exp_types = sizeof exp_type / sizeof exp_type[0];

void general_exception(struct lwp *, struct trapframe *);
void tlb_exception(struct lwp *, struct trapframe *, u_int32_t);
void syscall(struct lwp *, struct trapframe *);
void ast(struct lwp *, struct trapframe *);

/*
 * void general_exception(struct lwp *l, struct trapframe *tf):
 *	l  ... curlwp when exception occur.
 *	tf ... full user context.
 */
void
general_exception(struct lwp *l, struct trapframe *tf)
{
	int expevt = tf->tf_expevt;
	boolean_t usermode = !KERNELMODE(tf->tf_ssr);

	uvmexp.traps++;

	if (l == NULL)
 		goto do_panic;

	if (usermode) {
		KDASSERT(l->l_md.md_regs == tf); /* check exception depth */
		expevt |= EXP_USER;
	}

	switch (expevt) {
	case EXPEVT_TRAPA | EXP_USER:
		/* Check for debugger break */
		if (_reg_read_4(SH_(TRA)) == (_SH_TRA_BREAK << 2)) {
			trapsignal(l, SIGTRAP, tf->tf_expevt);
		} else {
			syscall(l, tf);
			return;
		}
		break;
	case EXPEVT_ADDR_ERR_LD:
		/*FALLTHROUGH*/
	case EXPEVT_ADDR_ERR_ST:
		KDASSERT(l->l_md.md_pcb->pcb_onfault != NULL);
		tf->tf_spc = (int)l->l_md.md_pcb->pcb_onfault;
		if (tf->tf_spc == NULL)
			goto do_panic;
		break;

	case EXPEVT_ADDR_ERR_LD | EXP_USER:
		/*FALLTHROUGH*/
	case EXPEVT_ADDR_ERR_ST | EXP_USER:
		trapsignal(l, SIGSEGV, tf->tf_expevt);
		break;

	case EXPEVT_RES_INST | EXP_USER:
		/*FALLTHROUGH*/
	case EXPEVT_SLOT_INST | EXP_USER:
		trapsignal(l, SIGILL, tf->tf_expevt);
		break;

	case EXPEVT_BREAK | EXP_USER:
		trapsignal(l, SIGTRAP, tf->tf_expevt);
		break;
	default:
		goto do_panic;
	}

	if (usermode)
		userret(l);
	return;

 do_panic:
#ifdef DDB
	if (kdb_trap(expevt, 0, tf))
		return;
#endif
#ifdef KGDB
	if (kgdb_trap(EXPEVT_BREAK, tf))
		return;
#endif
	if (expevt >> 5 < exp_types)
		printf("fatal %s", exp_type[expevt >> 5]);
	else
		printf("EXPEVT 0x%03x", expevt);
	printf(" in %s mode\n", expevt & EXP_USER ? "user" : "kernel");
	printf(" spc %x ssr %x \n", tf->tf_spc, tf->tf_ssr);

	panic("general_exception");
	while (/*CONSTCOND*/1)
			;
	/*NOTREACHED*/
}

/*
 * void syscall(struct lwp *l, struct trapframe *tf):
 *	l  ... curlwp when exception occur.
 *	tf ... full user context.
 *	System call request from POSIX system call gate interface to kernel.
 */
void
syscall(struct lwp *l, struct trapframe *tf)
{
	struct proc *p = l->l_proc;
	caddr_t params;
	const struct sysent *callp;
	int error, opc, nsys;
	size_t argsize;
	register_t code, args[8], rval[2], ocode;

	uvmexp.syscalls++;

	opc = tf->tf_spc;
	ocode = code = tf->tf_r0;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (caddr_t)tf->tf_r15;

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
	        code = tf->tf_r4;  /* fuword(params); */
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
		code = tf->tf_r5;
#else
		code = tf->tf_r4;
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
			args[0] = tf->tf_r5;
			args[1] = tf->tf_r6;
			args[2] = tf->tf_r7;
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
			args[0] = tf->tf_r6;
			args[1] = tf->tf_r7;
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
			args[0] = tf->tf_r4;
			args[1] = tf->tf_r5;
			args[2] = tf->tf_r6;
			args[3] = tf->tf_r7;
			if (argsize > 4 * sizeof(int)) {
				argsize -= 4 * sizeof(int);
				error = copyin(params, (caddr_t)&args[4],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}

	if (error)
		goto bad;

	if ((error = trace_enter(l, code, code, NULL, args, rval)) != 0)
		goto bad;

	rval[0] = 0;
	rval[1] = tf->tf_r1;
	error = (*callp->sy_call)(l, args, rval);
	switch (error) {
	case 0:
		tf->tf_r0 = rval[0];
		tf->tf_r1 = rval[1];
		tf->tf_ssr |= PSL_TBIT;	/* T bit */

		break;
	case ERESTART:
		/* 2 = TRAPA instruction size */
		tf->tf_spc = opc - 2;

		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_r0 = error;
		tf->tf_ssr &= ~PSL_TBIT;	/* T bit */

		break;
	}


	trace_exit(l, code, args, rval, error);

	userret(l);
}

/*
 * void tlb_exception(struct lwp *l, struct trapframe *tf, u_int32_t va):
 *	l  ... curlwp when exception occur.
 *	tf ... full user context.
 *	va ... fault address.
 */
void
tlb_exception(struct lwp *l, struct trapframe *tf, u_int32_t va)
{
#define	TLB_ASSERT(assert, msg)						\
do {									\
	if (!(assert)) {						\
		panic_msg =  msg;					\
		goto tlb_panic;						\
	}								\
} while(/*CONSTCOND*/0)
	struct vm_map *map;
	pmap_t pmap;
	boolean_t usermode;
	int err, track, ftype;
	char *panic_msg;

	usermode = !KERNELMODE(tf->tf_ssr);
	if (usermode) {
		KDASSERT(l->l_md.md_regs == tf);
	} else {
		KDASSERT(l == NULL ||		/* idle */
		    l == &lwp0 ||		/* kthread */
		    l->l_md.md_regs != tf);	/* other */
	}

	switch (tf->tf_expevt) {
	case EXPEVT_TLB_MISS_LD:
		track = PVH_REFERENCED;
		ftype = VM_PROT_READ;
		break;
	case EXPEVT_TLB_MISS_ST:
		track = PVH_REFERENCED;
		ftype = VM_PROT_WRITE;
		break;
	case EXPEVT_TLB_MOD:
		track = PVH_REFERENCED | PVH_MODIFIED;
		ftype = VM_PROT_WRITE;
		break;
	case EXPEVT_TLB_PROT_LD:
		TLB_ASSERT((int)va > 0,
		    "kernel virtual protection fault (load)");
		if (usermode) {
			trapsignal(l, SIGSEGV, tf->tf_expevt);
			goto user_fault;
		} else {
			TLB_ASSERT(l && l->l_md.md_pcb->pcb_onfault != NULL,
			    "no copyin/out fault handler (load protection)");
			tf->tf_spc = (int)l->l_md.md_pcb->pcb_onfault;
		}
		return;

	case EXPEVT_TLB_PROT_ST:
		track = 0;	/* call uvm_fault first. (COW) */
		ftype = VM_PROT_WRITE;
		break;
	}

	/* Select address space */
	if (usermode) {
		TLB_ASSERT(l != NULL, "no curlwp");
		map = &l->l_proc->p_vmspace->vm_map;
		pmap = map->pmap;
	} else {
		if ((int)va < 0) {
			map = kernel_map;
			pmap = pmap_kernel();
		} else {
			TLB_ASSERT(l != NULL &&
			    l->l_md.md_pcb->pcb_onfault != NULL,
			    "invalid user-space access from kernel mode");
			if (va == 0) {
				tf->tf_spc = (int)l->l_md.md_pcb->pcb_onfault;
				return;
			}
			map = &l->l_proc->p_vmspace->vm_map;
			pmap = map->pmap;
		}
	}

	/* Lookup page table. if entry found, load it. */
	if (track && __pmap_pte_load(pmap, va, track)) {
		if (usermode)
			userret(l);
		return;
	}

	/* Page not found. call fault handler */
	if (!usermode && pmap != pmap_kernel() &&
	    l->l_md.md_pcb->pcb_faultbail) {
		TLB_ASSERT(l->l_md.md_pcb->pcb_onfault != NULL,
		    "no copyin/out fault handler (interrupt context)");
		tf->tf_spc = (int)l->l_md.md_pcb->pcb_onfault;
		return;
	}

	err = uvm_fault(map, va, 0, ftype);

	/* User stack extension */
	if (map != kernel_map &&
	    (va >= (vaddr_t)l->l_proc->p_vmspace->vm_maxsaddr) &&
	    (va < USRSTACK)) {
		if (err == 0) {
			struct vmspace *vm = l->l_proc->p_vmspace;
			u_int32_t nss;
			nss = btoc(USRSTACK - va);
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
		} else if (err == EACCES) {
			err = EFAULT;
		}
	}

	/* Page in. load PTE to TLB. */
	if (err == 0) {
		boolean_t loaded = __pmap_pte_load(pmap, va, track);
		TLB_ASSERT(loaded, "page table entry not found");
		if (usermode)
			userret(l);
		return;
	}

	/* Page not found. */
	if (usermode) {
		trapsignal(l, err == ENOMEM ? SIGKILL : SIGSEGV, tf->tf_expevt);
		goto user_fault;
	} else {
		TLB_ASSERT(l->l_md.md_pcb->pcb_onfault,
		    "no copyin/out fault handler (page not found)");
		tf->tf_spc = (int)l->l_md.md_pcb->pcb_onfault;
	}
	return;

 user_fault:
	userret(l);
	ast(l, tf);
	return;

 tlb_panic:
	panic("tlb_handler: %s va=0x%08x, ssr=0x%08x, spc=0x%08x"
	    "  lwp=%p onfault=%p", panic_msg, va, tf->tf_ssr, tf->tf_spc,
	    l, l ? l->l_md.md_pcb->pcb_onfault : 0);
#undef	TLB_ASSERT
}

/*
 * void ast(struct lwp *l, struct trapframe *tf):
 *	l  ... curlwp when exception occur.
 *	tf ... full user context.
 *	This is called when exception return. if return from kernel to user,
 *	handle asynchronous software traps and context switch if needed.
 */
void
ast(struct lwp *l, struct trapframe *tf)
{
	struct proc *p;

	if (KERNELMODE(tf->tf_ssr))
		return;
	KDASSERT(l != NULL);
	KDASSERT(l->l_md.md_regs == tf);

	p = l->l_proc;

	while (p->p_md.md_astpending) {
		uvmexp.softs++;
		p->p_md.md_astpending = 0;

		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}

		if (want_resched) {
			/* We are being preempted. */
			preempt(0);
		}

		userret(l);
	}
}

/*
 * void child_return(void *arg):
 *
 *	uvm_fork sets this routine to proc_trampoline's service function.
 *	when return from here, jump to user-land.
 */
void
child_return(void *arg)
{
	struct lwp *l = arg;
#ifdef KTRACE
	struct proc *p = l->l_proc;
#endif
	struct trapframe *tf = l->l_md.md_regs;

	tf->tf_r0 = 0;
	tf->tf_ssr |= PSL_TBIT; /* This indicates no error. */

	userret(l);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}

/*
 * void startlwp(void *arg):
 *
 *	Start a new LWP.
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;
	int error;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#ifdef DIAGNOSTIC
	if (error)
		printf("startlwp: error %d from cpu_setmcontext()", error);
#endif
	pool_put(&lwp_uc_pool, uc);

	userret(l);
}

/*
 * void upcallret(struct lwp *l):
 *
 *	Perform userret() for an LWP.
 *	XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{

	userret(l);
}
