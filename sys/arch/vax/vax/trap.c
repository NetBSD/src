/*	$NetBSD: trap.c,v 1.131.2.2 2017/12/03 11:36:48 jdolecek Exp $     */

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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.131.2.2 2017/12/03 11:36:48 jdolecek Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <sys/signalvar.h>

#include <uvm/uvm_extern.h>

#include <machine/trap.h>
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

#define USERMODE_P(tf)   ((((tf)->tf_psl) & (PSL_U)) == PSL_U)

void
trap(struct trapframe *tf)
{
	u_int	sig = 0, type = tf->tf_trap, code = 0;
	u_int	rv, addr;
	bool trapsig = true;
	const bool usermode = USERMODE_P(tf);
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct pcb * const pcb = lwp_getpcb(l);
	u_quad_t oticks = 0;
	struct vmspace *vm;
	struct vm_map *map;
	vm_prot_t ftype;
	void *onfault = pcb->pcb_onfault;

	KASSERT(p != NULL);
	curcpu()->ci_data.cpu_ntrap++;
	if (usermode) {
		type |= T_USER;
		oticks = p->p_sticks;
		l->l_md.md_utf = tf;
		LWP_CACHE_CREDS(l, p);
	}

	type &= ~(T_WRITE|T_PTEFETCH);


#ifdef TRAPDEBUG
if(tf->tf_trap==7) goto fram;
if(faultdebug)printf("Trap: type %lx, code %lx, pc %lx, psl %lx\n",
		tf->tf_trap, tf->tf_code, tf->tf_pc, tf->tf_psl);
fram:
#endif
	switch (type) {

	default:
#ifdef DDB
		kdb_trap(tf);
#endif
		panic("trap: type %x, code %x, pc %x, psl %x",
		    (u_int)tf->tf_trap, (u_int)tf->tf_code,
		    (u_int)tf->tf_pc, (u_int)tf->tf_psl);

	case T_KSPNOTVAL:
		panic("%d.%d (%s): KSP invalid %#x@%#x pcb %p fp %#x psl %#x)",
		    p->p_pid, l->l_lid, l->l_name ? l->l_name : "??",
		    mfpr(PR_KSP), (u_int)tf->tf_pc, pcb,
		    (u_int)tf->tf_fp, (u_int)tf->tf_psl);

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
		if (tf->tf_code < 0) { /* Check for kernel space */
			sig = SIGSEGV;
			code = SEGV_ACCERR;
			break;
		}

	case T_PTELEN:
#ifndef MULTIPROCESSOR
		/*
		 * If we referred to an address beyond the end of the system
		 * page table, it may be due to a failed CAS
		 * restartable-atomic-sequence.  If it is, restart it at the
		 * beginning and restart.
		 */
		{
			extern const uint8_t cas32_ras_start[], cas32_ras_end[];
			if (tf->tf_code == CASMAGIC
			    && tf->tf_pc >= (uintptr_t) cas32_ras_start
			    && tf->tf_pc < (uintptr_t) cas32_ras_end) {
				tf->tf_pc = (uintptr_t) cas32_ras_start;
				trapsig = false;
				break;
			}
		}
		/* FALLTHROUGH */
#endif
	case T_ACCFLT:
#ifdef TRAPDEBUG
if(faultdebug)printf("trap accflt type %lx, code %lx, pc %lx, psl %lx\n",
			tf->tf_trap, tf->tf_code, tf->tf_pc, tf->tf_psl);
#endif
#ifdef DIAGNOSTIC
		if (p == 0)
			panic("trap: access fault: addr %lx code %lx",
			    tf->tf_pc, tf->tf_code);
		if (tf->tf_psl & PSL_IS)
			panic("trap: pflt on IS");
#endif

		/*
		 * Page tables are allocated in pmap_enter(). We get 
		 * info from below if it is a page table fault, but
		 * UVM may want to map in pages without faults, so
		 * because we must check for PTE pages anyway we don't
		 * bother doing it here.
		 */
		addr = trunc_page(tf->tf_code);
		if (!usermode && (tf->tf_code < 0)) {
			vm = NULL;
			map = kernel_map;

		} else {
			vm = p->p_vmspace;
			map = &vm->vm_map;
		}

		if (tf->tf_trap & T_WRITE)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

		pcb->pcb_onfault = NULL;
		rv = uvm_fault(map, addr, ftype);
		pcb->pcb_onfault = onfault;
		if (rv != 0) {
			if (!usermode) {
				if (onfault) {
					pcb->pcb_onfault = NULL;
					tf->tf_pc = (unsigned)onfault;
					tf->tf_psl &= ~PSL_FPD;
					tf->tf_r0 = rv;
					return;
				}
				printf("r0=%08lx r1=%08lx r2=%08lx r3=%08lx ",
				    tf->tf_r0, tf->tf_r1, tf->tf_r2, tf->tf_r3);
				printf("r4=%08lx r5=%08lx r6=%08lx r7=%08lx\n",
				    tf->tf_r4, tf->tf_r5, tf->tf_r6, tf->tf_r7);
				printf(
				    "r8=%08lx r9=%08lx r10=%08lx r11=%08lx\n",
				    tf->tf_r8, tf->tf_r9, tf->tf_r10,
				    tf->tf_r11);
				printf("ap=%08lx fp=%08lx sp=%08lx pc=%08lx\n",
				    tf->tf_ap, tf->tf_fp, tf->tf_sp, tf->tf_pc);
				panic("SEGV in kernel mode: pc %#lx addr %#lx",
				    tf->tf_pc, tf->tf_code);
			}
			switch (rv) {
			case ENOMEM:
				printf("UVM: pid %d (%s), uid %d killed: "
				       "out of swap\n",
				       p->p_pid, p->p_comm,
				       l->l_cred ?
				       kauth_cred_geteuid(l->l_cred) : -1);
				sig = SIGKILL;
				code = SI_NOINFO;
				break;
			case EINVAL:
				code = BUS_ADRERR;
				sig = SIGBUS;
				break;
			case EACCES:
				code = SEGV_ACCERR;
				sig = SIGSEGV;
				break;
			default:
				code = SEGV_MAPERR;
				sig = SIGSEGV;
				break;
			}
		} else {
			trapsig = false;
			if (map != kernel_map && addr > 0
			    && (void *)addr >= vm->vm_maxsaddr)
				uvm_grow(p, addr);
		}
		break;

	case T_BPTFLT|T_USER:
		sig = SIGTRAP;
		code = TRAP_BRKPT;
		break;
	case T_TRCTRAP|T_USER:
		sig = SIGTRAP;
		code = TRAP_TRACE;
		tf->tf_psl &= ~PSL_T;
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
		switch (tf->tf_code) {
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
		kdb_trap(tf);
		return;
#endif
	}
	if (trapsig) {
		ksiginfo_t ksi;
		if ((sig == SIGSEGV || sig == SIGILL)
		    && cpu_printfataltraps
		    && (p->p_slflag & PSL_TRACED) == 0
		    && !sigismember(&p->p_sigctx.ps_sigcatch, sig))
			printf("pid %d.%d (%s): sig %d: type %lx, code %lx, pc %lx, psl %lx\n",
			       p->p_pid, l->l_lid, p->p_comm, sig, tf->tf_trap,
			       tf->tf_code, tf->tf_pc, tf->tf_psl);
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = sig;
		ksi.ksi_trap = tf->tf_trap;
		ksi.ksi_addr = (void *)tf->tf_code;
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
		if (type == (T_ARITHFLT | T_USER) && (tf->tf_code & 8))
			tf->tf_pc = skip_opcode(tf->tf_pc);

		trapsignal(l, &ksi);
	}

	if (!usermode)
		return;

	userret(l, tf, oticks);
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe * const tf = l->l_md.md_utf;

	tf->tf_pc = pack->ep_entry + 2;
	tf->tf_sp = stack;
	tf->tf_r6 = stack;				/* for ELF */
	tf->tf_r7 = 0;				/* for ELF */
	tf->tf_r8 = 0;				/* for ELF */
	tf->tf_r9 = l->l_proc->p_psstrp;		/* for ELF */
}


/* 
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t * const uc = arg;
	lwp_t * const l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	/* XXX - profiling spoiled here */
	userret(l, l->l_md.md_utf, l->l_proc->p_sticks);
}
