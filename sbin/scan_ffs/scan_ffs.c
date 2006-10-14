/* $NetBSD: scan_ffs.c,v 1.14 2006/10/14 13:22:34 xtraeme Exp $ */

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
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
 * Currently it can detect FFS and LFS partitions (version 1 or 2)
 * up to 8192/65536 fragsize/blocksize.
 */
 
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: scan_ffs.c,v 1.14 2006/10/14 13:22:34 xtraeme Exp $");
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

/* Undefine macros defined by both lfs/lfs.h and ffs/fs.h */
#undef fsbtodb
#undef dbtofsb
#undef blkoff
#undef fragoff
#undef lblktosize
#undef lblkno
#undef numfrags
#undef blkroundup
#undef fragroundup
#undef fragstoblks
#undef blkstofrags
#undef fragnum
#undef blknum
#undef blksize
#undef INOPB
#undef INOPF
#undef NINDIR

#include <ufs/ffs/fs.h>

/* Undefine macros defined by both lfs/lfs.h and ffs/fs.h */
/* ...to make sure we don't later depend on their (ambigious) definition */
#undef fsbtodb
#undef dbtofsb
#undef blkoff
#undef fragoff
#undef lblktosize
#undef lblkno
#undef numfrags
#undef blkroundup
#undef fragroundup
#undef fragstoblks
#undef blkstofrags
#undef fragnum
#undef blknum
#undef blksize
#undef INOPB
#undef INOPF
#undef NINDIR

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <util.h>

#define BLK_CNT		(blk + (n / 512))

/* common struct for FFS/LFS */
struct sblockinfo {
	struct lfs	*lfs;
	struct fs	*ffs;
	uint64_t	lfs_off;
	uint64_t	ffs_off;
	char		lfs_path[MAXMNTLEN];
	char		ffs_path[MAXMNTLEN];
};

static daddr_t	blk, lastblk;

static int	eflag = 0;
static int	fflag = 0;
static int	flags = 0;
static int	sbaddr = 0; /* counter for the LFS superblocks */

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
static void	ffs_printpart(struct sblockinfo *, int, size_t, int);
static void	ffs_scan(struct sblockinfo *, int);
static int	ffs_checkver(struct sblockinfo *);
/* LFS functions */
static void	lfs_printpart(struct sblockinfo *, int, int);
static void	lfs_scan(struct sblockinfo *, int);
/* common functions */
static void	usage(void) __attribute__((__noreturn__));
static int	scan_disk(int, daddr_t, daddr_t, int);

static int
ffs_checkver(struct sblockinfo *sbi)
{
	switch (sbi->ffs->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			sbi->ffs->fs_size = sbi->ffs->fs_old_size;
			return FSTYPE_FFSV1;
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			return FSTYPE_FFSV2;
		default:
			return FSTYPE_NONE;
	}
}

static void
ffs_printpart(struct sblockinfo *sbi, int flag, size_t ffsize, int n)
{
	
	int fstype = ffs_checkver(sbi);

	switch (flag) {
	case VERBOSE:
		switch (fstype) {
		case FSTYPE_FFSV1:
			(void)printf("offset: %" PRIu64 " n: %d "
			    "id %x,%x size: %" PRIu64 "\n",
			    BLK_CNT - (2 * SBLOCKSIZE / 512), n,
			    sbi->ffs->fs_id[0], sbi->ffs->fs_id[1],
			    (uint64_t)(off_t)sbi->ffs->fs_size *
			    sbi->ffs->fs_fsize / 512);
			break;
		case FSTYPE_FFSV2:
			(void)printf("offset: %" PRIu64 " n: %d "
			    "id %x,%x size: %" PRIu64 "\n",
			    BLK_CNT - (ffsize * SBLOCKSIZE / 512+128),
			    n, sbi->ffs->fs_id[0], sbi->ffs->fs_id[1],
			    (uint64_t)(off_t)sbi->ffs->fs_size *
			    sbi->ffs->fs_fsize / 512);
			break;
		default:
			break;
		}
		break;
	case LABELS:
		(void)printf("X:  %9" PRIu64,
			(uint64_t)((off_t)sbi->ffs->fs_size *
			sbi->ffs->fs_fsize / 512));
		switch (fstype) {
		case FSTYPE_FFSV1:
			(void)printf(" %9" PRIu64,
			    BLK_CNT - (ffsize * SBLOCKSIZE / 512));
			break;
		case FSTYPE_FFSV2:
			(void)printf(" %9" PRIu64,
			    BLK_CNT - (ffsize * SBLOCKSIZE / 512 + 128)); 
			break;
		default:
			break;
		}
		(void)printf(" 4.2BSD %6d %5d %7d # %s [%s]\n",
			sbi->ffs->fs_fsize, sbi->ffs->fs_bsize,
			sbi->ffs->fs_old_cpg, 
			sbi->ffs_path, fstypes[fstype]);
		break;
	default:
		(void)printf("%s ", fstypes[fstype]);
		switch (fstype) {
		case FSTYPE_FFSV1:
			(void)printf("at %" PRIu64,
			    BLK_CNT - (2 * SBLOCKSIZE / 512));
			break;
		case FSTYPE_FFSV2:
			(void)printf("at %" PRIu64,
			    BLK_CNT - (ffsize * SBLOCKSIZE / 512 + 128));
			break;
		default:
			break;
		}
		(void)printf(" size %" PRIu64 ", last mounted on %s\n",
			(uint64_t)((off_t)sbi->ffs->fs_size *
			sbi->ffs->fs_fsize / 512), sbi->ffs_path);
		break;
	}
}

