/*	$NetBSD: vfs_bio.c,v 1.170.2.12 2007/07/15 22:20:28 ad Exp $	*/

/*-
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
 *	@(#)vfs_bio.c	8.6 (Berkeley) 1/11/94
 */

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
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

#include "fs_ffs.h"
#include "opt_bufcache.h"
#include "opt_softdep.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_bio.c,v 1.170.2.12 2007/07/15 22:20:28 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <miscfs/specfs/specdev.h>

#ifndef	BUFPAGES
# define BUFPAGES 0
#endif

#ifdef BUFCACHE
# if (BUFCACHE < 5) || (BUFCACHE > 95)
#  error BUFCACHE is not between 5 and 95
# endif
#else
# define BUFCACHE 15
#endif

u_int	nbuf;			/* XXX - for softdep_lockedbufs */
u_int	bufpages = BUFPAGES;	/* optional hardwired count */
u_int	bufcache = BUFCACHE;	/* max % of RAM to use for buffer cache */

/* Function prototypes */
struct bqueue;

static void buf_setwm(void);
static int buf_trim(void);
static void *bufpool_page_alloc(struct pool *, int);
static void bufpool_page_free(struct pool *, void *);
static inline struct buf *bio_doread(struct vnode *, daddr_t, int,
    kauth_cred_t, int);
static struct buf *getnewbuf(int, int, int);
static int buf_lotsfree(void);
static int buf_canrelease(void);
static inline u_long buf_mempoolidx(u_long);
static inline u_long buf_roundsize(u_long);
static inline void *buf_malloc(size_t);
static void buf_mrelease(void *, size_t);
static inline void binsheadfree(struct buf *, struct bqueue *);
static inline void binstailfree(struct buf *, struct bqueue *);
int count_lock_queue(void); /* XXX */
#ifdef DEBUG
static int checkfreelist(struct buf *, struct bqueue *);
#endif
static void biointr(void *);
static void biodone2(struct buf *);

/*
 * Definitions for the buffer hash lists.
 */
#define	BUFHASH(dvp, lbn)	\
	(&bufhashtbl[(((long)(dvp) >> 8) + (int)(lbn)) & bufhash])
LIST_HEAD(bufhashhdr, buf) *bufhashtbl, invalhash;
u_long	bufhash;
#if !defined(SOFTDEP) || !defined(FFS)
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
#define	BQUEUES		3		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */

struct bqueue {
	TAILQ_HEAD(, buf) bq_queue;
	uint64_t bq_bytes;
	struct buf *bq_marker;
} bufqueues[BQUEUES];
kcondvar_t needbuffer_cv;

/*
 * Buffer queue lock.
 * Take this lock first if also taking some buffer's b_interlock.
 */
kmutex_t bqueue_lock;

/* Software ISR for completed transfers. */
void *biodone_sih;

/* Buffer pool for I/O buffers. */
static POOL_INIT(bufpool, sizeof(struct buf), 0, 0, 0, "bufpl",
    &pool_allocator_nointr, IPL_NONE);


/* XXX - somewhat gross.. */
#if MAXBSIZE == 0x2000
#define NMEMPOOLS 5
#elif MAXBSIZE == 0x4000
#define NMEMPOOLS 6
#elif MAXBSIZE == 0x8000
#define NMEMPOOLS 7
#else
#define NMEMPOOLS 8
#endif

#define MEMPOOL_INDEX_OFFSET 9	/* smallest pool is 512 bytes */
#if (1 << (NMEMPOOLS + MEMPOOL_INDEX_OFFSET - 1)) != MAXBSIZE
#error update vfs_bio buffer memory parameters
#endif

/* Buffer memory pools */
static struct pool bmempools[NMEMPOOLS];

struct vm_map *buf_map;

/*
 * Buffer memory pool allocator.
 */
static void *
bufpool_page_alloc(struct pool *pp, int flags)
{

	return (void *)uvm_km_alloc(buf_map,
	    MAXBSIZE, MAXBSIZE,
	    ((flags & PR_WAITOK) ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK)
	    | UVM_KMF_WIRED);
}

static void
bufpool_page_free(struct pool *pp, void *v)
{

	uvm_km_free(buf_map, (vaddr_t)v, MAXBSIZE, UVM_KMF_WIRED);
}

static struct pool_allocator bufmempool_allocator = {
	.pa_alloc = bufpool_page_alloc,
	.pa_free = bufpool_page_free,
	.pa_pagesz = MAXBSIZE,
};

/* Buffer memory management variables */
u_long bufmem_valimit;
u_long bufmem_hiwater;
u_long bufmem_lowater;
u_long bufmem;

/*
 * MD code can call this to set a hard limit on the amount
 * of virtual memory used by the buffer cache.
 */
int
buf_setvalimit(vsize_t sz)
{

	/* We need to accommodate at least NMEMPOOLS of MAXBSIZE each */
	if (sz < NMEMPOOLS * MAXBSIZE)
		return EINVAL;

	bufmem_valimit = sz;
	return 0;
}

static void
buf_setwm(void)
{

	bufmem_hiwater = buf_memcalc();
	/* lowater is approx. 2% of memory (with bufcache = 15) */
#define	BUFMEM_WMSHIFT	3
#define	BUFMEM_HIWMMIN	(64 * 1024 << BUFMEM_WMSHIFT)
	if (bufmem_hiwater < BUFMEM_HIWMMIN)
		/* Ensure a reasonable minimum value */
		bufmem_hiwater = BUFMEM_HIWMMIN;
	bufmem_lowater = bufmem_hiwater >> BUFMEM_WMSHIFT;
}

#ifdef DEBUG
int debug_verify_freelist = 0;
static int
checkfreelist(struct buf *bp, struct bqueue *dp)
{
	struct buf *b;

	if (!debug_verify_freelist)
		return 1;

	TAILQ_FOREACH(b, &dp->bq_queue, b_freelist) {
		if (b == bp)
			return 1;
	}

	return 0;
}
#endif

/*
 * Insq/Remq for the buffer hash lists.
 * Call with buffer queue locked.
 */
static inline void
binsheadfree(struct buf *bp, struct bqueue *dp)
{

	KASSERT(bp->b_freelistindex == -1);
	TAILQ_INSERT_HEAD(&dp->bq_queue, bp, b_freelist);
	dp->bq_bytes += bp->b_bufsize;
	bp->b_freelistindex = dp - bufqueues;
}

