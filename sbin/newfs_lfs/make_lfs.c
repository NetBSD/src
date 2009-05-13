/*	$NetBSD: make_lfs.c,v 1.13.4.1 2009/05/13 19:19:04 jym Exp $	*/

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
/*-
 * Copyright (c) 1991, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)lfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: make_lfs.c,v 1.13.4.1 2009/05/13 19:19:04 jym Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/disk.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>

/* Override certain things to make <ufs/lfs/lfs.h> work */
# undef simple_lock
# define simple_lock(x)
# undef simple_unlock
# define simple_unlock(x)
# define vnode uvnode
# define buf ubuf
# define panic call_panic
#include <ufs/lfs/lfs.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "extern.h"

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "segwrite.h"

extern int Nflag; /* Don't write anything */
ufs_daddr_t ifibc; /* How many indirect blocks */

#ifdef MAKE_LF_DIR
# define HIGHEST_USED_INO LOSTFOUNDINO
#else
# define HIGHEST_USED_INO ROOTINO
#endif

static struct lfs lfs_default =  {
	.lfs_dlfs = { /* lfs_dlfs */
		/* dlfs_magic */	LFS_MAGIC,
		/* dlfs_version */	LFS_VERSION,
		/* dlfs_size */		0,
		/* dlfs_ssize */	DFL_LFSSEG,
		/* dlfs_dsize */	0,
		/* dlfs_bsize */	DFL_LFSBLOCK,
		/* dlfs_fsize */	DFL_LFSFRAG,
		/* dlfs_frag */		DFL_LFSBLOCK/DFL_LFSFRAG,
		/* dlfs_freehd */	HIGHEST_USED_INO + 1,
		/* dlfs_bfree */	0,
		/* dlfs_nfiles */	0,
		/* dlfs_avail */	0,
		/* dlfs_uinodes */	0,
		/* dlfs_idaddr */	0,
		/* dlfs_ifile */	LFS_IFILE_INUM,
		/* dlfs_lastseg */	0,
		/* dlfs_nextseg */	0,
		/* dlfs_curseg */	0,
		/* dlfs_offset */	0,
		/* dlfs_lastpseg */	0,
		/* dlfs_inopf */	0,
		/* dlfs_minfree */	MINFREE,
		/* dlfs_maxfilesize */	0,
		/* dlfs_fsbpseg */	0,
		/* dlfs_inopb */	DFL_LFSBLOCK/sizeof(struct ufs1_dinode),
		/* dlfs_ifpb */		DFL_LFSBLOCK/sizeof(IFILE),
		/* dlfs_sepb */		DFL_LFSBLOCK/sizeof(SEGUSE),
		/* XXX ondisk32 */
		/* dlfs_nindir */	DFL_LFSBLOCK/sizeof(int32_t),
		/* dlfs_nseg */		0,
		/* dlfs_nspf */		0,
		/* dlfs_cleansz */	0,
		/* dlfs_segtabsz */	0,
		/* dlfs_segmask */	DFL_LFSSEG_MASK,
		/* dlfs_segshift */	DFL_LFSSEG_SHIFT,
		/* dlfs_bshift */	DFL_LFSBLOCK_SHIFT,
		/* dlfs_ffshift */	DFL_LFS_FFSHIFT,
		/* dlfs_fbshift */	DFL_LFS_FBSHIFT,
		/* dlfs_bmask */	DFL_LFSBLOCK_MASK,
		/* dlfs_ffmask */	DFL_LFS_FFMASK,
		/* dlfs_fbmask */	DFL_LFS_FBMASK,
		/* dlfs_blktodb */	0,
		/* dlfs_sushift */	0,
		/* dlfs_maxsymlinklen */	MAXSYMLINKLEN_UFS1,
		/* dlfs_sboffs */	{ 0 },
		/* dlfs_nclean */       0,
		/* dlfs_fsmnt */        { 0 },
		/* dlfs_pflags */       LFS_PF_CLEAN,
		/* dlfs_dmeta */	0,
		/* dlfs_minfreeseg */   0,
		/* dlfs_sumsize */	0,
		/* dlfs_serial */	0,
		/* dlfs_ibsize */	DFL_LFSFRAG,
		/* dlfs_start */	0,
		/* dlfs_tstamp */       0,
		/* dlfs_inodefmt */     LFS_44INODEFMT,
		/* dlfs_interleave */   0,
		/* dlfs_ident */        0,
		/* dlfs_fsbtodb */      0,
		/* dlfs_resvseg */      0,

		/* dlfs_pad */ 		{ 0 },
		/* dlfs_cksum */	0
	},
};

