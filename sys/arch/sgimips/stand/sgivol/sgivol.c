/*	$NetBSD: sgivol.c,v 1.1 2001/11/20 18:35:22 soren Exp $	*/

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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <util.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/stat.h>

#define SGI_SIZE_VOLHDR	3135	/* XXX Irix: 2592, NetBSD: 3753 */

int	fd;
int	n;
int	opt_i;			/* Initialize volume header */
int	opt_r;			/* Read a file from volume header */
int	opt_w;			/* Write a file to volume header */
int	opt_d;			/* Delete a file from volume header */
int	opt_p;			/* Modify a partition */
char	*vfilename = "";
char	*ufilename = "";
int	partno, partfirst, partblocks, parttype;
struct sgilabel *volhdr;
long	*l;
long	checksum;

struct disklabel lbl;

unsigned char buf[512];

char *sgi_types[] = {
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

main(argc, argv)
	int	argc;
	char	*argv[];
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
		fd = open(buf, (opt_i | opt_w | opt_d | opt_p) ? O_RDWR : O_RDONLY);
		if (fd < 0) {
			perror("open");
			exit(1);
		}
	}
	n = read(fd, buf, sizeof(buf));
	if (n != sizeof(buf)) {
		perror("read volhdr");
		exit(1);
	}
	n = ioctl(fd, DIOCGDINFO, &lbl);
	if (n < 0) {
		perror("DIOCGDINFO");
		exit(1);
	}
	volhdr = (struct sgilabel *)buf;
	if (opt_i) {
		init_volhdr();
		exit(0);
	}
	if (volhdr->magic != SGILABEL_MAGIC) {
		printf("No SGI volumn header found, magic=%x\n", volhdr->magic);
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

display_vol()
{
	printf("disklabel shows %d sectors\n", lbl.d_secperunit);
	l = (long *)buf;
	checksum = 0;
	for (n = 0; n < 512 / 4; ++n)
		checksum += l[n];
	printf("checksum: %08x%s\n", checksum, checksum == 0 ? "" : " *ERROR*");
	printf("root part: %d\n", volhdr->root);
	printf("swap part: %d\n", volhdr->swap);
	printf("bootfile: %\n", volhdr->bootfile);
	/* volhdr->devparams[0..47] */
	printf("\nVolume header files:\n");
	for (n = 0; n < 15; ++n)
		if (volhdr->voldir[n].name[0])
			printf("%-8s offset %4d blocks, length %8d bytes (%d blocks)\n",
			    volhdr->voldir[n].name, volhdr->voldir[n].block,
			    volhdr->voldir[n].bytes, (volhdr->voldir[n].bytes + 511 ) / 512);
	printf("\nSGI partitions:\n");
	for (n = 0; n < MAXPARTITIONS; ++n)
		if (volhdr->partitions[n].blocks)
			printf("%2d:%c blocks %8d first %8d type %2d (%s)\n", n, n + 'a',
			    volhdr->partitions[n].blocks, volhdr->partitions[n].first,
			    volhdr->partitions[n].type,
			    volhdr->partitions[n].type > 13 ? "???" :
			    sgi_types[volhdr->partitions[n].type]);
}

init_volhdr()
{
	memset(buf, 0, sizeof(buf));
	volhdr->magic = SGILABEL_MAGIC;
	volhdr->root = 0;
	volhdr->swap = 1;
	strcpy(volhdr->bootfile, "netbsd");
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
	volhdr->partitions[0].type = SGI_PTYPE_EFS;
	write_volhdr();
}

read_file()
{
	FILE *fp;

	printf("Reading file %s\n", vfilename);
	for (n = 0; n < 15; ++n) {
		if (strcmp(vfilename, volhdr->voldir[n].name) == NULL)
			break;
	}
	if (n >= 15) {
		printf("file %s not found\n", vfilename);
		exit(1);
	}
	/* XXX assumes volume header starts at 0? */
	lseek(fd, volhdr->voldir[n].block * 512, SEEK_SET);
	fp = fopen(ufilename, "w");
	if (fp == NULL) {
		perror("open write");
		exit(1);
	}
	n = volhdr->voldir[n].bytes;
	while (n > 0) {
		if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
			perror("read file");
			exit(1);
		}
		fwrite(buf, 1, n > sizeof(buf) ? sizeof(buf) : n, fp);
		n -= n > sizeof(buf) ? sizeof(buf) : n;
	}
	fclose(fp);
}

