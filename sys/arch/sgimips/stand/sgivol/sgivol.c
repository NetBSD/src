/*	$NetBSD: sgivol.c,v 1.18.38.1 2014/08/20 00:03:23 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Hitch and Hubert Feyrer.
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
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#if HAVE_NBTOOL_CONFIG_H
#include "../../../../../sys/sys/bootblock.h"
/* Ficticious geometry for cross tool usage against a file image */
#define SGIVOL_NBTOOL_NSECS	32
#define SGIVOL_NBTOOL_NTRACKS	64
#else
#include <sys/disklabel.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <util.h>
#include <err.h>
#ifndef HAVE_NBTOOL_CONFIG_H
#include <sys/endian.h>
#endif

int	fd;
int	opt_i;			/* Initialize volume header */
int	opt_r;			/* Read a file from volume header */
int	opt_w;			/* Write a file to volume header */
int	opt_d;			/* Delete a file from volume header */
int	opt_m;			/* Move (rename) a file in the volume header */
int	opt_p;			/* Modify a partition */
int	opt_q;			/* quiet mode */
int	opt_f;			/* Don't ask, just do what you're told */
int	partno, partfirst, partblocks, parttype;
struct sgi_boot_block *volhdr;
int32_t	checksum;
u_int32_t	volhdr_size = SGI_BOOT_BLOCK_SIZE_VOLHDR;

const char *vfilename = "";
const char *ufilename = "";

#if HAVE_NBTOOL_CONFIG_H
struct stat st;
#else
struct disklabel lbl;
#endif

char buf[512];

const char *sgi_types[] = {
	"Volume Header",
	"Repl Trks",
	"Repl Secs",
	"Raw",
	"BSD4.2",
	"SysV",
	"Volume",
	"EFS",
	"LVol",
	"RLVol",
	"XFS",
	"XSFLog",
	"XLV",
	"XVM"
};

void	display_vol(void);
void	init_volhdr(const char *);
void	read_file(void);
void	write_file(const char *);
void	delete_file(const char *);
void	move_file(const char *);
void	modify_partition(const char *);
void	write_volhdr(const char *);
int	allocate_space(int);
void	checksum_vol(void);
int	names_match(int, const char *);
void	usage(void) __dead;