static inline void
binstailfree(struct buf *bp, struct bqueue *dp)
{

	KASSERT(bp->b_freelistindex == -1);
	TAILQ_INSERT_TAIL(&dp->bq_queue, bp, b_freelist);
	dp->bq_bytes += bp->b_bufsize;
	bp->b_freelistindex = dp - bufqueues;
}

void
bremfree(struct buf *bp)
{
	struct bqueue *dp;
	int bqidx = bp->b_freelistindex;

	KASSERT(mutex_owned(&bqueue_lock));

	KASSERT(bqidx != -1);
	dp = &bufqueues[bqidx];
	KDASSERT(checkfreelist(bp, dp));
	KASSERT(dp->bq_bytes >= bp->b_bufsize);
	TAILQ_REMOVE(&dp->bq_queue, bp, b_freelist);
	dp->bq_bytes -= bp->b_bufsize;

	/* For the sysctl helper. */
	if (bp == dp->bq_marker)
		dp->bq_marker = NULL;

#if defined(DIAGNOSTIC)
	bp->b_freelistindex = -1;
#endif /* defined(DIAGNOSTIC) */
}

u_long
buf_memcalc(void)
{
	u_long n;

	/*
	 * Determine the upper bound of memory to use for buffers.
	 *
	 *	- If bufpages is specified, use that as the number
	 *	  pages.
	 *
	 *	- Otherwise, use bufcache as the percentage of
	 *	  physical memory.
	 */
	if (bufpages != 0) {
		n = bufpages;
	} else {
		if (bufcache < 5) {
			printf("forcing bufcache %d -> 5", bufcache);
			bufcache = 5;
		}
		if (bufcache > 95) {
			printf("forcing bufcache %d -> 95", bufcache);
			bufcache = 95;
		}
		n = physmem / 100 * bufcache;
	}

	n <<= PAGE_SHIFT;
	if (bufmem_valimit != 0 && n > bufmem_valimit)
		n = bufmem_valimit;

	return (n);
}

/*
 * Initialize buffers and hash links for buffers.
 */
void
bufinit(void)
{
	struct bqueue *dp;
	int use_std;
	u_int i;

	mutex_init(&bqueue_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&needbuffer_cv, "needbuf");

	/*
	 * Initialize buffer cache memory parameters.
	 */
	bufmem = 0;
	buf_setwm();

	if (bufmem_valimit != 0) {
		vaddr_t minaddr = 0, maxaddr;
		buf_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
					  bufmem_valimit, 0, false, 0);
		if (buf_map == NULL)
			panic("bufinit: cannot allocate submap");
	} else
		buf_map = kernel_map;

	/* On "small" machines use small pool page sizes where possible */
	use_std = (physmem < atop(16*1024*1024));

	/*
	 * Also use them on systems that can map the pool pages using
	 * a direct-mapped segment.
	 */
#ifdef PMAP_MAP_POOLPAGE
	use_std = 1;
#endif

	bufmempool_allocator.pa_backingmap = buf_map;
	for (i = 0; i < NMEMPOOLS; i++) {
		struct pool_allocator *pa;
		struct pool *pp = &bmempools[i];
		u_int size = 1 << (i + MEMPOOL_INDEX_OFFSET);
		char *name = malloc(8, M_TEMP, M_WAITOK);
		if (__predict_true(size >= 1024))
			(void)snprintf(name, 8, "buf%dk", size / 1024);
		else
			(void)snprintf(name, 8, "buf%db", size);
		pa = (size <= PAGE_SIZE && use_std)
			? &pool_allocator_nointr
			: &bufmempool_allocator;
		pool_init(pp, size, 0, 0, 0, name, pa, IPL_NONE);
		pool_setlowat(pp, 1);
		pool_sethiwat(pp, 1);
	}

	/* Initialize the buffer queues */
	for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++) {
		TAILQ_INIT(&dp->bq_queue);
		dp->bq_bytes = 0;
	}

	/*
	 * Estimate hash table size based on the amount of memory we
	 * intend to use for the buffer cache. The average buffer
	 * size is dependent on our clients (i.e. filesystems).
	 *
	 * For now, use an empirical 3K per buffer.
	 */
	nbuf = (bufmem_hiwater / 1024) / 3;
	bufhashtbl = hashinit(nbuf, HASH_LIST, M_CACHE, M_WAITOK, &bufhash);
}

void
bufinit2(void)
{

	biodone_sih = softint_establish(SOFTINT_BIO, biointr, NULL);
	if (biodone_sih == NULL)
		panic("bufinit2: can't establish soft interrupt");
}

static int
buf_lotsfree(void)
{
	int try, thresh;

	/* Always allocate if doing copy on write */
	if (curlwp->l_pflag & LP_UFSCOW)
		return 1;

	/* Always allocate if less than the low water mark. */
	if (bufmem < bufmem_lowater)
		return 1;

	/* Never allocate if greater than the high water mark. */
	if (bufmem > bufmem_hiwater)
		return 0;

	/* If there's anything on the AGE list, it should be eaten. */
	if (TAILQ_FIRST(&bufqueues[BQ_AGE].bq_queue) != NULL)
		return 0;

	/*
	 * The probabily of getting a new allocation is inversely
	 * proportional to the current size of the cache, using
	 * a granularity of 16 steps.
	 */
	try = random() & 0x0000000fL;

	/* Don't use "16 * bufmem" here to avoid a 32-bit overflow. */
	thresh = (bufmem - bufmem_lowater) /
	    ((bufmem_hiwater - bufmem_lowater) / 16);

	if (try >= thresh)
		return 1;

	/* Otherwise don't allocate. */
	return 0;
}

/*
 * Return estimate of bytes we think need to be
 * released to help resolve low memory conditions.
 *
 * => called with bqueue_lock held.
 */
static int
buf_canrelease(void)
{
	int pagedemand, ninvalid = 0;

	KASSERT(mutex_owned(&bqueue_lock));

	if (bufmem < bufmem_lowater)
		return 0;

	if (bufmem > bufmem_hiwater)
		return bufmem - bufmem_hiwater;

	ninvalid += bufqueues[BQ_AGE].bq_bytes;

	pagedemand = uvmexp.freetarg - uvmexp.free;
	if (pagedemand < 0)
		return ninvalid;
	return MAX(ninvalid, MIN(2 * MAXBSIZE,
	    MIN((bufmem - bufmem_lowater) / 16, pagedemand * PAGE_SIZE)));
}

