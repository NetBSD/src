/* $NetBSD: installboot.c,v 1.14.8.1 2001/03/12 13:27:07 bouyer Exp $ */

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

#include "stand/common/bbinfo.h"

int	verbose, nowrite, hflag, conblockmode, conblockstart;
char	*boot, *proto, *dev;

struct bbinfoloc *bbinfolocp;
struct bbinfo *bbinfop;
int	max_block_count;


void		setup_contig_blks(u_long, u_long, int, int, char *);
char		*loadprotoblocks __P((char *, long *));
int		loadblocknums_passthru __P((char *, int, unsigned long));
static void	usage __P((void));
int 		main __P((int, char *[]));

static void
usage()
{
	fprintf(stderr,
	    "usage: %s [-n] [-v] -b blkno boot proto dev\n", getprogname());
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

	if (!conblockmode)
		usage();

	loadblocknums_func = loadblocknums_passthru;

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
