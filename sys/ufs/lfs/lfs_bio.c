/*	$NetBSD: lfs_bio.c,v 1.15 1999/12/04 12:18:21 ragge Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 * #define WRITE_THRESHHOLD    ((nbuf >> 1) - 10)
 * #define WAIT_THRESHHOLD     (nbuf - (nbuf >> 2) - 10)
 */
#define LFS_MAX_BUFS        ((nbuf >> 2) - 10)
#define LFS_WAIT_BUFS       ((nbuf >> 1) - (nbuf >> 3) - 10)
/* These are new ... is LFS taking up too much memory in its buffers? */
#define LFS_MAX_BYTES       (((bufpages >> 2) - 10) * NBPG)
#define LFS_WAIT_BYTES      (((bufpages >> 1) - (bufpages >> 3) - 10) * NBPG)
#define LFS_BUFWAIT         2
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
lfs_bwrite(v)
	void *v;
{
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	register struct buf *bp = ap->a_bp;

#ifdef DIAGNOSTIC
	if(bp->b_flags & B_ASYNC)
		panic("bawrite LFS buffer");
#endif /* DIAGNOSTIC */
	return lfs_bwrite_ext(bp,0);
}

/* 
 * Determine if there is enough room currently available to write db
 * disk blocks.  We need enough blocks for the new blocks, the current
 * inode blocks, a summary block, plus potentially the ifile inode and
 * the segment usage table, plus an ifile page.
 */
inline static int lfs_fits(struct lfs *fs, int db)
{
	if(((db + (fs->lfs_uinodes + INOPB((fs))) /
	     INOPB(fs) + fsbtodb(fs, 1) + LFS_SUMMARY_SIZE / DEV_BSIZE +
	     fs->lfs_segtabsz)) >= fs->lfs_avail)
	{
		return 0;
	}

	/*
	 * Also check the number of segments available for writing.
	 * If you don't do this here, it is possible for the *cleaner* to
	 * cause us to become starved of segments, by flushing the pending
	 * block list.
	 *
	 * XXX the old lfs_markv did not have this problem.
	 */
	if (fs->lfs_nclean <= MIN_FREE_SEGS)
		return 0;

	return 1;
}

int
lfs_bwrite_ext(bp, flags)
	struct buf *bp;
	int flags;
{
	struct lfs *fs;
	struct inode *ip;
	int db, error, s;
	
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
		db = fragstodb(fs, numfrags(fs, bp->b_bcount));
#ifdef DEBUG_LFS
		if(CANT_WAIT(bp,flags)) {
			if(((db + (fs->lfs_uinodes + INOPB((fs))) / INOPB(fs)
			     + fsbtodb(fs, 1)
			     + LFS_SUMMARY_SIZE / DEV_BSIZE
			     + fs->lfs_segtabsz)) >= fs->lfs_avail)
			{
				printf("A");
			}
			if (fs->lfs_nclean <= MIN_FREE_SEGS-1)
				printf("M");
		}
#endif
		while (!lfs_fits(fs, db) && !CANT_WAIT(bp,flags)) {
			/* Out of space, need cleaner to run */
			
			wakeup(&lfs_allclean_wakeup);
			wakeup(&fs->lfs_nextseg);
			error = tsleep(&fs->lfs_avail, PCATCH | PUSER, "cleaner", NULL);
			if (error) {
				/* printf("lfs_bwrite: error in tsleep"); */
				brelse(bp);
				return (error);
			}
		}
		
		ip = VTOI(bp->b_vp);
		if (bp->b_flags & B_CALL)
		{
			if(!(ip->i_flag & IN_CLEANING))
				++fs->lfs_uinodes;
			ip->i_flag |= IN_CLEANING;
		} else {
			if(!(ip->i_flag & IN_MODIFIED))
				++fs->lfs_uinodes;
			ip->i_flag |= IN_CHANGE | IN_MODIFIED | IN_UPDATE;
		}
		fs->lfs_avail -= db;
		++locked_queue_count;
		locked_queue_bytes += bp->b_bufsize;
		s = splbio();
#ifdef LFS_HONOR_RDONLY
		/*
		 * XXX KS - Don't write blocks if we're mounted ro.
		 * Placement here means that the cleaner can't write
		 * blocks either.
		 */
	        if(VTOI(bp->b_vp)->i_lfs->lfs_ronly)
			bp->b_flags &= ~(B_DELWRI|B_LOCKED);
		else
#endif
			bp->b_flags |= B_DELWRI | B_LOCKED;
		bp->b_flags &= ~(B_READ | B_ERROR);
		reassignbuf(bp, bp->b_vp);
		splx(s);

	}
	
	if(bp->b_flags & B_CALL)
		bp->b_flags &= ~B_BUSY;
	else
		brelse(bp);
	
	return (0);
}

