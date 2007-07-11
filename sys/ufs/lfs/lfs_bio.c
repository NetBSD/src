/*	$NetBSD: lfs_bio.c,v 1.98.10.1 2007/07/11 20:12:44 mjf Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
 *	@(#)lfs_bio.c	8.10 (Berkeley) 6/10/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_bio.c,v 1.98.10.1 2007/07/11 20:12:44 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/kauth.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <uvm/uvm.h>

/*
 * LFS block write function.
 *
 * XXX
 * No write cost accounting is done.
 * This is almost certainly wrong for synchronous operations and NFS.
 *
 * protected by lfs_subsys_lock.
 */
int	locked_queue_count   = 0;	/* Count of locked-down buffers. */
long	locked_queue_bytes   = 0L;	/* Total size of locked buffers. */
int	lfs_subsys_pages     = 0L;	/* Total number LFS-written pages */
int	lfs_fs_pagetrip	     = 0;	/* # of pages to trip per-fs write */
int	lfs_writing	     = 0;	/* Set if already kicked off a writer
					   because of buffer space */
/* Lock for aboves */
struct simplelock lfs_subsys_lock = SIMPLELOCK_INITIALIZER;

extern int lfs_dostats;

/*
 * reserved number/bytes of locked buffers
 */
int locked_queue_rcount = 0;
long locked_queue_rbytes = 0L;

int lfs_fits_buf(struct lfs *, int, int);
int lfs_reservebuf(struct lfs *, struct vnode *vp, struct vnode *vp2,
    int, int);
int lfs_reserveavail(struct lfs *, struct vnode *vp, struct vnode *vp2, int);

int
lfs_fits_buf(struct lfs *fs, int n, int bytes)
{
	int count_fit, bytes_fit;

	ASSERT_NO_SEGLOCK(fs);
	LOCK_ASSERT(simple_lock_held(&lfs_subsys_lock));

	count_fit =
	    (locked_queue_count + locked_queue_rcount + n < LFS_WAIT_BUFS);
	bytes_fit =
	    (locked_queue_bytes + locked_queue_rbytes + bytes < LFS_WAIT_BYTES);

#ifdef DEBUG
	if (!count_fit) {
		DLOG((DLOG_AVAIL, "lfs_fits_buf: no fit count: %d + %d + %d >= %d\n",
		      locked_queue_count, locked_queue_rcount,
		      n, LFS_WAIT_BUFS));
	}
	if (!bytes_fit) {
		DLOG((DLOG_AVAIL, "lfs_fits_buf: no fit bytes: %ld + %ld + %d >= %ld\n",
		      locked_queue_bytes, locked_queue_rbytes,
		      bytes, LFS_WAIT_BYTES));
	}
#endif /* DEBUG */

	return (count_fit && bytes_fit);
}

/* ARGSUSED */
int
lfs_reservebuf(struct lfs *fs, struct vnode *vp,
    struct vnode *vp2, int n, int bytes)
{
	ASSERT_MAYBE_SEGLOCK(fs);
	KASSERT(locked_queue_rcount >= 0);
	KASSERT(locked_queue_rbytes >= 0);

	simple_lock(&lfs_subsys_lock);
	while (n > 0 && !lfs_fits_buf(fs, n, bytes)) {
		int error;

		lfs_flush(fs, 0, 0);

		error = ltsleep(&locked_queue_count, PCATCH | PUSER,
		    "lfsresbuf", hz * LFS_BUFWAIT, &lfs_subsys_lock);
		if (error && error != EWOULDBLOCK) {
			simple_unlock(&lfs_subsys_lock);
			return error;
		}
	}

	locked_queue_rcount += n;
	locked_queue_rbytes += bytes;

	simple_unlock(&lfs_subsys_lock);

	KASSERT(locked_queue_rcount >= 0);
	KASSERT(locked_queue_rbytes >= 0);

	return 0;
}

