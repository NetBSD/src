/* $NetBSD: loadfile.c,v 1.2 1999/01/28 22:45:07 christos Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#ifndef INSTALLBOOT
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#else
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#endif

#include <sys/param.h>
#include <sys/exec.h>

#include "loadfile.h"

#ifdef BOOT_ECOFF
#include <sys/exec_ecoff.h>
static int coff_exec __P((int, struct ecoff_exechdr *, u_long *, int));
#endif
#ifdef BOOT_ELF
#include <sys/exec_elf.h>
static int elf_exec __P((int, Elf_Ehdr *, u_long *, int));
#endif
#ifdef BOOT_AOUT
#include <sys/exec_aout.h>
static int aout_exec __P((int, struct exec *, u_long *, int));
#endif

/*
 * Open 'filename', read in program and and return 0 if ok 1 on error.
 * Fill in entryp and endp.
 */
int
loadfile(fname, marks, flags)
	const char *fname;
	u_long *marks;
	int flags;
{
	union {
#ifdef BOOT_ECOFF
		struct ecoff_exechdr coff;
#endif
#ifdef BOOT_ELF
		Elf_Ehdr elf;
#endif
#ifdef BOOT_AOUT
		struct exec aout;
#endif
		
	} hdr;
	ssize_t nr;
	int fd, rval;

	/* Open the file. */
	if ((fd = open(fname, 0)) < 0) {
		WARN(("open %s", fname));
		return -1;
	}

	/* Read the exec header. */
	if ((nr = read(fd, &hdr, sizeof(hdr))) != sizeof(hdr)) {
		WARN(("read header"));
		goto err;
	}

#ifdef BOOT_ECOFF
	if (!ECOFF_BADMAG(&hdr.coff)) {
		rval = coff_exec(fd, &hdr.coff, marks, flags);
	} else
#endif
#ifdef BOOT_ELF
	if (memcmp(Elf_e_ident, hdr.elf.e_ident, Elf_e_siz) == 0) {
		rval = elf_exec(fd, &hdr.elf, marks, flags);
	} else
#endif
#ifdef BOOT_AOUT
	if (OKMAGIC(N_GETMAGIC(hdr.aout)) &&
	    N_GETMID(hdr.aout) == MID_MACHINE) {
		rval = aout_exec(fd, &hdr.aout, marks, flags);
	} else
#endif
	{
		rval = 1;
		errno = EFTYPE;
		WARN(("%s", fname));
	}

	if (rval == 0)
		PROGRESS(("=0x%lx\n", marks[MARK_END] - marks[MARK_START]));
	return fd;
err:
	(void)close(fd);
	return -1;
}

#ifdef BOOT_ECOFF
static int
coff_exec(fd, coff, marks, flags)
	int fd;
	struct ecoff_exechdr *coff;
	u_long *marks;
	int flags;
{
	paddr_t ffp_save = marks[MARK_START];

	/* Read in text. */
	if (lseek(fd, ECOFF_TXTOFF(coff), SEEK_SET) == -1)  {
		WARN(("lseek text"));
		return (1);
	}
	if (flags & LOAD_TEXT) {
		PROGRESS(("%lu", coff->a.tsize));
		if (READ(fd, coff->a.text_start, ffp_save, coff->a.tsize) !=
		    coff->a.tsize) {
			WARN(("read text"));
			return (1);
		}
	}
	else {
		if (lseek(fd, coff->a.tsize, SEEK_CUR) == -1) {
			WARN(("read text"));
			return (1);
		}
	}

	/* Read in data. */
	if (coff->a.dsize != 0 && (flags & LOAD_DATA)) {
		PROGRESS(("+%lu", coff->a.dsize));
		if (READ(fd, coff->a.data_start, ffp_save, coff->a.dsize) !=
		    coff->a.dsize) {
			WARN(("read data"));
			return (1);
		}
	}

	/* Zero out bss. */
	if (coff->a.bsize != 0 && (flags & LOAD_BSS)) {
		PROGRESS(("+%lu", coff->a.bsize));
		BZERO(coff->a.bss_start, ffp_save, coff->a.bsize);
	}

#ifndef INSTALLBOOT
	ffp_save = coff->a.text_start + coff->a.tsize;
	if (ffp_save < coff->a.data_start + coff->a.dsize)
		ffp_save = coff->a.data_start + coff->a.dsize;
	if (ffp_save < coff->a.bss_start + coff->a.bsize)
		ffp_save = coff->a.bss_start + coff->a.bsize;
#endif

	marks[MARK_START] = LOADADDR(coff->a.entry);
	marks[MARK_NSYM] = 1;	/* XXX: Kernel needs >= 0 */
	marks[MARK_SYM] = LOADADDR(ffp_save);
	marks[MARK_END] = LOADADDR(ffp_save);
	return (0);
}
#endif /* BOOT_ECOFF */

