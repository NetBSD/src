/*	$NetBSD: trap.c,v 1.53.4.7 2002/04/01 07:42:09 nathanw Exp $	*/

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
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ktrace.h>
#include <sys/lwp.h>
#include <sys/pool.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/db_machdep.h>
#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <powerpc/spr.h>

/* These definitions should probably be somewhere else			XXX */
#define	FIRSTARG	3		/* first argument is in reg 3 */
#define	NARGREG		8		/* 8 args are in registers */
#define	MOREARGS(sp)	((caddr_t)((int)(sp) + 8)) /* more args go here */

#ifndef MULTIPROCESSOR
volatile int astpending;
volatile int want_resched;
extern int intr_depth;
#endif

void *syscall = NULL;	/* XXX dummy symbol for emul_netbsd */

static int fix_unaligned __P((struct lwp *l, struct trapframe *frame));
static inline void setusr __P((int));

void trap __P((struct trapframe *));	/* Called from locore / trap_subr */
int setfault __P((faultbuf));	/* defined in locore.S */
/* Why are these not defined in a header? */
int badaddr __P((void *, size_t));
int badaddr_read __P((void *, size_t, int *));

void
trap(frame)
	struct trapframe *frame;
{
	struct lwp *l = curproc;
	struct proc *p = l ? l->l_proc : NULL;
	int type = frame->exc;
	int ftype, rv;

	curcpu()->ci_ev_traps.ev_count++;

	if (frame->srr1 & PSL_PR)
		type |= EXC_USER;

#ifdef DIAGNOSTIC
	if (curpcb->pcb_pmreal != curpm)
		panic("trap: curpm (%p) != curpcb->pcb_pmreal (%p)",
		    curpm, curpcb->pcb_pmreal);
#endif

	uvmexp.traps++;

	switch (type) {
	case EXC_RUNMODETRC|EXC_USER:
		/* FALLTHROUGH */
	case EXC_TRC|EXC_USER:
		KERNEL_PROC_LOCK(l);
		frame->srr1 &= ~PSL_SE;
		trapsignal(l, SIGTRAP, EXC_TRC);
		KERNEL_PROC_UNLOCK(l);
		break;
	case EXC_DSI: {
		faultbuf *fb;
		/*
		 * Only query UVM if no interrupts are active (this applies
		 * "on-fault" as well.
		 */
		curcpu()->ci_ev_kdsi.ev_count++;
		if (intr_depth < 0) {
			struct vm_map *map;
			vaddr_t va;

			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
			map = kernel_map;
			va = frame->dar;
			if ((va >> ADDR_SR_SHFT) == USER_SR) {
				sr_t user_sr;

				asm ("mfsr %0, %1"
				     : "=r"(user_sr) : "K"(USER_SR));
				va &= ADDR_PIDX | ADDR_POFF;
				va |= user_sr << ADDR_SR_SHFT;
				/* KERNEL_PROC_LOCK(p); XXX */
				map = &p->p_vmspace->vm_map;
			}
			if (frame->dsisr & DSISR_STORE)
				ftype = VM_PROT_WRITE;
			else
				ftype = VM_PROT_READ;
			rv = uvm_fault(map, trunc_page(va), 0, ftype);
			if (map != kernel_map) {
				/*
				 * Record any stack growth...
				 */
				if (rv == 0)
					uvm_grow(p, trunc_page(va));
				/* KERNEL_PROC_UNLOCK(p); XXX */
			}
			KERNEL_UNLOCK();
			if (rv == 0)
				return;
			if (rv == EACCES)
				rv = EFAULT;
		} else {
			rv = EFAULT;
		}
		if ((fb = l->l_addr->u_pcb.pcb_onfault) != NULL) {
			frame->srr0 = (*fb)[0];
			frame->fixreg[1] = (*fb)[1];
			frame->fixreg[2] = (*fb)[2];
			frame->fixreg[3] = rv;
			frame->cr = (*fb)[3];
			memcpy(&frame->fixreg[13], &(*fb)[4],
				      19 * sizeof(register_t));
			return;
		}
		printf("trap: kernel %s DSI @ %#x by %#x (DSISR %#x, err=%d)\n",
		    (frame->dsisr & DSISR_STORE) ? "write" : "read",
		    frame->dar, frame->srr0, frame->dsisr, rv);
		goto brain_damage2;
	}
	case EXC_DSI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		curcpu()->ci_ev_udsi.ev_count++;
		if (frame->dsisr & DSISR_STORE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(frame->dar),
		    0, ftype);
		if (rv == 0) {
			/*
			 * Record any stack growth...
			 */
			uvm_grow(p, trunc_page(frame->dar));
			KERNEL_PROC_UNLOCK(l);
			break;
		}
		curcpu()->ci_ev_udsi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s): user %s DSI @ %#x "
			    "by %#x (DSISR %#x, err=%d)\n",
			    p->p_pid, p->p_comm,
			    (frame->dsisr & DSISR_STORE) ? "write" : "read",
			    frame->dar, frame->srr0, frame->dsisr, rv);
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
	case EXC_ISI:
		printf("trap: kernel ISI by %#x (SRR1 %#x)\n",
		    frame->srr0, frame->srr1);
		goto brain_damage2;
	case EXC_ISI|EXC_USER:
		KERNEL_PROC_LOCK(l);
		curcpu()->ci_ev_isi.ev_count++;
		ftype = VM_PROT_READ | VM_PROT_EXECUTE;
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(frame->srr0),
		    0, ftype);
		if (rv == 0) {
			KERNEL_PROC_UNLOCK(l);
			break;
		}
		curcpu()->ci_ev_isi_fatal.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s) lid %d: user ISI trap @ %#x "
			    "(SSR1=%#x)\n", p->p_pid, p->p_comm, l->l_lid,
			    frame->srr0, frame->srr1);
		}
		trapsignal(l, SIGSEGV, EXC_ISI);
		KERNEL_PROC_UNLOCK(l);
		break;
	case EXC_SC|EXC_USER:
		curcpu()->ci_ev_scalls.ev_count++;
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