/*
 * Try to reserve some blocks, prior to performing a sensitive operation that
 * requires the vnode lock to be honored.  If there is not enough space, give
 * up the vnode lock temporarily and wait for the space to become available.
 *
 * Called with vp locked.  (Note nowever that if fsb < 0, vp is ignored.)
 *
 * XXX YAMT - it isn't safe to unlock vp here
 * because the node might be modified while we sleep.
 * (eg. cached states like i_offset might be stale,
 *  the vnode might be truncated, etc..)
 * maybe we should have a way to restart the vnodeop (EVOPRESTART?)
 * or rearrange vnodeop interface to leave vnode locking to file system
 * specific code so that each file systems can have their own vnode locking and
 * vnode re-using strategies.
 */
int
lfs_reserveavail(struct lfs *fs, struct vnode *vp,
    struct vnode *vp2, int fsb)
{
	CLEANERINFO *cip;
	struct buf *bp;
	int error, slept;

	ASSERT_MAYBE_SEGLOCK(fs);
	slept = 0;
	simple_lock(&fs->lfs_interlock);
	while (fsb > 0 && !lfs_fits(fs, fsb + fs->lfs_ravail + fs->lfs_favail)) {
		simple_unlock(&fs->lfs_interlock);
#if 0
		/*
		 * XXX ideally, we should unlock vnodes here
		 * because we might sleep very long time.
		 */
		VOP_UNLOCK(vp, 0);
		if (vp2 != NULL) {
			VOP_UNLOCK(vp2, 0);
		}
#else
		/*
		 * XXX since we'll sleep for cleaner with vnode lock holding,
		 * deadlock will occur if cleaner tries to lock the vnode.
		 * (eg. lfs_markv -> lfs_fastvget -> getnewvnode -> vclean)
		 */
#endif

		if (!slept) {
			DLOG((DLOG_AVAIL, "lfs_reserve: waiting for %ld (bfree = %d,"
			      " est_bfree = %d)\n",
			      fsb + fs->lfs_ravail + fs->lfs_favail,
			      fs->lfs_bfree, LFS_EST_BFREE(fs)));
		}
		++slept;

		/* Wake up the cleaner */
		LFS_CLEANERINFO(cip, fs, bp);
		LFS_SYNC_CLEANERINFO(cip, fs, bp, 0);
		lfs_wakeup_cleaner(fs);

		simple_lock(&fs->lfs_interlock);
		/* Cleaner might have run while we were reading, check again */
		if (lfs_fits(fs, fsb + fs->lfs_ravail + fs->lfs_favail))
			break;

		error = ltsleep(&fs->lfs_avail, PCATCH | PUSER, "lfs_reserve",
				0, &fs->lfs_interlock);
#if 0
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY); /* XXX use lockstatus */
		vn_lock(vp2, LK_EXCLUSIVE | LK_RETRY); /* XXX use lockstatus */
#endif
		if (error) {
			simple_unlock(&fs->lfs_interlock);
			return error;
		}
	}
#ifdef DEBUG
	if (slept) {
		DLOG((DLOG_AVAIL, "lfs_reserve: woke up\n"));
	}
#endif
	fs->lfs_ravail += fsb;
	simple_unlock(&fs->lfs_interlock);

	return 0;
}

#ifdef DIAGNOSTIC
int lfs_rescount;
int lfs_rescountdirop;
#endif

int
lfs_reserve(struct lfs *fs, struct vnode *vp, struct vnode *vp2, int fsb)
{
	int error;
	int cantwait;

	ASSERT_MAYBE_SEGLOCK(fs);
	if (vp2) {
		/* Make sure we're not in the process of reclaiming vp2 */
		simple_lock(&fs->lfs_interlock);
		while(fs->lfs_flags & LFS_UNDIROP) {
			ltsleep(&fs->lfs_flags, PRIBIO + 1, "lfsrundirop", 0,
			    &fs->lfs_interlock);
		}
		simple_unlock(&fs->lfs_interlock);
	}

	KASSERT(fsb < 0 || VOP_ISLOCKED(vp));
	KASSERT(vp2 == NULL || fsb < 0 || VOP_ISLOCKED(vp2));
	KASSERT(vp2 == NULL || !(VTOI(vp2)->i_flag & IN_ADIROP));
	KASSERT(vp2 == NULL || vp2 != fs->lfs_unlockvp);

	cantwait = (VTOI(vp)->i_flag & IN_ADIROP) || fs->lfs_unlockvp == vp;
#ifdef DIAGNOSTIC
	if (cantwait) {
		if (fsb > 0)
			lfs_rescountdirop++;
		else if (fsb < 0)
			lfs_rescountdirop--;
		if (lfs_rescountdirop < 0)
			panic("lfs_rescountdirop");
	}
	else {
		if (fsb > 0)
			lfs_rescount++;
		else if (fsb < 0)
			lfs_rescount--;
		if (lfs_rescount < 0)
			panic("lfs_rescount");
	}
#endif
	if (cantwait)
		return 0;

	/*
	 * XXX
	 * vref vnodes here so that cleaner doesn't try to reuse them.
	 * (see XXX comment in lfs_reserveavail)
	 */
	lfs_vref(vp);
	if (vp2 != NULL) {
		lfs_vref(vp2);
	}

	error = lfs_reserveavail(fs, vp, vp2, fsb);
	if (error)
		goto done;

	/*
	 * XXX just a guess. should be more precise.
	 */
	error = lfs_reservebuf(fs, vp, vp2,
	    fragstoblks(fs, fsb), fsbtob(fs, fsb));
	if (error)
		lfs_reserveavail(fs, vp, vp2, -fsb);

done:
	lfs_vunref(vp);
	if (vp2 != NULL) {
		lfs_vunref(vp2);
	}

	return error;
}

