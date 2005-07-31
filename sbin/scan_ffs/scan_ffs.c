/* $NetBSD: scan_ffs.c,v 1.6 2005/07/31 20:19:40 christos Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
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
 *      This product includes software developed by Juan Romero Pardines
 *      for the NetBSD Foundation, Inc. and its contributors.
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

/*
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
 * 	o FFSv1 fsize/bsize: 512/4096, 1024/8192, 2048/16384.
 *	o FFSv2 fsize/bsize: 512/4096, 1024/8192, 2048/16384,
 *	  		     4096/32768, 8192/65536.
 *	o LFSv[12] fsize/bsize: 512/4096, 1024/8192, 2048/16384,
 *				4096/32768, 8192/65536.
 *
 * TODO:
 *	o Detect FFSv1 partitions with fsize/bsize > 2048/16384.
 *
 *	-- xtraeme --
 */
 
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: scan_ffs.c,v 1.6 2005/07/31 20:19:40 christos Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/queue.h>
#include <sys/mount.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>
#include <ufs/ffs/fs.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <util.h>

/* common struct for FFS/LFS */
struct sblockinfo {
	struct lfs	*lfs;
	struct fs	*ffs;
	u_int64_t	lfs_off;
	u_int64_t	ffs_off;
	char		lfs_path[MAXMNTLEN];
	char		ffs_path[MAXMNTLEN];
} sbinfo;

static daddr_t	blk, lastblk;

static int	eflag = 0;
static int	flags = 0;
static int 	sbaddr = 0; /* counter for the LFS superblocks */

static char	device[MAXPATHLEN];
static const char *fstypes[] = { "NONE", "FFSv1", "FFSv2", "LFS" };

#define FSTYPE_NONE	0
#define FSTYPE_FFSV1	1
#define FSTYPE_FFSV2	2

#define SBCOUNT		64 /* may be changed */
#define SBPASS		(SBCOUNT * SBLOCKSIZE / 512)

/* This is only useful for LFS */

/* first sblock address contains the correct offset */
#define FIRST_SBLOCK_ADDRESS    1 
/* second and third sblock address contain lfs_fsmnt[MAXMNTLEN] */
#define SECOND_SBLOCK_ADDRESS   2
/* last sblock address in a LFS partition */
#define MAX_SBLOCK_ADDRESS      10

enum { NADA, VERBOSE, LABELS };

/* FFS functions */
static void	ffs_printpart(int, size_t, int);
static void	ffs_scan(int);
static int	ffs_checkver(void);
/* LFS functions */
static void	lfs_printpart(int, int, struct sblockinfo *);
static void	lfs_scan(int);
/* common functions */
static void	usage(void) __attribute__((__noreturn__));
static int	scan_disk(int, daddr_t, daddr_t, int);

static int
ffs_checkver(void)
{
	switch (sbinfo.ffs->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			sbinfo.ffs->fs_size = sbinfo.ffs->fs_old_size;
			return FSTYPE_FFSV1;
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			return FSTYPE_FFSV2;
		default:
			return FSTYPE_NONE;
	}
}

static void
ffs_printpart(int flag, size_t ffsize, int n)
{
	
	int fstype = ffs_checkver();

	switch (flag) {
	case VERBOSE:
		(void)printf("block: %" PRIu64 " id %x,%x size %" PRIu64 "\n",
			blk + (n / 512),
			sbinfo.ffs->fs_id[0],
			sbinfo.ffs->fs_id[1], sbinfo.ffs->fs_size);
		break;
	case LABELS:
		(void)printf("X:  %9" PRIu64,
			(uint64_t)((off_t)sbinfo.ffs->fs_size *
			sbinfo.ffs->fs_fsize / 512));
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
		(void)printf(" 4.2BSD %6d %5d %8d # %s [%s]\n",
			sbinfo.ffs->fs_fsize, sbinfo.ffs->fs_bsize,
			sbinfo.ffs->fs_old_cpg, 
			sbinfo.ffs_path, fstypes[fstype]);
		break;
	default:
		(void)printf("%s ", fstypes[fstype]);
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
			(uint64_t)((off_t)sbinfo.ffs->fs_size *
			sbinfo.ffs->fs_fsize / 512), sbinfo.ffs_path);
		break;
	}
}

static void
ffs_scan(int n)
{
	int fstype = ffs_checkver();
	size_t i;

	/* 
	 * XXX:
	 * It cannot find FFSv1 partitions with fsize/bsize > 2048/16384,
	 * same problem found in the original program that comes from
	 * OpenBSD (scan_ffs(8)).
	 */
	if (flags & VERBOSE)
		ffs_printpart(VERBOSE, NADA, n);
	switch (fstype) {
	case FSTYPE_FFSV1:
		if (((blk + (n / 512)) - lastblk) == (SBLOCKSIZE / 512)) {
			if (flags & LABELS)
				ffs_printpart(LABELS, NADA, n);
			else
				ffs_printpart(NADA, NADA, n);
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
					ffs_printpart(LABELS, i, n);
				else
					ffs_printpart(NADA, i, n);
			}
		break;
	}
}

