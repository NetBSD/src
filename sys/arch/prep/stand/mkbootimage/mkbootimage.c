/*	$NetBSD: mkbootimage.c,v 1.2 2002/03/02 07:05:30 matt Exp $	*/

/*-
 * Copyright (C) 1999, 2000 NONAKA Kimihiro (nonaka@netbsd.org)
 * Copyright (C) 1996, 1997, 1998, 1999 Cort Dougan (cort@fsmlasb.com)
 * Copyright (C) 1996, 1997, 1998, 1999 Paul Mackeras (paulus@linuxcare.com)
 * All rights reserved.
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
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/disklabel_mbr.h>
#include <sys/exec_elf.h>
#include <machine/bswap.h>
#include <machine/endian.h>

#include "magic.h"

int
main(argc, argv)
	int argc;
	char **argv;
{
	int i;
	int elf_fd, prep_fd, kern_fd;
	int elf_img_len = 0, kern_len = 0;
	struct stat elf_stat, kern_stat;
	unsigned char *elf_img = NULL, *kern_img = NULL;
	unsigned char mbr[512];
	unsigned long entry;
	unsigned long length;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	struct mbr_partition *mbrp = (struct mbr_partition *)&mbr[MBR_PARTOFF];

	switch (argc) { 
	case 4:
		if ((kern_fd = open(argv[3], 0)) < 0) {
			fprintf(stderr, "Can't open kernel '%s': %s\n",
				argv[3], strerror(errno));
			exit(2);
		}
		fstat(kern_fd, &kern_stat);
		kern_len = kern_stat.st_size + MAGICSIZE + KERNLENSIZE;
		/* FALL THROUGH */
	case 3:
		if ((elf_fd = open(argv[1], 0)) < 0) {
			fprintf(stderr, "Can't open input '%s': %s\n",
				argv[1], strerror(errno));
			exit(2);
		}
		if ((prep_fd = creat(argv[2], 0644)) < 0) {
			fprintf(stderr, "Can't open output '%s': %s\n",
				argv[2], strerror(errno));
			exit(2);
		}

		break;

	default:
		fprintf(stderr, "usage: %s "
		    "<boot-prog> <boot-image> [<gzip'd kernel>]\n", argv[0]);
		exit(1);
		break;
	}

	/*
	 * ELF file operation
	 */
	if (read(elf_fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		fprintf(stderr, "Can't read input '%s': %s\n",
			argv[1], strerror(errno));
		exit(3);
	}
	if (memcmp(hdr.e_ident, ELFMAG, SELFMAG) != 0 ||
	    hdr.e_ident[EI_CLASS] != ELFCLASS32) {
		fprintf(stderr, "input '%s' is not ELF32 format\n", argv[1]);
		exit(3);
	}
	if (be16toh(hdr.e_machine) != EM_PPC) {
		fprintf(stderr, "input '%s' is not PowerPC exec binary\n",
			argv[1]);
		exit(3);
	}

	for (i = 0; i < be16toh(hdr.e_phnum); i++) {
		lseek(elf_fd, be32toh(hdr.e_phoff) + sizeof(phdr) * i,
			SEEK_SET);
		if (read(elf_fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
			fprintf(stderr, "Can't read input '%s' phdr : %s\n",
				argv[1], strerror(errno));
			exit(3);
                }

		if ((be32toh(phdr.p_type) != PT_LOAD) ||
		    !(be32toh(phdr.p_flags) & PF_X))
			continue;

		fstat(elf_fd, &elf_stat);
		elf_img_len = elf_stat.st_size - be32toh(phdr.p_offset);
		lseek(elf_fd, be32toh(phdr.p_offset), SEEK_SET);

		break;
	}

	memset(mbr, 0, sizeof(mbr));
 
	/* Set entry point and boot image size skipping over elf header */
	entry  = htole32(0x400);
	length = htole32(elf_stat.st_size - sizeof(hdr) + 0x400);

	/*
	 * Set magic number for msdos partition
	 */
	*(unsigned short *)&mbr[MBR_MAGICOFF] = htole16(MBR_MAGIC);
  
	/*
	 * Build a "PReP" partition table entry in the boot record
	 *  - "PReP" may only look at the system_indicator
	 */
	mbrp->mbrp_flag = MBR_FLAGS_ACTIVE;
	mbrp->mbrp_typ  = MBR_PTYPE_PREP;

	/*
	 * The first block of the diskette is used by this "boot record" which
	 * actually contains the partition table. (The first block of the
	 * partition contains the boot image, but I digress...)  We'll set up
	 * one partition on the diskette and it shall contain the rest of the
	 * diskette.
	 */
	mbrp->mbrp_shd   = 0;	/* zero-based			     */
	mbrp->mbrp_ssect = 2;	/* one-based			     */
	mbrp->mbrp_scyl  = 0;	/* zero-based			     */
	mbrp->mbrp_ehd   = 1;	/* assumes two heads		     */
	mbrp->mbrp_esect = 18;	/* assumes 18 sectors/track	     */
	mbrp->mbrp_ecyl  = 79;	/* assumes 80 cylinders/diskette     */

	/*
	 * The "PReP" software ignores the above fields and just looks at
	 * the next two.
	 *   - size of the diskette is (assumed to be)
	 *     (2 tracks/cylinder)(18 sectors/tracks)(80 cylinders/diskette)
	 *   - unlike the above sector numbers,
	 *     the beginning sector is zero-based!
	 */

	/* This has to be 0 on the PowerStack? */   
	mbrp->mbrp_start = htole32(0);
	mbrp->mbrp_size  = htole32(2 * 18 * 80 - 1);

	write(prep_fd, mbr, sizeof(mbr));
	write(prep_fd, &entry, sizeof(entry));
	write(prep_fd, &length, sizeof(length));  

	/* Set file position to 2nd sector where image will be written */
	lseek(prep_fd, 0x400, SEEK_SET);

	/* Copy boot image */
	elf_img = (unsigned char *)malloc(elf_img_len);
	if (!elf_img) {
		fprintf(stderr, "Can't malloc: %s\n", strerror(errno));
		exit(3);
	}
	if (read(elf_fd, elf_img, elf_img_len) != elf_img_len) {
	    	fprintf(stderr, "Can't read file '%s' : %s\n",
			argv[1], strerror(errno));
		free(elf_img);
		exit(3);
	}
	write(prep_fd, elf_img, elf_img_len);
	free(elf_img);

	/* Copy kernl */
	if (kern_len) {
		int tmp;
		kern_img = (unsigned char *)malloc(kern_stat.st_size);
		if (kern_img == NULL) {
			fprintf(stderr, "Can't malloc: %s\n", strerror(errno));
			exit(3);
		}
		if (read(kern_fd, (void *)kern_img, kern_stat.st_size) !=
		    kern_stat.st_size) {
			fprintf(stderr, "Can't read kernel '%s' : %s\n",
				argv[3], strerror(errno));
			exit(3);
		}
		write(prep_fd, (void *)magic, MAGICSIZE);
		tmp = htobe32(kern_stat.st_size);
		write(prep_fd, (void *)&tmp, KERNLENSIZE);
		write(prep_fd, (void *)kern_img, kern_stat.st_size);

		length = htole32(0x400 + elf_img_len + 8 + kern_stat.st_size);
		lseek(prep_fd, sizeof(mbr) + 4, SEEK_SET);
		write(prep_fd, &length, sizeof(length));  

		free(kern_img);
		close(kern_fd);
	}

	close(prep_fd);
	close(elf_fd);

	return 0;
}
