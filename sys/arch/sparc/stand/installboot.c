/*	$NetBSD: installboot.c,v 1.4 1995/05/16 15:28:01 pk Exp $ */

/*
 * Copyright (c) 1994 Paul Kranenburg
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <err.h>
#include <a.out.h>
#include <fcntl.h>
#include <nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int	verbose, nowrite;
char	*boot, *proto, *dev;
struct nlist nl[] = {
#define X_BLOCKNUM	0
	{"_blocknum"},
#define X_MAXBLOCKNUM	1
	{"_maxblocknum"},
	{NULL}
};
daddr_t	*blocknums;		/* block number array in prototype image */
int	maxblocknum;		/* size of this array */


char		*loadprotoblocks __P((char *, long *));
int		loadblocknums __P((char *, int));
static void	devread __P((int, void *, daddr_t, size_t, char *));


	void
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
	long	protosize;

	while ((c = getopt(argc, argv, "vn")) != EOF) {
		switch (c) {
		case 'n':
			nowrite = 1;
			break;
		case 'v':
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

	if (lseek(devfd, DEV_BSIZE, SEEK_SET) != DEV_BSIZE)
		err(1, "lseek bootstrap");

	if (write(devfd, protostore, protosize) != protosize)
		err(1, "write bootstrap");
	(void)close(devfd);
	return 0;
}

	char *
loadprotoblocks(fname, size)
	char *fname;
	long *size;
{
	int	fd;
	char	*bp;
	struct	stat statbuf;
	struct	exec *hp;
	long	off;

	/* Locate block number array in proto file */
	if (nlist(fname, nl) != 0) {
		warnx("nlist: %s: symbols not found", fname);
		return NULL;
	}
	if (nl[X_BLOCKNUM].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_BLOCKNUM].n_un.n_name);
		return NULL;
	}
	if (nl[X_MAXBLOCKNUM].n_type != N_DATA + N_EXT) {
		warnx("nlist: %s: wrong type", nl[X_MAXBLOCKNUM].n_un.n_name);
		return NULL;
	}

	if ((fd = open(fname, O_RDONLY)) < 0) {
		warn("open: %s", fname);
		return NULL;
	}
	if (fstat(fd, &statbuf) != 0) {
		warn("fstat: %s", fname);
		close(fd);
		return NULL;
	}
	if ((bp = calloc(roundup(statbuf.st_size, DEV_BSIZE), 1)) == NULL) {
		warnx("malloc: %s: no memory", fname);
		close(fd);
		return NULL;
	}
	if (read(fd, bp, statbuf.st_size) != statbuf.st_size) {
		warn("read: %s", fname);
		free(bp);
		close(fd);
		return NULL;
	}
	close(fd);

	hp = (struct exec *)bp;
	*size = roundup(hp->a_text + hp->a_data, DEV_BSIZE);

	/* Calculate the symbols' location within the proto file */
	off = N_DATOFF(*hp) - N_DATADDR(*hp) - (hp->a_entry - N_TXTADDR(*hp));
	blocknums = (daddr_t *) (bp + nl[X_BLOCKNUM].n_value + off);
	bcopy(bp + nl[X_MAXBLOCKNUM].n_value + off,
	      &maxblocknum, sizeof(maxblocknum));

	if (verbose) {
		printf("%s: entry point %#x\n", fname, hp->a_entry);
		printf("proto bootblock size %ld\n", *size);
		printf("room for %d filesystem blocks at %#x\n",
			maxblocknum, nl[X_BLOCKNUM].n_value);
	}

	return bp;
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

	if (strcmp(statfsbuf.f_fstypename, "ufs"))
		errx(1, "%s: must be on a UFS filesystem", boot);

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
	 * Get the block numbers; we don't handle fragments
	 */
	ndb = howmany(ip->di_size, fs->fs_bsize);
	ap = ip->di_db;
	for (i = 0; i < NDADDR && *ap && ndb; i++, ap++, ndb--) {
		if (i >= maxblocknum)
			errx(1, "Too many blocks");
		*blocknums++ = *ap;
	}
	if (ndb == 0)
		return 0;

	/*
	 * Just one level of indirections; there isn't much room
	 * for more in the 1st-level bootblocks anyway.
	 */
	blk = ip->di_ib[0];
	devread(devfd, buf, blk, fs->fs_bsize, "indirect block");
	ap = (daddr_t *)buf;
	for (; i < NINDIR(fs) && *ap && ndb; i++, ap++, ndb--) {
		if (i >= maxblocknum)
			errx(1, "Too many blocks");
		*blocknums++ = *ap;
	}

	if (ndb)
		errx(1, "Too many blocks");
	return 0;
}

