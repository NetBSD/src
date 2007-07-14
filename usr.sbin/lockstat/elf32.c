/*	$NetBSD: elf32.c,v 1.6 2007/07/14 13:30:43 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *          NetBSD Project.  See http://www.NetBSD.org/ for
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
#if !defined(lint)
__RCSID("$NetBSD: elf32.c,v 1.6 2007/07/14 13:30:43 ad Exp $");
#endif

#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/param.h>
#include <sys/exec_elf.h>
#include <sys/queue.h>

#include <dev/lockstat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "extern.h"

#if (ELFSIZE == 32)
#define	NAME(x)	x##32
#elif (ELFSIZE == 64)
#define	NAME(x)	x##64
#endif

static int		nsyms;
static Elf_Sym		*symp;
static char		*strp;

int
NAME(loadsym)(int fd)
{
	Elf_Shdr symhdr, strhdr;
	Elf_Ehdr ehdr;
	size_t sz;
	off_t off;
	int i;

	/*
	 * Read the ELF header and make sure it's OK.
	 */
	if (pread(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr))
		return -1;

	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdr.e_ident[EI_CLASS] != ELFCLASS)
		return -1;

	switch (ehdr.e_machine) {
	ELFDEFNNAME(MACHDEP_ID_CASES)
	default:
		return -1;
	}

	/*
	 * Find the symbol table header, and make sure the binary isn't
	 * stripped.
	 */
	off = ehdr.e_shoff;
	for (i = 0; i < ehdr.e_shnum; i++, off += sizeof(symhdr)) {
		sz = pread(fd, &symhdr, sizeof(symhdr), off);
		if (sz != sizeof(symhdr))
			err(EXIT_FAILURE, "pread (section headers)");
		if (symhdr.sh_type == SHT_SYMTAB)
			break;
	}
	if (i == ehdr.e_shnum || symhdr.sh_offset == 0)
		err(EXIT_FAILURE, "namelist is stripped");

	/*
	 * Pull in the string table header, and then read in both the symbol
	 * table and string table proper.
	 *
	 * XXX We can't use mmap(), as /dev/ksyms doesn't support mmap yet.
	 */
	off = ehdr.e_shoff + symhdr.sh_link * sizeof(symhdr);  
	if (pread(fd, &strhdr, sizeof(strhdr), off) != sizeof(strhdr))
		err(EXIT_FAILURE, "pread");

	if ((symp = malloc(symhdr.sh_size)) == NULL)
		err(EXIT_FAILURE, "malloc (symbol table)");
	sz = pread(fd, symp, symhdr.sh_size, symhdr.sh_offset	);
	if (sz != symhdr.sh_size)
		err(EXIT_FAILURE, "pread (symbol table)");

	if ((strp = malloc(strhdr.sh_size)) == NULL)
		err(EXIT_FAILURE, "malloc (string table)");
	sz = pread(fd, strp, strhdr.sh_size, strhdr.sh_offset);
	if (sz != strhdr.sh_size)
		err(EXIT_FAILURE, "pread (string table)");

	nsyms = (int)(symhdr.sh_size / sizeof(Elf_Sym));

	return 0;
}

int
NAME(findsym)(findsym_t find, char *name, uintptr_t *start, uintptr_t *end)
{
	static int lastptr[FIND_MAX];
	uintptr_t sa, ea;
	int i, rv, st, off;

	switch (find) {
	case LOCK_BYNAME:
	case LOCK_BYADDR:
		st = STT_OBJECT;
		break;
	case FUNC_BYNAME:
	case FUNC_BYADDR:
		st = STT_FUNC;
		break;
	default:
		return -1;
	}

	rv = -1;

#ifdef dump_core
	for (i = lastptr[find];;) {
#else
	for (i = 0; i < nsyms; i++) {
#endif
		switch (find) {
		case LOCK_BYNAME:
		case FUNC_BYNAME:
			if (ELF_ST_TYPE(symp[i].st_info) != st)
				break;
			if (strcmp(&strp[symp[i].st_name], name) != 0)
				break;
			*start = (uintptr_t)symp[i].st_value;
			*end = *start + (uintptr_t)symp[i].st_size;
			goto found;

		case LOCK_BYADDR:
		case FUNC_BYADDR:
			if (ELF_ST_TYPE(symp[i].st_info) != st)
				break;
			sa = (uintptr_t)symp[i].st_value;
			ea = sa + (uintptr_t)symp[i].st_size - 1;
			if (*start < sa || *start > ea)
				break;
			off = (int)(*start - sa);
			*start = sa;
			if (end != NULL)
				*end = ea;
			if (name == NULL)
				goto found;
			if (off == 0)
				strlcpy(name, &strp[symp[i].st_name],
				    NAME_SIZE);
			else
				snprintf(name, NAME_SIZE, "%s+%x",
				    &strp[symp[i].st_name], off);
			goto found;
			
		default:
			break;
		}

#ifdef dump_core
		if (++i >= nsyms)
			i = 0;
		if (i == lastptr[find])
			return -1;
#endif
	}

	return -1;

 found:
 	lastptr[find] = i;
 	return 0;
}