int
lfs_bwrite(void *v)
{
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	struct buf *bp = ap->a_bp;

#ifdef DIAGNOSTIC
	if (VTOI(bp->b_vp)->i_lfs->lfs_ronly == 0 && (bp->b_flags & B_ASYNC)) {
		panic("bawrite LFS buffer");
	}
#endif /* DIAGNOSTIC */
	return lfs_bwrite_ext(bp, 0);
}

/*
 * Determine if there is enough room currently available to write fsb
 * blocks.  We need enough blocks for the new blocks, the current
 * inode blocks (including potentially the ifile inode), a summary block,
 * and the segment usage table, plus an ifile block.
 */
int
lfs_fits(struct lfs *fs, int fsb)
{
	int needed;

	ASSERT_NO_SEGLOCK(fs);
	needed = fsb + btofsb(fs, fs->lfs_sumsize) +
		 ((howmany(fs->lfs_uinodes + 1, INOPB(fs)) + fs->lfs_segtabsz +
		   1) << (fs->lfs_blktodb - fs->lfs_fsbtodb));

	if (needed >= fs->lfs_avail) {
#ifdef DEBUG
		DLOG((DLOG_AVAIL, "lfs_fits: no fit: fsb = %ld, uinodes = %ld, "
		      "needed = %ld, avail = %ld\n",
		      (long)fsb, (long)fs->lfs_uinodes, (long)needed,
		      (long)fs->lfs_avail));
#endif
		return 0;
	}
	return 1;
}

int
lfs_availwait(struct lfs *fs, int fsb)
{
	int error;
	CLEANERINFO *cip;
	struct buf *cbp;

	ASSERT_NO_SEGLOCK(fs);
	/* Push cleaner blocks through regardless */
	simple_lock(&fs->lfs_interlock);
	if (LFS_SEGLOCK_HELD(fs) &&
	    fs->lfs_sp->seg_flags & (SEGM_CLEAN | SEGM_FORCE_CKP)) {
		simple_unlock(&fs->lfs_interlock);
		return 0;
	}
	simple_unlock(&fs->lfs_interlock);

	while (!lfs_fits(fs, fsb)) {
		/*
		 * Out of space, need cleaner to run.
		 * Update the cleaner info, then wake it up.
		 * Note the cleanerinfo block is on the ifile
		 * so it CANT_WAIT.
		 */
		LFS_CLEANERINFO(cip, fs, cbp);
		LFS_SYNC_CLEANERINFO(cip, fs, cbp, 0);

#ifdef DEBUG
		DLOG((DLOG_AVAIL, "lfs_availwait: out of available space, "
		      "waiting on cleaner\n"));
#endif

		lfs_wakeup_cleaner(fs);
#ifdef DIAGNOSTIC
		if (LFS_SEGLOCK_HELD(fs))
			panic("lfs_availwait: deadlock");
#endif
		error = tsleep(&fs->lfs_avail, PCATCH | PUSER, "cleaner", 0);
		if (error)
			return (error);
	}
	return 0;
}

