/* $NetBSD: nlist_elf32.c,v 1.14 2001/07/22 13:34:17 wiz Exp $ */

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
__RCSID("$NetBSD: nlist_elf32.c,v 1.14 2001/07/22 13:34:17 wiz Exp $");
#endif /* not lint */

/* If not included by nlist_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <a.out.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#if defined(NLIST_ELF32) || defined(NLIST_ELF64)
#include <sys/exec_elf.h>
#endif

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

typedef struct nlist NLIST;
#define	_strx	n_un.n_strx
#define	_name	n_un.n_name

#define	badfmt(str)							\
	do {								\
		warnx("%s: %s: %s", kfile, str, strerror(EFTYPE));	\
		punt();							\
	} while (0)

#define	check(off, size)	((off < 0) || (off + size > mappedsize))
#define	BAD			do { rv = -1; goto out; } while (0)
#define	BADUNMAP		do { rv = -1; goto unmap; } while (0)

static const char *kfile;

int
ELFNAMEEND(create_knlist)(name, db)
	const char *name;
	DB *db;
{
	struct stat st;
	struct nlist nbuf;
	DBT key, data;
	char *mappedfile, *symname, *fsymname, *tmpcp, *strtab;
	size_t mappedsize, symnamesize, fsymnamesize;
	Elf_Ehdr *ehdrp;
	Elf_Shdr *shdrp, *symshdrp, *symstrshdrp;
	Elf_Sym *symp;
	Elf_Off shdr_off;
	Elf_Word shdr_size;
#if (ELFSIZE == 32)
	Elf32_Half nshdr;
#elif (ELFSIZE == 64)
	Elf64_Half nshdr;
#endif
	unsigned long i, nsyms;
	int fd, rv;

	rv = -1;
#ifdef __GNUC__
	/* fix compiler warnings */
	symshdrp = NULL;
	symstrshdrp = NULL;
