/*	$NetBSD: trap.c,v 1.2.6.5 2002/06/20 03:40:31 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/user.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/lwp.h>
#include <sys/pool.h>
#include <sys/sa.h>
#include <sys/savar.h>
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/pmap.h>
#include <powerpc/ibm4xx/tlb.h>
#include <powerpc/fpu/fpu_extern.h>

/* These definitions should probably be somewhere else			XXX */
#define	FIRSTARG	3		/* first argument is in reg 3 */
#define	NARGREG		8		/* 8 args are in registers */
#define	MOREARGS(sp)	((caddr_t)((int)(sp) + 8)) /* more args go here */

#ifndef MULTIPROCESSOR
volatile int astpending;
volatile int want_resched;
#endif

void *syscall = NULL;	/* XXX dummy symbol for emul_netbsd */

static int fix_unaligned __P((struct lwp *p, struct trapframe *frame));

void trap __P((struct trapframe *));	/* Called from locore / trap_subr */
int setfault __P((faultbuf));	/* defined in locore.S */
/* Why are these not defined in a header? */
int badaddr __P((void *, size_t));
int badaddr_read __P((void *, size_t, int *));
int ctx_setup __P((int, int));

#ifdef DEBUG
#define TDB_ALL	0x1
int trapdebug = /* TDB_ALL */ 0;
#define	DBPRINTF(x, y)	if (trapdebug & (x)) printf y
#else
#define DBPRINTF(x, y)
#endif

void
trap(struct trapframe *frame)
{
	struct lwp *l = curproc;
	struct proc *p = l ? l->l_proc : NULL;
	int type = frame->exc;
	int ftype, rv;

	KASSERT(l == 0 || (l->l_stat == LSONPROC));

	if (frame->srr1 & PSL_PR)
		type |= EXC_USER;

	ftype = VM_PROT_READ;

DBPRINTF(TDB_ALL, ("trap(%x) at %x from frame %p &frame %p\n", 
	type, frame->srr0, frame, &frame));

	switch (type) {
	case EXC_DEBUG|EXC_USER:
{
	int srr2, srr3;
__asm __volatile("mfspr %0,0x3f0" : "=r" (rv), "=r" (srr2), "=r" (srr3) :);
printf("debug reg is %x srr2 %x srr3 %x\n", rv, srr2, srr3);
}
		/*
		 * DEBUG intr -- probably single-step.
		 */
	case EXC_TRC|EXC_USER:
		KERNEL_PROC_LOCK(l);
		frame->srr1 &= ~PSL_SE;
		trapsignal(l, SIGTRAP, EXC_TRC);
		KERNEL_PROC_UNLOCK(l);
		break;
		
	  /* If we could not find and install appropriate TLB entry, fall through */
	  
	case EXC_DSI:
		/* FALLTHROUGH */
	case EXC_DTMISS:
		{
			struct vm_map *map;
			vaddr_t va;
			faultbuf *fb = NULL;

			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			va = frame->dear;
			if (frame->pid == KERNEL_PID) {
				map = kernel_map;
			} else {
				map = &p->p_vmspace->vm_map;
			}

			if (frame->esr & (ESR_DST|ESR_DIZ))
				ftype = VM_PROT_WRITE;

DBPRINTF(TDB_ALL, ("trap(EXC_DSI) at %x %s fault on %p esr %x\n", 
frame->srr0, (ftype&VM_PROT_WRITE) ? "write" : "read", (void *)va, frame->esr));
			rv = uvm_fault(map, trunc_page(va), 0, ftype);
			KERNEL_UNLOCK();
			if (rv == 0)
				goto done;
			if ((fb = l->l_addr->u_pcb.pcb_onfault) != NULL) {
				frame->pid = KERNEL_PID;
				frame->srr0 = (*fb)[0];
				frame->srr1 |= PSL_IR; /* Re-enable IMMU */
				frame->fixreg[1] = (*fb)[1];
				frame->fixreg[2] = (*fb)[2];
				frame->fixreg[3] = 1; /* Return TRUE */
				frame->cr = (*fb)[3];
				memcpy(&frame->fixreg[13], &(*fb)[4],
				      19 * sizeof(register_t));
				goto done;
			}
		}
		goto brain_damage;
		
	case EXC_DSI|EXC_USER:
		/* FALLTHROUGH */
	case EXC_DTMISS|EXC_USER:
		KERNEL_PROC_LOCK(l);
			
		if (frame->esr & (ESR_DST|ESR_DIZ))
			ftype = VM_PROT_WRITE;

DBPRINTF(TDB_ALL, ("trap(EXC_DSI|EXC_USER) at %x %s fault on %x %x\n", 
frame->srr0, (ftype&VM_PROT_WRITE) ? "write" : "read", frame->dear, frame->esr));
KASSERT(l == curproc && (l->l_stat == LSONPROC));
		rv = uvm_fault(&p->p_vmspace->vm_map,
			       trunc_page(frame->dear), 0, ftype);
		if (rv == 0) {
		  KERNEL_PROC_UNLOCK(l);
		  break;
		}
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s) lid %d, uid %d killed: "
			       "out of swap\n",
			       p->p_pid, p->p_comm, l->l_lid,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
			trapsignal(l, SIGKILL, EXC_DSI);
		} else {
			trapsignal(l, SIGSEGV, EXC_DSI);
		}
		KERNEL_PROC_UNLOCK(l);
		break;
	case EXC_ITMISS|EXC_USER:
	case EXC_ISI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		ftype = VM_PROT_READ | VM_PROT_EXECUTE;