int
main(int argc, char *argv[])
{
#define RESET_OPTS()	opt_i = opt_m = opt_r = opt_w = opt_d = opt_p = 0

	int ch;
	while ((ch = getopt(argc, argv, "qfih:rwdmp?")) != -1) {
		switch (ch) {
		/* -i, -r, -w, -d, -m and -p override each other */
		/* -q implies -f */
		case 'q':
			++opt_q;
			++opt_f;
			break;
		case 'f':
			++opt_f;
			break;
		case 'i':
			RESET_OPTS();
			++opt_i;
			break;
		case 'h':
			volhdr_size = atoi(optarg);
			break;
		case 'r':
			RESET_OPTS();
			++opt_r;
			break;
		case 'w':
			RESET_OPTS();
			++opt_w;
			break;
		case 'd':
			RESET_OPTS();
			++opt_d;
			break;
		case 'm':
			RESET_OPTS();
			++opt_m;
			break;
		case 'p':
			RESET_OPTS();
			++opt_p;
			partno = atoi(argv[0]);
			partfirst = atoi(argv[1]);
			partblocks = atoi(argv[2]);
			parttype = atoi(argv[3]);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (opt_m || opt_r || opt_w) {
		if (argc != 3)
			usage();
		vfilename = argv[0];
		ufilename = argv[1];
		argc -= 2;
		argv += 2;
	}
	if (opt_d) {
		if (argc != 2)
			usage();
		vfilename = argv[0];
		argc--;
		argv++;
	}

	if (opt_p) {
		if (argc != 5)
			usage();
		partno = atoi(argv[0]);
		partfirst = atoi(argv[1]);
		partblocks = atoi(argv[2]);
		parttype = atoi(argv[3]);
		argc -= 4;
		argv += 4;
	}
	if (argc != 1)
		usage();
	
	fd = open(argv[0],
	    (opt_i | opt_m | opt_w | opt_d | opt_p) ? O_RDWR : O_RDONLY);
	if (fd == -1) {
#ifndef HAVE_NBTOOL_CONFIG_H
		snprintf(buf, sizeof(buf), "/dev/r%s%c", argv[0],
		    'a' + getrawpartition());
		fd = open(buf, (opt_i | opt_w | opt_d | opt_p) 
		    ? O_RDWR : O_RDONLY);
		if (fd == -1)
#endif
		err(EXIT_FAILURE, "Error opening device `%s'", argv[0]);
	}

	if (read(fd, buf, sizeof(buf)) != sizeof(buf))
		err(EXIT_FAILURE, "Can't read volhdr from `%s'", argv[0]);

#if HAVE_NBTOOL_CONFIG_H
	if (fstat(fd, &st) == -1)
		err(EXIT_FAILURE, "Can't stat `%s'", argv[0]);
	if (!S_ISREG(st.st_mode))
		errx(EXIT_FAILURE, "Not a regular file `%s'", argv[0]);

	if (st.st_size % SGI_BOOT_BLOCK_BLOCKSIZE)
		errx(EXIT_FAILURE, "Size must be multiple of %d", 
		    SGI_BOOT_BLOCK_BLOCKSIZE);
	if (st.st_size < (SGIVOL_NBTOOL_NSECS * SGIVOL_NBTOOL_NTRACKS))
		errx(EXIT_FAILURE, "Minimum size of %d required",
		    SGIVOL_NBTOOL_NSECS * SGIVOL_NBTOOL_NTRACKS);
#else
	if (ioctl(fd, DIOCGDINFO, &lbl) == -1)
		err(EXIT_FAILURE, "ioctl DIOCGDINFO failed");
#endif
	volhdr = (struct sgi_boot_block *) buf;
	if (opt_i) {
		init_volhdr(argv[0]);
		return 0;
	}
	if (be32toh(volhdr->magic) != SGI_BOOT_BLOCK_MAGIC)
		errx(EXIT_FAILURE, "No Volume Header found, magic=%x. "
		    "Use -i first.\n", be32toh(volhdr->magic));
	if (opt_r) {
		read_file();
		return 0;
	}
	if (opt_w) {
		write_file(argv[0]);
		return 0;
	}
	if (opt_d) {
		delete_file(argv[0]);
		return 0;
	}
	if (opt_m) {
		move_file(argv[0]);
		return 0;
	}
	if (opt_p) {
		modify_partition(argv[0]);
		return 0;
	}

	if (!opt_q)
		display_vol();

	return 0;
}

/*
 * Compare the name in `slot' of voldir to `b'. Be careful, as the
 * name in voldir need not be nul-terminated and `b' may be longer
 * than the maximum (in which case it will never match).
 *
 * Returns non-0 if names are equal.
 */
int
names_match(int slot, const char *b)
{
	int cmp;

	if (slot < 0 || slot >= SGI_BOOT_BLOCK_MAXVOLDIRS)
		errx(EXIT_FAILURE, "Internal error: bad slot in %s()",
		    __func__);

	cmp = strncmp(volhdr->voldir[slot].name, b,
	    sizeof(volhdr->voldir[slot].name));

	return cmp == 0 && strlen(b) <= sizeof(volhdr->voldir[slot].name);
}

void
display_vol(void)
{
	int32_t *l;
	int i;

#if HAVE_NBTOOL_CONFIG_H
	printf("disklabel shows %d sectors\n", 
	    st.st_size / SGI_BOOT_BLOCK_BLOCKSIZE);
#else
	printf("disklabel shows %d sectors\n", lbl.d_secperunit);
#endif
	l = (int32_t *)buf;
	checksum = 0;
	for (i = 0; i < 512 / 4; ++i)
		checksum += be32toh(l[i]);
	printf("checksum: %08x%s\n", checksum, checksum == 0 ? "" : " *ERROR*");
	printf("root part: %d\n", be16toh(volhdr->root));
	printf("swap part: %d\n", be16toh(volhdr->swap));
	printf("bootfile: %s\n", volhdr->bootfile);
	/* volhdr->devparams[0..47] */
	printf("\nVolume header files:\n");
	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; ++i)
		if (volhdr->voldir[i].name[0])
			printf("%-8s offset %4d blocks, length %8d bytes "
			    "(%d blocks)\n",
			    volhdr->voldir[i].name, 
                            be32toh(volhdr->voldir[i].block),
			    be32toh(volhdr->voldir[i].bytes), 
                            (be32toh(volhdr->voldir[i].bytes) + 511) / 512);
	printf("\nSGI partitions:\n");
	for (i = 0; i < SGI_BOOT_BLOCK_MAXPARTITIONS; ++i) {
		if (be32toh(volhdr->partitions[i].blocks)) {
			printf("%2d:%c blocks %8d first %8d type %2d (%s)\n",
			  i, i + 'a', be32toh(volhdr->partitions[i].blocks),
			       be32toh(volhdr->partitions[i].first),
			       be32toh(volhdr->partitions[i].type),
			  be32toh(volhdr->partitions[i].type) > 13 ? "???" :
			    sgi_types[be32toh(volhdr->partitions[i].type)]);
		}
	}
}

void
init_volhdr(const char *fname)
{
	memset(buf, 0, sizeof(buf));
	volhdr->magic = htobe32(SGI_BOOT_BLOCK_MAGIC);
	volhdr->root = htobe16(0);
	volhdr->swap = htobe16(1);
	strcpy(volhdr->bootfile, "/netbsd");
#if HAVE_NBTOOL_CONFIG_H
	volhdr->dp.dp_skew = 0;
#else
	volhdr->dp.dp_skew = lbl.d_trackskew;
#endif
	volhdr->dp.dp_gap1 = 1; /* XXX */
	volhdr->dp.dp_gap2 = 1; /* XXX */
#if HAVE_NBTOOL_CONFIG_H        
	volhdr->dp.dp_cyls = 
	    htobe16(st.st_size / (SGIVOL_NBTOOL_NSECS * SGIVOL_NBTOOL_NTRACKS));
#else
	volhdr->dp.dp_cyls = htobe16(lbl.d_ncylinders);
#endif
	volhdr->dp.dp_shd0 = 0;
#if HAVE_NBTOOL_CONFIG_H
	volhdr->dp.dp_trks0 = htobe16(SGIVOL_NBTOOL_NTRACKS);
	volhdr->dp.dp_secs = htobe16(SGIVOL_NBTOOL_NSECS);
	volhdr->dp.dp_secbytes = htobe16(SGI_BOOT_BLOCK_BLOCKSIZE);
	volhdr->dp.dp_interleave = htobe16(1);
#else
	volhdr->dp.dp_trks0 = htobe16(lbl.d_ntracks);
	volhdr->dp.dp_secs = htobe16(lbl.d_nsectors);
	volhdr->dp.dp_secbytes = htobe16(lbl.d_secsize);
	volhdr->dp.dp_interleave = htobe16(lbl.d_interleave);
#endif
	volhdr->dp.dp_nretries = htobe32(22);
#if HAVE_NBTOOL_CONFIG_H
	volhdr->partitions[10].blocks = 
	    htobe32(st.st_size / SGI_BOOT_BLOCK_BLOCKSIZE);
#else
	volhdr->partitions[10].blocks = htobe32(lbl.d_secperunit);
#endif
	volhdr->partitions[10].first = 0;
	volhdr->partitions[10].type = htobe32(SGI_PTYPE_VOLUME);
	volhdr->partitions[8].blocks = htobe32(volhdr_size);
	volhdr->partitions[8].first = 0;
	volhdr->partitions[8].type = htobe32(SGI_PTYPE_VOLHDR);
#if HAVE_NBTOOL_CONFIG_H
	volhdr->partitions[0].blocks = 
	    htobe32((st.st_size / SGI_BOOT_BLOCK_BLOCKSIZE) - volhdr_size);
#else
	volhdr->partitions[0].blocks = htobe32(lbl.d_secperunit - volhdr_size);
#endif
	volhdr->partitions[0].first = htobe32(volhdr_size);
	volhdr->partitions[0].type = htobe32(SGI_PTYPE_BSD);
	write_volhdr(fname);
}

void
read_file(void)
{
	FILE *fp;
	int i;

	if (!opt_q)
		printf("Reading file %s\n", vfilename);
	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; ++i) {
		if (strncmp(vfilename, volhdr->voldir[i].name,
			    strlen(volhdr->voldir[i].name)) == 0)
			break;
	}
	if (i >= SGI_BOOT_BLOCK_MAXVOLDIRS)
		errx(EXIT_FAILURE, "File `%s' not found", vfilename);
	/* XXX assumes volume header starts at 0? */
	lseek(fd, be32toh(volhdr->voldir[i].block) * 512, SEEK_SET);
	fp = fopen(ufilename, "w");
	if (fp == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", ufilename);
	i = be32toh(volhdr->voldir[i].bytes);
	while (i > 0) {
		if (read(fd, buf, sizeof(buf)) != sizeof(buf))
			err(EXIT_FAILURE, "Error reading from `%s'", ufilename);
		fwrite(buf, 1, i > sizeof(buf) ? sizeof(buf) : i, fp);
		i -= i > sizeof(buf) ? sizeof(buf) : i;
	}
	fclose(fp);
}

void
write_file(const char *fname)
{
	FILE *fp;
	int slot;
	size_t namelen;
	int block, i;
	off_t off;
	struct stat st;
	char fbuf[512];

	if (!opt_q)
		printf("Writing file %s\n", ufilename);
	if (stat(ufilename, &st) == -1)
		err(EXIT_FAILURE, "Can't stat `%s'", ufilename);
	if (!opt_q)
		printf("File %s has %ju bytes\n", ufilename,
		    (uintmax_t)st.st_size);
	slot = -1;
	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; ++i) {
		if (volhdr->voldir[i].name[0] == '\0' && slot < 0)
			slot = i;
		if (names_match(i, vfilename)) {
			slot = i;
			break;
		}
	}
	if (slot == -1)
		errx(EXIT_FAILURE, "No directory space for file %s", vfilename);
	/* -w can overwrite, -a won't overwrite */
	if (be32toh(volhdr->voldir[slot].block) > 0) {
		if (!opt_q)
			printf("File %s exists, removing old file\n", 
				vfilename);
		volhdr->voldir[slot].name[0] = 0;
		volhdr->voldir[slot].block = volhdr->voldir[slot].bytes = 0;
	}
	if (st.st_size == 0) {
		errx(EXIT_FAILURE, "Empty file `%s'", ufilename);
		exit(1);
	}
	/* XXX assumes volume header starts at 0? */
	block = allocate_space((int)st.st_size);
	if (block < 0)
		errx(EXIT_FAILURE, "No space for file `%s'", vfilename);

	/*
	 * Make sure the name in the volume header is max. 8 chars,
	 * NOT including NUL.
	 */
	namelen = strlen(vfilename);
	if (namelen > sizeof(volhdr->voldir[slot].name)) {
		printf("Warning: '%s' is too long for volume header, ",
		       vfilename);
		namelen = sizeof(volhdr->voldir[slot].name);
		printf("truncating to '%.8s'\n", vfilename);
	}

	/* Populate it w/ NULs */
	memset(volhdr->voldir[slot].name, 0,
	    sizeof(volhdr->voldir[slot].name));
	/* Then copy the name */
	memcpy(volhdr->voldir[slot].name, vfilename, namelen);

	volhdr->voldir[slot].block = htobe32(block);
	volhdr->voldir[slot].bytes = htobe32(st.st_size);

	write_volhdr(fname);

	/* write the file itself */
	off = lseek(fd, block * 512, SEEK_SET);
	if (off == -1)
		err(EXIT_FAILURE, "Seek failed `%s'", fname);
	i = st.st_size;
	fp = fopen(ufilename, "r");
	if (fp == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", ufilename);
	while (i > 0) {
		int j = i > 512 ? 512 : i;
		if (fread(fbuf, 1, j, fp) != j)
			err(EXIT_FAILURE, "Can't read `%s'", ufilename);
		if (write(fd, fbuf, 512) != 512)
			err(EXIT_FAILURE, "Can't write `%s'", fname);
		i -= j;
	}
	fclose(fp);
}

void
delete_file(const char *fname)
{
	int i;

	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; ++i) {
		if (names_match(i, vfilename)) {
			break;
		}
	}
	if (i >= SGI_BOOT_BLOCK_MAXVOLDIRS)
		errx(EXIT_FAILURE, "File `%s' not found", vfilename);

	/* XXX: we don't compact the file space, so get fragmentation */
	volhdr->voldir[i].name[0] = '\0';
	volhdr->voldir[i].block = volhdr->voldir[i].bytes = 0;
	write_volhdr(fname);
}

