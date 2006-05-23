/* $NetBSD: segwrite.c,v 1.12 2006/05/23 22:35:20 jnemeth Exp $ */
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
 *
 *	@(#)lfs_segment.c	8.10 (Berkeley) 6/10/95
 */

/*
 * Partial segment writer, taken from the kernel and adapted for userland.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>

/* Override certain things to make <ufs/lfs/lfs.h> work */
#define vnode uvnode
#define buf ubuf
#define panic call_panic

#include <ufs/lfs/lfs.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "segwrite.h"

/* Compatibility definitions */
extern off_t locked_queue_bytes;
int locked_queue_count;
off_t written_bytes = 0;
off_t written_data = 0;
off_t written_indir = 0;
off_t written_dev = 0;
int written_inodes = 0;

/* Global variables */
time_t write_time;

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);
extern int preen;

/*
 * Logical block number match routines used when traversing the dirty block
 * chain.
 */
int
lfs_match_data(struct lfs * fs, struct ubuf * bp)
{
	return (bp->b_lblkno >= 0);
}

int
lfs_match_indir(struct lfs * fs, struct ubuf * bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 0);
}

int
lfs_match_dindir(struct lfs * fs, struct ubuf * bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 1);
}

int
lfs_match_tindir(struct lfs * fs, struct ubuf * bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 2);
}

/*
 * Do a checkpoint.
 */
int
lfs_segwrite(struct lfs * fs, int flags)
{
	struct inode *ip;
	struct segment *sp;
	struct uvnode *vp;
	int redo;

	lfs_seglock(fs, flags | SEGM_CKP);
	sp = fs->lfs_sp;

	lfs_writevnodes(fs, sp, VN_REG);
	lfs_writevnodes(fs, sp, VN_DIROP);
	((SEGSUM *) (sp->segsum))->ss_flags &= ~(SS_CONT);

	do {
		vp = fs->lfs_ivnode;
		fs->lfs_flags &= ~LFS_IFDIRTY;
		ip = VTOI(vp);
		if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL || fs->lfs_idaddr <= 0)
			lfs_writefile(fs, sp, vp);

		redo = lfs_writeinode(fs, sp, ip);
		redo += lfs_writeseg(fs, sp);
		redo += (fs->lfs_flags & LFS_IFDIRTY);
	} while (redo);

	lfs_segunlock(fs);
#if 0
	printf("wrote %" PRId64 " bytes (%" PRId32 " fsb)\n",
		written_bytes, (ufs_daddr_t)btofsb(fs, written_bytes));
	printf("wrote %" PRId64 " bytes data (%" PRId32 " fsb)\n",
		written_data, (ufs_daddr_t)btofsb(fs, written_data));
	printf("wrote %" PRId64 " bytes indir (%" PRId32 " fsb)\n",
		written_indir, (ufs_daddr_t)btofsb(fs, written_indir));
	printf("wrote %" PRId64 " bytes dev (%" PRId32 " fsb)\n",
		written_dev, (ufs_daddr_t)btofsb(fs, written_dev));
	printf("wrote %d inodes (%" PRId32 " fsb)\n",
		written_inodes, btofsb(fs, written_inodes * fs->lfs_ibsize));
#endif
	return 0;
}

/*
 * Write the dirty blocks associated with a vnode.
 */
void
lfs_writefile(struct lfs * fs, struct segment * sp, struct uvnode * vp)
{
	struct ubuf *bp;
	struct finfo *fip;
	struct inode *ip;
	IFILE *ifp;

	ip = VTOI(vp);

	if (sp->seg_bytes_left < fs->lfs_bsize ||
	    sp->sum_bytes_left < sizeof(struct finfo))
		(void) lfs_writeseg(fs, sp);

	sp->sum_bytes_left -= FINFOSIZE;
	++((SEGSUM *) (sp->segsum))->ss_nfinfo;

	if (vp->v_flag & VDIROP)
		((SEGSUM *) (sp->segsum))->ss_flags |= (SS_DIROP | SS_CONT);

	fip = sp->fip;
	fip->fi_nblocks = 0;
	fip->fi_ino = ip->i_number;
	LFS_IENTRY(ifp, fs, fip->fi_ino, bp);
	fip->fi_version = ifp->if_version;
	brelse(bp);

	lfs_gather(fs, sp, vp, lfs_match_data);
	lfs_gather(fs, sp, vp, lfs_match_indir);
	lfs_gather(fs, sp, vp, lfs_match_dindir);
	lfs_gather(fs, sp, vp, lfs_match_tindir);

	fip = sp->fip;
	if (fip->fi_nblocks != 0) {
		sp->fip = (FINFO *) ((caddr_t) fip + FINFOSIZE +
		    sizeof(ufs_daddr_t) * (fip->fi_nblocks));
		sp->start_lbp = &sp->fip->fi_blocks[0];
	} else {
		sp->sum_bytes_left += FINFOSIZE;
		--((SEGSUM *) (sp->segsum))->ss_nfinfo;
	}
}

