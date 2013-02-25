/*	$NetBSD: installboot.c,v 1.15.44.1 2013/02/25 00:28:52 tls Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <err.h>
#include <fcntl.h>
#include <nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/disklabel.h>

#include "loadfile.h"

int	verbose, nowrite, hflag;
char	*boot, *proto, *dev;

struct nlist nl[] = {
#define X_BLOCK_SIZE	0
#define X_BLOCK_COUNT	1
#define X_BLOCK_TABLE	2
#ifdef __ELF__
	{ "block_size" },
	{ "block_count" },
	{ "block_table" },
#else
	{ "_block_size" },
	{ "_block_count" },
	{ "_block_table" },
#endif
	{ NULL }
};

int *block_size_p;		/* block size var. in prototype image */
int *block_count_p;		/* block count var. in prototype image */
/* XXX ondisk32 */
int32_t	*block_table;		/* block number array in prototype image */
int	maxblocknum;		/* size of this array */


char		*loadprotoblocks(char *, size_t *);
int		loadblocknums(char *, int);
static void	devread(int, void *, daddr_t, size_t, char *);
static void	usage(void);
int 		main(int, char *[]);


static void
usage(void)
{

	fprintf(stderr,
	    "usage: installboot [-n] [-v] [-h] <boot> <proto> <device>\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int	c;
	int	devfd;
	char	*protostore;
	size_t	protosize;

	while ((c = getopt(argc, argv, "vnh")) != -1) {
		switch (c) {
		case 'h':
			/* Don't strip a.out header */
			hflag = 1;
			break;
		case 'n':
			/* Do not actually write the bootblock to disk */
			nowrite = 1;
			break;
		case 'v':
			/* Chat */
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	if (argc - optind < 3) {
		usage();
	}

	boot = argv[optind];
	proto = argv[optind + 1];
	dev = argv[optind + 2];

	if (verbose) {
		printf("boot: %s\n", boot);
		printf("proto: %s\n", proto);
		printf("device: %s\n", dev);
	}

	/* Load proto blocks into core */
	if ((protostore = loadprotoblocks(proto, &protosize)) == NULL)
		exit(1);

	/* XXX - Paranoia: Make sure size is aligned! */
	if (protosize & (DEV_BSIZE - 1))
		err(1, "proto bootblock bad size=%d", protosize);

	/* Open and check raw disk device */
	if ((devfd = open(dev, O_RDONLY, 0)) < 0)
		err(1, "open: %s", dev);

	/* Extract and load block numbers */
	if (loadblocknums(boot, devfd) != 0)
		exit(1);

	(void)close(devfd);

	if (nowrite)
		return 0;

	/* Write patched proto bootblocks into the superblock */
	if (protosize > SBLOCKSIZE - DEV_BSIZE)
		errx(1, "proto bootblocks too big");

	/* The primary bootblock needs to be written to the raw partition */
	dev[strlen(dev) - 1] = 'a' + RAW_PART;

	if ((devfd = open(dev, O_RDWR, 0)) < 0)
		err(1, "open: %s", dev);

	if (lseek(devfd, DEV_BSIZE, SEEK_SET) != DEV_BSIZE)
		err(1, "lseek bootstrap");

	/* Sync filesystems (to clean in-memory superblock?) */
	sync();

	if (write(devfd, protostore, protosize) != protosize)
		err(1, "write bootstrap");
	(void)close(devfd);
	return 0;
}

char *
loadprotoblocks(char *fname, size_t *size)
{
	int	fd;
	u_long	marks[MARK_MAX], bp, offs;

	fd = -1;

	/* Locate block number array in proto file */
	if (nlist(fname, nl) != 0) {
		warnx("nlist: %s: symbols not found", fname);
		return NULL;
	}

	marks[MARK_START] = 0;
	if ((fd = loadfile(fname, marks, COUNT_TEXT|COUNT_DATA)) == -1)
		return NULL;
	(void)close(fd);

	*size = roundup(marks[MARK_END] - marks[MARK_START], DEV_BSIZE);
	bp = (u_long)malloc(*size);

	offs = marks[MARK_START];
	marks[MARK_START] = bp - offs;

	if ((fd = loadfile(fname, marks, LOAD_TEXT|LOAD_DATA)) == -1)
		return NULL;
	(void)close(fd);

	/* Calculate the symbols' locations within the proto file */
	block_size_p  =   (int *)(bp + (nl[X_BLOCK_SIZE ].n_value - offs));
	block_count_p =   (int *)(bp + (nl[X_BLOCK_COUNT].n_value - offs));
	/* XXX ondisk32 */
	block_table = (int32_t *)(bp + (nl[X_BLOCK_TABLE].n_value - offs));
	maxblocknum = *block_count_p;

	if (verbose) {
		printf("%s: entry point %#lx\n", fname, marks[MARK_ENTRY]);
		printf("proto bootblock size %d\n", *size);
		printf("room for %d filesystem blocks at %#lx\n",
			maxblocknum, nl[X_BLOCK_TABLE].n_value);
	}

	return (char *)bp;

	if (bp)
		free((void *)bp);
	return NULL;
}

static void
devread(int fd, void *buf, daddr_t blk, size_t size, char *msg)
{

	if (lseek(fd, dbtob(blk), SEEK_SET) != dbtob(blk))
		err(1, "%s: devread: lseek", msg);

	if (read(fd, buf, size) != size)
		err(1, "%s: devread: read", msg);
}

static char sblock[SBLOCKSIZE];

int
loadblocknums(char *boot, int devfd)
{
	int		i, fd;
	struct	stat	statbuf;
	struct	statvfs	statvfsbuf;
	struct fs	*fs;
	char		*buf;
	daddr_t		blk, *ap;
	struct ufs1_dinode *ip;
	int		ndb;

	/*
	 * Open 2nd-level boot program and record the block numbers
	 * it occupies on the filesystem represented by `devfd'.
	 */

	/* Make sure the (probably new) boot file is on disk. */
	sync(); sleep(1);

	if ((fd = open(boot, O_RDONLY)) < 0)
		err(1, "open: %s", boot);

	if (fstatvfs(fd, &statvfsbuf) != 0)
		err(1, "statfs: %s", boot);

	if (strncmp(statvfsbuf.f_fstypename, "ffs",
	    sizeof(statvfsbuf.f_fstypename)) &&
	    strncmp(statvfsbuf.f_fstypename, "ufs",
	    sizeof(statvfsbuf.f_fstypename))) {
		errx(1, "%s: must be on an FFS filesystem", boot);
	}

	if (fsync(fd) != 0)
		err(1, "fsync: %s", boot);

	if (fstat(fd, &statbuf) != 0)
		err(1, "fstat: %s", boot);

	close(fd);

	/* Read superblock */
	devread(devfd, sblock, (daddr_t)(BBSIZE / DEV_BSIZE),
	    SBLOCKSIZE, "superblock");
	fs = (struct fs *)sblock;

	/* Sanity-check super-block. */
	if (fs->fs_magic != FS_UFS1_MAGIC)
		errx(1, "Bad magic number in superblock, must be UFS1");
	if (fs->fs_inopb <= 0)
		err(1, "Bad inopb=%d in superblock", fs->fs_inopb);

	/* Read inode */
	if ((buf = malloc(fs->fs_bsize)) == NULL)
		errx(1, "No memory for filesystem block");

	blk = fsbtodb(fs, ino_to_fsba(fs, statbuf.st_ino));
	devread(devfd, buf, blk, fs->fs_bsize, "inode");
	ip = (struct ufs1_dinode *)(buf) + ino_to_fsbo(fs, statbuf.st_ino);

	/*
	 * Have the inode.  Figure out how many blocks we need.
	 */
	ndb = howmany(ip->di_size, fs->fs_bsize);
	if (ndb > maxblocknum)
		errx(1, "Too many blocks");
	*block_count_p = ndb;
	*block_size_p = fs->fs_bsize;
	if (verbose)
		printf("Will load %d blocks of size %d each.\n",
		    ndb, fs->fs_bsize);

	/*
	 * Get the block numbers; we don't handle fragments
	 */
	ap = ip->di_db;
	for (i = 0; i < UFS_NDADDR && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		if (verbose)
			printf("%d: %d\n", i, blk);
		block_table[i] = blk;
	}
	if (ndb == 0)
		return 0;

	/*
	 * Just one level of indirections; there isn't much room
	 * for more in the 1st-level bootblocks anyway.
	 */
	blk = fsbtodb(fs, ip->di_ib[0]);
	devread(devfd, buf, blk, fs->fs_bsize, "indirect block");
	/* XXX ondisk32 */
	ap = (int32_t *)buf;
	for (; i < NINDIR(fs) && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		if (verbose)
			printf("%d: %d\n", i, blk);
		block_table[i] = blk;
	}

	return 0;
}