void
move_file(const char *fname)
{
	char dstfile[sizeof(volhdr->voldir[0].name) + 1];
	size_t namelen;
	int i, slot = -1;

	/*
	 * Make sure the name in the volume header is max. 8 chars,
	 * NOT including NUL.
	 */
	namelen = strlen(ufilename);
	if (namelen > sizeof(volhdr->voldir[0].name)) {
		printf("Warning: '%s' is too long for volume header, ",
		       ufilename);
		namelen = sizeof(volhdr->voldir[0].name);
		printf("truncating to '%.8s'\n", ufilename);
	}
	memset(dstfile, 0, sizeof(dstfile));
	memcpy(dstfile, ufilename, namelen);

	for (i = 0; i < SGI_BOOT_BLOCK_MAXVOLDIRS; i++) {
		if (names_match(i, vfilename)) {
			if (slot != -1)
				errx(EXIT_FAILURE,
				    "Error: Cannot move '%s' to '%s' - "
				    "duplicate source files exist!",
				    vfilename, dstfile);
			slot = i;
		}
		if (names_match(i, dstfile))
			errx(EXIT_FAILURE, "Error: Cannot move '%s' to '%s' - "
			    "destination file already exists!",
			    vfilename, dstfile);
	}
	if (slot == -1)
		errx(EXIT_FAILURE, "File `%s' not found", vfilename);

	/* `dstfile' is already padded with NULs */ 
	memcpy(volhdr->voldir[slot].name, dstfile,
	    sizeof(volhdr->voldir[slot].name));
	
	write_volhdr(fname);
}

