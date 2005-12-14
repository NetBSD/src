/*	$NetBSD: linux_exec_machdep.c,v 1.4 2005/12/14 18:33:32 christos Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_exec_machdep.c,v 1.4 2005/12/14 18:33:32 christos Exp $");

#ifdef __amd64__
#define ELFSIZE 64
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/exec_elf.h>
#include <sys/vnode.h>
#include <sys/lwp.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <sys/exec_elf.h>

#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <uvm/uvm.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_errno.h>

int
linux_exec_setup_stack(struct lwp *l, struct exec_package *epp)
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

#ifndef USRSTACK32
#define USRSTACK32      (0x00000000ffffffffL & ~PGOFSET)
#endif

	if (epp->ep_flags & EXEC_32) {
		epp->ep_minsaddr = USRSTACK32;
		max_stack_size = MAXSSIZ;
		if (epp->ep_minsaddr > LINUX_USRSTACK32)
			epp->ep_minsaddr = LINUX_USRSTACK32;
	} else {
		epp->ep_minsaddr = USRSTACK;
		max_stack_size = MAXSSIZ;
		if (epp->ep_minsaddr > LINUX_USRSTACK)
			epp->ep_minsaddr = LINUX_USRSTACK;

	}

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
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
		    noaccess_linear_min, NULLVP, 0, VM_PROT_NONE);
	}
	KASSERT(access_size > 0);
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULLVP, 0, VM_PROT_READ | VM_PROT_WRITE);

	return 0;
}

int
ELFNAME2(linux,copyargs)(p, pack, arginfo, stackp, argp)
	struct proc *p;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	struct linux_extra_stack_data64 *esdp, esd;
	struct elf_args *ap;
	struct vattr *vap; 
	Elf_Ehdr *eh;
	Elf_Phdr *ph;
	u_long phsize;
	Elf_Addr phdr = 0;
	int error;
	int i;

	if ((error = copyargs(p, pack, arginfo, stackp, argp)) != 0)
		return error;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries and static binaries as well.
	 */
	memset(&esd, 0, sizeof(esd));
	esdp = (struct linux_extra_stack_data64 *)(*stackp);
	ap = (struct elf_args *)pack->ep_emul_arg;
	vap = pack->ep_vap;
	eh = (Elf_Ehdr *)pack->ep_hdr;

	/*
	 * We forgot this, so we ned to reload it now. XXX keep track of it?
	 */
	if (ap == NULL) {
		phsize = eh->e_phnum * sizeof(Elf_Phdr);
		ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
		error = exec_read_from(p, pack->ep_vp, eh->e_phoff, ph, phsize);
		if (error != 0) {
			for (i = 0; i < eh->e_phnum; i++) {
				if (ph[i].p_type == PT_PHDR) {
					phdr = ph[i].p_vaddr;
					break;
				}
			}
		}
		free(ph, M_TEMP);
	}


	/*
	 * The exec_package doesn't have a proc pointer and it's not
	 * exactly trivial to add one since the credentials are
	 * changing. XXX Linux uses curlwp's credentials.
	 * Why can't we use them too?
	 */

	i = 0;
	esd.ai[i].a_type = LINUX_AT_HWCAP;
	esd.ai[i++].a_v = rcr4();

	esd.ai[i].a_type = AT_PAGESZ;
	esd.ai[i++].a_v = PAGE_SIZE;

	esd.ai[i].a_type = LINUX_AT_CLKTCK;
	esd.ai[i++].a_v = hz;

	esd.ai[i].a_type = AT_PHDR;
	esd.ai[i++].a_v = (ap ? ap->arg_phaddr: phdr);

	esd.ai[i].a_type = AT_PHENT;
	esd.ai[i++].a_v = (ap ? ap->arg_phentsize : eh->e_phentsize);

	esd.ai[i].a_type = AT_PHNUM;
	esd.ai[i++].a_v = (ap ? ap->arg_phnum : eh->e_phnum);

	esd.ai[i].a_type = AT_BASE;
	esd.ai[i++].a_v = (ap ? ap->arg_interp : 0);

	esd.ai[i].a_type = AT_FLAGS;
	esd.ai[i++].a_v = 0;

	esd.ai[i].a_type = AT_ENTRY;
	esd.ai[i++].a_v = (ap ? ap->arg_entry : eh->e_entry);

	esd.ai[i].a_type = LINUX_AT_EGID;
	esd.ai[i++].a_v =
	    ((vap->va_mode & S_ISGID) ? vap->va_gid : p->p_ucred->cr_gid);

	esd.ai[i].a_type = LINUX_AT_GID;
	esd.ai[i++].a_v = p->p_cred->p_rgid;

	esd.ai[i].a_type = LINUX_AT_EUID;
	esd.ai[i++].a_v = 
	    ((vap->va_mode & S_ISUID) ? vap->va_uid : p->p_ucred->cr_uid);

	esd.ai[i].a_type = LINUX_AT_UID;
	esd.ai[i++].a_v = p->p_cred->p_ruid;

	esd.ai[i].a_type = LINUX_AT_SECURE;
	esd.ai[i++].a_v = 0;

	esd.ai[i].a_type = LINUX_AT_PLATFORM;
	esd.ai[i++].a_v = (Elf_Addr)&esdp->hw_platform[0];

	esd.ai[i].a_type = AT_NULL;
	esd.ai[i++].a_v = 0;

#ifdef DEBUG_LINUX
	if (i != LINUX_ELF_AUX_ENTRIES) {
		printf("linux_elf64_copyargs: %d Aux entries\n", i);
		return EINVAL;
	}
#endif
		
	strcpy(esd.hw_platform, LINUX_PLATFORM); 

	if (ap) {
		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}
	
	/*
	 * Copy out the ELF auxiliary table and hw platform name
	 */
	if ((error = copyout(&esd, esdp, sizeof(esd))) != 0)
		return error;
	*stackp += sizeof(esd);

	return 0;
}
