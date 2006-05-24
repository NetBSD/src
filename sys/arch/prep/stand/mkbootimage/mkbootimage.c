/*	$NetBSD: mkbootimage.c,v 1.12.12.2 2006/05/24 15:48:21 tron Exp $	*/

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#include "../../sys/sys/bootblock.h"
#else
#include <sys/bootblock.h>
#endif

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
#include "magic.h"

/* Globals */

int saloneflag = 0;
Elf32_External_Ehdr hdr, khdr;
struct stat elf_stat;
unsigned char mbr[512];

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
static void check_mbr(int, int, char *);
int main(int, char **);

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-ls] [-b bootfile] [-k kernel] [-r rawdev]"
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

static void
check_mbr(int prep_fd, int lfloppyflag, char *rawdev)
{
	int raw_fd;
	unsigned long entry, length;
	struct mbr_partition *mbrp;
	struct stat raw_stat;

	/* If we are building a standalone image, do not write an MBR, just
	 * set entry point and boot image size skipping over elf header
	 */
	if (saloneflag) {
		entry  = sa_htole32(0x400);
		length = sa_htole32(elf_stat.st_size - sizeof(hdr) + 0x400);
		lseek(prep_fd, sizeof(mbr), SEEK_SET);
		write(prep_fd, &entry, sizeof(entry));
		write(prep_fd, &length, sizeof(length));
		return;
	}

	/*
	 * if we have a raw device, we need to check to see if it already
	 * has a partition table, and if so, read it in and check for
	 * suitability.
	 */
	if (rawdev != NULL) {
		raw_fd = open(rawdev, O_RDONLY, 0);
		if (raw_fd == -1)
			errx(3, "couldn't open raw device %s: %s", rawdev,
			    strerror(errno));

		fstat(raw_fd, &raw_stat);
		if (!(raw_stat.st_mode & S_IFCHR))
			errx(3, "%s is not a raw device", rawdev);

		if (read(raw_fd, mbr, 512) != 512)
			errx(3, "MBR Read Failed: %s", strerror(errno));

		mbrp = (struct mbr_partition *)&mbr[MBR_PART_OFFSET];
		if (mbrp->mbrp_type != MBR_PTYPE_PREP)
			errx(3, "First partition is not of type 0x%x.",
			    MBR_PTYPE_PREP);
		if (mbrp->mbrp_start != 0)
			errx(3, "Use of the raw device is intended for"
			    " upgrading of legacy installations.  Your"
			    " install does not have a PReP boot partition"
			    " starting at sector 0.  Use the -s option"
			    " to build an image instead.");

		/* if we got this far, we are fine, write back the partition
		 * and write the entry points and get outta here */
	/* Set entry point and boot image size skipping over elf header */
		lseek(prep_fd, 0, SEEK_SET);
		entry  = sa_htole32(0x400);
		length = sa_htole32(elf_stat.st_size - sizeof(hdr) + 0x400);
		write(prep_fd, mbr, sizeof(mbr));
		write(prep_fd, &entry, sizeof(entry));
		write(prep_fd, &length, sizeof(length));
		close(raw_fd);
		return;
	}

	/* if we get to here, we want to build a standard floppy or netboot
	 * image to file, so just build it */

	memset(mbr, 0, sizeof(mbr));
	mbrp = (struct mbr_partition *)&mbr[MBR_PART_OFFSET];
 
	/* Set entry point and boot image size skipping over elf header */
	entry  = sa_htole32(0x400);
	length = sa_htole32(elf_stat.st_size - sizeof(hdr) + 0x400);

	/*
	 * Set magic number for msdos partition
	 */
	*(unsigned short *)&mbr[MBR_MAGIC_OFFSET] = sa_htole16(MBR_MAGIC);
  
	/*
	 * Build a "PReP" partition table entry in the boot record
	 *  - "PReP" may only look at the system_indicator
	 */
	mbrp->mbrp_flag = MBR_PFLAG_ACTIVE;
	mbrp->mbrp_type  = MBR_PTYPE_PREP;

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
	if (lfloppyflag)
		mbrp->mbrp_esect = 36;  /* 2.88MB floppy	     */
	else
		mbrp->mbrp_esect = 18;	/* assumes 18 sectors/track  */
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
	mbrp->mbrp_start = sa_htole32(0);
	mbrp->mbrp_size  = sa_htole32(2 * 18 * 80 - 1);

	write(prep_fd, mbr, sizeof(mbr));
	write(prep_fd, &entry, sizeof(entry));
	write(prep_fd, &length, sizeof(length));  
}

int
main(int argc, char **argv)
{
	unsigned char *elf_img = NULL, *kern_img = NULL;
	int i, ch, tmp, kgzlen, err, lfloppyflag=0;
	int elf_fd, prep_fd, kern_fd, elf_img_len = 0;
	off_t lenpos, kstart, kend;
	unsigned long length;
	long flength;
	gzFile gzf;
	struct stat kern_stat;
	char *kernel = NULL, *boot = NULL, *rawdev = NULL;
	Elf32_External_Phdr phdr;

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
		case 'r':
			rawdev = optarg;
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

	check_mbr(prep_fd, lfloppyflag, rawdev);

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
	tmp = sa_htobe32(0);
	write(prep_fd, (void *)&tmp, KERNLENSIZE);

	/* write in the compressed kernel */
	kstart = lseek(prep_fd, 0, SEEK_CUR);
	kgzlen = gzwrite(gzf, kern_img, kern_stat.st_size);
	gzclose(gzf);
	kend = lseek(prep_fd, 0, SEEK_CUR);

	/* jump back to the length position now that we know the length */
	lseek(prep_fd, lenpos, SEEK_SET);
	kgzlen = kend - kstart;
	tmp = sa_htobe32(kgzlen);
	write(prep_fd, (void *)&tmp, KERNLENSIZE);

	length = sa_htole32(0x400 + elf_img_len + 8 + kgzlen);
	lseek(prep_fd, sizeof(mbr) + 4, SEEK_SET);
	write(prep_fd, &length, sizeof(length));

	flength = 0x400 + elf_img_len + 8 + kgzlen;
	if (lfloppyflag)
		flength -= (5760 * 512);
	else
		flength -= (2880 * 512);
	if (flength > 0 && !saloneflag)
		fprintf(stderr, "%s: Image %s is %d bytes larger than single"
		    " floppy. Can only be used for netboot.\n", getprogname(),
		    argv[0], flength);

	free(kern_img);
	close(kern_fd);
	close(prep_fd);
	close(elf_fd);

	return 0;
}