int
lfs_bwrite_ext(struct buf *bp, int flags)
{
	struct lfs *fs;
	struct inode *ip;
	int fsb, s;

	fs = VFSTOUFS(bp->b_vp->v_mount)->um_lfs;

	ASSERT_MAYBE_SEGLOCK(fs);
	KASSERT(bp->b_flags & B_BUSY);
	KASSERT(flags & BW_CLEAN || !LFS_IS_MALLOC_BUF(bp));
	KASSERT((bp->b_flags & (B_DELWRI|B_LOCKED)) != B_DELWRI);
	KASSERT((bp->b_flags & (B_DELWRI|B_LOCKED)) != B_LOCKED);

	/*
	 * Don't write *any* blocks if we're mounted read-only, or
	 * if we are "already unmounted".
	 *
	 * In particular the cleaner can't write blocks either.
	 */
	if (fs->lfs_ronly || (fs->lfs_pflags & LFS_PF_CLEAN)) {
		bp->b_flags &= ~(B_DELWRI | B_READ | B_ERROR);
		LFS_UNLOCK_BUF(bp);
		if (LFS_IS_MALLOC_BUF(bp))
			bp->b_flags &= ~B_BUSY;
		else
			brelse(bp);
		return (fs->lfs_ronly ? EROFS : 0);
	}

	/*
	 * Set the delayed write flag and use reassignbuf to move the buffer
	 * from the clean list to the dirty one.
	 *
	 * Set the B_LOCKED flag and unlock the buffer, causing brelse to move
	 * the buffer onto the LOCKED free list.  This is necessary, otherwise
	 * getnewbuf() would try to reclaim the buffers using bawrite, which
	 * isn't going to work.
	 *
	 * XXX we don't let meta-data writes run out of space because they can
	 * come from the segment writer.  We need to make sure that there is
	 * enough space reserved so that there's room to write meta-data
	 * blocks.
	 */
	if (!(bp->b_flags & B_LOCKED)) {
		fsb = fragstofsb(fs, numfrags(fs, bp->b_bcount));

		ip = VTOI(bp->b_vp);
		if (flags & BW_CLEAN) {
			LFS_SET_UINO(ip, IN_CLEANING);
		} else {
			LFS_SET_UINO(ip, IN_MODIFIED);
		}
		fs->lfs_avail -= fsb;
		bp->b_flags |= B_DELWRI;

		LFS_LOCK_BUF(bp);
		bp->b_flags &= ~(B_READ | B_DONE | B_ERROR);
		s = splbio();
		reassignbuf(bp, bp->b_vp);
		splx(s);
	}

	if (bp->b_flags & B_CALL)
		bp->b_flags &= ~B_BUSY;
	else
		brelse(bp);

	return (0);
}

/*
 * Called and return with the lfs_interlock held, but no other simple_locks
 * held.
 */
void
lfs_flush_fs(struct lfs *fs, int flags)
{
	ASSERT_NO_SEGLOCK(fs);
	LOCK_ASSERT(simple_lock_held(&fs->lfs_interlock));
	LOCK_ASSERT(!simple_lock_held(&lfs_subsys_lock));
	if (fs->lfs_ronly)
		return;

	simple_lock(&lfs_subsys_lock);
	if (lfs_dostats)
		++lfs_stats.flush_invoked;
	simple_unlock(&lfs_subsys_lock);

	simple_unlock(&fs->lfs_interlock);
	lfs_writer_enter(fs, "fldirop");
	lfs_segwrite(fs->lfs_ivnode->v_mount, flags);
	lfs_writer_leave(fs);
	simple_lock(&fs->lfs_interlock);
	fs->lfs_favail = 0; /* XXX */
}

/*
 * This routine initiates segment writes when LFS is consuming too many
 * resources.  Ideally the pageout daemon would be able to direct LFS
 * more subtly.
 * XXX We have one static count of locked buffers;
 * XXX need to think more about the multiple filesystem case.
 *
 * Called and return with lfs_subsys_lock held.
 * If fs != NULL, we hold the segment lock for fs.
 */