static void
ffs_scan(struct sblockinfo *sbi, int n)
{
	int fstype = ffs_checkver(sbi);
	size_t i = 0;

	if (flags & VERBOSE)
		ffs_printpart(sbi, VERBOSE, NADA, n);
	switch (fstype) {
	case FSTYPE_FFSV1:
		/* fsize/bsize > 512/4096 and < 4096/32768. */
		if ((BLK_CNT - lastblk) == (SBLOCKSIZE / 512)) {
			i = 2;
		/* fsize/bsize 4096/32768. */
		} else if ((BLK_CNT - lastblk) == (SBLOCKSIZE / 170)) {
			i = 4;
		/* fsize/bsize 8192/65536 */
		} else if ((BLK_CNT - lastblk) == (SBLOCKSIZE / 73)) {
			i = 8;
		} else
			break;

		if (flags & LABELS)
			ffs_printpart(sbi, LABELS, i, n);
		else
			ffs_printpart(sbi, NADA, i, n);

		break;
	case FSTYPE_FFSV2:
		/*
		 * That checks for FFSv2 partitions with fragsize/blocksize:
		 * 512/4096, 1024/8192, 2048/16384, 4096/32768 and 8192/65536.
		 * Really enough for now.
		 */
		for (i = 1; i < 16; i <<= 1)
			if ((BLK_CNT - lastblk) == (i * SBLOCKSIZE / 512)) {
				if (flags & LABELS)
					ffs_printpart(sbi, LABELS, i, n);
				else
					ffs_printpart(sbi, NADA, i, n);
			}
		break;
	}
}

static void
lfs_printpart(struct sblockinfo *sbi, int flag, int n)
{
	if (flags & VERBOSE)
               	(void)printf("offset: %" PRIu64 " size %" PRIu32 "\n",
                	sbi->lfs_off, sbi->lfs->lfs_size);
	switch (flag) {
	case LABELS:
		(void)printf("X:  %9" PRIu64,
               		(uint64_t)((off_t)sbi->lfs->lfs_size *
               		sbi->lfs->lfs_fsize / 512));
		(void)printf(" %9" PRIu64, sbi->lfs_off); 
		(void)printf(" 4.4LFS %6d %5d %7d # %s [LFSv%d]\n",
			sbi->lfs->lfs_fsize, sbi->lfs->lfs_bsize,
			sbi->lfs->lfs_nseg, sbi->lfs_path, 
			sbi->lfs->lfs_version);
		break;
	default:
		(void)printf("LFSv%d ", sbi->lfs->lfs_version);
		(void)printf("at %" PRIu64, sbi->lfs_off);
		(void)printf(" size %" PRIu64 ", last mounted on %s\n",
			(uint64_t)((off_t)sbi->lfs->lfs_size *
			sbi->lfs->lfs_fsize / 512), sbi->lfs_path);
		break;
	}
}

