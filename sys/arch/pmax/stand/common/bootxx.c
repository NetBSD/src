/*	$NetBSD: bootxx.c,v 1.30.14.1 2009/05/13 17:18:13 jym Exp $	*/

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
#include <machine/dec_prom.h>

typedef void (*entrypt)(int, char **, int, const void *);

int main(int, char **);
entrypt loadfile(char *path, char *name);

extern int clear_cache(char *addr, int len);

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
main(int argc, char **argv)
{
	char *cp;
	entrypt entry;

	/* check for DS5000 boot */
	if (strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}
	cp = *argv;

        printf("\nNetBSD/pmax " NETBSD_VERS " " BOOTXX_FS_NAME " Primary Bootstrap\n");

	entry = loadfile(cp, "/boot.pmax");
	if ((int)entry != -1)
		goto goodload;

	/* Give old /boot a go... */
	entry = loadfile(cp, "/boot");
	if ((int)entry != -1)
		goto goodload;

	/* Booting off an 8.3 filesystem? */
	entry = loadfile(cp, "/boot.pma");
	if ((int)entry != -1)
		goto goodload;

	goto bad;
goodload:

	clear_cache((char *)PRIMARY_LOAD_ADDRESS, 1024 * 1024);
	if (callv == &callvec)
		entry(argc, argv, 0, 0);
	else
		entry(argc, argv, DEC_PROM_MAGIC, callv);
bad:
	/* XXX would calling prom_halt here be cleaner? */
	return (1);
}

/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
entrypt
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
		 * the extra '/' before appending 'bootpmax' to the path.
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
	return ((entrypt)ehdr.e_entry);

cerr:
#ifndef LIBSA_NO_FS_CLOSE
	(void) close(fd);
#endif
err:
	printf("Can't load '%s'\n", bootfname);
	return ((entrypt)-1);
}
