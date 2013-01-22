/* $NetBSD: setup.c,v 1.39 2013/01/22 09:39:12 dholland Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* #define DKTYPENAMES */
#define FSTYPENAMES
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/file.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern u_int32_t cksum(void *, size_t);
static uint64_t calcmaxfilesize(int);

ufs_daddr_t *din_table;
SEGUSE *seg_table;

#ifdef DKTYPENAMES
int useless(void);

int
useless(void)
{
	char **foo = (char **) dktypenames;
	char **bar = (char **) fscknames;

	return foo - bar;
}
#endif

/*
 * calculate the maximum file size allowed with the specified block shift.
 */
static uint64_t
calcmaxfilesize(int bshift)
{
	uint64_t nptr; /* number of block pointers per block */
	uint64_t maxblock;

	nptr = (1 << bshift) / sizeof(uint32_t);
	maxblock = UFS_NDADDR + nptr + nptr * nptr + nptr * nptr * nptr;

	return maxblock << bshift;
}

void
reset_maxino(ino_t len)
{
	if (debug)
		pwarn("maxino reset from %lld to %lld\n", (long long)maxino,
			(long long)len);

	din_table = erealloc(din_table, len * sizeof(*din_table));
	statemap = erealloc(statemap, len * sizeof(char));
	typemap = erealloc(typemap, len * sizeof(char));
	lncntp = erealloc(lncntp, len * sizeof(int16_t));

	memset(din_table + maxino, 0, (len - maxino) * sizeof(*din_table));
	memset(statemap + maxino, USTATE, (len - maxino) * sizeof(char));
	memset(typemap + maxino, 0, (len - maxino) * sizeof(char));
	memset(lncntp + maxino, 0, (len - maxino) * sizeof(int16_t));

	maxino = len;

	/*
	 * We can't roll forward after allocating new inodes in previous
	 * phases, or thy would conflict (lost+found, for example, might
	 * disappear to be replaced by a file found in roll-forward).
	 */
	no_roll_forward = 1;

	return;
}
 
extern time_t write_time;

