/*	$NetBSD: exec_elf32.c,v 1.7 1999/10/21 21:16:07 erh Exp $	*/

/*
 * Copyright (c) 1997, 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: exec_elf32.c,v 1.7 1999/10/21 21:16:07 erh Exp $");
#endif
 
#ifndef ELFSIZE
#define ELFSIZE         32
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

#include <sys/exec_elf.h>

struct listelem {
	struct listelem *next;
	void *mem;
	off_t file;
	size_t size;
};

static ssize_t
xreadatoff(int fd, void *buf, off_t off, size_t size, const char *fn)
{
	ssize_t rv;

	if (lseek(fd, off, SEEK_SET) != off) {
		perror(fn);
		return -1;
	}
	if ((rv = read(fd, buf, size)) != size) {
		fprintf(stderr, "%s: read error: %s\n", fn,
		    rv == -1 ? strerror(errno) : "short read");
		return -1;
	}
	return size;
}

static ssize_t
xwriteatoff(int fd, void *buf, off_t off, size_t size, const char *fn)
{
	ssize_t rv;

	if (lseek(fd, off, SEEK_SET) != off) {
		perror(fn);
		return -1;
	}
	if ((rv = write(fd, buf, size)) != size) {
		fprintf(stderr, "%s: write error: %s\n", fn,
		    rv == -1 ? strerror(errno) : "short write");
		return -1;
	}
	return size;
}

static void *
xmalloc(size_t size, const char *fn, const char *use)
{
	void *rv;

	rv = malloc(size);
	if (rv == NULL)
		fprintf(stderr, "%s: out of memory (allocating for %s)\n",
		    fn, use);
	return (rv);
}

static void *
xrealloc(void *ptr, size_t size, const char *fn, const char *use)
{
	void *rv;

	rv = realloc(ptr, size);
	if (rv == NULL) {
		free(ptr);
		fprintf(stderr, "%s: out of memory (reallocating for %s)\n",
		    fn, use);
	}
	return (rv);
}

int
ELFNAMEEND(check)(int fd, const char *fn)
{
	Elf_Ehdr eh;
	struct stat sb;

	/*
	 * Check the header to maek sure it's an ELF file (of the
	 * appropriate size).
	 */
	if (fstat(fd, &sb) == -1)
		return 0;
	if (sb.st_size < sizeof eh)
		return 0;
	if (read(fd, &eh, sizeof eh) != sizeof eh)
		return 0;

	if (memcmp(eh.e_ident, Elf_e_ident, Elf_e_siz))
                return 0;

        switch (eh.e_machine) {
        ELFDEFNNAME(MACHDEP_ID_CASES)

        default:
                return 0;
        }

	return 1;
}

/*
 * This function 'hides' (some of) ELF executable file's symbols.
 * It hides them by renaming them to "_$$hide$$ <filename> <symbolname>".
 * Symbols in the global keep list, or which are marked as being undefined,
 * are left alone.
 *
 * An old version of this code shuffled various tables around, turning
 * global symbols to be hidden into local symbols.  That lost on the
 * mips, because CALL16 relocs must reference global symbols, and, if
 * those symbols were being hidden, they were no longer global.
 *
 * The new renaming behaviour doesn't take global symbols out of the
 * namespace.  However, it's ... unlikely that there will ever be
 * any collisions in practice because of the new method.
 */