#ifdef BOOT_ELF
static int
elf_exec(fd, elf, marks, flags)
	int fd;
	Elf_Ehdr *elf;
	u_long *marks;
	int flags;
{
	Elf_Shdr *shp;
	Elf_Off off;
	int i;
	size_t sz;
	int first = 1;
	int havesyms;
	paddr_t ffp_save = marks[MARK_START], shp_save;
	vaddr_t elf_save;

	for (i = 0; i < elf->e_phnum; i++) {
		Elf_Phdr phdr;
		if (lseek(fd, elf->e_phoff + sizeof(phdr) * i, SEEK_SET)
		    == -1)  {
			WARN(("lseek phdr"));
			return (1);
		}
		if (read(fd, (void *)&phdr, sizeof(phdr)) != sizeof(phdr)) {
			WARN(("read phdr"));
			return (1);
		}
		if (phdr.p_type != Elf_pt_load ||
		    (phdr.p_flags & (Elf_pf_w|Elf_pf_x)) == 0)
			continue;

		/* Read in segment. */
		PROGRESS(("%s%lu", first ? "" : "+", (u_long)phdr.p_filesz));
		if (lseek(fd, phdr.p_offset, SEEK_SET) == -1)  {
		    WARN(("lseek text"));
		    return (1);
		}
		/* XXX: Assume text comes before data! */
		if ((first && (flags & LOAD_TEXT)) ||
		    (!first && (flags & LOAD_DATA))) {
			if (READ(fd, phdr.p_vaddr, ffp_save, phdr.p_filesz) !=
			    phdr.p_filesz) {
				WARN(("read text"));
				return (1);
			}
		}

#ifndef INSTALLBOOT
		if (first || ffp_save < phdr.p_vaddr + phdr.p_memsz)
			ffp_save = phdr.p_vaddr + phdr.p_memsz;
#endif

		/* Zero out bss. */
		if (phdr.p_filesz < phdr.p_memsz && (flags & LOAD_BSS)) {
			PROGRESS(("+%lu",
			    (u_long)(phdr.p_memsz - phdr.p_filesz)));
			BZERO((phdr.p_vaddr + phdr.p_filesz), ffp_save,
			    phdr.p_memsz - phdr.p_filesz);
		}
		first = 0;
	}
	/*
	 * Copy the ELF and section headers.
	 */
	ffp_save = roundup(ffp_save, sizeof(long));
	if (flags & LOAD_HDR) {
		elf_save = ffp_save;
		ffp_save += sizeof(Elf_Ehdr);
	}

	if (flags & LOAD_SYM) {
		if (lseek(fd, elf->e_shoff, SEEK_SET) == -1)  {
			WARN(("lseek section headers"));
			return (1);
		}
		sz = elf->e_shnum * sizeof(Elf_Shdr);

		shp = ALLOC(sz);

		if (read(fd, shp, sz) != sz) {
			WARN(("read section headers"));
			return (1);
		}

		shp_save = ffp_save;
		ffp_save += roundup(sz, sizeof(long));

		/*
		 * Now load the symbol sections themselves.  Make sure the
		 * sections are aligned. Don't bother with string tables if
		 * there are no symbol sections.
		 */
		off = roundup((sizeof(Elf_Ehdr) + sz), sizeof(long));

		for (havesyms = i = 0; i < elf->e_shnum; i++)
			if (shp[i].sh_type == Elf_sht_symtab)
				havesyms = 1;

		for (first = 1, i = 0; i < elf->e_shnum; i++) {
			if (shp[i].sh_type == Elf_sht_symtab ||
			    shp[i].sh_type == Elf_sht_strtab) {
				PROGRESS(("%s%ld", first ? " [" : "+",
				    (u_long)shp[i].sh_size));
				if (havesyms) {
					if (lseek(fd, shp[i].sh_offset,
					    SEEK_SET)
						== -1) {
						WARN(("lseek symbols"));
						FREE(shp, sz);
						return (1);
					}
					if (READ(fd, ffp_save, ffp_save, 
					    shp[i].sh_size) != shp[i].sh_size) {
						WARN(("read symbols"));
						FREE(shp, sz);
						return (1);
					}
				}
				ffp_save += roundup(shp[i].sh_size,
				    sizeof(long));
				shp[i].sh_offset = off;
				off += roundup(shp[i].sh_size, sizeof(long));
				first = 0;
			}
		}
		BCOPY(shp, shp_save, shp_save, sz);
		FREE(shp, sz);

		if (first == 0)
			PROGRESS(("]"));
	}

	/*
	 * Frob the copied ELF header to give information relative
	 * to elf_save.
	 */
	if (flags & LOAD_HDR) {
		elf->e_phoff = 0;
		elf->e_shoff = sizeof(Elf_Ehdr);
		elf->e_phentsize = 0;
		elf->e_phnum = 0;
		BCOPY(elf, elf_save, elf_save, sizeof(*elf));
	}

	marks[MARK_START] = LOADADDR(elf->e_entry);
	marks[MARK_NSYM] = 1;	/* XXX: Kernel needs >= 0 */
	marks[MARK_SYM] = LOADADDR(elf_save);
	marks[MARK_END] = LOADADDR(ffp_save);
	return (0);
}
#endif /* BOOT_ELF */

