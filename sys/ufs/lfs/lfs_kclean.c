/*	$NetBSD: lfs_kclean.c,v 1.4 2026/01/05 05:02:47 perseant Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_kclean.c,v 1.4 2026/01/05 05:02:47 perseant Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/buf.h>
#include <sys/kthread.h>

#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_kernel.h>
#include <ufs/lfs/lfs_extern.h>

static int ino_func_setclean(struct lfs_inofuncarg *);
static int finfo_func_rewrite(struct lfs_finfofuncarg *);
static int finfo_func_setclean(struct lfs_finfofuncarg *);
static int rewrite_block(struct lfs *, struct vnode *, daddr_t, daddr_t,
			 size_t, int *);

static int ino_func_rewrite(struct lfs_inofuncarg *);
static int ino_func_setclean(struct lfs_inofuncarg *);
static int ino_func_checkempty(struct lfs_inofuncarg *);

static int clean(struct lfs *);
static long segselect_cb_rosenblum(struct lfs *, int, SEGUSE *, long);
static long segselect_greedy(struct lfs *, int, SEGUSE *);
static long segselect_cb_time(struct lfs *, int, SEGUSE *);
#if 0
static long segselect_cb_serial(struct lfs *, int, SEGUSE *);
#endif
static int check_clean_list(struct lfs *, ino_t);

struct lwp * lfs_cleaner_daemon = NULL;
extern kcondvar_t	lfs_allclean_wakeup;
static int lfs_ncleaners = 0;

static int
ino_func_setclean(struct lfs_inofuncarg *lifa)
{
	struct lfs *fs;
	daddr_t offset;
	struct vnode *devvp, *vp;
	union lfs_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	IFILE *ifp;
	unsigned i, num;
	daddr_t true_addr;
	ino_t ino;

	fs = lifa->fs;
	offset = lifa->offset;
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/* Read inode block */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getibsize(fs),
	    0, &dbp);
	if (error) {
		DLOG((DLOG_RF, "ino_func_setclean: bread returned %d\n",
		      error));
		return error;
	}
	memcpy(lifa->buf, dbp->b_data, dbp->b_bcount);
	brelse(dbp, BC_AGE);

	/* Check each inode against ifile entry */
	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, lifa->buf, i);
		ino = lfs_dino_getinumber(fs, dip);
		if (ino == LFS_IFILE_INUM) {
			/* Check address against superblock */
			true_addr = lfs_sb_getidaddr(fs);
		} else {
			/* Not ifile.  Check address against ifile. */
			LFS_IENTRY(ifp, fs, ino, ibp);
			true_addr = lfs_if_getdaddr(fs, ifp);
			brelse(ibp, 0);
		}
		if (offset != true_addr)
			continue;

		LFS_ASSERT_MAXINO(fs, ino);

		/* XXX We can use fastvget here! */

		/*
		 * An inode we need to relocate.
		 * Get it if we can.
		 */
		if (ino == LFS_IFILE_INUM)
			vp = fs->lfs_ivnode;
		else
			error = VFS_VGET(fs->lfs_ivnode->v_mount, ino,
					 LK_EXCLUSIVE | LK_NOWAIT, &vp);
		if (error)
			continue;

		KASSERT(VTOI(vp)->i_gen == lfs_dino_getgen(fs, dip));
		lfs_setclean(fs, vp);
		if (vp != fs->lfs_ivnode) {
			VOP_UNLOCK(vp);
			vrele(vp);
		}
	}

	return error;
}

static int
ino_func_rewrite(struct lfs_inofuncarg *lifa)
{
	struct lfs *fs;
	daddr_t offset;
	struct vnode *devvp, *vp;
	union lfs_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	IFILE *ifp;
	unsigned i, num;
	daddr_t true_addr;
	ino_t ino;

	fs = lifa->fs;
	offset = lifa->offset;
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/* Read inode block */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getibsize(fs),
	    0, &dbp);
	if (error) {
		DLOG((DLOG_RF, "ino_func_rewrite: bread returned %d\n",
		      error));
		return error;
	}
	memcpy(lifa->buf, dbp->b_data, dbp->b_bcount);
	brelse(dbp, BC_AGE);

	/* Check each inode against ifile entry */
	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, lifa->buf, i);
		ino = lfs_dino_getinumber(fs, dip);
		if (ino == LFS_IFILE_INUM) {
			/* Check address against superblock */
			true_addr = lfs_sb_getidaddr(fs);
		} else {
			/* Not ifile.  Check address against ifile. */
			LFS_IENTRY(ifp, fs, ino, ibp);
			true_addr = lfs_if_getdaddr(fs, ifp);
			brelse(ibp, 0);
		}
		if (offset != true_addr)
			continue;

		if (ino == LFS_IFILE_INUM)
			continue;

		LFS_ASSERT_MAXINO(fs, ino);

		/* XXX We can use fastvget here! */

		/*
		 * An inode we need to relocate.
		 * Get it if we can.
		 */
		error = check_clean_list(fs, ino);
		if (error)
			continue;
		error = VFS_VGET(fs->lfs_ivnode->v_mount, ino,
				 LK_EXCLUSIVE | LK_NOWAIT, &vp);
		if (error)
			continue;

		KASSERT(VTOI(vp)->i_gen == lfs_dino_getgen(fs, dip));
		
		if (!(VTOI(vp)->i_state & IN_CLEANING)) {
			lfs_setclean(fs, vp);
			lfs_writeinode(fs, fs->lfs_sp, VTOI(vp));
		}

		VOP_UNLOCK(vp);
		vrele(vp);

	}

	return error;
}

