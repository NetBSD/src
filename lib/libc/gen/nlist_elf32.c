/*	$NetBSD: nlist_elf32.c,v 1.1 1996/09/27 22:23:06 cgd Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)nlist.c	8.1 (Berkeley) 6/4/93";
#else
static char rcsid[] = "$NetBSD: nlist_elf32.c,v 1.1 1996/09/27 22:23:06 cgd Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/* If not included by nlist_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <a.out.h>			/* for 'struct nlist' declaration */

#include "nlist_private.h"
#if defined(NLIST_ELF32) || defined(NLIST_ELF64)
#include <sys/exec_elf.h>
#endif

#if (defined(NLIST_ELF32) && (ELFSIZE == 32)) || \
    (defined(NLIST_ELF64) && (ELFSIZE == 64))

#define	CONCAT(x,y)	__CONCAT(x,y)
#define	ELFNAME(x)	CONCAT(elf,CONCAT(ELFSIZE,CONCAT(_,x)))
#define	ELFNAME2(x,y)	CONCAT(x,CONCAT(_elf,CONCAT(ELFSIZE,CONCAT(_,y))))
#define	ELFNAMEEND(x)	CONCAT(x,CONCAT(_elf,ELFSIZE))
#define	ELFDEFNNAME(x)	CONCAT(ELF,CONCAT(ELFSIZE,CONCAT(_,x)))

int
ELFNAMEEND(__fdnlist_is)(fd)
	int fd;
{
        Elf_Ehdr ehdr;

        if (lseek(fd, (off_t)0, SEEK_SET) == -1 ||
            read(fd, &ehdr, sizeof(Elf_Ehdr)) != sizeof(Elf_Ehdr))
                return (-1);

	if (bcmp(ehdr.e_ident, Elf_e_ident, Elf_e_siz))
		return (-1);

	switch (ehdr.e_machine) {
	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		return (-1);
	}

	return (0);
}

int
ELFNAMEEND(__fdnlist)(fd, list)
        register int fd;
        register struct nlist *list;
{
        register struct nlist *p;
        register caddr_t strtab;
#if (ELFSIZE == 32)
        register Elf32_Word symsize, symstrsize;
        register Elf32_Off symoff = 0, symstroff;
#elif (ELFSIZE == 64)
        register Elf64_Word symsize, symstrsize;
        register Elf64_Off symoff = 0, symstroff;
#endif
        register long nent, cc, i;
        Elf_Sym sbuf[1024];
        Elf_Sym *s;
        Elf_Ehdr ehdr;
        Elf_Shdr *shdr = NULL;
        Elf_Word shdr_size;
        struct stat st;

        /* Make sure obj is OK */
        if (lseek(fd, (off_t)0, SEEK_SET) == -1 ||
            read(fd, &ehdr, sizeof(Elf_Ehdr)) != sizeof(Elf_Ehdr) ||
            fstat(fd, &st) < 0)
                return (-1);

        /* calculate section header table size */
        shdr_size = ehdr.e_shentsize * ehdr.e_shnum;

        /* Make sure it's not too big to mmap */
        if (shdr_size > SIZE_T_MAX) {
                errno = EFBIG;
                return (-1);
        }

        /* mmap section header table */
        shdr = (Elf_Shdr *)mmap(NULL, (size_t)shdr_size,
                                  PROT_READ, 0, fd, ehdr.e_shoff);
        if (shdr == (Elf_Shdr *)-1)
                return (-1);

        /*
         * Find the symbol table entry and it's corresponding
         * string table entry.  Version 1.1 of the ABI states
         * that there is only one symbol table but that this
         * could change in the future.
         */
        for (i = 0; i < ehdr.e_shnum; i++) {
                if (shdr[i].sh_type == Elf_sht_symtab) {
                        symoff = shdr[i].sh_offset;
                        symsize = shdr[i].sh_size;
                        symstroff = shdr[shdr[i].sh_link].sh_offset;
                        symstrsize = shdr[shdr[i].sh_link].sh_size;
                        break;
                }
        }

        /* Flush the section header table */
        munmap((caddr_t)shdr, shdr_size);

        /* Check for files too large to mmap. */
        /* XXX is this really possible? */
        if (symstrsize > SIZE_T_MAX) {
                errno = EFBIG;
                return (-1);
        }
        /*
         * Map string table into our address space.  This gives us
         * an easy way to randomly access all the strings, without
         * making the memory allocation permanent as with malloc/free
         * (i.e., munmap will return it to the system).
         */
        strtab = mmap(NULL, (size_t)symstrsize, PROT_READ, 0, fd, symstroff);
        if (strtab == (char *)-1)
                return (-1);
        /*
         * clean out any left-over information for all valid entries.
         * Type and value defined to be 0 if not found; historical
         * versions cleared other and desc as well.  Also figure out
         * the largest string length so don't read any more of the
         * string table than we have to.
         *
         * XXX clearing anything other than n_type and n_value violates
         * the semantics given in the man page.
         */
        nent = 0;
        for (p = list; !ISLAST(p); ++p) {
                p->n_type = 0;
                p->n_other = 0;
                p->n_desc = 0;
                p->n_value = 0;
                ++nent;
        }

        /* Don't process any further if object is stripped. */
        /* ELFism - dunno if stripped by looking at header */
        if (symoff == 0)
                goto done;
                
        if (lseek(fd, symoff, SEEK_SET) == -1) {
                nent = -1;
                goto done;
        }
        
        while (symsize > 0) {
                cc = MIN(symsize, sizeof(sbuf));
                if (read(fd, sbuf, cc) != cc)
                        break;
                symsize -= cc;
                for (s = sbuf; cc > 0; ++s, cc -= sizeof(*s)) {
                        register int soff = s->st_name;

                        if (soff == 0)
                                continue;

                        for (p = list; !ISLAST(p); p++) {
				char *nlistname;

				nlistname = p->n_un.n_name;
				if (*nlistname == '_')
					nlistname++;

                                if (!strcmp(&strtab[soff], nlistname)) {
                                        p->n_value = s->st_value;

                                        /* XXX - type conversion */
                                        /*       is pretty rude. */
                                        switch(ELF_SYM_TYPE(s->st_info)) {
                                                case Elf_estt_notype:
                                                        p->n_type = N_UNDF;
                                                        break;
                                                case Elf_estt_object:
                                                        p->n_type = N_DATA;
                                                        break;
                                                case Elf_estt_func:
                                                        p->n_type = N_TEXT;
                                                        break;
                                                case Elf_estt_file:
                                                        p->n_type = N_FN;
                                                        break;
                                        }
                                        if (ELF_SYM_BIND(s->st_info) !=
                                            Elf_estb_local)
                                                p->n_type |= N_EXT;
                                        p->n_desc = 0;
                                        p->n_other = 0;
                                        if (--nent <= 0)
                                                break;
                                }
                        }
                }
        }
  done:
        munmap(strtab, symstrsize);

        return (nent);
}

#endif
