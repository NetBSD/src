/*	$NetBSD: lfs_bio.c,v 1.36 2001/07/13 20:30:23 perseant Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 *	@(#)lfs_bio.c	8.10 (Berkeley) 6/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/kernel.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <sys/malloc.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/* Macros to clear/set/test flags. */
# define	SET(t, f)	(t) |= (f)
# define	CLR(t, f)	(t) &= ~(f)
# define	ISSET(t, f)	((t) & (f))

/*
 * LFS block write function.
 *
 * XXX
 * No write cost accounting is done.
 * This is almost certainly wrong for synchronous operations and NFS.
 */
int	lfs_allclean_wakeup;		/* Cleaner wakeup address. */
int	locked_queue_count   = 0;	/* XXX Count of locked-down buffers. */
long	locked_queue_bytes   = 0L;	/* XXX Total size of locked buffers. */
int	lfs_writing          = 0;	/* Set if already kicked off a writer
					   because of buffer space */
extern int lfs_dostats;

/*
 * Try to reserve some blocks, prior to performing a sensitive operation that
 * requires the vnode lock to be honored.  If there is not enough space, give
 * up the vnode lock temporarily and wait for the space to become available.
 *
 * Called with vp locked.  (Note nowever that if nb < 0, vp is ignored.)
 */
int
lfs_reserve(struct lfs *fs, struct vnode *vp, int nb)
{
	CLEANERINFO *cip;
	struct buf *bp;
	int error, slept;

	slept = 0;
	while (nb > 0 && !lfs_fits(fs, nb + fs->lfs_ravail)) {
		VOP_UNLOCK(vp, 0);

		if (!slept) {
#ifdef DEBUG
			printf("lfs_reserve: waiting for %ld (bfree = %d,"
			       " est_bfree = %d)\n",
			       nb + fs->lfs_ravail, fs->lfs_bfree,
			       LFS_EST_BFREE(fs));
#endif
		}
		++slept;

		/* Wake up the cleaner */
		LFS_CLEANERINFO(cip, fs, bp);
		LFS_SYNC_CLEANERINFO(cip, fs, bp, 0);
		wakeup(&lfs_allclean_wakeup);
		wakeup(&fs->lfs_nextseg);
			
		error = tsleep(&fs->lfs_avail, PCATCH | PUSER, "lfs_reserve",
			       0);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY); /* XXX use lockstatus */
		if (error)
			return error;
	}
	if (slept)
		printf("lfs_reserve: woke up\n");
	fs->lfs_ravail += nb;
	return 0;
}

/*
 *
 * XXX we don't let meta-data writes run out of space because they can
 * come from the segment writer.  We need to make sure that there is
 * enough space reserved so that there's room to write meta-data
 * blocks.
 *
 * Also, we don't let blocks that have come to us from the cleaner
 * run out of space.
 */
#define CANT_WAIT(BP,F) (IS_IFILE((BP)) || (BP)->b_lblkno<0 || ((F) & BW_CLEAN))

int
lfs_bwrite(void *v)
{
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	struct buf *bp = ap->a_bp;
	struct inode *ip;

	ip = VTOI(bp->b_vp);

#ifdef DIAGNOSTIC
        if (VTOI(bp->b_vp)->i_lfs->lfs_ronly == 0 && (bp->b_flags & B_ASYNC)) {
		panic("bawrite LFS buffer");
	}
#endif /* DIAGNOSTIC */
	return lfs_bwrite_ext(bp,0);
}

/* 
 * Determine if there is enough room currently available to write db
 * disk blocks.  We need enough blocks for the new blocks, the current
 * inode blocks, a summary block, plus potentially the ifile inode and
 * the segment usage table, plus an ifile page.
 */
int
lfs_fits(struct lfs *fs, int fsb)
{
	int needed;

	needed = fsb + btofsb(fs, fs->lfs_sumsize) +
		fsbtodb(fs, howmany(fs->lfs_uinodes + 1, INOPB(fs)) +
			    fs->lfs_segtabsz + btofsb(fs, fs->lfs_sumsize));

	if (needed >= fs->lfs_avail) {
#ifdef DEBUG
		printf("lfs_fits: no fit: fsb = %d, uinodes = %d, "
		       "needed = %d, avail = %d\n",
		       fsb, fs->lfs_uinodes, needed, fs->lfs_avail);
#endif
		return 0;
	}
	return 1;
}