int
lfs_writeinode(struct lfs * fs, struct segment * sp, struct inode * ip)
{
	struct ubuf *bp, *ibp;
	struct ufs1_dinode *cdp;
	IFILE *ifp;
	SEGUSE *sup;
	daddr_t daddr;
	ino_t ino;
	int error, i, ndx, fsb = 0;
	int redo_ifile = 0;
	struct timespec ts;
	int gotblk = 0;

	/* Allocate a new inode block if necessary. */
	if ((ip->i_number != LFS_IFILE_INUM || sp->idp == NULL) &&
	    sp->ibp == NULL) {
		/* Allocate a new segment if necessary. */
		if (sp->seg_bytes_left < fs->lfs_ibsize ||
		    sp->sum_bytes_left < sizeof(ufs_daddr_t))
			(void) lfs_writeseg(fs, sp);

		/* Get next inode block. */
		daddr = fs->lfs_offset;
		fs->lfs_offset += btofsb(fs, fs->lfs_ibsize);
		sp->ibp = *sp->cbpp++ =
		    getblk(fs->lfs_devvp, fsbtodb(fs, daddr),
		    fs->lfs_ibsize);
		sp->ibp->b_flags |= B_GATHERED;
		gotblk++;

		/* Zero out inode numbers */
		for (i = 0; i < INOPB(fs); ++i)
			((struct ufs1_dinode *) sp->ibp->b_data)[i].di_inumber = 0;

		++sp->start_bpp;
		fs->lfs_avail -= btofsb(fs, fs->lfs_ibsize);
		/* Set remaining space counters. */
		sp->seg_bytes_left -= fs->lfs_ibsize;
		sp->sum_bytes_left -= sizeof(ufs_daddr_t);
		ndx = fs->lfs_sumsize / sizeof(ufs_daddr_t) -
		    sp->ninodes / INOPB(fs) - 1;
		((ufs_daddr_t *) (sp->segsum))[ndx] = daddr;
	}
	/* Update the inode times and copy the inode onto the inode page. */
	ts.tv_nsec = 0;
	ts.tv_sec = write_time;
	/* XXX kludge --- don't redirty the ifile just to put times on it */
	if (ip->i_number != LFS_IFILE_INUM)
		LFS_ITIMES(ip, &ts, &ts, &ts);

	/*
	 * If this is the Ifile, and we've already written the Ifile in this
	 * partial segment, just overwrite it (it's not on disk yet) and
	 * continue.
	 *
	 * XXX we know that the bp that we get the second time around has
	 * already been gathered.
	 */
	if (ip->i_number == LFS_IFILE_INUM && sp->idp) {
		*(sp->idp) = *ip->i_din.ffs1_din;
		ip->i_lfs_osize = ip->i_ffs1_size;
		return 0;
	}
	bp = sp->ibp;
	cdp = ((struct ufs1_dinode *) bp->b_data) + (sp->ninodes % INOPB(fs));
	*cdp = *ip->i_din.ffs1_din;

	/* If all blocks are goig to disk, update the "size on disk" */
	ip->i_lfs_osize = ip->i_ffs1_size;

	if (ip->i_number == LFS_IFILE_INUM)	/* We know sp->idp == NULL */
		sp->idp = ((struct ufs1_dinode *) bp->b_data) +
		    (sp->ninodes % INOPB(fs));
	if (gotblk) {
		LFS_LOCK_BUF(bp);
		assert(!(bp->b_flags & B_INVAL));
		brelse(bp);
	}
	/* Increment inode count in segment summary block. */
	++((SEGSUM *) (sp->segsum))->ss_ninos;

	/* If this page is full, set flag to allocate a new page. */
	if (++sp->ninodes % INOPB(fs) == 0)
		sp->ibp = NULL;

	/*
	 * If updating the ifile, update the super-block.  Update the disk
	 * address and access times for this inode in the ifile.
	 */
	ino = ip->i_number;
	if (ino == LFS_IFILE_INUM) {
		daddr = fs->lfs_idaddr;
		fs->lfs_idaddr = dbtofsb(fs, bp->b_blkno);
	} else {
		LFS_IENTRY(ifp, fs, ino, ibp);
		daddr = ifp->if_daddr;
		ifp->if_daddr = dbtofsb(fs, bp->b_blkno) + fsb;
		error = LFS_BWRITE_LOG(ibp);	/* Ifile */
	}

	/*
	 * Account the inode: it no longer belongs to its former segment,
	 * though it will not belong to the new segment until that segment
	 * is actually written.
	 */
	if (daddr != LFS_UNUSED_DADDR) {
		u_int32_t oldsn = dtosn(fs, daddr);
		LFS_SEGENTRY(sup, fs, oldsn, bp);
		sup->su_nbytes -= DINODE1_SIZE;
		redo_ifile =
		    (ino == LFS_IFILE_INUM && !(bp->b_flags & B_GATHERED));
		if (redo_ifile)
			fs->lfs_flags |= LFS_IFDIRTY;
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp);	/* Ifile */
	}
	return redo_ifile;
}

