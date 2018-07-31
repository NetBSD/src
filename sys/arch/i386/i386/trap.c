
/*	$NetBSD: trap.c,v 1.287.6.2 2018/07/31 15:57:11 martin Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2005, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.287.6.2 2018/07/31 15:57:11 martin Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_vm86.h"
#include "opt_xen.h"
#include "opt_dtrace.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/ras.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/cpu.h>
#include <sys/ucontext.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>
#include <machine/db_machdep.h>

#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>
#endif

#include <x86/nmi.h>

#include "isa.h"

#include <sys/kgdb.h>

#ifdef KDTRACE_HOOKS
#include <sys/dtrace_bsd.h>

/*
 * This is a hook which is initialized by the dtrace module
 * to handle traps which might occur during DTrace probe
 * execution.
 */
dtrace_trap_func_t	dtrace_trap_func = NULL;

dtrace_doubletrap_func_t	dtrace_doubletrap_func = NULL;
#endif

void trap(struct trapframe *);
void trap_tss(struct i386tss *, int, int);
void trap_return_fault_return(struct trapframe *) __dead;

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
int	trap_types = __arraycount(trap_type);

#ifdef DEBUG
int	trapdebug = 0;
#endif

#define	IDTVEC(name)	__CONCAT(X, name)

#ifdef TRAP_SIGDEBUG
static void frame_dump(struct trapframe *, struct pcb *);
#endif

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

static void *
onfault_handler(const struct pcb *pcb, const struct trapframe *tf)
{
	struct onfault_table {
		uintptr_t start;
		uintptr_t end;
		void *handler;
	};
	extern const struct onfault_table onfault_table[];
	const struct onfault_table *p;
	uintptr_t pc;

	if (pcb->pcb_onfault != NULL) {
		return pcb->pcb_onfault;
	}

	pc = tf->tf_eip;
	for (p = onfault_table; p->start; p++) {
		if (p->start <= pc && pc < p->end) {
			return p->handler;
		}
	}
	return NULL;
}

static void
trap_print(const struct trapframe *frame, const lwp_t *l)
{
	const int type = frame->tf_trapno;

	if (frame->tf_trapno < trap_types) {
		printf("fatal %s", trap_type[type]);
	} else {
		printf("unknown trap %d", type);
	}
	printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");

	printf("trap type %d code %#x eip %#x cs %#x eflags %#x cr2 %#lx "
	    "ilevel %#x esp %#x\n",
	    type, frame->tf_err, frame->tf_eip, frame->tf_cs, frame->tf_eflags,
	    (long)rcr2(), curcpu()->ci_ilevel, frame->tf_esp);

	printf("curlwp %p pid %d lid %d lowest kstack %p\n",
	    l, l->l_proc->p_pid, l->l_lid, KSTACK_LOWEST_ADDR(l));
}

/*
 * trap(frame): exception, fault, and trap interface to BSD kernel.
 *
 * This common code is called from assembly language IDT gate entry routines
 * that prepare a suitable stack frame, and restore this frame after the
 * exception has been processed. Note that the effect is as if the arguments
 * were passed call by reference.
 */
