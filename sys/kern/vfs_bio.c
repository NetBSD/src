/*	$NetBSD: vfs_bio.c,v 1.90 2003/02/06 11:46:49 pk Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_bio.c	8.6 (Berkeley) 1/11/94
 */

/*
 * Some references:
 *	Bach: The Design of the UNIX Operating System (Prentice Hall, 1986)
 *	Leffler, et al.: The Design and Implementation of the 4.3BSD
 *		UNIX Operating System (Addison Welley, 1989)
 */

#include "opt_softdep.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_bio.c,v 1.90 2003/02/06 11:46:49 pk Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>

#include <uvm/uvm.h>

#include <miscfs/specfs/specdev.h>

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

/*
 * Definitions for the buffer hash lists.
 */
#define	BUFHASH(dvp, lbn)	\
	(&bufhashtbl[(((long)(dvp) >> 8) + (int)(lbn)) & bufhash])
LIST_HEAD(bufhashhdr, buf) *bufhashtbl, invalhash;
u_long	bufhash;
#ifndef SOFTDEP
struct bio_ops bioops;	/* I/O operation notification */
#endif

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	binshash(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_hash)
#define	bremhash(bp)		LIST_REMOVE(bp, b_hash)

/*
 * Definitions for the buffer free lists.
 */
#define	BQUEUES		4		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
int needbuffer;

/*
 * Buffer queue lock.
 * Take this lock first if also taking some buffer's b_interlock.
 */
struct simplelock bqueue_slock = SIMPLELOCK_INITIALIZER;

/*
 * Buffer pool for I/O buffers.
 */
struct pool bufpool;

/*
 * bread()/breadn() helper.
 */
static __inline struct buf *bio_doread(struct vnode *, daddr_t, int,
					struct ucred *, int);
int count_lock_queue(void);

/*
 * Insq/Remq for the buffer free lists.
 * Call with buffer queue locked.
 */
#define	binsheadfree(bp, dp)	TAILQ_INSERT_HEAD(dp, bp, b_freelist)
#define	binstailfree(bp, dp)	TAILQ_INSERT_TAIL(dp, bp, b_freelist)

void
bremfree(bp)
	struct buf *bp;
{
	struct bqueues *dp = NULL;

	/*
	 * We only calculate the head of the freelist when removing
	 * the last element of the list as that is the only time that
	 * it is needed (e.g. to reset the tail pointer).
	 *
	 * NB: This makes an assumption about how tailq's are implemented.
	 */
	if (TAILQ_NEXT(bp, b_freelist) == NULL) {
		for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
			if (dp->tqh_last == &bp->b_freelist.tqe_next)
				break;
		if (dp == &bufqueues[BQUEUES])
			panic("bremfree: lost tail");
	}
	TAILQ_REMOVE(dp, bp, b_freelist);
}

/*
 * Initialize buffers and hash links for buffers.
 */
void
bufinit()
{
	struct buf *bp;
	struct bqueues *dp;
	u_int i, base, residual;

	/*
	 * Initialize the buffer pool.  This pool is used for buffers
	 * which are strictly I/O control blocks, not buffer cache
	 * buffers.
	 */
	pool_init(&bufpool, sizeof(struct buf), 0, 0, 0, "bufpl", NULL);

	for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
		TAILQ_INIT(dp);
	bufhashtbl = hashinit(nbuf, HASH_LIST, M_CACHE, M_WAITOK, &bufhash);
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		memset((char *)bp, 0, sizeof(*bp));
		simple_lock_init(&bp->b_interlock);
		bp->b_dev = NODEV;
		bp->b_vnbufs.le_next = NOLIST;
		LIST_INIT(&bp->b_dep);
		bp->b_data = buffers + i * MAXBSIZE;
		if (i < residual)
			bp->b_bufsize = (base + 1) * PAGE_SIZE;
		else
			bp->b_bufsize = base * PAGE_SIZE;
		bp->b_flags = B_INVAL;
		dp = bp->b_bufsize ? &bufqueues[BQ_AGE] : &bufqueues[BQ_EMPTY];
		binsheadfree(bp, dp);
		binshash(bp, &invalhash);
	}
}