int
lfs_gatherblock(struct segment * sp, struct ubuf * bp)
{
	struct lfs *fs;
	int version;
	int j, blksinblk;

	/*
	 * If full, finish this segment.  We may be doing I/O, so
	 * release and reacquire the splbio().
	 */
	fs = sp->fs;
	blksinblk = howmany(bp->b_bcount, fs->lfs_bsize);
	if (sp->sum_bytes_left < sizeof(ufs_daddr_t) * blksinblk ||
	    sp->seg_bytes_left < bp->b_bcount) {
		lfs_updatemeta(sp);

		version = sp->fip->fi_version;
		(void) lfs_writeseg(fs, sp);

		sp->fip->fi_version = version;
		sp->fip->fi_ino = VTOI(sp->vp)->i_number;
		/* Add the current file to the segment summary. */
		++((SEGSUM *) (sp->segsum))->ss_nfinfo;
		sp->sum_bytes_left -= FINFOSIZE;

		return 1;
	}
	/* Insert into the buffer list, update the FINFO block. */
	bp->b_flags |= B_GATHERED;
	/* bp->b_flags &= ~B_DONE; */

	*sp->cbpp++ = bp;
	for (j = 0; j < blksinblk; j++)
		sp->fip->fi_blocks[sp->fip->fi_nblocks++] = bp->b_lblkno + j;

	sp->sum_bytes_left -= sizeof(ufs_daddr_t) * blksinblk;
	sp->seg_bytes_left -= bp->b_bcount;
	return 0;
}

int
lfs_gather(struct lfs * fs, struct segment * sp, struct uvnode * vp, int (*match) (struct lfs *, struct ubuf *))
{
	struct ubuf *bp, *nbp;
	int count = 0;

	sp->vp = vp;
loop:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);

		assert(bp->b_flags & B_DELWRI);
		if ((bp->b_flags & (B_BUSY | B_GATHERED)) || !match(fs, bp)) {
			continue;
		}
		if (lfs_gatherblock(sp, bp)) {
			goto loop;
		}
		count++;
	}

	lfs_updatemeta(sp);
	sp->vp = NULL;
	return count;
}


/*
 * Change the given block's address to ndaddr, finding its previous
 * location using ufs_bmaparray().
 *
 * Account for this change in the segment table.
 */