static int
rewrite_block(struct lfs *fs, struct vnode *vp, daddr_t lbn, daddr_t offset, size_t size, int *have_finfop)
{
	daddr_t daddr;
	int error;
	struct buf *bp;
	struct inode *ip;

	KASSERT(have_finfop != NULL);
	
	/* Look up current location of this block. */
	error = VOP_BMAP(vp, lbn, NULL, &daddr, NULL);
	if (error)
		return error;
	
	/* Skip any block that is not here. */
	if (offset != 0 && LFS_DBTOFSB(fs, daddr) != offset)
		return ESTALE;
	
	/*
	 * It is (was recently) here.  Read the block.
	 */
	//size = lfs_blksize(fs, VTOI(vp), lbn);
	error = bread(vp, lbn, size, 0, &bp);
	if (error)
		return error;

	if (vp == fs->lfs_ivnode) {
		VOP_BWRITE(vp, bp);
	} else {
		/* Get ready to write. */
		if (!*have_finfop) {
			ip = VTOI(vp);
			lfs_acquire_finfo(fs, ip->i_number, ip->i_gen);
			fs->lfs_sp->vp = vp;
			*have_finfop = 1;
		}
	
		KASSERT(bp->b_vp == vp);
		/* bp->b_cflags |= BC_INVAL; */ /* brelse will kill the buffer */
		error = lfs_bwrite_ext(bp, BW_CLEAN);
		if (error)
			return error;
		KASSERT(bp->b_vp == vp);
		mutex_enter(&bufcache_lock);
		while (lfs_gatherblock(fs->lfs_sp, bp, &bufcache_lock)) {
			KASSERT(bp->b_vp != NULL);
		}
		mutex_exit(&bufcache_lock);
	
		KASSERT(bp->b_flags & B_GATHERED);
		KASSERT(fs->lfs_sp->cbpp[-1] == bp);
	}
	return 0;
}

static int
check_clean_list(struct lfs *fs, ino_t ino)
{
	struct inode *ip;
	
	/*
	 * Look for the inode on the clean list.
	 * If it is not there, we can't lock it without risking a deadlock.
	 */
	TAILQ_FOREACH(ip, &fs->lfs_cleanhd, i_lfs_clean) {
		if (ip->i_number == ino) {
			return 0;
		}
	}
	return EWOULDBLOCK;
}