DBPRINTF(TDB_ALL, ("trap(EXC_ISI|EXC_USER) at %x %s fault on %x tf %p\n", 
frame->srr0, (ftype&VM_PROT_WRITE) ? "write" : "read", frame->srr0, frame));
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(frame->srr0), 0, ftype);
		if (rv == 0) {
		  KERNEL_PROC_UNLOCK(l);
		  break;
		}
		trapsignal(l, SIGSEGV, EXC_ISI);
		KERNEL_PROC_UNLOCK(l);
		break;
	case EXC_SC|EXC_USER:
		{
			const struct sysent *callp;
			size_t argsize;
			register_t code, error;
			register_t *params, rval[2];
			int n;
			register_t args[10];

			KERNEL_PROC_LOCK(l);

			uvmexp.syscalls++;

			code = frame->fixreg[0];
			callp = p->p_emul->e_sysent;
			params = frame->fixreg + FIRSTARG;
			n = NARGREG;

			switch (code) {
			case SYS_syscall:
				/*
				 * code is first argument,
				 * followed by actual args.
				 */
				code = *params++;
				n -= 1;
				break;
			case SYS___syscall:
				params++;
				code = *params++;
				n -= 2;
				break;
			default:
				break;
			}

			code &= (SYS_NSYSENT - 1);
			callp += code;
			argsize = callp->sy_argsize;

			if (argsize > n * sizeof(register_t)) {
				memcpy(args, params, n * sizeof(register_t));
				error = copyin(MOREARGS(frame->fixreg[1]),
					       args + n,
					       argsize - n * sizeof(register_t));
				if (error)
					goto syscall_bad;
				params = args;
			}


			if ((error = trace_enter(p, code, params, rval)) != 0)
				goto syscall_bad;

			rval[0] = 0;
			rval[1] = 0;

			error = (*callp->sy_call)(l, params, rval);
			switch (error) {
			case 0:
				frame->fixreg[FIRSTARG] = rval[0];
				frame->fixreg[FIRSTARG + 1] = rval[1];
				frame->cr &= ~0x10000000;
				break;
			case ERESTART:
				/*
				 * Set user's pc back to redo the system call.
				 */
				frame->srr0 -= 4;
				break;
			case EJUSTRETURN:
				/* nothing to do */
				break;
			default:
syscall_bad:
				if (p->p_emul->e_errno)
					error = p->p_emul->e_errno[error];
				frame->fixreg[FIRSTARG] = error;
				frame->cr |= 0x10000000;
				break;
			}
			KERNEL_PROC_UNLOCK(p);

			trace_exit(p, code, args, rval, error);
		}
		break;

	case EXC_AST|EXC_USER:
		astpending = 0;		/* we are about to do it */
		KERNEL_PROC_LOCK(l);
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		/* Check whether we are being preempted. */
		if (want_resched)
			preempt(NULL);
		KERNEL_PROC_UNLOCK(l);
		break;


	case EXC_ALI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		if (fix_unaligned(l, frame) != 0)
			trapsignal(l, SIGBUS, EXC_ALI);
		else
			frame->srr0 += 4;
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_PGM|EXC_USER:
		/* 
		 * Illegal insn: 
		 *
		 * let's try to see if it's FPU and can be emulated. 
		 */
		uvmexp.traps ++;
		if (!(l->l_addr->u_pcb.pcb_flags & PCB_FPU)) {
			memset(&l->l_addr->u_pcb.pcb_fpu, 0,
				sizeof l->l_addr->u_pcb.pcb_fpu);
			l->l_addr->u_pcb.pcb_flags |= PCB_FPU;
		}

		if ((rv = fpu_emulate(frame, 
			(struct fpreg *)&l->l_addr->u_pcb.pcb_fpu))) {
			KERNEL_PROC_LOCK(l);
			trapsignal(l, rv, EXC_PGM);
			KERNEL_PROC_UNLOCK(l);
		}
		break;

	case EXC_MCHK:
		{
			faultbuf *fb;

			if ((fb = l->l_addr->u_pcb.pcb_onfault) != NULL) {
				frame->pid = KERNEL_PID;
				frame->srr0 = (*fb)[0];
				frame->srr1 |= PSL_IR; /* Re-enable IMMU */
				frame->fixreg[1] = (*fb)[1];
				frame->fixreg[2] = (*fb)[2];
				frame->fixreg[3] = 1; /* Return TRUE */
				frame->cr = (*fb)[3];
				memcpy(&frame->fixreg[13], &(*fb)[4],
				      19 * sizeof(register_t));
				goto done;
			}
		}
		goto brain_damage;
	default:
