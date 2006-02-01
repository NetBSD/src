/*	$NetBSD: trap.c,v 1.7.2.1 2006/02/01 14:51:42 yamt Exp $	*/
/*	NetBSD: trap.c,v 1.200 2004/03/14 01:08:48 cl Exp 	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
 * 3. Neither the name of the University nor the names of its contributors
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
 * 386 Trap and System call handling
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.7.2.1 2006/02/01 14:51:42 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_math_emulate.h"
#include "opt_multiprocessor.h"
#include "opt_vm86.h"
#include "opt_kvm86.h"
#include "opt_cputype.h"
#include "opt_kstack_dr0.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/ras.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <sys/ucontext.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>
#ifdef DDB
#include <machine/db_machdep.h>
#endif

#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>
#endif

#include "isa.h"

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "npx.h"

void trap(struct trapframe *);
void trap_tss(struct i386tss *, int, int);
#if defined(I386_CPU)
int trapwrite(unsigned);
#endif

#ifdef KVM86
#ifdef MULTIPROCESSOR
#error KVM86 needs a rewrite to support MP systems.
#endif
#include <machine/kvm86.h>
#define KVM86MODE (kvm86_incall)
#else
#define KVM86MODE (0)
#endif

const char * const trap_type[] = {
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
	"machine check fault",			/* 18 T_MCA */
	"SSE FP exception",			/* 19 T_XMM */
	"reserved trap",			/* 20 T_RESERVED */
};
int	trap_types = sizeof trap_type / sizeof trap_type[0];

#ifdef DEBUG
int	trapdebug = 0;
#endif

#define	IDTVEC(name)	__CONCAT(X, name)