static int
finfo_func_rewrite(struct lfs_finfofuncarg *lffa)
{
	struct lfs *fs;
	FINFO *fip;
	daddr_t *offsetp;
	int j, have_finfo, error;
	size_t size, bytes;
	ino_t ino;
	uint32_t gen;
	struct vnode *vp;
	daddr_t lbn;
	int *fragsp;

	fs = lffa->fs;
	fip = lffa->finfop;
	offsetp = lffa->offsetp;
	fragsp = (int *)lffa->arg;

	/* Get the inode and check its version. */
	ino = lfs_fi_getino(fs, fip);
	gen = lfs_fi_getversion(fs, fip);
	error = 0;
	if (ino == LFS_IFILE_INUM)
		vp = fs->lfs_ivnode;
	else {
		LFS_ASSERT_MAXINO(fs, ino);
		error = check_clean_list(fs, ino);
		if (error)
			vp = NULL;
		else
			error = VFS_VGET(fs->lfs_ivnode->v_mount, ino,
					 LK_EXCLUSIVE|LK_NOWAIT, &vp);
	}

	/*
	 * If we can't, or if version is wrong, or it has dirop blocks on it,
	 * we can't relocate its blocks; but we still have to count
	 * blocks through the partial segment to return the right offset.
	 * XXX actually we can move DIROP vnodes' *old* data, as long
	 * XXX as we are sure that we are moving *only* the old data---?
	 */
	if (error || VTOI(vp)->i_gen != gen || (vp->v_uflag & VU_DIROP)) {
		if (error == 0)
			error = ESTALE;
		
		if (vp != NULL && vp != fs->lfs_ivnode) {
			VOP_UNLOCK(vp);
			vrele(vp);
		}
		vp = NULL;
		bytes = ((lfs_fi_getnblocks(fs, fip) - 1) << lfs_sb_getbshift(fs))
			+ lfs_fi_getlastlength(fs, fip);
		*offsetp += lfs_btofsb(fs, bytes);
		
		return error;
	}

	/*
	 * We have the vnode and its version is correct.
	 * Take a cleaning reference; and loop through the blocks
	 * and rewrite them.
	 */
	lfs_setclean(fs, vp);
	size = lfs_sb_getbsize(fs);
	have_finfo = 0;
	for (j = 0; j < lfs_fi_getnblocks(fs, fip); ++j) {
		if (j == lfs_fi_getnblocks(fs, fip) - 1)
			size = lfs_fi_getlastlength(fs, fip);
		/*
		 * An error of ESTALE indicates that there was nothing
		 * to rewrite; this is not a problem.  Any other error
		 * causes us to skip the rest of this FINFO.
		 */
		if (vp != NULL && error == 0) {
			lbn = lfs_fi_getblock(fs, fip, j);
			error = rewrite_block(fs, vp, lbn, *offsetp,
					      size, &have_finfo);
			if (error == ESTALE)
				error = 0;
			if (fragsp != NULL && error == 0)
				*fragsp += lfs_btofsb(fs, size);
		}
		*offsetp += lfs_btofsb(fs, size);
	}

	/*
	 * If we acquired finfo, release it and write the blocks.
	 */
	if (have_finfo) {
		lfs_updatemeta(fs->lfs_sp);
		fs->lfs_sp->vp = NULL;
		lfs_release_finfo(fs);
		lfs_writeinode(fs, fs->lfs_sp, VTOI(vp));
	}
	
	/* Release vnode */
	if (vp != fs->lfs_ivnode) {
		VOP_UNLOCK(vp);
		vrele(vp);
	}

	return error;
}

static int
finfo_func_setclean(struct lfs_finfofuncarg *lffa)
{
	struct lfs *fs;
	FINFO *fip;
	daddr_t *offsetp;
	int error;
	size_t bytes;
	ino_t ino;
	uint32_t gen;
	struct vnode *vp;

	fs = lffa->fs;
	fip = lffa->finfop;
	offsetp = lffa->offsetp;

	/* Get the inode and check its version. */
	ino = lfs_fi_getino(fs, fip);
	gen = lfs_fi_getversion(fs, fip);
	error = 0;
	if (ino == LFS_IFILE_INUM)
		vp = fs->lfs_ivnode;
	else {
		LFS_ASSERT_MAXINO(fs, ino);
		error = VFS_VGET(fs->lfs_ivnode->v_mount, ino,
				 LK_EXCLUSIVE|LK_NOWAIT, &vp);
	}

	/* If we have it and its version is right, take a cleaning reference */
	if (error == 0 && VTOI(vp)->i_gen == gen)
		lfs_setclean(fs, vp);

	if (vp == fs->lfs_ivnode)
		vp = NULL;
	else if (vp != NULL) {
		VOP_UNLOCK(vp);
		vrele(vp);
		vp = NULL;
	}

	/* Skip to the next block */
	bytes = ((lfs_fi_getnblocks(fs, fip) - 1) << lfs_sb_getbshift(fs))
		+ lfs_fi_getlastlength(fs, fip);
	*offsetp += lfs_btofsb(fs, bytes);
	
	return error;
}

/*
 * Use the partial-segment parser to rewrite (clean) a segment.
 */
int
lfs_rewrite_segment(struct lfs *fs, int sn, int *fragsp, kauth_cred_t cred, struct lwp *l)
{
	daddr_t ooffset, offset, endpseg;

	ASSERT_SEGLOCK(fs);

	offset = lfs_sntod(fs, sn);
	lfs_skip_superblock(fs, &offset);
	endpseg = lfs_sntod(fs, sn + 1);
	
	while (offset > 0 && offset != endpseg) {
		/* First check summary validity (XXX unnecessary?) */
		ooffset = offset;
		lfs_parse_pseg(fs, &offset, 0, cred, NULL, l,
			     NULL, NULL, CKSEG_CKSUM, NULL);
		if (offset == ooffset)
			break;

		/*
		 * Valid, proceed.
		 *
		 * First write the file blocks, marking their
		 * inodes IN_CLEANING.
		 */
		offset = ooffset;
		lfs_parse_pseg(fs, &offset, 0, cred, NULL, l,
			       NULL, finfo_func_rewrite,
			       CKSEG_NONE, fragsp);

		/*
		 * Now go back and pick up any inodes that
		 * were not already marked IN_CLEANING, and
		 * write them as well.
		 */
		offset = ooffset;
		lfs_parse_pseg(fs, &offset, 0, cred, NULL, l,
			       ino_func_rewrite, NULL,
			       CKSEG_NONE, fragsp);
	}
	return 0;
}