void
lfs_update_single(struct lfs * fs, struct segment * sp, daddr_t lbn,
    ufs_daddr_t ndaddr, int size)
{
	SEGUSE *sup;
	struct ubuf *bp;
	struct indir a[NIADDR + 2], *ap;
	struct inode *ip;
	struct uvnode *vp;
	daddr_t daddr, ooff;
	int num, error;
	int bb, osize, obb;

	vp = sp->vp;
	ip = VTOI(vp);

	error = ufs_bmaparray(fs, vp, lbn, &daddr, a, &num);
	if (error)
		errx(1, "lfs_updatemeta: ufs_bmaparray returned %d looking up lbn %" PRId64 "\n", error, lbn);
	if (daddr > 0)
		daddr = dbtofsb(fs, daddr);

	bb = fragstofsb(fs, numfrags(fs, size));
	switch (num) {
	case 0:
		ooff = ip->i_ffs1_db[lbn];
		if (ooff == UNWRITTEN)
			ip->i_ffs1_blocks += bb;
		else {
			/* possible fragment truncation or extension */
			obb = btofsb(fs, ip->i_lfs_fragsize[lbn]);
			ip->i_ffs1_blocks += (bb - obb);
		}
		ip->i_ffs1_db[lbn] = ndaddr;
		break;
	case 1:
		ooff = ip->i_ffs1_ib[a[0].in_off];
		if (ooff == UNWRITTEN)
			ip->i_ffs1_blocks += bb;
		ip->i_ffs1_ib[a[0].in_off] = ndaddr;
		break;
	default:
		ap = &a[num - 1];
		if (bread(vp, ap->in_lbn, fs->lfs_bsize, NULL, &bp))
			errx(1, "lfs_updatemeta: bread bno %" PRId64,
			    ap->in_lbn);

		ooff = ((ufs_daddr_t *) bp->b_data)[ap->in_off];
		if (ooff == UNWRITTEN)
			ip->i_ffs1_blocks += bb;
		((ufs_daddr_t *) bp->b_data)[ap->in_off] = ndaddr;
		(void) VOP_BWRITE(bp);
	}

	/*
	 * Update segment usage information, based on old size
	 * and location.
	 */
	if (daddr > 0) {
		u_int32_t oldsn = dtosn(fs, daddr);
		if (lbn >= 0 && lbn < NDADDR)
			osize = ip->i_lfs_fragsize[lbn];
		else
			osize = fs->lfs_bsize;
		LFS_SEGENTRY(sup, fs, oldsn, bp);
		sup->su_nbytes -= osize;
		if (!(bp->b_flags & B_GATHERED))
			fs->lfs_flags |= LFS_IFDIRTY;
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp);
	}
	/*
	 * Now that this block has a new address, and its old
	 * segment no longer owns it, we can forget about its
	 * old size.
	 */
	if (lbn >= 0 && lbn < NDADDR)
		ip->i_lfs_fragsize[lbn] = size;
}

/*
 * Update the metadata that points to the blocks listed in the FINFO
 * array.
 */