static __inline struct buf *
bio_doread(vp, blkno, size, cred, async)
	struct vnode *vp;
	daddr_t blkno;
	int size;
	struct ucred *cred;
	int async;
{
	struct buf *bp;
	struct lwp *l  = (curlwp != NULL ? curlwp : &lwp0);	/* XXX */
	struct proc *p = l->l_proc;

	bp = getblk(vp, blkno, size, 0, 0);

#ifdef DIAGNOSTIC
	if (bp == NULL) {
		panic("bio_doread: no such buf");
	}
#endif

	/*
	 * If buffer does not have data valid, start a read.
	 * Note that if buffer is B_INVAL, getblk() won't return it.
	 * Therefore, it's valid if its I/O has completed or been delayed.
	 */
	if (!ISSET(bp->b_flags, (B_DONE | B_DELWRI))) {
		/* Start I/O for the buffer. */
		SET(bp->b_flags, B_READ | async);
		VOP_STRATEGY(bp);

		/* Pay for the read. */
		p->p_stats->p_ru.ru_inblock++;
	} else if (async) {
		brelse(bp);
	}

	return (bp);
}

/*
 * Read a disk block.
 * This algorithm described in Bach (p.54).
 */
int
bread(vp, blkno, size, cred, bpp)
	struct vnode *vp;
	daddr_t blkno;
	int size;
	struct ucred *cred;
	struct buf **bpp;
{
	struct buf *bp;

	/* Get buffer for block. */
	bp = *bpp = bio_doread(vp, blkno, size, cred, 0);

	/* Wait for the read to complete, and return result. */
	return (biowait(bp));
}

/*
 * Read-ahead multiple disk blocks. The first is sync, the rest async.
 * Trivial modification to the breada algorithm presented in Bach (p.55).
 */
int
breadn(vp, blkno, size, rablks, rasizes, nrablks, cred, bpp)
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablks[]; int rasizes[];
	int nrablks;
	struct ucred *cred;
	struct buf **bpp;
{
	struct buf *bp;
	int i;

	bp = *bpp = bio_doread(vp, blkno, size, cred, 0);

	/*
	 * For each of the read-ahead blocks, start a read, if necessary.
	 */
	for (i = 0; i < nrablks; i++) {
		/* If it's in the cache, just go on to next one. */
		if (incore(vp, rablks[i]))
			continue;

		/* Get a buffer for the read-ahead block */
		(void) bio_doread(vp, rablks[i], rasizes[i], cred, B_ASYNC);
	}

	/* Otherwise, we had to start a read for it; wait until it's valid. */
	return (biowait(bp));
}

/*
 * Read with single-block read-ahead.  Defined in Bach (p.55), but
 * implemented as a call to breadn().
 * XXX for compatibility with old file systems.
 */
int
breada(vp, blkno, size, rablkno, rabsize, cred, bpp)
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablkno; int rabsize;
	struct ucred *cred;
	struct buf **bpp;
{

	return (breadn(vp, blkno, size, &rablkno, &rabsize, 1, cred, bpp));	
}

/*
 * Block write.  Described in Bach (p.56)
 */
int
bwrite(bp)
	struct buf *bp;
{
	int rv, sync, wasdelayed, s;
	struct lwp *l  = (curlwp != NULL ? curlwp : &lwp0);	/* XXX */
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct mount *mp;

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	vp = bp->b_vp;
	if (vp != NULL) {
		if (vp->v_type == VBLK)
			mp = vp->v_specmountpoint;
		else
			mp = vp->v_mount;
	} else {
		mp = NULL;
	}

	/*
	 * Remember buffer type, to switch on it later.  If the write was
	 * synchronous, but the file system was mounted with MNT_ASYNC,
	 * convert it to a delayed write.  
	 * XXX note that this relies on delayed tape writes being converted
	 * to async, not sync writes (which is safe, but ugly).
	 */
	sync = !ISSET(bp->b_flags, B_ASYNC);
	if (sync && mp != NULL && ISSET(mp->mnt_flag, MNT_ASYNC)) {
		bdwrite(bp);
		return (0);
	}

	/*
	 * Collect statistics on synchronous and asynchronous writes.
	 * Writes to block devices are charged to their associated
	 * filesystem (if any).
	 */
	if (mp != NULL) {
		if (sync)
			mp->mnt_stat.f_syncwrites++;
		else
			mp->mnt_stat.f_asyncwrites++;
	}

