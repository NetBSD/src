/*	$NetBSD: elf2pef.c,v 1.5.6.1 1999/12/27 18:31:53 wrstuden Exp $	*/

/*-
 * Copyright (C) 1997-1998 Kazuki Sakamoto (sakamoto@netbsd.org)
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * turn ELF image into a PEF image.
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
#include <sys/exec_elf.h>
#include <machine/endian.h>
#include <machine/bswap.h>
#include "pef.h"
#include "magic.h"

#if BYTE_ORDER == LITTLE_ENDIAN
#define	_BE_long(x)	bswap32(x)
#define	_BE_short(x)	bswap16(x)
#else
#define	_BE_long(x) (x)
#define	_BE_short(x) (x)
#endif

/*
 * Align to a 16 byte boundary
 */
unsigned long
align(p)
	unsigned long p;
{
	return ((p + 0x0f) & 0xfffffff0);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	int len, elf_image_len = 0;
	int offset;
	int elf_fd, pef_fd, kern_fd;
	unsigned char *elf_image = NULL, *kern_image = NULL;
	unsigned long textOffset, dataOffset, ldrOffset;
	unsigned long entry_vector[3];
	struct stat kern_stat;
	struct FileHeader fileHdr;
	struct SectionHeader textHdr, dataHdr, ldrHdr;
	struct LoaderHeader lh;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;

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
		if ((pef_fd = creat(argv[2], 0644)) < 0) {
			fprintf(stderr, "Can't open output '%s': %s\n",
				argv[2], strerror(errno));
			exit(2);
		}
		break;

	default:
		fprintf(stderr, "usage: %s <ELF> <PEF> [<kernel>]\n",
			argv[0]);
		exit(1);
	}

	/*
	 * ELF file operation
	 */
	if (read(elf_fd, &hdr, sizeof (hdr)) != sizeof (hdr)) {
		fprintf(stderr, "Can't read input '%s' : %s\n",
			argv[1], strerror(errno));
		exit(3);
	}
	if (bcmp(hdr.e_ident, ELFMAG, SELFMAG) != 0 ||
	    hdr.e_ident[EI_CLASS] != ELFCLASS32) {
		fprintf(stderr, "input '%s' is not ELF32 format\n", argv[1]);
		exit(3);
	}
	if (_BE_short(hdr.e_machine) != EM_PPC) {
		fprintf(stderr, "input '%s' is not PowerPC exec binary\n",
			argv[1]);
		exit(3);
	}
#ifdef DEBUG
	printf("e_entry= 0x%x\n", _BE_long(hdr.e_entry));
	printf("e_phoff= 0x%x\n", _BE_long(hdr.e_phoff));
	printf("e_shoff= 0x%x\n", _BE_long(hdr.e_shoff));
	printf("e_flags= 0x%x\n", _BE_short(hdr.e_flags));
	printf("e_ehsize= 0x%x\n", _BE_short(hdr.e_ehsize));
	printf("e_phentsize= 0x%x\n", _BE_short(hdr.e_phentsize));
	printf("e_phnum= 0x%x\n", _BE_short(hdr.e_phnum));
	printf("e_shentsize= 0x%x\n", _BE_short(hdr.e_shentsize));
	printf("e_shnum= 0x%x\n", _BE_short(hdr.e_shnum));
	printf("e_shstrndx= 0x%x\n", _BE_short(hdr.e_shstrndx));
