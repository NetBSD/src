/*	$NetBSD: installboot.c,v 1.6.6.2 2002/04/17 00:03:43 nathanw Exp $ */

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
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <err.h>
#ifdef BOOT_AOUT
#include <a.out.h>
#endif
#include <sys/exec_elf.h>
#include <fcntl.h>
#include <nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dpme.h"

int	verbose, nowrite, conblockmode, conblockstart;
char	*boot, *proto, *dev;

#define BOOTSECTOR_OFFSET 2048

#ifndef DEFAULT_ENTRY
#define DEFAULT_ENTRY 0x600000
#endif

struct nlist nl[] = {
#define X_BLOCKTABLE	0
	{"_block_table"},
#define X_BLOCKCOUNT	1
	{"_block_count"},
#define X_BLOCKSIZE	2
	{"_block_size"},
#define X_ENTRY_POINT	3
	{"_entry_point"},
	{NULL}
};

daddr_t	*block_table;		/* block number array in prototype image */
int32_t	*block_count_p;		/* size of this array */
int32_t	*block_size_p;		/* filesystem block size */
int32_t	*entry_point_p;		/* entry point */
int32_t	max_block_count;

char		*loadprotoblocks __P((char *, long *));
int		loadblocknums_contig __P((char *, int));
int		loadblocknums_ffs __P((char *, int));
static void	devread __P((int, void *, daddr_t, size_t, char *));
static void	usage __P((void));
int 		main __P((int, char *[]));