void lfs_flush_fs(mp, flags)
	struct mount *mp;
	int flags;
{
	struct lfs *lfsp;

	lfsp = ((struct ufsmount *)mp->mnt_data)->ufsmount_u.lfs;
	if((mp->mnt_flag & MNT_RDONLY) == 0 &&
	   lfsp->lfs_dirops==0)
	{
		/* disallow dirops during flush */
		lfsp->lfs_writer++;

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
		lfs_segwrite(mp, flags);

		/* XXX KS - allow dirops again */
		if(--lfsp->lfs_writer==0)
			wakeup(&lfsp->lfs_dirops);
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
lfs_flush(fs, flags)
	struct lfs *fs;
	int flags;
{
	register struct mount *mp, *nmp;
	
	if(lfs_dostats) 
		++lfs_stats.write_exceeded;
	if (lfs_writing && flags==0) /* XXX flags */
		return;
	lfs_writing = 1;
	
	simple_lock(&mountlist_slock);
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		if (strncmp(&mp->mnt_stat.f_fstypename[0], MOUNT_LFS, MFSNAMELEN)==0)
			lfs_flush_fs(mp, flags);
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);

	lfs_countlocked(&locked_queue_count,&locked_queue_bytes);
	wakeup(&locked_queue_count);

	lfs_writing = 0;
}

int
lfs_check(vp, blkno, flags)
	struct vnode *vp;
	ufs_daddr_t blkno;
	int flags;
{
	int error;
	struct lfs *fs;
	extern int lfs_dirvcount;

	error = 0;
	
	/* If out of buffers, wait on writer */
	/* XXX KS - if it's the Ifile, we're probably the cleaner! */
	if (VTOI(vp)->i_number == LFS_IFILE_INUM)
		return 0;

	/* If dirops are active, can't flush.  Wait for SET_ENDOP */
	fs = VTOI(vp)->i_lfs;
	if (fs->lfs_dirops)
		return 0;

	if (locked_queue_count > LFS_MAX_BUFS
	    || locked_queue_bytes > LFS_MAX_BYTES
	    || lfs_dirvcount > LFS_MAXDIROP
	    || fs->lfs_diropwait > 0)
	{
		++fs->lfs_writer;
		lfs_flush(fs, flags);
		if(--fs->lfs_writer==0)
			wakeup(&fs->lfs_dirops);
	}

	while  (locked_queue_count > LFS_WAIT_BUFS
		|| locked_queue_bytes > LFS_WAIT_BYTES)
	{
		if(lfs_dostats)
			++lfs_stats.wait_exceeded;
		error = tsleep(&locked_queue_count, PCATCH | PUSER,
			       "buffers", hz * LFS_BUFWAIT);
	}
	return (error);
}

/*
 * Allocate a new buffer header.
 */
struct buf *
lfs_newbuf(vp, daddr, size)
	struct vnode *vp;
	ufs_daddr_t daddr;
	size_t size;
{
	struct buf *bp;
	size_t nbytes;
	int s;
	
	nbytes = roundup(size, DEV_BSIZE);
	
	bp = malloc(sizeof(struct buf), M_SEGMENT, M_WAITOK);
	bzero(bp, sizeof(struct buf));
	if (nbytes)
		bp->b_data = malloc(nbytes, M_SEGMENT, M_WAITOK);
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

void
lfs_freebuf(bp)
	struct buf *bp;
{
	int s;
	
	s = splbio();
	if(bp->b_vp)
		brelvp(bp);
	splx(s);
	if (!(bp->b_flags & B_INVAL)) { /* B_INVAL indicates a "fake" buffer */
		free(bp->b_data, M_SEGMENT);
		bp->b_data = NULL;
	}
	free(bp, M_SEGMENT);
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
 */
void
lfs_countlocked(count, bytes)
	int *count;
	long *bytes;
{
	register struct buf *bp;
	register int n = 0;
	register long int size = 0L;

	for (bp = bufqueues[BQ_LOCKED].tqh_first; bp;
	    bp = bp->b_freelist.tqe_next) {
		n++;
		size += bp->b_bufsize;
	}
	*count = n;
	*bytes = size;
	return;
}
