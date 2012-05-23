/*	$NetBSD: mkbootimage.c,v 1.14.4.1 2012/05/23 10:07:47 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour and NONAKA Kimihiro
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
#include <sys/signal.h>

#undef USE_SYSCTL

#if defined(__NetBSD__) && !defined(HAVE_NBTOOL_CONFIG_H)
#define USE_SYSCTL 1
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#endif

/* BFD ELF headers */
#include <elf/common.h>
#include <elf/external.h>

#include "bebox_bootrec.h"
#include "byteorder.h"
#include "magic.h"
#include "pef.h"
#include "rs6000_bootrec.h"

/* Globals */

int inkernflag = 1;
int saloneflag = 0;
int verboseflag = 0;
int lfloppyflag = 0;
Elf32_External_Ehdr hdr, khdr;
struct stat elf_stat;
unsigned char mbr[512];

/* the boot and config records for rs6000 */
rs6000_boot_record_t bootrec;
rs6000_config_record_t confrec;

/* supported platforms */
char *sup_plats[] = {
	"bebox",
	"prep",
	"rs6000",
	NULL,
};

/*
 * Macros to get values from multi-byte ELF header fields.  These assume
 * a big-endian image.
 */
#define	ELFGET16(x)	(((x)[0] << 8) | (x)[1])

#define	ELFGET32(x)	(((x)[0] << 24) | ((x)[1] << 16) |		\
			 ((x)[2] <<  8) |  (x)[3])

#define ULALIGN(x)	((x + 0x0f) & 0xfffffff0)

static void usage(int);
static int open_file(const char *, char *, Elf32_External_Ehdr *,
    struct stat *);
static void check_mbr(int, char *);
static int prep_build_image(char *, char *, char *, char *);
static void rs6000_build_records(int);
static int rs6000_build_image(char *, char *, char *, char *);
int main(int, char **);


static void
usage(int extended)
{
	int i;

	if (extended) {
		fprintf(stderr, "You are not running this program on"
		    " the target machine.  You must supply the\n"
		    "machine architecture with the -m flag\n");
		fprintf(stderr, "Supported architectures: ");
		for (i=0; sup_plats[i] != NULL; i++)
			fprintf(stderr, " %s", sup_plats[i]);
		fprintf(stderr, "\n\n");
	}
#ifdef USE_SYSCTL
	fprintf(stderr, "usage: %s [-Ilsv] [-m machine] [-b bootfile] "
	    "[-k kernel] [-r rawdev] bootimage\n", getprogname());
#else
	fprintf(stderr, "usage: %s [-Ilsv] -m machine [-b bootfile] "
	    "[-k kernel] [-r rawdev] bootimage\n", getprogname());
#endif
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
prep_check_mbr(int prep_fd, char *rawdev)
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
		if (!S_ISCHR(raw_stat.st_mode))
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
	mbrp->mbrp_shd   = 0;	/* zero-based */
	mbrp->mbrp_ssect = 2;	/* one-based */
	mbrp->mbrp_scyl  = 0;	/* zero-based */
	mbrp->mbrp_ehd   = 1;	/* assumes two heads */
	if (lfloppyflag)
		mbrp->mbrp_esect = 36;  /* 2.88MB floppy */
	else
		mbrp->mbrp_esect = 18;	/* assumes 18 sectors/track */
	mbrp->mbrp_ecyl  = 79;	/* assumes 80 cylinders/diskette */

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

static int
prep_build_image(char *kernel, char *boot, char *rawdev, char *outname)
{
	unsigned char *elf_img = NULL, *kern_img = NULL;
	int i, ch, tmp, kgzlen, err;
	int elf_fd, prep_fd, kern_fd, elf_img_len = 0;
	off_t lenpos, kstart, kend;
	unsigned long length;
	long flength;
	gzFile gzf;
	struct stat kern_stat;
	Elf32_External_Phdr phdr;

	elf_fd = open_file("bootloader", boot, &hdr, &elf_stat);
	if (inkernflag) {
		kern_fd = open_file("kernel", kernel, &khdr, &kern_stat);
		kern_len = kern_stat.st_size + PREP_MAGICSIZE + KERNLENSIZE;
	} else
		kern_len = PREP_MAGICSIZE + KERNLENSIZE;

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
	if ((prep_fd = open(outname, O_RDWR|O_TRUNC, 0)) < 0) {
		/* we couldn't open it, it must be new */
		prep_fd = creat(outname, 0644);
		if (prep_fd < 0)
			errx(2, "Can't open output '%s': %s", outname,
			    strerror(errno));
	}

	prep_check_mbr(prep_fd, rawdev);

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

	if (inkernflag) {
		/* Copy kernel */
		kern_img = (unsigned char *)malloc(kern_stat.st_size);

		if (kern_img == NULL)
			errx(3, "Can't malloc: %s", strerror(errno));

		/* we need to jump back after having read the headers */
		lseek(kern_fd, 0, SEEK_SET);
		if (read(kern_fd, (void *)kern_img, kern_stat.st_size) !=
		    kern_stat.st_size)
			errx(3, "Can't read kernel '%s' : %s",
			    kernel, strerror(errno));
	}

	gzf = gzdopen(dup(prep_fd), "a");
	if (gzf == NULL)
		errx(3, "Can't init compression: %s", strerror(errno));
	if (gzsetparams(gzf, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY) != Z_OK)
		errx(3, "%s", gzerror(gzf, &err));

	/* write a magic number and size before the kernel */
	write(prep_fd, (void *)prep_magic, PREP_MAGICSIZE);
	lenpos = lseek(prep_fd, 0, SEEK_CUR);
	tmp = sa_htobe32(0);
	write(prep_fd, (void *)&tmp, KERNLENSIZE);

	/* write in the compressed kernel */
	kstart = lseek(prep_fd, 0, SEEK_CUR);
	if (inkernflag) {
		kgzlen = gzwrite(gzf, kern_img, kern_stat.st_size);
		gzclose(gzf);
	}
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
		    outname, flength);

	if (inkernflag) {
		free(kern_img);
		close(kern_fd);
	}
	close(prep_fd);
	close(elf_fd);

	return 0;
}