brain_damage:
		printf("trap type 0x%x at 0x%x\n", type, frame->srr0);
#ifdef DDB
		if (kdb_trap(type, frame))
			goto done;
#endif
#ifdef TRAP_PANICWAIT
		printf("Press a key to panic.\n");
		cngetc();
#endif
		panic("trap");
	}

	/* Take pending signals. */
	{
		int sig;

		while ((sig = CURSIG(l)) != 0)
			postsig(sig);
	}

	/* If our process is on the way out, die. */
	if (p->p_flag & P_WEXIT)
		lwp_exit(l);

	/* Invoke any pending upcalls */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
  done:
}

int
ctx_setup(int ctx, int srr1)
{
	volatile struct pmap *pm;

	/* Update PID if we're returning to user mode. */
	if (srr1 & PSL_PR) {
		pm = curproc->l_proc->p_vmspace->vm_map.pmap;
		if (!pm->pm_ctx) {
			ctx_alloc((struct pmap *)pm);
		}
		ctx = pm->pm_ctx;
		if (srr1 & PSL_SE) {
			int dbreg, mask = 0x48000000;
				/*
				 * Set the Internal Debug and
				 * Instruction Completion bits of
				 * the DBCR0 register.
				 *
				 * XXX this is also used by jtag debuggers...
				 */
			__asm __volatile("mfspr %0,0x3f2;"
				"or %0,%0,%1;"
				"mtspr 0x3f2,%0;" :
				"=&r" (dbreg) : "r" (mask));
		}
	}
	else if (!ctx) {
		ctx = KERNEL_PID;
	}
	return (ctx);
}

