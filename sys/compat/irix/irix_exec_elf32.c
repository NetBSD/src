/*	$NetBSD: irix_exec_elf32.c,v 1.4 2002/08/26 21:05:59 christos Exp $ */

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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: irix_exec_elf32.c,v 1.4 2002/08/26 21:05:59 christos Exp $");

#ifndef ELFSIZE
#define ELFSIZE		32	/* XXX should die */
#endif

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/null.h>
#include <sys/malloc.h>
#include <sys/syslimits.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <machine/vmparam.h>

#include <compat/irix/irix_exec.h>

int
ELFNAME2(irix,copyargs)(p, pack, arginfo, stackp, argp)
	struct proc *p;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	char **cpp, *dp, *sp;
	size_t len;
	void *nullp;
	long argc, envc;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	int error;

	/*
	 * IRIX seems to expect argc and **argv to be aligned on a 
	 * 16 bytes boundary. It seems there is no other way of
	 * getting **argv aligned than duplicating and customizing
	 * the code that sets up the stack in copyargs():
	 */
#ifdef DEBUG_IRIX
	printf("irix_elf32_copyargs(): *stackp = %p\n", *stackp);
#endif
	/* 
	 * This is borrowed from sys/kern/kern_exec.c:copyargs()
	 */
	cpp = (char **)*stackp;
	nullp = NULL;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_es->es_arglen);

	/* Align **argv on a 16 bytes boundary */
	dp = (char *)(((unsigned long)dp + 0xf) & ~0xfUL);

	sp = argp;

	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
#ifdef DEBUG_IRIX
	printf("irix_elf32_copyargs(): argc = %d, cpp = %p, dp = %p\n", (int)argc, cpp, dp);
#endif
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;
	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	*stackp = (char *)cpp;

	/*
	 * This is borrowed from sys/kern/exec_elf32.c:elf32_copyargs
	 */
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

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
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
/* 	*stackp += len; */
#ifdef DEBUG_IRIX
	printf("*stackp = %p\n", *stackp);
#endif
	return 0;
}