void
trap(struct trapframe *frame)
{
	struct lwp *l = curlwp;
	struct proc *p;
	struct pcb *pcb;
	extern char fusubail[], kcopy_fault[], return_address_fault[];
	struct trapframe *vframe;
	ksiginfo_t ksi;
	void *onfault;
	int type, error;
	uint32_t cr2;
	bool pfail;

	if (__predict_true(l != NULL)) {
		pcb = lwp_getpcb(l);
		p = l->l_proc;
	} else {
		/*
		 * this can happen eg. on break points in early on boot.
		 */
		pcb = NULL;
		p = NULL;
	}
	type = frame->tf_trapno;

#ifdef DEBUG
	if (trapdebug) {
		trap_print(frame, l);
	}
#endif
	if (type != T_NMI && !KERNELMODE(frame->tf_cs, frame->tf_eflags)) {
		type |= T_USER;
		l->l_md.md_regs = frame;
		pcb->pcb_cr2 = 0;
		LWP_CACHE_CREDS(l, p);
	}

#ifdef KDTRACE_HOOKS
	/*
	 * A trap can occur while DTrace executes a probe. Before
	 * executing the probe, DTrace blocks re-scheduling and sets
	 * a flag in its per-cpu flags to indicate that it doesn't
	 * want to fault. On returning from the the probe, the no-fault
	 * flag is cleared and finally re-scheduling is enabled.
	 *
	 * If the DTrace kernel module has registered a trap handler,
	 * call it and if it returns non-zero, assume that it has
	 * handled the trap and modified the trap frame so that this
	 * function can return normally.
	 */
	if ((type == T_PROTFLT || type == T_PAGEFLT) &&
	    dtrace_trap_func != NULL) {
		if ((*dtrace_trap_func)(frame, type)) {
			return;
		}
	}
#endif

	switch (type) {

	default:
	we_re_toast:
		if (type != T_TRCTRAP)
			trap_print(frame, l);

		if (kdb_trap(type, 0, frame))
			return;
		if (kgdb_trap(type, frame))
			return;
		/*
		 * If this is a breakpoint, don't panic if we're not connected.
		 */
		if (type == T_BPTFLT && kgdb_disconnected()) {
			printf("kgdb: ignored %s\n", trap_type[type]);
			return;
		}
		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_STKFLT:
	case T_TSSFLT:
		if (p == NULL)
			goto we_re_toast;
		/* Check for copyin/copyout fault. */
		onfault = onfault_handler(pcb, frame);
		if (onfault != NULL) {
copyefault:
			error = EFAULT;
copyfault:
			frame->tf_eip = (uintptr_t)onfault;
			frame->tf_eax = error;
			return;
		}

		/*
		 * Check for failure during return to user mode.
		 * This can happen loading invalid values into the segment
		 * registers, or during the 'iret' itself.
		 *
		 * We do this by looking at the instruction we faulted on.
		 * The specific instructions we recognize only happen when
		 * returning from a trap, syscall, or interrupt.
		 */

kernelfault:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = SEGV_ACCERR;
		ksi.ksi_trap = type;

		switch (*(u_char *)frame->tf_eip) {
		case 0xcf:	/* iret */
			/*
			 * The 'iret' instruction faulted, so we have the
			 * 'user' registers saved after the kernel %eip:%cs:%fl
			 * of the 'iret' and below that the user %eip:%cs:%fl
			 * the 'iret' was processing.
			 * We must delete the 3 words of kernel return address
			 * from the stack to generate a normal stack frame
			 * (eg for sending a SIGSEGV).
			 */
			vframe = (void *)((int *)frame + 3);
			if (KERNELMODE(vframe->tf_cs, vframe->tf_eflags))
				goto we_re_toast;
			memmove(vframe, frame,
			    offsetof(struct trapframe, tf_eip));
			/* Set the faulting address to the user %eip */
			ksi.ksi_addr = (void *)vframe->tf_eip;
			break;
		case 0x8e:
			switch (*(uint32_t *)frame->tf_eip) {
			case 0x8e242c8e:	/* mov (%esp,%gs), then */
			case 0x0424648e:	/* mov 0x4(%esp),%fs */
			case 0x0824448e:	/* mov 0x8(%esp),%es */
			case 0x0c245c8e:	/* mov 0xc(%esp),%ds */
				break;
			default:
				goto we_re_toast;
			}
			/*
			 * We faulted loading one of the user segment registers.
			 * The stack frame containing the user registers is
			 * still valid and is just below the %eip:%cs:%fl of
			 * the kernel fault frame.
			 */
			vframe = (void *)(&frame->tf_eflags + 1);
			if (KERNELMODE(vframe->tf_cs, vframe->tf_eflags))
				goto we_re_toast;
			/* There is no valid address for the fault */
			break;
		default:
			goto we_re_toast;
		}
		/*
		 * We might have faulted trying to execute the
		 * trampoline for a local (nested) signal handler.
		 * Only generate SIGSEGV if the user %cs isn't changed.
		 * (This is only strictly necessary in the 'iret' case.)
		 */
		if (!pmap_exec_fixup(&p->p_vmspace->vm_map, vframe, pcb)) {
			/* Save outer frame for any signal return */
			l->l_md.md_regs = vframe;
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
		/* Return to user by reloading the user frame */
		trap_return_fault_return(vframe);
		/* NOTREACHED */

	case T_PROTFLT|T_USER:		/* protection fault */
#if defined(COMPAT_10)
	{
		static const char lcall[7] = { 0x9a, 0, 0, 0, 0, 7, 0 };
		const size_t sz = sizeof(lcall);
		char tmp[sizeof(lcall)];

		/* Check for the osyscall lcall instruction. */
		if (frame->tf_eip < VM_MAXUSER_ADDRESS - sz &&
		    copyin((void *)frame->tf_eip, tmp, sz) == 0 &&
		    memcmp(tmp, lcall, sz) == 0) {

			/* Advance past the lcall. */
			frame->tf_eip += sz;

			/* Do the syscall. */
			p->p_md.md_syscall(frame);
			goto out;
		}
	}
#endif
	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:
	case T_ALIGNFLT|T_USER:
#ifdef TRAP_SIGDEBUG
		printf("pid %d.%d (%s): BUS/SEGV (%#x) at eip %#x addr %#lx\n",
		    p->p_pid, l->l_lid, p->p_comm, type, frame->tf_eip, rcr2());
		frame_dump(frame, pcb);
#endif
		KSI_INIT_TRAP(&ksi);

		ksi.ksi_addr = (void *)rcr2();
		switch (type) {
		case T_SEGNPFLT|T_USER:
		case T_STKFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case T_TSSFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_OBJERR;
			break;
		case T_ALIGNFLT|T_USER:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRALN;
			break;
		case T_PROTFLT|T_USER:
#ifdef VM86
			if (frame->tf_eflags & PSL_VM) {
				vm86_gpfault(l, type & ~T_USER);
				goto out;
			}
#endif
			/*
			 * If pmap_exec_fixup does something,
			 * let's retry the trap.
			 */
			if (pmap_exec_fixup(&p->p_vmspace->vm_map, frame, pcb)){
				goto out;
			}
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			break;
		default:
			KASSERT(0);
			break;
		}
		goto trapsignal;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
#ifdef TRAP_SIGDEBUG
		printf("pid %d.%d (%s): ILL at eip %#x addr %#lx\n",
		    p->p_pid, l->l_lid, p->p_comm, frame->tf_eip, rcr2());
		frame_dump(frame, pcb);
#endif
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_addr = (void *) frame->tf_eip;
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

	case T_ASTFLT|T_USER:
		/* Allow process switch. */
		//curcpu()->ci_data.cpu_nast++;
		if (l->l_pflag & LP_OWEUPC) {
			l->l_pflag &= ~LP_OWEUPC;
			ADDUPROF(l);
		}
		/* Allow a forced task switch. */
		if (curcpu()->ci_want_resched) {
			preempt();
		}
		goto out;

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_DIVIDE|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_addr = (void *)frame->tf_eip;
		switch (type) {
		case T_BOUND|T_USER:
			ksi.ksi_code = FPE_FLTSUB;
			break;
		case T_OFLOW|T_USER:
			ksi.ksi_code = FPE_INTOVF;
			break;
		case T_DIVIDE|T_USER:
			ksi.ksi_code = FPE_INTDIV;
			break;
		default:
			ksi.ksi_code = 0;
			break;
		}
		goto trapsignal;

	case T_PAGEFLT:
		/* Allow page faults in kernel mode. */
		if (__predict_false(l == NULL))
			goto we_re_toast;

		/*
		 * fusubail is used by [fs]uswintr() to prevent page faulting
		 * from inside the profiling interrupt.
		 */
		onfault = pcb->pcb_onfault;
		if (onfault == fusubail || onfault == return_address_fault) {
			goto copyefault;
		}
		if (cpu_intr_p() || (l->l_pflag & LP_INTR) != 0) {
			goto we_re_toast;
		}

		cr2 = rcr2();

		if (frame->tf_err & PGEX_X) {
			/* SMEP might have brought us here */
			if (cr2 > VM_MIN_ADDRESS && cr2 <= VM_MAXUSER_ADDRESS)
				panic("prevented execution of %p (SMEP)",
				    (void *)cr2);
		}

		goto faultcommon;

	case T_PAGEFLT|T_USER: {	/* page fault */
		register vaddr_t va;
		register struct vmspace *vm;
		register struct vm_map *map;
		vm_prot_t ftype;
		extern struct vm_map *kernel_map;

		cr2 = rcr2();
faultcommon:
		vm = p->p_vmspace;
		if (__predict_false(vm == NULL)) {
			goto we_re_toast;
		}
		pcb->pcb_cr2 = cr2;
		va = trunc_page((vaddr_t)cr2);
		/*
		 * It is only a kernel address space fault iff:
		 *	1. (type & T_USER) == 0  and
		 *	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= VM_MIN_KERNEL_ADDRESS)
			map = kernel_map;
		else
			map = &vm->vm_map;
		if (frame->tf_err & PGEX_W)
			ftype = VM_PROT_WRITE;
		else if (frame->tf_err & PGEX_X)
			ftype = VM_PROT_EXECUTE;
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
		error = uvm_fault(map, va, ftype);
		pcb->pcb_onfault = onfault;
		if (error == 0) {
			if (map != kernel_map && (void *)va >= vm->vm_maxsaddr)
				uvm_grow(p, va);

			pfail = false;
			while (type == T_PAGEFLT) {
				/*
				 * we need to switch pmap now if we're in
				 * the middle of copyin/out.
				 *
				 * but we don't need to do so for kcopy as
				 * it never touch userspace.
 				 */
				kpreempt_disable();
				if (curcpu()->ci_want_pmapload) {
					onfault = onfault_handler(pcb, frame);
					if (onfault != kcopy_fault) {
						pmap_load();
					}
				}
				/*
				 * We need to keep the pmap loaded and
				 * so avoid being preempted until back
				 * into the copy functions.  Disable
				 * interrupts at the hardware level before
				 * re-enabling preemption.  Interrupts
				 * will be re-enabled by 'iret' when
				 * returning back out of the trap stub.
				 * They'll only be re-enabled when the
				 * program counter is once again in
				 * the copy functions, and so visible
				 * to cpu_kpreempt_exit().
				 */
#ifndef XEN
				x86_disable_intr();
#endif
				l->l_nopreempt--;
				if (l->l_nopreempt > 0 || !l->l_dopreempt ||
				    pfail) {
					return;
				}
#ifndef XEN
				x86_enable_intr();
#endif
				/*
				 * If preemption fails for some reason,
				 * don't retry it.  The conditions won't
				 * change under our nose.
				 */
				pfail = kpreempt(0);
			}
			goto out;
		}

		if (type == T_PAGEFLT) {
			onfault = onfault_handler(pcb, frame);
			if (onfault != NULL)
				goto copyfault;
			printf("uvm_fault(%p, %#lx, %d) -> %#x\n",
			    map, va, ftype, error);
			goto kernelfault;
		}

		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = type & ~T_USER;
		ksi.ksi_addr = (void *)cr2;
		switch (error) {
		case EINVAL:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case EACCES:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			error = EFAULT;
			break;
		case ENOMEM:
			ksi.ksi_signo = SIGKILL;
			printf("UVM: pid %d.%d (%s), uid %d killed: "
			    "out of swap\n", p->p_pid, l->l_lid, p->p_comm,
			    l->l_cred ?  kauth_cred_geteuid(l->l_cred) : -1);
			break;
		default:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
			break;
		}

#ifdef TRAP_SIGDEBUG
		printf("pid %d.%d (%s): signal %d at eip %#x addr %#lx "
		    "error %d\n", p->p_pid, l->l_lid, p->p_comm, ksi.ksi_signo,
		    frame->tf_eip, va, error);
		frame_dump(frame, pcb);
#endif
		(*p->p_emul->e_trapsignal)(l, &ksi);
		break;
	}

	case T_TRCTRAP:
		/*
		 * Ignore debug register trace traps due to
		 * accesses in the user's address space, which
		 * can happen under several conditions such as
		 * if a user sets a watchpoint on a buffer and
		 * then passes that buffer to a system call.
		 * We still want to get TRCTRAPS for addresses
		 * in kernel space because that is useful when
		 * debugging the kernel.
		 */
		if (x86_dbregs_user_trap())
			break;

		goto we_re_toast;

	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
		/*
		 * Don't go single-stepping into a RAS.
		 */
		if (p->p_raslist == NULL ||
		    (ras_lookup(p, (void *)frame->tf_eip) == (void *)-1)) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGTRAP;
			ksi.ksi_trap = type & ~T_USER;
			if (x86_dbregs_user_trap()) {
				x86_dbregs_store_dr6(l);
				ksi.ksi_code = TRAP_DBREG;
			} else if (type == (T_BPTFLT|T_USER))
				ksi.ksi_code = TRAP_BRKPT;
			else
				ksi.ksi_code = TRAP_TRACE;
			ksi.ksi_addr = (void *)frame->tf_eip;
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
		break;

	case T_NMI:
		if (nmi_dispatch(frame))
			return;
		/* NMI can be hooked up to a pushbutton for debugging */
		if (kgdb_trap(type, frame))
			return;
		if (kdb_trap(type, 0, frame))
			return;
		/* machine/parity/power fail/"kitchen sink" faults */
#if NMCA > 0
		mca_nmi();
#endif
		x86_nmi();
	}

	if ((type & T_USER) == 0)
		return;