/*
 * Rewrite the contents of one or more segments, in preparation for
 * marking them clean.
 */
int
lfs_rewrite_segments(struct lfs *fs, int *snn, int len, int *directp, int *offsetp, struct lwp *l)
{
	kauth_cred_t cred;
	int i, error;
	struct buf *bp;
	SEGUSE *sup;
	daddr_t offset, endpseg;
	
	ASSERT_NO_SEGLOCK(fs);
	
	cred = l ? l->l_cred : NOCRED;

	/* Prevent new dirops and acquire the cleaner lock. */
	lfs_writer_enter(fs, "rewritesegs");
	if ((error = lfs_cleanerlock(fs)) != 0) {
		lfs_writer_leave(fs);
		return error;
	}
	
	/*
	 * Pre-reference vnodes now that we have cleaner lock
	 * but before we take the segment lock.  We don't want to
	 * mix cleaning blocks with flushed vnodes.
	 */
	for (i = 0; i < len; i++) {
		error = 0;
		/* Refuse to clean segments that are ACTIVE */
		LFS_SEGENTRY(sup, fs, snn[i], bp);
		if (sup->su_flags & SEGUSE_ACTIVE
		    || !(sup->su_flags & SEGUSE_DIRTY))
			error = EINVAL;

		brelse(bp, 0);
		if (error)
			break;

		offset = lfs_sntod(fs, snn[i]);
		lfs_skip_superblock(fs, &offset);
		endpseg = lfs_sntod(fs, snn[i] + 1);
		
		while (offset > 0 && offset != endpseg) {
			lfs_parse_pseg(fs, &offset, 0, cred, NULL, l,
				       ino_func_setclean, finfo_func_setclean,
				       CKSEG_NONE, NULL);
		}
	}

	/*
	 * Actually rewrite the contents of the segment.
	 */
	lfs_seglock(fs, SEGM_CLEAN);

	for (i = 0; i < len; i++) {
		error = 0;
		/* Refuse to clean segments that are ACTIVE */
		LFS_SEGENTRY(sup, fs, snn[i], bp);
		if (sup->su_flags & SEGUSE_ACTIVE
		    || !(sup->su_flags & SEGUSE_DIRTY))
			error = EINVAL;

		brelse(bp, 0);
		if (error)
			break;

		error = lfs_rewrite_segment(fs, snn[i], directp, cred, l);
		if (error)
			break;
	}
	while (lfs_writeseg(fs, fs->lfs_sp))
		;

	*offsetp = lfs_btofsb(fs, fs->lfs_sp->bytes_written);
	lfs_segunlock(fs);
	lfs_cleanerunlock(fs);
	lfs_writer_leave(fs);

	return error;
}

static int
ino_func_checkempty(struct lfs_inofuncarg *lifa)
{
	struct lfs *fs;
	daddr_t offset;
	struct vnode *devvp;
	union lfs_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	IFILE *ifp;
	unsigned i, num;
	daddr_t true_addr;
	ino_t ino;

	fs = lifa->fs;
	offset = lifa->offset;
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/* Read inode block */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getibsize(fs),
	    0, &dbp);
	if (error) {
		DLOG((DLOG_RF, "ino_func_checkempty: bread returned %d\n",
		      error));
		return error;
	}

	/* Check each inode against ifile entry */
	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, dbp->b_data, i);
		ino = lfs_dino_getinumber(fs, dip);
		if (ino == LFS_IFILE_INUM) {
			/* Check address against superblock */
			true_addr = lfs_sb_getidaddr(fs);
		} else {
			/* Not ifile.  Check address against ifile. */
			LFS_IENTRY(ifp, fs, ino, ibp);
			true_addr = lfs_if_getdaddr(fs, ifp);
			brelse(ibp, 0);
		}
		if (offset == true_addr) {
			error = EEXIST;
			break;
		}
	}
	brelse(dbp, BC_AGE);

	return error;
}