#ifdef	KTRACE
			if (KTRPOINT(p, KTR_SYSCALL))
				ktrsyscall(p, code, argsize, params);
#endif

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

#ifdef	KTRACE
			if (KTRPOINT(p, KTR_SYSRET))
				ktrsysret(p, code, error, rval[0]);
#endif
		}
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_FPU|EXC_USER:
		curcpu()->ci_ev_fpu.ev_count++;
		if (fpuproc) {
			curcpu()->ci_ev_fpusw.ev_count++;
			save_fpu(fpuproc);
		}
#if defined(MULTIPROCESSOR)
		if (l->l_addr->u_pcb.pcb_fpcpu)
			save_fpu_proc(l);
#endif
		fpuproc = l;
		l->l_addr->u_pcb.pcb_fpcpu = curcpu();
		enable_fpu(l);
		break;

#ifdef ALTIVEC
	case EXC_VEC|EXC_USER:
		curcpu()->ci_ev_vec.ev_count++;
		if (vecproc) {
			curcpu()->ci_ev_vecsw.ev_count++;
			save_vec(vecproc);
		}
		vecproc = l;
		enable_vec(l);
		break;
#endif

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
		curcpu()->ci_ev_ali.ev_count++;
		if (fix_unaligned(l, frame) != 0) {
			curcpu()->ci_ev_ali_fatal.ev_count++;
			if (cpu_printfataltraps) {
				printf("trap: pid %d (%s) lid %d: user ALI "
				    "trap @ %#x (SSR1=%#x)\n",
				    p->p_pid, p->p_comm, l->l_lid, frame->srr0,
				    frame->srr1);
			}
			trapsignal(l, SIGBUS, EXC_ALI);
		} else
			frame->srr0 += 4;
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_PGM|EXC_USER:
/* XXX temporarily */
		KERNEL_PROC_LOCK(l);
		curcpu()->ci_ev_pgm.ev_count++;
		if (cpu_printfataltraps) {
			printf("trap: pid %d (%s) lid %d: user PGM trap @ %#x "
			    "(SSR1=%#x)\n", p->p_pid, p->p_comm, l->l_lid,
			    frame->srr0, frame->srr1);
		}
		if (frame->srr1 & 0x00020000)	/* Bit 14 is set if trap */
			trapsignal(l, SIGTRAP, EXC_PGM);
		else
			trapsignal(l, SIGILL, EXC_PGM);
		KERNEL_PROC_UNLOCK(l);
		break;

	case EXC_MCHK: {
		faultbuf *fb;

		if ((fb = l->l_addr->u_pcb.pcb_onfault) != NULL) {
			frame->srr0 = (*fb)[0];
			frame->fixreg[1] = (*fb)[1];
			frame->fixreg[2] = (*fb)[2];
			frame->fixreg[3] = EFAULT;
			frame->cr = (*fb)[3];
			memcpy(&frame->fixreg[13], &(*fb)[4],
			      19 * sizeof(register_t));
			return;
		}
		goto brain_damage;
	}

	default:
brain_damage:
		printf("trap type %x at %x\n", type, frame->srr0);
brain_damage2:
#ifdef DDBX
		if (kdb_trap(type, frame))
			return;
#endif
#ifdef TRAP_PANICWAIT
		printf("Press a key to panic.\n");
		cnpollc(1);
		cngetc();
		cnpollc(0);
#endif
		panic("trap");
	}

	/* Take pending signals. */
	{
		int sig;

		while ((sig = CURSIG(l)) != 0)
			postsig(sig);
	}

	/*
	 * If someone stole the fp or vector unit while we were away,
	 * disable it
	 */
	if (l != fpuproc || l->l_addr->u_pcb.pcb_fpcpu != curcpu())
		frame->srr1 &= ~PSL_FP;