void
lfs_updatemeta(struct segment * sp)
{
	struct ubuf *sbp;
	struct lfs *fs;
	struct uvnode *vp;
	daddr_t lbn;
	int i, nblocks, num;
	int bb;
	int bytesleft, size;

	vp = sp->vp;
	nblocks = &sp->fip->fi_blocks[sp->fip->fi_nblocks] - sp->start_lbp;

	if (vp == NULL || nblocks == 0)
		return;

	/*
	 * This count may be high due to oversize blocks from lfs_gop_write.
	 * Correct for this. (XXX we should be able to keep track of these.)
	 */
	fs = sp->fs;
	for (i = 0; i < nblocks; i++) {
		if (sp->start_bpp[i] == NULL) {
			printf("nblocks = %d, not %d\n", i, nblocks);
			nblocks = i;
			break;
		}
		num = howmany(sp->start_bpp[i]->b_bcount, fs->lfs_bsize);
		nblocks -= num - 1;
	}

	/*
	 * Sort the blocks.
	 */
	lfs_shellsort(sp->start_bpp, sp->start_lbp, nblocks, fs->lfs_bsize);

	/*
	 * Record the length of the last block in case it's a fragment.
	 * If there are indirect blocks present, they sort last.  An
	 * indirect block will be lfs_bsize and its presence indicates
	 * that you cannot have fragments.
	 */
	sp->fip->fi_lastlength = ((sp->start_bpp[nblocks - 1]->b_bcount - 1) &
	    fs->lfs_bmask) + 1;

	/*
	 * Assign disk addresses, and update references to the logical
	 * block and the segment usage information.
	 */
	for (i = nblocks; i--; ++sp->start_bpp) {
		sbp = *sp->start_bpp;
		lbn = *sp->start_lbp;

		sbp->b_blkno = fsbtodb(fs, fs->lfs_offset);

		/*
		 * If we write a frag in the wrong place, the cleaner won't
		 * be able to correctly identify its size later, and the
		 * segment will be uncleanable.	 (Even worse, it will assume
		 * that the indirect block that actually ends the list
		 * is of a smaller size!)
		 */
		if ((sbp->b_bcount & fs->lfs_bmask) && i != 0)
			errx(1, "lfs_updatemeta: fragment is not last block");

		/*
		 * For each subblock in this possibly oversized block,
		 * update its address on disk.
		 */
		for (bytesleft = sbp->b_bcount; bytesleft > 0;
		    bytesleft -= fs->lfs_bsize) {
			size = MIN(bytesleft, fs->lfs_bsize);
			bb = fragstofsb(fs, numfrags(fs, size));
			lbn = *sp->start_lbp++;
			lfs_update_single(fs, sp, lbn, fs->lfs_offset, size);
			fs->lfs_offset += bb;
		}

	}
}

/*
 * Start a new segment.
 */
int
lfs_initseg(struct lfs * fs)
{
	struct segment *sp;
	SEGUSE *sup;
	SEGSUM *ssp;
	struct ubuf *bp, *sbp;
	int repeat;

	sp = fs->lfs_sp;

	repeat = 0;

	/* Advance to the next segment. */
	if (!LFS_PARTIAL_FITS(fs)) {
		/* lfs_avail eats the remaining space */
		fs->lfs_avail -= fs->lfs_fsbpseg - (fs->lfs_offset -
		    fs->lfs_curseg);
		lfs_newseg(fs);
		repeat = 1;
		fs->lfs_offset = fs->lfs_curseg;

		sp->seg_number = dtosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = fsbtob(fs, fs->lfs_fsbpseg);

		/*
		 * If the segment contains a superblock, update the offset
		 * and summary address to skip over it.
		 */
		LFS_SEGENTRY(sup, fs, sp->seg_number, bp);
		if (sup->su_flags & SEGUSE_SUPERBLOCK) {
			fs->lfs_offset += btofsb(fs, LFS_SBPAD);
			sp->seg_bytes_left -= LFS_SBPAD;
		}
		brelse(bp);
		/* Segment zero could also contain the labelpad */
		if (fs->lfs_version > 1 && sp->seg_number == 0 &&
		    fs->lfs_start < btofsb(fs, LFS_LABELPAD)) {
			fs->lfs_offset += btofsb(fs, LFS_LABELPAD) - fs->lfs_start;
			sp->seg_bytes_left -= LFS_LABELPAD - fsbtob(fs, fs->lfs_start);
		}
	} else {
		sp->seg_number = dtosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = fsbtob(fs, fs->lfs_fsbpseg -
		    (fs->lfs_offset - fs->lfs_curseg));
	}
	fs->lfs_lastpseg = fs->lfs_offset;

	sp->fs = fs;
	sp->ibp = NULL;
	sp->idp = NULL;
	sp->ninodes = 0;
	sp->ndupino = 0;

	/* Get a new buffer for SEGSUM and enter it into the buffer list. */
	sp->cbpp = sp->bpp;
	sbp = *sp->cbpp = getblk(fs->lfs_devvp,
	    fsbtodb(fs, fs->lfs_offset), fs->lfs_sumsize);
	sp->segsum = sbp->b_data;
	memset(sp->segsum, 0, fs->lfs_sumsize);
	sp->start_bpp = ++sp->cbpp;
	fs->lfs_offset += btofsb(fs, fs->lfs_sumsize);

	/* Set point to SEGSUM, initialize it. */
	ssp = sp->segsum;
	ssp->ss_next = fs->lfs_nextseg;
	ssp->ss_nfinfo = ssp->ss_ninos = 0;
	ssp->ss_magic = SS_MAGIC;

	/* Set pointer to first FINFO, initialize it. */
	sp->fip = (struct finfo *) ((caddr_t) sp->segsum + SEGSUM_SIZE(fs));
	sp->fip->fi_nblocks = 0;
	sp->start_lbp = &sp->fip->fi_blocks[0];
	sp->fip->fi_lastlength = 0;

	sp->seg_bytes_left -= fs->lfs_sumsize;
	sp->sum_bytes_left = fs->lfs_sumsize - SEGSUM_SIZE(fs);

	LFS_LOCK_BUF(sbp);
	brelse(sbp);
	return repeat;
}