#define	UMASK	0755

struct direct lfs_root_dir[] = {
	{ ROOTINO, sizeof(struct direct), DT_DIR, 1, "."},
	{ ROOTINO, sizeof(struct direct), DT_DIR, 2, ".."},
	/* { LFS_IFILE_INUM, sizeof(struct direct), DT_REG, 5, "ifile"}, */
#ifdef MAKE_LF_DIR
	{ LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 10, "lost+found"},
#endif
};

#ifdef MAKE_LF_DIR
struct direct lfs_lf_dir[] = {
        { LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 1, "." },
        { ROOTINO, sizeof(struct direct), DT_DIR, 2, ".." },
};
#endif

void pwarn(const char *, ...);
static void make_dinode(ino_t, struct ufs1_dinode *, int, struct lfs *);
static void make_dir( void *, struct direct *, int);
static uint64_t maxfilesize(int);

/*
 * calculate the maximum file size allowed with the specified block shift.
 */
static uint64_t
maxfilesize(int bshift)
{
	uint64_t nptr; /* number of block pointers per block */
	uint64_t maxblock;

	nptr = (1 << bshift) / sizeof(uint32_t);
	maxblock = NDADDR + nptr + nptr * nptr + nptr * nptr * nptr;

	return maxblock << bshift;
}

/*
 * Create the root directory for this file system and the lost+found
 * directory.
 */
static void
make_dinode(ino_t ino, struct ufs1_dinode *dip, int nfrags, struct lfs *fs)
{
	int fsb_per_blk, i;
	int nblocks, bb, base, factor, lvl;

	nblocks = howmany(nfrags, fs->lfs_frag);
	if(nblocks >= NDADDR)
		nfrags = roundup(nfrags, fs->lfs_frag);

	dip->di_nlink = 1;
	dip->di_blocks = fragstofsb(fs, nfrags);

	dip->di_size = (nfrags << fs->lfs_ffshift);
	dip->di_atime = dip->di_mtime = dip->di_ctime = fs->lfs_tstamp;
	dip->di_atimensec = dip->di_mtimensec = dip->di_ctimensec = 0;
	dip->di_inumber = ino;
	dip->di_gen = 1;

	fsb_per_blk = fragstofsb(fs, blkstofrags(fs, 1));

	if (NDADDR < nblocks) {
		/* Count up how many indirect blocks we need, recursively */
		/* XXX We are only called with nblocks > 1 for Ifile */
		bb = nblocks - NDADDR;
		while (bb > 0) {
			bb = howmany(bb, NINDIR(fs));
			ifibc += bb;
			--bb;
		}
		dip->di_blocks += fragstofsb(fs, blkstofrags(fs, ifibc));
	}

	/* Assign the block addresses for the ifile */
	for (i = 0; i < MIN(nblocks,NDADDR); i++) {
		dip->di_db[i] = 0x0;
	}
	if(nblocks > NDADDR) {
		dip->di_ib[0] = 0x0;
		bb = howmany(nblocks - NDADDR, NINDIR(fs)) - 1;
		factor = NINDIR(fs);
		base = -NDADDR - factor;
		lvl = 1;
		while (bb > 0) {
			dip->di_ib[lvl] = 0x0;
			bb = howmany(bb, NINDIR(fs));
			--bb;
			factor *= NINDIR(fs);
			base -= factor;
			++lvl;
		}
	}
}

/*
 * Construct a set of directory entries in "bufp".  We assume that all the
 * entries in protodir fir in the first DIRBLKSIZ.  
 */