void
child_return(void *arg)
{
	struct lwp *l = arg;
	struct proc *p = l->l_proc;
	struct trapframe *tf = trapframe(l);

	KERNEL_PROC_UNLOCK(l);

	tf->fixreg[FIRSTARG] = 0;
	tf->fixreg[FIRSTARG + 1] = 1;
	tf->cr &= ~0x10000000;
	tf->srr1 &= ~(PSL_FP|PSL_VEC);	/* Disable FP & AltiVec, as we can't be them */
#ifdef	KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(p, SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
	/* Profiling?							XXX */
	curcpu()->ci_schedstate.spc_curpriority = l->l_priority;
}

/*
 * Used by copyin()/copyout()
 */
extern vaddr_t vmaprange __P((struct proc *, vaddr_t, vsize_t, int));
extern void vunmaprange __P((vaddr_t, vsize_t));
static int bigcopyin __P((const void *,	void *,	size_t ));
static int bigcopyout __P((const void *, void *, size_t ));

int
copyin(const void *udaddr, void *kaddr, size_t len)
{
	struct pmap *pm = curproc->l_proc->p_vmspace->vm_map.pmap;
	int msr, pid, tmp, ctx;
	faultbuf env;

	/* For bigger buffers use the faster copy */
	if (len > 256) return (bigcopyin(udaddr, kaddr, len));

	if (setfault(env)) {
		curpcb->pcb_onfault = 0;
		return EFAULT;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	asm volatile("addi %6,%6,1; mtctr %6;"	/* Set up counter */
		"mfmsr %0;"			/* Save MSR */
		"li %1,0x20; "
		"andc %1,%0,%1; mtmsr %1;"	/* Disable IMMU */
		"mfpid %1;"			/* Save old PID */
		"sync; isync;"

		"1: bdz 2f;"			/* while len */
		"mtpid %3; sync;"		/* Load user ctx */
		"lbz %2,0(%4); addi %4,%4,1;"	/* Load byte */
		"sync; isync;"
		"mtpid %1;sync;"
		"stb %2,0(%5); dcbf 0,%5; addi %5,%5,1;"	/* Store kernel byte */
		"sync; isync;"
		"b 1b;"				/* repeat */

		"2: mtpid %1; mtmsr %0;"	/* Restore PID and MSR */
		"sync; isync;"
		: "=&r" (msr), "=&r" (pid), "=&r" (tmp)
		: "r" (ctx), "r" (udaddr), "r" (kaddr), "r" (len));

	curpcb->pcb_onfault = 0;
	return 0;
}

static int
bigcopyin(const void *udaddr, void *kaddr, size_t len)
{
	const char *up;
	char *kp = kaddr;
	struct lwp *l = curproc;
	struct proc *p;
	int error;

	if (!l) {
		return EFAULT;
	}

	p = l->l_proc;

	/*
	 * Stolen from physio(): 
	 */
	PHOLD(l);
	error = uvm_vslock(p, (caddr_t)udaddr, len, VM_PROT_READ);
	if (error) {
		PRELE(l);
		return EFAULT;
	}
	up = (char *)vmaprange(p, (vaddr_t)udaddr, len, VM_PROT_READ);

	memcpy(kp, up, len);
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p, (caddr_t)udaddr, len);
	PRELE(l);

	return 0; 
}

