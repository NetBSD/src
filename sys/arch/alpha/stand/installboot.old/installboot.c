/* $NetBSD: installboot.c,v 1.13 1999/04/05 03:02:07 cgd Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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
 * 3. <not applicable>
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
#include <sys/ioctl.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <isofs/cd9660/iso.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

#include "stand/common/bbinfo.h"

int	verbose, nowrite, hflag, cd9660, conblockmode, conblockstart;
char	*boot, *proto, *dev;

struct bbinfoloc *bbinfolocp;
struct bbinfo *bbinfop;
int	max_block_count;


void		setup_contig_blks(u_long, u_long, int, int, char *);
char		*loadprotoblocks __P((char *, long *));
int		loadblocknums_ffs __P((char *, int, unsigned long));
int		loadblocknums_cd9660 __P((char *, int, unsigned long));
int		loadblocknums_passthru __P((char *, int, unsigned long));
static void	devread __P((int, void *, daddr_t, size_t, char *));
static void	usage __P((void));
int 		main __P((int, char *[]));

extern	char *__progname;

static void
usage()
{
	fprintf(stderr,
	    "usage: %s [-n] [-v] <boot> <proto> <dev>\n", __progname);
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
	struct stat disksb, bootsb;
	struct statfs fssb;
	struct disklabel dl;
	unsigned long partoffset;
	int (*loadblocknums_func) __P((char *, int, unsigned long));

	while ((c = getopt(argc, argv, "nvb:")) != -1) {
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

	/* Open and check the target device. */
	if ((devfd = open(dev, O_RDONLY, 0)) < 0)
		err(1, "open: %s", dev);
	if (fstat(devfd, &disksb) == -1)
		err(1, "fstat: %s", dev);
	if (!S_ISCHR(disksb.st_mode))
		errx(1, "%s must be a character device node", dev);
	if ((minor(disksb.st_rdev) % getmaxpartitions()) != getrawpartition())
		errx(1, "%s must be the raw partition", dev);

	/* Extract and load block numbers */
	if (stat(boot, &bootsb) == -1)	
		err(1, "stat: %s", boot);
	/*
	 * The error below doesn't matter in conblockmode, but leave it in
	 * because it may catch misplaced arguments; this program doesn't
	 * get run enough for people to be familiar with it.
	 */
	if (!S_ISREG(bootsb.st_mode))
		errx(1, "%s must be a regular file", boot);
	if(!conblockmode) {
		if ((minor(disksb.st_rdev) / getmaxpartitions()) != 
		    (minor(bootsb.st_dev) / getmaxpartitions()))
			errx(1, "%s must be somewhere on %s", boot, dev);
		/*
		 * Determine the file system type of the file system on which
		 * the boot program resides.
		 */
		if (statfs(boot, &fssb) == -1)
			err(1, "statfs: %s", boot);
		if (strcmp(fssb.f_fstypename, MOUNT_CD9660) == 0) {
			/*
			 * Installing a boot block on a CD-ROM image.
			 */
			cd9660 = 1;
		} else if (strcmp(fssb.f_fstypename, MOUNT_FFS) != 0) {
			/*
			 * Some other file system type, which is not FFS.
			 * Can't handle these.
			 */
			errx(1, "unsupported file system type: %s",
			    fssb.f_fstypename);
		}
		if (verbose)
			printf("file system type: %s\n", fssb.f_fstypename);
		/*
		 * Find the offset of the secondary boot block's partition
		 * into the disk.
		 */
		if (ioctl(devfd, DIOCGDINFO, &dl) == -1)
			err(1, "read disklabel: %s", dev);
		partoffset = dl.d_partitions[minor(bootsb.st_dev) %
		    getmaxpartitions()].p_offset;
		if (verbose)
			printf("%s partition offset = 0x%lx\n",
				boot, partoffset);
		/* 
		 * sync filesystems (make sure boot's block numbers are stable)
		 */
		sync();
		sleep(2);
		sync();
		sleep(2);
	}
	if (conblockmode)
		loadblocknums_func = loadblocknums_passthru;
	else if (cd9660)
		loadblocknums_func = loadblocknums_cd9660;
	else
		loadblocknums_func = loadblocknums_ffs;

	if ((*loadblocknums_func)(boot, devfd, partoffset) != 0)
		exit(1);

	(void)close(devfd);

	if (nowrite)
		return 0;