static void
usage()
{
	fprintf(stderr,
	    "usage: installboot [-n] [-v] [-b bno] <boot> <proto> <device>\n");
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
	int	mib[2];
	size_t	size;
	int (*loadblocknums_func) __P((char *, int));

	while ((c = getopt(argc, argv, "vnb:")) != -1) {
		switch (c) {

		case 'b':
			/* generic override, supply starting block # */
			conblockmode = 1;
			conblockstart = atoi(optarg);
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

	/* Open and check raw disk device */
	if ((devfd = open(dev, O_RDONLY, 0)) < 0)
		err(1, "open: %s", dev);

	/* Extract and load block numbers */
	if (conblockmode)
		loadblocknums_func = loadblocknums_contig;
	else
		loadblocknums_func = loadblocknums_ffs;

	if ((loadblocknums_func)(boot, devfd) != 0)
		exit(1);

	(void)close(devfd);

	if (nowrite)
		return 0;

	/* Write patched proto bootblocks into the superblock */
	if (protosize > SBSIZE - DEV_BSIZE)
		errx(1, "proto bootblocks too big");

	if ((devfd = open(dev, O_RDWR, 0)) < 0)
		err(1, "open: %s", dev);

	if (writeapplepartmap(devfd) < 0)
		err(1, "write apm: %s", dev);

	if (lseek(devfd, BOOTSECTOR_OFFSET, SEEK_SET) != BOOTSECTOR_OFFSET)
		err(1, "lseek bootstrap");

	/* Sync filesystems (to clean in-memory superblock?) */
	sync();

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
	int	fd, sz;
	char	*bp;
	struct	stat statbuf;
#ifdef BOOT_AOUT
	struct	exec *hp;
#endif
	long	off;
	Elf32_Ehdr *eh;
	Elf32_Phdr *ph;

	/* Locate block number array in proto file */
	if (nlist(fname, nl) != 0) {
		warnx("nlist: %s: symbols not found", fname);
		return NULL;
	}
#ifdef BOOT_AOUT
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
#endif

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

#ifdef BOOT_AOUT
	hp = (struct exec *)bp;
#endif
	eh = (Elf32_Ehdr *)bp;
	ph = (Elf32_Phdr *)(bp + eh->e_phoff);
	sz = 1024;

	/* Calculate the symbols' location within the proto file */
	off = ph->p_offset - eh->e_entry;
	block_table = (daddr_t *)(bp + nl[X_BLOCKTABLE].n_value + off);
	block_count_p = (int32_t *)(bp + nl[X_BLOCKCOUNT].n_value + off);
	block_size_p = (int32_t *)(bp + nl[X_BLOCKSIZE].n_value + off);
	entry_point_p = (int32_t *)(bp + nl[X_ENTRY_POINT].n_value + off);

	if ((int)block_table & 3) {
		warn("%s: invalid address: block_table = %x",
		     fname, block_table);
		free(bp);
		close(fd);
		return NULL;
	}
	if ((int)block_count_p & 3) {
		warn("%s: invalid address: block_count_p = %x",
		     fname, block_count_p);
		free(bp);
		close(fd);
		return NULL;
	}
	if ((int)block_size_p & 3) {
		warn("%s: invalid address: block_size_p = %x",
		     fname, block_size_p);
		free(bp);
		close(fd);
		return NULL;
	}
	if ((int)entry_point_p & 3) {
		warn("%s: invalid address: entry_point_p = %x",
		     fname, entry_point_p);
		free(bp);
		close(fd);
		return NULL;
	}
	max_block_count = *block_count_p;

	if (verbose) {
		printf("proto bootblock size: %ld\n", sz);
	}

	/*
	 * We convert the a.out header in-vitro into something that
	 * Sun PROMs understand.
	 * Old-style (sun4) ROMs do not expect a header at all, so
	 * we turn the first two words into code that gets us past
	 * the 32-byte header where the actual code begins. In assembly
	 * speak:
	 *	.word	MAGIC		! a NOP
	 *	ba,a	start		!
	 *	.skip	24		! pad
	 * start:
	 */

	*size = sz;
	return (bp + 0x74);
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

int loadblocknums_contig(boot, devfd)
	char *boot;
	int devfd;
{
	int size;
	struct stat sb;

	if (stat(boot, &sb) == -1)
		err(1, "stat: %s", boot);
	size = sb.st_size;

	if (verbose) {
		printf("%s: blockstart %d\n", dev, conblockstart);
		printf("%s: size %d\n", boot, size);
	}

	*block_size_p = roundup(size, 512);
	*block_count_p = 1;
	*entry_point_p = DEFAULT_ENTRY;

	block_table[0] = conblockstart;

	return 0;
}

static char sblock[SBSIZE];

int
loadblocknums_ffs(boot, devfd)
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

	/*
	 * Register block count.
	 */
	*block_count_p = ndb;

	/*
	 * Register entry point.
	 */
	*entry_point_p = DEFAULT_ENTRY;
	if (verbose)
		printf("entry point: 0x%08x\n", *entry_point_p);

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

int
writeapplepartmap(fd)
	int fd;
{
	struct drvr_map dm;
	struct partmapentry pme;

	/* block 0 */
	if (lseek(fd, 0, SEEK_SET) != 0)
		return -1;
	if (read(fd, &dm, 512) != 512)		/* read existing disklabel */
		return -1;
	if (lseek(fd, 0, SEEK_SET) != 0)
		return -1;

	dm.sbSig = DRIVER_MAP_MAGIC;
	dm.sbBlockSize = 512;
	dm.sbBlkCount = 0;

	if (write(fd, &dm, 512) != 512)
		return -1;

	/* block 1: Apple Partition Map */
	memset(&pme, 0, sizeof(pme));
	pme.pmSig = DPME_MAGIC;
	pme.pmMapBlkCnt = 2;
	pme.pmPyPartStart = 1;
	pme.pmPartBlkCnt = pme.pmDataCnt = 2;
	strcpy(pme.pmPartName, "Apple");
	strcpy(pme.pmPartType, "Apple_partition_map");
	
	pme.pmPartStatus = 0x37;

	if (lseek(fd, 512, SEEK_SET) != 512)
		return -1;
	if (write(fd, &pme, 512) != 512)
		return -1;

	/* block 2: NetBSD partition */
	memset(&pme, 0, sizeof(pme));
	pme.pmSig = DPME_MAGIC;
	pme.pmMapBlkCnt = 2;
	pme.pmPyPartStart = 4;
	pme.pmPartBlkCnt = pme.pmDataCnt = 0x7fffffff;
	strcpy(pme.pmPartName, "NetBSD");
	strcpy(pme.pmPartType, "NetBSD/macppc");
	pme.pmPartStatus = 0x3b;
	pme.pmBootSize = 0x400;
	pme.pmBootLoad = 0x4000;
	pme.pmBootEntry = 0x4000;
	strcpy(pme.pmProcessor, "PowerPC");

	if (lseek(fd, 1024, SEEK_SET) != 1024)
		return -1;
	if (write(fd, &pme, 512) != 512)
		return -1;

	return 0;
}
