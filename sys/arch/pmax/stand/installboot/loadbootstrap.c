/* $NetBSD: loadbootstrap.c,v 1.4 2000/06/16 23:24:30 matt Exp $ */

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 *      This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>		/* XXX for roundup, howmany */
#include <sys/types.h>
#include <sys/exec_elf.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <dev/dec/dec_boot.h>

#include "installboot.h"

#define MAX_SEGMENTS	10	/* We can load up to 10 segments */

struct seglist {
	Elf32_Addr	addr;
	Elf32_Off	f_offset;
	Elf32_Word	f_size;
};

void
load_bootstrap(const char *bootstrap, char **data,
	u_int32_t *loadaddr, u_int32_t *execaddr, size_t *len)
{
	int fd, i, nsegs;
	Elf32_Addr lowaddr, highaddr;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;
	struct seglist seglist[MAX_SEGMENTS];

	if ((fd = open(bootstrap, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "open %s", bootstrap);

	if ((read(fd, &ehdr, sizeof(ehdr))) != sizeof(ehdr))
		err(EXIT_FAILURE, "read %s", bootstrap);
	if ((memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) ||
	    (ehdr.e_ident[EI_CLASS] != ELFCLASS32))
		errx(EXIT_FAILURE, "no ELF header in %s", bootstrap);
	
	nsegs = highaddr = 0;
	lowaddr = (uint32_t) ULONG_MAX;

	for (i = 0; i < ehdr.e_phnum; i++) {
		if (lseek(fd, (off_t) ehdr.e_phoff + i * sizeof(phdr), 0) < 0)
			err(1, "lseek %s", bootstrap);
		if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
			err(1, "read %s", bootstrap);
		if (phdr.p_type != PT_LOAD)
			continue;

		seglist[nsegs].addr = phdr.p_paddr;
		seglist[nsegs].f_offset = phdr.p_offset;
		seglist[nsegs].f_size = phdr.p_filesz;
		nsegs++;

		if (phdr.p_paddr < lowaddr)
			lowaddr = phdr.p_paddr;
		if (phdr.p_paddr + phdr.p_filesz > highaddr)
			highaddr = phdr.p_paddr + phdr.p_filesz;
	}

	*loadaddr = lowaddr;
	*execaddr = ehdr.e_entry;
	*len = roundup(highaddr - lowaddr, PMAX_BOOT_BLOCK_BLOCKSIZE);
	if ((*data = malloc(*len)) == NULL)
		err(1, "malloc %lu bytes", (unsigned long) *len);

	/* Now load the bootstrap into memory */
	for (i = 0; i < nsegs; i++) {
		if (lseek(fd, (off_t)seglist[i].f_offset, 0) < 0)
			err(1, "lseek %s", bootstrap);
		if (read(fd, *data + seglist[i].addr - lowaddr,
		    seglist[i].f_size) != seglist[i].f_size)
			err(1, "read %s", bootstrap);
	}
}