/*
 * Return the next segment to write.
 */
void
lfs_newseg(struct lfs * fs)
{
	CLEANERINFO *cip;
	SEGUSE *sup;
	struct ubuf *bp;
	int curseg, isdirty, sn;

	LFS_SEGENTRY(sup, fs, dtosn(fs, fs->lfs_nextseg), bp);
	sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
	sup->su_nbytes = 0;
	sup->su_nsums = 0;
	sup->su_ninos = 0;
	LFS_WRITESEGENTRY(sup, fs, dtosn(fs, fs->lfs_nextseg), bp);

	LFS_CLEANERINFO(cip, fs, bp);
	--cip->clean;
	++cip->dirty;
	fs->lfs_nclean = cip->clean;
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);

	fs->lfs_lastseg = fs->lfs_curseg;
	fs->lfs_curseg = fs->lfs_nextseg;
	for (sn = curseg = dtosn(fs, fs->lfs_curseg) + fs->lfs_interleave;;) {
		sn = (sn + 1) % fs->lfs_nseg;
		if (sn == curseg)
			errx(1, "lfs_nextseg: no clean segments");
		LFS_SEGENTRY(sup, fs, sn, bp);
		isdirty = sup->su_flags & SEGUSE_DIRTY;
		brelse(bp);

		if (!isdirty)
			break;
	}

	++fs->lfs_nactive;
	fs->lfs_nextseg = sntod(fs, sn);
}


