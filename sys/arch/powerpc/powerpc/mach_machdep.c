/*	$NetBSD: mach_machdep.c,v 1.15 2003/09/30 21:04:54 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_machdep.c,v 1.15 2003/09/30 21:04:54 manu Exp $");

#include "opt_ppcarch.h"
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

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>
#include <compat/mach/mach_thread.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/vmparam.h>
#include <machine/macho_machdep.h>

#include <uvm/uvm_extern.h>

void
mach_host_basic_info(struct mach_host_basic_info *info)
{
	/* XXX fill this  accurately */
	info->max_cpus = 1; /* XXX */
	info->avail_cpus = 1; /* XXX */
	info->memory_size = (uvmexp.active + uvmexp.inactive) * PAGE_SIZE;
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
		killproc(l->l_proc, "mach_create_thread_child: unknown flavor");
	}
	
	tf = trapframe(l);
	regs = (struct exec_macho_powerpc_thread_state *)mctc->mctc_state;

	/* Security warning */
	if ((regs->srr1 & PSL_USERSTATIC) != (tf->srr1 & PSL_USERSTATIC))
		uprintf("mach_create_thread_child: PSL_USERSTATIC change\n");		
	/* 
	 * Call upcallret before setting the register context as it
	 * affects R3, R4 and CR.
	 */
	upcallret(l);

	/* Set requested register context */
	tf->srr0 = regs->srr0;
	tf->srr1 = ((regs->srr1 & ~PSL_USERSTATIC) | 
	    (tf->srr1 & PSL_USERSTATIC));
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
