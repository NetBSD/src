/*	$NetBSD: installboot.c,v 1.4 2001/01/30 14:11:01 tsutsui Exp $ */

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

#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <err.h>
#include <a.out.h>
#include <fcntl.h>
#include <nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "loadfile.h"

int	verbose, nowrite;
char	*boot, *proto, *dev;

#define BOOTSECTOR_OFFSET 512
#define LABELOFFSET 64

#if 0
#ifdef __ELF__
#define	SYMNAME(a)	a
#else
#define	SYMNAME(a)	__CONCAT("_",a)
#endif
#else
/* XXX: Hack in libc nlist works with both formats */
#define	SYMNAME(a)	__CONCAT("_",a)
#endif

struct nlist nl[] = {
#define X_BLOCKTABLE	0
	{ {SYMNAME("block_table")} },
#define X_BLOCKCOUNT	1
	{ {SYMNAME("block_count")} },
#define X_BLOCKSIZE	2
	{ {SYMNAME("block_size")} },
#define X_ENTRY_POINT	3
	{ {SYMNAME("entry_point")} },
	{ {NULL} }
};

daddr_t	*block_table;		/* block number array in prototype image */
int32_t	*block_count_p;		/* size of this array */
int32_t	*block_size_p;		/* filesystem block size */
int32_t	*entry_point_p;		/* entry_point */
int32_t	max_block_count;

char		*loadprotoblocks __P((char *, size_t *));
int		loadblocknums __P((char *, int));
static void	devread __P((int, void *, daddr_t, size_t, char *));
static void	usage __P((void));
int 		main __P((int, char *[]));

static void
usage()
{
	fprintf(stderr,
		"usage: installboot [-n] [-v] <boot> <proto> <device>\n");
	exit(1);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int	c;
	int	devfd;
	char	*protostore;
	size_t	protosize;
	u_int32_t boot00[BOOTSECTOR_OFFSET / sizeof(u_int32_t)];

	while ((c = getopt(argc, argv, "vn")) != EOF) {
		switch (c) {

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
	if (protosize > SBSIZE - DEV_BSIZE)
		errx(1, "proto bootblocks too big");

	if ((devfd = open(dev, O_RDWR, 0)) < 0)
		err(1, "open: %s", dev);

	if (lseek(devfd, BOOTSECTOR_OFFSET, SEEK_SET) != BOOTSECTOR_OFFSET)
		err(1, "lseek bootstrap");

	/* Sync filesystems (to clean in-memory superblock?) */
	sync(); sync(); sync();

	if (write(devfd, protostore, protosize) != protosize)
		err(1, "write bootstrap");

	/* Write boot00 */
	if (lseek(devfd, 0, SEEK_SET) != 0)
		err(1, "lseek 0a");
	if (read(devfd, boot00, sizeof(boot00)) != sizeof(boot00))
		err(1, "read boot00");

	memset(boot00, 0, LABELOFFSET);
	boot00[0] = 0x600001fe;	/* jra +0x200 */
	boot00[2] = 0x0;
	if (lseek(devfd, 0, SEEK_SET) != 0)
		err(1, "lseek 0b");
	if (write(devfd, boot00, sizeof(boot00)) != sizeof(boot00))
		err(1, "write boot00");

	(void)close(devfd);
	return 0;
}

char *
loadprotoblocks(fname, size)
	char *fname;
	size_t *size;
{
	int	fd, sz;
	u_long	ap, bp, st;
	u_long	marks[MARK_MAX];

	/* Locate block number array in proto file */
	if (nlist(fname, nl) != 0) {
		warnx("nlist: %s: symbols not found", fname);
		return NULL;
	}
	if (nl[X_BLOCKTABLE].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_BLOCKTABLE].n_un.n_name);
		return NULL;
	}
	if (nl[X_BLOCKCOUNT].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_BLOCKCOUNT].n_un.n_name);
		return NULL;
	}
	if (nl[X_BLOCKSIZE].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_BLOCKSIZE].n_un.n_name);
		return NULL;
	}
	if (nl[X_ENTRY_POINT].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_ENTRY_POINT].n_un.n_name);
		return NULL;
	}

	marks[MARK_START] = 0;
	if ((fd = loadfile(fname, marks, COUNT_TEXT|COUNT_DATA)) == -1)
		return NULL;
	(void)close(fd);

	sz = (marks[MARK_END] - marks[MARK_START]);
	sz = roundup(sz, DEV_BSIZE);
	st = marks[MARK_START];

	if ((ap = (u_long)malloc(sz)) == NULL) {
		warn("malloc: %s", "");
		return NULL;
	}

	bp = ap;
	marks[MARK_START] = bp - st;
	if ((fd = loadfile(fname, marks, LOAD_TEXT|LOAD_DATA)) == -1)
		return NULL;
	(void)close(fd);

	block_table = (daddr_t *)(bp + nl[X_BLOCKTABLE].n_value - st);
	block_count_p = (int32_t *)(bp + nl[X_BLOCKCOUNT].n_value - st);
	block_size_p = (int32_t *)(bp + nl[X_BLOCKSIZE].n_value - st);
	entry_point_p = (int32_t *)(bp + nl[X_ENTRY_POINT].n_value - st);

	if ((int)block_table & 3) {
		warn("%s: invalid address: block_table = %p",
		     fname, block_table);
		free((void *)bp);
		close(fd);
		return NULL;
	}
	if ((int)block_count_p & 3) {
		warn("%s: invalid address: block_count_p = %p",
		     fname, block_count_p);
		free((void *)bp);
		close(fd);
		return NULL;
	}
	if ((int)block_size_p & 3) {
		warn("%s: invalid address: block_size_p = %p",
		     fname, block_size_p);
		free((void *)bp);
		close(fd);
		return NULL;
	}
	if ((int)entry_point_p & 3) {
		warn("%s: invalid address: entry_point_p = %p",
		     fname, entry_point_p);
		free((void *)bp);
		close(fd);
		return NULL;
	}
	max_block_count = *block_count_p;

	if (verbose) {
		printf("proto bootblock size: %d\n", sz);
	}

	*size = sz;
	ap += BOOTSECTOR_OFFSET; /* XXX */
	return (char *)ap;
}

