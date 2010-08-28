/* $NetBSD: exec_elf32.c,v 1.12.2.2 2010/08/28 21:30:04 joerg Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: exec_elf32.c,v 1.12.2.2 2010/08/28 21:30:04 joerg Exp $");
#endif /* not lint */

#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "extern.h"

#if defined(NLIST_ELF32) || defined(NLIST_ELF64)
#include <sys/exec_elf.h>
#endif

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

#define	check(off, size)	((off < 0) || (off + size > mappedsize))
#define	BAD			do { rv = -1; goto out; } while (0)

int
ELFNAMEEND(check)(mappedfile, mappedsize)
	const char *mappedfile;
	size_t mappedsize;
{
	const Elf_Ehdr *ehdrp;
	int rv;

	rv = 0;

	if (check(0, sizeof *ehdrp))
		BAD;
	ehdrp = (const Elf_Ehdr *)&mappedfile[0];

	if (memcmp(ehdrp->e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdrp->e_ident[EI_CLASS] != ELFCLASS)
		BAD;

	switch (ehdrp->e_machine) {
	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		BAD;
	}

out:
	return (rv);
}

int
ELFNAMEEND(findoff)(mappedfile, mappedsize, vmaddr, fileoffp)
	const char *mappedfile;
	size_t mappedsize, *fileoffp;
	u_long vmaddr;
{
	const Elf_Ehdr *ehdrp;
	const Elf_Phdr *phdrp;
	Elf_Off phdr_off;
	Elf_Word phdr_size;
#if (ELFSIZE == 32)
	Elf32_Half nphdr, i;
#elif (ELFSIZE == 64)
	Elf64_Word nphdr, i;
#endif
	int rv;

	rv = 0;

	ehdrp = (const Elf_Ehdr *)&mappedfile[0];
	nphdr = ehdrp->e_phnum;
	phdr_off = ehdrp->e_phoff;
	phdr_size = sizeof(Elf_Phdr) * nphdr;

	if (check(0, phdr_off + phdr_size))
		BAD;
	phdrp = (const Elf_Phdr *)&mappedfile[phdr_off];

#define IS_TEXT(p)	(p.p_flags & PF_X)
#define IS_DATA(p)	(p.p_flags & PF_W)

	for (i = 0; i < nphdr; i++) {
		if ((IS_TEXT(phdrp[i]) || IS_DATA(phdrp[i])) &&
		    phdrp[i].p_vaddr <= vmaddr &&
		    vmaddr < phdrp[i].p_vaddr + phdrp[i].p_filesz) {
			*fileoffp = vmaddr -
			    phdrp[i].p_vaddr + phdrp[i].p_offset;
			break;
		}
	}
	if (i == nphdr)
		BAD;

out:
	return (rv);
}

#endif /* include this size of ELF */
