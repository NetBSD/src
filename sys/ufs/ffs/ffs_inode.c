/*	$NetBSD: ffs_inode.c,v 1.35.2.3 2002/02/26 21:13:18 he Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#if defined(_KERNEL) && !defined(_LKM)
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

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

static int ffs_indirtrunc __P((struct inode *, ufs_daddr_t, ufs_daddr_t,
			       ufs_daddr_t, int, long *));

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
ffs_update(v)
	void *v;
{
	struct vop_update_args /* {
		struct vnode *a_vp;
		struct timespec *a_access;
		struct timespec *a_modify;
		int a_flags;
	} */ *ap = v;
	struct fs *fs;
	struct buf *bp;
	struct inode *ip;
	int error;
	struct timespec ts;
	caddr_t cp;
	int waitfor, flags;

	if (ap->a_vp->v_mount->mnt_flag & MNT_RDONLY)
		return (0);
	ip = VTOI(ap->a_vp);
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	FFS_ITIMES(ip,
	    ap->a_access ? ap->a_access : &ts,
	    ap->a_modify ? ap->a_modify : &ts, &ts);
	flags = ip->i_flag & (IN_MODIFIED | IN_ACCESSED);
	if (flags == 0)
		return (0);
	fs = ip->i_fs;

	if ((flags & IN_MODIFIED) != 0 &&
	    (ap->a_vp->v_mount->mnt_flag & MNT_ASYNC) == 0) {
		waitfor = ap->a_flags & UPDATE_WAIT;
		if ((ap->a_flags & UPDATE_DIROP) && !DOINGSOFTDEP(ap->a_vp))
			waitfor |= UPDATE_WAIT;
	} else
		waitfor = 0;

	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		ip->i_din.ffs_din.di_ouid = ip->i_ffs_uid;	/* XXX */
		ip->i_din.ffs_din.di_ogid = ip->i_ffs_gid;	/* XXX */
	}							/* XXX */
	error = bread(ip->i_devvp,
		      fsbtodb(fs, ino_to_fsba(fs, ip->i_number)),
		      (int)fs->fs_bsize, NOCRED, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}
	ip->i_flag &= ~(IN_MODIFIED | IN_ACCESSED);
	if (DOINGSOFTDEP(ap->a_vp))
		softdep_update_inodeblock(ip, bp, waitfor);
	else if (ip->i_ffs_effnlink != ip->i_ffs_nlink)
		panic("ffs_update: bad link cnt");
	cp = (caddr_t)bp->b_data +
	    (ino_to_fsbo(fs, ip->i_number) * DINODE_SIZE);
#ifdef FFS_EI
	if (UFS_FSNEEDSWAP(fs))
		ffs_dinode_swap(&ip->i_din.ffs_din, (struct dinode *)cp);
	else
#endif
		memcpy(cp, &ip->i_din.ffs_din, DINODE_SIZE);
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
ffs_truncate(v)
	void *v;
{
	struct vop_truncate_args /* {
		struct vnode *a_vp;
		off_t a_length;
		int a_flags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *ovp = ap->a_vp;
	ufs_daddr_t lastblock;
	struct inode *oip;
	ufs_daddr_t bn, lbn, lastiblock[NIADDR], indir_lbn[NIADDR];
	ufs_daddr_t oldblks[NDADDR + NIADDR], newblks[NDADDR + NIADDR];
	off_t length = ap->a_length;
	struct fs *fs;
	struct buf *bp;
	int offset, size, level;
	long count, nblocks, blocksreleased = 0;
	int i;
	int aflags, error, allerror = 0;
	off_t osize;

	if (length < 0)
		return (EINVAL);
	oip = VTOI(ovp);
#if 1
	/*
	 * XXX. Was in Kirk's patches. Is it good behavior to just
	 * return and not update modification times?
	 */
	if (oip->i_ffs_size == length)
		return (0);
#endif
	if (ovp->v_type == VLNK &&
	    (oip->i_ffs_size < ovp->v_mount->mnt_maxsymlinklen ||
	     (ovp->v_mount->mnt_maxsymlinklen == 0 &&
	      oip->i_din.ffs_din.di_blocks == 0))) {
#ifdef DIAGNOSTIC
		if (length != 0)
			panic("ffs_truncate: partial truncate of symlink");
#endif
		memset((char *)&oip->i_ffs_shortlink, 0, (u_int)oip->i_ffs_size);
		oip->i_ffs_size = 0;
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(ovp, NULL, NULL, UPDATE_WAIT));
	}
	if (oip->i_ffs_size == length) {
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(ovp, NULL, NULL, 0));
	}