#if 0
	/* Write patched proto bootblocks into the superblock */
	if (protosize > SBSIZE - DEV_BSIZE)
		errx(1, "proto bootblocks too big");
#endif

	if ((devfd = open(dev, O_RDWR, 0)) < 0)
		err(1, "open: %s", dev);

	if (lseek(devfd, DEV_BSIZE, SEEK_SET) != DEV_BSIZE)
		err(1, "lseek bootstrap");

	if (write(devfd, protostore, protosize) != protosize)
		err(1, "write bootstrap");

	/*
	 * Disks should already have a disklabel, but CD-ROM images
	 * may not.  Construct one as the SCSI CD driver would and
	 * write it to the image.
	 */
	if (cd9660) {
		char block[DEV_BSIZE];
		struct disklabel *lp;
		size_t imagesize;
		int rawpart = getrawpartition();
		off_t labeloff;

		labeloff = (LABELSECTOR * DEV_BSIZE) + LABELOFFSET;
		if (lseek(devfd, labeloff, SEEK_SET) != labeloff)
			err(1, "lseek to write fake label");

		if (read(devfd, block, sizeof(block)) != sizeof(block))
			err(1, "read fake label block");

		lp = (struct disklabel *)block;

		imagesize = howmany(dl.d_partitions[rawpart].p_size *
		    dl.d_secsize, DEV_BSIZE);

		memset(lp, 0, sizeof(struct disklabel));

		lp->d_secsize = DEV_BSIZE;
		lp->d_ntracks = 1;
		lp->d_nsectors = 100;
		lp->d_ncylinders = (imagesize / 100) + 1;
		lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

		strncpy(lp->d_typename, "SCSI CD-ROM", 16);
		lp->d_type = DTYPE_SCSI;
		strncpy(lp->d_packname, "NetBSD/alpha", 16);
		lp->d_secperunit = imagesize;
		lp->d_rpm = 300;
		lp->d_interleave = 1;
		lp->d_flags = D_REMOVABLE;

		lp->d_partitions[0].p_size = lp->d_secperunit;
		lp->d_partitions[0].p_offset = 0;
		lp->d_partitions[0].p_fstype = FS_ISO9660;
		lp->d_partitions[rawpart].p_size = lp->d_secperunit;
		lp->d_partitions[rawpart].p_offset = 0;
		lp->d_partitions[rawpart].p_fstype = FS_ISO9660;
		lp->d_npartitions = rawpart + 1;

		lp->d_bbsize = 8192;
		lp->d_sbsize = 8192;

		lp->d_magic = lp->d_magic2 = DISKMAGIC;

		lp->d_checksum = dkcksum(lp);

		if (lseek(devfd, labeloff, SEEK_SET) != labeloff)
			err(1, "lseek to write fake label");

		if (write(devfd, block, sizeof(block)) != sizeof(block))
			err(1, "write fake label");
	}

	{
	struct boot_block bb;
	long *lp, *ep;

	if (lseek(devfd, 0, SEEK_SET) != 0)
		err(1, "lseek label");

	if (read(devfd, &bb, sizeof (bb)) != sizeof (bb)) 
		err(1, "read label");

        bb.bb_secsize = 15;
        bb.bb_secstart = 1;
        bb.bb_flags = 0;
        bb.bb_cksum = 0;

        for (lp = (long *)&bb, ep = &bb.bb_cksum; lp < ep; lp++)
                bb.bb_cksum += *lp;

	if (lseek(devfd, 0, SEEK_SET) != 0)
		err(1, "lseek label 2");

        if (write(devfd, &bb, sizeof bb) != sizeof bb)
		err(1, "write label ");
	}

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
	u_int64_t *matchp;

	/*
	 * Read the prototype boot block into memory.
	 */
	if ((fd = open(fname, O_RDONLY)) < 0) {
		warn("open: %s", fname);
		return NULL;
	}
	if (fstat(fd, &statbuf) != 0) {
		warn("fstat: %s", fname);
		close(fd);
		return NULL;
	}
	sz = roundup(statbuf.st_size, DEV_BSIZE);
	if ((bp = calloc(sz, 1)) == NULL) {
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

	/*
	 * Find the magic area of the program, and figure out where
	 * the 'blocks' struct is, from that.
	 */
	bbinfolocp = NULL;
	for (matchp = (u_int64_t *)bp; (char *)matchp < bp + sz; matchp++) {
		if (*matchp != 0xbabefacedeadbeef)
			continue;
		bbinfolocp = (struct bbinfoloc *)matchp;
		if (bbinfolocp->magic1 == 0xbabefacedeadbeef &&
		    bbinfolocp->magic2 == 0xdeadbeeffacebabe)
			break;
		bbinfolocp = NULL;
	}

	if (bbinfolocp == NULL) {
		warnx("%s: not a valid boot block?", fname);
		return NULL;
	}

	bbinfop = (struct bbinfo *)(bp + bbinfolocp->end - bbinfolocp->start);	
	memset(bbinfop, 0, sz - (bbinfolocp->end - bbinfolocp->start));
	max_block_count =
	    ((char *)bbinfop->blocks - bp) / sizeof (bbinfop->blocks[0]);

	if (verbose) {
		printf("boot block info locator at offset 0x%lx\n",
			(u_long)((char *)bbinfolocp - bp));
		printf("boot block info at offset 0x%lx\n",
			(u_long)((char *)bbinfop - bp));
		printf("max number of blocks: %d\n", max_block_count);
	}

	*size = sz;
	return (bp);
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
loadblocknums_ffs(boot, devfd, partoffset)
	char	*boot;
	int	devfd;
	unsigned long partoffset;
{
	int		i, fd;
	struct	stat	statbuf;
	struct	statfs	statfsbuf;
	struct fs	*fs;
	char		*buf;
	daddr_t		blk, *ap;
	struct dinode	*ip;
	int		ndb;
	int32_t		cksum;

	/*
	 * Open 2nd-level boot program and record the block numbers
	 * it occupies on the filesystem represented by `devfd'.
	 */
	if ((fd = open(boot, O_RDONLY)) < 0)
		err(1, "open: %s", boot);

	if (fstatfs(fd, &statfsbuf) != 0)
		err(1, "statfs: %s", boot);

	if (strncmp(statfsbuf.f_fstypename, MOUNT_FFS, MFSNAMELEN))
		errx(1, "%s: must be on a FFS filesystem", boot);

	if (fsync(fd) != 0)
		err(1, "fsync: %s", boot);

	if (fstat(fd, &statbuf) != 0)
		err(1, "fstat: %s", boot);

	close(fd);

	/* Read superblock */
	devread(devfd, sblock, btodb(SBOFF) + partoffset, SBSIZE,
	    "superblock");
	fs = (struct fs *)sblock;

	/* Read inode */
	if ((buf = malloc(fs->fs_bsize)) == NULL)
		errx(1, "No memory for filesystem block");

	blk = fsbtodb(fs, ino_to_fsba(fs, statbuf.st_ino));
	devread(devfd, buf, blk + partoffset, fs->fs_bsize, "inode");
	ip = (struct dinode *)(buf) + ino_to_fsbo(fs, statbuf.st_ino);

	/*
	 * Register filesystem block size.
	 */
	bbinfop->bsize = fs->fs_bsize;

	/*
	 * Get the block numbers; we don't handle fragments
	 */
	ndb = howmany(ip->di_size, fs->fs_bsize);
	if (ndb > max_block_count)
		errx(1, "%s: Too many blocks", boot);

	/*
	 * Register block count.
	 */
	bbinfop->nblocks = ndb;

	if (verbose)
		printf("%s: block numbers: ", boot);
	ap = ip->di_db;
	for (i = 0; i < NDADDR && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		bbinfop->blocks[i] = blk + partoffset;
		if (verbose)
			printf("%d ", bbinfop->blocks[i]);
	}
	if (verbose)
		printf("\n");

	if (ndb == 0)
		goto checksum;

	/*
	 * Just one level of indirections; there isn't much room
	 * for more in the 1st-level bootblocks anyway.
	 */
	if (verbose)
		printf("%s: block numbers (indirect): ", boot);
	blk = ip->di_ib[0];
	devread(devfd, buf, blk + partoffset, fs->fs_bsize,
	    "indirect block");
	ap = (daddr_t *)buf;
	for (; i < NINDIR(fs) && *ap && ndb; i++, ap++, ndb--) {
		blk = fsbtodb(fs, *ap);
		bbinfop->blocks[i] = blk + partoffset;
		if (verbose)
			printf("%d ", bbinfop->blocks[i]);
	}
	if (verbose)
		printf("\n");

	if (ndb)
		errx(1, "%s: Too many blocks", boot);

checksum:
	cksum = 0;
	for (i = 0; i < bbinfop->nblocks +
	    (sizeof (*bbinfop) / sizeof (bbinfop->blocks[0])) - 1; i++) {
		cksum += ((int32_t *)bbinfop)[i];
	}
	bbinfop->cksum = -cksum;

	return 0;
}

/* ARGSUSED */
int
loadblocknums_cd9660(boot, devfd, partoffset)
	char *boot;
	int devfd;
	unsigned long partoffset;
{
	u_long blkno, size;
	char *fname;

	fname = strrchr(boot, '/');
	if (fname != NULL)
		fname++;
	else
		fname = boot;

	if (cd9660_lookup(fname, devfd, &blkno, &size))
		errx(1, "unable to find file `%s' in file system", fname);

	setup_contig_blks(blkno, size, ISO_DEFAULT_BLOCK_SIZE,
		ISO_DEFAULT_BLOCK_SIZE, fname);
	return 0;
}

int
loadblocknums_passthru(boot, devfd, partoffset)
	char	*boot;
	int	devfd;
	unsigned long partoffset;
{
	struct stat sb;

	if (stat(boot, &sb))
		err(1, "stat: %s", boot);
	setup_contig_blks(conblockstart, sb.st_size, 512, 512, boot);
	return 0;
}

void
setup_contig_blks(blkno, size, diskblksize, tableblksize, fname)
	u_long blkno, size;
	int diskblksize, tableblksize;
	char *fname;
{
	int i, ndb;
	int32_t cksum;

	ndb = howmany(size, tableblksize);
	if (verbose)
		printf("%s: block number %ld, size %ld table blocks: %d/%d\n",
			dev, blkno, size, ndb, max_block_count);
	if (ndb > max_block_count)
		errx(1, "%s: Too many blocks", fname);

	if (verbose)
		printf("%s: block numbers:", dev);
	for (i = 0; i < ndb; i++) {
		bbinfop->blocks[i] = blkno * (diskblksize / DEV_BSIZE)
				   +   i   * (tableblksize / DEV_BSIZE);
		if (verbose)
			printf(" %d", bbinfop->blocks[i]);
	}
	if (verbose)
		printf("\n");

	bbinfop->bsize = tableblksize;
	bbinfop->nblocks = ndb;

	cksum = 0;
	for (i = 0; i < ndb +
	    (sizeof(*bbinfop) / sizeof(bbinfop->blocks[0])) - 1; i++)
		cksum += ((int32_t *)bbinfop)[i];
	bbinfop->cksum = -cksum;
}
