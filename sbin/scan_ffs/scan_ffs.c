/*	$NetBSD: scan_ffs.c,v 1.1 2005/06/15 18:06:19 xtraeme Exp $	*/
/*	$OpenBSD: scan_ffs.c,v 1.11 2004/02/16 19:13:03 deraadt Exp$	*/

/*
 * Copyright (c) 2005 Juan Romero Pardines
 * Copyright (c) 1998 Niklas Hallqvist, Tobias Weingartner
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

/*
 * Currently it can detect:
 * 	o FFSv1 with fragsize/blocksize: 512/4096, 1024/8192, 2048/16384.
 *	o FFSv2 with fragsize/blocksize: 512/4096, 1024/8192, 2048/16384,
 *	  				 4096/32768, 8192/65536.
 * TODO:
 *	o Detect FFSv1 partitions with fsize/bsize > 2048/16384.
 *	o Detect FFSv2 partitions with fsize/bsize > 8192/65536.
 */
 
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: scan_ffs.c,v 1.1 2005/06/15 18:06:19 xtraeme Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>

#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <ufs/ffs/fs.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <util.h>

enum { NADA, VERBOSE, LABELS };

#define SBCOUNT	64 /* XXX should be configurable */

static void	printpart(int, int, int);
static void	ufsmagic(int);
static void	usage(void) __attribute__((__noreturn__));
static int	checkfstype(void);
static int	ufsscan(int, daddr_t, daddr_t, int);

static char	lastmount[MAXMNTLEN];
static char	device[MAXPATHLEN];
static const	char *fstype = NULL;

static int 	eflag = 0;
static int	flags = 0;

static daddr_t	blk, lastblk;

static struct fs *sb;

static int
checkfstype(void)
{
	switch (sb->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			sb->fs_size = sb->fs_old_size;
			fstype = "FFSv1";
			return 1;
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			fstype = "FFSv2";
			return 2;
		default:
			return -1;
	}
}

static void
printpart(int flag, int ffsize, int n)
{
	
	int fsrv = checkfstype();

	if (flag == VERBOSE) {
		(void)printf("block: %" PRIu64" "
			"id %x,%x size %" PRIu64"\n",
			blk + (n / 512), sb->fs_id[0],
			sb->fs_id[1], sb->fs_size);
	} else if (flag == LABELS) {
		(void)printf("X:  %9" PRIu64 "",
			(uint64_t)((off_t)sb->fs_size * sb->fs_fsize / 512));
		if (fsrv == 1)	/* FFSv1 */
			(void)printf(" %9" PRIu64 "",
				blk + (n / 512)-(2 * SBLOCKSIZE / 512));
		else if (fsrv == 2)	/* FFSv2 */
			(void)printf(" %9" PRIu64 "",
				blk + (n / 512)-(ffsize * SBLOCKSIZE / 512 + 128));
		(void)printf(" 4.2BSD %6d %5d%4d # %s [%s]\n",
			sb->fs_fsize, sb->fs_bsize,
			sb->fs_old_cpg, lastmount, fstype);
	} else {
		printf("%s ", fstype);
		if (fsrv == 1)	/* FFSv1 */
			(void)printf("at %" PRIu64 "",
				blk + (n / 512) - (2 * SBLOCKSIZE / 512));
		else if (fsrv == 2)	/* FFSv2 */
			(void)printf("at %" PRIu64 "",
				blk + (n / 512) - (ffsize * SBLOCKSIZE / 512 + 128));
		(void)printf(" size %" PRIu64 ", last mounted on %s\n",
			(uint64_t)((off_t)sb->fs_size * sb->fs_fsize / 512),
			lastmount);
	}
}

static void
ufsmagic(int n)
{
	int fsrv = checkfstype();

	/* 
	 * FIXME:
	 * It cannot find FFSv1 partitions with fsize/bsize > 2048/16384,
	 * same problem found in the original program that comes from
	 * OpenBSD (scan_ffs(1)).
	 */
	if (flags & VERBOSE)
		printpart(VERBOSE, NADA, n);
	if (fsrv == 1) {	/* FFSv1 */
		if (((blk + (n / 512)) - lastblk) == (SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, NADA, n);
			else
				printpart(NADA, NADA, n);
		}
	} else if (fsrv == 2) {	/* FFSv2 */
	/*
	 * That checks for FFSv2 partitions with fragsize/blocksize:
	 * 512/4096, 1024/8192, 2048/16384, 4096/32768 and 8192/65536.
	 * Really enough for now.
	 */
		if (((blk + (n / 512)) - lastblk) == (SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, 1, n);
			else
				printpart(NADA, 1, n);
		} else if (((blk + (n / 512)) - lastblk) == (2 * SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, 2, n);
			else
				printpart(NADA, 2, n);
		} else if (((blk + (n / 512)) - lastblk) == (4 * SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, 4, n);
			else
				printpart(NADA, 4, n);
		} else if (((blk + (n / 512)) - lastblk) == (8 * SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, 8, n);
			else
				printpart(NADA, 8, n);
		}
	}
}

static int
ufsscan(int fd, daddr_t beg, daddr_t end, int fflags)
{

	u_int8_t buf[SBLOCKSIZE * SBCOUNT];
	int n, fsrv;

	lastblk = -1;
	memset(lastmount, 0, MAXMNTLEN);

	if (fflags & LABELS)
		(void)printf("#        size    offset fstype [fsize bsize cpg]\n");

	for (blk = beg; blk <= ((end < 0) ? blk: end);
		blk += (SBCOUNT * SBLOCKSIZE / 512)) {
		memset(buf, 0, sizeof(buf));

		if (lseek(fd, (off_t)blk * 512, SEEK_SET) < 0)
			err(1, "lseek");
			/* NOTREACHED */

		if (read(fd, buf, sizeof(buf)) < 0)
			err(1, "read");
			/* NOTREACHED */

		for (n = 0; n < (SBLOCKSIZE * SBCOUNT); n += 512) {
			sb = (struct fs*)(&buf[n]);
			fsrv = checkfstype();

			if (fsrv >= 1) { /* found! */
				ufsmagic(n);
				/* Update last potential FS SBs seen */
				lastblk = blk + (n / 512);
				memcpy(lastmount, sb->fs_fsmnt, MAXMNTLEN);
			}
		}
	}
	return 0;
}


static void
usage(void)
{
	(void)fprintf(stderr,
		"usage: %s [-lv] [-s start] [-e end] device", getprogname());
	exit(1);
}


int
main(int argc, char **argv)
{
	int ch, fd;
	daddr_t end = -1, beg = 0;
	struct disklabel dl;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "e:ls:v")) != -1)
		switch(ch) {
		case 'e':
			eflag = 1;
			end = atoi(optarg);
			break;
		case 'l':
			flags |= LABELS;
			break;
		case 's':
			beg = atoi(optarg);
			break;
		case 'v':
			flags |= VERBOSE;
			break;
		default:
			usage();
			/* NOTREACHED */
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	fd = opendisk(argv[0], O_RDONLY, device, sizeof(device), 0);

	if (fd < 0)
		err(1, "%s", device);
		/* NOTREACHED */

	if (ioctl(fd, DIOCGDINFO, &dl) == -1)
		warn("couldn't retrieve disklabel.\n");
	else {
		(void)printf("Disk: %s\n", dl.d_typename);
		(void)printf("Total sectors on disk: %" PRIu32 "\n\n",
			dl.d_secperunit);
	}
	
	if (!eflag)
		end = dl.d_secperunit; /* default to max sectors */

	return (ufsscan(fd, beg, end, flags));
}