void
modify_partition(const char *fname)
{
	if (!opt_q)
		printf("Modify partition %d start %d length %d\n", 
			partno, partfirst, partblocks);
	if (partno < 0 || partno >= SGI_BOOT_BLOCK_MAXPARTITIONS)
		errx(EXIT_FAILURE, "Invalid partition number: %d", partno);

	volhdr->partitions[partno].blocks = htobe32(partblocks);
	volhdr->partitions[partno].first = htobe32(partfirst);
	volhdr->partitions[partno].type = htobe32(parttype);
	write_volhdr(fname);
}

void
write_volhdr(const char *fname)
{
	int i;

	checksum_vol();

	if (!opt_q)
		display_vol();
	if (!opt_f) {
		printf("\nDo you want to update volume (y/n)? ");
		i = getchar();
		if (i != 'Y' && i != 'y')
			exit(1);
	}
	i = lseek(fd, 0, SEEK_SET);
	if (i < 0) {
		perror("lseek 0");
		exit(1);
	}
	i = write(fd, buf, 512);
	if (i < 0)
		errx(EXIT_FAILURE, "write volhdr `%s'", fname);
}

int
allocate_space(int size)
{
	int n, blocks;
	int first;

	blocks = (size + 511) / 512;
	first = 2;
	n = 0;
	while (n < SGI_BOOT_BLOCK_MAXVOLDIRS) {
		if (volhdr->voldir[n].name[0]) {
			if (first < (be32toh(volhdr->voldir[n].block) +
			  (be32toh(volhdr->voldir[n].bytes) + 511) / 512) &&
			    (first + blocks) > be32toh(volhdr->voldir[n].block)) {
				first = be32toh(volhdr->voldir[n].block) +
					(be32toh(volhdr->voldir[n].bytes) + 511) / 512;
#if 0
				printf("allocate: n=%d first=%d blocks=%d size=%d\n", n, first, blocks, size);
				printf("%s %d %d\n", volhdr->voldir[n].name, volhdr->voldir[n].block, volhdr->voldir[n].bytes);
				printf("first=%d block=%d last=%d end=%d\n", first, volhdr->voldir[n].block,
				       first + blocks - 1, volhdr->voldir[n].block + (volhdr->voldir[n].bytes + 511) / 512);
#endif
				n = 0;
				continue;
			}
		}
		++n;
	}
#if HAVE_NBTOOL_CONFIG_H
	if (first + blocks > (st.st_size / SGI_BOOT_BLOCK_BLOCKSIZE))
#else
	if (first + blocks > lbl.d_secperunit)
#endif
		first = -1;
	/* XXX assumes volume header is partition 8 */
	/* XXX assumes volume header starts at 0? */
	if (first + blocks >= be32toh(volhdr->partitions[8].blocks))
		first = -1;
	return (first);
}

void
checksum_vol(void)
{
	int32_t *l;
	int i;

	volhdr->checksum = checksum = 0;
	l = (int32_t *)buf;
	for (i = 0; i < 512 / 4; ++i)
		checksum += be32toh(l[i]);
	volhdr->checksum = htobe32(-checksum);
}

void
usage(void)
{
	const char *p = getprogname();
	printf("Usage:\t%s [-qf] -i [-h vhsize] device\n"
	       "\t%s [-qf] -r vhfilename diskfilename device\n"
	       "\t%s [-qf] -w vhfilename diskfilename device\n"
	       "\t%s [-qf] -d vhfilename device\n"
	       "\t%s [-qf] -m vhfilename vhfilename device\n"
	       "\t%s [-qf] -p partno partfirst partblocks "
	       "parttype device\n", p, p, p, p, p, p);
	exit(0);
}
