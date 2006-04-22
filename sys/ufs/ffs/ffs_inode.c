/*	$NetBSD: ffs_inode.c,v 1.79.6.3 2006/04/22 11:40:24 simonb Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)ffs_inode.c	8.13 (Berkeley) 4/21/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_inode.c,v 1.79.6.3 2006/04/22 11:40:24 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/trace.h>
#include <sys/resourcevar.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

static int ffs_indirtrunc(struct inode *, daddr_t, daddr_t, daddr_t, int,
			  int64_t *);

/*
 * Update the access, modified, and inode change times as specified
 * by the IN_ACCESS, IN_UPDATE, and IN_CHANGE flags respectively.
 * The IN_MODIFIED flag is used to specify that the inode needs to be
 * updated but that the times have already been set. The access
 * and modified times are taken from the second and third parameters;
 * the inode change time is always taken from the current time. If
 * UPDATE_WAIT flag is set, or UPDATE_DIROP is set and we are not doing
 * softupdates, then wait for the disk write of the inode to complete.
 */

int
ffs_update(struct vnode *vp, const struct timespec *acc,
    const struct timespec *mod, int updflags)
{
	struct fs *fs;
	struct buf *bp;
	struct inode *ip;
	int error;
	caddr_t cp;
	int waitfor, flags;

	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return (0);
	ip = VTOI(vp);
	FFS_ITIMES(ip, acc, mod, NULL);
	if (updflags & UPDATE_CLOSE)
		flags = ip->i_flag & (IN_MODIFIED | IN_ACCESSED);
	else
		flags = ip->i_flag & IN_MODIFIED;
	if (flags == 0)
		return (0);
	fs = ip->i_fs;

	if ((flags & IN_MODIFIED) != 0 &&
	    (vp->v_mount->mnt_flag & MNT_ASYNC) == 0) {
		waitfor = updflags & UPDATE_WAIT;
		if ((updflags & UPDATE_DIROP) && !DOINGSOFTDEP(vp))
			waitfor |= UPDATE_WAIT;
	} else
		waitfor = 0;

	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
	if (fs->fs_magic == FS_UFS1_MAGIC &&			/* XXX */
	    fs->fs_old_inodefmt < FS_44INODEFMT) {		/* XXX */
		ip->i_ffs1_ouid = ip->i_uid;	/* XXX */
		ip->i_ffs1_ogid = ip->i_gid;	/* XXX */
	}							/* XXX */
	error = bread(ip->i_devvp,
		      fsbtodb(fs, ino_to_fsba(fs, ip->i_number)),
		      (int)fs->fs_bsize, NOCRED, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}
	ip->i_flag &= ~(IN_MODIFIED | IN_ACCESSED);
	if (DOINGSOFTDEP(vp))
		softdep_update_inodeblock(ip, bp, waitfor);
	else if (ip->i_ffs_effnlink != ip->i_nlink)
		panic("ffs_update: bad link cnt");
	if (fs->fs_magic == FS_UFS1_MAGIC) {
		cp = (caddr_t)bp->b_data +
		    (ino_to_fsbo(fs, ip->i_number) * DINODE1_SIZE);
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_dinode1_swap(ip->i_din.ffs1_din,
			    (struct ufs1_dinode *)cp);
		else
#endif
			memcpy(cp, ip->i_din.ffs1_din, DINODE1_SIZE);
	} else {
		cp = (caddr_t)bp->b_data +
		    (ino_to_fsbo(fs, ip->i_number) * DINODE2_SIZE);
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_dinode2_swap(ip->i_din.ffs2_din,
			    (struct ufs2_dinode *)cp);
		else
#endif
			memcpy(cp, ip->i_din.ffs2_din, DINODE2_SIZE);
	}
	if (waitfor) {
		return (bwrite(bp));
	} else {
		bdwrite(bp);
		return (0);
	}
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */
/*
 * Truncate the inode oip to at most length size, freeing the
 * disk blocks.
 */
int
ffs_truncate(struct vnode *ovp, off_t length, int ioflag, struct ucred *cred,
    struct lwp *l)
{
	struct genfs_node *gp = VTOG(ovp);
	daddr_t lastblock;
	struct inode *oip = VTOI(ovp);
	daddr_t bn, lastiblock[NIADDR], indir_lbn[NIADDR];
	daddr_t blks[NDADDR + NIADDR];
	struct fs *fs;
	int offset, size, level;
	int64_t count, blocksreleased = 0;
	int i, aflag, nblocks;
	int error, allerror = 0;
	off_t osize;
	int sync;
	struct ufsmount *ump = oip->i_ump;

	if (ovp->v_type == VCHR || ovp->v_type == VBLK ||
	    ovp->v_type == VFIFO || ovp->v_type == VSOCK) {
		KASSERT(oip->i_size == 0);
		return 0;
	}

	if (length < 0)
		return (EINVAL);

	if (ovp->v_type == VLNK &&
	    (oip->i_size < ump->um_maxsymlinklen ||
	     (ump->um_maxsymlinklen == 0 && DIP(oip, blocks) == 0))) {
		KDASSERT(length == 0);
		memset(SHORTLINK(oip), 0, (size_t)oip->i_size);
		oip->i_size = 0;
		DIP_ASSIGN(oip, size, 0);
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (ffs_update(ovp, NULL, NULL, 0));
	}
	if (oip->i_size == length) {
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (ffs_update(ovp, NULL, NULL, 0));
	}
#ifdef QUOTA
	if ((error = getinoquota(oip)) != 0)
		return (error);
#endif
	fs = oip->i_fs;
	if (length > ump->um_maxfilesize)
		return (EFBIG);

	if ((oip->i_flags & SF_SNAPSHOT) != 0)
		ffs_snapremove(ovp);

	osize = oip->i_size;
	aflag = ioflag & IO_SYNC ? B_SYNC : 0;

	/*
	 * Lengthen the size of the file. We must ensure that the
	 * last byte of the file is allocated. Since the smallest
	 * value of osize is 0, length will be at least 1.
	 */

	if (osize < length) {
		if (lblkno(fs, osize) < NDADDR &&
		    lblkno(fs, osize) != lblkno(fs, length) &&
		    blkroundup(fs, osize) != osize) {
			off_t eob;

			eob = blkroundup(fs, osize);
			error = ufs_balloc_range(ovp, osize, eob - osize,
			    cred, aflag);
			if (error)
				return error;
			if (ioflag & IO_SYNC) {
				ovp->v_size = eob;
				simple_lock(&ovp->v_interlock);
				VOP_PUTPAGES(ovp,
				    trunc_page(osize & fs->fs_bmask),
				    round_page(eob), PGO_CLEANIT | PGO_SYNCIO);
			}
		}
		error = ufs_balloc_range(ovp, length - 1, 1, cred, aflag);
		if (error) {
			(void) ffs_truncate(ovp, osize, ioflag & IO_SYNC,
			    cred, l);
			return (error);
		}
		uvm_vnp_setsize(ovp, length);
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		KASSERT(ovp->v_size == oip->i_size);
		return (ffs_update(ovp, NULL, NULL, 0));
	}

	/*
	 * When truncating a regular file down to a non-block-aligned size,
	 * we must zero the part of last block which is past the new EOF.
	 * We must synchronously flush the zeroed pages to disk
	 * since the new pages will be invalidated as soon as we
	 * inform the VM system of the new, smaller size.
	 * We must do this before acquiring the GLOCK, since fetching
	 * the pages will acquire the GLOCK internally.
	 * So there is a window where another thread could see a whole
	 * zeroed page past EOF, but that's life.
	 */

	offset = blkoff(fs, length);
	if (ovp->v_type == VREG && offset != 0 && osize > length) {
		daddr_t lbn;
		voff_t eoz;

		error = ufs_balloc_range(ovp, length - 1, 1, cred, aflag);
		if (error)
			return error;
		lbn = lblkno(fs, length);
		size = blksize(fs, oip, lbn);
		eoz = MIN(lblktosize(fs, lbn) + size, osize);
		uvm_vnp_zerorange(ovp, length, eoz - length);
		if (round_page(eoz) > round_page(length)) {
			simple_lock(&ovp->v_interlock);
			error = VOP_PUTPAGES(ovp, round_page(length),
			    round_page(eoz),
			    PGO_CLEANIT | PGO_DEACTIVATE |
			    ((ioflag & IO_SYNC) ? PGO_SYNCIO : 0));
			if (error)
				return error;
		}
	}

	lockmgr(&gp->g_glock, LK_EXCLUSIVE, NULL);

	if (DOINGSOFTDEP(ovp)) {
		if (length > 0) {
			/*
			 * If a file is only partially truncated, then
			 * we have to clean up the data structures
			 * describing the allocation past the truncation
			 * point. Finding and deallocating those structures
			 * is a lot of work. Since partial truncation occurs
			 * rarely, we solve the problem by syncing the file
			 * so that it will have no data structures left.
			 */
			if ((error = VOP_FSYNC(ovp, cred, FSYNC_WAIT,
			    0, 0, l)) != 0) {
				lockmgr(&gp->g_glock, LK_RELEASE, NULL);
				return (error);
			}
			if (oip->i_flag & IN_SPACECOUNTED)
				fs->fs_pendingblocks -= DIP(oip, blocks);
		} else {
			uvm_vnp_setsize(ovp, length);
#ifdef QUOTA
 			(void) chkdq(oip, -DIP(oip, blocks), NOCRED, 0);
#endif
			softdep_setup_freeblocks(oip, length, 0);
			(void) vinvalbuf(ovp, 0, cred, l, 0, 0);
			lockmgr(&gp->g_glock, LK_RELEASE, NULL);
			oip->i_flag |= IN_CHANGE | IN_UPDATE;
			return (ffs_update(ovp, NULL, NULL, 0));
		}
	}
	oip->i_size = length;
	DIP_ASSIGN(oip, size, length);
	uvm_vnp_setsize(ovp, length);
	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update file and block pointers on disk before we start freeing
	 * blocks.  If we crash before free'ing blocks below, the blocks
	 * will be returned to the free list.  lastiblock values are also
	 * normalized to -1 for calls to ffs_indirtrunc below.
	 */
	sync = 0;
	for (level = TRIPLE; level >= SINGLE; level--) {
		blks[NDADDR + level] = DIP(oip, ib[level]);
		if (lastiblock[level] < 0 && blks[NDADDR + level] != 0) {
			sync = 1;
			DIP_ASSIGN(oip, ib[level], 0);
			lastiblock[level] = -1;
		}
	}
	for (i = 0; i < NDADDR; i++) {
		blks[i] = DIP(oip, db[i]);
		if (i > lastblock && blks[i] != 0) {
			sync = 1;
			DIP_ASSIGN(oip, db[i], 0);
		}
	}
	oip->i_flag |= IN_CHANGE | IN_UPDATE;
	if (sync) {
		error = ffs_update(ovp, NULL, NULL, UPDATE_WAIT);
		if (error && !allerror)
			allerror = error;
	}

	/*
	 * Having written the new inode to disk, save its new configuration
	 * and put back the old block pointers long enough to process them.
	 * Note that we save the new block configuration so we can check it
	 * when we are done.
	 */
	for (i = 0; i < NDADDR; i++) {
		bn = DIP(oip, db[i]);
		DIP_ASSIGN(oip, db[i], blks[i]);
		blks[i] = bn;
	}
	for (i = 0; i < NIADDR; i++) {
		bn = DIP(oip, ib[i]);
		DIP_ASSIGN(oip, ib[i], blks[NDADDR + i]);
		blks[NDADDR + i] = bn;
	}

	oip->i_size = osize;
	DIP_ASSIGN(oip, size, osize);
	error = vtruncbuf(ovp, lastblock + 1, 0, 0);
	if (error && !allerror)
		allerror = error;

	/*
	 * Indirect blocks first.
	 */
	indir_lbn[SINGLE] = -NDADDR;
	indir_lbn[DOUBLE] = indir_lbn[SINGLE] - NINDIR(fs) - 1;
	indir_lbn[TRIPLE] = indir_lbn[DOUBLE] - NINDIR(fs) * NINDIR(fs) - 1;
	for (level = TRIPLE; level >= SINGLE; level--) {
		if (oip->i_ump->um_fstype == UFS1)
			bn = ufs_rw32(oip->i_ffs1_ib[level],UFS_FSNEEDSWAP(fs));
		else
			bn = ufs_rw64(oip->i_ffs2_ib[level],UFS_FSNEEDSWAP(fs));
		if (bn != 0) {
			error = ffs_indirtrunc(oip, indir_lbn[level],
			    fsbtodb(fs, bn), lastiblock[level], level, &count);
			if (error)
				allerror = error;
			blocksreleased += count;
			if (lastiblock[level] < 0) {
				DIP_ASSIGN(oip, ib[level], 0);
				ffs_blkfree(fs, oip->i_devvp, bn, fs->fs_bsize,
				    oip->i_number);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		long bsize;

		if (oip->i_ump->um_fstype == UFS1)
			bn = ufs_rw32(oip->i_ffs1_db[i], UFS_FSNEEDSWAP(fs));
		else
			bn = ufs_rw64(oip->i_ffs2_db[i], UFS_FSNEEDSWAP(fs));
		if (bn == 0)
			continue;
		DIP_ASSIGN(oip, db[i], 0);
		bsize = blksize(fs, oip, i);
		ffs_blkfree(fs, oip->i_devvp, bn, bsize, oip->i_number);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	if (oip->i_ump->um_fstype == UFS1)
		bn = ufs_rw32(oip->i_ffs1_db[lastblock], UFS_FSNEEDSWAP(fs));
	else
		bn = ufs_rw64(oip->i_ffs2_db[lastblock], UFS_FSNEEDSWAP(fs));
	if (bn != 0) {
		long oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, oip, lastblock);
		oip->i_size = length;
		DIP_ASSIGN(oip, size, length);
		newspace = blksize(fs, oip, lastblock);
		if (newspace == 0)
			panic("itrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			ffs_blkfree(fs, oip->i_devvp, bn, oldspace - newspace,
			    oip->i_number);
			blocksreleased += btodb(oldspace - newspace);
		}
	}

done:
#ifdef DIAGNOSTIC
	for (level = SINGLE; level <= TRIPLE; level++)
		if (blks[NDADDR + level] != DIP(oip, ib[level]))
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (blks[i] != DIP(oip, db[i]))
			panic("itrunc2");
	if (length == 0 &&
	    (!LIST_EMPTY(&ovp->v_cleanblkhd) || !LIST_EMPTY(&ovp->v_dirtyblkhd)))
		panic("itrunc3");
#endif /* DIAGNOSTIC */
	/*
	 * Put back the real size.
	 */
	oip->i_size = length;
	DIP_ASSIGN(oip, size, length);
	DIP_ADD(oip, blocks, -blocksreleased);
	lockmgr(&gp->g_glock, LK_RELEASE, NULL);
	oip->i_flag |= IN_CHANGE;
#ifdef QUOTA
	(void) chkdq(oip, -blocksreleased, NOCRED, 0);
#endif
	KASSERT(ovp->v_type != VREG || ovp->v_size == oip->i_size);
	return (allerror);
}

/*
 * Release blocks associated with the inode ip and stored in the indirect
 * block bn.  Blocks are free'd in LIFO order up to (but not including)
 * lastbn.  If level is greater than SINGLE, the block is an indirect block
 * and recursive calls to indirtrunc must be used to cleanse other indirect
 * blocks.
 *
 * NB: triple indirect blocks are untested.
 */
static int
ffs_indirtrunc(struct inode *ip, daddr_t lbn, daddr_t dbn, daddr_t lastbn,
    int level, int64_t *countp)
{
	int i;
	struct buf *bp;
	struct fs *fs = ip->i_fs;
	int32_t *bap1 = NULL;
	int64_t *bap2 = NULL;
	struct vnode *vp;
	daddr_t nb, nlbn, last;
	char *copy = NULL;
	int64_t blkcount, factor, blocksreleased = 0;
	int nblocks;
	int error = 0, allerror = 0;
#ifdef FFS_EI
	const int needswap = UFS_FSNEEDSWAP(fs);
#endif
#define RBAP(ip, i) (((ip)->i_ump->um_fstype == UFS1) ? \
	    ufs_rw32(bap1[i], needswap) : ufs_rw64(bap2[i], needswap))
#define BAP_ASSIGN(ip, i, value)					\
	do {								\
		if ((ip)->i_ump->um_fstype == UFS1)			\
			bap1[i] = (value);				\
		else							\
			bap2[i] = (value);				\
	} while(0)

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those entries corresponding
	 * to blocks to be free'd, and update on disk copy first.  Since
	 * double(triple) indirect before single(double) indirect, calls
	 * to bmap on these blocks will fail.  However, we already have
	 * the on disk address, so we have to set the b_blkno field
	 * explicitly instead of letting bread do everything for us.
	 */
	vp = ITOV(ip);
	bp = getblk(vp, lbn, (int)fs->fs_bsize, 0, 0);
	if (bp->b_flags & (B_DONE | B_DELWRI)) {
		/* Braces must be here in case trace evaluates to nothing. */
		trace(TR_BREADHIT, pack(vp, fs->fs_bsize), lbn);
	} else {
		trace(TR_BREADMISS, pack(vp, fs->fs_bsize), lbn);
		curproc->p_stats->p_ru.ru_inblock++;	/* pay for read */
		bp->b_flags |= B_READ;
		if (bp->b_bcount > bp->b_bufsize)
			panic("ffs_indirtrunc: bad buffer size");
		bp->b_blkno = dbn;
		BIO_SETPRIO(bp, BPRIO_TIMECRITICAL);
		VOP_STRATEGY(vp, bp);
		error = biowait(bp);
	}
	if (error) {
		brelse(bp);
		*countp = 0;
		return (error);
	}

	if (ip->i_ump->um_fstype == UFS1)
		bap1 = (int32_t *)bp->b_data;
	else
		bap2 = (int64_t *)bp->b_data;
	if (lastbn >= 0) {
		copy = malloc(fs->fs_bsize, M_TEMP, M_WAITOK);
		memcpy((caddr_t)copy, bp->b_data, (u_int)fs->fs_bsize);
		for (i = last + 1; i < NINDIR(fs); i++)
			BAP_ASSIGN(ip, i, 0);
		error = bwrite(bp);
		if (error)
			allerror = error;
		if (ip->i_ump->um_fstype == UFS1)
			bap1 = (int32_t *)copy;
		else
			bap2 = (int64_t *)copy;
	}

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1, nlbn = lbn + 1 - i * factor; i > last;
	    i--, nlbn += factor) {
		nb = RBAP(ip, i);
		if (nb == 0)
			continue;
		if (level > SINGLE) {
			error = ffs_indirtrunc(ip, nlbn, fsbtodb(fs, nb),
					       (daddr_t)-1, level - 1,
					       &blkcount);
			if (error)
				allerror = error;
			blocksreleased += blkcount;
		}
		ffs_blkfree(fs, ip->i_devvp, nb, fs->fs_bsize, ip->i_number);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = RBAP(ip, i);
		if (nb != 0) {
			error = ffs_indirtrunc(ip, nlbn, fsbtodb(fs, nb),
					       last, level - 1, &blkcount);
			if (error)
				allerror = error;
			blocksreleased += blkcount;
		}
	}

	if (copy != NULL) {
		FREE(copy, M_TEMP);
	} else {
		bp->b_flags |= B_INVAL;
		brelse(bp);
	}

	*countp = blocksreleased;
	return (allerror);
}