/* Fill in the needed information on the boot and config records.  Most of
 * this is just AIX garbage that we don't really need to boot.
 */
static void
rs6000_build_records(int img_len)
{
	int bcl;

	/* zero out all the fields, so we only have to set the ones
	 * we care about, which are rather few.
	 */
	memset(&bootrec, 0, sizeof(rs6000_boot_record_t));
	memset(&confrec, 0, sizeof(rs6000_config_record_t));

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

static int
rs6000_build_image(char *kernel, char *boot, char *rawdev, char *outname)
{
	unsigned char *elf_img = NULL, *kern_img = NULL;
	int i, ch, tmp, kgzlen, err;
	int elf_fd, rs6000_fd, kern_fd, elf_img_len = 0, elf_pad;
	uint32_t swapped[128];
	off_t lenpos, kstart, kend;
	unsigned long length;
	long flength;
	gzFile gzf;
	struct stat kern_stat;
	Elf32_External_Phdr phdr;

	elf_fd = open_file("bootloader", boot, &hdr, &elf_stat);
	kern_fd = open_file("kernel", kernel, &khdr, &kern_stat);
	kern_len = kern_stat.st_size + RS6000_MAGICSIZE + KERNLENSIZE;

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
		if (verboseflag)
			printf("Padding %d\n", elf_pad);
		lseek(elf_fd, ELFGET32(phdr.p_offset), SEEK_SET);

		break;
	}
	if ((rs6000_fd = open(outname, O_RDWR|O_TRUNC, 0)) < 0) {
		/* we couldn't open it, it must be new */
		rs6000_fd = creat(outname, 0644);
		if (rs6000_fd < 0)
			errx(2, "Can't open output '%s': %s", outname,
			    strerror(errno));
	}

	/* Set file pos. to 2nd sector where image will be written */
	lseek(rs6000_fd, 0x400, SEEK_SET);

	/* Copy boot image */
	elf_img = (unsigned char *)malloc(elf_img_len);
	if (!elf_img)
		errx(3, "Can't malloc: %s", strerror(errno));
	if (read(elf_fd, elf_img, elf_img_len) != elf_img_len)
		errx(3, "Can't read file '%s' : %s", boot, strerror(errno));

	write(rs6000_fd, elf_img, elf_img_len);
	free(elf_img);

	/* now dump in the padding space for the BSS */
	elf_pad += 100; /* just a little extra for good luck */
	lseek(rs6000_fd, elf_pad, SEEK_CUR);

	/* Copy kernel */
	kern_img = (unsigned char *)malloc(kern_stat.st_size);

	if (kern_img == NULL)
		errx(3, "Can't malloc: %s", strerror(errno));

	/* we need to jump back after having read the headers */
	lseek(kern_fd, 0, SEEK_SET);
	if (read(kern_fd, (void *)kern_img, kern_stat.st_size) !=
	    kern_stat.st_size)
		errx(3, "Can't read kernel '%s' : %s", kernel, strerror(errno));

	gzf = gzdopen(dup(rs6000_fd), "a");
	if (gzf == NULL)
		errx(3, "Can't init compression: %s", strerror(errno));
	if (gzsetparams(gzf, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY) != Z_OK)
		errx(3, "%s", gzerror(gzf, &err));

	/* write a magic number and size before the kernel */
	write(rs6000_fd, (void *)rs6000_magic, RS6000_MAGICSIZE);
	lenpos = lseek(rs6000_fd, 0, SEEK_CUR);
	if (verboseflag)
		printf("wrote magic at pos 0x%lx\n", (unsigned long)lenpos);
	tmp = sa_htobe32(0);
	write(rs6000_fd, (void *)&tmp, KERNLENSIZE);

	/* write in the compressed kernel */
	kstart = lseek(rs6000_fd, 0, SEEK_CUR);
	if (verboseflag)
		printf("kernel start at pos 0x%lx\n", (unsigned long)kstart);
	kgzlen = gzwrite(gzf, kern_img, kern_stat.st_size);
	gzclose(gzf);
	kend = lseek(rs6000_fd, 0, SEEK_CUR);
	if (verboseflag)
		printf("kernel end at pos 0x%lx\n", (unsigned long)kend);

	/* jump back to the length position now that we know the length */
	lseek(rs6000_fd, lenpos, SEEK_SET);
	kgzlen = kend - kstart;
	tmp = sa_htobe32(kgzlen);
	if (verboseflag)
		printf("kernel len = 0x%x tmp = 0x%x\n", kgzlen, tmp);
	write(rs6000_fd, (void *)&tmp, KERNLENSIZE);

#if 0
	lseek(rs6000_fd, sizeof(boot_record_t) + sizeof(config_record_t),
	    SEEK_SET);
	/* set entry and length */
	length = sa_htole32(0x400);
	write(rs6000_fd, &length, sizeof(length));
	length = sa_htole32(0x400 + elf_img_len + 8 + kgzlen);
	write(rs6000_fd, &length, sizeof(length));
#endif

	/* generate the header now that we know the kernel length */
	if (verboseflag)
		printf("building records\n");
	rs6000_build_records(elf_img_len + 8 + kgzlen);
	lseek(rs6000_fd, 0, SEEK_SET);
	/* ROM wants it byteswapped in 32bit chunks */
	if (verboseflag)
		printf("writing records\n");
	memcpy(swapped, &bootrec, sizeof(rs6000_boot_record_t));
	for (i=0; i < 128; i++)
		swapped[i] = htonl(swapped[i]);
	write(rs6000_fd, swapped, sizeof(rs6000_boot_record_t));
	memcpy(swapped, &confrec, sizeof(rs6000_config_record_t));
	for (i=0; i < 128; i++)
		swapped[i] = htonl(swapped[i]);
	write(rs6000_fd, swapped, sizeof(rs6000_config_record_t));

	free(kern_img);
	close(kern_fd);
	close(rs6000_fd);
	close(elf_fd);

	return 0;
}

