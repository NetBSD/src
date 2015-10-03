/* $NetBSD: setup.c,v 1.60 2015/10/03 08:29:21 dholland Exp $ */

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

#define vnode uvnode
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>
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
static uint64_t calcmaxfilesize(unsigned);

daddr_t *din_table;
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
calcmaxfilesize(unsigned bshift)
{
	uint64_t nptr; /* number of block pointers per block */
	uint64_t maxblock;

	nptr = (1 << bshift) / LFS_BLKPTRSIZE(fs);
	maxblock = ULFS_NDADDR + nptr + nptr * nptr + nptr * nptr * nptr;

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
	size_t sumstart;

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
        bufrehash((lfs_sb_getsegtabsz(fs) + maxino / lfs_sb_getifpb(fs)) << 4);

	if (lfs_sb_getpflags(fs) & LFS_PF_CLEAN) {
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
			pwarn("adjusting offset, serial for -i 0x%jx\n",
				(uintmax_t)idaddr);
		tdaddr = lfs_sntod(fs, lfs_dtosn(fs, idaddr));
		if (lfs_sntod(fs, lfs_dtosn(fs, tdaddr)) == tdaddr) {
			if (tdaddr == lfs_sb_gets0addr(fs))
				tdaddr += lfs_btofsb(fs, LFS_LABELPAD);
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				if (lfs_sb_getsboff(fs, i) == tdaddr)
					tdaddr += lfs_btofsb(fs, LFS_SBPAD);
				if (lfs_sb_getsboff(fs, i) > tdaddr)
					break;
			}
		}
		lfs_sb_setoffset(fs, tdaddr);
		if (debug)
			pwarn("begin with offset/serial 0x%jx/%jd\n",
				(uintmax_t)lfs_sb_getoffset(fs),
				(intmax_t)lfs_sb_getserial(fs));
		while (tdaddr < idaddr) {
			bread(fs->lfs_devvp, LFS_FSBTODB(fs, tdaddr),
			      lfs_sb_getsumsize(fs),
			      0, &bp);
			sp = (SEGSUM *)bp->b_data;
			sumstart = lfs_ss_getsumstart(fs);
			if (lfs_ss_getsumsum(fs, sp) !=
			    cksum((char *)sp + sumstart,
				  lfs_sb_getsumsize(fs) - sumstart)) {
				brelse(bp, 0);
				if (debug)
					printf("bad cksum at %jx\n",
					       (uintmax_t)tdaddr);
				break;
			}
			fp = SEGSUM_FINFOBASE(fs, sp);
			bc = howmany(lfs_ss_getninos(fs, sp), LFS_INOPB(fs)) <<
				(lfs_sb_getversion(fs) > 1 ? lfs_sb_getffshift(fs) :
						       lfs_sb_getbshift(fs));
			for (i = 0; i < lfs_ss_getnfinfo(fs, sp); i++) {
				bc += lfs_fi_getlastlength(fs, fp) + ((lfs_fi_getnblocks(fs, fp) - 1)
					<< lfs_sb_getbshift(fs));
				fp = NEXT_FINFO(fs, fp);
			}

			tdaddr += lfs_btofsb(fs, bc) + 1;
			lfs_sb_setoffset(fs, tdaddr);
			lfs_sb_setserial(fs, lfs_ss_getserial(fs, sp) + 1);
			brelse(bp, 0);
		}

		/*
		 * Set curseg, nextseg appropriately -- inlined from
		 * lfs_newseg()
		 */
		curseg = lfs_dtosn(fs, lfs_sb_getoffset(fs));
		lfs_sb_setcurseg(fs, lfs_sntod(fs, curseg));
		for (sn = curseg + lfs_sb_getinterleave(fs);;) {  
			sn = (sn + 1) % lfs_sb_getnseg(fs);
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
			if (lfs_sb_getoffset(fs) == lfs_sb_getsboff(fs, i))
				lfs_sb_addoffset(fs, lfs_btofsb(fs, LFS_SBPAD));

		++fs->lfs_nactive;
		lfs_sb_setnextseg(fs, lfs_sntod(fs, sn));
		if (debug) {
			pwarn("offset = 0x%" PRIx64 ", serial = %" PRIu64 "\n",
				lfs_sb_getoffset(fs), lfs_sb_getserial(fs));
			pwarn("curseg = %" PRIx64 ", nextseg = %" PRIx64 "\n",
				lfs_sb_getcurseg(fs), lfs_sb_getnextseg(fs));
		}

		if (!nflag && !skipclean) {
			lfs_sb_setidaddr(fs, idaddr);
			fsmodified = 1;
			sbdirty();
		}
	}

	if (debug) {
		pwarn("idaddr    = 0x%jx\n", idaddr ? (uintmax_t)idaddr :
			(uintmax_t)lfs_sb_getidaddr(fs));
		pwarn("dev_bsize = %lu\n", dev_bsize);
		pwarn("lfs_bsize = %lu\n", (unsigned long) lfs_sb_getbsize(fs));
		pwarn("lfs_fsize = %lu\n", (unsigned long) lfs_sb_getfsize(fs));
		pwarn("lfs_frag  = %lu\n", (unsigned long) lfs_sb_getfrag(fs));
		pwarn("lfs_inopb = %lu\n", (unsigned long) lfs_sb_getinopb(fs));
	}
	if (lfs_sb_getversion(fs) == 1)
		maxfsblock = lfs_blkstofrags(fs, lfs_sb_getsize(fs));
	else
		maxfsblock = lfs_sb_getsize(fs);
	maxfilesize = calcmaxfilesize(lfs_sb_getbshift(fs));
	if (/* lfs_sb_getminfree(fs) < 0 || */ lfs_sb_getminfree(fs) > 99) {
		pfatal("IMPOSSIBLE MINFREE=%u IN SUPERBLOCK",
		    lfs_sb_getminfree(fs));
		if (reply("SET TO DEFAULT") == 1) {
			lfs_sb_setminfree(fs, 10);
			sbdirty();
		}
	}
	if (lfs_sb_getbmask(fs) != lfs_sb_getbsize(fs) - 1) {
		pwarn("INCORRECT BMASK=0x%jx IN SUPERBLOCK (SHOULD BE 0x%x)",
		    (uintmax_t)lfs_sb_getbmask(fs),
		    lfs_sb_getbsize(fs) - 1);
		lfs_sb_setbmask(fs, lfs_sb_getbsize(fs) - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (lfs_sb_getffmask(fs) != lfs_sb_getfsize(fs) - 1) {
		pwarn("INCORRECT FFMASK=0x%jx IN SUPERBLOCK (SHOULD BE 0x%x)",
		    (uintmax_t)lfs_sb_getffmask(fs),
		    lfs_sb_getfsize(fs) - 1);
		lfs_sb_setffmask(fs, lfs_sb_getfsize(fs) - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (lfs_sb_getfbmask(fs) != (1U << lfs_sb_getfbshift(fs)) - 1) {
		pwarn("INCORRECT FBMASK=0x%jx IN SUPERBLOCK (SHOULD BE 0x%x)",
		    (uintmax_t)lfs_sb_getfbmask(fs),
		      (1U << lfs_sb_getfbshift(fs)) - 1);
		lfs_sb_setfbmask(fs, (1U << lfs_sb_getfbshift(fs)) - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
		}
	}
	if (lfs_sb_getmaxfilesize(fs) != maxfilesize) {
		pwarn(
		    "INCORRECT MAXFILESIZE=%ju IN SUPERBLOCK (SHOULD BE %ju WITH BSHIFT %u)",
		    (uintmax_t) lfs_sb_getmaxfilesize(fs),
		    (uintmax_t) maxfilesize, lfs_sb_getbshift(fs));
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			lfs_sb_setmaxfilesize(fs, maxfilesize);
			sbdirty();
		}
	}
	if (lfs_sb_getmaxsymlinklen(fs) != LFS_MAXSYMLINKLEN(fs)) {
		pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK (SHOULD BE %zu)",
		    lfs_sb_getmaxsymlinklen(fs), LFS_MAXSYMLINKLEN(fs));
		lfs_sb_setmaxsymlinklen(fs, LFS_MAXSYMLINKLEN(fs));
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
	maxino = ((lfs_dino_getsize(fs, VTOI(ivp)->i_din) - (lfs_sb_getcleansz(fs) + lfs_sb_getsegtabsz(fs))
		* lfs_sb_getbsize(fs)) / lfs_sb_getbsize(fs)) * lfs_sb_getifpb(fs);
	if (debug)
		pwarn("maxino    = %llu\n", (unsigned long long)maxino);
	for (i = 0; i < lfs_dino_getsize(fs, VTOI(ivp)->i_din); i += lfs_sb_getbsize(fs)) {
		bread(ivp, i >> lfs_sb_getbshift(fs), lfs_sb_getbsize(fs), 0, &bp);
		/* XXX check B_ERROR */
		brelse(bp, 0);
	}

	/*
	 * allocate and initialize the necessary maps
	 */
	din_table = ecalloc(maxino, sizeof(*din_table));
	seg_table = ecalloc(lfs_sb_getnseg(fs), sizeof(SEGUSE));
	/* Get segment flags */
	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		LFS_SEGENTRY(sup, fs, i, bp);
		seg_table[i].su_flags = sup->su_flags & ~SEGUSE_ACTIVE;
		if (preen)
			seg_table[i].su_nbytes = sup->su_nbytes;
		brelse(bp, 0);
	}

	/* Initialize Ifile entry */
	din_table[LFS_IFILE_INUM] = lfs_sb_getidaddr(fs);
	seg_table[lfs_dtosn(fs, lfs_sb_getidaddr(fs))].su_nbytes += DINOSIZE(fs);

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
		n_files = lfs_sb_getnfiles(fs);
		n_blks  = lfs_sb_getdsize(fs) - lfs_sb_getbfree(fs);
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