#endif

	/*
	 * Open and map the whole file.  If we can't open/stat it,
	 * something bad is going on so we punt.
	 */
	kfile = name;
	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		warn("%s", kfile);
		punt();
	}
	if (fstat(fd, &st) < 0) {
		warn("%s", kfile);
		punt();
	}
	if (st.st_size > SIZE_T_MAX)
		BAD;

	/*
	 * Map the file in its entirety.
	 */
	mappedsize = st.st_size;
	mappedfile = mmap(NULL, mappedsize, PROT_READ, MAP_PRIVATE|MAP_FILE,
	    fd, 0);
	if (mappedfile == (char *)-1)
		BAD;

	/*
	 * Make sure we can access the executable's header
	 * directly, and make sure the recognize the executable
	 * as an ELF binary.
	 */
	if (check(0, sizeof *ehdrp))
		BADUNMAP;
	ehdrp = (Elf_Ehdr *)&mappedfile[0];

	if (memcmp(ehdrp->e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdrp->e_ident[EI_CLASS] != ELFCLASS)
		BADUNMAP;

	switch (ehdrp->e_machine) {
	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		BADUNMAP;
	}

	/*
	 * We've recognized it as an ELF binary.  From here
	 * on out, all errors are fatal.
	 */

	/*
	 * Find the symbol list and string table.
	 */
	nshdr = ehdrp->e_shnum;
	shdr_off = ehdrp->e_shoff;
	shdr_size = ehdrp->e_shentsize * nshdr;

	if (check(shdr_off, shdr_size) ||
	    (sizeof *shdrp != ehdrp->e_shentsize))
		badfmt("bogus section header table");
	shdrp = (Elf_Shdr *)&mappedfile[shdr_off];

	for (i = 0; i < nshdr; i++) {
		if (shdrp[i].sh_type == SHT_SYMTAB) {
			symshdrp = &shdrp[i];
			symstrshdrp = &shdrp[shdrp[i].sh_link];
		}
	}

	if (symshdrp == NULL)
		badfmt("no symbol section header found");
	if (symshdrp->sh_offset == 0)
		badfmt("stripped");
	if (check(symshdrp->sh_offset, symshdrp->sh_size))
		badfmt("bogus symbol section header");
	if (check(symstrshdrp->sh_offset, symstrshdrp->sh_size))
		badfmt("bogus symbol string section header");

	symp = (Elf_Sym *)&mappedfile[symshdrp->sh_offset];
	nsyms = symshdrp->sh_size / sizeof(*symp);
	strtab = &mappedfile[symstrshdrp->sh_offset];

	/*
	 * Set up the data item, pointing to a nlist structure.
	 * which we fill in for each symbol.
	 */
	data.data = (u_char *)&nbuf;
	data.size = sizeof(nbuf);

	/*
	 * Create a buffer (to be expanded later, if necessary)
	 * to hold symbol names after we've added underscores
	 * to them.
	 */
	symnamesize = 1024;
	if ((symname = malloc(symnamesize)) == NULL) {
		warn("malloc");
		punt();
	}

	/*
	 * Read each symbol and enter it into the database.
	 */
	for (i = 0; i < nsyms; i++) {

		/*
		 * No symbol name; ignore this symbol.
		 */
		if (symp[i].st_name == 0)
			continue;

		/*
		 * Find symbol name, copy it (with added underscore) to
		 * temporary buffer, and prepare the database key for
		 * insertion.
		 */
		fsymname = &strtab[symp[i].st_name];
		fsymnamesize = strlen(fsymname) + 1;
		while (symnamesize < fsymnamesize + 1) {
			symnamesize *= 2;
			if ((symname = realloc(symname, symnamesize)) == NULL) {
				warn("malloc");
				punt();
			}
		}
		strcpy(symname, "_");
		strcat(symname, fsymname);

		key.data = symname;
		key.size = strlen((char *)key.data);

		/*
		 * Convert the symbol information into an nlist structure,
		 * as best we can.
		 */
		nbuf.n_value = symp[i].st_value;
		switch (ELFDEFNNAME(ST_TYPE)(symp[i].st_info)) {
		default:
		case STT_NOTYPE:
			nbuf.n_type = N_UNDF;
			break;
		case STT_OBJECT:
			nbuf.n_type = N_DATA;
			break;
		case STT_FUNC:
			nbuf.n_type = N_TEXT;
			break;
		case STT_FILE:
			nbuf.n_type = N_FN;
			break;
		}
		if (ELFDEFNNAME(ST_BIND)(symp[i].st_info) != STB_LOCAL)
			nbuf.n_type |= N_EXT;
		nbuf.n_desc = 0;				/* XXX */
		nbuf.n_other = 0;				/* XXX */

		/*
		 * Enter the symbol into the database.
		 */
		if (db->put(db, &key, &data, 0)) {
			warn("record enter");
			punt();
		}

		/*
		 * If it's the kernel version string, we've gotta keep
		 * some extra data around.  Under a separate key,
		 * we enter the first line (i.e. up to the first newline,
		 * with the newline replaced by a NUL to terminate the
		 * entered string) of the version string.
		 */
		if (strcmp((char *)key.data, VRS_SYM) == 0) {
			key.data = (u_char *)VRS_KEY;
			key.size = sizeof(VRS_KEY) - 1;
			/* Find the version string, relative to its section */
			data.data = strdup(&mappedfile[nbuf.n_value -
			    shdrp[symp[i].st_shndx].sh_addr +
			    shdrp[symp[i].st_shndx].sh_offset]);
			/* assumes newline terminates version. */
			if ((tmpcp = strchr(data.data, '\n')) != NULL)
				*tmpcp = '\0';
			data.size = strlen((char *)data.data);

			if (db->put(db, &key, &data, 0)) {
				warn("record enter");
				punt();
			}

			/* free pointer created by strdup(). */
			free(data.data);

			/* Restore to original values */
			data.data = (u_char *)&nbuf;
			data.size = sizeof(nbuf);
		}
	}

	rv = 0;

unmap:
	munmap(mappedfile, mappedsize);
out:
	return (rv);
}

#endif