/*
 * Buffer memory allocation helper functions
 */
static inline u_long
buf_mempoolidx(u_long size)
{
	u_int n = 0;

	size -= 1;
	size >>= MEMPOOL_INDEX_OFFSET;
	while (size) {
		size >>= 1;
		n += 1;
	}
	if (n >= NMEMPOOLS)
		panic("buf mem pool index %d", n);
	return n;
}

static inline u_long
buf_roundsize(u_long size)
{
	/* Round up to nearest power of 2 */
	return (1 << (buf_mempoolidx(size) + MEMPOOL_INDEX_OFFSET));
}

static inline void *
buf_malloc(size_t size)
{
	u_int n = buf_mempoolidx(size);
	void *addr;

	while (1) {
		addr = pool_get(&bmempools[n], PR_NOWAIT);
		if (addr != NULL)
			break;

		/* No memory, see if we can free some. If so, try again */
		if (buf_drain(1) > 0)
			continue;

		/* Wait for buffers to arrive on the LRU queue */
		mutex_enter(&bqueue_lock);
		cv_timedwait(&needbuffer_cv, &bqueue_lock, hz / 4);
		mutex_exit(&bqueue_lock);
	}

	return addr;
}

static void
buf_mrelease(void *addr, size_t size)
{

	pool_put(&bmempools[buf_mempoolidx(size)], addr);
}

/*
 * bread()/breadn() helper.
 */
static inline struct buf *
bio_doread(struct vnode *vp, daddr_t blkno, int size, kauth_cred_t cred,
    int async)
{
	struct buf *bp;
	struct mount *mp;

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
	mutex_enter(&bp->b_interlock);
	if (!ISSET(bp->b_flags, (B_DONE | B_DELWRI))) {
		/* Start I/O for the buffer. */
		SET(bp->b_flags, B_READ | async);
		if (async)
			BIO_SETPRIO(bp, BPRIO_TIMELIMITED);
		else
			BIO_SETPRIO(bp, BPRIO_TIMECRITICAL);
		mutex_exit(&bp->b_interlock);
		VOP_STRATEGY(vp, bp);

		/* Pay for the read. */
		curproc->p_stats->p_ru.ru_inblock++;
	} else {
		mutex_exit(&bp->b_interlock);
		if (async)
			brelse(bp, 0);
	}

	if (vp->v_type == VBLK)
		mp = vp->v_specmountpoint;
	else
		mp = vp->v_mount;

	/*
	 * Collect statistics on synchronous and asynchronous reads.
	 * Reads from block devices are charged to their associated
	 * filesystem (if any).
	 */
	if (mp != NULL) {
		if (async == 0)
			mp->mnt_stat.f_syncreads++;
		else
			mp->mnt_stat.f_asyncreads++;
	}

	return (bp);
}

/*
 * Read a disk block.
 * This algorithm described in Bach (p.54).
 */
int
bread(struct vnode *vp, daddr_t blkno, int size, kauth_cred_t cred,
    struct buf **bpp)
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
breadn(struct vnode *vp, daddr_t blkno, int size, daddr_t *rablks,
    int *rasizes, int nrablks, kauth_cred_t cred, struct buf **bpp)
{
	struct buf *bp;
	int i;

	bp = *bpp = bio_doread(vp, blkno, size, cred, 0);

	/*
	 * For each of the read-ahead blocks, start a read, if necessary.
	 */
	mutex_enter(&bqueue_lock);
	for (i = 0; i < nrablks; i++) {
		/* If it's in the cache, just go on to next one. */
		if (incore(vp, rablks[i]))
			continue;

		/* Get a buffer for the read-ahead block */
		mutex_exit(&bqueue_lock);
		(void) bio_doread(vp, rablks[i], rasizes[i], cred, B_ASYNC);
		mutex_enter(&bqueue_lock);
	}
	mutex_exit(&bqueue_lock);

	/* Otherwise, we had to start a read for it; wait until it's valid. */
	return (biowait(bp));
}

/*
 * Read with single-block read-ahead.  Defined in Bach (p.55), but
 * implemented as a call to breadn().
 * XXX for compatibility with old file systems.
 */
int
breada(struct vnode *vp, daddr_t blkno, int size, daddr_t rablkno,
    int rabsize, kauth_cred_t cred, struct buf **bpp)
{

	return (breadn(vp, blkno, size, &rablkno, &rabsize, 1, cred, bpp));
}

/*
 * Block write.  Described in Bach (p.56)
 */
int
bwrite(struct buf *bp)
{
	int rv, sync, wasdelayed;
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

	mutex_enter(&bp->b_interlock);

	wasdelayed = ISSET(bp->b_flags, B_DELWRI);

	CLR(bp->b_flags, (B_READ | B_DONE | B_ERROR | B_DELWRI));

	mutex_exit(&bp->b_interlock);

	/*
	 * Pay for the I/O operation and make sure the buf is on the correct
	 * vnode queue.
	 */
	mutex_enter(&vp->v_interlock);
	if (wasdelayed)
		reassignbuf(bp, bp->b_vp);
	else
		curproc->p_stats->p_ru.ru_oublock++;
	vp->v_numoutput++;
	mutex_exit(&vp->v_interlock);

	/* Initiate disk write. */
	if (sync)
		BIO_SETPRIO(bp, BPRIO_TIMECRITICAL);
	else
		BIO_SETPRIO(bp, BPRIO_TIMELIMITED);

	VOP_STRATEGY(vp, bp);

	if (sync) {
		/* If I/O was synchronous, wait for it to complete. */
		rv = biowait(bp);

		/* Release the buffer. */
		brelse(bp, 0);

		return (rv);
	} else {
		return (0);
	}
}

