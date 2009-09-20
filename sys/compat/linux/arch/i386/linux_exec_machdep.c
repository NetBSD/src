/*	$NetBSD: linux_exec_machdep.c,v 1.13 2009/09/20 10:29:30 taca Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_exec_machdep.c,v 1.13 2009/09/20 10:29:30 taca Exp $");

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
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
#include <compat/linux//linux_syscallargs.h>


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


#ifdef LINUX_NPTL
static __inline void
load_gs(u_int sel)
{
        __asm __volatile("movw %0,%%gs" : : "rm" ((unsigned short)sel)); 
}


int
linux_init_thread_area(struct lwp *l, struct lwp *l2)
{
	struct trapframe *tf = l->l_md.md_regs, *tf2 = l2->l_md.md_regs;
	struct pcb *pcb2 = &l2->l_addr->u_pcb;
	struct linux_user_desc info;
	struct segment_descriptor sd;
	int error, idx, a[2];

	error = copyin((void *)tf->tf_esi, &info, sizeof(info));
	if (error)
		return error;
	idx = info.entry_number;

	/* 
	 * looks like we're getting the idx we returned
	 * in the set_thread_area() syscall
	 */
	if (idx != LINUX_GLIBC_TLS_SEL && idx != GUGS_SEL) {
		DPRINTF(("resetting idx %d to GUGS_SEL", idx));
		idx = GUGS_SEL;
	}

	/* this doesnt happen in practice */
	if (idx == LINUX_GLIBC_TLS_SEL) {
		/* we might copy out the entry_number as 3 */
		info.entry_number = GUGS_SEL;
		error = copyout(&info, (void *)tf->tf_esi, sizeof(info));
		if (error)
			return error;
	}

	a[0] = LINUX_LDT_entry_a(&info);
	a[1] = LINUX_LDT_entry_b(&info);

	(void)memcpy(&sd, &a, sizeof(a));
	KASSERT(ISMEMSDP((&sd)));
	DPRINTF(("Segment created in clone with CLONE_SETTLS: lobase: %x, "
	    "hibase: %x, lolimit: %x, hilimit: %x, type: %i, dpl: %i, p: %i, "
	    "xx: %i, def32: %i, gran: %i\n", sd.sd_lobase,
	    sd.sd_hibase, sd.sd_lolimit, sd.sd_hilimit, sd.sd_type, sd.sd_dpl,
	    sd.sd_p, sd.sd_xx, sd.sd_def32, sd.sd_gran));

	(void)memcpy(&pcb2->pcb_gsd, &sd, sizeof(sd));
	tf2->tf_gs = GSEL(GUGS_SEL, SEL_UPL);

	return 0;
}


int
linux_sys_set_thread_area(struct lwp *l,
    const struct linux_sys_set_thread_area_args *uap, register_t *retval)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct linux_user_desc info;
	struct segment_descriptor sd;
	int error, idx, a[2];

	*retval = 0;
	error = copyin(SCARG(uap, desc), &info, sizeof(info));
	if (error)
		return error;

	DPRINTF(("set thread area: %i, %x, %x, %i, %i, %i, %i, %i, %i\n",
	    info.entry_number, info.base_addr, info.limit, info.seg_32bit,
	    info.contents, info.read_exec_only, info.limit_in_pages,
	    info.seg_not_present, info.useable));

	idx = info.entry_number;
	/* 
	 * Semantics of linux version: every thread in the system has array of
	 * 3 tls descriptors. 1st is GLIBC TLS, 2nd is WINE, 3rd unknown. This 
	 * syscall loads one of the selected tls decriptors with a value and
	 * also loads GDT descriptors 6, 7 and 8 with the content of the
	 * per-thread descriptors.
	 *
	 * Semantics of fbsd version: I think we can ignore that linux has 3 
	 * per-thread descriptors and use just the 1st one. The tls_array[]
	 * is used only in set/get-thread_area() syscalls and for loading the
	 * GDT descriptors. In fbsd we use just one GDT descriptor for TLS so
	 * we will load just one. 
	 *
	 * XXX: this doesn't work when a user space process tries to use more
	 * than 1 TLS segment. Comment in the linux sources says wine might do
	 * this.
	 */

	/* 
	 * we support just GLIBC TLS now 
	 * we should let 3 proceed as well because we use this segment so
	 * if code does two subsequent calls it should succeed
	 */
	if (idx != LINUX_GLIBC_TLS_SEL && idx != -1 && idx != GUGS_SEL)
		return EINVAL;

	/* 
	 * we have to copy out the GDT entry we use
	 * FreeBSD uses GDT entry #3 for storing %gs so load that
	 *
	 * XXX: what if a user space program doesn't check this value and tries
	 * to use 6, 7 or 8? 
	 */
	idx = info.entry_number = GUGS_SEL;
	error = copyout(&info, SCARG(uap, desc), sizeof(info));
	if (error)
		return error;

	if (LINUX_LDT_empty(&info)) {
		a[0] = 0;
		a[1] = 0;
	} else {
		a[0] = LINUX_LDT_entry_a(&info);
		a[1] = LINUX_LDT_entry_b(&info);
	}

	(void)memcpy(&sd, &a, sizeof(a));
	KASSERT(ISMEMSDP((&sd)));
	DPRINTF(("Segment created in set_thread_area: lobase: %x, hibase: %x, "
	    "lolimit: %x, hilimit: %x, type: %i, dpl: %i, p: %i, xx: %i, "
	    "def32: %i, gran: %i\n", sd.sd_lobase, sd.sd_hibase, sd.sd_lolimit,
	    sd.sd_hilimit, sd.sd_type, sd.sd_dpl, sd.sd_p, sd.sd_xx,
	    sd.sd_def32, sd.sd_gran));

	kpreempt_disable();
	(void)memcpy(&pcb->pcb_gsd, &sd, sizeof(sd));
	(void)memcpy(&curcpu()->ci_gdt[GUGS_SEL], &sd, sizeof(sd));
	load_gs(GSEL(GUGS_SEL, SEL_UPL));
	kpreempt_enable();
	return 0;
}

int
linux_sys_get_thread_area(struct lwp *l,
    const struct linux_sys_get_thread_area_args *uap, register_t *retval)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct linux_user_desc info;
	struct linux_desc_struct desc;
	struct segment_descriptor sd;
	int error, idx;

	*retval = 0;
	error = copyin(SCARG(uap, desc), &info, sizeof(info));
	if (error)
		return error;

	idx = info.entry_number;
	/* XXX: I am not sure if we want 3 to be allowed too. */
	if (idx != LINUX_GLIBC_TLS_SEL && idx != GUGS_SEL)
		return EINVAL;

	idx = GUGS_SEL;

	(void)memset(&info, 0, sizeof(info));
	(void)memcpy(&sd, pcb->pcb_gsd, sizeof(sd));
	(void)memcpy(&desc, &sd, sizeof(desc));

	info.entry_number = idx;
	info.base_addr = LINUX_GET_BASE(&desc);
	info.limit = LINUX_GET_LIMIT(&desc);
	info.seg_32bit = LINUX_GET_32BIT(&desc);
	info.contents = LINUX_GET_CONTENTS(&desc);
	info.read_exec_only = !LINUX_GET_WRITABLE(&desc);
	info.limit_in_pages = LINUX_GET_LIMIT_PAGES(&desc);
	info.seg_not_present = !LINUX_GET_PRESENT(&desc);
	info.useable = LINUX_GET_USEABLE(&desc);

	return copyout(&info, SCARG(uap, desc), sizeof(info));
}

#endif