	wasdelayed = ISSET(bp->b_flags, B_DELWRI);

	s = splbio();
	simple_lock(&bp->b_interlock);

	CLR(bp->b_flags, (B_READ | B_DONE | B_ERROR | B_DELWRI));

	/*
	 * Pay for the I/O operation and make sure the buf is on the correct
	 * vnode queue.
	 */
	if (wasdelayed)
		reassignbuf(bp, bp->b_vp);
	else
		p->p_stats->p_ru.ru_oublock++;

	/* Initiate disk write.  Make sure the appropriate party is charged. */
	V_INCR_NUMOUTPUT(bp->b_vp);
	simple_unlock(&bp->b_interlock);
	splx(s);

	VOP_STRATEGY(bp);

	if (sync) {
		/* If I/O was synchronous, wait for it to complete. */
		rv = biowait(bp);

		/* Release the buffer. */
		brelse(bp);

		return (rv);
	} else {
		return (0);
	}
}

int
vn_bwrite(v)
	void *v;
{
	struct vop_bwrite_args *ap = v;

	return (bwrite(ap->a_bp));
}

/*
 * Delayed write.
 *
 * The buffer is marked dirty, but is not queued for I/O.
 * This routine should be used when the buffer is expected
 * to be modified again soon, typically a small write that
 * partially fills a buffer.
 *
 * NB: magnetic tapes cannot be delayed; they must be
 * written in the order that the writes are requested.
 *
 * Described in Leffler, et al. (pp. 208-213).
 */
void
bdwrite(bp)
	struct buf *bp;
{
	struct lwp *l  = (curlwp != NULL ? curlwp : &lwp0);	/* XXX */
	struct proc *p = l->l_proc;
	const struct bdevsw *bdev;
	int s;

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	/* If this is a tape block, write the block now. */
	bdev = bdevsw_lookup(bp->b_dev);
	if (bdev != NULL && bdev->d_type == D_TAPE) {
		bawrite(bp);
		return;
	}

	/*
	 * If the block hasn't been seen before:
	 *	(1) Mark it as having been seen,
	 *	(2) Charge for the write,
	 *	(3) Make sure it's on its vnode's correct block list.
	 */
	s = splbio();
	simple_lock(&bp->b_interlock);

	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
		p->p_stats->p_ru.ru_oublock++;
		reassignbuf(bp, bp->b_vp);
	}

	/* Otherwise, the "write" is done, so mark and release the buffer. */
	CLR(bp->b_flags, B_NEEDCOMMIT|B_DONE);
	simple_unlock(&bp->b_interlock);
	splx(s);

	brelse(bp);
}

/*
 * Asynchronous block write; just an asynchronous bwrite().
 */
void
bawrite(bp)
	struct buf *bp;
{
	int s;

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	s = splbio();
	simple_lock(&bp->b_interlock);
	SET(bp->b_flags, B_ASYNC);
	simple_unlock(&bp->b_interlock);
	splx(s);
	VOP_BWRITE(bp);
}

/*
 * Same as first half of bdwrite, mark buffer dirty, but do not release it.
 * Call at splbio() and with the buffer interlock locked.
 * Note: called only from biodone() through ffs softdep's bioops.io_complete()
 */
void
bdirty(bp)
	struct buf *bp;
{
	struct lwp *l  = (curlwp != NULL ? curlwp : &lwp0);	/* XXX */
	struct proc *p = l->l_proc;

	KASSERT(ISSET(bp->b_flags, B_BUSY));
	LOCK_ASSERT(simple_lock_held(&bp->b_interlock));

	CLR(bp->b_flags, B_AGE);

	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
		p->p_stats->p_ru.ru_oublock++;
		reassignbuf(bp, bp->b_vp);
	}
}

/*
 * Release a buffer on to the free lists.
 * Described in Bach (p. 46).
 */
void
brelse(bp)
	struct buf *bp;
{
	struct bqueues *bufq;
	int s;

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	/* Block disk interrupts. */
	s = splbio();
	simple_lock(&bqueue_slock);
	simple_lock(&bp->b_interlock);