#endif

	for (i = 0; i < _BE_short(hdr.e_phnum); i++) {
		lseek(elf_fd, _BE_long(hdr.e_phoff) + sizeof (phdr) * i,
			SEEK_SET);
		if (read(elf_fd, &phdr, sizeof (phdr)) != sizeof (phdr)) {
			fprintf(stderr, "Can't read input '%s' phdr : %s\n",
				argv[1], strerror(errno));
			exit(3);
		}

		/* Read in text segment. */
		offset = elf_image_len;
		len = _BE_long(phdr.p_filesz);
		elf_image_len += len;
#ifdef DEBUG
		printf("read text segment %d bytes\n", len);
#endif
		elf_image = (unsigned char *)realloc(elf_image, elf_image_len);
		if (elf_image == NULL) {
			fprintf(stderr, "Can't realloc: %s\n", strerror(errno));
			exit(3);
		}
		lseek(elf_fd, _BE_long(phdr.p_offset), SEEK_SET);
		if (read(elf_fd, (void *)(elf_image + offset), len) != len) {
			fprintf(stderr, "Can't read input '%s' text : %s\n",
				argv[1], strerror(errno));
			exit(3);
		}
	}

	ldrOffset = align(sizeof (fileHdr) + sizeof (textHdr) +
		sizeof (dataHdr) + sizeof (ldrHdr));
	dataOffset = align(ldrOffset + sizeof (lh));
	textOffset = align(dataOffset + sizeof (entry_vector) + kern_len);

	/*
	 * Create the File Header
	 */
	bzero(&fileHdr, sizeof (fileHdr));
	fileHdr.magic = _BE_long(PEF_MAGIC);
	fileHdr.fileTypeID = _BE_long(PEF_FILE);
	fileHdr.archID = _BE_long(PEF_PPC);
	fileHdr.versionNumber = _BE_long(1);
	fileHdr.numSections = _BE_short(3);
	fileHdr.loadableSections = _BE_short(2);
	write(pef_fd, &fileHdr, sizeof (fileHdr));

	/*
	 * Create the Section Header for TEXT
	 */
	bzero(&textHdr, sizeof (textHdr));
	textHdr.sectionName = _BE_long(-1);
	textHdr.sectionAddress = _BE_long(0);
	textHdr.execSize = _BE_long(elf_image_len);
	textHdr.initSize = _BE_long(elf_image_len);
	textHdr.rawSize = _BE_long(elf_image_len);
	textHdr.fileOffset = _BE_long(textOffset);
	textHdr.regionKind = CodeSection;
	textHdr.shareKind = ContextShare;
	textHdr.alignment = 4;  /* 16 byte alignment */
	write(pef_fd, &textHdr, sizeof (textHdr));

	/*
	 * Create the Section Header for DATA
	 */
	bzero(&dataHdr, sizeof (dataHdr));
	dataHdr.sectionName = _BE_long(-1);
	dataHdr.sectionAddress = _BE_long(0);
	dataHdr.execSize = _BE_long(sizeof (entry_vector) + kern_len);
	dataHdr.initSize = _BE_long(sizeof (entry_vector) + kern_len);
	dataHdr.rawSize = _BE_long(sizeof (entry_vector) + kern_len);
	dataHdr.fileOffset = _BE_long(dataOffset);
	dataHdr.regionKind = DataSection;
	dataHdr.shareKind = ContextShare;
	dataHdr.alignment = 4;  /* 16 byte alignment */
	write(pef_fd, &dataHdr, sizeof (dataHdr));

	/*
	 * Create the Section Header for loader stuff
	 */
	bzero(&ldrHdr, sizeof (ldrHdr));
	ldrHdr.sectionName = _BE_long(-1);
	ldrHdr.sectionAddress = _BE_long(0);
	ldrHdr.execSize = _BE_long(sizeof (lh));
	ldrHdr.initSize = _BE_long(sizeof (lh));
	ldrHdr.rawSize = _BE_long(sizeof (lh));
	ldrHdr.fileOffset = _BE_long(ldrOffset);
	ldrHdr.regionKind = LoaderSection;
	ldrHdr.shareKind = GlobalShare;
	ldrHdr.alignment = 4;  /* 16 byte alignment */
	write(pef_fd, &ldrHdr, sizeof (ldrHdr));

	/*
	 * Create the Loader Header
	 */
	bzero(&lh, sizeof (lh));
	lh.entryPointSection = _BE_long(1);	/* Data */
	lh.entryPointOffset = _BE_long(0);
	lh.initPointSection = _BE_long(-1);
	lh.initPointOffset = _BE_long(0);
	lh.termPointSection = _BE_long(-1);
	lh.termPointOffset = _BE_long(0);
	lseek(pef_fd, ldrOffset, 0);
	write(pef_fd, &lh, sizeof (lh));

	/*
	 * Copy the pseudo-DATA
	 */
	bzero(entry_vector, sizeof (entry_vector));
	entry_vector[0] = _BE_long(ENTRY);	/* Magic */
	lseek(pef_fd, dataOffset, 0);
	write(pef_fd, entry_vector, sizeof (entry_vector));

	if (kern_len) {
		int tmp;
		kern_image = (unsigned char *)malloc(kern_stat.st_size);
		if (kern_image == NULL) {
			fprintf(stderr, "Can't malloc: %s\n", strerror(errno));
			exit(3);
		}
		if (read(kern_fd, (void *)kern_image, kern_stat.st_size) !=
		    kern_stat.st_size) {
			fprintf(stderr, "Can't read kernel '%s' : %s\n",
				argv[3], strerror(errno));
			exit(3);
		}
		write(pef_fd, (void *)magic, MAGICSIZE);
		tmp = _BE_long(kern_stat.st_size);
		write(pef_fd, (void *)&tmp, KERNLENSIZE);
		write(pef_fd, (void *)kern_image, kern_stat.st_size);

		free(kern_image);
		close(kern_fd);
	}

	/*
	 * Copy the TEXT data
	 */
	lseek(pef_fd, textOffset, 0);
	write(pef_fd, elf_image, elf_image_len);
	free(elf_image);

	/*
	 * Clean up
	 */
	close(elf_fd);
	close(pef_fd);
	exit(0);
}