int
ELFNAMEEND(hide)(int fd, const char *fn)
{
	Elf_Ehdr ehdr;
	Elf_Shdr *shdrp = NULL;
	int symtabsnum, strtabsnum;
	Elf_Sym *symtabp = NULL;
	char *strtabp = NULL, *nstrtabp = NULL;
	Elf_Word nsyms;
	Elf_Off stroff, maxoff;
	const char *weirdreason;
	ssize_t shdrsize;
	size_t nstrtab_size, nstrtab_nextoff, fn_size;
	int rv, i, weird;

	rv = 0;
	if (xreadatoff(fd, &ehdr, 0, sizeof ehdr, fn) != sizeof ehdr)
		goto bad;

	shdrsize = ehdr.e_shnum * ehdr.e_shentsize;
	if ((shdrp = xmalloc(shdrsize, fn, "section header table")) == NULL)
		goto bad;
	if (xreadatoff(fd, shdrp, ehdr.e_shoff, shdrsize, fn) != shdrsize)
		goto bad;

	symtabsnum = strtabsnum = -1;
	maxoff = stroff = 0;
	weird = 0;
	weirdreason = "???";
	for (i = 0; i < ehdr.e_shnum; i++) {
		if (shdrp[i].sh_offset > maxoff) {
			maxoff = shdrp[i].sh_offset;
		}
		switch (shdrp[i].sh_type) {
		case Elf_sht_symtab:
			if (!weird && symtabsnum != -1) {
				weird = 1;
				weirdreason = "multiple symbol tables";
			}
			symtabsnum = i;
			strtabsnum = shdrp[i].sh_link;
			stroff = shdrp[strtabsnum].sh_offset;
			if (!weird && strtabsnum != (ehdr.e_shnum - 1)) {
				weird = 1;
				weirdreason = "string table not last section";
			}
			break;
		}
	}
	if (symtabsnum == -1)
		goto out;
	if (!weird && strtabsnum == -1) {
		weird = 1;
		weirdreason = "no string table found";
	}
	if (!weird && stroff != maxoff) {
		weird = 1;
		weirdreason = "string table section not last in file";
	}
	if (weird) {
		fprintf(stderr, "%s: weird executable (%s); unsupported\n", fn,
		    weirdreason);
		goto bad;
	}

	/*
	 * load up everything we need
	 */

	/* symbol table */
	if ((symtabp = xmalloc(shdrp[symtabsnum].sh_size, fn, "symbol table"))
	    == NULL)
		goto bad;
	if (xreadatoff(fd, symtabp, shdrp[symtabsnum].sh_offset,
	    shdrp[symtabsnum].sh_size, fn) != shdrp[symtabsnum].sh_size)
		goto bad;

	/* string table */
	if ((strtabp = xmalloc(shdrp[strtabsnum].sh_size, fn, "string table"))
	    == NULL)
		goto bad;
	if (xreadatoff(fd, strtabp, shdrp[strtabsnum].sh_offset,
	    shdrp[strtabsnum].sh_size, fn) != shdrp[strtabsnum].sh_size)
		goto bad;

	nsyms = shdrp[symtabsnum].sh_size / shdrp[symtabsnum].sh_entsize;

	nstrtab_size = 256;
	nstrtabp = xmalloc(nstrtab_size, fn, "new string table");
	if (nstrtabp == NULL)
		goto bad;
	nstrtab_nextoff = 0;

	fn_size = strlen(fn);

	for (i = 0; i < nsyms; i++) {
		Elf_Sym *sp = &symtabp[i];
		const char *symname = strtabp + sp->st_name;
		size_t newent_len;

		/*
		 * make sure there's size for the next entry, even if it's
		 * as large as it can be.
		 *
		 * "_$$hide$$ <filename> <symname><NUL>" ->
		 *    9 + 3 + sizes of fn and sym name
		 */
		while ((nstrtab_size - nstrtab_nextoff) <
		    strlen(symname) + fn_size + 12) {
			nstrtab_size *= 2;
			nstrtabp = xrealloc(nstrtabp, nstrtab_size, fn,
			    "new string table");
			if (nstrtabp == NULL)
				goto bad;
		}

		sp->st_name = nstrtab_nextoff;

		/* if it's a keeper or is undefined, don't rename it. */
		if (in_keep_list(symname) ||
		    sp->st_shndx == Elf_eshn_undefined) {
			newent_len = sprintf(nstrtabp + nstrtab_nextoff,
			    "%s", symname) + 1;
		} else {
			newent_len = sprintf(nstrtabp + nstrtab_nextoff,
			    "_$$hide$$ %s %s", fn, symname) + 1;
		}
		nstrtab_nextoff += newent_len;
	}	
	shdrp[strtabsnum].sh_size = nstrtab_nextoff;

	/*
	 * write new tables to the file
	 */
	if (xwriteatoff(fd, shdrp, ehdr.e_shoff, shdrsize, fn) != shdrsize)
		goto bad;
	if (xwriteatoff(fd, symtabp, shdrp[symtabsnum].sh_offset,
	    shdrp[symtabsnum].sh_size, fn) != shdrp[symtabsnum].sh_size)
		goto bad;
	if (xwriteatoff(fd, nstrtabp, shdrp[strtabsnum].sh_offset,
	    shdrp[strtabsnum].sh_size, fn) != shdrp[strtabsnum].sh_size)
		goto bad;

out:
	if (shdrp != NULL)
		free(shdrp);
	if (symtabp != NULL)
		free(symtabp);
	if (strtabp != NULL)
		free(strtabp);
	if (nstrtabp != NULL)
		free(nstrtabp);
	return (rv);

bad:
	rv = 1;
	goto out;
}

#endif /* include this size of ELF */
