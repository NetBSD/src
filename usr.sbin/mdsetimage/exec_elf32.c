/* $NetBSD: exec_elf32.c,v 1.6.2.1 2000/06/22 18:01:03 minoura Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: exec_elf32.c,v 1.6.2.1 2000/06/22 18:01:03 minoura Exp $");
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
	Elf_Ehdr *ehdrp;
	int rv;

	rv = 0;

	if (check(0, sizeof *ehdrp))
		BAD;
	ehdrp = (Elf_Ehdr *)&mappedfile[0];

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
	Elf_Ehdr *ehdrp;
	Elf_Shdr *shdrp;
	Elf_Off shdr_off;
	Elf_Word shdr_size;
#if (ELFSIZE == 32)
	Elf32_Half nshdr, i;
#elif (ELFSIZE == 64)
	Elf64_Half nshdr, i;
#endif
	int rv;

	rv = 0;

	ehdrp = (Elf_Ehdr *)&mappedfile[0];
	nshdr = ehdrp->e_shnum;
	shdr_off = ehdrp->e_shoff;
	shdr_size = ehdrp->e_shentsize * nshdr;

	if (check(shdr_off, shdr_size) ||
	    (sizeof *shdrp != ehdrp->e_shentsize))
		BAD;
	shdrp = (Elf_Shdr *)&mappedfile[shdr_off];

	for (i = 0; i < nshdr; i++) {
		if (shdrp[i].sh_addr <= vmaddr &&
		    vmaddr < (shdrp[i].sh_addr + shdrp[i].sh_size)) {
			*fileoffp = vmaddr -
			    shdrp[i].sh_addr + shdrp[i].sh_offset;
			break;
		}
	}
	if (i == nshdr)
		BAD;

out:
	return (rv);
}

#endif /* include this size of ELF */