write_file()
{
	FILE *fp;
	int fileno;
	int block, bytes;
	struct stat st;
	char fbuf[512];

	printf("Writing file %s\n", ufilename);
	if (stat(ufilename, &st) < 0) {
		perror("stat");
		exit(1);
	}
	printf("File %s has %lld bytes\n", ufilename, st.st_size);
	fileno = -1;
	for (n = 0; n < 15; ++n) {
		if (volhdr->voldir[n].name[0] == '\0' && fileno < 0)
			fileno = n;
		if (strcmp(vfilename, volhdr->voldir[n].name) == 0) {
			fileno = n;
			break;
		}
	}
	if (fileno == -1) {
		printf("No directory space for file %\n", vfilename);
		exit(1);
	}
	/* -w can overwrite, -a won't overwrite */
	if (volhdr->voldir[fileno].block > 0) {
		printf("File %s exists, removing old file\n", vfilename);
		volhdr->voldir[fileno].name[0] = 0;
		volhdr->voldir[fileno].block = volhdr->voldir[fileno].bytes = 0;
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

	/* Make sure name in volume header is max. 8 chars including NUL */
	if (strlen(vfilename) > sizeof(volhdr->voldir[fileno].name) - 1) {
		printf("Warning: '%s' is too long for volume header, ",
		       vfilename);
		vfilename[sizeof(volhdr->voldir[fileno].name) - 1] = '\0';
		printf("truncating to '%s'\n", vfilename);
	}
	
	strcpy(volhdr->voldir[fileno].name, vfilename);
	volhdr->voldir[fileno].block = block;
	volhdr->voldir[fileno].bytes = st.st_size;

	write_volhdr();

	/* write the file itself */
	n = lseek(fd, block * 512, SEEK_SET);
	if (n < 0) {
		perror("lseek write");
		exit(1);
	}
	n = st.st_size;
	fp = fopen(ufilename, "r");
	while (n > 0) {
		fread(fbuf, 1, n > 512 ? 512 : n, fp);
		if (write(fd, fbuf, 512) != 512) {
			perror("write file");
			exit(1);
		}
		n -= n > 512 ? 512 : n;
	}
}

delete_file()
{
	for (n = 0; n < 15; ++n) {
		if (strcmp(vfilename, volhdr->voldir[n].name) == NULL) {
			break;
		}
	}
	if (n >= 15) {
		printf("File %s not found\n", vfilename);
		exit(1);
	}
	volhdr->voldir[n].name[0] = '\0';
	volhdr->voldir[n].block = volhdr->voldir[n].bytes = 0;
	write_volhdr();
}

modify_partition()
{
	printf("Modify partition %d start %d length %d\n", partno, partfirst, partblocks);
	if (partno < 0 || partno > 15) {
		printf("Invalue partition number: %d\n", partno);
		exit(1);
	}
	volhdr->partitions[partno].blocks = partblocks;
	volhdr->partitions[partno].first = partfirst;
	volhdr->partitions[partno].type = parttype;
	write_volhdr();
}

write_volhdr()
{
	checksum_vol();
	display_vol();
	printf("\nDo you want to update volume (y/n)? ");
	n = getchar();
	if (n != 'Y' && n != 'y')
		exit(1);
	n = lseek(fd, 0 , SEEK_SET);
	if (n < 0) {
		perror("lseek 0");
		exit(1);
	}
	n = write(fd, buf, 512);
	if (n < 0)
		perror("write volhdr");
}

/*
 */
allocate_space(size)
	int size;
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

checksum_vol()
{
	volhdr->checksum = checksum = 0;
	l = (long *)buf;
	for (n = 0; n < 512 / 4; ++n)
		checksum += l[n];
	volhdr->checksum = -checksum;
}

usage()
{
	printf("Usage:  sgivol [-i] device\n"
	       "        sgivol [-r vhfilename diskfilename] device\n"
	       "        sgivol [-w vhfilename diskfilename] device\n"
	       "        sgivol [-d vhfilename] device\n"
	       );
	exit(0);
}