static void
lfs_scan(struct sblockinfo *sbi, int n)
{
	/* backup offset */
	lastblk = BLK_CNT - (LFS_SBPAD / 512);
	/* increment counter */
        ++sbaddr;

	switch (sbaddr) {
	/*
	 * first superblock contains the right offset, but lfs_fsmnt is
	 * empty... afortunately the next superblock address has it.
	 */
	case FIRST_SBLOCK_ADDRESS:
		/* copy partition offset */
		if (sbi->lfs_off != lastblk)
			sbi->lfs_off = BLK_CNT - (LFS_SBPAD / 512);
		break;
	case SECOND_SBLOCK_ADDRESS:
		/* copy the path of last mount */
		(void)memcpy(sbi->lfs_path, sbi->lfs->lfs_fsmnt, MAXMNTLEN);
		/* print now that we have the info */
		if (flags & LABELS)
			lfs_printpart(sbi, LABELS, n);
		else
			lfs_printpart(sbi, NADA, n);
		/* clear our struct */
		(void)memset(sbi, 0, sizeof(*sbi));
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
	struct sblockinfo sbinfo;
	uint8_t buf[SBLOCKSIZE * SBCOUNT];
	int n, fstype;

	n = fstype = 0;
	lastblk = -1;

	/* clear our struct before using it */
	(void)memset(&sbinfo, 0, sizeof(sbinfo));

	if (fflags & LABELS)
		(void)printf(
		    "#        size    offset fstype [fsize bsize cpg/sgs]\n");

	for (blk = beg; blk <= ((end < 0) ? blk: end); blk += SBPASS) {
		(void)memset(&buf, 0, sizeof(buf));

		if (pread(fd, buf, sizeof(buf), (off_t)blk * 512) == (off_t)-1)
			err(1, "pread");

		for (n = 0; n < (SBLOCKSIZE * SBCOUNT); n += 512) {
			sbinfo.ffs = (struct fs *)(void *)&buf[n];
			sbinfo.lfs = (struct lfs *)(void *)&buf[n];
			fstype = ffs_checkver(&sbinfo);
			switch (fstype) {
			case FSTYPE_FFSV1:
			case FSTYPE_FFSV2:
				ffs_scan(&sbinfo, n);
				lastblk = BLK_CNT;
				(void)memcpy(sbinfo.ffs_path,
					sbinfo.ffs->fs_fsmnt, MAXMNTLEN);
				break;
			case FSTYPE_NONE:
				/* maybe LFS? */
				if (sbinfo.lfs->lfs_magic == LFS_MAGIC)
					lfs_scan(&sbinfo, n);
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
		"Usage: %s [-lv] [-e end] [-F file] [-s start] "
		"device\n", getprogname());
	exit(EXIT_FAILURE);
}


int
main(int argc, char **argv)
{
	int ch, fd;
	const char *fpath;
	daddr_t end = -1, beg = 0;
	struct disklabel dl;

	fpath = NULL;

	setprogname(*argv);
	while ((ch = getopt(argc, argv, "e:F:ls:v")) != -1)
		switch(ch) {
		case 'e':
			eflag = 1;
			end = atoi(optarg);
			break;
		case 'F':
			fflag = 1;
			fpath = optarg;
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

	if (fflag) {
		struct stat stp;

		if (stat(fpath, &stp))
			err(1, "Cannot stat `%s'", fpath);

		if (!eflag)
			end = (uint64_t)stp.st_size;

		fd = open(fpath, O_RDONLY);
	} else {
		if (argc != 1)
			usage();

		fd = opendisk(argv[0], O_RDONLY, device, sizeof(device), 0);

		if (ioctl(fd, DIOCGDINFO, &dl) == -1) {
			warn("Couldn't retrieve disklabel");
			(void)memset(&dl, 0, sizeof(dl));
			dl.d_secperunit = 0x7fffffff;
		} else {
			(void)printf("Disk: %s\n", dl.d_typename);
			(void)printf("Total sectors on disk: %" PRIu32 "\n\n",
			    dl.d_secperunit);
		}
	}

	if (!eflag && !fflag)
		end = dl.d_secperunit; /* default to max sectors */

	if (fd == -1)
		err(1, "Cannot open `%s'", device);
		/* NOTREACHED */

	return scan_disk(fd, beg, end, flags);
}