int
copyout(const void *kaddr, void *udaddr, size_t len)
{
	struct pmap *pm = curproc->l_proc->p_vmspace->vm_map.pmap;
	int msr, pid, tmp, ctx;
	faultbuf env;

	/* For big copies use more efficient routine */
	if (len > 256) return (bigcopyout(kaddr, udaddr, len));

	if (setfault(env)) {
		curpcb->pcb_onfault = 0;
		return EFAULT;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	asm volatile("addi %6,%6,1; mtctr %6;"	/* Set up counter */
		"mfmsr %0;"			/* Save MSR */
		"li %1,0x20; "
		"andc %1,%0,%1; mtmsr %1;"	/* Disable IMMU */
		"mfpid %1;"			/* Save old PID */
		"sync; isync;"

		"1: bdz 2f;"			/* while len */
		"mtpid %1;sync;"
		"lbz %2,0(%5); addi %5,%5,1;"	/* Load kernel byte */
		"sync; isync;"
		"mtpid %3; sync;"		/* Load user ctx */
		"stb %2,0(%4);  dcbf 0,%4; addi %4,%4,1;"	/* Store user byte */
		"sync; isync;"
		"b 1b;"				/* repeat */

		"2: mtpid %1; mtmsr %0;"	/* Restore PID and MSR */
		"sync; isync;"
		: "=&r" (msr), "=&r" (pid), "=&r" (tmp)
		: "r" (ctx), "r" (udaddr), "r" (kaddr), "r" (len));

	curpcb->pcb_onfault = 0;
	return 0;
}

static int
bigcopyout(const void *kaddr, void *udaddr, size_t len)
{
	char *up;
	const char *kp = (char *)kaddr;
	struct lwp *l = curproc;
	struct proc *p;
	int error;

	if (!l) {
		return EFAULT;
	}

	p = l->l_proc;

	/*
	 * Stolen from physio(): 
	 */
	PHOLD(l);
	error = uvm_vslock(p, udaddr, len, VM_PROT_WRITE);
	if (error) {
		PRELE(l);
		return EFAULT;
	}
	up = (char *)vmaprange(p, (vaddr_t)udaddr, len, 
		VM_PROT_READ|VM_PROT_WRITE);

	memcpy(up, kp, len);
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p, udaddr, len);
	PRELE(l);

	return 0; 
}

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
int
kcopy(const void *src, void *dst, size_t len)
{
	faultbuf env, *oldfault;

	oldfault = curpcb->pcb_onfault;
	if (setfault(env)) {
		curpcb->pcb_onfault = oldfault;
		return EFAULT;
	}

	memcpy(dst, src, len);

	curpcb->pcb_onfault = oldfault;
	return 0;
}

int
badaddr(void *addr, size_t size)
{

	return badaddr_read(addr, size, NULL);
}

int
badaddr_read(void *addr, size_t size, int *rptr)
{
	faultbuf env;
	int x;

	/* Get rid of any stale machine checks that have been waiting.  */
	__asm __volatile ("sync; isync");

	if (setfault(env)) {
		curpcb->pcb_onfault = 0;
		__asm __volatile ("sync");
		return 1;
	}

	__asm __volatile ("sync");

	switch (size) {
	case 1:
		x = *(volatile int8_t *)addr;
		break;
	case 2:
		x = *(volatile int16_t *)addr;
		break;
	case 4:
		x = *(volatile int32_t *)addr;
		break;
	default:
		panic("badaddr: invalid size (%d)", size);
	}

	/* Make sure we took the machine check, if we caused one. */
	__asm __volatile ("sync; isync");

	curpcb->pcb_onfault = 0;
	__asm __volatile ("sync");	/* To be sure. */

	/* Use the value to avoid reorder. */
	if (rptr)
		*rptr = x;

	return 0;
}

/*
 * For now, this only deals with the particular unaligned access case
 * that gcc tends to generate.  Eventually it should handle all of the
 * possibilities that can happen on a 32-bit PowerPC in big-endian mode.
 */

static int
fix_unaligned(struct lwp *l, struct trapframe *frame)
{

	return -1;
}

/* 
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curproc;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	upcallret(l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(arg)
	void *arg;
{
	struct lwp *l = curproc;
	int sig;

	/* Take pending signals. */
	while ((sig = CURSIG(l)) != 0)
		postsig(sig);

	/* If our process is on the way out, die. */
	if (l->l_proc->p_flag & P_WEXIT)
		lwp_exit(l);

	/* Invoke any pending upcalls */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
}
