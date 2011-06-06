/*	$NetBSD: bootxx.c,v 1.1.8.2 2011/06/06 09:05:21 jruoho Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

typedef void (entrypt) (int, char **, int, const void *, int, char *);

entrypt main;
entrypt *loadfile (char *path, char *name);

/*
 * This gets arguments from the PROM, calls other routines to open
 * and load the secondary boot loader called boot, and then transfers
 * execution to that program.
 */
void
main(int argc, char **argv, int code, const void *cv, int bim, char *bip)
{
	char *cp;
	entrypt *entry;

	cp = *argv;

    printf("\nNetBSD/emips " NETBSD_VERS " " BOOTXX_FS_NAME " Bootstrap\n");

	entry = loadfile(cp, "netbsd");
	if ((int)entry != -1)
		goto goodload;

	/* Give old /boot a go... */
	entry = loadfile(cp, "/boot");
	if ((int)entry != -1)
		goto goodload;

	goto bad;
goodload:

    entry(argc, argv, code, cv, bim, bip);
bad:
    printf("Boot failed.\n");
}

/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
entrypt *
loadfile(char *path, char *name)
{
	int fd, i;
	char c, *buf, bootfname[64];
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;

	strcpy(bootfname, path);
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
		 * the extra '/' before appending 'bootemips' to the path.
		 */
		if (c != '/') {
			buf--;
			*buf++ = '/';
			*buf = '\0';
		}
		break;
	}
	strcpy(buf, name);
	if ((fd = open(bootfname, 0)) < 0) {
		printf("open %s: %d\n", bootfname, errno);
		goto err;
	}

	/* read the exec header */
	i = read(fd, (char *)&ehdr, sizeof(ehdr));
	if ((i != sizeof(ehdr)) ||
	    (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) ||
	    (ehdr.e_ident[EI_CLASS] != ELFCLASS32)) {
		printf("%s: No ELF header\n", bootfname);
		goto cerr;
	}

	for (i = 0; i < ehdr.e_phnum; i++) {
		if (lseek(fd, (off_t) ehdr.e_phoff + i * sizeof(phdr), 0) < 0)
			goto cerr;
		if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
			goto cerr;
		if (phdr.p_type != PT_LOAD)
			continue;
		if (lseek(fd, (off_t)phdr.p_offset, 0) < 0)
			goto cerr;
		if (read(fd, (char *)phdr.p_paddr, phdr.p_filesz) != phdr.p_filesz)
			goto cerr;
	}
	return ((entrypt*)ehdr.e_entry);

cerr:
#ifndef LIBSA_NO_FS_CLOSE
	(void) close(fd);
#endif
err:
	printf("Can't load '%s'\n", bootfname);
	return ((entrypt*)-1);
}
