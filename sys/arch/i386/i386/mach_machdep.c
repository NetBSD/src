/*	$NetBSD: mach_machdep.c,v 1.26 2009/11/21 03:11:00 rmind Exp $	 */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_machdep.c,v 1.26 2009/11/21 03:11:00 rmind Exp $");

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#include "opt_compat_mach.h"
#include "opt_compat_darwin.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>
#include <sys/exec_macho.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_vm.h>
#include <compat/mach/mach_exec.h>
#include <compat/darwin/darwin_exec.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/vm86.h>
#include <machine/vmparam.h>

void mach_trap(struct trapframe);


/*
 * Fast syscall gate trap...
 */
void
mach_trap(struct trapframe frame)
{
	int ok = 0;
	ksiginfo_t ksi;
	struct proc *p = curproc;
	struct lwp *l = curlwp;
	struct mach_emuldata *med;
#ifdef COMPAT_DARWIN
	ok |= p->p_emul == &emul_darwin;
#endif
#ifdef COMPAT_MACH
	ok |= p->p_emul == &emul_mach;
#endif

	if (!ok) {
		DPRINTF(("mach trap %d on bad emulation\n", frame.tf_eax));
		goto abort;
	}

	/*
	 * We only know about the following traps:
	 * 0 mach_pthread_self
	 * 1 mach_pthread_set_self
	 * 2 mach_old_pthread_create
	 * 3 mach_pthread_fast_set_self
	 *
	 * So odd calls take one argument for now 
	 *
	 * Arguments are passed on the stack, and we return values in %eax.
	 */

	/* mach_pthread_self */
	if (frame.tf_eax == 0) {
		/*
		 * After a fork or exec, l_private is not initialized.
		 * We have no way of setting it before, so we keep track
		 * of it being uninitialized with med_dirty_thid.
		 * XXX This is probably not the most efficient way
		 */
		med = p->p_emuldata;
		if (med->med_dirty_thid != 0) {
			l->l_private = NULL;
			med->med_dirty_thid = 0;
		}
		frame.tf_eax = (intptr_t)l->l_private;
		return;
	}
	/* mach_pthread_set_*self */
	if (frame.tf_eax & 1) {
		int error = copyin((void *)frame.tf_esp, &l->l_private,
		    sizeof(l->l_private));
		if (error) {
			DPRINTF(("fast trap %d copyin error %d\n", frame.tf_eax,
			    error));
			goto abort;
		}
		frame.tf_eax = 0;
		return;
	}
	uprintf("unknown mach fast trap %d\n", frame.tf_eax);
abort:
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGILL;
	ksi.ksi_code = ILL_ILLTRP;
	trapsignal(l, &ksi);
	return;
}

void
mach_host_basic_info(struct mach_host_basic_info *info)
{
	int active, inactive;

	/* XXX fill this  accurately */
	info->max_cpus = 1;
	info->avail_cpus = 1;
	uvm_estimatepageable(&active, &inactive);
	info->memory_size = active + inactive;
#undef cpu_type
	info->cpu_type = MACHO_CPU_TYPE_I386;
	switch (cpu_class) {
	case CPUCLASS_386:
		info->cpu_subtype = MACHO_CPU_SUBTYPE_386;
		break;
	case CPUCLASS_486:
		info->cpu_subtype = MACHO_CPU_SUBTYPE_486;
		break;
	case CPUCLASS_586:
		info->cpu_subtype = MACHO_CPU_SUBTYPE_586;
		break;
	case CPUCLASS_686:
		info->cpu_subtype = MACHO_CPU_SUBTYPE_PENTPRO;
		break;
	default:
		uprintf("Undefined CPU class %d", cpu_class);
		info->cpu_subtype = MACHO_CPU_SUBTYPE_I386_ALL;
	}
}

void
mach_create_thread_child(void *arg)
{
	/* 
	 * This is a plain copy of the powerpc version. 
	 * It should be converted to i386 MD bits.
	 */
#ifdef notyet
	struct mach_create_thread_child_args *mctc;
	struct proc *p;
	struct trapframe *tf;
	struct exec_macho_powerpc_thread_state *regs;

	mctc = (struct mach_create_thread_child_args *)arg;
	p = *mctc->mctc_proc;

	if (mctc->mctc_flavor != MACHO_POWERPC_THREAD_STATE) {
		mctc->mctc_child_done = 1;
		wakeup(&mctc->mctc_child_done);	
		mutex_enter(proc_lock);
		killproc(p, "mach_create_thread_child: unknown flavor");
		mutex_exit(proc_lock);
	}
	
	/*
	 * Copy right from parent. Will disaprear
	 * the day we will have struct lwp.  
	 */
	mach_copy_right(p->p_pptr, p); 

	tf = trapframe(p);
	regs = (struct exec_macho_powerpc_thread_state *)mctc->mctc_state;

	/* Security warning */
	if ((regs->srr1 & PSL_USERSTATIC) != (tf->srr1 & PSL_USERSTATIC))
		uprintf("mach_create_thread_child: PSL_USERSTATIC change\n");		
	/* 
	 * Call child return before setting the register context as it
	 * affects R3, R4 and CR.
	 */
	child_return((void *)p);

	/* Set requested register context */
	tf->srr0 = regs->srr0;
	tf->srr1 = ((regs->srr1 & ~PSL_USERSTATIC) | 
	    (tf->srr1 & PSL_USERSTATIC));
	memcpy(tf->fixreg, &regs->r0, 32 * sizeof(register_t));
	tf->cr = regs->cr;
	tf->xer = regs->xer;
	tf->lr = regs->lr;
	tf->ctr = regs->ctr;
	/* XXX regs->mq ? (601 specific) */
	tf->vrsave = regs->vrsave;

	/* Wakeup the parent */
	mctc->mctc_child_done = 1;
	wakeup(&mctc->mctc_child_done);	
#else
	printf("mach_create_thread_child %p\n", arg);
#endif
}

int
mach_thread_get_state_machdep(struct lwp *l, int flavor,
    void *state, int *size)
{
	printf("Unimplemented thread state flavor %d\n", flavor);
	return EINVAL;
}

int
mach_thread_set_state_machdep(struct lwp *l, int flavor,
    void *state)
{
	printf("Unimplemented thread state flavor %d\n", flavor);
	return EINVAL;
}

int
mach_vm_machine_attribute_machdep(struct lwp *l, vaddr_t v,
    size_t s, int *ip)
{
	return 0;
}

