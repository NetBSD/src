/*	$NetBSD: trap.c,v 1.98 2005/12/24 20:07:41 perry Exp $     */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* All bugs are subject to removal without further notice */
		
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.98 2005/12/24 20:07:41 perry Exp $");

#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_multiprocessor.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>

#include <machine/mtpr.h>
#include <machine/pte.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/cpu.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif
#include <kern/syscalls.c>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif

#ifdef TRAPDEBUG
volatile int startsysc = 0, faultdebug = 0;
#endif

int	cpu_printfataltraps = 0;

static inline void userret (struct lwp *, struct trapframe *, u_quad_t);

void	trap (struct trapframe *);
void	syscall (struct trapframe *);

const char * const traptypes[]={
	"reserved addressing",
	"privileged instruction",
	"reserved operand",
	"breakpoint instruction",
	"XFC instruction",
	"system call ",
	"arithmetic trap",
	"asynchronous system trap",
	"page table length fault",
	"translation violation fault",
	"trace trap",
	"compatibility mode fault",
	"access violation fault",
	"",
	"",
	"KSP invalid",
	"",
	"kernel debugger trap"
};
int no_traps = 18;

#define USERMODE(framep)   ((((framep)->psl) & (PSL_U)) == PSL_U)
#define FAULTCHK						\
	if (l->l_addr->u_pcb.iftrap) {				\
		frame->pc = (unsigned)l->l_addr->u_pcb.iftrap;	\
		frame->psl &= ~PSL_FPD;				\
		frame->r0 = EFAULT;/* for copyin/out */		\
		frame->r1 = -1; /* for fetch/store */		\
		return;						\
	}

/*
 * userret:
 *
 *	Common code used by various execption handlers to
 *	return to usermode.
 */
static inline void
userret(struct lwp *l, struct trapframe *frame, u_quad_t oticks)
{
	int sig;
	struct proc *p = l->l_proc;

	/* Generate UNBLOCKED upcall. */
	if (l->l_flag & L_SA_BLOCKING)
		sa_unblock_userret(l);

	/* Take pending signals. */
	while ((sig = CURSIG(l)) != 0)
		postsig(sig);
	l->l_priority = l->l_usrpri;
	if (curcpu()->ci_want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(0);
		while ((sig = CURSIG(l)) != 0)
			postsig(sig);
	}

	/* Invoke per-process kernel-exit handling, if any */
	if (p->p_userret)
		(p->p_userret)(l, p->p_userret_arg);

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, frame->pc,
		    (int)(p->p_sticks - oticks) * psratio);
	}
	/* Invoke any pending upcalls. */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority;
}