int
vn_bwrite(void *v)
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
bdwrite(struct buf *bp)
{
	const struct bdevsw *bdev;
	struct vnode *vp;

	/* If this is a tape block, write the block now. */
	bdev = bdevsw_lookup(bp->b_dev);
	if (bdev != NULL && (bdev->d_flag & D_TYPEMASK) == D_TAPE) {
		bawrite(bp);
		return;
	}

	/*
	 * If the block hasn't been seen before:
	 *	(1) Mark it as having been seen,
	 *	(2) Charge for the write,
	 *	(3) Make sure it's on its vnode's correct block list.
	 */
	if ((vp = bp->b_vp) != NULL) {
		mutex_enter(&vp->v_interlock);
	}
	mutex_enter(&bp->b_interlock);

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
		curproc->p_stats->p_ru.ru_oublock++;
		reassignbuf(bp, vp);
	}

	/* Otherwise, the "write" is done, so mark and release the buffer. */
	CLR(bp->b_flags, B_DONE);
	mutex_exit(&bp->b_interlock);
	if (vp != NULL) {
		mutex_exit(&vp->v_interlock);
	}

	brelse(bp, 0);
}

/*
 * Asynchronous block write; just an asynchronous bwrite().
 */
void
bawrite(struct buf *bp)
{

	KASSERT(ISSET(bp->b_flags, B_BUSY));

	mutex_enter(&bp->b_interlock);
	SET(bp->b_flags, B_ASYNC);
	mutex_exit(&bp->b_interlock);
	VOP_BWRITE(bp);
}

/*
 * Same as first half of bdwrite, mark buffer dirty, but do not release it.
 * Call with the buffer interlock held.
 *
 * Note: called only from biodone() through ffs softdep's bioops.io_complete()
 */
void
bdirty(struct buf *bp)
{

	KASSERT(mutex_owned(&bp->b_interlock));
	KASSERT(mutex_owned(&bp->b_vp->v_interlock));
	KASSERT(ISSET(bp->b_flags, B_BUSY));

	CLR(bp->b_flags, B_AGE);

	if (!ISSET(bp->b_flags, B_DELWRI)) {
		SET(bp->b_flags, B_DELWRI);
		curproc->p_stats->p_ru.ru_oublock++;
		reassignbuf(bp, bp->b_vp);
	}
}

/*
 * Release a buffer on to the free lists.
 * Described in Bach (p. 46).
 */
void
brelse(struct buf *bp, int set)
{
	struct bqueue *bufq;
	struct vnode *vp;

	vp = NULL;

	mutex_enter(&bqueue_lock);
	mutex_enter(&bp->b_interlock);

	SET(bp->b_flags, set);

	KASSERT(ISSET(bp->b_flags, B_BUSY));
	KASSERT(!ISSET(bp->b_flags, B_CALL));

	/* Wake up any processes waiting for any buffer to become free. */
	cv_signal(&needbuffer_cv);

	/* Wake up any proceeses waiting for _this_ buffer to become free. */
	if (ISSET(bp->b_flags, B_WANTED)) {
		CLR(bp->b_flags, B_WANTED|B_AGE);
		cv_broadcast(&bp->b_cv);
	}

	/*
	 * Determine which queue the buffer should be on, then put it there.
	 */

	/* If it's locked, don't report an error; try again later. */
	if (ISSET(bp->b_flags, (B_LOCKED|B_ERROR)) == (B_LOCKED|B_ERROR)) {
		CLR(bp->b_flags, B_ERROR);
		bp->b_error = 0;
	}

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
		if (!ISSET(bp->b_flags, B_ERROR|B_INVAL|B_LOCKED|B_AGE)) {
			KDASSERT(checkfreelist(bp, &bufqueues[BQ_LRU]));
			goto already_queued;
		} else {
			bremfree(bp);
		}
	}

	KDASSERT(checkfreelist(bp, &bufqueues[BQ_AGE]));
	KDASSERT(checkfreelist(bp, &bufqueues[BQ_LRU]));
	KDASSERT(checkfreelist(bp, &bufqueues[BQ_LOCKED]));

	if ((bp->b_bufsize <= 0) || ISSET(bp->b_flags, B_INVAL)) {
		/*
		 * If it's invalid or empty, dissociate it from its vnode
		 * and put on the head of the appropriate queue.
		 */
		if (LIST_FIRST(&bp->b_dep) != NULL && bioops.io_deallocate)
			(*bioops.io_deallocate)(bp);
		CLR(bp->b_flags, B_DONE|B_DELWRI);
		if (bp->b_vp)
			vp = bp->b_vp;
		if (bp->b_bufsize <= 0)
			/* no data */
			goto already_queued;
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
	mutex_exit(&bp->b_interlock);
	if (vp != NULL) {
		mutex_enter(&vp->v_interlock);
		reassignbuf(bp, bp->b_vp);
		brelvp(bp);
		mutex_exit(&vp->v_interlock);
	}
	mutex_exit(&bqueue_lock);
	if (bp->b_bufsize <= 0) {
		buf_destroy(bp);
#ifdef DEBUG
		memset((char *)bp, 0, sizeof(*bp));
#endif
		pool_put(&bufpool, bp);
	}

	if (vp != NULL)
		HOLDRELE(vp);
}

/*
 * Determine if a block is in the cache.
 * Just look on what would be its hash chain.  If it's there, return
 * a pointer to it, unless it's marked invalid.  If it's marked invalid,
 * we normally don't return the buffer, unless the caller explicitly
 * wants us to.
 */