	/* Wake up any processes waiting for any buffer to become free. */
	if (needbuffer) {
		needbuffer = 0;
		wakeup(&needbuffer);
	}

	/* Wake up any proceeses waiting for _this_ buffer to become free. */
	if (ISSET(bp->b_flags, B_WANTED)) {
		CLR(bp->b_flags, B_WANTED|B_AGE);
		wakeup(bp);
	}

	/*
	 * Determine which queue the buffer should be on, then put it there.
	 */

	/* If it's locked, don't report an error; try again later. */
	if (ISSET(bp->b_flags, (B_LOCKED|B_ERROR)) == (B_LOCKED|B_ERROR))
		CLR(bp->b_flags, B_ERROR);

	/* If it's not cacheable, or an error, mark it invalid. */
	if (ISSET(bp->b_flags, (B_NOCACHE|B_ERROR)))
		SET(bp->b_flags, B_INVAL);

	if (ISSET(bp->b_flags, B_VFLUSH)) {
		/*
		 * This is a delayed write buffer that was just flushed to
		 * disk.  It is still on the LRU queue.  If it's become
		 * invalid, then we need to move it to a different queue;
		 * otherwise leave it in its current position.
		 */
		CLR(bp->b_flags, B_VFLUSH);
		if (!ISSET(bp->b_flags, B_ERROR|B_INVAL|B_LOCKED|B_AGE))
			goto already_queued;
		else
			bremfree(bp);
	}

	if ((bp->b_bufsize <= 0) || ISSET(bp->b_flags, B_INVAL)) {
		/*
		 * If it's invalid or empty, dissociate it from its vnode
		 * and put on the head of the appropriate queue.
		 */
		if (LIST_FIRST(&bp->b_dep) != NULL && bioops.io_deallocate)
			(*bioops.io_deallocate)(bp);
		CLR(bp->b_flags, B_DONE|B_DELWRI);
		if (bp->b_vp) {
			reassignbuf(bp, bp->b_vp);
			brelvp(bp);
		}
		if (bp->b_bufsize <= 0)
			/* no data */
			bufq = &bufqueues[BQ_EMPTY];
		else
			/* invalid data */
			bufq = &bufqueues[BQ_AGE];
		binsheadfree(bp, bufq);
	} else {
		/*
		 * It has valid data.  Put it on the end of the appropriate
		 * queue, so that it'll stick around for as long as possible.
		 * If buf is AGE, but has dependencies, must put it on last
		 * bufqueue to be scanned, ie LRU. This protects against the
		 * livelock where BQ_AGE only has buffers with dependencies,
		 * and we thus never get to the dependent buffers in BQ_LRU.
		 */
		if (ISSET(bp->b_flags, B_LOCKED))
			/* locked in core */
			bufq = &bufqueues[BQ_LOCKED];
		else if (!ISSET(bp->b_flags, B_AGE))
			/* valid data */
			bufq = &bufqueues[BQ_LRU];
		else {
			/* stale but valid data */
			int has_deps;

			if (LIST_FIRST(&bp->b_dep) != NULL &&
			    bioops.io_countdeps)
				has_deps = (*bioops.io_countdeps)(bp, 0);
			else
				has_deps = 0;
			bufq = has_deps ? &bufqueues[BQ_LRU] :
			    &bufqueues[BQ_AGE];
		}
		binstailfree(bp, bufq);
	}

already_queued:
	/* Unlock the buffer. */
	CLR(bp->b_flags, B_AGE|B_ASYNC|B_BUSY|B_NOCACHE);
	SET(bp->b_flags, B_CACHE);

	/* Allow disk interrupts. */
	simple_unlock(&bp->b_interlock);
	simple_unlock(&bqueue_slock);
	splx(s);
}

/*
 * Determine if a block is in the cache.
 * Just look on what would be its hash chain.  If it's there, return
 * a pointer to it, unless it's marked invalid.  If it's marked invalid,
 * we normally don't return the buffer, unless the caller explicitly
 * wants us to.
 */
struct buf *
incore(vp, blkno)
	struct vnode *vp;
	daddr_t blkno;
{
	struct buf *bp;

	/* Search hash chain */
	LIST_FOREACH(bp, BUFHASH(vp, blkno), b_hash) {
		if (bp->b_lblkno == blkno && bp->b_vp == vp &&
		    !ISSET(bp->b_flags, B_INVAL))
		return (bp);
	}

	return (NULL);
}

