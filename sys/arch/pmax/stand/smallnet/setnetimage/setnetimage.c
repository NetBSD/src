/*	$NetBSD: setnetimage.c,v 1.1 1999/05/13 08:38:05 simonb Exp $	*/

/*-
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

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/exec_elf.h>

#include <err.h>
#include <fcntl.h>
#include <nlist.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "extern.h"

#define MAX_SEGMENTS		10	/* We can load up to 10 segments */

struct nlist nl[] = {
#define	X_KERNEL_ENTRY		0
	{ "_kernel_entry" },
#define	X_KERNEL_IMAGE		1
	{ "_kernel_image" },
#define	X_KERNEL_LOADADDR	2
	{ "_kernel_loadaddr" },
#define	X_KERNEL_SIZE		3
	{ "_kernel_size" },
#define	X_MAXKERNEL_SIZE	4
	{ "_maxkernel_size" },
	{ NULL }
};
#define X_NSYMS			((sizeof(nl) / sizeof(struct nlist)) - 1)

struct seglist {
	Elf32_Addr addr;
	Elf32_Off f_offset;
	Elf32_Word f_size;
};

#define NLADDR(x)	(mappedbfile + offsets[(x)])
#define NLVAR(x)	(*(u_long *)(NLADDR(x)))

extern const char *__progname;

int main __P((int, char **));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ifd, ofd, i, nsegs;
	size_t offsets[X_NSYMS];
	const char *kernel, *bootfile;
	char *mappedbfile;
	char *uncomp_kernel, *comp_kernel;
	Elf32_Addr lowaddr, highaddr;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;
	uLongf destlen;
	struct stat osb;
	struct seglist seglist[MAX_SEGMENTS];

	if (argc != 3) {
		fprintf(stderr, "usage: %s kernel bootfile\n", __progname);
		exit(1);
	}

	kernel = argv[1];
	bootfile = argv[2];

	if ((ifd = open(kernel, O_RDONLY)) < 0)
		err(1, "%s", kernel);

	if ((ofd = open(bootfile, O_RDWR)) < 0)
		err(1, "%s", bootfile);

	if (nlist(bootfile, nl) != 0)
		errx(1, "Could not find symbols in %s", bootfile);

	if (fstat(ofd, &osb) == -1)
		err(1, "fstat %s", bootfile);
	if (osb.st_size > SIZE_T_MAX)
		errx(1, "%s too big to map", bootfile);

	if ((mappedbfile = mmap(NULL, osb.st_size, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_SHARED, ofd, 0)) == (caddr_t)-1)
		err(1, "mmap %s", bootfile);
	printf("mapped %s\n", bootfile);

	if (check_elf32(mappedbfile, osb.st_size) != 0)
		errx(1, "No ELF header in %s", bootfile);

	for (i = 0; i < X_NSYMS; i++) {
		if (findoff_elf32(mappedbfile, osb.st_size, nl[i].n_value, &offsets[i]) != 0)
			errx(1, "Couldn't find offset for %s in %s\n", nl[i].n_name, bootfile);
#ifdef DEBUG
		printf("%s is at offset %#x in %s\n", nl[i].n_name, offsets[i], bootfile);
#endif
	}

	/* read the exec header */
	i = read(ifd, (char *)&ehdr, sizeof(ehdr));
	if ((i != sizeof(ehdr)) ||
	    (memcmp(ehdr.e_ident, Elf32_e_ident, Elf32_e_siz) != 0)) {
		errx(1, "No ELF header in %s", kernel);
	}

	nsegs = highaddr = 0;
	lowaddr = UINT_MAX;
	for (i = 0; i < ehdr.e_phnum; i++) {
		if (lseek(ifd, (off_t) ehdr.e_phoff + i * sizeof(phdr), 0) < 0)
			err(1, "%s", kernel);
		if (read(ifd, &phdr, sizeof(phdr)) != sizeof(phdr))
			err(1, "%s", kernel);
		if (phdr.p_type != Elf_pt_load)
			continue;

		printf("load section %d at addr 0x%08x, file offset %8d, length %8d\n",
		    i, phdr.p_paddr, phdr.p_offset, phdr.p_filesz);

		seglist[nsegs].addr = phdr.p_paddr;
		seglist[nsegs].f_offset = phdr.p_offset;
		seglist[nsegs].f_size = phdr.p_filesz;
		nsegs++;

		if (phdr.p_paddr < lowaddr)
			lowaddr = phdr.p_paddr;
		if (phdr.p_paddr + phdr.p_filesz > highaddr)
			highaddr = phdr.p_paddr + phdr.p_filesz;

	}

#ifdef DEBUG
	printf("lowaddr =  0x%08x, highaddr = 0x%08x\n", lowaddr, highaddr);
#endif
	printf("load address: 0x%08x\n", lowaddr);
	printf("entry point:  0x%08x\n", ehdr.e_entry);

	destlen = highaddr - lowaddr;

	uncomp_kernel = (char *)malloc(destlen);
	comp_kernel = (char *)malloc(destlen);		/* Worst case... */
	for (i = 0; i < nsegs; i++) {
#ifdef DEBUG
		printf("lseek(ifd, %d, 0)\n", seglist[i].f_offset);
#endif
		if (lseek(ifd, (off_t)seglist[i].f_offset, 0) < 0)
			err(1, "%s", kernel);
#ifdef DEBUG
		printf("read(ifd, %p, %d)\n", \
		    uncomp_kernel + seglist[i].addr - lowaddr,
		    seglist[i].f_size);
#endif
		if (read(ifd, uncomp_kernel + seglist[i].addr - lowaddr,
		    seglist[i].f_size) != seglist[i].f_size)
			err(1, "%s", kernel);
	}
	close(ifd);

	printf("Compressing %d bytes...", highaddr - lowaddr); fflush(stdout);
	i = compress2(comp_kernel, &destlen, uncomp_kernel, \
	    highaddr - lowaddr, Z_BEST_COMPRESSION);
	if (i != Z_OK) {
		printf("\n");
		errx(1, "%s compression error %d\n", kernel, i);
	}
	printf("done.\n"); fflush(stdout);

	printf("max kernelsize = %ld\n", NLVAR(X_MAXKERNEL_SIZE));
	printf("compressed size = %ld\n", destlen);
	if (destlen > NLVAR(X_MAXKERNEL_SIZE))
		errx(1, "kernel %s is too big, "
		    "increase KERNELSIZE to at least %ld\n",
		    kernel, destlen);

	NLVAR(X_KERNEL_SIZE) = destlen;
	NLVAR(X_KERNEL_LOADADDR) = lowaddr;
	NLVAR(X_KERNEL_ENTRY) = ehdr.e_entry;
	memcpy(NLADDR(X_KERNEL_IMAGE), comp_kernel, destlen);
	munmap(mappedbfile, osb.st_size);
	printf("unmapped %s\n", bootfile);
	close(ofd);

	exit(0);
}