int
lfs_writeseg(struct lfs * fs, struct segment * sp)
{
	struct ubuf **bpp, *bp;
	SEGUSE *sup;
	SEGSUM *ssp;
	char *datap, *dp;
	int i;
	int do_again, nblocks, byteoffset;
	size_t el_size;
	u_short ninos;
	struct uvnode *devvp;

	/*
	 * If there are no buffers other than the segment summary to write
	 * and it is not a checkpoint, don't do anything.  On a checkpoint,
	 * even if there aren't any buffers, you need to write the superblock.
	 */
	if ((nblocks = sp->cbpp - sp->bpp) == 1)
		return 0;

	devvp = fs->lfs_devvp;

	/* Update the segment usage information. */
	LFS_SEGENTRY(sup, fs, sp->seg_number, bp);

	/* Loop through all blocks, except the segment summary. */
	for (bpp = sp->bpp; ++bpp < sp->cbpp;) {
		if ((*bpp)->b_vp != devvp) {
			sup->su_nbytes += (*bpp)->b_bcount;
		}
	}

	ssp = (SEGSUM *) sp->segsum;

	ninos = (ssp->ss_ninos + INOPB(fs) - 1) / INOPB(fs);
	sup->su_nbytes += ssp->ss_ninos * DINODE1_SIZE;

	if (fs->lfs_version == 1)
		sup->su_olastmod = write_time;
	else
		sup->su_lastmod = write_time;
	sup->su_ninos += ninos;
	++sup->su_nsums;
	fs->lfs_dmeta += (btofsb(fs, fs->lfs_sumsize) + btofsb(fs, ninos *
		fs->lfs_ibsize));
	fs->lfs_avail -= btofsb(fs, fs->lfs_sumsize);

	do_again = !(bp->b_flags & B_GATHERED);
	LFS_WRITESEGENTRY(sup, fs, sp->seg_number, bp);	/* Ifile */

	/*
	 * Compute checksum across data and then across summary; the first
	 * block (the summary block) is skipped.  Set the create time here
	 * so that it's guaranteed to be later than the inode mod times.
	 */
	if (fs->lfs_version == 1)
		el_size = sizeof(u_long);
	else
		el_size = sizeof(u_int32_t);
	datap = dp = malloc(nblocks * el_size);
	if (dp == NULL)
		err(1, NULL);
	for (bpp = sp->bpp, i = nblocks - 1; i--;) {
		++bpp;
		/* Loop through gop_write cluster blocks */
		for (byteoffset = 0; byteoffset < (*bpp)->b_bcount;
		    byteoffset += fs->lfs_bsize) {
			memcpy(dp, (*bpp)->b_data + byteoffset, el_size);
			dp += el_size;
		}
		bremfree(*bpp);
		(*bpp)->b_flags |= B_BUSY;
	}
	if (fs->lfs_version == 1)
		ssp->ss_ocreate = write_time;
	else {
		ssp->ss_create = write_time;
		ssp->ss_serial = ++fs->lfs_serial;
		ssp->ss_ident = fs->lfs_ident;
	}
	/* Set the summary block busy too */
	bremfree(*(sp->bpp));
	(*(sp->bpp))->b_flags |= B_BUSY;

	ssp->ss_datasum = cksum(datap, (nblocks - 1) * el_size);
	ssp->ss_sumsum =
	    cksum(&ssp->ss_datasum, fs->lfs_sumsize - sizeof(ssp->ss_sumsum));
	free(datap);
	datap = dp = NULL;
	fs->lfs_bfree -= (btofsb(fs, ninos * fs->lfs_ibsize) +
	    btofsb(fs, fs->lfs_sumsize));

	if (devvp == NULL)
		errx(1, "devvp is NULL");
	for (bpp = sp->bpp, i = nblocks; i; bpp++, i--) {
		bp = *bpp;
#if 0
		printf("i = %d, bp = %p, flags %lx, bn = %" PRIx64 "\n",
		       nblocks - i, bp, bp->b_flags, bp->b_blkno);
		printf("  vp = %p\n", bp->b_vp);
		if (bp->b_vp != fs->lfs_devvp)
			printf("  ino = %d lbn = %" PRId64 "\n",
			       VTOI(bp->b_vp)->i_number, bp->b_lblkno);
#endif
		if (bp->b_vp == fs->lfs_devvp)
			written_dev += bp->b_bcount;
		else {
			if (bp->b_lblkno >= 0)
				written_data += bp->b_bcount;
			else
				written_indir += bp->b_bcount;
		}
		bp->b_flags &= ~(B_DELWRI | B_READ | B_GATHERED | B_ERROR |
				 B_LOCKED);
		bwrite(bp);
		written_bytes += bp->b_bcount;
	}
	written_inodes += ninos;

	return (lfs_initseg(fs) || do_again);
}

/*
 * Our own copy of shellsort.  XXX use qsort or heapsort.
 */
void
lfs_shellsort(struct ubuf ** bp_array, ufs_daddr_t * lb_array, int nmemb, int size)
{
	static int __rsshell_increments[] = {4, 1, 0};
	int incr, *incrp, t1, t2;
	struct ubuf *bp_temp;

	for (incrp = __rsshell_increments; (incr = *incrp++) != 0;)
		for (t1 = incr; t1 < nmemb; ++t1)
			for (t2 = t1 - incr; t2 >= 0;)
				if ((u_int32_t) bp_array[t2]->b_lblkno >
				    (u_int32_t) bp_array[t2 + incr]->b_lblkno) {
					bp_temp = bp_array[t2];
					bp_array[t2] = bp_array[t2 + incr];
					bp_array[t2 + incr] = bp_temp;
					t2 -= incr;
				} else
					break;

	/* Reform the list of logical blocks */
	incr = 0;
	for (t1 = 0; t1 < nmemb; t1++) {
		for (t2 = 0; t2 * size < bp_array[t1]->b_bcount; t2++) {
			lb_array[incr++] = bp_array[t1]->b_lblkno + t2;
		}
	}
}


/*
 * lfs_seglock --
 *	Single thread the segment writer.
 */
