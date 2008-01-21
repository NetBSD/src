/*	$NetBSD: mkbootimage.c,v 1.1.8.2 2008/01/21 09:39:07 yamt Exp $	*/

/*-
 * Copyright (C) 2006 Tim Rightnour
 * Copyright (C) 1999, 2000 NONAKA Kimihiro (nonaka@NetBSD.org)
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
#include <zlib.h>
#include <err.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

/* BFD ELF headers */
#include <elf/common.h>
#include <elf/external.h>

#include "byteorder.h"
#include "bootrec.h"
#include "magic.h"

/* Globals */

int saloneflag = 0;
Elf32_External_Ehdr hdr, khdr;
struct stat elf_stat;
/* the boot and config records */
boot_record_t bootrec;
config_record_t confrec;

/*
 * Macros to get values from multi-byte ELF header fields.  These assume
 * a big-endian image.
 */
#define	ELFGET16(x)	(((x)[0] << 8) | (x)[1])

#define	ELFGET32(x)	(((x)[0] << 24) | ((x)[1] << 16) |		\
			 ((x)[2] <<  8) |  (x)[3])

static void usage(void);
static int open_file(const char *, char *, Elf32_External_Ehdr *,
    struct stat *);
static void build_records(int);
int main(int, char **);

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-s] [-b bootfile] [-k kernel]"
	    " bootimage\n", getprogname());
	exit(1);
}

/* verify the file is ELF and ppc, and open it up */
static int
open_file(const char *ftype, char *file, Elf32_External_Ehdr *hdr,
	struct stat *f_stat)
{
	int fd;

	if ((fd = open(file, 0)) < 0)
		errx(2, "Can't open %s '%s': %s", ftype, file, strerror(errno));
	fstat(fd, f_stat);

	if (read(fd, hdr, sizeof(Elf32_External_Ehdr)) !=
	    sizeof(Elf32_External_Ehdr))
		errx(3, "Can't read input '%s': %s", file, strerror(errno));

	if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    hdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    hdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    hdr->e_ident[EI_MAG3] != ELFMAG3 ||
	    hdr->e_ident[EI_CLASS] != ELFCLASS32)
		errx(3, "input '%s' is not ELF32 format", file);

	if (hdr->e_ident[EI_DATA] != ELFDATA2MSB)
		errx(3, "input '%s' is not big-endian", file);

	if (ELFGET16(hdr->e_machine) != EM_PPC)
		errx(3, "input '%s' is not PowerPC exec binary", file);

	return(fd);
}

/* Fill in the needed information on the boot and config records.  Most of
 * this is just AIX garbage that we don't really need to boot.
 */
static void
build_records(int img_len)
{
	int bcl;

	/* zero out all the fields, so we only have to set the ones
	 * we care about, which are rather few.
	 */
	memset(&bootrec, 0, sizeof(boot_record_t));
	memset(&confrec, 0, sizeof(config_record_t));

	bootrec.ipl_record = IPLRECID;
	bcl = img_len/512;
	if (img_len%512 != 0)
		bcl++;
	bootrec.bootcode_len = bcl;
	bootrec.bootcode_off = 0; /* XXX */
	bootrec.bootpart_start = 2; /* skip bootrec and confrec */
	bootrec.bootprg_start = 2;
	bootrec.bootpart_len = bcl;
	bootrec.boot_load_addr = 0x800000; /* XXX? */
	bootrec.boot_frag = 1;
	bootrec.boot_emul = 0x02; /* ?? */
	/* service mode is a repeat of normal mode */
	bootrec.servcode_len = bootrec.bootcode_len;
	bootrec.servcode_off = bootrec.bootcode_off;
	bootrec.servpart_start = bootrec.bootpart_start;
	bootrec.servprg_start = bootrec.bootprg_start;
	bootrec.servpart_len = bootrec.bootpart_len;
	bootrec.serv_load_addr = bootrec.boot_load_addr;
	bootrec.serv_frag = bootrec.boot_frag;
	bootrec.serv_emul = bootrec.boot_emul;

	/* now the config record */
	confrec.conf_rec = CONFRECID;
	confrec.sector_size = 0x02; /* 512 bytes */
	confrec.last_cyl = 0x4f; /* 79 cyl, emulates floppy */
}
	
