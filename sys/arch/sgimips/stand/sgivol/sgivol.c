/*	$NetBSD: sgivol.c,v 1.3.2.3 2002/03/16 15:59:34 jdolecek Exp $	*/

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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <util.h>

#define SGI_SIZE_VOLHDR	3135	/* XXX Irix: 2592, NetBSD: 3753 */

int	fd;
int	opt_i;			/* Initialize volume header */
int	opt_r;			/* Read a file from volume header */
int	opt_w;			/* Write a file to volume header */
int	opt_d;			/* Delete a file from volume header */
int	opt_p;			/* Modify a partition */
int	partno, partfirst, partblocks, parttype;
struct sgilabel *volhdr;
int32_t	checksum;

const char *vfilename = "";
const char *ufilename = "";

struct disklabel lbl;

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
	if (argc < 2)
		usage();

	if (argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'i':
			++opt_i;
			argv++;
			argc--;
			break;
		case 'r':
		case 'w':
			if (argc < 4)
				usage();
			if (argv[1][1] == 'r')
				++opt_r;
			else
				++opt_w;
			vfilename = argv[2];
			ufilename = argv[3];
			argv += 3;
			argc -= 3;
			break;
		case 'd':
			if (argc < 3)
				usage();
			++opt_d;
			vfilename = argv[2];
			argv += 2;
			argc -= 2;
				break;
		case 'p':
			if (argc < 6)
				usage();
			++opt_p;
			partno = atoi(argv[2]);
			partfirst = atoi(argv[3]);
			partblocks = atoi(argv[4]);
			parttype = atoi(argv[5]);
			argv += 5;
			argc -= 5;
			break;
		default:
			printf("-%c Invalid\n", argv[1][1]);
			usage();
		}
	}

	if (argc < 2)
		usage();

	fd = open(argv[1], (opt_i | opt_w | opt_d | opt_p) ? O_RDWR : O_RDONLY);
	if (fd < 0) {
		sprintf(buf, "/dev/r%s%c", argv[1], 'a' + getrawpartition());
		fd = open(buf,
		    (opt_i | opt_w | opt_d | opt_p) ? O_RDWR : O_RDONLY);
		if (fd < 0) {
			perror("open");
			exit(1);
		}
	}
	if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
		perror("read volhdr");
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &lbl) < 0) {
		perror("DIOCGDINFO");
		exit(1);
	}
	volhdr = (struct sgilabel *)buf;
	if (opt_i) {
		init_volhdr();
		exit(0);
	}
	if (volhdr->magic != SGILABEL_MAGIC) {
		printf("No SGI volume header found, magic=%x\n", volhdr->magic);
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
	display_vol();

	return 0;
}

void
display_vol(void)
{
	int32_t *l;
	int i;

	printf("disklabel shows %d sectors\n", lbl.d_secperunit);
	l = (int32_t *)buf;
	checksum = 0;
	for (i = 0; i < 512 / 4; ++i)
		checksum += l[i];
	printf("checksum: %08x%s\n", checksum, checksum == 0 ? "" : " *ERROR*");
	printf("root part: %d\n", volhdr->root);
	printf("swap part: %d\n", volhdr->swap);
	printf("bootfile: %s\n", volhdr->bootfile);
	/* volhdr->devparams[0..47] */
	printf("\nVolume header files:\n");
	for (i = 0; i < 15; ++i)
		if (volhdr->voldir[i].name[0])
			printf("%-8s offset %4d blocks, length %8d bytes (%d blocks)\n",
			    volhdr->voldir[i].name, volhdr->voldir[i].block,
			    volhdr->voldir[i].bytes, (volhdr->voldir[i].bytes + 511 ) / 512);
	printf("\nSGI partitions:\n");
	for (i = 0; i < MAXPARTITIONS; ++i) {
		if (volhdr->partitions[i].blocks) {
			printf("%2d:%c blocks %8d first %8d type %2d (%s)\n",
			    i, i + 'a', volhdr->partitions[i].blocks,
			    volhdr->partitions[i].first,
			    volhdr->partitions[i].type,
			    volhdr->partitions[i].type > 13 ? "???" :
			    sgi_types[volhdr->partitions[i].type]);
		}
	}
}

