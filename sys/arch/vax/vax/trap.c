/*	$NetBSD: trap.c,v 1.121 2009/12/10 14:13:53 matt Exp $     */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.121 2009/12/10 14:13:53 matt Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/pool.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <machine/mtpr.h>
#include <machine/pte.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/userret.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif
#include <vax/vax/db_disasm.h>
#include <kern/syscalls.c>
#include <sys/ktrace.h>

#ifdef TRAPDEBUG
volatile int faultdebug = 0;
#endif

int	cpu_printfataltraps = 0;

void	trap (struct trapframe *);

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

#define USERMODE_P(framep)   ((((framep)->psl) & (PSL_U)) == PSL_U)
#define FAULTCHK(pcb)						\
	if (pcb->iftrap) {					\
		frame->pc = (unsigned)pcb->iftrap;		\
		frame->psl &= ~PSL_FPD;				\
		frame->r0 = EFAULT;/* for copyin/out */		\
		frame->r1 = -1; /* for fetch/store */		\
		return;						\
	}


void
trap(struct trapframe *frame)
{
	u_int	sig = 0, type = frame->trap, code = 0;
	u_int	rv, addr;
	bool trapsig = true;
	const bool usermode = USERMODE_P(frame);;
	struct	lwp *l;
	struct	proc *p;
	struct	pcb *pcb;
	u_quad_t oticks = 0;
	struct vmspace *vm;
	struct vm_map *map;
	vm_prot_t ftype;

	l = curlwp;
	KASSERT(l != NULL);
	p = l->l_proc;
	KASSERT(p != NULL);
	pcb = lwp_getpcb(l);
	uvmexp.traps++;
	if (usermode) {
		type |= T_USER;
		oticks = p->p_sticks;
		pcb->framep = frame; 
		LWP_CACHE_CREDS(l, p);
	}

	type &= ~(T_WRITE|T_PTEFETCH);


#ifdef TRAPDEBUG
if(frame->trap==7) goto fram;
if(faultdebug)printf("Trap: type %lx, code %lx, pc %lx, psl %lx\n",
		frame->trap, frame->code, frame->pc, frame->psl);
fram:
#endif
	switch (type) {

	default:
#ifdef DDB
		kdb_trap(frame);
#endif
		panic("trap: type %x, code %x, pc %x, psl %x",
		    (u_int)frame->trap, (u_int)frame->code,
		    (u_int)frame->pc, (u_int)frame->psl);

	case T_KSPNOTVAL:
		panic("%d.%d (%s): KSP invalid %#x@%#x pcb %p fp %#x psl %#x)",
		    p->p_pid, l->l_lid, l->l_name ? l->l_name : "??",
		    mfpr(PR_KSP), (u_int)frame->pc, lwp_getpcb(l),
		    (u_int)frame->fp, (u_int)frame->psl);

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
		if (!usermode && (frame->code < 0)) {
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

		if ((usermode) && (l->l_flag & LW_SA)) {
			l->l_savp->savp_faultaddr = (vaddr_t)frame->code;
			l->l_pflag |= LP_SA_PAGEFAULT;
		}

		rv = uvm_fault(map, addr, ftype);
		if (rv != 0) {
			if (!usermode) {
				FAULTCHK(pcb);
				panic("Segv in kernel mode: pc %x addr %x",
				    (u_int)frame->pc, (u_int)frame->code);
			}
			code = SEGV_ACCERR;
			if (rv == ENOMEM) {
				printf("UVM: pid %d (%s), uid %d killed: "
				       "out of swap\n",
				       p->p_pid, p->p_comm,
				       l->l_cred ?
				       kauth_cred_geteuid(l->l_cred) : -1);
				sig = SIGKILL;
			} else {
				sig = SIGSEGV;
				if (rv != EACCES)
					code = SEGV_MAPERR;
			}
		} else {
			trapsig = false;
			if (map != kernel_map && addr > 0
			    && (void *)addr >= vm->vm_maxsaddr)
				uvm_grow(p, addr);
		}
		if (usermode) {
			l->l_pflag &= ~LP_SA_PAGEFAULT;
		}
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
		switch (frame->code) {
		case ATRP_INTOVF: code = FPE_INTOVF; break;
		case ATRP_INTDIV: code = FPE_INTDIV; break;
		case ATRP_FLTOVF: code = FPE_FLTOVF; break;
		case ATRP_FLTDIV: code = FPE_FLTDIV; break;
		case ATRP_FLTUND: code = FPE_FLTUND; break;
		case ATRP_DECOVF: code = FPE_INTOVF; break;
		case ATRP_FLTSUB: code = FPE_FLTSUB; break;
		case AFLT_FLTDIV: code = FPE_FLTDIV; break;
		case AFLT_FLTUND: code = FPE_FLTUND; break;
		case AFLT_FLTOVF: code = FPE_FLTOVF; break;
		default:	  code = FPE_FLTINV; break;
		}
		break;

	case T_ASTFLT|T_USER:
		mtpr(AST_NO,PR_ASTLVL);
		trapsig = false;
		if (curcpu()->ci_want_resched)
			preempt();
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
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = sig;
		ksi.ksi_trap = frame->trap;
		ksi.ksi_addr = (void *)frame->code;
		ksi.ksi_code = code;

		/*
		 * Arithmetic exceptions can be of two kinds:
		 * - traps (codes 1..7), where pc points to the
		 *   next instruction to execute.
		 * - faults (codes 8..10), where pc points to the
		 *   faulting instruction.
		 * In the latter case, we need to advance pc by ourselves
		 * to prevent a signal loop.
		 *
		 * XXX this is gross -- miod
		 */
		if (type == (T_ARITHFLT | T_USER) && (frame->code & 8))
			frame->pc = skip_opcode(frame->pc);

		trapsignal(l, &ksi);
	}

	if (!usermode)
		return;

	userret(l, frame, oticks);
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe *exptr;
	struct pcb *pcb;

	pcb = lwp_getpcb(l);
	exptr = pcb->framep;
	exptr->pc = pack->ep_entry + 2;
	exptr->sp = stack;
	exptr->r6 = stack;				/* for ELF */
	exptr->r7 = 0;					/* for ELF */
	exptr->r8 = 0;					/* for ELF */
	exptr->r9 = (u_long) l->l_proc->p_psstr;	/* for ELF */
}


/* 
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;
	struct pcb *pcb;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	/* XXX - profiling spoiled here */
	pcb = lwp_getpcb(l);
	userret(l, pcb->framep, l->l_proc->p_sticks);
}

void
upcallret(struct lwp *l)
{
	struct pcb *pcb;

	/* XXX - profiling */
	pcb = lwp_getpcb(l);
	userret(l, pcb->framep, l->l_proc->p_sticks);
}

