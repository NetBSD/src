/*	$NetBSD: boot.c,v 1.1 1999/03/25 12:11:41 simonb Exp $	*/

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

#undef OLD_LOADFILE	/* XXX */

#define PMAX_BOOT_AOUT
#define PMAX_BOOT_ECOFF
#define PMAX_BOOT_ELF

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <stand.h>
#include <machine/dec_prom.h>

#ifndef OLD_LOADFILE
#include "loadfile.h"
#endif

#include "bootinfo.h"
#include "byteswap.h"

int	errno;
extern	char bootprog_name[], bootprog_rev[], bootprog_date[], bootprog_maker[];

/*
 * This gets arguments from the PROM, calls other routines to open
 * and load the program to boot, and then transfers execution to that
 * new program.
 * Argv[0] should be something like "rz(0,0,0)vmunix" on a DECstation 3100.
 * Argv[0,1] should be something like "boot 5/rz0/vmunix" on a DECstation 5000.
 * The argument "-a" means vmunix should do an automatic reboot.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	int entry;
#ifndef OLD_LOADFILE
	u_long marks[MARK_MAX];
	struct btinfo_symtab bi_syms;
#endif
	struct btinfo_bootpath bi_bpath;

	/* initialise bootinfo structure early */
	bi_init(BOOTINFO_ADDR);

	/* check for DS5000 boot */
	if (strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}
	cp = *argv;
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	printf("Boot: %s\n", cp);
	strncpy(bi_bpath.bootpath, cp, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));
#ifdef OLD_LOADFILE
	entry = loadfile(cp);
	if (entry == -1)
		return 0;
#else
	marks[MARK_START] = 0;
	if (loadfile(cp, marks, LOAD_ALL) == -1)
		return 0;
	entry = marks[MARK_ENTRY];
	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));
#endif

	printf("Starting at 0x%x\n\n", entry);
	if (callv == &callvec)
		startprog(entry, entry, argc, argv, 0, 0,
		    BOOTINFO_MAGIC, BOOTINFO_ADDR);
	else
		startprog(entry, entry, argc, argv, DEC_PROM_MAGIC,
		    callv, BOOTINFO_MAGIC, BOOTINFO_ADDR);
}

#ifdef OLD_LOADFILE
/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
loadfile(fname)
	register char *fname;
{
	register struct devices *dp;
	register int fd, i, n;
	union {
#ifdef PMAX_BOOT_ELF
		Elf32_Ehdr ehdr;
#endif
#ifdef PMAX_BOOT_AOUT
		struct exec aout;
#endif
	} hdr;

	if ((fd = open(fname, 0)) < 0) {
		printf("open(%s) failed: %d\n", fname, errno);
		goto err;
	}

	/* read the exec header */
	i = read(fd, (char *)&hdr, sizeof(hdr));
	if (i != sizeof(hdr)) {
		printf("short header\n");
		goto cerr;
	}
#ifdef PMAX_BOOT_ELF
	if (bcmp(hdr.ehdr.e_ident, Elf32_e_ident, Elf32_e_siz) == 0) {
		int first;
		Elf32_Phdr phdr;
		printf("Elfboot: [");
		for (i = 0, first=1; i < hdr.ehdr.e_phnum; ++i) {
			if (lseek(fd, (off_t) hdr.ehdr.e_phoff + i *  sizeof(phdr),  0) < 0)
				goto cerr;
			if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
				goto cerr;
			if (phdr.p_type != Elf_pt_load)
				continue;
			if (lseek(fd, (off_t)phdr.p_offset, 0) < 0)
				goto cerr;
			printf("%d+", phdr.p_filesz);
			if (read(fd, (char *)phdr.p_paddr, phdr.p_filesz) != phdr.p_filesz)
				goto cerr;
			if (!first && phdr.p_filesz != phdr.p_memsz)
				printf("%d]\n", phdr.p_memsz - phdr.p_filesz);
			first = 0;
		}
		return (hdr.ehdr.e_entry);
	} else
#endif
#ifdef PMAX_BOOT_AOUT
	if ((N_GETMAGIC(hdr.aout) == OMAGIC)
		   || (hdr.aout.a_midmag & 0xfff) == OMAGIC) {
		char *buf;

		/* read the code ... */
		printf("Size: %d", hdr.aout.a_text);
		/* In an OMAGIC file, we're already there. */
		if (lseek(fd, (off_t)N_TXTOFF(hdr.aout), 0) < 0) {
			goto cerr;
		}
		n = read(fd, buf = (char *)hdr.aout.a_entry, hdr.aout.a_text);
		/* ... and initialized data */
		printf("+%d", hdr.aout.a_data);
		n += read(fd, buf + hdr.aout.a_text, hdr.aout.a_data);
		i = hdr.aout.a_text + hdr.aout.a_data;

		if (hdr.aout.a_syms) {
			/*
			 * Copy exec header to start of bss.
			 * Load symbols into end + sizeof(int).
			 */
			memcpy(buf += i, (char *)&hdr.aout, sizeof(hdr.aout));
			printf("+[%d", hdr.aout.a_syms + 4);
			n += read(fd, buf += hdr.aout.a_bss + 4, hdr.aout.a_syms + 4);
			i += hdr.aout.a_syms + 4;
			buf += hdr.aout.a_syms;
			/* printf("+%d]", *(long *)buf); */
			n += read(fd, buf + 4, *(long *)buf);
			i += *(long *)buf;
		}
#ifndef UFS_NOCLOSE
		(void) close(fd);
#endif
		if (n < 0) {
			printf("read error %d\n", errno);
			goto err;
		} else if (n != i) {
			printf("read() short %d bytes\n", i - n);
			goto err;
			
		}

		/* kernel will zero out its own bss */
		n = hdr.aout.a_bss;
		printf("+%d\n", n);

		return ((int)hdr.aout.a_entry);
	} else
#endif
	{
		printf("%s: Unknown executable format\n", fname);
	}

cerr:
#ifndef UFS_NOCLOSE
	(void) close(fd);
#endif
err:
	printf("Can't boot '%s'\n", fname);
	return (-1);
}
#endif
