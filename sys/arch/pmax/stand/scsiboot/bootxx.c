/*	$NetBSD: bootxx.c,v 1.19 1999/04/12 05:14:51 simonb Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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
#include <sys/exec_elf.h>
#include <stand.h>
#include <machine/dec_prom.h>

#include "byteswap.h"


typedef void (*entrypt) __P((int, char **, int, const void *));

int main __P((int, char **));
entrypt loadfile __P((char *name));

extern int clear_cache __P((char *addr, int len));
extern int bcmp __P((const void *, const void *, size_t));	/* XXX */

/*
 * This gets arguments from the PROM, calls other routines to open
 * and load the secondary boot loader called boot, and then transfers
 * execution to that program.
 *
 * Argv[0] should be something like "rz(0,0,0)netbsd" on a DECstation 3100.
 * Argv[0,1] should be something like "boot 5/rz0/netbsd" on a DECstation 5000.
 * The argument "-a" means netbsd should do an automatic reboot.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	char *cp;
	entrypt entry;

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
	if ((int)entry == -1)
		return (1);

	clear_cache((char *)RELOC, 1024 * 1024);
	if (callv == &callvec)
		entry(argc, argv, 0, 0);
	else
		entry(argc, argv, DEC_PROM_MAGIC, callv);
	return (1);
}

/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
entrypt
loadfile(fname)
	char *fname;
{
	int fd, i;
	char c, *buf, bootfname[64];
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;

	strcpy(bootfname, fname);
	buf = bootfname;
	while ((c = *buf++) != '\0') {
		if (c == ')')
			break;
		if (c != '/')
			continue;
		while ((c = *buf++) != '\0')
			if (c == '/')
				break;
		/*
		 * Make "N/rzY" with no trailing '/' valid by adding
		 * the extra '/' before appending 'boot' to the path.
		 */
		if (c != '/') {
			buf--;
			*buf++ = '/';
			*buf = '\0';
		}
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
	return ((entrypt)ehdr.e_entry);

cerr:
#ifndef LIBSA_NO_FS_CLOSE
	(void) close(fd);
#endif
err:
	printf("Can't load '%s'\n", bootfname);
	return ((entrypt)-1);
}