static int
finfo_func_checkempty(struct lfs_finfofuncarg *lffa)
{
	struct lfs *fs;
	FINFO *fip;
	daddr_t *offsetp;
	int j, error;
	size_t size, bytes;
	ino_t ino;
	uint32_t gen;
	struct vnode *vp;
	daddr_t lbn, daddr;
	
	fs = lffa->fs;
	fip = lffa->finfop;
	offsetp = lffa->offsetp;

	/* Get the inode and check its version. */
	ino = lfs_fi_getino(fs, fip);
	gen = lfs_fi_getversion(fs, fip);
	error = VFS_VGET(fs->lfs_ivnode->v_mount, ino, LK_EXCLUSIVE|LK_NOWAIT, &vp);

	/*
	 * If we can't, or if version is wrong, this FINFO does not refer
	 * to a live file.  Skip over it and continue.
	 */
	if (error || VTOI(vp)->i_gen != gen) {
		if (error == 0)
			error = ESTALE;
		
		if (vp != NULL) {
			VOP_UNLOCK(vp);
			vrele(vp);
			vp = NULL;
		}
		bytes = ((lfs_fi_getnblocks(fs, fip) - 1)
			 << lfs_sb_getbshift(fs))
			+ lfs_fi_getlastlength(fs, fip);
		*offsetp += lfs_btofsb(fs, bytes);
		
		return error;
	}

	/*
	 * We have the vnode and its version is correct.
	 * Loop through the blocks and check their currency.
	 */
	size = lfs_sb_getbsize(fs);
	for (j = 0; j < lfs_fi_getnblocks(fs, fip); ++j) {
		if (j == lfs_fi_getnblocks(fs, fip) - 1)
			size = lfs_fi_getlastlength(fs, fip);
		if (vp != NULL) {
			lbn = lfs_fi_getblock(fs, fip, j);
			
			/* Look up current location of this block. */
			error = VOP_BMAP(vp, lbn, NULL, &daddr, NULL);
			if (error)
				break;
	
			/* If it is here, the segment is not empty. */
			if (LFS_DBTOFSB(fs, daddr) == *offsetp) {
				error = EEXIST;
				break;
			}
		}
		*offsetp += lfs_btofsb(fs, size);
	}

	/* Release vnode */
	VOP_UNLOCK(vp);
	vrele(vp);

	return error;
}

int
lfs_checkempty(struct lfs *fs, int sn, kauth_cred_t cred, struct lwp *l)
{
	daddr_t offset, endpseg;
	int error;

	ASSERT_SEGLOCK(fs);

	offset = lfs_sntod(fs, sn);
	lfs_skip_superblock(fs, &offset);
	endpseg = lfs_sntod(fs, sn + 1);
	
	while (offset > 0 && offset < endpseg) {
		error = lfs_parse_pseg(fs, &offset, 0, cred, NULL, l,
				     ino_func_checkempty,
				     finfo_func_checkempty,
				     CKSEG_NONE, NULL);
		if (error)
			return error;
	}
	return 0;
}

static long
segselect_greedy(struct lfs *fs, int sn, SEGUSE *sup)
{
	return lfs_sb_getssize(fs) - sup->su_nbytes;
}

__inline static long
segselect_cb_rosenblum(struct lfs *fs, int sn, SEGUSE *sup, long age)
{
	long benefit, cost;

	benefit = (int64_t)lfs_sb_getssize(fs) - sup->su_nbytes -
		(sup->su_nsums + 1) * lfs_sb_getfsize(fs);
	if (sup->su_flags & SEGUSE_SUPERBLOCK)
		benefit -= LFS_SBPAD;
	if (lfs_sb_getbsize(fs) > lfs_sb_getfsize(fs)) /* fragmentation */
		benefit -= (lfs_sb_getbsize(fs) / 2);
	if (benefit <= 0) {
		return 0;
	}

	cost = lfs_sb_getssize(fs) + sup->su_nbytes;
	return (256 * benefit * age) / cost;
}

static long
segselect_cb_time(struct lfs *fs, int sn, SEGUSE *sup)
{
	long age;
	
	age = time_second - sup->su_lastmod;
	if (age < 0)
		age = 0;
	return segselect_cb_rosenblum(fs, sn, sup, age);
}

#if 0
/*
 * Same as the time comparator, but fetch the serial number from the
 * segment header to compare.
 *
 * This is ugly.  Whether serial number or wall time is better is a
 * worthy question, but if we want to use serial number to compute
 * age, we should record the serial number in su_lastmod instead of
 * the time.
 */