void
trap_tss(struct i386tss *tss, int trapno, int code)
{
	struct trapframe tf;

	tf.tf_gs = tss->tss_gs;
	tf.tf_fs = tss->tss_fs;
	tf.tf_es = tss->__tss_es;
	tf.tf_ds = tss->__tss_ds;
	tf.tf_edi = tss->__tss_edi;
	tf.tf_esi = tss->__tss_esi;
	tf.tf_ebp = tss->tss_ebp;
	tf.tf_ebx = tss->__tss_ebx;
	tf.tf_edx = tss->__tss_edx;
	tf.tf_ecx = tss->__tss_ecx;
	tf.tf_eax = tss->__tss_eax;
	tf.tf_trapno = trapno;
	tf.tf_err = code | TC_TSS;
	tf.tf_eip = tss->__tss_eip;
	tf.tf_cs = tss->__tss_cs;
	tf.tf_eflags = tss->__tss_eflags;
	tf.tf_esp = tss->tss_esp;
	tf.tf_ss = tss->__tss_ss;
	trap(&tf);
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
	struct trapframe *frame;
{
	struct lwp *l = curlwp;
	struct proc *p = l ? l->l_proc : 0;
	int type = frame->tf_trapno;
	struct pcb *pcb;
	extern char fusubail[], kcopy_fault[],
		    resume_iret[], resume_pop_ds[], resume_pop_es[],
		    resume_pop_fs[], resume_pop_gs[],
		    IDTVEC(osyscall)[];
	struct trapframe *vframe;
	ksiginfo_t ksi;
	int resume;
	caddr_t onfault;
	int error;
	uint32_t cr2;

	uvmexp.traps++;

	pcb = (l != NULL) ? &l->l_addr->u_pcb : NULL;
#ifdef DEBUG
	if (trapdebug) {
		printf("trap %d code %x eip %x cs %x/%x eflags %x cr2 %x cpl %x\n",
		    frame->tf_trapno, frame->tf_err, frame->tf_eip,
		    frame->tf_cs, IDXSEL(frame->tf_cs),
		    frame->tf_eflags, rcr2(), curcpu()->ci_ilevel);
		printf("curlwp %p%s", curlwp, curlwp ? " " : "\n");
		if (curlwp)
			printf("pid %d lid %d\n", l->l_proc->p_pid, l->l_lid);
	}
#endif

	if (!KVM86MODE && !KERNELMODE(frame->tf_cs, frame->tf_eflags)) {
		type |= T_USER;
		l->l_md.md_regs = frame;
		pcb->pcb_cr2 = 0;		
	}

	switch (type) {

	default:
	we_re_toast:
#ifdef KSTACK_CHECK_DR0
		if (type == T_TRCTRAP) {
			u_int mask, dr6 = rdr6();

			mask = 1 << 0; /* dr0 */
			if (dr6 & mask) {
				panic("trap on DR0: maybe kernel stack overflow\n");
#if 0
				dr6 &= ~mask;
				ldr6(dr6);
				return;
#endif
			}
		}
#endif
#ifdef DDB
		if (kdb_trap(type, 0, frame))
			return;
#endif
#ifdef KGDB
		if (kgdb_trap(type, frame))
			return;
		else {
			/*
			 * If this is a breakpoint, don't panic
			 * if we're not connected.
			 */
			if (type == T_BPTFLT) {
				printf("kgdb: ignored %s\n", trap_type[type]);
				return;
			}
		}
#endif
		if (frame->tf_trapno < trap_types)
			printf("fatal %s", trap_type[frame->tf_trapno]);
		else
			printf("unknown trap %d", frame->tf_trapno);
		printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");
		printf("trap type %d code %x eip %x cs %x eflags %x cr2 %x ilevel %x\n",
		    type, frame->tf_err, frame->tf_eip, frame->tf_cs,
		    frame->tf_eflags, rcr2(), curcpu()->ci_ilevel);

		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:
#ifdef KVM86
		if (KVM86MODE) {
			kvm86_gpfault(frame);
			return;
		}
#endif
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_TSSFLT:
		if (p == NULL)
			goto we_re_toast;
		/* Check for copyin/copyout fault. */
		if (pcb->pcb_onfault != 0) {
copyefault:
			error = EFAULT;
copyfault:
			frame->tf_eip = (int)pcb->pcb_onfault;
			frame->tf_eax = error;
			return;
		}

		/*
		 * Check for failure during return to user mode.
		 *
		 * We do this by looking at the instruction we faulted on.  The
		 * specific instructions we recognize only happen when
		 * returning from a trap, syscall, or interrupt.
		 *
		 * At this point, there are (at least) two trap frames on
		 * the kernel stack; we presume here that we faulted while
		 * loading our registers out of the outer one.
		 *
		 * The inner frame does not involve a ring crossing, so it
		 * ends right before &frame.tf_esp.  The outer frame has
		 * been partially consumed by the INTRFASTEXIT; exactly
		 * how much depends which register we were popping when we
		 * faulted, so we compute the outer frame address based on
		 * register-dependant offsets computed from &frame.tf_esp
		 * below.  To decide whether this was a kernel-mode or
		 * user-mode error, we look at this outer frame's tf_cs
		 * and tf_eflags, which are (fortunately) not consumed until
		 * the final instruction of INTRFASTEXIT.
		 *
		 * XXX
		 * The heuristic used here will currently fail for the case of
		 * one of the 2 pop instructions faulting when returning from a
		 * a fast interrupt.  This should not be possible.  It can be
		 * fixed by rearranging the trap frame so that the stack format
		 * at this point is the same as on exit from a `slow'
		 * interrupt.
		 */
		switch (*(u_char *)frame->tf_eip) {
		case 0xcf:	/* iret */
			vframe = (void *)((int)&frame->tf_esp -
			    offsetof(struct trapframe, tf_eip));
			resume = (int)resume_iret;
			break;
		case 0x8e:
			switch (*(uint32_t *)frame->tf_eip) {
			case 0x0c245c8e:	/* movl 0xc(%esp,1),%ds */
				resume = (int)resume_pop_ds;
				break;
			case 0x0824448e:	/* movl 0x8(%esp,1),%es */
				resume = (int)resume_pop_es;
				break;
			case 0x0424648e:	/* movl 0x4(%esp,1),%fs */
				resume = (int)resume_pop_fs;
				break;
			case 0x00246c8e:	/* movl 0x0(%esp,1),%gs */
				resume = (int)resume_pop_gs;
				break;
			default:
				goto we_re_toast;
			}
			vframe = (void *)(int)&frame->tf_esp;
			break;
		default:
			goto we_re_toast;
		}
		if (KERNELMODE(vframe->tf_cs, vframe->tf_eflags))
			goto we_re_toast;

		frame->tf_eip = resume;

		/*
		 * clear PSL_NT.  it can be set by userland because setting it
		 * isn't a privileged operation.
		 *
		 * set PSL_I.  otherwise, if SIGSEGV is ignored, we'll
		 * continue to generate traps infinitely with
		 * interrupts disabled.
		 */

		frame->tf_eflags = (frame->tf_eflags & ~PSL_NT) | PSL_I;
		return;

	case T_PROTFLT|T_USER:		/* protection fault */
#ifdef VM86
		if (frame->tf_eflags & PSL_VM) {
			vm86_gpfault(l, type & ~T_USER);
			goto out;
		}
#endif
		/* If pmap_exec_fixup does something, let's retry the trap. */
		if (pmap_exec_fixup(&p->p_vmspace->vm_map, frame,
		    &l->l_addr->u_pcb)) {
			goto out;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		ksi.ksi_code = SEGV_ACCERR;
		goto trapsignal;

	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_ALIGNFLT|T_USER:
	case T_NMI|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_SEGNPFLT|T_USER:
		case T_STKFLT|T_USER:
			ksi.ksi_code = BUS_ADRERR;
			break;
		case T_TSSFLT|T_USER:
		case T_NMI|T_USER:
			ksi.ksi_code = BUS_OBJERR;
			break;
		case T_ALIGNFLT|T_USER:
			ksi.ksi_code = BUS_ADRALN;
			break;
		default:
			KASSERT(1);
			break;
		}
		goto trapsignal;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_PRIVINFLT|T_USER:
			ksi.ksi_code = ILL_PRVOPC;
			break;
		case T_FPOPFLT|T_USER:
			ksi.ksi_code = ILL_COPROC;
			break;
		default:
			ksi.ksi_code = 0;
			break;
		}
		goto trapsignal;

	case T_ASTFLT|T_USER:		/* Allow process switch */
		uvmexp.softs++;
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			KERNEL_PROC_LOCK(l);
			ADDUPROF(p);
			KERNEL_PROC_UNLOCK(l);
		}
		/* Allow a forced task switch. */
		if (curcpu()->ci_want_resched) /* XXX CSE me? */
			preempt(0);
		goto out;

	case T_DNA|T_USER: {
#ifdef MATH_EMULATE
		if (math_emulate(frame, &ksi) == 0) {
			if (frame->tf_eflags & PSL_T)
				goto trace;
			return;
		}
		ksi.ksi_trap = type & ~T_USER;
		goto trapsignal;
#else
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGKILL;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)frame->tf_eip;
		printf("pid %d killed due to lack of floating point\n",
		    p->p_pid);
		goto trapsignal;
#endif
	}

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)frame->tf_eip;
		switch (type) {
		case T_BOUND|T_USER:
		case T_OFLOW|T_USER:
			ksi.ksi_code = FPE_FLTOVF;
			break;
		case T_DIVIDE|T_USER:
			ksi.ksi_code = FPE_FLTDIV;
			break;
		default:
			ksi.ksi_code = 0;
			break;
		}
		goto trapsignal;

	case T_ARITHTRAP|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_trap = frame->tf_err & ~TC_FLAGMASK;
		ksi.ksi_addr = (void *)frame->tf_eip;
		ksi.ksi_code = FPE_INTOVF;
		goto trapsignal;

	case T_PAGEFLT:			/* allow page faults in kernel mode */
		if (l == 0)
			goto we_re_toast;
