/* $NetBSD: loadfile.c,v 1.1 1999/01/28 20:18:31 christos Exp $ */

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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>

#include "loadfile.h"

#ifdef BOOT_ECOFF
#include <sys/exec_ecoff.h>
static int coff_exec __P((int, struct ecoff_exechdr *, u_long *));
#endif
#ifdef BOOT_ELF
#include <sys/exec_elf.h>
static int elf_exec __P((int, Elf_Ehdr *, u_long *));
#endif
#ifdef BOOT_AOUT
#include <sys/exec_aout.h>
static int aout_exec __P((int, struct exec *, u_long *));
#endif

/*
 * Open 'filename', read in program and and return 0 if ok 1 on error.
 * Fill in entryp and endp.
 */
int
loadfile(fname, marks)
	const char *fname;
	u_long *marks;
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
		(void)printf("open %s: %s\n", fname, strerror(errno));
		return -1;
	}

	/* Read the exec header. */
	if ((nr = read(fd, &hdr, sizeof(hdr))) != sizeof(hdr)) {
		(void)printf("read header: %s\n", strerror(errno));
		goto err;
	}

#ifdef BOOT_ECOFF
	if (!ECOFF_BADMAG(&hdr.coff)) {
		rval = coff_exec(fd, &hdr.coff, marks);
	} else
#endif
#ifdef BOOT_ELF
	if (memcmp(Elf_e_ident, hdr.elf.e_ident, Elf_e_siz) == 0) {
		rval = elf_exec(fd, &hdr.elf, marks);
	} else
#endif
#ifdef BOOT_AOUT
	if (N_GETMAGIC(hdr.aout) == ZMAGIC &&
	    N_GETMID(hdr.aout) == MID_MACHINE) {
		rval = aout_exec(fd, &hdr.aout, marks);
	} else
#endif
	{
		rval = 1;
		(void)printf("%s: unknown executable format\n", fname);
	}

	if (rval == 0)
		(void)printf("=0x%lx\n", marks[LOAD_END] - marks[LOAD_START]);
	return fd;
err:
	(void)close(fd);
	return -1;
}

#ifdef BOOT_ECOFF
static int
coff_exec(fd, coff, marks)
	int fd;
	struct ecoff_exechdr *coff;
	u_long *marks;
{
	paddr_t ffp_save;

	/* Read in text. */
	(void)printf("%lu", coff->a.tsize);
	if (lseek(fd, ECOFF_TXTOFF(coff), SEEK_SET) == -1)  {
		(void)printf("lseek text: %s\n", strerror(errno));
		return (1);
	}
	if (READ(fd, coff->a.text_start, coff->a.tsize) !=
	    coff->a.tsize) {
		(void)printf("read text: %s\n", strerror(errno));
		return (1);
	}

	/* Read in data. */
	if (coff->a.dsize != 0) {
		(void)printf("+%lu", coff->a.dsize);
		if (READ(fd, coff->a.data_start, coff->a.dsize) !=
		    coff->a.dsize) {
			(void)printf("read data: %s\n", strerror(errno));
			return (1);
		}
	}


	/* Zero out bss. */
	if (coff->a.bsize != 0) {
		(void)printf("+%lu", coff->a.bsize);
		BZERO(coff->a.bss_start, coff->a.bsize);
	}

	ffp_save = coff->a.text_start + coff->a.tsize;
	if (ffp_save < coff->a.data_start + coff->a.dsize)
		ffp_save = coff->a.data_start + coff->a.dsize;
	if (ffp_save < coff->a.bss_start + coff->a.bsize)
		ffp_save = coff->a.bss_start + coff->a.bsize;

	marks[LOAD_START] = LOADADDR(coff->a.entry);
	marks[LOAD_NSYM] = 1;	/* XXX: Kernel needs >= 0 */
	marks[LOAD_SYM] = LOADADDR(ffp_save);
	marks[LOAD_END] = LOADADDR(ffp_save);
	return (0);
}
#endif /* BOOT_ECOFF */