/*
 * Get a block of requested size that is associated with
 * a given vnode and block offset. If it is found in the
 * block cache, mark it as having been found, make it busy
 * and return it. Otherwise, return an empty block of the
 * correct size. It is up to the caller to insure that the
 * cached blocks be of the correct size.
 */
struct buf *
getblk(vp, blkno, size, slpflag, slptimeo)
	struct vnode *vp;
	daddr_t blkno;
	int size, slpflag, slptimeo;
{
	struct buf *bp;
	int s, err;

start:
	s = splbio();
	simple_lock(&bqueue_slock);
	bp = incore(vp, blkno);
	if (bp != NULL) {
		simple_lock(&bp->b_interlock);
		if (ISSET(bp->b_flags, B_BUSY)) {
			simple_unlock(&bqueue_slock);
			if (curproc == uvm.pagedaemon_proc) {
				simple_unlock(&bp->b_interlock);
				splx(s);
				return NULL;
			}
			SET(bp->b_flags, B_WANTED);
			err = ltsleep(bp, slpflag | (PRIBIO + 1) | PNORELOCK,
					"getblk", slptimeo, &bp->b_interlock);
			splx(s);
			if (err)
				return (NULL);
			goto start;
		}
#ifdef DIAGNOSTIC
		if (ISSET(bp->b_flags, B_DONE|B_DELWRI) &&
		    bp->b_bcount < size && vp->v_type != VBLK)
			panic("getblk: block size invariant failed");
#endif
		SET(bp->b_flags, B_BUSY);
		bremfree(bp);
	} else {
		if ((bp = getnewbuf(slpflag, slptimeo)) == NULL) {
			simple_unlock(&bqueue_slock);
			splx(s);
			goto start;
		}

		binshash(bp, BUFHASH(vp, blkno));
		bp->b_blkno = bp->b_lblkno = bp->b_rawblkno = blkno;
		bgetvp(vp, bp);
	}
	simple_unlock(&bp->b_interlock);
	simple_unlock(&bqueue_slock);
	splx(s);
	allocbuf(bp, size);
	return (bp);
}

/*
 * Get an empty, disassociated buffer of given size.
 */
struct buf *
geteblk(size)
	int size;
{
	struct buf *bp; 
	int s;

	s = splbio();
	simple_lock(&bqueue_slock);
	while ((bp = getnewbuf(0, 0)) == 0)
		;

	SET(bp->b_flags, B_INVAL);
	binshash(bp, &invalhash);
	simple_unlock(&bqueue_slock);
	simple_unlock(&bp->b_interlock);
	splx(s);
	allocbuf(bp, size);
	return (bp);
}

/*
 * Expand or contract the actual memory allocated to a buffer.
 *
 * If the buffer shrinks, data is lost, so it's up to the
 * caller to have written it out *first*; this routine will not
 * start a write.  If the buffer grows, it's the callers
 * responsibility to fill out the buffer's additional contents.
 */