#ifdef QUOTA
	if ((error = getinoquota(oip)) != 0)
		return (error);
#endif
	fs = oip->i_fs;
	osize = oip->i_ffs_size;
	ovp->v_lasta = ovp->v_clen = ovp->v_cstart = ovp->v_lastw = 0;

	if (DOINGSOFTDEP(ovp)) {
		uvm_vnp_setsize(ovp, length);
		if (ovp->v_usecount)	/* can't be cached if usecount=0 */
			(void) uvm_vnp_uncache(ovp);
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
			if ((error = VOP_FSYNC(ovp, ap->a_cred, FSYNC_WAIT,
			    0, 0, ap->a_p)) != 0)
				return (error);
		} else {
#ifdef QUOTA
 			(void) chkdq(oip, -oip->i_ffs_blocks, NOCRED, 0);
#endif
			softdep_setup_freeblocks(oip, length);
			(void) vinvalbuf(ovp, 0, ap->a_cred, ap->a_p, 0, 0);
			oip->i_flag |= IN_CHANGE | IN_UPDATE;
			return (VOP_UPDATE(ovp, NULL, NULL, 0));
		}
	}
	/*
	 * Lengthen the size of the file. We must ensure that the
	 * last byte of the file is allocated. Since the smallest
	 * value of osize is 0, length will be at least 1.
	 */
	if (osize < length) {
		if (length > fs->fs_maxfilesize)
			return (EFBIG);
		aflags = B_CLRBUF;
		if (ap->a_flags & IO_SYNC)
			aflags |= B_SYNC;
		error = VOP_BALLOC(ovp, length - 1, 1, ap->a_cred, aflags, &bp);
		if (error)
			return (error);
		oip->i_ffs_size = length;
		uvm_vnp_setsize(ovp, length);
		(void) uvm_vnp_uncache(ovp);
		if (aflags & B_SYNC)
			bwrite(bp);
		else
			bawrite(bp);
		oip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(ovp, NULL, NULL, UPDATE_WAIT));
	}
	/*
	 * Shorten the size of the file. If the file is not being
	 * truncated to a block boundary, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever becomes accessible again because
	 * of subsequent file growth. Directories however are not
	 * zero'ed as they should grow back initialized to empty.
	 */
	offset = blkoff(fs, length);
	if (offset == 0) {
		oip->i_ffs_size = length;
	} else {
		lbn = lblkno(fs, length);
		aflags = B_CLRBUF;
		if (ap->a_flags & IO_SYNC)
			aflags |= B_SYNC;
		error = VOP_BALLOC(ovp, length - 1, 1, ap->a_cred, aflags, &bp);
		if (error)
			return (error);
		/*
		 * When we are doing soft updates and the VOP_BALLOC
		 * above fills in a direct block hole with a full sized
		 * block that will be truncated down to a fragment below,
		 * we must flush out the block dependency with an FSYNC
		 * so that we do not get a soft updates inconsistency
		 * when we create the fragment below.
		 */
		if (DOINGSOFTDEP(ovp) && lbn < NDADDR &&
		    fragroundup(fs, blkoff(fs, length)) < fs->fs_bsize) {
			error = VOP_FSYNC(ovp, ap->a_cred, FSYNC_WAIT, 0, 0,
			    ap->a_p);
			if (error != 0)
				return error;
		}

		oip->i_ffs_size = length;
		size = blksize(fs, oip, lbn);
		(void) uvm_vnp_uncache(ovp);
		if (ovp->v_type != VDIR)
			memset((char *)bp->b_data + offset, 0,
			       (u_int)(size - offset));
		allocbuf(bp, size);
		if (aflags & B_SYNC)
			bwrite(bp);
		else
			bawrite(bp);
	}
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
	memcpy((caddr_t)oldblks, (caddr_t)&oip->i_ffs_db[0], sizeof oldblks);
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ffs_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_ffs_db[i] = 0;
	oip->i_flag |= IN_CHANGE | IN_UPDATE;
	error = VOP_UPDATE(ovp, NULL, NULL, UPDATE_WAIT);
	if (error && !allerror)
		allerror = error;

	/*
	 * Having written the new inode to disk, save its new configuration
	 * and put back the old block pointers long enough to process them.
	 * Note that we save the new block configuration so we can check it
	 * when we are done.
	 */
	memcpy((caddr_t)newblks, (caddr_t)&oip->i_ffs_db[0], sizeof newblks);
	memcpy((caddr_t)&oip->i_ffs_db[0], (caddr_t)oldblks, sizeof oldblks);
	oip->i_ffs_size = osize;
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
		bn = ufs_rw32(oip->i_ffs_ib[level], UFS_FSNEEDSWAP(fs));
		if (bn != 0) {
			error = ffs_indirtrunc(oip, indir_lbn[level],
			    fsbtodb(fs, bn), lastiblock[level], level, &count);
			if (error)
				allerror = error;
			blocksreleased += count;
			if (lastiblock[level] < 0) {
				oip->i_ffs_ib[level] = 0;
				ffs_blkfree(oip, bn, fs->fs_bsize);
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

		bn = ufs_rw32(oip->i_ffs_db[i], UFS_FSNEEDSWAP(fs));
		if (bn == 0)
			continue;
		oip->i_ffs_db[i] = 0;
		bsize = blksize(fs, oip, i);
		ffs_blkfree(oip, bn, bsize);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ufs_rw32(oip->i_ffs_db[lastblock], UFS_FSNEEDSWAP(fs));
	if (bn != 0) {
		long oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, oip, lastblock);
		oip->i_ffs_size = length;
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
			ffs_blkfree(oip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}

done:
#ifdef DIAGNOSTIC
	for (level = SINGLE; level <= TRIPLE; level++)
		if (newblks[NDADDR + level] != oip->i_ffs_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (newblks[i] != oip->i_ffs_db[i])
			panic("itrunc2");
	if (length == 0 &&
	    (!LIST_EMPTY(&ovp->v_cleanblkhd) || !LIST_EMPTY(&ovp->v_dirtyblkhd)))
		panic("itrunc3");
#endif /* DIAGNOSTIC */
	/*
	 * Put back the real size.
	 */
	oip->i_ffs_size = length;
	oip->i_ffs_blocks -= blocksreleased;
	if (oip->i_ffs_blocks < 0)			/* sanity */
		oip->i_ffs_blocks = 0;
	oip->i_flag |= IN_CHANGE;
#ifdef QUOTA
	(void) chkdq(oip, -blocksreleased, NOCRED, 0);
#endif
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
ffs_indirtrunc(ip, lbn, dbn, lastbn, level, countp)
	struct inode *ip;
	ufs_daddr_t lbn, lastbn;
	ufs_daddr_t dbn;
	int level;
	long *countp;
{
	int i;
	struct buf *bp;
	struct fs *fs = ip->i_fs;
	ufs_daddr_t *bap;
	struct vnode *vp;
	ufs_daddr_t *copy = NULL, nb, nlbn, last;
	long blkcount, factor;
	int nblocks, blocksreleased = 0;
	int error = 0, allerror = 0;

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
		VOP_STRATEGY(bp);
		error = biowait(bp);
	}
	if (error) {
		brelse(bp);
		*countp = 0;
		return (error);
	}

	bap = (ufs_daddr_t *)bp->b_data;
	if (lastbn >= 0) {
		MALLOC(copy, ufs_daddr_t *, fs->fs_bsize, M_TEMP, M_WAITOK);
		memcpy((caddr_t)copy, (caddr_t)bap, (u_int)fs->fs_bsize);
		memset((caddr_t)&bap[last + 1], 0,
		  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (ufs_daddr_t));
		error = bwrite(bp);
		if (error)
			allerror = error;
		bap = copy;
	}

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1, nlbn = lbn + 1 - i * factor; i > last;
	    i--, nlbn += factor) {
		nb = ufs_rw32(bap[i], UFS_FSNEEDSWAP(fs));
		if (nb == 0)
			continue;
		if (level > SINGLE) {
			error = ffs_indirtrunc(ip, nlbn, fsbtodb(fs, nb),
					       (ufs_daddr_t)-1, level - 1,
					       &blkcount);
			if (error)
				allerror = error;
			blocksreleased += blkcount;
		}
		ffs_blkfree(ip, nb, fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = ufs_rw32(bap[i], UFS_FSNEEDSWAP(fs));
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