static void
devread(fd, buf, blk, size, msg)
	int	fd;
	void	*buf;
	daddr_t	blk;
	size_t	size;
	char	*msg;
{
	if (lseek(fd, dbtob(blk), SEEK_SET) != dbtob(blk))
		err(1, "%s: devread: lseek", msg);

	if (read(fd, buf, size) != size)
		err(1, "%s: devread: read", msg);
}

static char sblock[SBSIZE];

int
loadblocknums(boot, devfd)
char	*boot;
int	devfd;
{
	int		i, fd;
	struct	stat	statbuf;
	struct	statfs	statfsbuf;
	struct fs	*fs;
	char		*buf;
	daddr_t		blk, *ap;
	struct dinode	*ip;
	int		ndb;

	/*
	 * Open 2nd-level boot program and record the block numbers
	 * it occupies on the filesystem represented by `devfd'.
	 */
	if ((fd = open(boot, O_RDONLY)) < 0)
		err(1, "open: %s", boot);

	if (fstatfs(fd, &statfsbuf) != 0)
		err(1, "statfs: %s", boot);

	if (strncmp(statfsbuf.f_fstypename, "ffs", MFSNAMELEN) &&
	    strncmp(statfsbuf.f_fstypename, "ufs", MFSNAMELEN)) {
		errx(1, "%s: must be on an FFS filesystem", boot);
	}

	if (fsync(fd) != 0)
		err(1, "fsync: %s", boot);

	if (fstat(fd, &statbuf) != 0)
		err(1, "fstat: %s", boot);

	close(fd);

	/* Read superblock */
	devread(devfd, sblock, btodb(SBOFF), SBSIZE, "superblock");
	fs = (struct fs *)sblock;

	/* Read inode */
	if ((buf = malloc(fs->fs_bsize)) == NULL)
		errx(1, "No memory for filesystem block");

	blk = fsbtodb(fs, ino_to_fsba(fs, statbuf.st_ino));
	devread(devfd, buf, blk, fs->fs_bsize, "inode");
	ip = (struct dinode *)(buf) + ino_to_fsbo(fs, statbuf.st_ino);

	/*
	 * Register filesystem block size.
	 */
	*block_size_p = fs->fs_bsize;

	/*
	 * Get the block numbers; we don't handle fragments
	 */
	ndb = howmany(ip->di_size, fs->fs_bsize);
	if (ndb > max_block_count)
		errx(1, "%s: Too many blocks", boot);

	if (verbose)
		printf("entry point: 0x%08x\n", *entry_point_p);

	/*
	 * Register block count.
	 */
	*block_count_p = ndb;

	if (verbose)
		printf("%s: block numbers: ", boot);
	ap = ip->di_db;
	for (i = 0; i < NDADDR && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		block_table[i] = blk;
		if (verbose)
			printf("%d ", blk);
	}
	if (verbose)
		printf("\n");

	if (ndb == 0)
		return 0;

	/*
	 * Just one level of indirections; there isn't much room
	 * for more in the 1st-level bootblocks anyway.
	 */
	if (verbose)
		printf("%s: block numbers (indirect): ", boot);
	blk = ip->di_ib[0];
	devread(devfd, buf, blk, fs->fs_bsize, "indirect block");
	ap = (daddr_t *)buf;
	for (; i < NINDIR(fs) && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		block_table[i] = blk;
		if (verbose)
			printf("%d ", blk);
	}
	if (verbose)
		printf("\n");

	if (ndb)
		errx(1, "%s: Too many blocks", boot);
	return 0;
}