#ifdef BOOT_AOUT
static int
aout_exec(fd, x, marks, flags)
	int fd;
	struct exec *x;
	u_long *marks;
	int flags;
{
	u_long entry = x->a_entry;
	paddr_t ffp_save, aout_save = 0;
	int cc;

	if (marks[MARK_START] == 0)
		ffp_save = ALIGNENTRY(entry);
	else
		ffp_save = marks[MARK_START];
		
	if (lseek(fd, sizeof(*x), SEEK_SET) == -1)  {
		WARN(("lseek text"));
		return 1;
	}

	/*
	 * Leave a copy of the exec header before the text.
	 * The kernel may use this to verify that the
	 * symbols were loaded by this boot program.
	 */
	if (flags & LOAD_HDR) {
		BCOPY(x, ffp_save, ffp_save, sizeof(*x));
		ffp_save += sizeof(*x);
	}
	/*
	 * Read in the text segment.
	 */
	if (flags & LOAD_TEXT) {
		PROGRESS(("%ld", x->a_text));

		if (READ(fd, ffp_save, ffp_save, x->a_text - sizeof(*x)) !=
		    x->a_text - sizeof(*x)) {
			WARN(("read text"));
			return 1;
		}
		ffp_save += x->a_text - sizeof(*x);
	} else {
		if (lseek(fd, x->a_text - sizeof(*x), SEEK_CUR) == -1) {
			WARN(("seek text"));
			return (1);
		}
	}

	/*
	 * Read in the data segment.
	 */
	if (flags & LOAD_DATA) {
		PROGRESS(("+%ld", x->a_data));

		if (READ(fd, ffp_save, ffp_save, x->a_data) != x->a_data) {
			WARN(("read data"));
			return 1;
		}
		ffp_save += x->a_data;
	}

	/*
	 * Zero out the BSS section.
	 * (Kernel doesn't care, but do it anyway.)
	 */
	if (flags & LOAD_BSS) {
		PROGRESS(("+%ld", x->a_bss));

		BZERO(ffp_save, ffp_save, x->a_bss);
		ffp_save += x->a_bss;
	}

	/*
	 * Read in the symbol table and strings.
	 * (Always set the symtab size word.)
	 */
	if (flags & LOAD_SYM) {
		BCOPY(&x->a_syms, ffp_save, ffp_save, sizeof(x->a_syms));
		ffp_save += sizeof(x->a_syms);
		aout_save = ffp_save;

		if (x->a_syms > 0) {
			/* Symbol table and string table length word. */

			PROGRESS(("+[%ld", x->a_syms));

			if (READ(fd, ffp_save, ffp_save, x->a_syms) !=
			    x->a_syms) {
				WARN(("read symbols"));
				return 1;
			}
			ffp_save += x->a_syms;

			read(fd, &cc, sizeof(cc));

			BCOPY(&cc, ffp_save, ffp_save, sizeof(cc));
			ffp_save += sizeof(cc);

			/* String table.  Length word includes itself. */

			PROGRESS(("+%d]", cc));

			cc -= sizeof(int);
			if (cc <= 0) {
				WARN(("symbol table too short"));
				return 1;
			}
			if (READ(fd, ffp_save, ffp_save, cc) != cc) {
				WARN(("read strings"));
				return 1;
			}
			ffp_save += cc;
		}
	}

	marks[MARK_START] = LOADADDR(entry);
	marks[MARK_NSYM] = x->a_syms;
	marks[MARK_SYM] = LOADADDR(aout_save);
	marks[MARK_END] = LOADADDR(ffp_save);

	return 0;
}
#endif /* BOOT_AOUT */