void
lfs_flush(struct lfs *fs, int flags, int only_onefs)
{
	extern u_int64_t locked_fakequeue_count;
	struct mount *mp, *nmp;
	struct lfs *tfs;

	LOCK_ASSERT(simple_lock_held(&lfs_subsys_lock));
	KDASSERT(fs == NULL || !LFS_SEGLOCK_HELD(fs));

	if (lfs_dostats)
		++lfs_stats.write_exceeded;
	/* XXX should we include SEGM_CKP here? */
	if (lfs_writing && !(flags & SEGM_SYNC)) {
		DLOG((DLOG_FLUSH, "lfs_flush: not flushing because another flush is active\n"));
		return;
	}
	while (lfs_writing)
		ltsleep(&lfs_writing, PRIBIO + 1, "lfsflush", 0,
			&lfs_subsys_lock);
	lfs_writing = 1;

	simple_unlock(&lfs_subsys_lock);

	if (only_onefs) {
		KASSERT(fs != NULL);
		if (vfs_busy(fs->lfs_ivnode->v_mount, LK_NOWAIT,
			     &mountlist_slock))
			goto errout;
		simple_lock(&fs->lfs_interlock);
		lfs_flush_fs(fs, flags);
		simple_unlock(&fs->lfs_interlock);
		vfs_unbusy(fs->lfs_ivnode->v_mount);
	} else {
		locked_fakequeue_count = 0;
		simple_lock(&mountlist_slock);
		for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
		     mp = nmp) {
			if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
				DLOG((DLOG_FLUSH, "lfs_flush: fs vfs_busy\n"));
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				continue;
			}
			if (strncmp(&mp->mnt_stat.f_fstypename[0], MOUNT_LFS,
			    MFSNAMELEN) == 0) {
				tfs = VFSTOUFS(mp)->um_lfs;
				simple_lock(&tfs->lfs_interlock);
				lfs_flush_fs(tfs, flags);
				simple_unlock(&tfs->lfs_interlock);
			}
			simple_lock(&mountlist_slock);
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			vfs_unbusy(mp);
		}
		simple_unlock(&mountlist_slock);
	}
	LFS_DEBUG_COUNTLOCKED("flush");
	wakeup(&lfs_subsys_pages);

    errout:
	simple_lock(&lfs_subsys_lock);
	KASSERT(lfs_writing);
	lfs_writing = 0;
	wakeup(&lfs_writing);
}

#define INOCOUNT(fs) howmany((fs)->lfs_uinodes, INOPB(fs))
#define INOBYTES(fs) ((fs)->lfs_uinodes * sizeof (struct ufs1_dinode))

/*
 * make sure that we don't have too many locked buffers.
 * flush buffers if needed.
 */