static void
make_dir(void *bufp, struct direct *protodir, int entries)
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	for (cp = bufp, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(NEWDIRFMT, &protodir[i], 0);
		memmove(cp, &protodir[i], protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		if ((spcleft -= protodir[i].d_reclen) < 0)
			fatal("%s: %s", special, "directory too big");
	}
	protodir[i].d_reclen = spcleft;
	memmove(cp, &protodir[i], DIRSIZ(NEWDIRFMT, &protodir[i], 0));
}

int
make_lfs(int devfd, uint secsize, struct dkwedge_info *dkw, int minfree,
	 int block_size, int frag_size, int seg_size, int minfreeseg,
	 int resvseg, int version, daddr_t start, int ibsize, int interleave,
	 u_int32_t roll_id)
{
	struct ufs1_dinode *dip;	/* Pointer to a disk inode */
	CLEANERINFO *cip;	/* Segment cleaner information table */
	IFILE *ip;		/* Pointer to array of ifile structures */
	IFILE_V1 *ip_v1 = NULL;
	struct lfs *fs;		/* Superblock */
	SEGUSE *segp;		/* Segment usage table */
	daddr_t	sb_addr;	/* Address of superblocks */
	daddr_t	seg_addr;	/* Address of current segment */
	int bsize;		/* Block size */
	int fsize;		/* Fragment size */
	int db_per_blk;		/* Disk blocks per file block */
	int i, j;
	int sb_interval;	/* number of segs between super blocks */
	int ssize;		/* Segment size */
	double fssize;
	int warned_segtoobig=0;
	int label_fsb, sb_fsb;
	int curw, ww;
	char tbuf[BUFSIZ];
	struct ubuf *bp;
	struct uvnode *vp, *save_devvp;
	int bb, ubb, dmeta, labelskew;
	u_int64_t tsepb, tnseg;

	/*
	 * Initialize buffer cache.  Use a ballpark guess of the length of
	 * the segment table for the number of hash chains.
	 */
	tnseg = dkw->dkw_size / ((seg_size ? seg_size : DFL_LFSSEG) / secsize);
	tsepb = (block_size ? block_size : DFL_LFSBLOCK) / sizeof(SEGSUM);
	if (tnseg == 0)
		fatal("zero size partition");
	bufinit(tnseg / tsepb);

	/* Initialize LFS subsystem with blank superblock and ifile. */
	fs = lfs_init(devfd, start, (ufs_daddr_t)0, 1, 1/* XXX debug*/);
	save_devvp = fs->lfs_devvp;
	vp = fs->lfs_ivnode;
	*fs = lfs_default;
	fs->lfs_ivnode = vp;
	fs->lfs_devvp = save_devvp;


	/* Set version first of all since it is used to compute other fields */
	fs->lfs_version = version;

	/* If partition is not an LFS partition, warn that that is the case */
	if (strcmp(dkw->dkw_ptype, DKW_PTYPE_LFS) != 0) {
		fatal("partition label indicated fs type \"%s\", "
		    "expected \"%s\"", dkw->dkw_ptype, DKW_PTYPE_LFS);
	}

	if (!(bsize = block_size))
		bsize = DFL_LFSBLOCK;
	if (!(fsize = frag_size))
		fsize = DFL_LFSFRAG;
	if (!(ssize = seg_size)) {
		ssize = DFL_LFSSEG;
	}
	if (version > 1) {
		if (ibsize == 0)
			ibsize = fsize;
		if (ibsize <= 0 || ibsize % fsize)
			fatal("illegal inode block size: %d\n", ibsize);
	} else if (ibsize && ibsize != bsize)
		fatal("cannot specify inode block size when version == 1\n");

	/* Sanity check: fsize<=bsize<ssize */
	if (fsize > bsize) {
		/* Only complain if fsize was explicitly set */
		if(frag_size)
			fatal("fragment size must be <= block size %d", bsize);
		fsize = bsize;
	}
	if (bsize >= ssize) {
		/* Only fatal if ssize was explicitly set */
		if(seg_size)
			fatal("block size must be < segment size");
		warnx("%s: disklabel segment size (%d) too small, using default (%d)",
		      progname, ssize, DFL_LFSSEG);
		ssize = DFL_LFSSEG;
	}
	if (start < 0 || start >= dkw->dkw_size)
		fatal("filesystem offset %ld out of range", (long)start);
	if (version == 1) {
		if (start)
			warnx("filesystem offset ignored for version 1 filesystem");
		start = LFS_LABELPAD / secsize;
	}

    tryagain:
	/* Modify parts of superblock overridden by command line arguments */
	if (bsize != DFL_LFSBLOCK || fsize != DFL_LFSFRAG) {
		fs->lfs_bshift = lfs_log2(bsize);
		if (1 << fs->lfs_bshift != bsize)
			fatal("%d: block size not a power of 2", bsize);
		fs->lfs_bsize = bsize;
		fs->lfs_fsize = fsize;
		fs->lfs_bmask = bsize - 1;
		fs->lfs_ffmask = fsize - 1;
		fs->lfs_ffshift = lfs_log2(fsize);
		if (1 << fs->lfs_ffshift != fsize)
			fatal("%d: frag size not a power of 2", fsize);
		fs->lfs_frag = numfrags(fs, bsize);
		fs->lfs_fbmask = fs->lfs_frag - 1;
		fs->lfs_fbshift = lfs_log2(fs->lfs_frag);
		fs->lfs_ifpb = bsize / sizeof(IFILE);
		/* XXX ondisk32 */
		fs->lfs_nindir = bsize / sizeof(int32_t);
	}

	if (fs->lfs_version == 1) {
		fs->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		fs->lfs_segshift = lfs_log2(ssize);
		if (1 << fs->lfs_segshift != ssize)
			fatal("%d: segment size not power of 2", ssize);
		fs->lfs_segmask = ssize - 1;
		fs->lfs_ifpb = fs->lfs_bsize / sizeof(IFILE_V1);
		fs->lfs_ibsize = fs->lfs_bsize;
		fs->lfs_sepb = bsize / sizeof(SEGUSE_V1);
		fs->lfs_ssize = ssize >> fs->lfs_bshift;
	} else {
		if (ssize % fsize) {
			fprintf(stderr, 
				"Segment size %d is not a multiple of frag size; ",
				 ssize);
			ssize = roundup(ssize, fsize);
			fprintf(stderr, "trying size %d.\n", ssize);
			goto tryagain;
		}
		fs->lfs_sumsize = fsize;
		fs->lfs_segshift = 0;
		fs->lfs_segmask = 0;
		fs->lfs_sepb = bsize / sizeof(SEGUSE);
		fs->lfs_ssize = ssize;
		fs->lfs_ibsize = ibsize;
	}
	fs->lfs_inopb = fs->lfs_ibsize / sizeof(struct ufs1_dinode);
	fs->lfs_minfree = minfree;

	if (version > 1) {
		fs->lfs_inopf = secsize/DINODE1_SIZE;
		fs->lfs_interleave = interleave;
		if (roll_id == 0)
			roll_id = arc4random();
		fs->lfs_ident = roll_id;
	}

	/*
	 * Fill in parts of superblock that can be computed from file system
	 * size, disk geometry and current time.
	 */
	db_per_blk = bsize/secsize;
	fs->lfs_blktodb = lfs_log2(db_per_blk);
	fs->lfs_fsbtodb = lfs_log2(fsize / secsize);
	if (version == 1) {
		fs->lfs_sushift = lfs_log2(fs->lfs_sepb);
		fs->lfs_fsbtodb = 0;
		fs->lfs_size = dkw->dkw_size >> fs->lfs_blktodb;
	}
	label_fsb = btofsb(fs, roundup(LFS_LABELPAD, fsize));
	sb_fsb = btofsb(fs, roundup(LFS_SBPAD, fsize));
	fs->lfs_fsbpseg = dbtofsb(fs, ssize / secsize);
	fs->lfs_size = dkw->dkw_size >> fs->lfs_fsbtodb;
	fs->lfs_dsize = dbtofsb(fs, dkw->dkw_size) -
		MAX(label_fsb, dbtofsb(fs, start));
	fs->lfs_nseg = fs->lfs_dsize / segtod(fs, 1);

	fs->lfs_nclean = fs->lfs_nseg - 1;
	fs->lfs_maxfilesize = maxfilesize(fs->lfs_bshift);

	if (minfreeseg == 0)
		fs->lfs_minfreeseg = fs->lfs_nseg / DFL_MIN_FREE_SEGS;
	else
		fs->lfs_minfreeseg = minfreeseg;
	if (fs->lfs_minfreeseg < MIN_FREE_SEGS)
		fs->lfs_minfreeseg = MIN_FREE_SEGS;

	if (resvseg == 0)
		fs->lfs_resvseg = fs->lfs_minfreeseg / 2 + 1;
	else
		fs->lfs_resvseg = resvseg;
	if (fs->lfs_resvseg < MIN_RESV_SEGS)
		fs->lfs_resvseg = MIN_RESV_SEGS;

	if(fs->lfs_nseg < fs->lfs_minfreeseg + 1
	   || fs->lfs_nseg < LFS_MIN_SBINTERVAL + 1)
	{
		if(seg_size == 0 && ssize > (bsize<<1)) {
			if(!warned_segtoobig) {
				fprintf(stderr,"Segment size %d is too large; "
					"trying smaller sizes.\n", ssize);
				if (ssize == (bsize << 16)) {
					fprintf(stderr, "(Did you perhaps "
						"accidentally leave \"16\" "
						"in the disklabel's sgs "
						"field?)\n");
				}
			}
			++warned_segtoobig;
			ssize >>= 1;
			goto tryagain;
		}
		fatal("Could not allocate enough segments with segment "
			"size %d and block size %d;\nplease decrease the "
			"segment size.\n", ssize, fs->lfs_bsize);
	}

	/*
	 * Now that we've determined what we're going to do, announce it
	 * to the user.
	 */
        printf("Creating a version %d LFS", fs->lfs_version);
        if (fs->lfs_version > 1)
                printf(" with roll-forward ident 0x%x", fs->lfs_ident);
        printf("\n");   
        fssize = (double)fs->lfs_nseg;
        fssize *= (double)ssize;
        fssize /= 1048576.0;
        printf("%.1fMB in %d segments of size %d\n", fssize,
               fs->lfs_nseg, ssize);

	/* 
	 * The number of free blocks is set from the number of segments
	 * times the segment size - lfs_minfreesegs (that we never write
	 * because we need to make sure the cleaner can run).  Then
	 * we'll subtract off the room for the superblocks ifile entries
	 * and segment usage table, and half a block per segment that can't
	 * be written due to fragmentation.
	 */
	fs->lfs_dsize = (fs->lfs_nseg - fs->lfs_minfreeseg) *
		segtod(fs, 1);
	fs->lfs_bfree = fs->lfs_dsize;
	fs->lfs_bfree -= dbtofsb(fs, ((fs->lfs_nseg / 2) << 
		fs->lfs_blktodb));

	fs->lfs_segtabsz = SEGTABSIZE_SU(fs);
	fs->lfs_cleansz = CLEANSIZE_SU(fs);
	if ((fs->lfs_tstamp = time(NULL)) == -1)
		fatal("time: %s", strerror(errno));
	if (version == 1)
		fs->lfs_otstamp = fs->lfs_tstamp;

	if ((sb_interval = fs->lfs_nseg / LFS_MAXNUMSB) < LFS_MIN_SBINTERVAL)
		sb_interval = LFS_MIN_SBINTERVAL;

	/*
	 * Figure out where the superblocks are going to live.
	 *
	 * Make segment 0 start at either zero, or LFS_LABELPAD, or
	 * >= LFS_SBPAD+LFS_LABELPAD, in order to prevent segment 0
	 * from having half a superblock in it.
	 */
	if (fsbtodb(fs, dbtofsb(fs, start)) != start)
		fatal("Segment 0 offset is not multiple of frag size\n");
	if (start != 0 && dbtob(start) != LFS_LABELPAD &&
	    dbtob(start) < LFS_SBPAD + LFS_LABELPAD) {
		fatal("Using flags \"-O %" PRId64 "\" would result in the "
		      "first segment containing only\npart of a superblock.  "
		      "Please choose an offset of 0, %d, or %d or more,\n",
		      start, btodb(LFS_LABELPAD),
		      btodb(LFS_LABELPAD + LFS_SBPAD));
	}
	fs->lfs_sboffs[0] = label_fsb;
	if (version == 1)
		fs->lfs_start = fs->lfs_sboffs[0];
	else
		fs->lfs_start = dbtofsb(fs, start);
        fs->lfs_dsize -= sb_fsb;
	for (i = 1; i < LFS_MAXNUMSB; i++) {
		sb_addr = ((i * sb_interval) * segtod(fs, 1))
		    + fs->lfs_sboffs[0];
		/* Segment 0 eats the label, except for version 1 */
		if (fs->lfs_version > 1 && fs->lfs_start < label_fsb)
			sb_addr -= label_fsb - start;
		if (sb_addr + sizeof(struct dlfs)
		    >= dbtofsb(fs, dkw->dkw_size))
			break;
		fs->lfs_sboffs[i] = sb_addr;
		fs->lfs_dsize -= sb_fsb;
	}

	/* We need >= 2 superblocks */
	if(fs->lfs_sboffs[1] == 0x0) {
		fatal("Could not assign a disk address for the second "
		      "superblock.\nPlease decrease the segment size.\n");
	}

	fs->lfs_lastseg = sntod(fs, fs->lfs_nseg - 2);
	fs->lfs_curseg = sntod(fs, fs->lfs_nseg - 1);
	fs->lfs_offset = sntod(fs, fs->lfs_nseg);
	fs->lfs_nextseg = sntod(fs, 0);

	/*
	 * Initialize the Ifile inode.  Do this before we do anything
	 * with the Ifile or segment tables.
	 */
	dip = VTOI(fs->lfs_ivnode)->i_din.ffs1_din = (struct ufs1_dinode *)
		malloc(sizeof(*dip));
	if (dip == NULL)
		err(1, NULL);
	memset(dip, 0, sizeof(*dip));
	dip->di_mode  = IFREG|IREAD|IWRITE;
	dip->di_flags = SF_IMMUTABLE;
	make_dinode(LFS_IFILE_INUM, dip,
		blkstofrags(fs, fs->lfs_cleansz + fs->lfs_segtabsz + 1), fs);
	dip->di_size = (fs->lfs_cleansz + fs->lfs_segtabsz + 1) << fs->lfs_bshift;
	for (i = 0; i < NDADDR && i < (dip->di_size >> fs->lfs_bshift); i++)
		VTOI(fs->lfs_ivnode)->i_lfs_fragsize[i] = fs->lfs_bsize;

	/*
	 * Set up in-superblock segment usage cache
	 */
 	fs->lfs_suflags = (u_int32_t **) malloc(2 * sizeof(u_int32_t *));       
	if (fs->lfs_suflags == NULL)
		err(1, NULL);
	fs->lfs_suflags[0] = (u_int32_t *) malloc(fs->lfs_nseg * sizeof(u_int32_t));
	if (fs->lfs_suflags[0] == NULL)
		err(1, NULL);
	fs->lfs_suflags[1] = (u_int32_t *) malloc(fs->lfs_nseg * sizeof(u_int32_t));
	if (fs->lfs_suflags[1] == NULL)
		err(1, NULL);

	/*
	 * Initialize the cleanerinfo block
	 */
	LFS_CLEANERINFO(cip, fs, bp);
	cip->clean = fs->lfs_nseg;
	cip->dirty = 0;
	if (version > 1) {
		cip->free_head = HIGHEST_USED_INO + 1;
		cip->free_tail = fs->lfs_ifpb - 1;
	}
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);

	/*
	 * Run through segment table and initialize that
	 */
	for (i = j = 0; i < fs->lfs_nseg; i++) {
		LFS_SEGENTRY(segp, fs, i, bp);

		if (i == 0 &&
		    fs->lfs_start < btofsb(fs, LFS_LABELPAD + LFS_SBPAD)) {
			segp->su_flags = SEGUSE_SUPERBLOCK;
			fs->lfs_bfree -= sb_fsb;
			++j;
		}
		if (i > 0) { 
			if ((i % sb_interval) == 0 && j < LFS_MAXNUMSB) {
				segp->su_flags = SEGUSE_SUPERBLOCK;
				fs->lfs_bfree -= sb_fsb;
				++j;
			} else
				segp->su_flags = 0;
		}
		segp->su_lastmod = 0;
		segp->su_nbytes = 0;
		segp->su_ninos = 0;
		segp->su_nsums = 0;
		
		LFS_WRITESEGENTRY(segp, fs, i, bp);
	}

	/* Initialize root directory */
	vp = lfs_raw_vget(fs, ROOTINO, devfd, 0x0);
	dip = VTOI(vp)->i_din.ffs1_din;
	make_dinode(ROOTINO, dip, howmany(DIRBLKSIZ,fs->lfs_fsize), fs);
	dip->di_mode = IFDIR | UMASK;
	VTOI(vp)->i_lfs_osize = dip->di_size = DIRBLKSIZ;