out:
	userret(l);
	return;
trapsignal:
	ksi.ksi_trap = type & ~T_USER;
	(*p->p_emul->e_trapsignal)(l, &ksi);
	userret(l);
}

/* 
 * startlwp: start of a new LWP.
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	lwp_t *l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}

#ifdef TRAP_SIGDEBUG
void
frame_dump(struct trapframe *tf, struct pcb *pcb)
{
	int i;
	unsigned long *p;
	uint64_t fsd, gsd;

	printf("trapframe %p\n", tf);
	printf("eip 0x%08x  esp 0x%08x  efl 0x%08x\n",
	    tf->tf_eip, tf->tf_esp, tf->tf_eflags);
	printf("edi 0x%08x  esi 0x%08x  edx 0x%08x\n",
	    tf->tf_edi, tf->tf_esi, tf->tf_edx);
	printf("ecx 0x%08x\n",
	    tf->tf_ecx);
	printf("ebp 0x%08x  ebx 0x%08x  eax 0x%08x\n",
	    tf->tf_ebp, tf->tf_ebx, tf->tf_eax);
	printf("cs 0x%04x  ds 0x%04x  es 0x%04x  "
	       "fs 0x%04x  gs 0x%04x  ss 0x%04x\n",
		tf->tf_cs & 0xffff, tf->tf_ds & 0xffff, tf->tf_es & 0xffff,
		tf->tf_fs & 0xffff, tf->tf_gs & 0xffff, tf->tf_ss & 0xffff);
	memcpy(&fsd, &pcb->pcb_fsd, sizeof(fsd));
	memcpy(&gsd, &pcb->pcb_gsd, sizeof(gsd));
	printf("fsbase 0x%016llx gsbase 0x%016llx\n", fsd, gsd);
	printf("\n");
	printf("Stack dump:\n");
	for (i = 0, p = (unsigned long *) tf; i < 20; i ++, p += 8)
		printf(" 0x%.8lx 0x%.8lx 0x%.8lx 0x%.8lx"
		       " 0x%.8lx 0x%.8lx 0x%.8lx 0x%.8lx\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	printf("\n");
}
#endif