int
lfs_check(struct vnode *vp, daddr_t blkno, int flags)
{
	int error;
	struct lfs *fs;
	struct inode *ip;
	extern pid_t lfs_writer_daemon;

	error = 0;
	ip = VTOI(vp);

	/* If out of buffers, wait on writer */
	/* XXX KS - if it's the Ifile, we're probably the cleaner! */
	if (ip->i_number == LFS_IFILE_INUM)
		return 0;
	/* If we're being called from inside a dirop, don't sleep */
	if (ip->i_flag & IN_ADIROP)
		return 0;

	fs = ip->i_lfs;

	ASSERT_NO_SEGLOCK(fs);
	LOCK_ASSERT(!simple_lock_held(&fs->lfs_interlock));

	/*
	 * If we would flush below, but dirops are active, sleep.
	 * Note that a dirop cannot ever reach this code!
	 */
	simple_lock(&fs->lfs_interlock);
	simple_lock(&lfs_subsys_lock);
	while (fs->lfs_dirops > 0 &&
	       (locked_queue_count + INOCOUNT(fs) > LFS_MAX_BUFS ||
		locked_queue_bytes + INOBYTES(fs) > LFS_MAX_BYTES ||
		lfs_subsys_pages > LFS_MAX_PAGES ||
		fs->lfs_dirvcount > LFS_MAX_FSDIROP(fs) ||
		lfs_dirvcount > LFS_MAX_DIROP || fs->lfs_diropwait > 0))
	{
		simple_unlock(&lfs_subsys_lock);
		++fs->lfs_diropwait;
		ltsleep(&fs->lfs_writer, PRIBIO+1, "bufdirop", 0,
			&fs->lfs_interlock);
		--fs->lfs_diropwait;
		simple_lock(&lfs_subsys_lock);
	}

#ifdef DEBUG
	if (locked_queue_count + INOCOUNT(fs) > LFS_MAX_BUFS)
		DLOG((DLOG_FLUSH, "lfs_check: lqc = %d, max %d\n",
		      locked_queue_count + INOCOUNT(fs), LFS_MAX_BUFS));
	if (locked_queue_bytes + INOBYTES(fs) > LFS_MAX_BYTES)
		DLOG((DLOG_FLUSH, "lfs_check: lqb = %ld, max %ld\n",
		      locked_queue_bytes + INOBYTES(fs), LFS_MAX_BYTES));
	if (lfs_subsys_pages > LFS_MAX_PAGES)
		DLOG((DLOG_FLUSH, "lfs_check: lssp = %d, max %d\n",
		      lfs_subsys_pages, LFS_MAX_PAGES));
	if (lfs_fs_pagetrip && fs->lfs_pages > lfs_fs_pagetrip)
		DLOG((DLOG_FLUSH, "lfs_check: fssp = %d, trip at %d\n",
		      fs->lfs_pages, lfs_fs_pagetrip));
	if (lfs_dirvcount > LFS_MAX_DIROP)
		DLOG((DLOG_FLUSH, "lfs_check: ldvc = %d, max %d\n",
		      lfs_dirvcount, LFS_MAX_DIROP));
	if (fs->lfs_dirvcount > LFS_MAX_FSDIROP(fs))
		DLOG((DLOG_FLUSH, "lfs_check: lfdvc = %d, max %d\n",
		      fs->lfs_dirvcount, LFS_MAX_FSDIROP(fs)));
	if (fs->lfs_diropwait > 0)
		DLOG((DLOG_FLUSH, "lfs_check: ldvw = %d\n",
		      fs->lfs_diropwait));
#endif

	/* If there are too many pending dirops, we have to flush them. */
	if (fs->lfs_dirvcount > LFS_MAX_FSDIROP(fs) ||
	    lfs_dirvcount > LFS_MAX_DIROP || fs->lfs_diropwait > 0) {
		flags |= SEGM_CKP;
	}

	if (locked_queue_count + INOCOUNT(fs) > LFS_MAX_BUFS ||
	    locked_queue_bytes + INOBYTES(fs) > LFS_MAX_BYTES ||
	    lfs_subsys_pages > LFS_MAX_PAGES ||
	    fs->lfs_dirvcount > LFS_MAX_FSDIROP(fs) ||
	    lfs_dirvcount > LFS_MAX_DIROP || fs->lfs_diropwait > 0) {
		simple_unlock(&fs->lfs_interlock);
		lfs_flush(fs, flags, 0);
	} else if (lfs_fs_pagetrip && fs->lfs_pages > lfs_fs_pagetrip) {
		/*
		 * If we didn't flush the whole thing, some filesystems
		 * still might want to be flushed.
		 */
		++fs->lfs_pdflush;
		wakeup(&lfs_writer_daemon);
		simple_unlock(&fs->lfs_interlock);
	} else
		simple_unlock(&fs->lfs_interlock);

	while (locked_queue_count + INOCOUNT(fs) > LFS_WAIT_BUFS ||
		locked_queue_bytes + INOBYTES(fs) > LFS_WAIT_BYTES ||
		lfs_subsys_pages > LFS_WAIT_PAGES ||
		fs->lfs_dirvcount > LFS_MAX_FSDIROP(fs) ||
		lfs_dirvcount > LFS_MAX_DIROP) {

		if (lfs_dostats)
			++lfs_stats.wait_exceeded;
		DLOG((DLOG_AVAIL, "lfs_check: waiting: count=%d, bytes=%ld\n",
		      locked_queue_count, locked_queue_bytes));
		error = ltsleep(&locked_queue_count, PCATCH | PUSER,
				"buffers", hz * LFS_BUFWAIT, &lfs_subsys_lock);
		if (error != EWOULDBLOCK)
			break;

		/*
		 * lfs_flush might not flush all the buffers, if some of the
		 * inodes were locked or if most of them were Ifile blocks
		 * and we weren't asked to checkpoint.	Try flushing again
		 * to keep us from blocking indefinitely.
		 */
		if (locked_queue_count + INOCOUNT(fs) > LFS_MAX_BUFS ||
		    locked_queue_bytes + INOBYTES(fs) > LFS_MAX_BYTES) {
			lfs_flush(fs, flags | SEGM_CKP, 0);
		}
	}
	simple_unlock(&lfs_subsys_lock);
	return (error);
}