int
lfs_availwait(struct lfs *fs, int db)
{
	int error;
	CLEANERINFO *cip;
	struct buf *cbp;

	while (!lfs_fits(fs, db)) {
		/*
		 * Out of space, need cleaner to run.
		 * Update the cleaner info, then wake it up.
		 * Note the cleanerinfo block is on the ifile
		 * so it CANT_WAIT.
		 */
		LFS_CLEANERINFO(cip, fs, cbp);
		LFS_SYNC_CLEANERINFO(cip, fs, cbp, 0);
		
		printf("lfs_availwait: out of available space, "
		       "waiting on cleaner\n");
		
		wakeup(&lfs_allclean_wakeup);
		wakeup(&fs->lfs_nextseg);
#ifdef DIAGNOSTIC
		if (fs->lfs_seglock && fs->lfs_lockpid == curproc->p_pid)
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
	int fsb, error, s;
	
	/*
	 * Don't write *any* blocks if we're mounted read-only.
	 * In particular the cleaner can't write blocks either.
	 */
        if(VTOI(bp->b_vp)->i_lfs->lfs_ronly) {
		bp->b_flags &= ~(B_DELWRI | B_READ | B_ERROR);
		LFS_UNLOCK_BUF(bp);
		if(bp->b_flags & B_CALL)
			bp->b_flags &= ~B_BUSY;
		else
			brelse(bp);
		return EROFS;
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
		fs = VFSTOUFS(bp->b_vp->v_mount)->um_lfs;
		fsb = fragstofsb(fs, numfrags(fs, bp->b_bcount));
		if (!CANT_WAIT(bp, flags)) {
			if ((error = lfs_availwait(fs, fsb)) != 0) {
				brelse(bp);
				return error;
			}
		}
		
		ip = VTOI(bp->b_vp);
		if (bp->b_flags & B_CALL) {
			LFS_SET_UINO(ip, IN_CLEANING);
		} else {
			LFS_SET_UINO(ip, IN_MODIFIED);
			if (bp->b_lblkno >= 0)
				LFS_SET_UINO(ip, IN_UPDATE);
		}
		fs->lfs_avail -= fsb;
		bp->b_flags |= B_DELWRI;

		LFS_LOCK_BUF(bp);
		bp->b_flags &= ~(B_READ | B_ERROR);
		s = splbio();
		reassignbuf(bp, bp->b_vp);
		splx(s);

	}
	
	if(bp->b_flags & B_CALL)
		bp->b_flags &= ~B_BUSY;
	else
		brelse(bp);
	
	return (0);
}

void
lfs_flush_fs(struct lfs *fs, int flags)
{
	if(fs->lfs_ronly == 0 && fs->lfs_dirops == 0)
	{
		/* disallow dirops during flush */
		fs->lfs_writer++;

		/*
		 * We set the queue to 0 here because we
		 * are about to write all the dirty
		 * buffers we have.  If more come in
		 * while we're writing the segment, they
		 * may not get written, so we want the
		 * count to reflect these new writes
		 * after the segwrite completes.
		 */
		if(lfs_dostats)
			++lfs_stats.flush_invoked;
		lfs_segwrite(fs->lfs_ivnode->v_mount, flags);

		/* XXX KS - allow dirops again */
		if(--fs->lfs_writer==0)
			wakeup(&fs->lfs_dirops);
	}
}

/*
 * XXX
 * This routine flushes buffers out of the B_LOCKED queue when LFS has too
 * many locked down.  Eventually the pageout daemon will simply call LFS
 * when pages need to be reclaimed.  Note, we have one static count of locked
 * buffers, so we can't have more than a single file system.  To make this
 * work for multiple file systems, put the count into the mount structure.
 */
void
lfs_flush(struct lfs *fs, int flags)
{
	int s;
	struct mount *mp, *nmp;
	
	if(lfs_dostats) 
		++lfs_stats.write_exceeded;
	if (lfs_writing && flags==0) {/* XXX flags */
#ifdef DEBUG_LFS
		printf("lfs_flush: not flushing because another flush is active\n");
#endif
		return;
	}
	lfs_writing = 1;
	
	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		if (strncmp(&mp->mnt_stat.f_fstypename[0], MOUNT_LFS, MFSNAMELEN)==0)
			lfs_flush_fs(((struct ufsmount *)mp->mnt_data)->ufsmount_u.lfs, flags);
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);

#if 1 || defined(DEBUG)
	s = splbio();
	lfs_countlocked(&locked_queue_count, &locked_queue_bytes);
	splx(s);
	wakeup(&locked_queue_count);
#endif /* 1 || DEBUG */

	lfs_writing = 0;
}