int
lfs_seglock(struct lfs * fs, unsigned long flags)
{
	struct segment *sp;

	if (fs->lfs_seglock) {
		++fs->lfs_seglock;
		fs->lfs_sp->seg_flags |= flags;
		return 0;
	}
	fs->lfs_seglock = 1;

	sp = fs->lfs_sp = (struct segment *) malloc(sizeof(*sp));
	if (sp == NULL)
		err(1, NULL);
	sp->bpp = (struct ubuf **) malloc(fs->lfs_ssize * sizeof(struct ubuf *));
	if (!sp->bpp)
		errx(!preen, "Could not allocate %zu bytes: %s",
			(size_t)(fs->lfs_ssize * sizeof(struct ubuf *)),
			strerror(errno));
	sp->seg_flags = flags;
	sp->vp = NULL;
	sp->seg_iocount = 0;
	(void) lfs_initseg(fs);

	return 0;
}

/*
 * lfs_segunlock --
 *	Single thread the segment writer.
 */
void
lfs_segunlock(struct lfs * fs)
{
	struct segment *sp;
	struct ubuf *bp;

	sp = fs->lfs_sp;

	if (fs->lfs_seglock == 1) {
		if (sp->bpp != sp->cbpp) {
			/* Free allocated segment summary */
			fs->lfs_offset -= btofsb(fs, fs->lfs_sumsize);
			bp = *sp->bpp;
			bremfree(bp);
			bp->b_flags |= B_DONE | B_INVAL;
			bp->b_flags &= ~B_DELWRI;
			reassignbuf(bp, bp->b_vp);
			bp->b_flags |= B_BUSY; /* XXX */
			brelse(bp);
		} else
			printf("unlock to 0 with no summary");

		free(sp->bpp);
		sp->bpp = NULL;
		free(sp);
		fs->lfs_sp = NULL;

		fs->lfs_nactive = 0;

		/* Since we *know* everything's on disk, write both sbs */
		lfs_writesuper(fs, fs->lfs_sboffs[0]);
		lfs_writesuper(fs, fs->lfs_sboffs[1]);

		--fs->lfs_seglock;
		fs->lfs_lockpid = 0;
	} else if (fs->lfs_seglock == 0) {
		errx(1, "Seglock not held");
	} else {
		--fs->lfs_seglock;
	}
}

int
lfs_writevnodes(struct lfs *fs, struct segment *sp, int op)
{
	struct inode *ip;
	struct uvnode *vp;
	int inodes_written = 0;
	
	LIST_FOREACH(vp, &vnodelist, v_mntvnodes) {
		if (vp->v_bmap_op != lfs_vop_bmap)
			continue;

		ip = VTOI(vp);

		if ((op == VN_DIROP && !(vp->v_flag & VDIROP)) ||
		    (op != VN_DIROP && (vp->v_flag & VDIROP))) {
			continue;
		}
		/*
		 * Write the inode/file if dirty and it's not the IFILE.
		 */
		if (ip->i_flag & IN_ALLMOD || !LIST_EMPTY(&vp->v_dirtyblkhd)) {
			if (ip->i_number != LFS_IFILE_INUM)
				lfs_writefile(fs, sp, vp);
			(void) lfs_writeinode(fs, sp, ip);
			inodes_written++;
		}
	}
	return inodes_written;
}

void
lfs_writesuper(struct lfs *fs, ufs_daddr_t daddr)
{
	struct ubuf *bp;

	/* Set timestamp of this version of the superblock */
	if (fs->lfs_version == 1)
		fs->lfs_otstamp = write_time;
	fs->lfs_tstamp = write_time;

	/* Checksum the superblock and copy it into a buffer. */
	fs->lfs_cksum = lfs_sb_cksum(&(fs->lfs_dlfs));
	assert(daddr > 0);
	bp = getblk(fs->lfs_devvp, fsbtodb(fs, daddr), LFS_SBPAD);
	memset(bp->b_data + sizeof(struct dlfs), 0,
	    LFS_SBPAD - sizeof(struct dlfs));
	*(struct dlfs *) bp->b_data = fs->lfs_dlfs;

	bwrite(bp);
}
