/*	$NetBSD: linux32_exec_machdep.c,v 1.2 2023/04/09 12:29:26 riastradh Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1996 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_exec_machdep.c,v 1.2 2023/04/09 12:29:26 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/syscallargs.h>
#include <sys/vnode.h>

#include <machine/vmparam.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux32/common/linux32_exec.h>

#ifdef LINUX32_ARM_KUSER_HELPER_ADDR
static int
vmcmd_linux32_kuser_helper_map(struct lwp *l, struct exec_vmcmd *cmd)
{
	const struct proc *p = l->l_proc;
	/* l->l_proc->p_emul still points to emul_netbsd at this point */
	const struct emul *e = &emul_linux32;
	struct uvm_object *uobj;
	vsize_t sz;
	vaddr_t va;
	int error;

	/*
	 * kuser_helper code share the page prepared for sigcode.
	 * See also sys/compat/linux32/arch/aarch64/linux32_sigcode.S
	 */
	if (e->e_sigobject == NULL)
		return 0;
	uobj = *e->e_sigobject;
	if (uobj == NULL)
		return 0;

	va = LINUX32_ARM_KUSER_HELPER_ADDR;
	sz = LINUX32_ARM_KUSER_HELPER_SIZE;

	(*uobj->pgops->pgo_reference)(uobj);
	error = uvm_map(&p->p_vmspace->vm_map, &va, round_page(sz), uobj, 0, 0,
	    UVM_MAPFLAG(UVM_PROT_RX, UVM_PROT_RX,
	    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
	if (error) {
		(*uobj->pgops->pgo_detach)(uobj);
		return error;
	}

	return 0;
}
#endif

int
linux32_exec_setup_stack(struct lwp *l, struct exec_package *epp)
{
	vaddr_t access_linear_min, noaccess_linear_min;
	vsize_t access_size, noaccess_size;
	vsize_t max_stack_size;

#ifndef LINUX32_USRSTACK
#define LINUX32_USRSTACK	USRSTACK32
#endif
#ifndef LINUX32_MAXSSIZ
#define LINUX32_MAXSSIZ		MAXSSIZ32
#endif

	KASSERT((epp->ep_flags & EXEC_32) != 0);
	epp->ep_minsaddr = LINUX32_USRSTACK;
	max_stack_size = LINUX32_MAXSSIZ;

	epp->ep_ssize =
	    MIN(l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur, max_stack_size);
	epp->ep_maxsaddr =
	    (vaddr_t)STACK_GROW(epp->ep_minsaddr, max_stack_size);

	l->l_proc->p_stackbase = epp->ep_minsaddr;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 */
	access_size = epp->ep_ssize;
	access_linear_min = (vaddr_t)STACK_ALLOC(epp->ep_minsaddr, access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (vaddr_t)STACK_ALLOC(STACK_GROW(epp->ep_minsaddr,
	    access_size), noaccess_size);

	if (noaccess_size > 0 && noaccess_size <= max_stack_size) {
		NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
		    noaccess_linear_min, NULLVP, 0, VM_PROT_NONE, VMCMD_STACK);
	}
	KASSERT(access_size > 0);
	KASSERT(access_size <= max_stack_size);
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULLVP, 0, VM_PROT_READ | VM_PROT_WRITE,
	    VMCMD_STACK);

#ifdef LINUX32_ARM_KUSER_HELPER_ADDR
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_linux32_kuser_helper_map,
	    LINUX32_ARM_KUSER_HELPER_SIZE, LINUX32_ARM_KUSER_HELPER_ADDR,
	    NULLVP, 0, VM_PROT_READ | VM_PROT_EXECUTE, 0);
#endif /* LINUX32_ARM_KUSER_HELPER_ADDR */

	return 0;
}
