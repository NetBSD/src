/*	$NetBSD: svr4_32_exec_elf32.c,v 1.1 2001/02/06 16:37:57 eeh Exp $	 */

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

#define	ELFSIZE		32				/* XXX should die */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec_elf.h>
#include <sys/exec.h>

#include <sys/mman.h>

#include <machine/cpu.h>
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

void *
svr4_32_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	size_t len;
	AuxInfo ai[14+(256/4)], *a, *platform=NULL, *exec=NULL;
	struct elf_args *ap;
	extern char platform_type[32];

	stack = netbsd32_copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return NULL;

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

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0; /* XXXX -- what are these? */
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_SUN_UID;
		a->a_v = p->p_ucred->cr_uid;
		a++;

		a->a_type = AT_SUN_RUID;
		a->a_v = p->p_cred->p_ruid;
		a++;

		a->a_type = AT_SUN_GID;
		a->a_v = p->p_ucred->cr_gid;
		a++;

		a->a_type = AT_SUN_RGID;
		a->a_v = p->p_cred->p_rgid;
		a++;

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);

	if (platform) {
		char *ptr = (char *)a;

		/* Copy out the platform name. */
		platform->a_v = (u_long)ptr;
		strncpy(ptr, platform_type, 32);
		ptr += 33;
		len += 33;

		/* Copy out the file we're executing. */
		exec->a_v = (u_long)ptr;
		strcpy(ptr, pack->ep_name);
		len += strlen(pack->ep_name)+1;

		/* Round to 32-bits */
		len = (len+7)&~0x7L;
	}
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return stack;
}

int
svr4_32_elf32_probe(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp;
	vaddr_t *pos;
{
	const char *bp;
	int error;
	size_t len;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path, itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	epp->ep_flags |= EXEC_32;
	*pos = SVR4_32_INTERP_ADDR;
	return 0;
}