int
main(int argc, char **argv)
{
	unsigned char *elf_img = NULL, *kern_img = NULL;
	int i, ch, tmp, kgzlen, err, lfloppyflag=0;
	int elf_fd, prep_fd, kern_fd, elf_img_len = 0, elf_pad = 0;
	off_t lenpos, kstart, kend;
	unsigned long length;
	long flength;
	gzFile gzf;
	struct stat kern_stat;
	char *kernel = NULL, *boot = NULL;
	Elf32_External_Phdr phdr;
	uint32_t swapped[128];

	setprogname(argv[0]);
	kern_len = 0;

	while ((ch = getopt(argc, argv, "b:k:lr:s")) != -1)
		switch (ch) {
		case 'b':
			boot = optarg;
			break;
		case 'k':
			kernel = optarg;
			break;
		case 'l':
			lfloppyflag = 1;
			break;
		case 's':
			saloneflag = 1;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	if (kernel == NULL)
		kernel = "/netbsd";

	if (boot == NULL)
		boot = "/usr/mdec/boot";

	elf_fd = open_file("bootloader", boot, &hdr, &elf_stat);
	kern_fd = open_file("kernel", kernel, &khdr, &kern_stat);
	kern_len = kern_stat.st_size + MAGICSIZE + KERNLENSIZE;

	for (i = 0; i < ELFGET16(hdr.e_phnum); i++) {
		lseek(elf_fd, ELFGET32(hdr.e_phoff) + sizeof(phdr) * i,
			SEEK_SET);
		if (read(elf_fd, &phdr, sizeof(phdr)) != sizeof(phdr))
			errx(3, "Can't read input '%s' phdr : %s", boot,
			    strerror(errno));

		if ((ELFGET32(phdr.p_type) != PT_LOAD) ||
		    !(ELFGET32(phdr.p_flags) & PF_X))
			continue;

		fstat(elf_fd, &elf_stat);
		elf_img_len = elf_stat.st_size - ELFGET32(phdr.p_offset);
		elf_pad = ELFGET32(phdr.p_memsz) - ELFGET32(phdr.p_filesz);
		printf("Padding %d\n", elf_pad);
		lseek(elf_fd, ELFGET32(phdr.p_offset), SEEK_SET);

		break;
	}
	if ((prep_fd = open(argv[0], O_RDWR|O_TRUNC, 0)) < 0) {
		/* we couldn't open it, it must be new */
		prep_fd = creat(argv[0], 0644);
		if (prep_fd < 0)
			errx(2, "Can't open output '%s': %s", argv[0],
			    strerror(errno));
	}

	/* Set file pos. to 2nd sector where image will be written */
	lseek(prep_fd, 0x400, SEEK_SET);

	/* Copy boot image */
	elf_img = (unsigned char *)malloc(elf_img_len);
	if (!elf_img)
		errx(3, "Can't malloc: %s", strerror(errno));
	if (read(elf_fd, elf_img, elf_img_len) != elf_img_len)
		errx(3, "Can't read file '%s' : %s", boot, strerror(errno));

	write(prep_fd, elf_img, elf_img_len);
	free(elf_img);

	/* now dump in the padding space for the BSS */
	elf_pad += 100; /* just a little extra for good luck */
	lseek(prep_fd, elf_pad, SEEK_CUR);

	/* Copy kernel */
	kern_img = (unsigned char *)malloc(kern_stat.st_size);

	if (kern_img == NULL)
		errx(3, "Can't malloc: %s", strerror(errno));

	/* we need to jump back after having read the headers */
	lseek(kern_fd, 0, SEEK_SET);
	if (read(kern_fd, (void *)kern_img, kern_stat.st_size) !=
	    kern_stat.st_size)
		errx(3, "Can't read kernel '%s' : %s", kernel, strerror(errno));

	gzf = gzdopen(dup(prep_fd), "a");
	if (gzf == NULL)
		errx(3, "Can't init compression: %s", strerror(errno));
	if (gzsetparams(gzf, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY) != Z_OK)
		errx(3, "%s", gzerror(gzf, &err));

	/* write a magic number and size before the kernel */
	write(prep_fd, (void *)magic, MAGICSIZE);
	lenpos = lseek(prep_fd, 0, SEEK_CUR);
	printf("wrote magic at pos 0x%x\n", lenpos);
	tmp = sa_htobe32(0);
	write(prep_fd, (void *)&tmp, KERNLENSIZE);

	/* write in the compressed kernel */
	kstart = lseek(prep_fd, 0, SEEK_CUR);
	printf("kernel start at pos 0x%x\n", kstart);
	kgzlen = gzwrite(gzf, kern_img, kern_stat.st_size);
	gzclose(gzf);
	kend = lseek(prep_fd, 0, SEEK_CUR);
	printf("kernel end at pos 0x%x\n", kend);

	/* jump back to the length position now that we know the length */
	lseek(prep_fd, lenpos, SEEK_SET);
	kgzlen = kend - kstart;
	tmp = sa_htobe32(kgzlen);
	printf("kernel len = 0x%x tmp = 0x%x\n", kgzlen, tmp);
	write(prep_fd, (void *)&tmp, KERNLENSIZE);

#if 0
	lseek(prep_fd, sizeof(boot_record_t) + sizeof(config_record_t)),
	    SEEK_SET);
	/* set entry and length */
	length = sa_htole32(0x400);
	write(prep_fd, &length, sizeof(length));
	length = sa_htole32(0x400 + elf_img_len + 8 + kgzlen);
	write(prep_fd, &length, sizeof(length));
#endif

	/* generate the header now that we know the kernel length */
	printf("building records\n");
	build_records(elf_img_len + 8 + kgzlen);
	lseek(prep_fd, 0, SEEK_SET);
	/* ROM wants it byteswapped in 32bit chunks */
	printf("writing records\n");
	memcpy(swapped, &bootrec, sizeof(boot_record_t));
	for (i=0; i < 128; i++)
		swapped[i] = htonl(swapped[i]);
	write(prep_fd, swapped, sizeof(boot_record_t));
	memcpy(swapped, &confrec, sizeof(config_record_t));
	for (i=0; i < 128; i++)
		swapped[i] = htonl(swapped[i]);
	write(prep_fd, swapped, sizeof(config_record_t));

	free(kern_img);
	close(kern_fd);
	close(prep_fd);
	close(elf_fd);

	return 0;
}