int
setup(const char *dev)
{
	long bmapsize;
	struct stat statb;
	int doskipclean;
	u_int64_t maxfilesize;
	int open_flags;
	struct uvnode *ivp;
	struct ubuf *bp;
	int i, isdirty;
	long sn, curseg;
	SEGUSE *sup;

	havesb = 0;
	doskipclean = skipclean;
	if (stat(dev, &statb) < 0) {
		pfatal("Can't stat %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (!S_ISCHR(statb.st_mode) && skipclean) {
		pfatal("%s is not a character device", dev);
		if (reply("CONTINUE") == 0)
			return (0);
	}
	if (nflag)
		open_flags = O_RDONLY;
	else
		open_flags = O_RDWR;

	if ((fsreadfd = open(dev, open_flags)) < 0) {
		pfatal("Can't open %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (nflag) {
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf("** %s (NO WRITE)\n", dev);
		quiet = 0;
	} else if (!preen && !quiet)
		printf("** %s\n", dev);

	fsmodified = 0;
	lfdir = 0;

	/* Initialize time in case we have to write */
	time(&write_time);

	bufinit(0); /* XXX we could make a better guess */
	fs = lfs_init(fsreadfd, bflag, idaddr, 0, debug);
	if (fs == NULL) {
		if (preen)
			printf("%s: ", cdevname());
		errexit("BAD SUPER BLOCK OR IFILE INODE NOT FOUND");
	}

        /* Resize buffer cache now that we have a superblock to guess from. */ 
        bufrehash((fs->lfs_segtabsz + maxino / fs->lfs_ifpb) << 4);

	if (fs->lfs_pflags & LFS_PF_CLEAN) {
		if (doskipclean) {
			if (!quiet)
				pwarn("%sile system is clean; not checking\n",
				      preen ? "f" : "** F");
			return (-1);
		}
		if (!preen)
			pwarn("** File system is already clean\n");
	}

	if (idaddr) {
		daddr_t tdaddr;
		SEGSUM *sp;
		FINFO *fp;
		int bc;

		if (debug)
			pwarn("adjusting offset, serial for -i 0x%lx\n",
				(unsigned long)idaddr);
		tdaddr = sntod(fs, dtosn(fs, idaddr));
		if (sntod(fs, dtosn(fs, tdaddr)) == tdaddr) {
			if (tdaddr == fs->lfs_start)
				tdaddr += btofsb(fs, LFS_LABELPAD);
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				if (fs->lfs_sboffs[i] == tdaddr)
					tdaddr += btofsb(fs, LFS_SBPAD);
				if (fs->lfs_sboffs[i] > tdaddr)
					break;
			}
		}
		fs->lfs_offset = tdaddr;
		if (debug)
			pwarn("begin with offset/serial 0x%x/%d\n",
				(int)fs->lfs_offset, (int)fs->lfs_serial);
		while (tdaddr < idaddr) {
			bread(fs->lfs_devvp, fsbtodb(fs, tdaddr),
			      fs->lfs_sumsize,
			      NULL, 0, &bp);
			sp = (SEGSUM *)bp->b_data;
			if (sp->ss_sumsum != cksum(&sp->ss_datasum,
						   fs->lfs_sumsize -
						   sizeof(sp->ss_sumsum))) {
				brelse(bp, 0);
				if (debug)
					printf("bad cksum at %x\n",
					       (unsigned)tdaddr);
				break;
			}
			fp = (FINFO *)(sp + 1);
			bc = howmany(sp->ss_ninos, INOPB(fs)) <<
				(fs->lfs_version > 1 ? fs->lfs_ffshift :
						       fs->lfs_bshift);
			for (i = 0; i < sp->ss_nfinfo; i++) {
				bc += fp->fi_lastlength + ((fp->fi_nblocks - 1)
					<< fs->lfs_bshift);
				fp = (FINFO *)(fp->fi_blocks + fp->fi_nblocks);
			}

			tdaddr += btofsb(fs, bc) + 1;
			fs->lfs_offset = tdaddr;
			fs->lfs_serial = sp->ss_serial + 1;
			brelse(bp, 0);
		}

		/*
		 * Set curseg, nextseg appropriately -- inlined from
		 * lfs_newseg()
		 */
		curseg = dtosn(fs, fs->lfs_offset);
		fs->lfs_curseg = sntod(fs, curseg);
		for (sn = curseg + fs->lfs_interleave;;) {  
			sn = (sn + 1) % fs->lfs_nseg;
			if (sn == curseg)
				errx(1, "init: no clean segments");
			LFS_SEGENTRY(sup, fs, sn, bp);
			isdirty = sup->su_flags & SEGUSE_DIRTY;
			brelse(bp, 0);

			if (!isdirty)
				break;
		}

		/* Skip superblock if necessary */
		for (i = 0; i < LFS_MAXNUMSB; i++)
			if (fs->lfs_offset == fs->lfs_sboffs[i])
				fs->lfs_offset += btofsb(fs, LFS_SBPAD);

		++fs->lfs_nactive;
		fs->lfs_nextseg = sntod(fs, sn);
		if (debug) {
			pwarn("offset = 0x%" PRIx32 ", serial = %" PRId64 "\n",
				fs->lfs_offset, fs->lfs_serial);
			pwarn("curseg = %" PRIx32 ", nextseg = %" PRIx32 "\n",
				fs->lfs_curseg, fs->lfs_nextseg);
		}

		if (!nflag && !skipclean) {
			fs->lfs_idaddr = idaddr;
			fsmodified = 1;
			sbdirty();
		}
	}

	if (debug) {
		pwarn("idaddr    = 0x%lx\n", idaddr ? (unsigned long)idaddr :
			(unsigned long)fs->lfs_idaddr);
		pwarn("dev_bsize = %lu\n", dev_bsize);
		pwarn("lfs_bsize = %lu\n", (unsigned long) fs->lfs_bsize);
		pwarn("lfs_fsize = %lu\n", (unsigned long) fs->lfs_fsize);
		pwarn("lfs_frag  = %lu\n", (unsigned long) fs->lfs_frag);
		pwarn("lfs_inopb = %lu\n", (unsigned long) fs->lfs_inopb);
	}
	if (fs->lfs_version == 1)
		maxfsblock = fs->lfs_size * (fs->lfs_bsize / dev_bsize);
	else
		maxfsblock = fs->lfs_size;
	maxfilesize = calcmaxfilesize(fs->lfs_bshift);
	if (/* fs->lfs_minfree < 0 || */ fs->lfs_minfree > 99) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
		    fs->lfs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			fs->lfs_minfree = 10;
			sbdirty();
		}
	}
	if (fs->lfs_bmask != fs->lfs_bsize - 1) {
		pwarn("INCORRECT BMASK=0x%x IN SUPERBLOCK (SHOULD BE 0x%x)",
		    (unsigned int) fs->lfs_bmask,
		    (unsigned int) fs->lfs_bsize - 1);
		fs->lfs_bmask = fs->lfs_bsize - 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (fs->lfs_ffmask != fs->lfs_fsize - 1) {
		pwarn("INCORRECT FFMASK=%" PRId64 " IN SUPERBLOCK",
		    fs->lfs_ffmask);
		fs->lfs_ffmask = fs->lfs_fsize - 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (fs->lfs_fbmask != (1 << fs->lfs_fbshift) - 1) {
		pwarn("INCORRECT FBMASK=%" PRId64 " IN SUPERBLOCK",
		    fs->lfs_fbmask);
		fs->lfs_fbmask = (1 << fs->lfs_fbshift) - 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (fs->lfs_maxfilesize != maxfilesize) {
		pwarn(
		    "INCORRECT MAXFILESIZE=%llu IN SUPERBLOCK (SHOULD BE %llu WITH BSHIFT %d)",
		    (unsigned long long) fs->lfs_maxfilesize,
		    (unsigned long long) maxfilesize, (int)fs->lfs_bshift);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			fs->lfs_maxfilesize = maxfilesize;
			sbdirty();
		}
	}
	if (fs->lfs_maxsymlinklen != UFS1_MAXSYMLINKLEN) {
		pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK",
		    fs->lfs_maxsymlinklen);
		fs->lfs_maxsymlinklen = UFS1_MAXSYMLINKLEN;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}

	/*
	 * Read in the Ifile; we'll be using it a lot.
	 * XXX If the Ifile is corrupted we are in bad shape.  We need to
	 * XXX run through the segment headers of the entire disk to
	 * XXX reconstruct the inode table, then pretend all segments are
	 * XXX dirty while we do the rest.
	 */
	ivp = fs->lfs_ivnode;
	maxino = ((VTOI(ivp)->i_ffs1_size - (fs->lfs_cleansz + fs->lfs_segtabsz)
		* fs->lfs_bsize) / fs->lfs_bsize) * fs->lfs_ifpb;
	if (debug)
		pwarn("maxino    = %llu\n", (unsigned long long)maxino);
	for (i = 0; i < VTOI(ivp)->i_ffs1_size; i += fs->lfs_bsize) {
		bread(ivp, i >> fs->lfs_bshift, fs->lfs_bsize, NOCRED, 0, &bp);
		/* XXX check B_ERROR */
		brelse(bp, 0);
	}

	/*
	 * allocate and initialize the necessary maps
	 */
	din_table = ecalloc(maxino, sizeof(*din_table));
	seg_table = ecalloc(fs->lfs_nseg, sizeof(SEGUSE));
	/* Get segment flags */
	for (i = 0; i < fs->lfs_nseg; i++) {
		LFS_SEGENTRY(sup, fs, i, bp);
		seg_table[i].su_flags = sup->su_flags & ~SEGUSE_ACTIVE;
		if (preen)
			seg_table[i].su_nbytes = sup->su_nbytes;
		brelse(bp, 0);
	}

	/* Initialize Ifile entry */
	din_table[fs->lfs_ifile] = fs->lfs_idaddr;
	seg_table[dtosn(fs, fs->lfs_idaddr)].su_nbytes += DINODE1_SIZE;

#ifndef VERBOSE_BLOCKMAP
	bmapsize = roundup(howmany(maxfsblock, NBBY), sizeof(int16_t));
	blockmap = ecalloc(bmapsize, sizeof(char));
#else
	bmapsize = maxfsblock * sizeof(ino_t);
	blockmap = ecalloc(maxfsblock, sizeof(ino_t));
#endif
	statemap = ecalloc(maxino, sizeof(char));
	typemap = ecalloc(maxino, sizeof(char));
	lncntp = ecalloc(maxino, sizeof(int16_t));

	if (preen) {
		n_files = fs->lfs_nfiles;
		n_blks  = fs->lfs_dsize - fs->lfs_bfree;
		numdirs = maxino;
		inplast = 0; 
		listmax = numdirs + 10;
		inpsort = ecalloc(listmax, sizeof(struct inoinfo *));
		inphead = ecalloc(numdirs, sizeof(struct inoinfo *));
	}

	return (1);

	ckfini(0);
	return (0);
}