struct buf *
incore(struct vnode *vp, daddr_t blkno)
{
	struct buf *bp;

	KASSERT(mutex_owned(&bqueue_lock));

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
getblk(struct vnode *vp, daddr_t blkno, int size, int slpflag, int slptimeo)
{
	struct buf *bp;
	int err;
	int preserve;

	VHOLD(vp);
start:
	mutex_enter(&bqueue_lock);
	bp = incore(vp, blkno);
	if (bp != NULL) {
		mutex_enter(&bp->b_interlock);
		if (ISSET(bp->b_flags, B_BUSY)) {
			mutex_exit(&bqueue_lock);
			if (curlwp == uvm.pagedaemon_lwp) {
				mutex_exit(&bp->b_interlock);
				return NULL;
			}
			SET(bp->b_flags, B_WANTED);
			if ((slpflag & PCATCH) != 0)
				err = cv_timedwait_sig(&bp->b_cv,
				    &bp->b_interlock, slptimeo);
			else
				err = cv_timedwait(&bp->b_cv,
				    &bp->b_interlock, slptimeo);
			mutex_exit(&bp->b_interlock);
			if (err) {
				HOLDRELE(vp);
				return (NULL);
			}
			goto start;
		}
#ifdef DIAGNOSTIC
		if (ISSET(bp->b_flags, B_DONE|B_DELWRI) &&
		    bp->b_bcount < size && vp->v_type != VBLK)
			panic("getblk: block size invariant failed");
#endif
		SET(bp->b_flags, B_BUSY);
		bremfree(bp);
		preserve = 1;
		mutex_exit(&bp->b_interlock);
	} else {
		if ((bp = getnewbuf(slpflag, slptimeo, 0)) == NULL) {
			mutex_exit(&bqueue_lock);
			goto start;
		}

		if (incore(vp, blkno) != NULL) {
			/* The block has come into memory in the meantime. */
			mutex_exit(&bqueue_lock);
			brelse(bp, 0);
			mutex_enter(&bqueue_lock);
			goto start;		
		}

		binshash(bp, BUFHASH(vp, blkno));
		bp->b_blkno = bp->b_lblkno = bp->b_rawblkno = blkno;
		mutex_exit(&bp->b_interlock);
		mutex_enter(&vp->v_interlock);
		bgetvp(vp, bp);
		mutex_exit(&vp->v_interlock);
		preserve = 0;
	}
	mutex_exit(&bqueue_lock);

	if (preserve)
		HOLDRELE(vp);

	/*
	 * LFS can't track total size of B_LOCKED buffer (locked_queue_bytes)
	 * if we re-size buffers here.
	 */
	if (ISSET(bp->b_flags, B_LOCKED)) {
		KASSERT(bp->b_bufsize >= size);
	} else {
		allocbuf(bp, size, preserve);
	}
	BIO_SETPRIO(bp, BPRIO_DEFAULT);
	return (bp);
}

/*
 * Get an empty, disassociated buffer of given size.
 */
struct buf *
geteblk(int size)
{
	struct buf *bp;

	mutex_enter(&bqueue_lock);
	while ((bp = getnewbuf(0, 0, 0)) == 0)
		;

	SET(bp->b_flags, B_INVAL);
	binshash(bp, &invalhash);
	mutex_exit(&bqueue_lock);
	mutex_exit(&bp->b_interlock);
	BIO_SETPRIO(bp, BPRIO_DEFAULT);
	allocbuf(bp, size, 0);
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
allocbuf(struct buf *bp, int size, int preserve)
{
	vsize_t oldsize, desired_size;
	void *addr;
	int delta;

	desired_size = buf_roundsize(size);
	if (desired_size > MAXBSIZE)
		printf("allocbuf: buffer larger than MAXBSIZE requested");

	bp->b_bcount = size;

	oldsize = bp->b_bufsize;
	if (oldsize == desired_size)
		return;

	/*
	 * If we want a buffer of a different size, re-allocate the
	 * buffer's memory; copy old content only if needed.
	 */
	addr = buf_malloc(desired_size);
	if (preserve)
		memcpy(addr, bp->b_data, MIN(oldsize,desired_size));
	if (bp->b_data != NULL)
		buf_mrelease(bp->b_data, oldsize);
	bp->b_data = addr;
	bp->b_bufsize = desired_size;

	/*
	 * Update overall buffer memory counter (protected by bqueue_lock)
	 */
	delta = (long)desired_size - (long)oldsize;

	mutex_enter(&bqueue_lock);
	if ((bufmem += delta) > bufmem_hiwater) {
		/*
		 * Need to trim overall memory usage.
		 */
		while (buf_canrelease()) {
			if (curcpu()->ci_schedstate.spc_flags &
			    SPCF_SHOULDYIELD) {
				mutex_exit(&bqueue_lock);
				preempt();
				mutex_enter(&bqueue_lock);
			}

			if (buf_trim() == 0)
				break;
		}
	}

	mutex_exit(&bqueue_lock);
}

/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.
 *
 * Called with the buffer queues locked.
 * Return buffer locked.
 */
struct buf *
getnewbuf(int slpflag, int slptimeo, int from_bufq)
{
	struct buf *bp;
	struct vnode *vp;

 start:
	KASSERT(mutex_owned(&bqueue_lock));

	/*
	 * Get a new buffer from the pool.
	 */
	if (!from_bufq && buf_lotsfree()) {
		mutex_exit(&bqueue_lock);
		bp = pool_get(&bufpool, PR_NOWAIT);
		if (bp != NULL) {
			memset((char *)bp, 0, sizeof(*bp));
			buf_init(bp);
			bp->b_dev = NODEV;
			bp->b_vnbufs.le_next = NOLIST;
			bp->b_flags = B_BUSY;
			mutex_enter(&bqueue_lock);
			mutex_enter(&bp->b_interlock);
#if defined(DIAGNOSTIC)
			bp->b_freelistindex = -1;
#endif /* defined(DIAGNOSTIC) */
			return (bp);
		}
		mutex_enter(&bqueue_lock);
	}

	if ((bp = TAILQ_FIRST(&bufqueues[BQ_AGE].bq_queue)) != NULL ||
	    (bp = TAILQ_FIRST(&bufqueues[BQ_LRU].bq_queue)) != NULL) {
		mutex_enter(&bp->b_interlock);
		bremfree(bp);
	} else {
		/*
		 * XXX: !from_bufq should be removed.
		 */
		if (!from_bufq || curlwp != uvm.pagedaemon_lwp) {
			/* wait for a free buffer of any kind */
			if ((slpflag & PCATCH) != 0)
				(void)cv_timedwait_sig(&needbuffer_cv,
				    &bqueue_lock, slptimeo);
			else
				(void)cv_timedwait(&needbuffer_cv,
				    &bqueue_lock, slptimeo);
		}
		return (NULL);
	}

#ifdef DIAGNOSTIC
	if (bp->b_bufsize <= 0)
		panic("buffer %p: on queue but empty", bp);
#endif

	if (ISSET(bp->b_flags, B_VFLUSH)) {
		/*
		 * This is a delayed write buffer being flushed to disk.  Make
		 * sure it gets aged out of the queue when it's finished, and
		 * leave it off the LRU queue.
		 */
		CLR(bp->b_flags, B_VFLUSH);
		SET(bp->b_flags, B_AGE);
		mutex_exit(&bp->b_interlock);
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
		mutex_exit(&bp->b_interlock);
		mutex_exit(&bqueue_lock);
		bawrite(bp);
		mutex_enter(&bqueue_lock);
		return (NULL);
	}

	vp = bp->b_vp;

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

	/* disassociate us from our vnode, if we had one... */
	if (vp != NULL) {
		mutex_exit(&bp->b_interlock);
		mutex_exit(&bqueue_lock);

		mutex_enter(&vp->v_interlock);
		brelvp(bp);
		holdrelel(vp);
		mutex_exit(&vp->v_interlock);

		mutex_enter(&bqueue_lock);
		mutex_enter(&bp->b_interlock);
	}

	return (bp);
}

/*
 * Attempt to free an aged buffer off the queues.
 * Called with queue lock held.
 * Returns the amount of buffer memory freed.
 */
static int
buf_trim(void)
{
	struct buf *bp;
	long size = 0;

	/* Instruct getnewbuf() to get buffers off the queues */
	if ((bp = getnewbuf(PCATCH, 1, 1)) == NULL)
		return 0;

	KASSERT(!ISSET(bp->b_flags, B_WANTED));
	mutex_exit(&bp->b_interlock);
	size = bp->b_bufsize;
	bufmem -= size;
	mutex_exit(&bqueue_lock);
	if (size > 0) {
		buf_mrelease(bp->b_data, size);
		bp->b_bcount = bp->b_bufsize = 0;
	}
	/* brelse() will return the buffer to the global buffer pool */
	brelse(bp, 0);
	mutex_enter(&bqueue_lock);
	return size;
}

int
buf_drain(int n)
{
	int size = 0, sz;

	mutex_enter(&bqueue_lock);

	while (size < n && bufmem > bufmem_lowater) {
		sz = buf_trim();
		if (sz <= 0)
			break;
		size += sz;
	}

	mutex_exit(&bqueue_lock);
	return size;
}

/*
 * Wait for operations on the buffer to complete.
 * When they do, extract and return the I/O's error value.
 */
int
biowait(struct buf *bp)
{
	int error;

	mutex_enter(&bp->b_interlock);
	while (!ISSET(bp->b_flags, B_DONE | B_DELWRI))
		cv_wait(&bp->b_cv, &bp->b_interlock);

	/* check errors. */
	if (ISSET(bp->b_flags, B_ERROR)) {
		KASSERT(bp->b_error != 0);
		error = bp->b_error;
	} else {
		KASSERT(bp->b_error == 0);
		error = 0;
	}

	mutex_exit(&bp->b_interlock);
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
biodone(struct buf *bp, int error, int resid)
{
	int s;

	KASSERT(!ISSET(bp->b_flags, B_DONE));

	bp->b_error = error;
	if (error)
		bp->b_resid = bp->b_bcount;
	else
		bp->b_resid = resid;

	if (cpu_intr_p()) {
		/* From interrupt mode: defer to a soft interrupt. */
		s = splvm();
		TAILQ_INSERT_TAIL(&curcpu()->ci_data.cpu_biodone, bp, b_actq);
		softint_schedule(biodone_sih);
		splx(s);
	} else {
		/* Process now - the buffer may be freed soon. */
		biodone2(bp);
	}
}

static void
biodone2(struct buf *bp)
{

	mutex_enter(&bp->b_interlock);
	if (bp->b_error)
		SET(bp->b_flags, B_ERROR);
	else
		CLR(bp->b_flags, B_ERROR);
	if (ISSET(bp->b_flags, B_DONE))
		panic("biodone2 already");
	SET(bp->b_flags, B_DONE);	/* note that it's done */
	BIO_SETPRIO(bp, BPRIO_DEFAULT);

	if (LIST_FIRST(&bp->b_dep) != NULL && bioops.io_complete) {
		mutex_exit(&bp->b_interlock);
		(*bioops.io_complete)(bp);
		mutex_enter(&bp->b_interlock);
	}

	/*
	 * If necessary, call out.  Unlock the buffer before
	 * calling iodone() as the buffer isn't valid any more
	 * when it return.
	 */
	if (ISSET(bp->b_flags, B_CALL)) {
		/* But note callout done */
		CLR(bp->b_flags, B_CALL);
		mutex_exit(&bp->b_interlock);
		if (!ISSET(bp->b_flags, B_READ))/* wake up writer */
			vwakeup(bp);
		(*bp->b_iodone)(bp);
	} else if (ISSET(bp->b_flags, B_ASYNC)) {
		/* If async, release */
		mutex_exit(&bp->b_interlock);
		if (!ISSET(bp->b_flags, B_READ))/* wake up writer */
			vwakeup(bp);
		brelse(bp, 0);
	} else {
		/* Or just wakeup the buffer */
		CLR(bp->b_flags, B_WANTED);
		cv_broadcast(&bp->b_cv);
		mutex_exit(&bp->b_interlock);
		if (!ISSET(bp->b_flags, B_READ))/* wake up writer */
			vwakeup(bp);
	}
}


static void
biointr(void *cookie)
{
	struct cpu_info *ci;
	struct buf *bp;
	int s;

	ci = curcpu();

	while (!TAILQ_EMPTY(&ci->ci_data.cpu_biodone)) {
		KASSERT(curcpu() == ci);

		s = splvm();
		bp = TAILQ_FIRST(&ci->ci_data.cpu_biodone);
		TAILQ_REMOVE(&ci->ci_data.cpu_biodone, bp, b_actq);
		splx(s);

		biodone2(bp);
	}
}

/*
 * Return a count of buffers on the "locked" queue.
 */
int
count_lock_queue(void)
{
	struct buf *bp;
	int n = 0;

	mutex_enter(&bqueue_lock);
	TAILQ_FOREACH(bp, &bufqueues[BQ_LOCKED].bq_queue, b_freelist)
		n++;
	mutex_exit(&bqueue_lock);
	return (n);
}

/*
 * Wait for all buffers to complete I/O
 * Return the number of "stuck" buffers.
 */
int
buf_syncwait(void)
{
	struct buf *bp;
	int iter, nbusy, nbusy_prev = 0, dcount, ihash;

	dcount = 10000;
	for (iter = 0; iter < 20;) {
		mutex_enter(&bqueue_lock);
		nbusy = 0;
		for (ihash = 0; ihash < bufhash+1; ihash++) {
		    LIST_FOREACH(bp, &bufhashtbl[ihash], b_hash) {
			if ((bp->b_flags & (B_BUSY|B_INVAL|B_READ)) == B_BUSY)
				nbusy++;
			/*
			 * With soft updates, some buffers that are
			 * written will be remarked as dirty until other
			 * buffers are written.
			 */
			if (bp->b_vp && bp->b_vp->v_mount
			    && (bp->b_vp->v_mount->mnt_flag & MNT_SOFTDEP)
			    && (bp->b_flags & B_DELWRI)) {
				mutex_enter(&bp->b_interlock);
				bremfree(bp);
				bp->b_flags |= B_BUSY;
				nbusy++;
				mutex_exit(&bp->b_interlock);
				mutex_exit(&bqueue_lock);
				bawrite(bp);
				if (dcount-- <= 0) {
					printf("softdep ");
					goto fail;
				}
				mutex_enter(&bqueue_lock);
			}
		    }
		}

		mutex_exit(&bqueue_lock);

		if (nbusy == 0)
			break;
		if (nbusy_prev == 0)
			nbusy_prev = nbusy;
		printf("%d ", nbusy);
		tsleep(&nbusy, PRIBIO, "bflush",
		    (iter == 0) ? 1 : hz / 25 * iter);
		if (nbusy >= nbusy_prev) /* we didn't flush anything */
			iter++;
		else
			nbusy_prev = nbusy;
	}

	if (nbusy) {
fail:;
#if defined(DEBUG) || defined(DEBUG_HALT_BUSY)
		printf("giving up\nPrinting vnodes for busy buffers\n");
		for (ihash = 0; ihash < bufhash+1; ihash++) {
		    LIST_FOREACH(bp, &bufhashtbl[ihash], b_hash) {
			if ((bp->b_flags & (B_BUSY|B_INVAL|B_READ)) == B_BUSY)
				vprint(NULL, bp->b_vp);
		    }
		}
#endif
	}

	return nbusy;
}

static void
sysctl_fillbuf(struct buf *i, struct buf_sysctl *o)
{

	o->b_flags = i->b_flags;
	o->b_error = i->b_error;
	o->b_prio = i->b_prio;
	o->b_dev = i->b_dev;
	o->b_bufsize = i->b_bufsize;
	o->b_bcount = i->b_bcount;
	o->b_resid = i->b_resid;
	o->b_addr = PTRTOUINT64(i->b_un.b_addr);
	o->b_blkno = i->b_blkno;
	o->b_rawblkno = i->b_rawblkno;
	o->b_iodone = PTRTOUINT64(i->b_iodone);
	o->b_proc = PTRTOUINT64(i->b_proc);
	o->b_vp = PTRTOUINT64(i->b_vp);
	o->b_saveaddr = PTRTOUINT64(i->b_saveaddr);
	o->b_lblkno = i->b_lblkno;
}

#define KERN_BUFSLOP 20
static int
sysctl_dobuf(SYSCTLFN_ARGS)
{
	struct buf *bp;
	struct buf_sysctl bs;
	struct bqueue *bq;
	char *dp;
	u_int i, op, arg;
	size_t len, needed, elem_size, out_size;
	int error, elem_count, retries;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (namelen != 4)
		return (EINVAL);

	retries = 100;
 retry:
	dp = oldp;
	len = (oldp != NULL) ? *oldlenp : 0;
	op = name[0];
	arg = name[1];
	elem_size = name[2];
	elem_count = name[3];
	out_size = MIN(sizeof(bs), elem_size);

	/*
	 * at the moment, these are just "placeholders" to make the
	 * API for retrieving kern.buf data more extensible in the
	 * future.
	 *
	 * XXX kern.buf currently has "netbsd32" issues.  hopefully
	 * these will be resolved at a later point.
	 */
	if (op != KERN_BUF_ALL || arg != KERN_BUF_ALL ||
	    elem_size < 1 || elem_count < 0)
		return (EINVAL);

	error = 0;
	needed = 0;
	mutex_enter(&bqueue_lock);
	for (i = 0; i < BQUEUES; i++) {
		bq = &bufqueues[i];
		TAILQ_FOREACH(bp, &bq->bq_queue, b_freelist) {
			bq->bq_marker = bp;
			if (len >= elem_size && elem_count > 0) {
				sysctl_fillbuf(bp, &bs);
				mutex_exit(&bqueue_lock);
				error = copyout(&bs, dp, out_size);
				mutex_enter(&bqueue_lock);
				if (error)
					break;
				if (bq->bq_marker != bp) {
					/*
					 * This sysctl node is only for
					 * statistics.  Retry; if the
					 * queue keeps changing, then
					 * bail out.
					 */
					if (retries-- == 0) {
						error = EAGAIN;
						break;
					}
					mutex_exit(&bqueue_lock);
					goto retry;
				}
				dp += elem_size;
				len -= elem_size;
			}
			if (elem_count > 0) {
				needed += elem_size;
				if (elem_count != INT_MAX)
					elem_count--;
			}
		}
		if (error != 0)
			break;
	}
	mutex_exit(&bqueue_lock);

	*oldlenp = needed;
	if (oldp == NULL)
		*oldlenp += KERN_BUFSLOP * sizeof(struct buf);

	return (error);
}

static int
sysctl_bufvm_update(SYSCTLFN_ARGS)
{
	int t, error;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &t;
	t = *(int *)rnode->sysctl_data;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (t < 0)
		return EINVAL;
	if (rnode->sysctl_data == &bufcache) {
		if (t > 100)
			return (EINVAL);
		bufcache = t;
		buf_setwm();
	} else if (rnode->sysctl_data == &bufmem_lowater) {
		if (bufmem_hiwater - t < 16)
			return (EINVAL);
		bufmem_lowater = t;
	} else if (rnode->sysctl_data == &bufmem_hiwater) {
		if (t - bufmem_lowater < 16)
			return (EINVAL);
		bufmem_hiwater = t;
	} else
		return (EINVAL);

	/* Drain until below new high water mark */
	while ((t = bufmem - bufmem_hiwater) >= 0) {
		if (buf_drain(t / (2 * 1024)) <= 0)
			break;
	}

	return 0;
}

SYSCTL_SETUP(sysctl_kern_buf_setup, "sysctl kern.buf subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "buf",
		       SYSCTL_DESCR("Kernel buffer cache information"),
		       sysctl_dobuf, 0, NULL, 0,
		       CTL_KERN, KERN_BUF, CTL_EOL);
}

SYSCTL_SETUP(sysctl_vm_buf_setup, "sysctl vm.buf* subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vm", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VM, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "bufcache",
		       SYSCTL_DESCR("Percentage of physical memory to use for "
				    "buffer cache"),
		       sysctl_bufvm_update, 0, &bufcache, 0,
		       CTL_VM, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "bufmem",
		       SYSCTL_DESCR("Amount of kernel memory used by buffer "
				    "cache"),
		       NULL, 0, &bufmem, 0,
		       CTL_VM, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "bufmem_lowater",
		       SYSCTL_DESCR("Minimum amount of kernel memory to "
				    "reserve for buffer cache"),
		       sysctl_bufvm_update, 0, &bufmem_lowater, 0,
		       CTL_VM, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "bufmem_hiwater",
		       SYSCTL_DESCR("Maximum amount of kernel memory to use "
				    "for buffer cache"),
		       sysctl_bufvm_update, 0, &bufmem_hiwater, 0,
		       CTL_VM, CTL_CREATE, CTL_EOL);
}

#ifdef DEBUG
/*
 * Print out statistics on the current allocation of the buffer pool.
 * Can be enabled to print out on every ``sync'' by setting "syncprt"
 * in vfs_syscalls.c using sysctl.
 */
void
vfs_bufstats(void)
{
	int i, j, count;
	struct buf *bp;
	struct bqueue *dp;
	int counts[(MAXBSIZE / PAGE_SIZE) + 1];
	static const char *bname[BQUEUES] = { "LOCKED", "LRU", "AGE" };

	for (dp = bufqueues, i = 0; dp < &bufqueues[BQUEUES]; dp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			counts[j] = 0;
		TAILQ_FOREACH(bp, &dp->bq_queue, b_freelist) {
			counts[bp->b_bufsize/PAGE_SIZE]++;
			count++;
		}
		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE/PAGE_SIZE; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * PAGE_SIZE, counts[j]);
		printf("\n");
	}
}
#endif /* DEBUG */