static long
segselect_cb_serial(struct lfs *fs, int sn, SEGUSE *sup)
{
	struct buf *bp;
	uint32_t magic;
	uint64_t age, serial;
	daddr_t addr;
	
	addr = lfs_segtod(fs, sn);
	lfs_skip_superblock(fs, &addr);
	bread(fs->lfs_devvp, LFS_FSBTODB(fs, addr),
	      lfs_sb_getsumsize(fs), 0, &bp);
	magic = lfs_ss_getmagic(fs, ((SEGSUM *)bp->b_data));
	serial = lfs_ss_getserial(fs, ((SEGSUM *)bp->b_data));
	brelse(bp, 0);

	if (magic != SS_MAGIC)
		return 0;
	
	age = lfs_sb_getserial(fs) - serial;
	return segselect_cb_rosenblum(fs, sn, sup, age);
}
#endif

void
lfs_cleanerd(void *arg)
{
	mount_iterator_t *iter;
 	struct mount *mp;
 	struct lfs *fs;
	struct vfsops *vfs = NULL;
	int lfsc;
	int cleaned_something = 0;
 
	/* Take an extra reference to the LFS vfsops. */
	vfs = vfs_getopsbyname(MOUNT_LFS);
 
 	mutex_enter(&lfs_lock);
 	for (;;) {
		KASSERT(mutex_owned(&lfs_lock));
		if (cleaned_something == 0)
			cv_timedwait(&lfs_allclean_wakeup, &lfs_lock, hz/10 + 1);
		KASSERT(mutex_owned(&lfs_lock));
		cleaned_something = 0;

		KASSERT(mutex_owned(&lfs_lock));
		mutex_exit(&lfs_lock);
 
 		/*
 		 * Look through the list of LFSs to see if any of them
		 * need cleaning.
 		 */
 		mountlist_iterator_init(&iter);
		lfsc = 0;
		while ((mp = mountlist_iterator_next(iter)) != NULL) {
			KASSERT(!mutex_owned(&lfs_lock));
 			if (strncmp(mp->mnt_stat.f_fstypename, MOUNT_LFS,
 			    sizeof(mp->mnt_stat.f_fstypename)) == 0) {
 				fs = VFSTOULFS(mp)->um_lfs;

				mutex_enter(&lfs_lock);
				if (fs->lfs_clean_selector == NULL) {
					/* Notify cleanctl */
					if (fs->lfs_autoclean_status) {
						fs->lfs_autoclean_status =
							LFS_AUTOCLEAN_STATUS_OFF;
						cv_broadcast(&fs->lfs_cleanquitcv);
					}
				} else
					++lfsc;
				mutex_exit(&lfs_lock);
				cleaned_something += clean(fs);
			}
 		}
		if (lfsc == 0) {
			mutex_enter(&lfs_lock);
			lfs_cleaner_daemon = NULL;
			mutex_exit(&lfs_lock);
			mountlist_iterator_destroy(iter);
			break;
		}
 		mountlist_iterator_destroy(iter);

 		mutex_enter(&lfs_lock);
 	}
	KASSERT(!mutex_owned(&lfs_lock));

	/* Give up our extra reference so the module can be unloaded. */
	mutex_enter(&vfs_list_lock);
	if (vfs != NULL)
		vfs->vfs_refcount--;
	mutex_exit(&vfs_list_lock);

	/* Done! */
	kthread_exit(0);
}

/*
 * Look at the file system to see whether it needs cleaning, and if it does,
 * clean a segment.
 */
