/*	$NetBSD: irix_exec.c,v 1.5 2001/12/02 16:16:57 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_exec.c,v 1.5 2001/12/02 16:16:57 manu Exp $");

#ifndef ELFSIZE
#define ELFSIZE		32	/* XXX should die */
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/malloc.h>

#include <compat/common/compat_util.h>

#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_errno.h>

static int ELFNAME2(irix,mipsopt_signature) __P((struct proc *, 
    struct exec_package *epp, Elf_Ehdr *eh));

extern char sigcode[], esigcode[];
extern struct sysent irix_sysent[];

#ifndef __HAVE_SYSCALL_INTERN
void irix_syscall __P((void));
#else
void irix_syscall_intern __P((struct proc *));
#endif

const struct emul emul_irix = {
	"irix",
	"/emul/irix",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	native_to_irix_errno,
	IRIX_SYS_syscall,
	IRIX_SYS_MAXSYSCALL,
#endif
	irix_sysent,
#ifdef SYSCALL_DEBUG
	irix_syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	setregs,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	irix_syscall_intern,
#else
	irix_syscall,
#endif
};

static int
ELFNAME2(irix,mipsopt_signature)(p, epp, eh)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
{
	size_t shsize;
	int     strndx;
	size_t i;
	static const char signature[] = ".MIPS.options";
	char* strtable;
	Elf_Shdr *sh;

	int error;

	/*
	 * load the section header table
	 */
	shsize = eh->e_shnum * sizeof(Elf_Shdr);
	sh = (Elf_Shdr *) malloc(shsize, M_TEMP, M_WAITOK);
	error = exec_read_from(p, epp->ep_vp, eh->e_shoff, sh, shsize);
	if (error)
		goto out;

	/*
	 * Now let's find the string table. If it does not exists, give up.
	 */
	strndx = (int)(eh->e_shstrndx);
	if (strndx == SHN_UNDEF) {
		error = ENOEXEC;
		goto out;
	}

	/*
	 * strndx is the index in section header table of the string table
	 * section get the whole string table in strtable, and then we 
	 * get access to the names
	 * s->sh_name is the offset of the section name in strtable.
	 */
	strtable = malloc(sh[strndx].sh_size, M_TEMP, M_WAITOK);
	error = exec_read_from(p, epp->ep_vp, sh[strndx].sh_offset, strtable,
	    sh[strndx].sh_size);
	if (error)
		goto out;

	for (i = 0; i < eh->e_shnum; i++) {
		Elf_Shdr *s = &sh[i];
		if (!memcmp((void*)(&(strtable[s->sh_name])), signature,
				sizeof(signature))) {
#ifdef DEBUG_IRIX
			printf("irix_mipsopt_sig=%s\n",&(strtable[s->sh_name]));
#endif
			error = 0;
			goto out;
		}
	}
	error = ENOEXEC;

out:
	free(sh, M_TEMP);
	free(strtable, M_TEMP);
	return (error);
}


int
ELFNAME2(irix,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp; 
	vaddr_t *pos; 
{
	const char *bp;
	int error;
	size_t len;

#ifdef DEBUG_IRIX
	printf("irix_probe()\n");
#endif
	if ((error = ELFNAME2(irix,mipsopt_signature)(p, epp, eh)) != 0)
		return error;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path,
		    itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	*pos = ELF_NO_ADDR;
#ifdef DEBUG_IRIX
	printf("irix_probe: returning 0\n");
#endif
	return 0;
}