void
allocbuf(bp, size)
	struct buf *bp;
	int size;
{
	struct buf *nbp;
	vsize_t desired_size;
	int s;

	desired_size = round_page((vsize_t)size);
	if (desired_size > MAXBSIZE)
		panic("allocbuf: buffer larger than MAXBSIZE requested");

	if (bp->b_bufsize == desired_size)
		goto out;

	/*
	 * If the buffer is smaller than the desired size, we need to snarf
	 * it from other buffers.  Get buffers (via getnewbuf()), and
	 * steal their pages.
	 */
	while (bp->b_bufsize < desired_size) {
		int amt;

		/* find a buffer */
		s = splbio();
		simple_lock(&bqueue_slock);
		while ((nbp = getnewbuf(0, 0)) == NULL)
			;

		SET(nbp->b_flags, B_INVAL);
		binshash(nbp, &invalhash);

		simple_unlock(&nbp->b_interlock);
		simple_unlock(&bqueue_slock);
		splx(s);

		/* and steal its pages, up to the amount we need */
		amt = min(nbp->b_bufsize, (desired_size - bp->b_bufsize));
		pagemove((nbp->b_data + nbp->b_bufsize - amt),
			 bp->b_data + bp->b_bufsize, amt);
		bp->b_bufsize += amt;
		nbp->b_bufsize -= amt;

		/* reduce transfer count if we stole some data */
		if (nbp->b_bcount > nbp->b_bufsize)
			nbp->b_bcount = nbp->b_bufsize;

#ifdef DIAGNOSTIC
		if (nbp->b_bufsize < 0)
			panic("allocbuf: negative bufsize");
#endif
		brelse(nbp);
	}

	/*
	 * If we want a buffer smaller than the current size,
	 * shrink this buffer.  Grab a buf head from the EMPTY queue,
	 * move a page onto it, and put it on front of the AGE queue.
	 * If there are no free buffer headers, leave the buffer alone.
	 */
	if (bp->b_bufsize > desired_size) {
		s = splbio();
		simple_lock(&bqueue_slock);
		if ((nbp = TAILQ_FIRST(&bufqueues[BQ_EMPTY])) == NULL) {
			/* No free buffer head */
			simple_unlock(&bqueue_slock);
			splx(s);
			goto out;
		}
		/* No need to lock nbp since it came from the empty queue */
		bremfree(nbp);
		SET(nbp->b_flags, B_BUSY | B_INVAL);
		simple_unlock(&bqueue_slock);
		splx(s);

		/* move the page to it and note this change */
		pagemove(bp->b_data + desired_size,
		    nbp->b_data, bp->b_bufsize - desired_size);
		nbp->b_bufsize = bp->b_bufsize - desired_size;
		bp->b_bufsize = desired_size;
		nbp->b_bcount = 0;

		/* release the newly-filled buffer and leave */
		brelse(nbp);
	}

out:
	bp->b_bcount = size;
}

/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.    
 *
 * Called with buffer queues locked.
 * Return buffer locked.
 */
struct buf *
getnewbuf(slpflag, slptimeo)
	int slpflag, slptimeo;
{
	struct buf *bp;

start:
	LOCK_ASSERT(simple_lock_held(&bqueue_slock));

	if ((bp = TAILQ_FIRST(&bufqueues[BQ_AGE])) != NULL ||
	    (bp = TAILQ_FIRST(&bufqueues[BQ_LRU])) != NULL) {
		simple_lock(&bp->b_interlock);
		bremfree(bp);
	} else {
		/* wait for a free buffer of any kind */
		needbuffer = 1;
		ltsleep(&needbuffer, slpflag|(PRIBIO+1),
			"getnewbuf", slptimeo, &bqueue_slock);
		return (NULL);
	}

	if (ISSET(bp->b_flags, B_VFLUSH)) {
		/*
		 * This is a delayed write buffer being flushed to disk.  Make
		 * sure it gets aged out of the queue when it's finished, and
		 * leave it off the LRU queue.
		 */
		CLR(bp->b_flags, B_VFLUSH);
		SET(bp->b_flags, B_AGE);
		simple_unlock(&bp->b_interlock);
		goto start;
	}

	/* Buffer is no longer on free lists. */
	SET(bp->b_flags, B_BUSY);

	/*
	 * If buffer was a delayed write, start it and return NULL
	 * (since we might sleep while starting the write).
	 */
	if (ISSET(bp->b_flags, B_DELWRI)) {
		/*
		 * This buffer has gone through the LRU, so make sure it gets
		 * reused ASAP.
		 */
		SET(bp->b_flags, B_AGE);
		simple_unlock(&bp->b_interlock);
		simple_unlock(&bqueue_slock);
		bawrite(bp);
		simple_lock(&bqueue_slock);
		return (NULL);
	}

	/* disassociate us from our vnode, if we had one... */
	if (bp->b_vp)
		brelvp(bp);

	if (LIST_FIRST(&bp->b_dep) != NULL && bioops.io_deallocate)
		(*bioops.io_deallocate)(bp);

	/* clear out various other fields */
	bp->b_flags = B_BUSY;
	bp->b_dev = NODEV;
	bp->b_blkno = bp->b_lblkno = bp->b_rawblkno = 0;
	bp->b_iodone = 0;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	
	bremhash(bp);
	return (bp); 
}

