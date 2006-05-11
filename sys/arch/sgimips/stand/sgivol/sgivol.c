/*	$NetBSD: sgivol.c,v 1.11.10.1 2006/05/11 23:27:03 elad Exp $	*/

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
#ifndef HAVE_NBTOOL_CONFIG_H
#include <sys/endian.h>
#endif

int	fd;
int	opt_i;			/* Initialize volume header */
int	opt_r;			/* Read a file from volume header */
int	opt_w;			/* Write a file to volume header */
int	opt_d;			/* Delete a file from volume header */
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

unsigned char buf[512];

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

int	main(int, char *[]);

void	display_vol(void);
void	init_volhdr(void);
void	read_file(void);
void	write_file(void);
void	delete_file(void);
void	modify_partition(void);
void	write_volhdr(void);
int	allocate_space(int);
void	checksum_vol(void);
void	usage(void);

int
main(int argc, char *argv[])
{
	int ch;
	while ((ch = getopt(argc, argv, "irwpdqfh:")) != -1) {
		switch (ch) {
		/* -i, -r, -w, -d and -p override each other */
		/* -q implies -f */
		case 'q':
			++opt_q;
			++opt_f;
			break;
		case 'f':
			++opt_f;
			break;
		case 'i':
			++opt_i;
			opt_r = opt_w = opt_d = opt_p = 0;
			break;
		case 'h':
			volhdr_size = atoi(optarg);
			break;
		case 'r':
			++opt_r;
			opt_i = opt_w = opt_d = opt_p = 0;
			break;
		case 'w':
			++opt_w;
			opt_i = opt_r = opt_d = opt_p = 0;
			break;
		case 'd':
			++opt_d;
			opt_i = opt_r = opt_w = opt_p = 0;
			break;
		case 'p':
			++opt_p;
			opt_i = opt_r = opt_w = opt_d = 0;
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

	if (opt_r || opt_w) {
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
	
	fd = open(argv[0], (opt_i | opt_w | opt_d | opt_p) ? O_RDWR : O_RDONLY);
	if (fd < 0) {
#if HAVE_NBTOOL_CONFIG_H
		perror("File open");
		exit(1);
#else
		sprintf(buf, "/dev/r%s%c", argv[0], 'a' + getrawpartition());
		fd = open(buf, (opt_i | opt_w | opt_d | opt_p) 
				? O_RDWR : O_RDONLY);
		if (fd < 0) {
			printf("Error opening device %s: %s\n",
				argv[0], strerror(errno));
			exit(1);
		}
#endif
	}
	if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
		perror("read volhdr");
		exit(1);
	}
#if HAVE_NBTOOL_CONFIG_H
	if (fstat(fd, &st) < 0) {
		perror("stat error");
		exit(1);
	}
	if (!S_ISREG(st.st_mode)) {
		printf("Must be regular file\n");
		exit(1);
	}
	if (st.st_size % SGI_BOOT_BLOCK_BLOCKSIZE) {
		printf("Size must be multiple of %d\n", 
		    SGI_BOOT_BLOCK_BLOCKSIZE);
		exit(1);
	}
	if (st.st_size < (SGIVOL_NBTOOL_NSECS * SGIVOL_NBTOOL_NTRACKS)) {
		printf("Minimum size of %d required\n",
		    SGIVOL_NBTOOL_NSECS * SGIVOL_NBTOOL_NTRACKS);
		exit(1);
	}
#else
	if (ioctl(fd, DIOCGDINFO, &lbl) < 0) {
		perror("DIOCGDINFO");
		exit(1);
	}
#endif
	volhdr = (struct sgi_boot_block *) buf;
	if (opt_i) {
		init_volhdr();
		exit(0);
	}
	if (be32toh(volhdr->magic) != SGI_BOOT_BLOCK_MAGIC) {
		printf("No Volume Header found, magic=%x.  Use -i first.\n", 
		       be32toh(volhdr->magic));
		exit(1);
	}
	if (opt_r) {
		read_file();
		exit(0);
	}
	if (opt_w) {
		write_file();
		exit(0);
	}
	if (opt_d) {
		delete_file();
		exit(0);
	}
	if (opt_p) {
		modify_partition();
		exit(0);
	}

	if (!opt_q)
		display_vol();

	return 0;
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
	for (i = 0; i < 15; ++i)
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
init_volhdr(void)
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
	write_volhdr();
}

void
read_file(void)
{
	FILE *fp;
	int i;

	if (!opt_q)
		printf("Reading file %s\n", vfilename);
	for (i = 0; i < 15; ++i) {
		if (strncmp(vfilename, volhdr->voldir[i].name,
			    strlen(volhdr->voldir[i].name)) == 0)
			break;
	}
	if (i >= 15) {
		printf("file %s not found\n", vfilename);
		exit(1);
	}
	/* XXX assumes volume header starts at 0? */
	lseek(fd, be32toh(volhdr->voldir[i].block) * 512, SEEK_SET);
	fp = fopen(ufilename, "w");
	if (fp == NULL) {
		perror("open write");
		exit(1);
	}
	i = be32toh(volhdr->voldir[i].bytes);
	while (i > 0) {
		if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
			perror("read file");
			exit(1);
		}
		fwrite(buf, 1, i > sizeof(buf) ? sizeof(buf) : i, fp);
		i -= i > sizeof(buf) ? sizeof(buf) : i;
	}
	fclose(fp);
}

void
write_file(void)
{
	FILE *fp;
	int slot;
	size_t namelen;
	int block, i;
	struct stat st;
	char fbuf[512];

	if (!opt_q)
		printf("Writing file %s\n", ufilename);
	if (stat(ufilename, &st) < 0) {
		perror("stat");
		exit(1);
	}
	if (!opt_q)
		printf("File %s has %lld bytes\n", ufilename, st.st_size);
	slot = -1;
	for (i = 0; i < 15; ++i) {
		if (volhdr->voldir[i].name[0] == '\0' && slot < 0)
			slot = i;
		if (strcmp(vfilename, volhdr->voldir[i].name) == 0) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		printf("No directory space for file %s\n", vfilename);
		exit(1);
	}
	/* -w can overwrite, -a won't overwrite */
	if (be32toh(volhdr->voldir[slot].block) > 0) {
		if (!opt_q)
			printf("File %s exists, removing old file\n", 
				vfilename);
		volhdr->voldir[slot].name[0] = 0;
		volhdr->voldir[slot].block = volhdr->voldir[slot].bytes = 0;
	}
	if (st.st_size == 0) {
		printf("bad file size\n");
		exit(1);
	}
	/* XXX assumes volume header starts at 0? */
	block = allocate_space((int)st.st_size);
	if (block < 0) {
		printf("No space for file\n");
		exit(1);
	}

	/*
	 * Make sure the name in the volume header is max. 8 chars,
	 * NOT including NUL.
	 */
	namelen = strlen(vfilename);
	if (namelen > sizeof(volhdr->voldir[slot].name)) {
		printf("Warning: '%s' is too long for volume header, ",
		       vfilename);
		namelen = sizeof(volhdr->voldir[slot].name);
		printf("truncating to '%-8s'\n", vfilename);
	}

	/* Populate it w/ NULs */
	memset(volhdr->voldir[slot].name, 0,
	    sizeof(volhdr->voldir[slot].name));
	/* Then copy the name */
	memcpy(volhdr->voldir[slot].name, vfilename, namelen);

	volhdr->voldir[slot].block = htobe32(block);
	volhdr->voldir[slot].bytes = htobe32(st.st_size);

	write_volhdr();

	/* write the file itself */
	i = lseek(fd, block * 512, SEEK_SET);
	if (i < 0) {
		perror("lseek write");
		exit(1);
	}
	i = st.st_size;
	fp = fopen(ufilename, "r");
	while (i > 0) {
		fread(fbuf, 1, i > 512 ? 512 : i, fp);
		if (write(fd, fbuf, 512) != 512) {
			perror("write file");
			exit(1);
		}
		i -= i > 512 ? 512 : i;
	}
}

void
delete_file(void)
{
	int i;

	for (i = 0; i < 15; ++i) {
		if (strcmp(vfilename, volhdr->voldir[i].name) == 0) {
			break;
		}
	}
	if (i >= 15) {
		printf("File %s not found\n", vfilename);
		exit(1);
	}

	/* XXX: we don't compact the file space, so get fragmentation */
	volhdr->voldir[i].name[0] = '\0';
	volhdr->voldir[i].block = volhdr->voldir[i].bytes = 0;
	write_volhdr();
}

void
modify_partition(void)
{
	if (!opt_q)
		printf("Modify partition %d start %d length %d\n", 
			partno, partfirst, partblocks);
	if (partno < 0 || partno > 15) {
		printf("Invalid partition number: %d\n", partno);
		exit(1);
	}
	volhdr->partitions[partno].blocks = htobe32(partblocks);
	volhdr->partitions[partno].first = htobe32(partfirst);
	volhdr->partitions[partno].type = htobe32(parttype);
	write_volhdr();
}

void
write_volhdr(void)
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
		perror("write volhdr");
}

int
allocate_space(int size)
{
	int n, blocks;
	int first;

	blocks = (size + 511) / 512;
	first = 2;
	n = 0;
	while (n < 15) {
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
	printf("Usage:	sgivol [-qf] [-i] [-h vhsize] device\n"
	       "	sgivol [-qf] [-r vhfilename diskfilename] device\n"
	       "	sgivol [-qf] [-w vhfilename diskfilename] device\n"
	       "	sgivol [-qf] [-d vhfilename] device\n"
	       );
	exit(0);
}