/* ------------------------------ */

static POOL_INIT(bufiopool, sizeof(struct buf), 0, 0, 0, "biopl", NULL,
    IPL_NONE);

static struct buf *
getiobuf1(int prflags)
{
	struct buf *bp;

	bp = pool_get(&bufiopool, prflags);
	if (bp != NULL) {
		buf_init(bp);
	}
	return bp;
}

struct buf *
getiobuf(void)
{

	return getiobuf1(PR_WAITOK);
}

struct buf *
getiobuf_nowait(void)
{

	return getiobuf1(PR_NOWAIT);
}

void
putiobuf(struct buf *bp)
{

	buf_destroy(bp);
	pool_put(&bufiopool, bp);
}

/*
 * nestiobuf_iodone: b_iodone callback for nested buffers.
 */

void
nestiobuf_iodone(struct buf *bp)
{
	struct buf *mbp = bp->b_private;
	int error;
	int donebytes;

	KASSERT(bp->b_bcount <= bp->b_bufsize);
	KASSERT(mbp != bp);

	error = 0;
	if ((bp->b_flags & B_ERROR) != 0) {
		KASSERT(bp->b_error != 0);
		error = bp->b_error;
	} else if ((bp->b_bcount < bp->b_bufsize) || (bp->b_resid > 0)) {
		/*
		 * Not all got transfered, raise an error. We have no way to
		 * propagate these conditions to mbp.
		 */
		error = EIO;
	}

	donebytes = bp->b_bufsize;

	putiobuf(bp);
	nestiobuf_done(mbp, donebytes, error);
}