static int
bebox_write_header(int bebox_fd, int elf_image_len, int kern_img_len)
{
	int hsize = BEBOX_HEADER_SIZE;
	unsigned long textOffset, dataOffset, ldrOffset;
	unsigned long entry_vector[3];
	struct FileHeader fileHdr;
	struct SectionHeader textHdr, dataHdr, ldrHdr;
	struct LoaderHeader lh;

	if (saloneflag)
		hsize = 0;

	ldrOffset = ULALIGN(sizeof (fileHdr) + sizeof (textHdr) +
	    sizeof (dataHdr) + sizeof (ldrHdr));
	dataOffset = ULALIGN(ldrOffset + sizeof (lh));
	textOffset = ULALIGN(dataOffset + sizeof (entry_vector) + kern_img_len);

	/* Create the File Header */
	memset(&fileHdr, 0, sizeof (fileHdr));
	fileHdr.magic = sa_htobe32(PEF_MAGIC);
	fileHdr.fileTypeID = sa_htobe32(PEF_FILE);
	fileHdr.archID = sa_htobe32(PEF_PPC);
	fileHdr.versionNumber = sa_htobe32(1);
	fileHdr.numSections = sa_htobe16(3);
	fileHdr.loadableSections = sa_htobe16(2);
	write(bebox_fd, &fileHdr, sizeof (fileHdr));

	/* Create the Section Header for TEXT */
	memset(&textHdr, 0, sizeof (textHdr));
	textHdr.sectionName = sa_htobe32(-1);
	textHdr.sectionAddress = sa_htobe32(0);
	textHdr.execSize = sa_htobe32(elf_image_len);
	textHdr.initSize = sa_htobe32(elf_image_len);
	textHdr.rawSize = sa_htobe32(elf_image_len);
	textHdr.fileOffset = sa_htobe32(textOffset);
	textHdr.regionKind = CodeSection;
	textHdr.shareKind = ContextShare;
	textHdr.alignment = 4;  /* 16 byte alignment */
	write(bebox_fd, &textHdr, sizeof (textHdr));

	/* Create the Section Header for DATA */
	memset(&dataHdr, 0, sizeof (dataHdr));
	dataHdr.sectionName = sa_htobe32(-1);
	dataHdr.sectionAddress = sa_htobe32(0);
	dataHdr.execSize = sa_htobe32(sizeof (entry_vector) + kern_img_len);
	dataHdr.initSize = sa_htobe32(sizeof (entry_vector) + kern_img_len);
	dataHdr.rawSize = sa_htobe32(sizeof (entry_vector) + kern_img_len);
	dataHdr.fileOffset = sa_htobe32(dataOffset);
	dataHdr.regionKind = DataSection;
	dataHdr.shareKind = ContextShare;
	dataHdr.alignment = 4;  /* 16 byte alignment */
	write(bebox_fd, &dataHdr, sizeof (dataHdr));

	/* Create the Section Header for loader stuff */
	memset(&ldrHdr, 0, sizeof (ldrHdr));
	ldrHdr.sectionName = sa_htobe32(-1);
	ldrHdr.sectionAddress = sa_htobe32(0);
	ldrHdr.execSize = sa_htobe32(sizeof (lh));
	ldrHdr.initSize = sa_htobe32(sizeof (lh));
	ldrHdr.rawSize = sa_htobe32(sizeof (lh));
	ldrHdr.fileOffset = sa_htobe32(ldrOffset);
	ldrHdr.regionKind = LoaderSection;
	ldrHdr.shareKind = GlobalShare;
	ldrHdr.alignment = 4;  /* 16 byte alignment */
	write(bebox_fd, &ldrHdr, sizeof (ldrHdr));

	/* Create the Loader Header */
	memset(&lh, 0, sizeof (lh));
	lh.entryPointSection = sa_htobe32(1);		/* Data */
	lh.entryPointOffset = sa_htobe32(0);
	lh.initPointSection = sa_htobe32(-1);
	lh.initPointOffset = sa_htobe32(0);
	lh.termPointSection = sa_htobe32(-1);
	lh.termPointOffset = sa_htobe32(0);
	lseek(bebox_fd, ldrOffset + hsize, SEEK_SET);
	write(bebox_fd, &lh, sizeof (lh));

	/* Copy the pseudo-DATA */
	memset(entry_vector, 0, sizeof (entry_vector));
	entry_vector[0] = sa_htobe32(BEBOX_ENTRY);	/* Magic */
	lseek(bebox_fd, dataOffset + hsize, SEEK_SET);
	write(bebox_fd, entry_vector, sizeof (entry_vector));

	return textOffset;
}