void
init_volhdr(void)
{
	memset(buf, 0, sizeof(buf));
	volhdr->magic = SGILABEL_MAGIC;
	volhdr->root = 0;
	volhdr->swap = 1;
	strcpy(volhdr->bootfile, "/netbsd");
	volhdr->dp.dp_skew = lbl.d_trackskew;
	volhdr->dp.dp_gap1 = 1; /* XXX */
	volhdr->dp.dp_gap2 = 1; /* XXX */
	volhdr->dp.dp_cyls = lbl.d_ncylinders;
	volhdr->dp.dp_shd0 = 0;
	volhdr->dp.dp_trks0 = lbl.d_ntracks;
	volhdr->dp.dp_secs = lbl.d_nsectors;
	volhdr->dp.dp_secbytes = lbl.d_secsize;
	volhdr->dp.dp_interleave = lbl.d_interleave;
	volhdr->dp.dp_nretries = 22;
	volhdr->partitions[10].blocks = lbl.d_secperunit;
	volhdr->partitions[10].first = 0;
	volhdr->partitions[10].type = SGI_PTYPE_VOLUME;
	volhdr->partitions[8].blocks = SGI_SIZE_VOLHDR;
	volhdr->partitions[8].first = 0;
	volhdr->partitions[8].type = SGI_PTYPE_VOLHDR;
	volhdr->partitions[0].blocks = lbl.d_secperunit - SGI_SIZE_VOLHDR;
	volhdr->partitions[0].first = SGI_SIZE_VOLHDR;
	volhdr->partitions[0].type = SGI_PTYPE_BSD;
	write_volhdr();
}

void
read_file(void)
{
	FILE *fp;
	int i;

	printf("Reading file %s\n", vfilename);
	for (i = 0; i < 15; ++i) {
		if (strncmp(vfilename, volhdr->voldir[i].name,
		    sizeof(volhdr->voldir[i].name)) == NULL)
			break;
	}
	if (i >= 15) {
		printf("file %s not found\n", vfilename);
		exit(1);
	}
	/* XXX assumes volume header starts at 0? */
	lseek(fd, volhdr->voldir[i].block * 512, SEEK_SET);
	fp = fopen(ufilename, "w");
	if (fp == NULL) {
		perror("open write");
		exit(1);
	}
	i = volhdr->voldir[i].bytes;
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

	printf("Writing file %s\n", ufilename);
	if (stat(ufilename, &st) < 0) {
		perror("stat");
		exit(1);
	}
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
	if (volhdr->voldir[slot].block > 0) {
		printf("File %s exists, removing old file\n", vfilename);
		volhdr->voldir[slot].name[0] = 0;
		volhdr->voldir[slot].block = volhdr->voldir[slot].bytes = 0;
	}
	if (st.st_size == 0) {
		printf("bad file size?\n");
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

	volhdr->voldir[slot].block = block;
	volhdr->voldir[slot].bytes = st.st_size;

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
		if (strcmp(vfilename, volhdr->voldir[i].name) == NULL) {
			break;
		}
	}
	if (i >= 15) {
		printf("File %s not found\n", vfilename);
		exit(1);
	}
	volhdr->voldir[i].name[0] = '\0';
	volhdr->voldir[i].block = volhdr->voldir[i].bytes = 0;
	write_volhdr();
}

void
modify_partition(void)
{
	printf("Modify partition %d start %d length %d\n", partno, partfirst,
	    partblocks);
	if (partno < 0 || partno > 15) {
		printf("Invalue partition number: %d\n", partno);
		exit(1);
	}
	volhdr->partitions[partno].blocks = partblocks;
	volhdr->partitions[partno].first = partfirst;
	volhdr->partitions[partno].type = parttype;
	write_volhdr();
}

void
write_volhdr(void)
{
	int i;

	checksum_vol();
	display_vol();
	printf("\nDo you want to update volume (y/n)? ");
	i = getchar();
	if (i != 'Y' && i != 'y')
		exit(1);
	i = lseek(fd, 0 , SEEK_SET);
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
			if (first < (volhdr->voldir[n].block +
			    (volhdr->voldir[n].bytes + 511) / 512) &&
			    (first + blocks) >= volhdr->voldir[n].block) {
				first = volhdr->voldir[n].block +
				    (volhdr->voldir[n].bytes + 511) / 512;
#if 0
printf("allocate: n=%d first=%d blocks=%d size=%d\n", n, first, blocks, size);
printf("%s %d %d\n", volhdr->voldir[n].name, volhdr->voldir[n].block, volhdr->voldir[n].bytes);
printf("first=%d block=%d last=%d end=%d\n", first, volhdr->voldir[n].block,
    first + blocks - 1, volhdr->voldir[n].block + (volhdr->voldir[n].bytes + 511)/512);
#endif
				n = 0;
				continue;
			}
		}
		++n;
	}
	if (first + blocks > lbl.d_secperunit)
		first = -1;
	/* XXX assumes volume header is partition 8 */
	/* XXX assumes volume header starts at 0? */
	if (first + blocks >= volhdr->partitions[8].blocks)
		first = -1;
	return(first);
}

void
checksum_vol(void)
{
	int32_t *l;
	int i;

	volhdr->checksum = checksum = 0;
	l = (int32_t *)buf;
	for (i = 0; i < 512 / 4; ++i)
		checksum += l[i];
	volhdr->checksum = -checksum;
}

void
usage(void)
{
	printf("Usage:	sgivol [-i] device\n"
	       "	sgivol [-r vhfilename diskfilename] device\n"
	       "	sgivol [-w vhfilename diskfilename] device\n"
	       "	sgivol [-d vhfilename] device\n"
	       );
	exit(0);
}