int
lfs_check(struct vnode *vp, ufs_daddr_t blkno, int flags)
{
	int error;
	struct lfs *fs;
	struct inode *ip;
	extern int lfs_dirvcount;

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

	/*
	 * If we would flush below, but dirops are active, sleep.
	 * Note that a dirop cannot ever reach this code!
	 */
	while (fs->lfs_dirops > 0 &&
	       (locked_queue_count > LFS_MAX_BUFS ||
                locked_queue_bytes > LFS_MAX_BYTES ||
                lfs_dirvcount > LFS_MAXDIROP || fs->lfs_diropwait > 0))
	{
		++fs->lfs_diropwait;
		tsleep(&fs->lfs_writer, PRIBIO+1, "bufdirop", 0);
		--fs->lfs_diropwait;
	}

	if (locked_queue_count > LFS_MAX_BUFS ||
	    locked_queue_bytes > LFS_MAX_BYTES ||
	    lfs_dirvcount > LFS_MAXDIROP || fs->lfs_diropwait > 0)
	{
		++fs->lfs_writer;
		lfs_flush(fs, flags);
		if(--fs->lfs_writer==0)
			wakeup(&fs->lfs_dirops);
	}

	while (locked_queue_count > LFS_WAIT_BUFS
	       || locked_queue_bytes > LFS_WAIT_BYTES)
	{
		if(lfs_dostats)
			++lfs_stats.wait_exceeded;
#ifdef DEBUG_LFS
		printf("lfs_check: waiting: count=%d, bytes=%ld\n",
			locked_queue_count, locked_queue_bytes);
#endif
		error = tsleep(&locked_queue_count, PCATCH | PUSER,
			       "buffers", hz * LFS_BUFWAIT);
		if (error != EWOULDBLOCK)
			break;
		/*
		 * lfs_flush might not flush all the buffers, if some of the
		 * inodes were locked or if most of them were Ifile blocks
		 * and we weren't asked to checkpoint.  Try flushing again
		 * to keep us from blocking indefinitely.
		 */
		if (locked_queue_count > LFS_MAX_BUFS ||
		    locked_queue_bytes > LFS_MAX_BYTES)
		{
			++fs->lfs_writer;
			lfs_flush(fs, flags | SEGM_CKP);
			if(--fs->lfs_writer==0)
				wakeup(&fs->lfs_dirops);
		}
	}
	return (error);
}

/*
 * Allocate a new buffer header.
 */
#ifdef MALLOCLOG
# define DOMALLOC(S, T, F) _malloc((S), (T), (F), file, line)
struct buf *
lfs_newbuf_malloclog(struct lfs *fs, struct vnode *vp, ufs_daddr_t daddr, size_t size, char *file, int line)
#else
# define DOMALLOC(S, T, F) malloc((S), (T), (F))
struct buf *
lfs_newbuf(struct lfs *fs, struct vnode *vp, ufs_daddr_t daddr, size_t size)
#endif
{
	struct buf *bp;
	size_t nbytes;
	int s;
	
	nbytes = roundup(size, fsbtob(fs, 1));
	
	bp = DOMALLOC(sizeof(struct buf), M_SEGMENT, M_WAITOK);
	bzero(bp, sizeof(struct buf));
	if (nbytes)
		bp->b_data = DOMALLOC(nbytes, M_SEGMENT, M_WAITOK);
	if(nbytes) {
		bzero(bp->b_data, nbytes);
	}
#ifdef DIAGNOSTIC	
	if(vp==NULL)
		panic("vp is NULL in lfs_newbuf");
	if(bp==NULL)
		panic("bp is NULL after malloc in lfs_newbuf");
#endif
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
	bp->b_flags |= B_BUSY | B_CALL | B_NOCACHE;
	
	return (bp);
}

#ifdef MALLOCLOG
# define DOFREE(A, T) _free((A), (T), file, line)
void
lfs_freebuf_malloclog(struct buf *bp, char *file, int line)
#else
# define DOFREE(A, T) free((A), (T))
void
lfs_freebuf(struct buf *bp)
#endif
{
	int s;
	
	s = splbio();
	if(bp->b_vp)
		brelvp(bp);
	splx(s);
	if (!(bp->b_flags & B_INVAL)) { /* B_INVAL indicates a "fake" buffer */
		DOFREE(bp->b_data, M_SEGMENT);
		bp->b_data = NULL;
	}
	DOFREE(bp, M_SEGMENT);
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

/*
 * Return a count of buffers on the "locked" queue.
 * Don't count malloced buffers, since they don't detract from the total.
 */
void
lfs_countlocked(int *count, long *bytes)
{
	struct buf *bp;
	int n = 0;
	long int size = 0L;

	for (bp = bufqueues[BQ_LOCKED].tqh_first; bp;
	    bp = bp->b_freelist.tqe_next) {
		if (bp->b_flags & B_CALL) /* Malloced buffer */
			continue;
		n++;
		size += bp->b_bufsize;
#ifdef DEBUG_LOCKED_LIST
		if (n > nbuf)
			panic("lfs_countlocked: this can't happen: more"
			      " buffers locked than exist");
#endif
	}
#ifdef DEBUG
	/* Theoretically this function never really does anything */
	if (n != *count)
		printf("lfs_countlocked: adjusted buf count from %d to %d\n",
		       *count, n);
	if (size != *bytes)
		printf("lfs_countlocked: adjusted byte count from %ld to %ld\n",
		       *bytes, size);
#endif
	*count = n;
	*bytes = size;
	return;
}
