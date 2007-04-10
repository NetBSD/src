/*	$NetBSD: linux32_exec_elf32.c,v 1.5.16.1 2007/04/10 13:26:24 ad Exp $ */

/*-                     
 * Copyright (c) 1995, 1998, 2000, 2001,2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *              
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden, Eric Haszlakiewicz and
 * Emmanuel Dreyfus.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: linux32_exec_elf32.c,v 1.5.16.1 2007/04/10 13:26:24 ad Exp $");

#define	ELFSIZE		32

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <compat/linux/common/linux_exec.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/linux32/common/linux32_exec.h>

#include <machine/frame.h>

#ifdef DEBUG_LINUX
#define DPRINTF(a)      uprintf a
#else
#define DPRINTF(a)
#endif

int linux32_copyinargs(struct exec_package *, struct ps_strings *,
			void *, size_t, const void *, const void *);

int
ELFNAME2(linux32,probe)(l, epp, eh, itp, pos)
	struct lwp *l;
	struct exec_package *epp;
	void *eh;
	char *itp;
	vaddr_t *pos;
{
	int error;

	if (((error = ELFNAME2(linux,signature)(l, epp, eh, itp)) != 0) &&
#ifdef LINUX32_GCC_SIGNATURE
	    ((error = ELFNAME2(linux,gcc_signature)(l, epp, eh)) != 0) &&
#endif
#ifdef LINUX32_ATEXIT_SIGNATURE
	    ((error = ELFNAME2(linux,atexit_signature)(l, epp, eh)) != 0) &&
#endif
#ifdef LINUX32_DEBUGLINK_SIGNATURE
	    ((error = ELFNAME2(linux,debuglink_signature)(l, epp, eh)) != 0) &&
#endif
	    1)
			return error;

	if (itp) {
		if ((error = emul_find_interp(l, epp->ep_esch->es_emul->e_path,
		    itp)))
			return (error);
	}
#if 0
	DPRINTF(("linux32_probe: returning 0\n"));
#endif

	epp->ep_flags |= EXEC_32;
	epp->ep_vm_minaddr = VM_MIN_ADDRESS;
	epp->ep_vm_maxaddr = USRSTACK32;

	return 0;
}

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
linux32_elf32_copyargs(struct lwp *l, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	struct linux32_extra_stack_data *esdp, esd;
	struct elf_args *ap;
	struct vattr *vap;
	Elf_Ehdr *eh;
	Elf_Phdr *ph;
	Elf_Addr phdr = 0;
	u_long phsize;
	int error;
	int i;

	if ((error = netbsd32_copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries and static binaries as well.
	 */
	memset(&esd, 0, sizeof(esd));
	esdp = (struct linux32_extra_stack_data *)(*stackp);
	ap = (struct elf_args *)pack->ep_emul_arg;
	vap = pack->ep_vap;
	eh = (Elf_Ehdr *)pack->ep_hdr;

	/*
	 * We forgot this, so we ned to reload it now. XXX keep track of it?
	 */
	if (ap == NULL) {
		phsize = eh->e_phnum * sizeof(Elf_Phdr);
		ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
		error = exec_read_from(l, pack->ep_vp, eh->e_phoff, ph, phsize);
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
	 * Why can't we use them too? XXXad we do now; what's different
	 * about Linux's LWP creds?
	 */

	i = 0;
	esd.ai[i].a_type = LINUX_AT_SYSINFO;
	esd.ai[i++].a_v = NETBSD32PTR32I(&esdp->kernel_vsyscall[0]);

	esd.ai[i].a_type = LINUX_AT_SYSINFO_EHDR;
	esd.ai[i++].a_v = NETBSD32PTR32I(&esdp->elfhdr);

	esd.ai[i].a_type = LINUX_AT_HWCAP;
	esd.ai[i++].a_v = LINUX32_CPUCAP;

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
	esd.ai[i++].a_v = ((vap->va_mode & S_ISGID) ?
	    vap->va_gid : kauth_cred_getegid(l->l_cred));

	esd.ai[i].a_type = LINUX_AT_GID;
	esd.ai[i++].a_v = kauth_cred_getgid(l->l_cred);

	esd.ai[i].a_type = LINUX_AT_EUID;
	esd.ai[i++].a_v = ((vap->va_mode & S_ISUID) ?
	    vap->va_uid : kauth_cred_geteuid(l->l_cred));

	esd.ai[i].a_type = LINUX_AT_UID;
	esd.ai[i++].a_v = kauth_cred_getuid(l->l_cred);

	esd.ai[i].a_type = LINUX_AT_SECURE;
	esd.ai[i++].a_v = 0;

	esd.ai[i].a_type = LINUX_AT_PLATFORM;
	esd.ai[i++].a_v = NETBSD32PTR32I(&esdp->hw_platform[0]);

	esd.ai[i].a_type = AT_NULL;
	esd.ai[i++].a_v = 0;

#ifdef DEBUG_LINUX
	if (i != LINUX32_ELF_AUX_ENTRIES) {
		printf("linux32_elf32_copyargs: %d Aux entries\n", i);
		return EINVAL;
	}
#endif
		
	memcpy(esd.kernel_vsyscall, linux32_kernel_vsyscall, 
	    sizeof(linux32_kernel_vsyscall));

	memcpy(&esd.elfhdr, eh, sizeof(*eh));

	strcpy(esd.hw_platform, LINUX32_PLATFORM); 

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

