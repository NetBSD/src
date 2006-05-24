/* $NetBSD: linux_exec_powerpc.c,v 1.17.8.1 2006/05/24 10:57:28 yamt Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

/*
 * From NetBSD's sys/compat/arch/alpha/linux_exec_alpha.c, with some
 * powerpc add-ons (ifdef LINUX_SHIFT).
 *
 * This code is to be common to alpha and powerpc. If it works on alpha, it
 * should be moved to common/linux_exec_elf32.c. Beware that it needs
 * LINUX_ELF_AUX_ENTRIES in arch/<arch>/linux_exec.h to also be moved to common
 *
 * Emmanuel Dreyfus <p99dreyf@criens.u-psud.fr>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_exec_powerpc.c,v 1.17.8.1 2006/05/24 10:57:28 yamt Exp $");

#if defined (__alpha__)
#define ELFSIZE 64
#elif defined (__powerpc__)
#define ELFSIZE 32
#else
#error Unified linux_elf_{32|64}copyargs not tested for this platform
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/resourcevar.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <compat/linux/common/linux_exec.h>

/*
 * Alpha and PowerPC specific linux copyargs function.
 */
int
ELFNAME2(linux,copyargs)(l, pack, arginfo, stackp, argp)
	struct lwp *l;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	size_t len;
	AuxInfo ai[LINUX_ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	struct proc *p;
	int error;

	p = l->l_proc;
#ifdef LINUX_SHIFT
	/*
	 * Seems that PowerPC Linux binaries expect argc to start on a 16 bytes
	 * aligned address. And we need one more 16 byte shift if it was already
	 * 16 bytes aligned,
	 */
	*stackp = (char *)(((unsigned long)*stackp - 1) & ~LINUX_SHIFT);
#endif

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

#ifdef LINUX_SHIFT
	/*
	 * From Linux's arch/ppc/kernel/process.c:shove_aux_table(). GNU ld.so
	 * expects the ELF auxiliary table to start on a 16 bytes boundary on
	 * the PowerPC.
	 */
	*stackp = (char *)(((unsigned long)(*stackp) + LINUX_SHIFT)
	    & ~LINUX_SHIFT);
#endif

	memset(ai, 0, sizeof(AuxInfo) * LINUX_ELF_AUX_ENTRIES);

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries.
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {
#if 1
		/*
		 * The exec_package doesn't have a proc pointer and it's not
		 * exactly trivial to add one since the credentials are
		 * changing. XXX Linux uses curlwp's credentials.
		 * Why can't we use them too?
		 */
		a->a_type = LINUX_AT_EGID;
		a->a_v = kauth_cred_getegid(p->p_cred);
		a++;

		a->a_type = LINUX_AT_GID;
		a->a_v = kauth_cred_getgid(p->p_cred);
		a++;

		a->a_type = LINUX_AT_EUID;
		a->a_v = kauth_cred_geteuid(p->p_cred);
		a++;

		a->a_type = LINUX_AT_UID;
		a->a_v = kauth_cred_getuid(p->p_cred);
		a++;
#endif

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = LINUX_AT_CLKTCK;
		a->a_v = LINUX_CLOCKS_PER_SEC;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = LINUX_AT_HWCAP;
		a->a_v = LINUX_ELF_HWCAP;
		a++;

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);

	if ((error = copyout(ai, *stackp, len)) != 0)
		return error;
	*stackp += len;
	return 0;
}