void
trap(struct trapframe *frame)
{
	u_int	sig = 0, type = frame->trap, trapsig = 1, code = 0;
	u_int	rv, addr, umode;
	struct	lwp *l;
	struct	proc *p = NULL;
	u_quad_t oticks = 0;
	struct vmspace *vm;
	struct vm_map *map;
	vm_prot_t ftype;

	if ((l = curlwp) != NULL)
		p = l->l_proc;
	uvmexp.traps++;
	if ((umode = USERMODE(frame))) {
		type |= T_USER;
		oticks = p->p_sticks;
		l->l_addr->u_pcb.framep = frame; 
	}

	type&=~(T_WRITE|T_PTEFETCH);


#ifdef TRAPDEBUG
if(frame->trap==7) goto fram;
if(faultdebug)printf("Trap: type %lx, code %lx, pc %lx, psl %lx\n",
		frame->trap, frame->code, frame->pc, frame->psl);
fram:
#endif
	switch(type){

	default:
#ifdef DDB
		kdb_trap(frame);
#endif
		printf("Trap: type %x, code %x, pc %x, psl %x\n",
		    (u_int)frame->trap, (u_int)frame->code,
		    (u_int)frame->pc, (u_int)frame->psl);
		panic("trap");

	case T_KSPNOTVAL:
		panic("kernel stack invalid");

	case T_TRANSFLT|T_USER:
	case T_TRANSFLT:
		/*
		 * BUG! BUG! BUG! BUG! BUG!
		 * Due to a hardware bug (at in least KA65x CPUs) a double
		 * page table fetch trap will cause a translation fault
		 * even if access in the SPT PTE entry specifies 'no access'.
		 * In for example section 6.4.2 in VAX Architecture 
		 * Reference Manual it states that if a page both are invalid
		 * and have no access set, a 'access violation fault' occurs.
		 * Therefore, we must fall through here...
		 */
#ifdef nohwbug
		panic("translation fault");
#endif

	case T_PTELEN|T_USER:	/* Page table length exceeded */
	case T_ACCFLT|T_USER:
		if (frame->code < 0) { /* Check for kernel space */
			sig = SIGSEGV;
			code = SEGV_ACCERR;
			break;
		}

	case T_PTELEN:
	case T_ACCFLT:
#ifdef TRAPDEBUG
if(faultdebug)printf("trap accflt type %lx, code %lx, pc %lx, psl %lx\n",
			frame->trap, frame->code, frame->pc, frame->psl);
#endif
#ifdef DIAGNOSTIC
		if (p == 0)
			panic("trap: access fault: addr %lx code %lx",
			    frame->pc, frame->code);
		if (frame->psl & PSL_IS)
			panic("trap: pflt on IS");
#endif

		/*
		 * Page tables are allocated in pmap_enter(). We get 
		 * info from below if it is a page table fault, but
		 * UVM may want to map in pages without faults, so
		 * because we must check for PTE pages anyway we don't
		 * bother doing it here.
		 */
		addr = trunc_page(frame->code);
		if ((umode == 0) && (frame->code < 0)) {
			vm = NULL;
			map = kernel_map;
		} else {
			vm = p->p_vmspace;
			map = &vm->vm_map;
		}

		if (frame->trap & T_WRITE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		if (umode) {
			KERNEL_PROC_LOCK(l);
			if (l->l_flag & L_SA) {
				l->l_savp->savp_faultaddr = (vaddr_t)frame->code;
				l->l_flag |= L_SA_PAGEFAULT;
			}
		} else
			KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);

		rv = uvm_fault(map, addr, 0, ftype);
		if (rv != 0) {
			if (umode == 0) {
				KERNEL_UNLOCK();
				FAULTCHK;
				panic("Segv in kernel mode: pc %x addr %x",
				    (u_int)frame->pc, (u_int)frame->code);
			}
			code = SEGV_ACCERR;
			if (rv == ENOMEM) {
				printf("UVM: pid %d (%s), uid %d killed: "
				       "out of swap\n",
				       p->p_pid, p->p_comm,
				       p->p_cred && p->p_ucred ?
				       p->p_ucred->cr_uid : -1);
				sig = SIGKILL;
			} else {
				sig = SIGSEGV;
				if (rv != EACCES)
					code = SEGV_MAPERR;
			}
		} else {
			trapsig = 0;
			if (map != kernel_map && (caddr_t)addr >= vm->vm_maxsaddr)
				uvm_grow(p, addr);
		}
		if (umode) {
			l->l_flag &= ~L_SA_PAGEFAULT;
			KERNEL_PROC_UNLOCK(l);
		} else
			KERNEL_UNLOCK();
		break;

	case T_BPTFLT|T_USER:
		sig = SIGTRAP;
		code = TRAP_BRKPT;
		break;
	case T_TRCTRAP|T_USER:
		sig = SIGTRAP;
		code = TRAP_TRACE;
		frame->psl &= ~PSL_T;
		break;

	case T_PRIVINFLT|T_USER:
		sig = SIGILL;
		code = ILL_PRVOPC;
		break;
	case T_RESADFLT|T_USER:
		sig = SIGILL;
		code = ILL_ILLADR;
		break;
	case T_RESOPFLT|T_USER:
		sig = SIGILL;
		code = ILL_ILLOPC;
		break;

	case T_XFCFLT|T_USER:
		sig = SIGEMT;
		break;

	case T_ARITHFLT|T_USER:
		sig = SIGFPE;
		break;

	case T_ASTFLT|T_USER:
		mtpr(AST_NO,PR_ASTLVL);
		trapsig = 0;
		break;

#ifdef DDB
	case T_BPTFLT: /* Kernel breakpoint */
	case T_KDBTRAP:
	case T_KDBTRAP|T_USER:
	case T_TRCTRAP:
		kdb_trap(frame);
		return;
#endif
	}
	if (trapsig) {
		ksiginfo_t ksi;
		if ((sig == SIGSEGV || sig == SIGILL) && cpu_printfataltraps)
			printf("pid %d.%d (%s): sig %d: type %lx, code %lx, pc %lx, psl %lx\n",
			       p->p_pid, l->l_lid, p->p_comm, sig, frame->trap,
			       frame->code, frame->pc, frame->psl);
		KERNEL_PROC_LOCK(l);
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = sig;
		ksi.ksi_trap = frame->trap;
		ksi.ksi_addr = (void *)frame->code;
		ksi.ksi_code = code;
		trapsignal(l, &ksi);
		KERNEL_PROC_UNLOCK(l);
	}

	if (umode == 0)
		return;

	userret(l, frame, oticks);
}

