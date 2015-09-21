/*	$NetBSD: make_lfs.c,v 1.57 2015/09/21 01:24:58 dholland Exp $	*/

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
__RCSID("$NetBSD: make_lfs.c,v 1.57 2015/09/21 01:24:58 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/disk.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/stat.h>

/* Override certain things to make <ufs/lfs/lfs.h> work */
# undef simple_lock
# define simple_lock(x)
# undef simple_unlock
# define simple_unlock(x)
# define vnode uvnode
# define buf ubuf
# define panic call_panic
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_inode.h>

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

static blkcnt_t ifibc; /* How many indirect blocks */

#ifdef MAKE_LF_DIR
# define HIGHEST_USED_INO LOSTFOUNDINO
#else
# define HIGHEST_USED_INO ULFS_ROOTINO
#endif

static const struct dlfs dlfs32_default = {
		.dlfs_magic =		LFS_MAGIC,
		.dlfs_version =		LFS_VERSION,
		.dlfs_size =		0,
		.dlfs_ssize =		DFL_LFSSEG,
		.dlfs_dsize =		0,
		.dlfs_bsize =		DFL_LFSBLOCK,
		.dlfs_fsize =		DFL_LFSFRAG,
		.dlfs_frag =		DFL_LFSBLOCK/DFL_LFSFRAG,
		.dlfs_freehd =		HIGHEST_USED_INO + 1,
		.dlfs_bfree =		0,
		.dlfs_nfiles =		0,
		.dlfs_avail =		0,
		.dlfs_uinodes =		0,
		.dlfs_idaddr =		0,
		.dlfs_ifile =		LFS_IFILE_INUM,
		.dlfs_lastseg =		0,
		.dlfs_nextseg =		0,
		.dlfs_curseg =		0,
		.dlfs_offset =		0,
		.dlfs_lastpseg =	0,
		.dlfs_inopf =		0,
		.dlfs_minfree =		MINFREE,
		.dlfs_maxfilesize =	0,
		.dlfs_fsbpseg =		0,
		.dlfs_inopb =		DFL_LFSBLOCK/sizeof(struct lfs32_dinode),
		.dlfs_ifpb =		DFL_LFSBLOCK/sizeof(IFILE32),
		.dlfs_sepb =		DFL_LFSBLOCK/sizeof(SEGUSE),
		.dlfs_nindir =		DFL_LFSBLOCK/sizeof(int32_t),
		.dlfs_nseg =		0,
		.dlfs_nspf =		0,
		.dlfs_cleansz =		0,
		.dlfs_segtabsz =	0,
		.dlfs_segmask =		DFL_LFSSEG_MASK,
		.dlfs_segshift =	DFL_LFSSEG_SHIFT,
		.dlfs_bshift =		DFL_LFSBLOCK_SHIFT,
		.dlfs_ffshift =		DFL_LFS_FFSHIFT,
		.dlfs_fbshift =		DFL_LFS_FBSHIFT,
		.dlfs_bmask =		DFL_LFSBLOCK_MASK,
		.dlfs_ffmask =		DFL_LFS_FFMASK,
		.dlfs_fbmask =		DFL_LFS_FBMASK,
		.dlfs_blktodb =		0,
		.dlfs_sushift =		0,
		.dlfs_maxsymlinklen =	LFS32_MAXSYMLINKLEN,
		.dlfs_sboffs =		{ 0 },
		.dlfs_nclean =  	0,
		.dlfs_fsmnt =   	{ 0 },
		.dlfs_pflags =  	LFS_PF_CLEAN,
		.dlfs_dmeta =		0,
		.dlfs_minfreeseg =	0,
		.dlfs_sumsize =		0,
		.dlfs_serial =		0,
		.dlfs_ibsize =		DFL_LFSFRAG,
		.dlfs_s0addr =		0,
		.dlfs_tstamp =  	0,
		.dlfs_inodefmt =	LFS_44INODEFMT,
		.dlfs_interleave =	0,
		.dlfs_ident =		0,
		.dlfs_fsbtodb =		0,
		.dlfs_resvseg =		0,

		.dlfs_pad = 		{ 0 },
		.dlfs_cksum =		0
};

static const struct dlfs64 dlfs64_default = {
		.dlfs_magic =		LFS64_MAGIC,
		.dlfs_version =		LFS_VERSION,
		.dlfs_size =		0,
		.dlfs_dsize =		0,
		.dlfs_ssize =		DFL_LFSSEG,
		.dlfs_bsize =		DFL_LFSBLOCK,
		.dlfs_fsize =		DFL_LFSFRAG,
		.dlfs_frag =		DFL_LFSBLOCK/DFL_LFSFRAG,
		.dlfs_freehd =		HIGHEST_USED_INO + 1,
		.dlfs_nfiles =		0,
		.dlfs_bfree =		0,
		.dlfs_avail =		0,
		.dlfs_idaddr =		0,
		.dlfs_uinodes =		0,
		.dlfs_unused_0 =	0,
		.dlfs_lastseg =		0,
		.dlfs_nextseg =		0,
		.dlfs_curseg =		0,
		.dlfs_offset =		0,
		.dlfs_lastpseg =	0,
		.dlfs_inopf =		0,
		.dlfs_minfree =		MINFREE,
		.dlfs_maxfilesize =	0,
		.dlfs_fsbpseg =		0,
		.dlfs_inopb =		DFL_LFSBLOCK/sizeof(struct lfs64_dinode),
		.dlfs_ifpb =		DFL_LFSBLOCK/sizeof(IFILE64),
		.dlfs_sepb =		DFL_LFSBLOCK/sizeof(SEGUSE),
		.dlfs_nindir =		DFL_LFSBLOCK/sizeof(int64_t),
		.dlfs_nseg =		0,
		.dlfs_nspf =		0,
		.dlfs_cleansz =		0,
		.dlfs_segtabsz =	0,
		.dlfs_bshift =		DFL_LFSBLOCK_SHIFT,
		.dlfs_ffshift =		DFL_LFS_FFSHIFT,
		.dlfs_fbshift =		DFL_LFS_FBSHIFT,
		.dlfs_bmask =		DFL_LFSBLOCK_MASK,
		.dlfs_ffmask =		DFL_LFS_FFMASK,
		.dlfs_fbmask =		DFL_LFS_FBMASK,
		.dlfs_blktodb =		0,
		.dlfs_sushift =		0,
		.dlfs_sboffs =		{ 0 },
		.dlfs_maxsymlinklen =	LFS64_MAXSYMLINKLEN,
		.dlfs_nclean =  	0,
		.dlfs_fsmnt =   	{ 0 },
		.dlfs_pflags =  	LFS_PF_CLEAN,
		.dlfs_dmeta =		0,
		.dlfs_minfreeseg =	0,
		.dlfs_sumsize =		0,
		.dlfs_ibsize =		DFL_LFSFRAG,
		.dlfs_inodefmt =	LFS_44INODEFMT,
		.dlfs_serial =		0,
		.dlfs_s0addr =		0,
		.dlfs_tstamp =  	0,
		.dlfs_interleave =	0,
		.dlfs_ident =		0,
		.dlfs_fsbtodb =		0,
		.dlfs_resvseg =		0,

		.dlfs_pad = 		{ 0 },
		.dlfs_cksum =		0
};

static const struct lfs lfs_default;

#define	UMASK	0755

struct dirproto {
	ino_t dp_ino;
	const char *dp_name;
	unsigned dp_type;
};

static const struct dirproto lfs_root_dir[] = {
	{ ULFS_ROOTINO, ".", LFS_DT_DIR },
	{ ULFS_ROOTINO, "..", LFS_DT_DIR },
	/*{ LFS_IFILE_INUM, "ifile", LFS_DT_REG },*/
#ifdef MAKE_LF_DIR
	{ LOSTFOUNDINO, "lost+found", LFS_DT_DIR },
#endif
};

#ifdef MAKE_LF_DIR
static const struct dirproto lfs_lf_dir[] = {
        { LOSTFOUNDINO, ".", LFS_DT_DIR },
	{ ULFS_ROOTINO, "..", LFS_DT_DIR },
};
#endif

void pwarn(const char *, ...);
static void make_dinode(ino_t, union lfs_dinode *, int, struct lfs *);
static void make_dir(struct lfs *, void *,
		const struct dirproto *, unsigned);

/*
 * calculate the maximum file size allowed with the specified block shift.
 */
static uint64_t
maxfilesize(struct lfs *fs, int bshift)
{
	uint64_t nptr; /* number of block pointers per block */
	uint64_t maxblock;

	nptr = (1 << bshift) / LFS_BLKPTRSIZE(fs);
	maxblock = ULFS_NDADDR + nptr + nptr * nptr + nptr * nptr * nptr;

	return maxblock << bshift;
}

/*
 * Create the root directory for this file system and the lost+found
 * directory.
 */
static void
make_dinode(ino_t ino, union lfs_dinode *dip, int nfrags, struct lfs *fs)
{
	int i;
	int nblocks, bb, base, factor, lvl;
	time_t t;

	nblocks = howmany(nfrags, lfs_sb_getfrag(fs));
	if (nblocks >= ULFS_NDADDR)
		nfrags = roundup(nfrags, lfs_sb_getfrag(fs));

	lfs_dino_setnlink(fs, dip, 1);
	lfs_dino_setblocks(fs, dip, nfrags);

	lfs_dino_setsize(fs, dip, nfrags << lfs_sb_getffshift(fs));
	t = lfs_sb_gettstamp(fs);
	lfs_dino_setatime(fs, dip, t);
	lfs_dino_setmtime(fs, dip, t);
	lfs_dino_setctime(fs, dip, t);
	lfs_dino_setatimensec(fs, dip, 0);
	lfs_dino_setmtimensec(fs, dip, 0);
	lfs_dino_setctimensec(fs, dip, 0);
	lfs_dino_setinumber(fs, dip, ino);
	lfs_dino_setgen(fs, dip, 1);

	if (ULFS_NDADDR < nblocks) {
		/* Count up how many indirect blocks we need, recursively */
		/* XXX We are only called with nblocks > 1 for Ifile */
		bb = nblocks - ULFS_NDADDR;
		while (bb > 0) {
			bb = howmany(bb, LFS_NINDIR(fs));
			ifibc += bb;
			--bb;
		}
		lfs_dino_setblocks(fs, dip,
		    lfs_dino_getblocks(fs, dip) + lfs_blkstofrags(fs, ifibc));
	}

	/* Assign the block addresses for the ifile */
	for (i = 0; i < MIN(nblocks,ULFS_NDADDR); i++) {
		lfs_dino_setdb(fs, dip, i, 0x0);
	}
	if (nblocks > ULFS_NDADDR) {
		lfs_dino_setib(fs, dip, 0, 0x0);
		bb = howmany(nblocks - ULFS_NDADDR, LFS_NINDIR(fs)) - 1;
		factor = LFS_NINDIR(fs);
		base = -ULFS_NDADDR - factor;
		lvl = 1;
		while (bb > 0) {
			lfs_dino_setib(fs, dip, lvl, 0x0);
			bb = howmany(bb, LFS_NINDIR(fs));
			--bb;
			factor *= LFS_NINDIR(fs);
			base -= factor;
			++lvl;
		}
	}
}

/*
 * Construct a set of directory entries in "bufp".  We assume that all the
 * entries in protodir fit in the first DIRBLKSIZ.  
 */
static void
make_dir(struct lfs *fs, void *bufp,
    const struct dirproto *protodir, unsigned numentries)
{
	LFS_DIRHEADER *ep;
	unsigned spaceleft;
	unsigned namlen, reclen;
	unsigned i;

	spaceleft = LFS_DIRBLKSIZ;
	ep = bufp;
	for (i = 0; i < numentries; i++) {
		namlen = strlen(protodir[i].dp_name);
		reclen = LFS_DIRECTSIZ(fs, namlen);
		if (spaceleft < reclen)
			fatal("%s: %s", special, "directory too big");

		/* Last entry includes all the free space. */
		if (i + 1 == numentries) {
			reclen = spaceleft;
		}
		spaceleft -= reclen;

		lfs_dir_setino(fs, ep, protodir[i].dp_ino);
		lfs_dir_setreclen(fs, ep, reclen);
		lfs_dir_settype(fs, ep, protodir[i].dp_type);
		lfs_dir_setnamlen(fs, ep, namlen);
		lfs_copydirname(fs, lfs_dir_nameptr(fs, ep), protodir[i].dp_name,
				namlen, reclen);
		ep = LFS_NEXTDIR(fs, ep);
	}
	assert(spaceleft == 0);
}

int
make_lfs(int devfd, uint secsize, struct dkwedge_info *dkw, int minfree,
	 int block_size, int frag_size, int seg_size, int minfreeseg,
	 int resvseg, int version, daddr_t start, int ibsize, int interleave,
	 u_int32_t roll_id)
{
	union lfs_dinode *dip;	/* Pointer to a disk inode */
	CLEANERINFO *cip;	/* Segment cleaner information table */
	IFILE *ipall;		/* Pointer to array of ifile structures */
	IFILE64 *ip64 = NULL;
	IFILE32 *ip32 = NULL;
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
	daddr_t bb, ubb;
	int dmeta, labelskew;
	u_int64_t tsepb, tnseg;
	time_t stamp;
	bool is64 = false; /* XXX notyet */
	bool dobyteswap = false; /* XXX notyet */

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
	fs = lfs_init(devfd, start, 0, 1, 1/* XXX debug*/);
	save_devvp = fs->lfs_devvp;
	vp = fs->lfs_ivnode;
	/* XXX this seems like a rubbish */
	*fs = lfs_default;
	if (is64) {
		fs->lfs_dlfs_u.u_64 = dlfs64_default;
	} else {
		fs->lfs_dlfs_u.u_32 = dlfs32_default;
	}
	fs->lfs_is64 = is64;
	fs->lfs_dobyteswap = dobyteswap;
	fs->lfs_hasolddirfmt = false;
	fs->lfs_ivnode = vp;
	fs->lfs_devvp = save_devvp;


	/* Set version first of all since it is used to compute other fields */
	lfs_sb_setversion(fs, version);

	/* If partition is not an LFS partition, warn that that is the case */
	if (strcmp(dkw->dkw_ptype, DKW_PTYPE_LFS) != 0) {
		fatal("partition label indicated fs type \"%s\", "
		    "expected \"%s\"", dkw->dkw_ptype, DKW_PTYPE_LFS);
	}

	if (!(bsize = block_size)) {
		bsize = DFL_LFSBLOCK;
		if (dkw->dkw_size <= SMALL_FSSIZE)
			bsize = SMALL_LFSBLOCK;
	}
	if (!(fsize = frag_size)) {
		fsize = DFL_LFSFRAG;
		if (dkw->dkw_size <= SMALL_FSSIZE)
			fsize = SMALL_LFSFRAG;
	}
	if (!(ssize = seg_size)) {
		ssize = DFL_LFSSEG;
		if (dkw->dkw_size <= SMALL_FSSIZE)
			ssize = SMALL_LFSSEG;
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
		lfs_sb_setbshift(fs, lfs_log2(bsize));
		if (1 << lfs_sb_getbshift(fs) != bsize)
			fatal("%d: block size not a power of 2", bsize);
		lfs_sb_setbsize(fs, bsize);
		lfs_sb_setfsize(fs, fsize);
		lfs_sb_setbmask(fs, bsize - 1);
		lfs_sb_setffmask(fs, fsize - 1);
		lfs_sb_setffshift(fs, lfs_log2(fsize));
		if (1 << lfs_sb_getffshift(fs) != fsize)
			fatal("%d: frag size not a power of 2", fsize);
		lfs_sb_setfrag(fs, lfs_numfrags(fs, bsize));
		lfs_sb_setfbmask(fs, lfs_sb_getfrag(fs) - 1);
		lfs_sb_setfbshift(fs, lfs_log2(lfs_sb_getfrag(fs)));
		lfs_sb_setifpb(fs, bsize / IFILE_ENTRYSIZE(fs));
		lfs_sb_setnindir(fs, bsize / LFS_BLKPTRSIZE(fs));
	}

	if (lfs_sb_getversion(fs) == 1) {
		lfs_sb_setsumsize(fs, LFS_V1_SUMMARY_SIZE);
		if (!is64) {
			unsigned segshift;

			segshift = lfs_log2(ssize);
			if (1 << segshift != ssize)
				fatal("%d: segment size not power of 2",
				      ssize);
			fs->lfs_dlfs_u.u_32.dlfs_segshift = segshift;
			fs->lfs_dlfs_u.u_32.dlfs_segmask = ssize - 1;
		}
		lfs_sb_setifpb(fs, lfs_sb_getbsize(fs) / sizeof(IFILE_V1));
		lfs_sb_setibsize(fs, lfs_sb_getbsize(fs));
		lfs_sb_setsepb(fs, bsize / sizeof(SEGUSE_V1));
		lfs_sb_setssize(fs, ssize >> lfs_sb_getbshift(fs));
	} else {
		if (ssize % fsize) {
			fprintf(stderr, 
				"Segment size %d is not a multiple of frag size; ",
				 ssize);
			ssize = roundup(ssize, fsize);
			fprintf(stderr, "trying size %d.\n", ssize);
			goto tryagain;
		}
		lfs_sb_setsumsize(fs, fsize);
		if (!is64) {
			/* these do not exist in the 64-bit superblock */
			fs->lfs_dlfs_u.u_32.dlfs_segshift = 0;
			fs->lfs_dlfs_u.u_32.dlfs_segmask = 0;
		}
		lfs_sb_setsepb(fs, bsize / sizeof(SEGUSE));
		lfs_sb_setssize(fs, ssize);
		lfs_sb_setibsize(fs, ibsize);
	}
	lfs_sb_setinopb(fs, lfs_sb_getibsize(fs) / DINOSIZE(fs));
	lfs_sb_setminfree(fs, minfree);

	if (version > 1) {
		lfs_sb_setinopf(fs, secsize/DINOSIZE(fs));
		lfs_sb_setinterleave(fs, interleave);
		if (roll_id == 0)
			roll_id = arc4random();
		lfs_sb_setident(fs, roll_id);
	}

	/*
	 * Fill in parts of superblock that can be computed from file system
	 * size, disk geometry and current time.
	 *
	 * XXX: this seems to set dlfs_size wrong for version 1... as in,
	 * sets it and then overwrites it a few lines later.
	 */
	db_per_blk = bsize/secsize;
	lfs_sb_setblktodb(fs, lfs_log2(db_per_blk));
	lfs_sb_setfsbtodb(fs, lfs_log2(fsize / secsize));
	if (version == 1) {
		lfs_sb_setsushift(fs, lfs_log2(lfs_sb_getsepb(fs)));
		lfs_sb_setfsbtodb(fs, 0);
		lfs_sb_setsize(fs, dkw->dkw_size >> lfs_sb_getblktodb(fs));
	}
	label_fsb = lfs_btofsb(fs, roundup(LFS_LABELPAD, fsize));
	sb_fsb = lfs_btofsb(fs, roundup(LFS_SBPAD, fsize));
	lfs_sb_setfsbpseg(fs, LFS_DBTOFSB(fs, ssize / secsize));
	lfs_sb_setsize(fs, dkw->dkw_size >> lfs_sb_getfsbtodb(fs));
	lfs_sb_setdsize(fs, LFS_DBTOFSB(fs, dkw->dkw_size) -
		MAX(label_fsb, LFS_DBTOFSB(fs, start)));
	lfs_sb_setnseg(fs, lfs_sb_getdsize(fs) / lfs_segtod(fs, 1));

	lfs_sb_setnclean(fs, lfs_sb_getnseg(fs) - 1);
	lfs_sb_setmaxfilesize(fs, maxfilesize(fs, lfs_sb_getbshift(fs)));

	if (minfreeseg == 0)
		lfs_sb_setminfreeseg(fs, lfs_sb_getnseg(fs) / DFL_MIN_FREE_SEGS);
	else
		lfs_sb_setminfreeseg(fs, minfreeseg);
	if (lfs_sb_getminfreeseg(fs) < MIN_FREE_SEGS)
		lfs_sb_setminfreeseg(fs, MIN_FREE_SEGS);

	if (resvseg == 0)
		lfs_sb_setresvseg(fs, lfs_sb_getminfreeseg(fs) / 2 + 1);
	else
		lfs_sb_setresvseg(fs, resvseg);
	if (lfs_sb_getresvseg(fs) < MIN_RESV_SEGS)
		lfs_sb_setresvseg(fs, MIN_RESV_SEGS);

	if(lfs_sb_getnseg(fs) < (4 * lfs_sb_getminfreeseg(fs))
	   || lfs_sb_getnseg(fs) < LFS_MIN_SBINTERVAL + 1)
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
			"segment size.\n", ssize, lfs_sb_getbsize(fs));
	}
	if(warned_segtoobig)
		fprintf(stderr,"Using segment size %d, block size %d, frag size %d.\n", ssize, bsize, fsize);

	/*
	 * Now that we've determined what we're going to do, announce it
	 * to the user.
	 */
        printf("Creating a version %d LFS", lfs_sb_getversion(fs));
        if (lfs_sb_getversion(fs) > 1)
                printf(" with roll-forward ident 0x%x", lfs_sb_getident(fs));
        printf("\n");   
        fssize = (double)lfs_sb_getnseg(fs);
        fssize *= (double)ssize;
        fssize /= 1048576.0;
        printf("%.1fMB in %d segments of size %d\n", fssize,
               lfs_sb_getnseg(fs), ssize);

	/* 
	 * The number of free blocks is set from the number of segments
	 * times the segment size - lfs_minfreesegs (that we never write
	 * because we need to make sure the cleaner can run).  Then
	 * we'll subtract off the room for the superblocks ifile entries
	 * and segment usage table, and half a block per segment that can't
	 * be written due to fragmentation.
	 */
	lfs_sb_setdsize(fs,
		lfs_segtod(fs, lfs_sb_getnseg(fs) - lfs_sb_getminfreeseg(fs)));
	lfs_sb_setbfree(fs, lfs_sb_getdsize(fs));
	lfs_sb_subbfree(fs, LFS_DBTOFSB(fs, ((lfs_sb_getnseg(fs) / 2) << 
		lfs_sb_getblktodb(fs))));

	lfs_sb_setsegtabsz(fs, SEGTABSIZE_SU(fs));
	lfs_sb_setcleansz(fs, CLEANSIZE_SU(fs));
	if (time(&stamp) == -1)
		fatal("time: %s", strerror(errno));
	lfs_sb_settstamp(fs, stamp);
	if (version == 1)
		lfs_sb_setotstamp(fs, stamp);

	if ((sb_interval = lfs_sb_getnseg(fs) / LFS_MAXNUMSB) < LFS_MIN_SBINTERVAL)
		sb_interval = LFS_MIN_SBINTERVAL;

	/*
	 * Figure out where the superblocks are going to live.
	 *
	 * Make segment 0 start at either zero, or LFS_LABELPAD, or
	 * >= LFS_SBPAD+LFS_LABELPAD, in order to prevent segment 0
	 * from having half a superblock in it.
	 */
	if (LFS_FSBTODB(fs, LFS_DBTOFSB(fs, start)) != start)
		fatal("Segment 0 offset is not multiple of frag size\n");
	if (start != 0 && dbtob(start) != LFS_LABELPAD &&
	    dbtob(start) < LFS_SBPAD + LFS_LABELPAD) {
		fatal("Using flags \"-O %" PRId64 "\" would result in the "
		      "first segment containing only\npart of a superblock.  "
		      "Please choose an offset of 0, %d, or %d or more,\n",
		      start, btodb(LFS_LABELPAD),
		      btodb(LFS_LABELPAD + LFS_SBPAD));
	}
	lfs_sb_setsboff(fs, 0, label_fsb);
	if (version == 1)
		lfs_sb_sets0addr(fs, lfs_sb_getsboff(fs, 0));
	else
		lfs_sb_sets0addr(fs, LFS_DBTOFSB(fs, start));
        lfs_sb_subdsize(fs, sb_fsb);
	for (i = 1; i < LFS_MAXNUMSB; i++) {
		sb_addr = ((i * sb_interval) * lfs_segtod(fs, 1))
		    + lfs_sb_getsboff(fs, 0);
		/* Segment 0 eats the label, except for version 1 */
		if (lfs_sb_getversion(fs) > 1 && lfs_sb_gets0addr(fs) < label_fsb)
			sb_addr -= label_fsb - start;
		if (sb_addr + sizeof(struct dlfs)
		    >= LFS_DBTOFSB(fs, dkw->dkw_size))
			break;
		lfs_sb_setsboff(fs, i, sb_addr);
		lfs_sb_subdsize(fs, sb_fsb);
	}

	/* We need >= 2 superblocks */
	if (lfs_sb_getsboff(fs, 1) == 0x0) {
		fatal("Could not assign a disk address for the second "
		      "superblock.\nPlease decrease the segment size.\n");
	}

	lfs_sb_setlastseg(fs, lfs_sntod(fs, lfs_sb_getnseg(fs) - 2));
	lfs_sb_setcurseg(fs, lfs_sntod(fs, lfs_sb_getnseg(fs) - 1));
	lfs_sb_setoffset(fs, lfs_sntod(fs, lfs_sb_getnseg(fs)));
	lfs_sb_setnextseg(fs, lfs_sntod(fs, 0));

	/*
	 * Initialize the Ifile inode.  Do this before we do anything
	 * with the Ifile or segment tables.
	 *
	 * XXX: is there some reason this allocates a new dinode? we
	 * already have an empty one generated by vget.
	 */
	dip = malloc(sizeof(*dip));
	if (dip == NULL)
		err(1, NULL);
	memset(dip, 0, sizeof(*dip));

	VTOI(fs->lfs_ivnode)->i_din = dip;
		
	lfs_dino_setmode(fs, dip, LFS_IFREG | 0600);
	lfs_dino_setflags(fs, dip, SF_IMMUTABLE);
	make_dinode(LFS_IFILE_INUM, dip,
		lfs_blkstofrags(fs, lfs_sb_getcleansz(fs) + lfs_sb_getsegtabsz(fs) + 1), fs);
	lfs_dino_setsize(fs, dip, (lfs_sb_getcleansz(fs) + lfs_sb_getsegtabsz(fs) + 1) << lfs_sb_getbshift(fs));
	for (i = 0; i < ULFS_NDADDR && i < (lfs_dino_getsize(fs, dip) >> lfs_sb_getbshift(fs)); i++)
		VTOI(fs->lfs_ivnode)->i_lfs_fragsize[i] = lfs_sb_getbsize(fs);

	/*
	 * Set up in-superblock segment usage cache
	 */
 	fs->lfs_suflags = (u_int32_t **) malloc(2 * sizeof(u_int32_t *));       
	if (fs->lfs_suflags == NULL)
		err(1, NULL);
	fs->lfs_suflags[0] = (u_int32_t *) malloc(lfs_sb_getnseg(fs) * sizeof(u_int32_t));
	if (fs->lfs_suflags[0] == NULL)
		err(1, NULL);
	fs->lfs_suflags[1] = (u_int32_t *) malloc(lfs_sb_getnseg(fs) * sizeof(u_int32_t));
	if (fs->lfs_suflags[1] == NULL)
		err(1, NULL);

	/*
	 * Initialize the cleanerinfo block
	 */
	LFS_CLEANERINFO(cip, fs, bp);
	lfs_ci_setclean(fs, cip, lfs_sb_getnseg(fs));
	lfs_ci_setdirty(fs, cip, 0);
	if (version > 1) {
		lfs_ci_setfree_head(fs, cip, HIGHEST_USED_INO + 1);
		lfs_ci_setfree_tail(fs, cip, lfs_sb_getifpb(fs) - 1);
	}
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);

	/*
	 * Run through segment table and initialize that
	 */
	for (i = j = 0; i < lfs_sb_getnseg(fs); i++) {
		LFS_SEGENTRY(segp, fs, i, bp);

		if (i == 0 &&
		    lfs_sb_gets0addr(fs) < lfs_btofsb(fs, LFS_LABELPAD + LFS_SBPAD)) {
			segp->su_flags = SEGUSE_SUPERBLOCK;
			lfs_sb_subbfree(fs, sb_fsb);
			++j;
		}
		if (i > 0) { 
			if ((i % sb_interval) == 0 && j < LFS_MAXNUMSB) {
				segp->su_flags = SEGUSE_SUPERBLOCK;
				lfs_sb_subbfree(fs, sb_fsb);
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
	vp = lfs_raw_vget(fs, ULFS_ROOTINO, devfd, 0x0);
	dip = VTOI(vp)->i_din;
	make_dinode(ULFS_ROOTINO, dip, howmany(LFS_DIRBLKSIZ, lfs_sb_getfsize(fs)), fs);
	lfs_dino_setmode(fs, dip, LFS_IFDIR | UMASK);
	VTOI(vp)->i_lfs_osize = LFS_DIRBLKSIZ;
#ifdef MAKE_LF_DIR
	VTOI(vp)->i_nlink = 3;
#else
	VTOI(vp)->i_nlink = 2;
#endif
        VTOI(vp)->i_lfs_effnblks =
		lfs_btofsb(fs, roundup(LFS_DIRBLKSIZ, lfs_sb_getfsize(fs)));
	lfs_dino_setsize(fs, dip, VTOI(vp)->i_lfs_osize);
	lfs_dino_setnlink(fs, dip, VTOI(vp)->i_nlink);
	lfs_dino_setblocks(fs, dip, VTOI(vp)->i_lfs_effnblks);
	for (i = 0; i < ULFS_NDADDR && i < howmany(LFS_DIRBLKSIZ, lfs_sb_getbsize(fs)); i++)
		VTOI(vp)->i_lfs_fragsize[i] = lfs_sb_getbsize(fs);
	if (LFS_DIRBLKSIZ < lfs_sb_getbsize(fs))
		VTOI(vp)->i_lfs_fragsize[i - 1] =
			roundup(LFS_DIRBLKSIZ, lfs_sb_getfsize(fs));
	bread(vp, 0, lfs_sb_getfsize(fs), 0, &bp);
	make_dir(fs, bp->b_data, lfs_root_dir, __arraycount(lfs_root_dir));
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
		lfs_btofsb(fs, roundup(DIRBLKSIZ,fs->lfs_fsize));
	for (i = 0; i < ULFS_NDADDR && i < howmany(DIRBLKSIZ, lfs_sb_getbsize(fs)); i++)
		VTOI(vp)->i_lfs_fragsize[i] = lfs_sb_getbsize(fs);
	if (DIRBLKSIZ < lfs_sb_getbsize(fs))
		VTOI(vp)->i_lfs_fragsize[i - 1] =
			roundup(DIRBLKSIZ,fs->lfs_fsize);
	bread(vp, 0, fs->lfs_fsize, 0, &bp);
	make_dir(fs, bp->b_data, lfs_lf_dir, __arraycount(lfs_lf_dir));
	VOP_BWRITE(bp);
#endif /* MAKE_LF_DIR */

	/* Set their IFILE entry version numbers to 1 */
	LFS_IENTRY(ipall, fs, 1, bp);
	if (is64) {
		ip64 = &ipall->u_64;
		for (i = LFS_IFILE_INUM; i <= HIGHEST_USED_INO; i++) {
			ip64->if_version = 1;
			ip64->if_daddr = 0x0;
			ip64->if_nextfree = 0;
			++ip64;
		}
	} else if (version > 1) {
		ip32 = &ipall->u_32;
		for (i = LFS_IFILE_INUM; i <= HIGHEST_USED_INO; i++) {
			ip32->if_version = 1;
			ip32->if_daddr = 0x0;
			ip32->if_nextfree = 0;
			++ip32;
		}
	} else {
		ip_v1 = &ipall->u_v1;
		for (i = LFS_IFILE_INUM; i <= HIGHEST_USED_INO; i++) {
			ip_v1->if_version = 1;
			ip_v1->if_daddr = 0x0;
			ip_v1->if_nextfree = 0;
			++ip_v1;
		}
	}
	/* Link remaining IFILE entries in free list */
	if (is64) {
		for (;
		     i < lfs_sb_getifpb(fs); ++ip64) {
			ip64->if_version = 1;
			ip64->if_daddr = LFS_UNUSED_DADDR;
			ip64->if_nextfree = ++i;
		}
		--ip64;
		ip64->if_nextfree = LFS_UNUSED_INUM;
	} else if (version > 1) {
		for (;
		     i < lfs_sb_getifpb(fs); ++ip32) {
			ip32->if_version = 1;
			ip32->if_daddr = LFS_UNUSED_DADDR;
			ip32->if_nextfree = ++i;
		}
		--ip32;
		ip32->if_nextfree = LFS_UNUSED_INUM;
	} else {
		for (;
		     i < lfs_sb_getifpb(fs); ++ip_v1) {
			ip_v1->if_version = 1;
			ip_v1->if_daddr = LFS_UNUSED_DADDR;
			ip_v1->if_nextfree = ++i;
		}
		--ip_v1;
		ip_v1->if_nextfree = LFS_UNUSED_INUM;
	}
	VOP_BWRITE(bp);

	/* Write it all to disk. */
	if (!Nflag)
		lfs_segwrite(fs, SEGM_CKP);

	/*
	 * Now that we've written everything, look to see what's available
	 * for writing.
	 */
	lfs_sb_setavail(fs, 0);
	bb = ubb = dmeta = 0;
	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		LFS_SEGENTRY(segp, fs, i, bp);
                if (segp->su_flags & SEGUSE_DIRTY) {
                        bb += lfs_btofsb(fs, segp->su_nbytes +
                            segp->su_nsums * lfs_sb_getsumsize(fs));
                        ubb += lfs_btofsb(fs, segp->su_nbytes +
                            segp->su_nsums * lfs_sb_getsumsize(fs) +
                            segp->su_ninos * lfs_sb_getibsize(fs));
                        dmeta += lfs_btofsb(fs,
                            lfs_sb_getsumsize(fs) * segp->su_nsums);
                        dmeta += lfs_btofsb(fs,
                            lfs_sb_getibsize(fs) * segp->su_ninos);
		} else {
                        lfs_sb_addavail(fs, lfs_segtod(fs, 1));
                        if (segp->su_flags & SEGUSE_SUPERBLOCK)
                                lfs_sb_subavail(fs, lfs_btofsb(fs, LFS_SBPAD));
                        if (i == 0 && lfs_sb_getversion(fs) > 1 &&
                            lfs_sb_gets0addr(fs) < lfs_btofsb(fs, LFS_LABELPAD))
                                lfs_sb_subavail(fs, lfs_btofsb(fs, LFS_LABELPAD) -
                                    lfs_sb_gets0addr(fs));
                }
		brelse(bp, 0);
        }
        /* Also may be available bytes in current seg */
        i = lfs_dtosn(fs, lfs_sb_getoffset(fs));
        lfs_sb_addavail(fs, lfs_sntod(fs, i + 1) - lfs_sb_getoffset(fs));
        /* But do not count minfreesegs */
        lfs_sb_subavail(fs, lfs_segtod(fs, (lfs_sb_getminfreeseg(fs) - (lfs_sb_getminfreeseg(fs) / 2))));

        labelskew = 0;
        if (lfs_sb_getversion(fs) > 1 && lfs_sb_gets0addr(fs) < lfs_btofsb(fs, LFS_LABELPAD))
                labelskew = lfs_btofsb(fs, LFS_LABELPAD);
        lfs_sb_setbfree(fs, lfs_sb_getdsize(fs) - labelskew - (ubb + bb) / 2);

	/* Put that in the Ifile version too, and write it */
	LFS_CLEANERINFO(cip, fs, bp);
	lfs_ci_setbfree(fs, cip, lfs_sb_getbfree(fs));
	lfs_ci_setavail(fs, cip, lfs_sb_getavail(fs));
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);
	if (!Nflag)
		lfs_segwrite(fs, SEGM_CKP);

	/*
	 * Finally write out superblocks.
	 */
	printf("super-block backups (for fsck -b #) at:\n");
	curw = 0;
	for (i = 0; i < LFS_MAXNUMSB; i++) {
		seg_addr = lfs_sb_getsboff(fs, i);
		if (seg_addr == 0)
			break;

		if (i != 0)
			curw += printf(", ");
		ww = snprintf(tbuf, sizeof(tbuf), "%lld",
		    (long long)LFS_FSBTODB(fs, seg_addr));
		curw += ww;
		if (curw >= 78) {
			printf("\n%s", tbuf);
			curw = ww;
		} else
			printf("%s", tbuf);
		fflush(stdout);

		/* Leave the time stamp on the alt sb, zero the rest */
		if (i == 2) {
			lfs_sb_settstamp(fs, 0);
			lfs_sb_setcksum(fs, lfs_sb_cksum(fs));
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
