/*	$NetBSD: mach_machdep.c,v 1.15 2005/06/25 07:46:01 christos Exp $	 */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_machdep.c,v 1.15 2005/06/25 07:46:01 christos Exp $");

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
#include <sys/user.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>
#include <sys/exec_macho.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_vm.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
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
mach_trap(frame)
	struct trapframe frame;
{
	int ok = 0;
#ifdef COMPAT_DARWIN
	extern struct emul emul_darwin;
#endif
#ifdef COMPAT_MACH
	extern struct emul emul_mach;
#endif
#ifdef COMPAT_DARWIN
	ok |= curproc->p_emul == &emul_darwin;
#endif
#ifdef COMPAT_MACH
	ok |= curproc->p_emul == &emul_mach;
#endif

	if (!ok) {
		ksiginfo_t ksi;
		DPRINTF(("mach trap %d on bad emulation\n", frame.tf_eax));
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLTRP;
		trapsignal(curlwp, &ksi);
		return;
	}

	switch (frame.tf_eax) {
	case 0:
		DPRINTF(("mach_pthread_self();\n"));
		break;
	case 1:
		DPRINTF(("mach_pthread_set_self();\n"));
		break;
	default:
		uprintf("unknown mach trap %d\n", frame.tf_eax);
		break;
	}
}

void
mach_host_basic_info(info)
    struct mach_host_basic_info *info;
{
	/* XXX fill this  accurately */
	info->max_cpus = 1;
	info->avail_cpus = 1;
	info->memory_size = uvmexp.active + uvmexp.inactive;
#undef cpu_type
	info->cpu_type = MACHO_CPU_TYPE_I386;
	switch (cpu_info_primary.ci_cpu_class) {
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
		uprintf("Undefined CPU class %d",
		    cpu_info_primary.ci_cpu_class);
		info->cpu_subtype = MACHO_CPU_SUBTYPE_I386_ALL;
	}
}

void
mach_create_thread_child(arg)
	void *arg;
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
		killproc(p, "mach_create_thread_child: unknown flavor");
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
mach_thread_get_state_machdep(struct lwp *l, int flavor, void *state,
    int *size)
{
	printf("Unimplemented thread state flavor %d\n", flavor);
	return EINVAL;
}

int
mach_thread_set_state_machdep(struct lwp *l, int flavor, void *state)
{
	printf("Unimplemented thread state flavor %d\n", flavor);
	return EINVAL;
}

int
mach_vm_machine_attribute_machdep(struct lwp *l, vaddr_t v, size_t s, int *ip)
{
	return 0;
}

