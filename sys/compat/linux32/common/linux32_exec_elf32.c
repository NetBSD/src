/*	$NetBSD: linux32_exec_elf32.c,v 1.23 2021/11/26 08:56:29 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux32_exec_elf32.c,v 1.23 2021/11/26 08:56:29 ryo Exp $");

#define	ELFSIZE		32

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/cprng.h>

#include <compat/linux/common/linux_exec.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/linux32/common/linux32_exec.h>

#ifndef __aarch64__
#include <machine/cpuvar.h>
#endif
#include <machine/frame.h>

#ifdef DEBUG_LINUX
#define DPRINTF(a)      uprintf a
#else
#define DPRINTF(a)
#endif

int linux32_copyinargs(struct exec_package *, struct ps_strings *,
			void *, size_t, const void *, const void *);

int
ELFNAME2(linux32,probe)(struct lwp *l, struct exec_package *epp,
			void *eh, char *itp, vaddr_t *pos)
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
#ifdef LINUX32_GO_RT0_SIGNATURE
	    ((error = ELFNAME2(linux,go_rt0_signature)(l, epp, eh)) != 0) &&
#endif
	    1)
			return error;

	if (itp) {
		if ((error = emul_find_interp(l, epp, itp)))
			return (error);
	}
#if 0
	DPRINTF(("linux32_probe: returning 0\n"));
#endif

	epp->ep_flags |= EXEC_32 | EXEC_FORCEAUX;
	epp->ep_vm_minaddr = exec_vm_minaddr(VM_MIN_ADDRESS);
	epp->ep_vm_maxaddr = USRSTACK32;

	return 0;
}

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
linux32_elf32_copyargs(struct lwp *l, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	struct linux32_extra_stack_data esd, *esdp;
	Aux32Info *a, *ai __diagused;
	struct elf_args *ap;
	struct vattr *vap;
	int error;

	if ((error = netbsd32_copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	esdp = (struct linux32_extra_stack_data *)(*stackp);	/* userspace */

	memset(&esd, 0, sizeof(esd));
	ai = a = esd.ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries and static binaries as well.
	 */

	a->a_type = AT_PAGESZ;
	a->a_v = PAGE_SIZE;
	a++;

	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		exec_free_emul_arg(pack);
	}

	/* Linux-specific items */
	a->a_type = LINUX_AT_CLKTCK;
	a->a_v = hz;
	a++;

	vap = pack->ep_vap;

	a->a_type = LINUX_AT_UID;
	a->a_v = kauth_cred_getuid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_EUID;
	a->a_v = ((vap->va_mode & S_ISUID) ?
	    vap->va_uid : kauth_cred_geteuid(l->l_cred));
	a++;

	a->a_type = LINUX_AT_GID;
	a->a_v = kauth_cred_getgid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_EGID;
	a->a_v = ((vap->va_mode & S_ISGID) ?
	    vap->va_gid : kauth_cred_getegid(l->l_cred));
	a++;

	a->a_type = LINUX_AT_SECURE;
	a->a_v = 0;
	a++;

	a->a_type = LINUX_AT_RANDOM;
	a->a_v = NETBSD32PTR32I(&esdp->randbytes[0]);
	a++;
	esd.randbytes[0] = cprng_strong32();
	esd.randbytes[1] = cprng_strong32();
	esd.randbytes[2] = cprng_strong32();
	esd.randbytes[3] = cprng_strong32();

#if 0 /* defined(__amd64__) */
	/* XXX: broken. vsyscall must be placed in the executable page */
	a->a_type = LINUX_AT_SYSINFO;
	a->a_v = NETBSD32PTR32I(&esdp->kernel_vsyscall[0]);
	a++;
	memcpy(esd.kernel_vsyscall, linux32_kernel_vsyscall,
	    sizeof(linux32_kernel_vsyscall));
#endif

#if 0 /* notyet */
	a->a_type = LINUX_AT_SYSINFO_EHDR;
	a->a_v = NETBSD32PTR32I(&esdp->elfhdr);
	a++;
	memcpy(&esd.elfhdr, eh, sizeof(*eh));
#endif

#ifdef LINUX32_CPUCAP
	a->a_type = LINUX_AT_HWCAP;
	a->a_v = LINUX32_CPUCAP;
	a++;
#endif

#ifdef LINUX32_PLATFORM
	a->a_type = LINUX_AT_PLATFORM;
	a->a_v = NETBSD32PTR32I(&esdp->hw_platform[0]);
	a++;
	strncpy(esd.hw_platform, LINUX32_PLATFORM, sizeof(esd.hw_platform));
#endif

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	KASSERTMSG(a <= &ai[LINUX32_ELF_AUX_ENTRIES],
	    "increase LINUX32_ELF_AUX_ENTRIES to %lu", a - ai);

	/*
	 * Copy out the ELF auxiliary table and hw platform name
	 */
	if ((error = copyout(&esd, esdp, sizeof(esd))) != 0)
		return error;
	*stackp += sizeof(esd);

	return 0;
}
