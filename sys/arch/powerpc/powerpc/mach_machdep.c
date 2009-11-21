/*	$NetBSD: mach_machdep.c,v 1.28 2009/11/21 17:40:29 rmind Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: mach_machdep.c,v 1.28 2009/11/21 17:40:29 rmind Exp $");

#include "opt_ppcarch.h"
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

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_vm.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/vmparam.h>
#include <machine/mach_machdep.h>
#include <machine/macho_machdep.h>

#include <uvm/uvm_extern.h>

void
mach_host_basic_info(struct mach_host_basic_info *info)
{
	int active, inactive;

	/* XXX fill this  accurately */
	info->max_cpus = 1; /* XXX */
	info->avail_cpus = 1; /* XXX */
	uvm_estimatepageable(&active, &inactive);
	info->memory_size = (active + inactive) * PAGE_SIZE;
	info->cpu_type = MACHO_CPU_TYPE_POWERPC;
	info->cpu_subtype = (mfpvr() >> 16);
}

void
mach_create_thread_child(void *arg)
{
	struct mach_create_thread_child_args *mctc;
	struct lwp *l;
	struct trapframe *tf;
	struct exec_macho_powerpc_thread_state *regs;

#ifdef DEBUG_MACH
	printf("entering mach_create_thread_child\n");
#endif

	mctc = (struct mach_create_thread_child_args *)arg;
	l = mctc->mctc_lwp;

	if (mctc->mctc_flavor != MACHO_POWERPC_THREAD_STATE) {
		mctc->mctc_child_done = 1;
		wakeup(&mctc->mctc_child_done);	
		mutex_enter(proc_lock);
		killproc(l->l_proc, "mach_create_thread_child: unknown flavor");
		mutex_exit(proc_lock);
	}
	
	tf = trapframe(l);
	regs = (struct exec_macho_powerpc_thread_state *)mctc->mctc_state;

	/* 
	 * Set the user static bits correctly, as the
	 * requester might not do it.
	 */
	regs->srr1 |= PSL_USERSET;

	/* 
	 * Call upcallret before setting the register context as it
	 * affects R3, R4 and CR.
	 */
	upcallret(l);

	/* Set requested register context */
	tf->srr0 = regs->srr0;
	tf->srr1 = regs->srr1;
	memcpy(tf->fixreg, &regs->r0, 32 * sizeof(register_t));
	tf->cr = regs->cr;
	tf->xer = regs->xer;
	tf->lr = regs->lr;
	tf->ctr = regs->ctr;
#ifdef PPC_OEA
	tf->tf_xtra[TF_MQ] = regs->mq;
	tf->tf_xtra[TF_VRSAVE] = regs->vrsave;
#endif

	/* Wakeup the parent */
	mctc->mctc_child_done = 1;
	wakeup(&mctc->mctc_child_done);	

#ifdef DEBUG_MACH
	printf("leaving mach_create_thread_child\n");
#endif
	return;
}

int
mach_thread_get_state_machdep(struct lwp *l, int flavor, void *state, int *size)
{
	struct trapframe *tf;

	tf = trapframe(l);

	switch (flavor) {
	case MACH_PPC_THREAD_STATE: {
		struct mach_ppc_thread_state *mpts;

		mpts = (struct mach_ppc_thread_state *)state;
		mpts->srr0 = tf->srr0;
		mpts->srr1 = tf->srr1 & PSL_USERSRR1;
		memcpy(mpts->gpreg, tf->fixreg, 32 * sizeof(register_t));
		mpts->cr = tf->cr;
		mpts->xer = tf->xer;
		mpts->lr = tf->lr;
		mpts->ctr = tf->ctr;
		mpts->mq = 0; /* XXX */
		mpts->vrsave = 0; /* XXX */

		*size = sizeof(*mpts);
		break;
	}

	case MACH_THREAD_STATE_NONE:
		*size = 0;
		break;
		
	case MACH_PPC_FLOAT_STATE: 
	case MACH_PPC_EXCEPTION_STATE:
	case MACH_PPC_VECTOR_STATE:
	default: 
		printf("Unimplemented thread state flavor %d\n", flavor);
		return EINVAL;
		break;
	}

	return 0;
}

int
mach_thread_set_state_machdep(struct lwp *l, int flavor, void *state)
{
	struct trapframe *tf;

	tf = trapframe(l);

	switch (flavor) {
	case MACH_PPC_THREAD_STATE: {
		struct mach_ppc_thread_state *mpts;

		mpts = (struct mach_ppc_thread_state *)state;
		tf->srr0 = mpts->srr0;
		tf->srr1 = mpts->srr1;
		memcpy(tf->fixreg, mpts->gpreg, 32 * sizeof(register_t));
		tf->cr = mpts->cr;
		tf->xer = mpts->xer;
		tf->lr = mpts->lr;
		tf->ctr = mpts->ctr;
  
  		break;
  	}

	case MACH_THREAD_STATE_NONE:
		break;
		
	case MACH_PPC_FLOAT_STATE: 
	case MACH_PPC_EXCEPTION_STATE:
	case MACH_PPC_VECTOR_STATE:
	default: 
		printf("Unimplemented thread state flavor %d\n", flavor);
		return EINVAL;
		break;
	}

	return 0;
}

int 
mach_vm_machine_attribute_machdep(struct lwp *l, vaddr_t addr, size_t size, int *valp)
{
	int error = 0;

	switch (*valp) {
	case MACH_MATTR_VAL_CACHE_FLUSH:
#ifdef DEBUG_MACH
		printf("MACH_MATTR_VAL_CACHE_FLUSH\n");
#endif
		break;

	case MACH_MATTR_VAL_DCACHE_FLUSH:
#ifdef DEBUG_MACH
		printf("MACH_MATTR_VAL_DCACHE_FLUSH\n");
#endif
		break;

	case MACH_MATTR_VAL_ICACHE_FLUSH:
#ifdef DEBUG_MACH
		printf("MACH_MATTR_VAL_ICACHE_FLUSH\n");
#endif
		break;

	case MACH_MATTR_VAL_CACHE_SYNC:
#ifdef DEBUG_MACH
		printf("MACH_MATTR_VAL_CACHE_SYNC\n");
#endif
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

