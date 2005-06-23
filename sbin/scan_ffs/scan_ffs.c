/*	$NetBSD: scan_ffs.c,v 1.4 2005/06/23 17:25:31 xtraeme Exp $	*/
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
__RCSID("$NetBSD: scan_ffs.c,v 1.4 2005/06/23 17:25:31 xtraeme Exp $");
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

static int 	eflag = 0;
static int	flags = 0;

static daddr_t	blk, lastblk;

static struct fs *sb;

static const char *fstypes[] = { "NONE", "FFSv1", "FFSv2" };

#define FSTYPE_NONE	0
#define FSTYPE_FFSV1	1
#define FSTYPE_FFSV2	2

static int
checkfstype(void)
{
	switch (sb->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			sb->fs_size = sb->fs_old_size;
			return FSTYPE_FFSV1;
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			return FSTYPE_FFSV2;
		default:
			return FSTYPE_NONE;
	}
}

static void
printpart(int flag, int ffsize, int n)
{
	
	int fstype = checkfstype();

	switch (flag) {
	case VERBOSE:
		(void)printf("block: %" PRIu64 "id %x,%x size %" PRIu64 "\n",
		    blk + (n / 512), sb->fs_id[0], sb->fs_id[1], sb->fs_size);
		break;
	case LABELS:
		(void)printf("X:  %9" PRIu64,
		    (uint64_t)((off_t)sb->fs_size * sb->fs_fsize / 512));
		switch (fstype) {
		case FSTYPE_FFSV1:
			(void)printf(" %9" PRIu64,
			    blk + (n / 512) - (2 * SBLOCKSIZE / 512));
			break;
		case FSTYPE_FFSV2:
			(void)printf(" %9" PRIu64,
			    blk + (n / 512) - 
			    (ffsize * SBLOCKSIZE / 512 + 128));
			break;
		default:
			break;
		}
		(void)printf(" 4.2BSD %6d %5d%4d # %s [%s]\n",
			sb->fs_fsize, sb->fs_bsize,
			sb->fs_old_cpg, lastmount, fstypes[fstype]);
		break;
	default:
		printf("%s ", fstypes[fstype]);
		switch (fstype) {
		case FSTYPE_FFSV1:
			(void)printf("at %" PRIu64,
			    blk + (n / 512) - (2 * SBLOCKSIZE / 512));
			break;
		case FSTYPE_FFSV2:
			(void)printf("at %" PRIu64,
			    blk + (n / 512) -
			    (ffsize * SBLOCKSIZE / 512 + 128));
			break;
		default:
			break;
		}
		(void)printf(" size %" PRIu64 ", last mounted on %s\n",
		    (uint64_t)((off_t)sb->fs_size * sb->fs_fsize / 512),
		    lastmount);
		break;
	}
}

static void
ufsmagic(int n)
{
	int fstype = checkfstype();
	size_t i;

	/* 
	 * FIXME:
	 * It cannot find FFSv1 partitions with fsize/bsize > 2048/16384,
	 * same problem found in the original program that comes from
	 * OpenBSD (scan_ffs(1)).
	 */
	if (flags & VERBOSE)
		printpart(VERBOSE, NADA, n);
	switch (fstype) {
	case FSTYPE_FFSV1:
		if (((blk + (n / 512)) - lastblk) == (SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				printpart(LABELS, NADA, n);
			else
				printpart(NADA, NADA, n);
		}
		break;
	case FSTYPE_FFSV2:
		/*
		 * That checks for FFSv2 partitions with fragsize/blocksize:
		 * 512/4096, 1024/8192, 2048/16384, 4096/32768 and 8192/65536.
		 * Really enough for now.
		 */
		for (i = 1; i < 16; i <<= 1)
			if (((blk + (n / 512)) - lastblk) ==
			    (i * SBLOCKSIZE / 512)) {
				if (flags & LABELS)
					printpart(LABELS, i, n);
				else
					printpart(NADA, i, n);
			}
	}
}

static int
ufsscan(int fd, daddr_t beg, daddr_t end, int fflags)
{

	u_int8_t buf[SBLOCKSIZE * SBCOUNT];
	int n, fstype;

	lastblk = -1;
	(void)memset(lastmount, 0, MAXMNTLEN);

	if (fflags & LABELS)
		(void)printf(
		    "#        size    offset fstype [fsize bsize cpg]\n");

	for (blk = beg; blk <= ((end < 0) ? blk: end);
		blk += (SBCOUNT * SBLOCKSIZE / 512)) {
		(void)memset(buf, 0, sizeof(buf));

		if (lseek(fd, (off_t)blk * 512, SEEK_SET) == (off_t)-1)
			err(1, "lseek");

		if (read(fd, buf, sizeof(buf)) == -1)
			err(1, "read");

		for (n = 0; n < (SBLOCKSIZE * SBCOUNT); n += 512) {
			sb = (struct fs *)(void *)&buf[n];
			if ((fstype = checkfstype()) == FSTYPE_NONE)
				continue;
			ufsmagic(n);
			/* Update last potential FS SBs seen */
			lastblk = blk + (n / 512);
			(void)memcpy(lastmount, sb->fs_fsmnt, MAXMNTLEN);
		}
	}
	return EXIT_SUCCESS;
}


static void
usage(void)
{
	(void)fprintf(stderr,
		"Usage: %s [-lv] [-s start] [-e end] device\n", getprogname());
	exit(EXIT_FAILURE);
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

	if (fd == -1)
		err(1, "Cannot open `%s'", device);
		/* NOTREACHED */

	if (ioctl(fd, DIOCGDINFO, &dl) == -1) {
		warn("Couldn't retrieve disklabel");
		(void)memset(&dl, 0, sizeof(dl));
		dl.d_secperunit = 0x7fffffff;
	} else {
		(void)printf("Disk: %s\n", dl.d_typename);
		(void)printf("Total sectors on disk: %" PRIu32 "\n\n",
		    dl.d_secperunit);
	}
	
	if (!eflag)
		end = dl.d_secperunit; /* default to max sectors */

	return ufsscan(fd, beg, end, flags);
}