#ifdef LOCKDEBUG
		/* If we page-fault while in scheduler, we're doomed. */
		if (simple_lock_held(&sched_lock))
			goto we_re_toast;
#endif
		/*
		 * fusubail is used by [fs]uswintr() to prevent page faulting
		 * from inside the profiling interrupt.
		 */
		if (pcb->pcb_onfault == fusubail)
			goto copyefault;

#if 0
		/* XXX - check only applies to 386's and 486's with WP off */
		if (frame->tf_err & PGEX_P)
			goto we_re_toast;
#endif
#ifdef XEN3
		cr2 = HYPERVISOR_shared_info->vcpu_info[0].arch.cr2;
#else
		cr2 = ((uint32_t *)(void *)&frame)[1];
#endif
		KERNEL_LOCK(LK_CANRECURSE|LK_EXCLUSIVE);
		goto faultcommon;

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register struct vm_map *map;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

#ifdef XEN3
		cr2 = HYPERVISOR_shared_info->vcpu_info[0].arch.cr2;
#else
		cr2 = ((uint32_t *)(void *)&frame)[1];
#endif
		KERNEL_PROC_LOCK(l);
		if (l->l_flag & L_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)cr2;
			l->l_flag |= L_SA_PAGEFAULT;
		}
	faultcommon:
		vm = p->p_vmspace;
		if (vm == NULL)
			goto we_re_toast;
		pcb->pcb_cr2 = cr2;
		va = trunc_page((vaddr_t)pcb->pcb_cr2);
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
		if (frame->tf_err & PGEX_W)
			ftype = VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %lx\n", va);
			goto we_re_toast;
		}