void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *exptr;

	exptr = l->l_addr->u_pcb.framep;
	exptr->pc = pack->ep_entry + 2;
	exptr->sp = stack;
	exptr->r6 = stack;				/* for ELF */
	exptr->r7 = 0;					/* for ELF */
	exptr->r8 = 0;					/* for ELF */
	exptr->r9 = (u_long) l->l_proc->p_psstr;	/* for ELF */
}

void
syscall(struct trapframe *frame)
{
	const struct sysent *callp;
	u_quad_t oticks;
	int nsys;
	int err, rval[2], args[8];
	struct trapframe *exptr;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

#ifdef TRAPDEBUG
if(startsysc)printf("trap syscall %s pc %lx, psl %lx, sp %lx, pid %d, frame %p\n",
	       syscallnames[frame->code], frame->pc, frame->psl,frame->sp,
		p->p_pid,frame);
#endif
	uvmexp.syscalls++;
 
	exptr = l->l_addr->u_pcb.framep = frame;
	callp = p->p_emul->e_sysent;
	nsys = p->p_emul->e_nsysent;
	oticks = p->p_sticks;

	if (frame->code == SYS___syscall) {
		int g = *(int *)(frame->ap);

		frame->code = *(int *)(frame->ap + 4);
		frame->ap += 8;
		*(int *)(frame->ap) = g - 2;
	}

	if ((unsigned long) frame->code >= nsys)
		callp += p->p_emul->e_nosys;
	else
		callp += frame->code;

	rval[0] = 0;
	rval[1] = frame->r1;
	KERNEL_PROC_LOCK(l);
	if (callp->sy_narg) {
		err = copyin((char*)frame->ap + 4, args, callp->sy_argsize);
		if (err) {
			KERNEL_PROC_UNLOCK(l);
			goto bad;
		}
	}

	if ((err = trace_enter(l, frame->code, frame->code, NULL, args)) != 0)
		goto out;

	err = (*callp->sy_call)(curlwp, args, rval);
out:
	KERNEL_PROC_UNLOCK(l);
	exptr = l->l_addr->u_pcb.framep;

#ifdef TRAPDEBUG
if(startsysc)
	printf("retur %s pc %lx, psl %lx, sp %lx, pid %d, err %d r0 %d, r1 %d, frame %p\n",
	       syscallnames[exptr->code], exptr->pc, exptr->psl,exptr->sp,
		p->p_pid,err,rval[0],rval[1],exptr); /* } */
#endif

bad:
	switch (err) {
	case 0:
		exptr->r1 = rval[1];
		exptr->r0 = rval[0];
		exptr->psl &= ~PSL_C;
		break;

	case EJUSTRETURN:
		break;

	case ERESTART:
		exptr->pc -= (exptr->code > 63 ? 4 : 2);
		break;

	default:
		exptr->r0 = err;
		exptr->psl |= PSL_C;
		break;
	}

	trace_exit(l, frame->code, args, rval, err);

	userret(l, frame, oticks);
}

void
child_return(void *arg)
{
        struct lwp *l = arg;

	KERNEL_PROC_UNLOCK(l);
	userret(l, l->l_addr->u_pcb.framep, 0);

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(l, SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
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
	struct lwp *l = curlwp;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	/* XXX - profiling spoiled here */
	userret(l, l->l_addr->u_pcb.framep, l->l_proc->p_sticks);
}

void
upcallret(struct lwp *l)
{

	/* XXX - profiling */
	userret(l, l->l_addr->u_pcb.framep, l->l_proc->p_sticks);
}