#ifdef MAKE_LF_DIR
	VTOI(vp)->i_nlink = dip->di_nlink = 3;
#else
	VTOI(vp)->i_nlink = dip->di_nlink = 2;
#endif
        VTOI(vp)->i_lfs_effnblks = dip->di_blocks =
		btofsb(fs, roundup(DIRBLKSIZ,fs->lfs_fsize));
	for (i = 0; i < NDADDR && i < howmany(DIRBLKSIZ, fs->lfs_bsize); i++)
		VTOI(vp)->i_lfs_fragsize[i] = fs->lfs_bsize;
	if (DIRBLKSIZ < fs->lfs_bsize)
		VTOI(vp)->i_lfs_fragsize[i - 1] =
			roundup(DIRBLKSIZ,fs->lfs_fsize);
	bread(vp, 0, fs->lfs_fsize, NOCRED, 0, &bp);
	make_dir(bp->b_data, lfs_root_dir, 
		 sizeof(lfs_root_dir) / sizeof(struct direct));
	VOP_BWRITE(bp);

#ifdef MAKE_LF_DIR
	/* Initialize lost+found directory */
	vp = lfs_raw_vget(fs, LOSTFOUNDINO, devfd, 0x0);
	dip = VTOI(vp)->i_din.ffs1_din;
	make_dinode(LOSTFOUNDINO, dip, howmany(DIRBLKSIZ,fs->lfs_fsize), fs);
	dip->di_mode = IFDIR | UMASK;
	VTOI(vp)->i_lfs_osize = dip->di_size = DIRBLKSIZ;
        VTOI(vp)->i_nlink = dip->di_nlink = 2;
        VTOI(vp)->i_lfs_effnblks = dip->di_blocks =
		btofsb(fs, roundup(DIRBLKSIZ,fs->lfs_fsize));
	for (i = 0; i < NDADDR && i < howmany(DIRBLKSIZ, fs->lfs_bsize); i++)
		VTOI(vp)->i_lfs_fragsize[i] = fs->lfs_bsize;
	if (DIRBLKSIZ < fs->lfs_bsize)
		VTOI(vp)->i_lfs_fragsize[i - 1] =
			roundup(DIRBLKSIZ,fs->lfs_fsize);
	bread(vp, 0, fs->lfs_fsize, NOCRED, 0, &bp);
	make_dir(bp->b_data, lfs_lf_dir, 
		 sizeof(lfs_lf_dir) / sizeof(struct direct));
	VOP_BWRITE(bp);