/*
 * Allocate a new buffer header.
 */
struct buf *
lfs_newbuf(struct lfs *fs, struct vnode *vp, daddr_t daddr, size_t size, int type)
{
	struct buf *bp;
	size_t nbytes;
	int s;

	ASSERT_MAYBE_SEGLOCK(fs);
	nbytes = roundup(size, fsbtob(fs, 1));

	bp = getiobuf();
	if (nbytes) {
		bp->b_data = lfs_malloc(fs, nbytes, type);
		/* memset(bp->b_data, 0, nbytes); */
	}
#ifdef DIAGNOSTIC
	if (vp == NULL)
		panic("vp is NULL in lfs_newbuf");
	if (bp == NULL)
		panic("bp is NULL after malloc in lfs_newbuf");
#endif
	bp->b_vp = NULL;
	s = splbio();
	bgetvp(vp, bp);
	splx(s);

	bp->b_bufsize = size;
	bp->b_bcount = size;
	bp->b_lblkno = daddr;
	bp->b_blkno = daddr;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_iodone = lfs_callback;
	bp->b_flags = B_BUSY | B_CALL | B_NOCACHE;
	bp->b_private = fs;

	return (bp);
}

void
lfs_freebuf(struct lfs *fs, struct buf *bp)
{
	int s;

	s = splbio();
	if (bp->b_vp)
		brelvp(bp);
	if (!(bp->b_flags & B_INVAL)) { /* B_INVAL indicates a "fake" buffer */
		lfs_free(fs, bp->b_data, LFS_NB_UNKNOWN);
		bp->b_data = NULL;
	}
	splx(s);
	putiobuf(bp);
}

/*
 * Definitions for the buffer free lists.
 */
#define BQUEUES		4		/* number of free buffer queues */

#define BQ_LOCKED	0		/* super-blocks &c */
#define BQ_LRU		1		/* lru, useful buffers */
#define BQ_AGE		2		/* rubbish */
#define BQ_EMPTY	3		/* buffer headers with no memory */

extern TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
extern struct simplelock bqueue_slock;

/*
 * Count buffers on the "locked" queue, and compare it to a pro-forma count.
 * Don't count malloced buffers, since they don't detract from the total.
 */
void
lfs_countlocked(int *count, long *bytes, const char *msg)
{
	struct buf *bp;
	int n = 0;
	long int size = 0L;
	int s;

	s = splbio();
	simple_lock(&bqueue_slock);
	TAILQ_FOREACH(bp, &bufqueues[BQ_LOCKED], b_freelist) {
		KASSERT(!(bp->b_flags & B_CALL));
		n++;
		size += bp->b_bufsize;
#ifdef DIAGNOSTIC
		if (n > nbuf)
			panic("lfs_countlocked: this can't happen: more"
			      " buffers locked than exist");
#endif
	}
	/*
	 * Theoretically this function never really does anything.
	 * Give a warning if we have to fix the accounting.
	 */
	if (n != *count) {
		DLOG((DLOG_LLIST, "lfs_countlocked: %s: adjusted buf count"
		      " from %d to %d\n", msg, *count, n));
	}
	if (size != *bytes) {
		DLOG((DLOG_LLIST, "lfs_countlocked: %s: adjusted byte count"
		      " from %ld to %ld\n", msg, *bytes, size));
	}
	*count = n;
	*bytes = size;
	simple_unlock(&bqueue_slock);
	splx(s);
	return;
}

int
lfs_wait_pages(void)
{
	int active, inactive;

	uvm_estimatepageable(&active, &inactive);
	return LFS_WAIT_RESOURCE(active + inactive + uvmexp.free, 1);
}

int
lfs_max_pages(void)
{
	int active, inactive;

	uvm_estimatepageable(&active, &inactive);
	return LFS_MAX_RESOURCE(active + inactive + uvmexp.free, 1);
}
