/*	$NetBSD: elf.c,v 1.4.4.2 2002/02/28 04:08:33 nathanw Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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

#ifdef TOSTOOLS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "exec_elf.h"

#define	MALLOC(x)	malloc(x)

#else

#include <stand.h>
#include <atari_stand.h>
#include <string.h>
#include <libkern.h>
#include <sys/exec_elf.h>

#define	MALLOC(x)	alloc(x)
#endif

#include "libtos.h"
#include "tosdefs.h"
#include "kparamb.h"
#include "cread.h"

/*
 * Load an elf image.
 * Exit codes:
 *	-1      : Not an ELF file.
 *	 0      : OK
 *	 error# : Error during load (*errp might contain error string).
 */
#define ELFMAGIC	((ELFMAG0 << 24) | (ELFMAG1 << 16) | \
				(ELFMAG2 << 8) | ELFMAG3)

int
elf_load(fd, od, errp, loadsyms)
int	fd;
osdsc_t	*od;
char	**errp;
int	loadsyms;
{
	int		i,j;
	int		err;
	Elf32_Ehdr	ehdr;
	Elf32_Phdr	*phdrs;
	Elf32_Word	symsize, symstart;
	long		kernsize;

	*errp = NULL;
	lseek(fd, (off_t)0, SEEK_SET);
	if (read(fd, (char *)&ehdr, sizeof(ehdr)) != sizeof(ehdr))
		return -1;

	if (*((u_int *)ehdr.e_ident) != ELFMAGIC)
		return -1;

	/*
	 * calculate highest used address
	 */
	i = ehdr.e_phnum * sizeof(Elf32_Phdr);
	err = 1;
	if ((phdrs = (Elf32_Phdr *)MALLOC(i)) == NULL)
		goto error;
	err = 2;
	if (read(fd, phdrs, i) != i)
		goto error;

	kernsize = 0;
	for (i = 0; i < ehdr.e_phnum; i++) {
		Elf32_Word sum;

		sum = phdrs[i].p_vaddr + phdrs[i].p_memsz;
		if ((phdrs[i].p_flags & (PF_W|PF_X)) && (sum > kernsize))
			kernsize = sum;
	}

	/*
	 * look for symbols and calculate the size
	 * XXX: This increases the load time by a factor 2 for gzipped
	 *      images!
	 */
	symsize  = 0;
	symstart = 0;
	if (loadsyms) {
	    i = ehdr.e_shnum + 1;
	    err = 3;
	    if (lseek(fd, (off_t)ehdr.e_shoff, SEEK_SET) != ehdr.e_shoff)
		goto error;
	    while (--i) {
	      Elf32_Shdr shdr;

	      err = 4;
	      if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr))
		goto error;
	      if ((shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_STRTAB))
		symsize += shdr.sh_size;
	    }
	}

	if (symsize) {
	  symstart = kernsize;
	  kernsize += symsize + sizeof(ehdr) + ehdr.e_shnum*sizeof(Elf32_Shdr);
	}

	/*
	 * Extract various sizes from the kernel executable
	 */
	od->k_esym = symsize ? kernsize : 0;
	od->ksize  = kernsize;
	od->kentry = ehdr.e_entry;

	err = 5;
	if ((od->kstart = (u_char *)MALLOC(od->ksize)) == NULL)
		goto error;

	/*
	 * Read text & data, clear bss
	 */
	for (i = 0; i < ehdr.e_phnum; i++) {
	    u_char	*p;
	    Elf32_Phdr	*php = &phdrs[i];

	    if (php->p_flags & (PF_W|PF_X)) {
		err = 6;
		if (lseek(fd, (off_t)php->p_offset, SEEK_SET) != php->p_offset)
		    goto error;
		p = (u_char *)(od->kstart) + php->p_vaddr;
		err = 7;
		if (read(fd, p, php->p_filesz) != php->p_filesz)
		    goto error;
		if (php->p_memsz > php->p_filesz)
		    bzero(p + php->p_filesz, php->p_memsz - php->p_filesz);
	    }
	}

	/*
	 * Read symbols and strings
	 */
	if (symsize) {
	    u_char	*p, *symtab;
	    int		nhdrs;
	    Elf32_Shdr	*shp;

	    symtab = od->kstart + symstart;

	    p = symtab + sizeof(ehdr);
	    nhdrs = ehdr.e_shnum;
	    err = 8;
	    if (lseek(fd, (off_t)ehdr.e_shoff, SEEK_SET) != ehdr.e_shoff)
		goto error;
	    err = 9;
	    if (read(fd, p, nhdrs * sizeof(*shp)) != nhdrs * sizeof(*shp))
		goto error;
	    shp = (Elf32_Shdr*)p;
	    p  += nhdrs * sizeof(*shp);
	    for (i = 0; i < nhdrs; i++) {
		if (shp[i].sh_type == SHT_SYMTAB) {
		    if (shp[i].sh_offset == 0)
			continue;
		    /* Got the symbol table. */
		    err = 10;
		    if (lseek(fd, (off_t)shp[i].sh_offset, SEEK_SET) !=
							shp[i].sh_offset)
		    	goto error;
		    err = 11;
		    if (read(fd, p, shp[i].sh_size) != shp[i].sh_size)
			goto error;
		    shp[i].sh_offset = p - symtab;
		    /* Find the string table to go with it. */
		    j = shp[i].sh_link;
		    if (shp[j].sh_offset == 0)
			continue;
		    p += shp[i].sh_size;
		    err = 12;
		    if (lseek(fd, (off_t)shp[j].sh_offset, SEEK_SET) !=
				    			shp[j].sh_offset)
		    	goto error;
		    err = 13;
		    if (read(fd, p, shp[j].sh_size) != shp[j].sh_size)
			goto error;
		    shp[j].sh_offset = p - symtab;
		    /* There should only be one symbol table. */
		    break;
		}
	    }
	    ehdr.e_shoff = sizeof(ehdr);
	    bcopy(&ehdr, symtab, sizeof(ehdr));
	}
	return 0;

error:
#ifdef TOSTOOLS
	{
		static char *errs[] = {
			/*  1 */ "Cannot malloc Elf phdr storage space",
			/*  2 */ "Cannot read Elf32_Phdrs",
			/*  3 */ "Cannot seek to e_shoff location",
			/*  4 */ "Cannot read Elf32_shdr",
			/*  5 */ "Cannot malloc kernel image space",
		    	/*  6 */ "Seek error while reading text segment\n",
		    	/*  7 */ "Read error in text segment\n",
			/*  8 */ "Error seeking to section headers",
			/*  9 */ "Error reading section headers",
		    	/* 10 */ "Error seeking to symbols",
			/* 11 */ "Error reading symbols",
		    	/* 12 */ "Error seeking to string table",
			/* 13 */ "Error reading strings"
		};
		*errp = errs[err];
	}
#endif /* TOSTOOLS */

	return err;
}