static int
clean(struct lfs *fs)
{
	struct buf *bp;
	SEGUSE *sup;
	int sn, maxsn, nclean, nready, nempty, nerror, nzero, again, target;
	long prio, maxprio, maxeprio, thresh;
	long (*func)(struct lfs *, int, SEGUSE *);
	uint32_t __debugused segflags = 0;
	daddr_t oldsn, bfree, avail;
	int direct, offset;

	mutex_enter(&lfs_lock);
	func = fs->lfs_clean_selector;
	mutex_exit(&lfs_lock);
	if (func == NULL)
		return 1; /* Run again so we get cleaned up immediately */

	thresh = fs->lfs_autoclean.thresh;
	if (fs->lfs_flags & LFS_MUSTCLEAN)
		thresh = 0;
	else if (thresh < 0) {
		/*
		 * Compute a priority threshold based on availability ratio.
		 * XXX These numbers only makes sense for the greedy cleaner.
		 * What is an appropriate threshold for the cost-benefit
		 * cleaner?
		 */
		bfree = lfs_sb_getbfree(fs)
			+ lfs_segtod(fs, 1) * lfs_sb_getminfree(fs);
		avail = lfs_sb_getavail(fs) - fs->lfs_ravail - fs->lfs_favail;
		if (avail > bfree)
			return 0;
		thresh = lfs_sb_getssize(fs) * (bfree - avail)
			/ (lfs_sb_getsize(fs) - avail);
		if (thresh > lfs_sb_getsumsize(fs) + 5 * lfs_sb_getbsize(fs))
			thresh = lfs_sb_getsumsize(fs) + 5 * lfs_sb_getbsize(fs);
		if (thresh > lfs_sb_getssize(fs) - lfs_sb_getbsize(fs))
			return 0;
	}
	
	target = fs->lfs_autoclean.target;
	if (target <= 0) {
		/* Default to half a segment target */
		target = lfs_segtod(fs, 1) / 2;
	}

	oldsn = lfs_dtosn(fs, lfs_sb_getoffset(fs));

	again = 0;
	maxprio = maxeprio = -1;
	nzero = nclean = nready = nempty = nerror = 0;
	for (sn = 0; sn < lfs_sb_getnseg(fs); sn++) {
		
		prio = 0;
		LFS_SEGENTRY(sup, fs, sn, bp);
		if (sup->su_flags & SEGUSE_ACTIVE)
			prio = 0;
		else if (!(sup->su_flags & SEGUSE_DIRTY))
			++nclean;
		else if (sup->su_flags & SEGUSE_READY)
			++nready;
		else if (sup->su_flags & SEGUSE_EMPTY)
			++nempty;
		else if (sup->su_nbytes == 0)
			++nzero;
		else
			prio = (*func)(fs, sn, sup);
		
		if (sup->su_flags & SEGUSE_ERROR) {
			if (prio > maxeprio)
				maxeprio = prio;
			prio = 0;
			++nerror;
		}
		
		if (prio > maxprio) {
			maxprio = prio;
			maxsn = sn;
			segflags = sup->su_flags;
		}
		brelse(bp, 0);
	}
	DLOG((DLOG_CLEAN, "%s clean=%d/%d zero=%d empty=%d ready=%d maxsn=%d maxprio=%ld/%ld segflags=0x%lx\n",
	       (maxprio > thresh ? "YES" : "NO "),
	       nclean, (int)lfs_sb_getnseg(fs), nzero, nempty, nready,
	       maxsn, maxprio, (unsigned long)thresh,
	       (unsigned long)segflags));

	/*
	 * If we are trying to clean the segment we cleaned last,
	 * cleaning did not work.  Mark this segment SEGUSE_ERROR
	 * and try again.
	 */
	if (maxprio > 0 && fs->lfs_lastcleaned == maxsn) {
		LFS_SEGENTRY(sup, fs, maxsn, bp);
		sup->su_flags |= SEGUSE_ERROR;
		LFS_WRITESEGENTRY(sup, fs, sn, bp);
		return 1;
	}

	/*
	 * If there were nothing but error segments, clear error.
	 * We will wait to try again.
	 */
	if (maxprio == 0 && maxeprio > 0) {
		DLOG((DLOG_CLEAN, "clear error on %d segments, try again\n",
		      nerror));
		lfs_seguse_clrflag_all(fs, SEGUSE_ERROR);
	}
	
	/* Rewrite the highest-priority segment */
	if (maxprio > thresh) {
		direct = offset = 0;
		(void)lfs_rewrite_segments(fs, &maxsn, 1,
					   &direct, &offset, curlwp);
		DLOG((DLOG_CLEAN, "  direct=%d offset=%d\n", direct, offset));
		again += direct;
		fs->lfs_clean_accum += offset;

		/* Don't clean this again immediately */
		fs->lfs_lastcleaned = maxsn;
	}

	/*
	 * If we are in dire straits but we have segments already
	 * empty, force a double checkpoint to reclaim them.
	 */
	if (fs->lfs_flags & LFS_MUSTCLEAN) {
		if (nready + nempty > 0) {
			DLOG((DLOG_CLEAN, "force checkpoint with nready=%d nempty=%d nzero=%d\n",
				nready, nempty, nzero));
			lfs_segwrite(fs->lfs_ivnode->v_mount,
				     SEGM_CKP | SEGM_FORCE_CKP | SEGM_SYNC);
			lfs_segwrite(fs->lfs_ivnode->v_mount,
				     SEGM_CKP | SEGM_FORCE_CKP | SEGM_SYNC);
			++again;
		}
	} else if (fs->lfs_clean_accum > target) {
		DLOG((DLOG_CLEAN, "checkpoint to flush\n")); 
		lfs_segwrite(fs->lfs_ivnode->v_mount, SEGM_CKP);
		fs->lfs_clean_accum = 0;
	} else if (lfs_dtosn(fs, lfs_sb_getoffset(fs)) != oldsn
		   || nempty + nready > LFS_MAX_ACTIVE) { /* XXX arbitrary */
		DLOG((DLOG_CLEAN, "write to promote empty segments\n"));
		lfs_segwrite(fs->lfs_ivnode->v_mount, SEGM_CKP);
		fs->lfs_clean_accum = 0;
	}

	return again;
}