static void
lfs_printpart(int flag, int n, struct sblockinfo *sbi)
{
	if (flags & VERBOSE)
               	(void)printf("block: %" PRIu64 " size %" PRIu32 "\n",
                	blk + (n / 512), sbi->lfs->lfs_size);
	switch (flag) {
	case LABELS:
		(void)printf("X:  %9" PRIu64,
               		(uint64_t)((off_t)sbi->lfs->lfs_size *
               		sbi->lfs->lfs_fsize / 512));
		(void)printf(" %9" PRIu64, sbi->lfs_off); 
		(void)printf(" 4.4LFS %6d %5d %8d # %s [LFSv%d]\n",
			sbi->lfs->lfs_fsize, sbi->lfs->lfs_bsize,
			sbi->lfs->lfs_nseg, sbi->lfs_path, 
			sbi->lfs->lfs_version);
		break;
	default:
		(void)printf("LFSv%d ", sbinfo.lfs->lfs_version);
		(void)printf("at %" PRIu64, sbinfo.lfs_off);
		(void)printf(" size %" PRIu64 ", last mounted on %s\n",
			(uint64_t)((off_t)sbinfo.lfs->lfs_size *
			sbinfo.lfs->lfs_fsize / 512), sbinfo.lfs_path);
		break;
	}
}

static void
lfs_scan(int n)
{
	/* backup offset */
	lastblk = blk + (n / 512) - (LFS_SBPAD / 512);
	/* increment counter */
        ++sbaddr;

	switch (sbaddr) {
	/*
	 * first superblock contains the right offset, but lfs_fsmnt is
	 * empty... afortunately the next superblock address has it.
	 */
	case FIRST_SBLOCK_ADDRESS:
		/* copy partition offset */
		if (sbinfo.lfs_off != lastblk)
			sbinfo.lfs_off = blk + (n / 512) - (LFS_SBPAD / 512);
		break;
	case SECOND_SBLOCK_ADDRESS:
		/* copy the path of last mount */
		(void)memcpy(sbinfo.lfs_path,
			sbinfo.lfs->lfs_fsmnt, MAXMNTLEN);
		/* print now that we have the info */
		if (flags & LABELS)
			lfs_printpart(LABELS, n, &sbinfo);
		else
			lfs_printpart(NADA, n, &sbinfo);
		/* clear our struct */
		(void)memset(&sbinfo, 0, sizeof(sbinfo));
		break;
	case MAX_SBLOCK_ADDRESS:
		/*
		 * reset the counter, this is the last superblock address,
		 * the next one will be another partition maybe.
		 */
		sbaddr = 0;
		break;
	default:
		break;
	}
}

static int
scan_disk(int fd, daddr_t beg, daddr_t end, int fflags)
{
	u_int8_t buf[SBLOCKSIZE * SBCOUNT];
	int n, fstype;

	n = fstype = 0;
	lastblk = -1;

	/* clear our struct before using it */
	(void)memset(&sbinfo, 0, sizeof(sbinfo));

	if (fflags & LABELS)
		(void)printf(
		    "#        size    offset fstype [fsize bsize cpg/sgs]\n");

	for (blk = beg; blk <= ((end < 0) ? blk: end); blk += SBPASS) {
		(void)memset(buf, 0, sizeof(buf));

		if (lseek(fd, (off_t)blk * 512, SEEK_SET) == (off_t)-1)
			err(1, "lseek");

		if (read(fd, buf, sizeof(buf)) == -1)
			err(1, "read");

		for (n = 0; n < (SBLOCKSIZE * SBCOUNT); n += 512) {
			sbinfo.ffs = (struct fs *)(void *)&buf[n];
			sbinfo.lfs = (struct lfs *)(void *)&buf[n];
			fstype = ffs_checkver();
			switch (fstype) {
			case FSTYPE_FFSV1:
			case FSTYPE_FFSV2:
				ffs_scan(n);
				lastblk = blk + (n / 512);
				(void)memcpy(sbinfo.ffs_path,
					sbinfo.ffs->fs_fsmnt, MAXMNTLEN);
				break;
			case FSTYPE_NONE:
				/* maybe LFS? */
				if (sbinfo.lfs->lfs_magic != LFS_MAGIC)
					break;
				if (sbinfo.lfs->lfs_magic == LFS_MAGIC)
					lfs_scan(n);
				break;
			default:
				break;
			}
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

	setprogname(*argv);
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

	return scan_disk(fd, beg, end, flags);
}