#endif

		/* Fault the original page in. */
		onfault = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		error = uvm_fault(map, va, 0, ftype);
		pcb->pcb_onfault = onfault;
		if (error == 0) {
			if (map != kernel_map && (caddr_t)va >= vm->vm_maxsaddr)
				uvm_grow(p, va);

			if (type == T_PAGEFLT) {
				KERNEL_UNLOCK();

				/*
				 * we need to switch pmap now if we're in
				 * the middle of copyin/out.
				 *
				 * but we don't need to do so for kcopy as
				 * it never touch userspace.
				 */

				if (onfault != kcopy_fault &&
				    curcpu()->ci_want_pmapload)
					pmap_load();
				return;
			}
			l->l_flag &= ~L_SA_PAGEFAULT;
			KERNEL_PROC_UNLOCK(l);
			goto out;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)cr2;
		if (error == EACCES) {
			ksi.ksi_code = SEGV_ACCERR;
			error = EFAULT;
		} else {
			ksi.ksi_code = SEGV_MAPERR;
		}

		if (type == T_PAGEFLT) {
			if (pcb->pcb_onfault != 0) {
				KERNEL_UNLOCK();
				goto copyfault;
			}
			printf("uvm_fault(%p, %#lx, 0, %d) -> %#x\n",
			    map, va, ftype, error);
			goto we_re_toast;
		}
		if (error == ENOMEM) {
			ksi.ksi_signo = SIGKILL;
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			       p->p_pid, p->p_comm,
			       p->p_cred && p->p_ucred ?
			       p->p_ucred->cr_uid : -1);
		} else {
			ksi.ksi_signo = SIGSEGV;
		}
		(*p->p_emul->e_trapsignal)(l, &ksi);
		if (type == T_PAGEFLT)
			KERNEL_UNLOCK();
		else {
			l->l_flag &= ~L_SA_PAGEFAULT;
			KERNEL_PROC_UNLOCK(l);
		}
		break;
	}

	case T_TRCTRAP:
		/* Check whether they single-stepped into a lcall. */
		if (frame->tf_eip == (int)IDTVEC(osyscall))
			return;
		if (frame->tf_eip == (int)IDTVEC(osyscall) + 1) {
			frame->tf_eflags &= ~PSL_T;
			return;
		}
		goto we_re_toast;

	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
#ifdef MATH_EMULATE
	trace:
#endif
		/*
		 * Don't go single-stepping into a RAS.
		 */
		if (LIST_EMPTY(&p->p_raslist) ||
		    (ras_lookup(p, (caddr_t)frame->tf_eip) == (caddr_t)-1)) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_trap = type & ~T_USER;
			if (type == (T_BPTFLT|T_USER))
				ksi.ksi_code = TRAP_BRKPT;
			else
				ksi.ksi_code = TRAP_TRACE;
			ksi.ksi_addr = (void *)frame->tf_eip;
			KERNEL_PROC_LOCK(l);
			(*p->p_emul->e_trapsignal)(l, &ksi);
			KERNEL_PROC_UNLOCK(l);
		}
		break;

#if	XXXNISA > 0 || NMCA > 0
	case T_NMI:
#if defined(KGDB) || defined(DDB)
		/* NMI can be hooked up to a pushbutton for debugging */
		printf ("NMI ... going to debugger\n");
#ifdef KGDB

		if (kgdb_trap(type, frame))
			return;
#endif
#ifdef DDB
		if (kdb_trap(type, 0, frame))
			return;
#endif
#endif /* KGDB || DDB */
		/* machine/parity/power fail/"kitchen sink" faults */

#if NMCA > 0
		/* mca_nmi() takes care to call x86_nmi() if appropriate */
		if (mca_nmi() != 0)
			goto we_re_toast;
		else
			return;
#else /* XXXNISA > 0 */
		if (x86_nmi() != 0)
			goto we_re_toast;
		else
			return;
#endif /* NMCA > 0 */
#endif /* XXXNISA > 0 || NMCA > 0 */
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(l);
	return;
trapsignal:
	KERNEL_PROC_LOCK(l);
	(*p->p_emul->e_trapsignal)(l, &ksi);
	KERNEL_PROC_UNLOCK(l);
	userret(l);
}

#if defined(I386_CPU)

#ifdef MULTIPROCESSOR
/* XXX XXX XXX */
#endif
/*
 * Compensate for 386 brain damage (missing URKR)
 */
int
trapwrite(addr)
	unsigned addr;
{
	vaddr_t va;
	struct proc *p;
	struct vmspace *vm;

	va = trunc_page((vaddr_t)addr);
	if (va >= VM_MAXUSER_ADDRESS)
		return 1;

	p = curproc;
	vm = p->p_vmspace;

	if (uvm_fault(&vm->vm_map, va, 0, VM_PROT_WRITE) != 0)
		return 1;

	if ((caddr_t)va >= vm->vm_maxsaddr)
		uvm_grow(p, va);

	return 0;
}
#endif /* I386_CPU */

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

	KERNEL_PROC_UNLOCK(l);

	userret(l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	KERNEL_PROC_UNLOCK(l);

	userret(l);
}