/*
 * Rewrite a file in its entirety.
 *
 * Generally this would be done to coalesce a file that is scattered
 * around the disk; but if the "scramble" flag is set, instead rewrite
 * only the even-numbered blocks, which provides the opposite effect
 * for testing purposes.
 *
 * It is the caller's responsibility to check the bounds of the inode
 * numbers.
 */
int
lfs_rewrite_file(struct lfs *fs, ino_t *inoa, int len, bool scramble,
		 int *directp, int *offsetp)
{
	daddr_t hiblk, lbn;
	struct vnode *vp;
	struct inode *ip;
	struct buf *bp;
	int i, error;

	KASSERT(directp != NULL);
	KASSERT(offsetp != NULL);

	*directp = 0;
	if ((error = lfs_cleanerlock(fs)) != 0)
		return error;
	lfs_seglock(fs, 0);
	for (i = 0; i < len; ++i) {
		error = VFS_VGET(fs->lfs_ivnode->v_mount, inoa[i],
		    LK_EXCLUSIVE | LK_NOWAIT, &vp);
		if (error)
			goto out;

		ip = VTOI(vp);
		if ((vp->v_uflag & VU_DIROP) || (ip->i_flags & IN_ADIROP)) {
			VOP_UNLOCK(vp);
			vrele(vp);
			error = EAGAIN;
			goto out;
		}

		/* Highest block in this inode */
		hiblk = lfs_lblkno(fs, ip->i_size + lfs_sb_getbsize(fs) - 1) - 1;

		for (lbn = 0; lbn <= hiblk; ++lbn) {
			if (scramble && (lbn & 0x01))
				continue;

			if (lfs_needsflush(fs)) {
				lfs_segwrite(fs->lfs_ivnode->v_mount, 0);
			}

			error = bread(vp, lbn, lfs_blksize(fs, ip, lbn), 0, &bp);
			if (error)
				break;

			/* bp->b_cflags |= BC_INVAL; */
			lfs_bwrite_ext(bp, 0);
			*directp += lfs_btofsb(fs, bp->b_bcount);
		}

		/* Done with this vnode */
		VOP_UNLOCK(vp);
		vrele(vp);
		if (error)
			break;
	}
out:
	lfs_segwrite(fs->lfs_ivnode->v_mount, 0);
	*offsetp += lfs_btofsb(fs, fs->lfs_sp->bytes_written);
	lfs_segunlock(fs);
	lfs_cleanerunlock(fs);

	return error;
}

int
lfs_cleanctl(struct lfs *fs, struct lfs_autoclean_params *params)
{
	long (*cleanfunc)(struct lfs *, int, SEGUSE *);
	
	fs->lfs_autoclean = *params;
		
	cleanfunc = NULL;
	switch (fs->lfs_autoclean.mode) {
	case LFS_CLEANMODE_NONE:
		cleanfunc = NULL;
		break;
		
	case LFS_CLEANMODE_GREEDY:
		cleanfunc = segselect_greedy;
		break;
		
	case LFS_CLEANMODE_CB:
		cleanfunc = segselect_cb_time;
		break;
		
	default:
		return EINVAL;
	}

	mutex_enter(&lfs_lock);
	while (cleanfunc == NULL &&
	       fs->lfs_autoclean_status != LFS_AUTOCLEAN_STATUS_OFF) {
		cv_wait(&fs->lfs_cleanquitcv, &lfs_lock);
	}
	if (fs->lfs_clean_selector == NULL && cleanfunc != NULL)
		if (++lfs_ncleaners == 1) {
			if (lfs_cleaner_daemon == NULL &&
			    kthread_create(PRI_BIO, 0, NULL,
					   lfs_cleanerd, NULL,
					   &lfs_cleaner_daemon,
					   "lfs_cleaner") != 0)
				panic("fork lfs_cleaner");
		}
	if (fs->lfs_clean_selector != NULL && cleanfunc == NULL) {
		if (--lfs_ncleaners == 0) {
#if 0
			kthread_join(lfs_cleaner_daemon);
			lfs_cleaner_daemon = NULL;
#endif /* 0 */
		}
	}
	fs->lfs_clean_selector = cleanfunc;
	mutex_exit(&lfs_lock);

	return 0;
}