#endif /* MAKE_LF_DIR */

	/* Set their IFILE entry version numbers to 1 */
	LFS_IENTRY(ip, fs, 1, bp);
	if (version == 1) {
		ip_v1 = (IFILE_V1 *)ip;
		for (i = LFS_IFILE_INUM; i <= HIGHEST_USED_INO; i++) {
			ip_v1->if_version = 1;
			ip_v1->if_daddr = 0x0;
			ip_v1->if_nextfree = 0;
			++ip_v1;
		}
	} else {
		for (i = LFS_IFILE_INUM; i <= HIGHEST_USED_INO; i++) {
			ip->if_version = 1;
			ip->if_daddr = 0x0;
			ip->if_nextfree = 0;
			++ip;
		}
	}
	/* Link remaining IFILE entries in free list */
	if (version == 1) {
		for (;
		     i < fs->lfs_ifpb; ++ip_v1) {
			ip_v1->if_version = 1;
			ip_v1->if_daddr = LFS_UNUSED_DADDR;
			ip_v1->if_nextfree = ++i;
		}
		--ip_v1;
		ip_v1->if_nextfree = LFS_UNUSED_INUM;
	} else {
		for (;
		     i < fs->lfs_ifpb; ++ip) {
			ip->if_version = 1;
			ip->if_daddr = LFS_UNUSED_DADDR;
			ip->if_nextfree = ++i;
		}
		--ip;
		ip->if_nextfree = LFS_UNUSED_INUM;
	}
	VOP_BWRITE(bp);

	/* Write it all to disk. */
	if (!Nflag)
		lfs_segwrite(fs, SEGM_CKP);

	/*
	 * Now that we've written everything, look to see what's available
	 * for writing.
	 */
	fs->lfs_avail = 0;
	bb = ubb = dmeta = 0;
	for (i = 0; i < fs->lfs_nseg; i++) {
		LFS_SEGENTRY(segp, fs, i, bp);
                if (segp->su_flags & SEGUSE_DIRTY) {
                        bb += btofsb(fs, segp->su_nbytes +
                            segp->su_nsums * fs->lfs_sumsize);
                        ubb += btofsb(fs, segp->su_nbytes +
                            segp->su_nsums * fs->lfs_sumsize +
                            segp->su_ninos * fs->lfs_ibsize);
                        dmeta += btofsb(fs,
                            fs->lfs_sumsize * segp->su_nsums);
                        dmeta += btofsb(fs,
                            fs->lfs_ibsize * segp->su_ninos);
		} else {
                        fs->lfs_avail += segtod(fs, 1);
                        if (segp->su_flags & SEGUSE_SUPERBLOCK)
                                fs->lfs_avail -= btofsb(fs, LFS_SBPAD);
                        if (i == 0 && fs->lfs_version > 1 &&
                            fs->lfs_start < btofsb(fs, LFS_LABELPAD))
                                fs->lfs_avail -= btofsb(fs, LFS_LABELPAD) -
                                    fs->lfs_start;
                }
		brelse(bp, 0);
        }
        /* Also may be available bytes in current seg */
        i = dtosn(fs, fs->lfs_offset);
        fs->lfs_avail += sntod(fs, i + 1) - fs->lfs_offset;
        /* But do not count minfreesegs */
        fs->lfs_avail -= segtod(fs, (fs->lfs_minfreeseg - (fs->lfs_minfreeseg / 2)));

        labelskew = 0;
        if (fs->lfs_version > 1 && fs->lfs_start < btofsb(fs, LFS_LABELPAD))
                labelskew = btofsb(fs, LFS_LABELPAD);
        fs->lfs_bfree = fs->lfs_dsize - labelskew - (ubb + bb) / 2;

	/* Put that in the Ifile version too, and write it */
	LFS_CLEANERINFO(cip, fs, bp);
	cip->bfree = fs->lfs_bfree;
	cip->avail = fs->lfs_avail;
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);
	if (!Nflag)
		lfs_segwrite(fs, SEGM_CKP);

	/*
	 * Finally write out superblocks.
	 */
	printf("super-block backups (for fsck -b #) at:\n");
	curw = 0;
	for (i = 0; i < LFS_MAXNUMSB; i++) {
		seg_addr = fs->lfs_sboffs[i];
		if (seg_addr == 0)
			break;

		if (i != 0)
			curw += printf(", ");
		ww = snprintf(tbuf, sizeof(tbuf), "%lld",
		    (long long)fsbtodb(fs, seg_addr));
		curw += ww;
		if (curw >= 78) {
			printf("\n%s", tbuf);
			curw = ww;
		} else
			printf("%s", tbuf);
		fflush(stdout);

		/* Leave the time stamp on the alt sb, zero the rest */
		if (i == 2) {
			fs->lfs_tstamp = 0;
			fs->lfs_cksum = lfs_sb_cksum(&(fs->lfs_dlfs));
		}
		if (!Nflag)
			lfs_writesuper(fs, seg_addr);
	}
	printf(".\n");

	return 0;
}

/*
 * Compatibility with fsck_lfs, since the "generic" LFS userland code uses it.
 */
void
pwarn(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
}