/*
 * nestiobuf_setup: setup a "nested" buffer.
 *
 * => 'mbp' is a "master" buffer which is being divided into sub pieces.
 * => 'bp' should be a buffer allocated by getiobuf or getiobuf_nowait.
 * => 'offset' is a byte offset in the master buffer.
 * => 'size' is a size in bytes of this nested buffer.
 */

void
nestiobuf_setup(struct buf *mbp, struct buf *bp, int offset, size_t size)
{
	const int b_read = mbp->b_flags & B_READ;
	struct vnode *vp = mbp->b_vp;

	KASSERT(mbp->b_bcount >= offset + size);
	bp->b_vp = vp;
	bp->b_flags = B_BUSY | B_CALL | B_ASYNC | b_read;
	bp->b_iodone = nestiobuf_iodone;
	bp->b_data = (char *)mbp->b_data + offset;
	bp->b_resid = bp->b_bcount = size;
	bp->b_bufsize = bp->b_bcount;
	bp->b_private = mbp;
	BIO_COPYPRIO(bp, mbp);
	if (!b_read && vp != NULL) {
		mutex_enter(&vp->v_interlock);
		vp->v_numoutput++;
		mutex_exit(&vp->v_interlock);
	}
}

/*
 * nestiobuf_done: propagate completion to the master buffer.
 *
 * => 'donebytes' specifies how many bytes in the 'mbp' is completed.
 * => 'error' is an errno(2) that 'donebytes' has been completed with.
 */

void
nestiobuf_done(struct buf *mbp, int donebytes, int error)
{

	if (donebytes == 0) {
		return;
	}
	mutex_enter(&mbp->b_interlock);
	KASSERT(mbp->b_resid >= donebytes);
	mbp->b_resid -= donebytes;
	if (mbp->b_resid == 0) {
		mutex_exit(&mbp->b_interlock);
		biodone(mbp, error, mbp->b_resid);
	} else
		mutex_exit(&mbp->b_interlock);
}

void
buf_init(struct buf *bp)
{

	LIST_INIT(&bp->b_dep);
	mutex_init(&bp->b_interlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&bp->b_cv, "biowait");
	bp->b_dev = NODEV;
	bp->b_error = 0;
	BIO_SETPRIO(bp, BPRIO_DEFAULT);
}

void
buf_destroy(struct buf *bp)
{

	mutex_destroy(&bp->b_interlock);
	cv_destroy(&bp->b_cv);
}