/*
 * Wait for operations on the buffer to complete.
 * When they do, extract and return the I/O's error value.
 */
int
biowait(bp)
	struct buf *bp;
{
	int s, error;
	
	s = splbio();
	simple_lock(&bp->b_interlock);
	while (!ISSET(bp->b_flags, B_DONE | B_DELWRI))
		ltsleep(bp, PRIBIO + 1, "biowait", 0, &bp->b_interlock);

	/* check for interruption of I/O (e.g. via NFS), then errors. */
	if (ISSET(bp->b_flags, B_EINTR)) {
		CLR(bp->b_flags, B_EINTR);
		error = EINTR;
	} else if (ISSET(bp->b_flags, B_ERROR))
		error = bp->b_error ? bp->b_error : EIO;
	else
		error = 0;

	simple_unlock(&bp->b_interlock);
	splx(s);
	return (error);
}

/*
 * Mark I/O complete on a buffer.
 *
 * If a callback has been requested, e.g. the pageout
 * daemon, do so. Otherwise, awaken waiting processes.
 *
 * [ Leffler, et al., says on p.247:
 *	"This routine wakes up the blocked process, frees the buffer
 *	for an asynchronous write, or, for a request by the pagedaemon
 *	process, invokes a procedure specified in the buffer structure" ]
 *
 * In real life, the pagedaemon (or other system processes) wants
 * to do async stuff to, and doesn't want the buffer brelse()'d.
 * (for swap pager, that puts swap buffers on the free lists (!!!),
 * for the vn device, that puts malloc'd buffers on the free lists!)
 */
void
biodone(bp)
	struct buf *bp;
{
	int s = splbio();

	simple_lock(&bp->b_interlock);
	if (ISSET(bp->b_flags, B_DONE))
		panic("biodone already");
	SET(bp->b_flags, B_DONE);		/* note that it's done */

	if (LIST_FIRST(&bp->b_dep) != NULL && bioops.io_complete)
		(*bioops.io_complete)(bp);

	if (!ISSET(bp->b_flags, B_READ))	/* wake up reader */
		vwakeup(bp);

	/*
	 * If necessary, call out.  Unlock the buffer before calling
	 * iodone() as the buffer isn't valid any more when it return.
	 */
	if (ISSET(bp->b_flags, B_CALL)) {
		CLR(bp->b_flags, B_CALL);	/* but note callout done */
		simple_unlock(&bp->b_interlock);
		(*bp->b_iodone)(bp);
	} else {
		if (ISSET(bp->b_flags, B_ASYNC)) {	/* if async, release */
			simple_unlock(&bp->b_interlock);
			brelse(bp);
		} else {			/* or just wakeup the buffer */
			CLR(bp->b_flags, B_WANTED);
			wakeup(bp);
			simple_unlock(&bp->b_interlock);
		}
	}

	splx(s);
}

/*
 * Return a count of buffers on the "locked" queue.
 */
int
count_lock_queue()
{
	struct buf *bp;
	int n = 0;

	simple_lock(&bqueue_slock);
	TAILQ_FOREACH(bp, &bufqueues[BQ_LOCKED], b_freelist)
		n++;
	simple_unlock(&bqueue_slock);
	return (n);
}

#ifdef DEBUG
/*
 * Print out statistics on the current allocation of the buffer pool.
 * Can be enabled to print out on every ``sync'' by setting "syncprt"
 * in vfs_syscalls.c using sysctl.
 */
void
vfs_bufstats()
{
	int s, i, j, count;
	struct buf *bp;
	struct bqueues *dp;
	int counts[(MAXBSIZE / PAGE_SIZE) + 1];
	static char *bname[BQUEUES] = { "LOCKED", "LRU", "AGE", "EMPTY" };

	for (dp = bufqueues, i = 0; dp < &bufqueues[BQUEUES]; dp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			counts[j] = 0;
		s = splbio();
		TAILQ_FOREACH(bp, dp, b_freelist) {
			counts[bp->b_bufsize/PAGE_SIZE]++;
			count++;
		}
		splx(s);
		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * PAGE_SIZE, counts[j]);
		printf("\n");
	}
}
#endif /* DEBUG */