#ifdef ALTIVEC
	if (l != vecproc)
		frame->srr1 &= ~PSL_VEC;
#endif

	/* Invoke per-process kernel-exit handling, if any */
	if (p->p_userret)
		(p->p_userret)(l, p->p_userret_arg);

	/* Invoke any pending upcalls */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
}

void
child_return(arg)
	void *arg;
{
	struct lwp *l = arg;
	struct trapframe *tf = trapframe(l);

	KERNEL_PROC_UNLOCK(l);

	tf->fixreg[FIRSTARG] = 0;
	tf->fixreg[FIRSTARG + 1] = 1;
	tf->cr &= ~0x10000000;
	tf->srr1 &= ~(PSL_FP|PSL_VEC);	/* Disable FP & AltiVec, as we can't
					   be them. */
	l->l_addr->u_pcb.pcb_fpcpu = NULL;
#ifdef	KTRACE
	if (KTRPOINT(l->l_proc, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(l->l_proc, SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
	/* Profiling?							XXX */
	curcpu()->ci_schedstate.spc_curpriority = l->l_priority;
}

static inline void
setusr(content)
	int content;
{
	asm volatile ("isync; mtsr %0,%1; isync"
		      :: "n"(USER_SR), "r"(content));
}

int
copyin(udaddr, kaddr, len)
	const void *udaddr;
	void *kaddr;
	size_t len;
{
	const char *up = udaddr;
	char *kp = kaddr;
	char *p;
	int rv;
	size_t l;
	faultbuf env;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	while (len > 0) {
		p = (char *)USER_ADDR + ((u_int)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(u_int)up >> ADDR_SR_SHFT]);
		memcpy(kp, p, l);
		up += l;
		kp += l;
		len -= l;
	}
	curpcb->pcb_onfault = 0;
	return 0;
}

int
copyout(kaddr, udaddr, len)
	const void *kaddr;
	void *udaddr;
	size_t len;
{
	const char *kp = kaddr;
	char *up = udaddr;
	char *p;
	int rv;
	size_t l;
	faultbuf env;

	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = 0;
		return rv;
	}
	while (len > 0) {
		p = (char *)USER_ADDR + ((u_int)up & ~SEGMENT_MASK);
		l = ((char *)USER_ADDR + SEGMENT_LENGTH) - p;
		if (l > len)
			l = len;
		setusr(curpcb->pcb_pm->pm_sr[(u_int)up >> ADDR_SR_SHFT]);
		memcpy(p, kp, l);
		up += l;
		kp += l;
		len -= l;
	}
	curpcb->pcb_onfault = 0;
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
kcopy(src, dst, len)
	const void *src;
	void *dst;
	size_t len;
{
	faultbuf env, *oldfault;
	int rv;

	oldfault = curpcb->pcb_onfault;
	if ((rv = setfault(env)) != 0) {
		curpcb->pcb_onfault = oldfault;
		return rv;
	}

	memcpy(dst, src, len);

	curpcb->pcb_onfault = oldfault;
	return 0;
}

int
badaddr(addr, size)
	void *addr;
	size_t size;
{
	return badaddr_read(addr, size, NULL);
}

int
badaddr_read(addr, size, rptr)
	void *addr;
	size_t size;
	int *rptr;
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
fix_unaligned(l, frame)
	struct lwp *l;
	struct trapframe *frame;
{
	int indicator = EXC_ALI_OPCODE_INDICATOR(frame->dsisr);

	switch (indicator) {
	case EXC_ALI_LFD:
	case EXC_ALI_STFD:
		{
			int reg = EXC_ALI_RST(frame->dsisr);
			double *fpr = &l->l_addr->u_pcb.pcb_fpu.fpr[reg];

			/* Juggle the FPU to ensure that we've initialized
			 * the FPRs, and that their current state is in
			 * the PCB.
			 */
			if (fpuproc != l) {
				if (fpuproc)
					save_fpu(fpuproc);
				enable_fpu(l);
			}
			save_fpu(l);

			if (indicator == EXC_ALI_LFD) {
				if (copyin((void *)frame->dar, fpr,
				    sizeof(double)) != 0)
					return -1;
				enable_fpu(l);
			} else {
				if (copyout(fpr, (void *)frame->dar,
				    sizeof(double)) != 0)
					return -1;
			}
			return 0;
		}
		break;
	}

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

	upcallret((void *) l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	int sig;

	/* Take pending signals. */
	while ((sig = CURSIG(l)) != 0)
		postsig(sig);

	/* Invoke per-process kernel-exit handling, if any */
	if (l->l_proc->p_userret)
		(l->l_proc->p_userret)(l, l->l_proc->p_userret_arg);

	/* Invoke any pending upcalls */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
}
