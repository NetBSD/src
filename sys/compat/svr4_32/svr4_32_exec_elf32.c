/*	$NetBSD: svr4_32_exec_elf32.c,v 1.21.8.1 2012/04/17 00:07:23 yamt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_exec_elf32.c,v 1.21.8.1 2012/04/17 00:07:23 yamt Exp $");

#define	ELFSIZE		32				/* XXX should die */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec_elf.h>
#include <sys/exec.h>

#include <sys/mman.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_util.h>
#include <compat/svr4_32/svr4_32_exec.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/svr4/svr4_errno.h>

/*
 * Since this only works on 64-bit machines trying to execute
 * 32-bit binaries, it's pretty much limited to sparc64
 * trying to run solaris binaries.  So, to simplify this
 * I'm making this svr4_32_copyargs() SPARC/Solaris specific
 * so we can run Solaris 8 dynamic binaries that require
 * lots of fancy specialized support.
 */

int sun_flags = EF_SPARC_SUN_US1|EF_SPARC_32PLUS;
int sun_hwcap = (AV_SPARC_HWMUL_32x32|AV_SPARC_HWDIV_32x32|AV_SPARC_HWFSMULD);

#if 0
int
svr4_32_copyargs(struct lwp *l, struct exec_package *pack, struct ps_strings *arginfo, char **stackp, void *argp)
{
	size_t len;
	AuxInfo ai[SVR4_32_AUX_ARGSIZ], *a, *platform=NULL, *exec=NULL;
	struct elf_args *ap;
	extern char machine_model[];
	int error;

	if ((error = netbsd32_copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {
		struct proc *p = curproc; /* XXXXX */

		a->a_type = AT_SUN_PLATFORM;
		platform = a; /* Patch this later. */
		a++;

		a->a_type = AT_SUN_EXECNAME;
		exec = a; /* Patch this later. */
		a++;

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		if (sun_flags) {
			a->a_type = AT_FLAGS;
			a->a_v = sun_flags;
			a++;
		}

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = AT_EUID;
		a->a_v = kauth_cred_geteuid(l->l_cred);
		a++;

		a->a_type = AT_RUID;
		a->a_v = kauth_cred_getuid(l->l_cred);
		a++;

		a->a_type = AT_EGID;
		a->a_v = kauth_cred_getegid(l->l_cred);
		a++;

		a->a_type = AT_RGID;
		a->a_v = kauth_cred_getgid(l->l_cred);
		a++;

		if (sun_hwcap) {
			a->a_type = AT_SUN_HWCAP;
			a->a_v = sun_hwcap;
			a++;
		}

		exec_free_emul_arg(pack);
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);

	if (platform) {
		char *ptr = (char *)a;
		const char *path = NULL;

		/* Copy out the platform name. */
		platform->a_v = (u_long)(*stackp) + len;
		/* XXXX extremely inefficient.... */
		strcpy(ptr, machine_model);
		ptr += strlen(machine_model) + 1;
		len += strlen(machine_model) + 1;

		if (exec) {
			path = pack->ep_resolvedname;

			/* Copy out the file we're executing. */
			exec->a_v = (u_long)(*stackp) + len;
			strcpy(ptr, path);
			len += strlen(ptr)+1;
		}

		/* Round to 32-bits */
		len = (len+7)&~0x7L;
	}
	if ((error = copyout(ai, *stackp, len)) != 0)
		return error;
	*stackp += len;

	return error;
}
#else
int
svr4_32_copyargs(struct lwp *l, struct exec_package *pack, struct ps_strings *arginfo, char **stackp, void *argp)
{
	size_t len;
	AuxInfo ai[SVR4_32_AUX_ARGSIZ], *a;
	struct elf_args *ap;
	int error;

	if ((error = netbsd32_copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
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

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		if (sun_flags) {
			a->a_type = AT_FLAGS;
			a->a_v = sun_flags;
			a++;
		}

		exec_free_emul_arg(pack);
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if (copyout(ai, *stackp, len))
		return error;
	*stackp += len;

	return 0;
}
#endif

int
svr4_32_elf32_probe(struct lwp *l, struct exec_package *epp, void *eh, char *itp, vaddr_t *pos)
{
	int error;

	if (itp) {
		if ((error = emul_find_interp(l, epp, itp)))
			return error;
	}
	epp->ep_flags |= EXEC_32;
#ifdef SVR4_32_INTERP_ADDR
	*pos = SVR4_32_INTERP_ADDR;
#endif
	return 0;
}