static int
bebox_build_image(char *kernel, char *boot, char *rawdev, char *outname)
{
	unsigned char *elf_img = NULL, *kern_img = NULL, *header_img = NULL;
	int i, ch, tmp, kgzlen, err, hsize = BEBOX_HEADER_SIZE;
	int elf_fd, bebox_fd, kern_fd, elf_img_len = 0;
	uint32_t swapped[128];
	off_t lenpos, kstart, kend, toff, endoff;
	unsigned long length;
	long flength, *offset;
	gzFile gzf;
	struct stat kern_stat;
	struct bebox_image_block *p;
	struct timeval tp;
	Elf32_External_Phdr phdr;

	if (saloneflag)
		hsize = 0;

	elf_fd = open_file("bootloader", boot, &hdr, &elf_stat);
	if (inkernflag) {
		kern_fd = open_file("kernel", kernel, &khdr, &kern_stat);
		kern_len = kern_stat.st_size + BEBOX_MAGICSIZE + KERNLENSIZE;
	} else
		kern_len = BEBOX_MAGICSIZE + KERNLENSIZE;

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
		elf_img_len = ELFGET32(phdr.p_filesz);
		lseek(elf_fd, ELFGET32(phdr.p_offset), SEEK_SET);

		break;
	}
	if ((bebox_fd = open(outname, O_RDWR|O_TRUNC, 0)) < 0) {
		/* we couldn't open it, it must be new */
		bebox_fd = creat(outname, 0644);
		if (bebox_fd < 0)
			errx(2, "Can't open output '%s': %s", outname,
			    strerror(errno));
	}
	lseek(bebox_fd, hsize, SEEK_SET);

	if (inkernflag) {
		/*
		 * write the header with the wrong values to get the offset
		 * right
		 */
		bebox_write_header(bebox_fd, elf_img_len, kern_stat.st_size);

		/* Copy kernel */
		kern_img = (unsigned char *)malloc(kern_stat.st_size);

		if (kern_img == NULL)
			errx(3, "Can't malloc: %s", strerror(errno));

		/* we need to jump back after having read the headers */
		lseek(kern_fd, 0, SEEK_SET);
		if (read(kern_fd, (void *)kern_img, kern_stat.st_size) !=
		    kern_stat.st_size)
			errx(3, "Can't read kernel '%s' : %s",
			    kernel, strerror(errno));

		gzf = gzdopen(dup(bebox_fd), "a");
		if (gzf == NULL)
			errx(3, "Can't init compression: %s", strerror(errno));
		if (gzsetparams(gzf, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY) !=
		    Z_OK)
			errx(3, "%s", gzerror(gzf, &err));
	} else
		bebox_write_header(bebox_fd, elf_img_len, 0);

	/* write a magic number and size before the kernel */
	write(bebox_fd, (void *)bebox_magic, BEBOX_MAGICSIZE);
	lenpos = lseek(bebox_fd, 0, SEEK_CUR);
	tmp = sa_htobe32(0);
	write(bebox_fd, (void *)&tmp, KERNLENSIZE);

	if (inkernflag) {
		/* write in the compressed kernel */
		kstart = lseek(bebox_fd, 0, SEEK_CUR);
		kgzlen = gzwrite(gzf, kern_img, kern_stat.st_size);
		gzclose(gzf);
		kend = lseek(bebox_fd, 0, SEEK_CUR);
		free(kern_img);
	} else {
		kstart = kend = lseek(bebox_fd, 0, SEEK_CUR);
		kgzlen = 0;
	}

	/* jump back to the length position now that we know the length */
	lseek(bebox_fd, lenpos, SEEK_SET);
	kgzlen = kend - kstart;
	tmp = sa_htobe32(kgzlen);
	write(bebox_fd, (void *)&tmp, KERNLENSIZE);

	/* now rewrite the header correctly */
	lseek(bebox_fd, hsize, SEEK_SET);
	tmp = kgzlen + BEBOX_MAGICSIZE + KERNLENSIZE;
	toff = bebox_write_header(bebox_fd, elf_img_len, tmp);

	/* Copy boot image */
	elf_img = (unsigned char *)malloc(elf_img_len);
	if (!elf_img)
		errx(3, "Can't malloc: %s", strerror(errno));
	if (read(elf_fd, elf_img, elf_img_len) != elf_img_len)
		errx(3, "Can't read file '%s' : %s", boot, strerror(errno));
	lseek(bebox_fd, toff + hsize, SEEK_SET);
	write(bebox_fd, elf_img, elf_img_len);
	free(elf_img);

	if (inkernflag)
		close(kern_fd);
	close(elf_fd);

	if (saloneflag) {
		close(bebox_fd);
		return 0;
	}

	/* Now go back and write in the block header */
	endoff = lseek(bebox_fd, 0, SEEK_END);
	lseek(bebox_fd, 0, SEEK_SET);
	header_img = (unsigned char *)malloc(BEBOX_HEADER_SIZE);
	if (!header_img)
		errx(3, "Can't malloc: %s", strerror(errno));
	memset(header_img, 0, BEBOX_HEADER_SIZE);

	/* copy the boot image into the buffer */
	for (p = bebox_image_block; p->offset != -1; p++)
		memcpy(header_img + p->offset, p->data, p->size);

	/* fill used block bitmap */
	memset(header_img + BEBOX_FILE_BLOCK_MAP_START, 0xff,
	    BEBOX_FILE_BLOCK_MAP_END - BEBOX_FILE_BLOCK_MAP_START);

	/* fix the file size in the header */
	tmp = endoff - BEBOX_HEADER_SIZE;
	*(long *)(header_img + BEBOX_FILE_SIZE_OFFSET) =
	    (long)sa_htobe32(tmp);
	*(long *)(header_img + BEBOX_FILE_SIZE_ALIGN_OFFSET) =
	    (long)sa_htobe32(roundup(tmp, BEBOX_FILE_BLOCK_SIZE));

	gettimeofday(&tp, 0);
	for (offset = bebox_mtime_offset; *offset != -1; offset++)
		*(long *)(header_img + *offset) = (long)sa_htobe32(tp.tv_sec);

	write(bebox_fd, header_img, BEBOX_HEADER_SIZE);

	/* now pad the end */
	flength = roundup(endoff, BEBOX_BLOCK_SIZE);
	/* refill the header_img with zeros */
	memset(header_img, 0, BEBOX_BLOCK_SIZE * 2);
	lseek(bebox_fd, 0, SEEK_END);
	write(bebox_fd, header_img, flength - endoff);

	close(bebox_fd);

	return 0;
}