#ifdef BOOT_ELF
static int
elf_exec(fd, elf, marks)
	int fd;
	Elf_Ehdr *elf;
	u_long *marks;
{
	Elf_Shdr *shp;
	Elf_Off off;
	int i;
	size_t sz;
	int first = 1;
	int havesyms;
	paddr_t ffp_save = marks[LOAD_START], shp_save;
	vaddr_t elf_save;

	for (i = 0; i < elf->e_phnum; i++) {
		Elf_Phdr phdr;
		if (lseek(fd, elf->e_phoff + sizeof(phdr) * i, SEEK_SET)
		    == -1)  {
			(void)printf("lseek phdr: %s\n", strerror(errno));
			return (1);
		}
		if (read(fd, (void *)&phdr, sizeof(phdr)) != sizeof(phdr)) {
			(void)printf("read phdr: %s\n", strerror(errno));
			return (1);
		}
		if (phdr.p_type != Elf_pt_load ||
		    (phdr.p_flags & (Elf_pf_w|Elf_pf_x)) == 0)
			continue;

		/* Read in segment. */
		(void)printf("%s%lu", first ? "" : "+", (u_long)phdr.p_filesz);
		if (lseek(fd, phdr.p_offset, SEEK_SET) == -1)  {
		    (void)printf("lseek text: %s\n", strerror(errno));
		    return (1);
		}
		if (READ(fd, phdr.p_vaddr, phdr.p_filesz) !=
		    phdr.p_filesz) {
			(void)printf("read text: %s\n", strerror(errno));
			return (1);
		}

		if (first || ffp_save < phdr.p_vaddr + phdr.p_memsz)
			ffp_save = phdr.p_vaddr + phdr.p_memsz;

		/* Zero out bss. */
		if (phdr.p_filesz < phdr.p_memsz) {
			(void)printf("+%lu",
			    (u_long)(phdr.p_memsz - phdr.p_filesz));
			BZERO((phdr.p_vaddr + phdr.p_filesz),
			    phdr.p_memsz - phdr.p_filesz);
		}
		first = 0;
	}
	/*
	 * Copy the ELF and section headers.
	 */
	ffp_save = roundup(ffp_save, sizeof(long));
	elf_save = ffp_save;
	ffp_save += sizeof(Elf_Ehdr);

	if (lseek(fd, elf->e_shoff, SEEK_SET) == -1)  {
		(void)printf("lseek section headers: %s\n", strerror(errno));
		return (1);
	}
	sz = elf->e_shnum * sizeof(Elf_Shdr);

	shp = alloc(sz);

	if (read(fd, shp, sz) != sz) {
		(void)printf("read section headers: %s\n", strerror(errno));
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
			(void)printf("%s%ld", first ? " [" : "+",
			    (u_long)shp[i].sh_size);
			if (havesyms) {
				if (lseek(fd, shp[i].sh_offset, SEEK_SET)
					== -1) {
					(void)printf("\nlseek symbols: %s\n",
					    strerror(errno));
					free(shp, sz);
					return (1);
				}
				if (READ(fd, ffp_save, shp[i].sh_size)
					!= shp[i].sh_size) {
					(void)printf("\nread symbols: %s\n",
					    strerror(errno));
					free(shp, sz);
					return (1);
				}
			}
			ffp_save += roundup(shp[i].sh_size, sizeof(long));
			shp[i].sh_offset = off;
			off += roundup(shp[i].sh_size, sizeof(long));
			first = 0;
		}
	}
	BCOPY(shp, shp_save, sz);
	free(shp, sz);

	if (first == 0)
		(void)printf("]");

	/*
	 * Frob the copied ELF header to give information relative
	 * to elf_save.
	 */
	elf->e_phoff = 0;
	elf->e_shoff = sizeof(Elf_Ehdr);
	elf->e_phentsize = 0;
	elf->e_phnum = 0;
	BCOPY(elf, elf_save, sizeof(*elf));

	marks[LOAD_START] = LOADADDR(elf->e_entry);
	marks[LOAD_NSYM] = 1;	/* XXX: Kernel needs >= 0 */
	marks[LOAD_SYM] = LOADADDR(elf_save);
	marks[LOAD_END] = LOADADDR(ffp_save);
	return (0);
}
#endif /* BOOT_ELF */

#ifdef BOOT_AOUT
static int
aout_exec(fd, x, marks)
	int fd;
	struct exec *x;
	u_long *marks;
{
	u_long entry = x->a_entry;
	paddr_t ffp_save, aout_save;
	int cc;

	if (marks[LOAD_START] == 0)
		ffp_save = ALIGNENTRY(entry);
	else
		ffp_save = marks[LOAD_START];
		
	if (lseek(fd, sizeof(*x), SEEK_SET) == -1)  {
		(void)printf("lseek a.out: %s\n", strerror(errno));
		return 1;
	}

	/*
	 * Leave a copy of the exec header before the text.
	 * The kernel may use this to verify that the
	 * symbols were loaded by this boot program.
	 */
	BCOPY(x, ffp_save, sizeof(*x));
	ffp_save += sizeof(*x);
	/*
	 * Read in the text segment.
	 */

	(void)printf("%ld", x->a_text);

	if (READ(fd, ffp_save, x->a_text - sizeof(*x)) !=
	    x->a_text - sizeof(*x)) {
		(void)printf("read text: %s\n", strerror(errno));
		return 1;
	}
	ffp_save += x->a_text - sizeof(*x);

	/*
	 * Read in the data segment.
	 */

	(void)printf("+%ld", x->a_data);

	if (READ(fd, ffp_save, x->a_data) != x->a_data) {
		(void)printf("read data: %s\n", strerror(errno));
		return 1;
	}
	ffp_save += x->a_data;

	/*
	 * Zero out the BSS section.
	 * (Kernel doesn't care, but do it anyway.)
	 */


	(void)printf("+%ld", x->a_bss);

	BZERO(ffp_save, x->a_bss);
	ffp_save += x->a_bss;

	/*
	 * Read in the symbol table and strings.
	 * (Always set the symtab size word.)
	 */
	BCOPY(&x->a_syms, ffp_save, sizeof(x->a_syms));
	ffp_save += sizeof(x->a_syms);
	aout_save = ffp_save;

	if (x->a_syms > 0) {
		/* Symbol table and string table length word. */

		(void)printf("+[%ld", x->a_syms);

		if (READ(fd, ffp_save, x->a_syms) != x->a_syms) {
			(void)printf("read symbols: %s\n", strerror(errno));
			return 1;
		}
		ffp_save += x->a_syms;

		read(fd, &cc, sizeof(cc));

		BCOPY(&cc, ffp_save, sizeof(cc));
		ffp_save += sizeof(cc);

		/* String table.  Length word includes itself. */

		(void)printf("+%d]", cc);

		cc -= sizeof(int);
		if (cc <= 0) {
			(void)printf("symbol table too short\n");
			return 1;
		}
		if (READ(fd, ffp_save, cc) != cc) {
			(void)printf("read strings: %s\n", strerror(errno));
			return 1;
		}
		ffp_save += cc;
	}

	marks[LOAD_START] = LOADADDR(entry);
	marks[LOAD_NSYM] = x->a_syms;
	marks[LOAD_SYM] = LOADADDR(aout_save);
	marks[LOAD_END] = LOADADDR(ffp_save);

	return 0;
}
#endif /* BOOT_AOUT */