void
ffs_itimes(struct inode *ip, const struct timespec *acc,
    const struct timespec *mod, const struct timespec *cre)
{
	struct timespec now;

	if (!(ip->i_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFY))) {
		return;
	}

	getnanotime(&now);
	if (ip->i_flag & IN_ACCESS) {
		if (acc == NULL)
			acc = &now;
		DIP_ASSIGN(ip, atime, acc->tv_sec);
		DIP_ASSIGN(ip, atimensec, acc->tv_nsec);
	}
	if (ip->i_flag & (IN_UPDATE | IN_MODIFY)) {
		if ((ip->i_flags & SF_SNAPSHOT) == 0) {
			if (mod == NULL)
				mod = &now;
			DIP_ASSIGN(ip, mtime, mod->tv_sec);
			DIP_ASSIGN(ip, mtimensec, mod->tv_nsec);
		}
		ip->i_modrev++;
	}
	if (ip->i_flag & (IN_CHANGE | IN_MODIFY)) {
		if (cre == NULL)
			cre = &now;
		DIP_ASSIGN(ip, ctime, cre->tv_sec);
		DIP_ASSIGN(ip, ctimensec, cre->tv_nsec);
	}
	if (ip->i_flag & (IN_ACCESS | IN_MODIFY))
		ip->i_flag |= IN_ACCESSED;
	if (ip->i_flag & (IN_UPDATE | IN_CHANGE))
		ip->i_flag |= IN_MODIFIED;
	ip->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFY);
}