int
main(int argc, char **argv)
{
	int ch, lfloppyflag=0;
	char *kernel = NULL, *boot = NULL, *rawdev = NULL, *outname = NULL;
	char *march = NULL;
#ifdef USE_SYSCTL
	char machine[SYS_NMLN];
	int mib[2] = { CTL_HW, HW_MACHINE };
#endif

	setprogname(argv[0]);
	kern_len = 0;

	while ((ch = getopt(argc, argv, "b:Ik:lm:r:sv")) != -1)
		switch (ch) {
		case 'b':
			boot = optarg;
			break;
		case 'I':
			inkernflag = 0;
			break;
		case 'k':
			kernel = optarg;
			inkernflag = 1;
			break;
		case 'l':
			lfloppyflag = 1;
			break;
		case 'm':
			march = optarg;
			break;
		case 'r':
			rawdev = optarg;
			break;
		case 's':
			saloneflag = 1;
			break;
		case 'v':
			verboseflag = 1;
			break;
		case '?':
		default:
			usage(0);
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage(0);

	if (kernel == NULL && inkernflag)
		kernel = "/netbsd";

	if (boot == NULL)
		boot = "/usr/mdec/boot";

	if (march != NULL && strcmp(march, "") == 0)
		march = NULL;
	if (march == NULL) {
		int i;
#ifdef USE_SYSCTL
		size_t len = sizeof(machine);

		if (sysctl(mib, sizeof (mib) / sizeof (mib[0]), machine,
			&len, NULL, 0) != -1) {
			for (i=0; sup_plats[i] != NULL; i++) {
				if (strcmp(sup_plats[i], machine) == 0) {
					march = strdup(sup_plats[i]);
					break;
				}
			}
		}
		if (march == NULL)
#endif
			usage(1);
	}

	outname = argv[0];

	if (strcmp(march, "prep") == 0)
		return(prep_build_image(kernel, boot, rawdev, outname));
	if (strcmp(march, "rs6000") == 0)
		return(rs6000_build_image(kernel, boot, rawdev, outname));
	if (strcmp(march, "bebox") == 0)
		return(bebox_build_image(kernel, boot, rawdev, outname));

	usage(1);
	return(0);
}
