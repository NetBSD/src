/*	$NetBSD: linux_exec_machdep.c,v 1.17.14.1 2017/12/03 11:36:54 jdolecek Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_exec_machdep.c,v 1.17.14.1 2017/12/03 11:36:54 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_user_ldt.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/vnode.h>
#include <sys/lwp.h>

#include <sys/cpu.h>
#include <machine/vmparam.h>

#include <uvm/uvm.h>

#include <sys/syscallargs.h>

#ifndef DEBUG_LINUX
#define DPRINTF(a)
#else
#define DPRINTF(a)	uprintf a
#endif

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/linux_syscallargs.h>

int
linux_exec_setup_stack(struct lwp *l, struct exec_package *epp)
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

#ifndef	USRSTACK32
#define USRSTACK32	(0x00000000ffffffffL&~PGOFSET)
#endif

	if (epp->ep_flags & EXEC_32) {
		epp->ep_minsaddr = USRSTACK32;
		max_stack_size = MAXSSIZ;
	} else {
		epp->ep_minsaddr = USRSTACK;
		max_stack_size = MAXSSIZ;
	}

	if (epp->ep_minsaddr > LINUX_USRSTACK)
		epp->ep_minsaddr = LINUX_USRSTACK;
#ifdef DEBUG_LINUX
	else {
		/*
		 * Someone needs to make KERNBASE and TEXTADDR
		 * java versions < 1.4.2 need the stack to be
		 * at 0xC0000000
		 */
		uprintf("Cannot setup stack to 0xC0000000, "
		    "java will not work properly\n");
	}
#endif
	epp->ep_maxsaddr = (u_long)STACK_GROW(epp->ep_minsaddr,
		max_stack_size);
	epp->ep_ssize = l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 */
	access_size = epp->ep_ssize;
	access_linear_min = (u_long)STACK_ALLOC(epp->ep_minsaddr, access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long)STACK_ALLOC(STACK_GROW(epp->ep_minsaddr,
	    access_size), noaccess_size);
	if (noaccess_size > 0) {
		NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
		    noaccess_linear_min, NULLVP, 0, VM_PROT_NONE, VMCMD_STACK);
	}
	KASSERT(access_size > 0);
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULLVP, 0, VM_PROT_READ | VM_PROT_WRITE,
	    VMCMD_STACK);

	return 0;
}

int
linux_sys_set_thread_area(struct lwp *l,
    const struct linux_sys_set_thread_area_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_user_desc *) desc;
	} */

	return linux_lwp_setprivate(l, SCARG(uap, desc));
}

int
linux_sys_get_thread_area(struct lwp *l,
    const struct linux_sys_get_thread_area_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_user_desc *) desc;
	} */

	/* glibc doesn't actually call this. */
	return ENOSYS;
}
