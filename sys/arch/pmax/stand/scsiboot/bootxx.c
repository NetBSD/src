/*	$NetBSD: bootxx.c,v 1.13 1999/03/25 05:16:06 simonb Exp $	*/

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

#include <sys/param.h>
#include <sys/exec.h>
#include <stand.h>
#include <machine/dec_prom.h>

#include "byteswap.h"

int errno;

int loadfile __P((char *name));
extern int clear_cache __P((char *addr, int len));

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

	/* check for DS5000 boot */
	if (strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}
	cp = *argv;

#ifdef notused
	printf(">> NetBSD/pmax Primary Boot\n");
#endif
	entry = loadfile(cp);
	if (entry == -1)
		return (1);

	clear_cache((char *)RELOC, 1024 * 1024);
	if (callv == &callvec)
		((void (*)())entry)(argc, argv, 0, 0);
	else
		((void (*)())entry)(argc, argv, DEC_PROM_MAGIC, callv);
	return (1);
}

/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
int
loadfile(fname)
	register char *fname;
{
	register int fd, i, n;
	char *buf;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;
	char bootfname[64];

	strcpy(bootfname, fname);
	buf = bootfname;
	while ((n = *buf++) != '\0') {
		if (n == ')')
			break;
		if (n != '/')
			continue;
		while ((n = *buf++) != '\0')
			if (n == '/')
				break;
		break;
	}
	strcpy(buf, "boot");
	if ((fd = open(bootfname, 0)) < 0) {
		printf("open %s: %d\n", bootfname, errno);
		goto err;
	}

	/* read the exec header */
	i = read(fd, (char *)&ehdr, sizeof(ehdr));
	if ((i != sizeof(ehdr)) ||
	    (bcmp(ehdr.e_ident, Elf32_e_ident, Elf32_e_siz) != 0)) {
		printf("%s: No ELF header\n", bootfname);
		goto cerr;
	}

	for (i = 0; i < ehdr.e_phnum; i++) {
		if (lseek(fd, (off_t) ehdr.e_phoff + i * sizeof(phdr), 0) < 0)
			goto cerr;
		if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
			goto cerr;
		if (phdr.p_type != Elf_pt_load)
			continue;
		if (lseek(fd, (off_t)phdr.p_offset, 0) < 0)
			goto cerr;
		if (read(fd, (char *)phdr.p_paddr, phdr.p_filesz) != phdr.p_filesz)
			goto cerr;
	}
	return (ehdr.e_entry);

cerr:
#ifndef UFS_NOCLOSE
	(void) close(fd);
#endif
err:
	printf("Can't load '%s'\n", bootfname);
	return (-1);
}
