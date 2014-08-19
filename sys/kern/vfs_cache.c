/*	$NetBSD: vfs_cache.c,v 1.89.2.2 2014/08/20 00:04:29 tls Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
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

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vfs_cache.c	8.3 (Berkeley) 8/22/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cache.c,v 1.89.2.2 2014/08/20 00:04:29 tls Exp $");

#include "opt_ddb.h"
#include "opt_revcache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/pool.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#include <sys/kthread.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>

#define NAMECACHE_ENTER_REVERSE
/*
 * Name caching works as follows:
 *
 * Names found by directory scans are retained in a cache
 * for future reference.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (dvp, name) where dvp refers to the directory
 * containing name.
 *
 * For simplicity (and economy of storage), names longer than
 * a maximum length of NCHNAMLEN are not cached; they occur
 * infrequently in any case, and are almost never of interest.
 *
 * Upon reaching the last segment of a path, if the reference
 * is for DELETE, or NOCACHE is set (rewrite), and the
 * name is located in the cache, it will be dropped.
 * The entry is dropped also when it was not possible to lock
 * the cached vnode, either because vget() failed or the generation
 * number has changed while waiting for the lock.
 */

/*
 * Per-cpu namecache data.
 */
struct nchcpu {
	kmutex_t	cpu_lock;
	struct nchstats	cpu_stats;
};

/*
 * The type for the hash code. While the hash function generates a
 * u32, the hash code has historically been passed around as a u_long,
 * and the value is modified by xor'ing a uintptr_t, so it's not
 * entirely clear what the best type is. For now I'll leave it
 * unchanged as u_long.
 */

typedef u_long nchash_t;

/*
 * Structures associated with name cacheing.
 */

static kmutex_t *namecache_lock __read_mostly;
static pool_cache_t namecache_cache __read_mostly;
static TAILQ_HEAD(, namecache) nclruhead __cacheline_aligned;

static LIST_HEAD(nchashhead, namecache) *nchashtbl __read_mostly;
static u_long	nchash __read_mostly;

#define	NCHASH2(hash, dvp)	\
	(((hash) ^ ((uintptr_t)(dvp) >> 3)) & nchash)

static LIST_HEAD(ncvhashhead, namecache) *ncvhashtbl __read_mostly;
static u_long	ncvhash __read_mostly;

#define	NCVHASH(vp)		(((uintptr_t)(vp) >> 3) & ncvhash)

/* Number of cache entries allocated. */
static long	numcache __cacheline_aligned;

/* Garbage collection queue and number of entries pending in it. */
static void	*cache_gcqueue;
static u_int	cache_gcpend;

/* Cache effectiveness statistics. */
struct nchstats	nchstats __cacheline_aligned;
#define	COUNT(c,x)	(c.x++)

static const int cache_lowat = 95;
static const int cache_hiwat = 98;
static const int cache_hottime = 5;	/* number of seconds */
static int doingcache = 1;		/* 1 => enable the cache */

static struct evcnt cache_ev_scan;
static struct evcnt cache_ev_gc;
static struct evcnt cache_ev_over;
static struct evcnt cache_ev_under;
static struct evcnt cache_ev_forced;

static void cache_invalidate(struct namecache *);
static struct namecache *cache_lookup_entry(
    const struct vnode *, const char *, size_t);
static void cache_thread(void *);
static void cache_invalidate(struct namecache *);
static void cache_disassociate(struct namecache *);
static void cache_reclaim(void);
static int cache_ctor(void *, void *, int);
static void cache_dtor(void *, void *);

/*
 * Compute the hash for an entry.
 *
 * (This is for now a wrapper around namei_hash, whose interface is
 * for the time being slightly inconvenient.)
 */
static nchash_t
cache_hash(const char *name, size_t namelen)
{
	const char *endptr;

	endptr = name + namelen;
	return namei_hash(name, &endptr);
}

/*
 * Invalidate a cache entry and enqueue it for garbage collection.
 */
static void
cache_invalidate(struct namecache *ncp)
{
	void *head;

	KASSERT(mutex_owned(&ncp->nc_lock));

	if (ncp->nc_dvp != NULL) {
		ncp->nc_vp = NULL;
		ncp->nc_dvp = NULL;
		do {
			head = cache_gcqueue;
			ncp->nc_gcqueue = head;
		} while (atomic_cas_ptr(&cache_gcqueue, head, ncp) != head);
		atomic_inc_uint(&cache_gcpend);
	}
}

/*
 * Disassociate a namecache entry from any vnodes it is attached to,
 * and remove from the global LRU list.
 */
static void
cache_disassociate(struct namecache *ncp)
{

	KASSERT(mutex_owned(namecache_lock));
	KASSERT(ncp->nc_dvp == NULL);

	if (ncp->nc_lru.tqe_prev != NULL) {
		TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
		ncp->nc_lru.tqe_prev = NULL;
	}
	if (ncp->nc_vhash.le_prev != NULL) {
		LIST_REMOVE(ncp, nc_vhash);
		ncp->nc_vhash.le_prev = NULL;
	}
	if (ncp->nc_vlist.le_prev != NULL) {
		LIST_REMOVE(ncp, nc_vlist);
		ncp->nc_vlist.le_prev = NULL;
	}
	if (ncp->nc_dvlist.le_prev != NULL) {
		LIST_REMOVE(ncp, nc_dvlist);
		ncp->nc_dvlist.le_prev = NULL;
	}
}

/*
 * Lock all CPUs to prevent any cache lookup activity.  Conceptually,
 * this locks out all "readers".
 */
#define	UPDATE(f) do { \
	nchstats.f += cpup->cpu_stats.f; \
	cpup->cpu_stats.f = 0; \
} while (/* CONSTCOND */ 0)

static void
cache_lock_cpus(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct nchcpu *cpup;

	for (CPU_INFO_FOREACH(cii, ci)) {
		cpup = ci->ci_data.cpu_nch;
		mutex_enter(&cpup->cpu_lock);
		UPDATE(ncs_goodhits);
		UPDATE(ncs_neghits);
		UPDATE(ncs_badhits);
		UPDATE(ncs_falsehits);
		UPDATE(ncs_miss);
		UPDATE(ncs_long);
		UPDATE(ncs_pass2);
		UPDATE(ncs_2passes);
		UPDATE(ncs_revhits);
		UPDATE(ncs_revmiss);
	}
}

#undef UPDATE

/*
 * Release all CPU locks.
 */
static void
cache_unlock_cpus(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct nchcpu *cpup;

	for (CPU_INFO_FOREACH(cii, ci)) {
		cpup = ci->ci_data.cpu_nch;
		mutex_exit(&cpup->cpu_lock);
	}
}

/*
 * Find a single cache entry and return it locked.
 */
static struct namecache *
cache_lookup_entry(const struct vnode *dvp, const char *name, size_t namelen)
{
	struct nchashhead *ncpp;
	struct namecache *ncp;
	nchash_t hash;

	KASSERT(dvp != NULL);
	KASSERT(mutex_owned(namecache_lock));
	hash = cache_hash(name, namelen);
	ncpp = &nchashtbl[NCHASH2(hash, dvp)];

	LIST_FOREACH(ncp, ncpp, nc_hash) {
		if (ncp->nc_dvp != dvp ||
		    ncp->nc_nlen != namelen ||
		    memcmp(ncp->nc_name, name, (u_int)ncp->nc_nlen))
		    	continue;
	    	mutex_enter(&ncp->nc_lock);
		if (__predict_true(ncp->nc_dvp == dvp)) {
			ncp->nc_hittime = hardclock_ticks;
			return ncp;
		}
		/* Raced: entry has been nullified. */
		mutex_exit(&ncp->nc_lock);
	}

	return NULL;
}

/*
 * Look for a the name in the cache. We don't do this
 * if the segment name is long, simply so the cache can avoid
 * holding long names (which would either waste space, or
 * add greatly to the complexity).
 *
 * Lookup is called with DVP pointing to the directory to search,
 * and CNP providing the name of the entry being sought: cn_nameptr
 * is the name, cn_namelen is its length, and cn_flags is the flags
 * word from the namei operation.
 *
 * DVP must be locked.
 *
 * There are three possible non-error return states:
 *    1. Nothing was found in the cache. Nothing is known about
 *       the requested name.
 *    2. A negative entry was found in the cache, meaning that the
 *       requested name definitely does not exist.
 *    3. A positive entry was found in the cache, meaning that the
 *       requested name does exist and that we are providing the
 *       vnode.
 * In these cases the results are:
 *    1. 0 returned; VN is set to NULL.
 *    2. 1 returned; VN is set to NULL.
 *    3. 1 returned; VN is set to the vnode found.
 *
 * The additional result argument ISWHT is set to zero, unless a
 * negative entry is found that was entered as a whiteout, in which
 * case ISWHT is set to one.
 *
 * The ISWHT_RET argument pointer may be null. In this case an
 * assertion is made that the whiteout flag is not set. File systems
 * that do not support whiteouts can/should do this.
 *
 * Filesystems that do support whiteouts should add ISWHITEOUT to
 * cnp->cn_flags if ISWHT comes back nonzero.
 *
 * When a vnode is returned, it is locked, as per the vnode lookup
 * locking protocol.
 *
 * There is no way for this function to fail, in the sense of
 * generating an error that requires aborting the namei operation.
 *
 * (Prior to October 2012, this function returned an integer status,
 * and a vnode, and mucked with the flags word in CNP for whiteouts.
 * The integer status was -1 for "nothing found", ENOENT for "a
 * negative entry found", 0 for "a positive entry found", and possibly
 * other errors, and the value of VN might or might not have been set
 * depending on what error occurred.)
 */
int
cache_lookup(struct vnode *dvp, const char *name, size_t namelen,
	     uint32_t nameiop, uint32_t cnflags,
	     int *iswht_ret, struct vnode **vn_ret)
{
	struct namecache *ncp;
	struct vnode *vp;
	struct nchcpu *cpup;
	int error;

	/* Establish default result values */
	if (iswht_ret != NULL) {
		*iswht_ret = 0;
	}
	*vn_ret = NULL;

	if (__predict_false(!doingcache)) {
		return 0;
	}

	cpup = curcpu()->ci_data.cpu_nch;
	if (__predict_false(namelen > NCHNAMLEN)) {
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_long);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}
	mutex_enter(namecache_lock);
	ncp = cache_lookup_entry(dvp, name, namelen);
	mutex_exit(namecache_lock);
	if (__predict_false(ncp == NULL)) {
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_miss);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}
	if ((cnflags & MAKEENTRY) == 0) {
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_badhits);
		mutex_exit(&cpup->cpu_lock);
		/*
		 * Last component and we are renaming or deleting,
		 * the cache entry is invalid, or otherwise don't
		 * want cache entry to exist.
		 */
		cache_invalidate(ncp);
		mutex_exit(&ncp->nc_lock);
		/* found nothing */
		return 0;
	}
	if (ncp->nc_vp == NULL) {
		if (iswht_ret != NULL) {
			/*
			 * Restore the ISWHITEOUT flag saved earlier.
			 */
			KASSERT((ncp->nc_flags & ~ISWHITEOUT) == 0);
			*iswht_ret = (ncp->nc_flags & ISWHITEOUT) != 0;
		} else {
			KASSERT(ncp->nc_flags == 0);
		}

		if (__predict_true(nameiop != CREATE ||
		    (cnflags & ISLASTCN) == 0)) {
			mutex_enter(&cpup->cpu_lock);
			COUNT(cpup->cpu_stats, ncs_neghits);
			mutex_exit(&cpup->cpu_lock);
			mutex_exit(&ncp->nc_lock);
			/* found neg entry; vn is already null from above */
			return 1;
		} else {
			mutex_enter(&cpup->cpu_lock);
			COUNT(cpup->cpu_stats, ncs_badhits);
			mutex_exit(&cpup->cpu_lock);
			/*
			 * Last component and we are renaming or
			 * deleting, the cache entry is invalid,
			 * or otherwise don't want cache entry to
			 * exist.
			 */
			cache_invalidate(ncp);
			mutex_exit(&ncp->nc_lock);
			/* found nothing */
			return 0;
		}
	}

	vp = ncp->nc_vp;
	mutex_enter(vp->v_interlock);
	mutex_exit(&ncp->nc_lock);
	error = vget(vp, LK_NOWAIT);
	if (error) {
		KASSERT(error == EBUSY);
		/*
		 * This vnode is being cleaned out.
		 * XXX badhits?
		 */
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_falsehits);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}

#ifdef DEBUG
	/*
	 * since we released nb->nb_lock,
	 * we can't use this pointer any more.
	 */
	ncp = NULL;
#endif /* DEBUG */

	/* We don't have the right lock, but this is only for stats. */
	mutex_enter(&cpup->cpu_lock);
	COUNT(cpup->cpu_stats, ncs_goodhits);
	mutex_exit(&cpup->cpu_lock);

	/* found it */
	*vn_ret = vp;
	return 1;
}

int
cache_lookup_raw(struct vnode *dvp, const char *name, size_t namelen,
		 uint32_t cnflags,
		 int *iswht_ret, struct vnode **vn_ret)
{
	struct namecache *ncp;
	struct vnode *vp;
	struct nchcpu *cpup;
	int error;

	/* Establish default results. */
	if (iswht_ret != NULL) {
		*iswht_ret = 0;
	}
	*vn_ret = NULL;

	if (__predict_false(!doingcache)) {
		/* found nothing */
		return 0;
	}

	cpup = curcpu()->ci_data.cpu_nch;
	if (__predict_false(namelen > NCHNAMLEN)) {
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_long);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}
	mutex_enter(namecache_lock);
	ncp = cache_lookup_entry(dvp, name, namelen);
	mutex_exit(namecache_lock);
	if (__predict_false(ncp == NULL)) {
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_miss);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}
	vp = ncp->nc_vp;
	if (vp == NULL) {
		/*
		 * Restore the ISWHITEOUT flag saved earlier.
		 */
		if (iswht_ret != NULL) {
			KASSERT((ncp->nc_flags & ~ISWHITEOUT) == 0);
			/*cnp->cn_flags |= ncp->nc_flags;*/
			*iswht_ret = (ncp->nc_flags & ISWHITEOUT) != 0;
		}
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_neghits);
		mutex_exit(&cpup->cpu_lock);
		mutex_exit(&ncp->nc_lock);
		/* found negative entry; vn is already null from above */
		return 1;
	}
	mutex_enter(vp->v_interlock);
	mutex_exit(&ncp->nc_lock);
	error = vget(vp, LK_NOWAIT);
	if (error) {
		KASSERT(error == EBUSY);
		/*
		 * This vnode is being cleaned out.
		 * XXX badhits?
		 */
		mutex_enter(&cpup->cpu_lock);
		COUNT(cpup->cpu_stats, ncs_falsehits);
		mutex_exit(&cpup->cpu_lock);
		/* found nothing */
		return 0;
	}

	/* Unlocked, but only for stats. */
	mutex_enter(&cpup->cpu_lock);
	COUNT(cpup->cpu_stats, ncs_goodhits); /* XXX can be "badhits" */
	mutex_exit(&cpup->cpu_lock);

	/* found it */
	*vn_ret = vp;
	return 1;
}

/*
 * Scan cache looking for name of directory entry pointing at vp.
 *
 * If the lookup succeeds the vnode is referenced and stored in dvpp.
 *
 * If bufp is non-NULL, also place the name in the buffer which starts
 * at bufp, immediately before *bpp, and move bpp backwards to point
 * at the start of it.  (Yes, this is a little baroque, but it's done
 * this way to cater to the whims of getcwd).
 *
 * Returns 0 on success, -1 on cache miss, positive errno on failure.
 */
int
cache_revlookup(struct vnode *vp, struct vnode **dvpp, char **bpp, char *bufp)
{
	struct namecache *ncp;
	struct vnode *dvp;
	struct nchcpu *cpup;
	struct ncvhashhead *nvcpp;
	char *bp;
	int error, nlen;

	if (!doingcache)
		goto out;

	nvcpp = &ncvhashtbl[NCVHASH(vp)];
	cpup = curcpu()->ci_data.cpu_nch;

	mutex_enter(namecache_lock);
	LIST_FOREACH(ncp, nvcpp, nc_vhash) {
		mutex_enter(&ncp->nc_lock);
		if (ncp->nc_vp == vp &&
		    (dvp = ncp->nc_dvp) != NULL &&
		    dvp != vp) { 		/* avoid pesky . entries.. */

#ifdef DIAGNOSTIC
			if (ncp->nc_nlen == 1 &&
			    ncp->nc_name[0] == '.')
				panic("cache_revlookup: found entry for .");

			if (ncp->nc_nlen == 2 &&
			    ncp->nc_name[0] == '.' &&
			    ncp->nc_name[1] == '.')
				panic("cache_revlookup: found entry for ..");
#endif
			mutex_enter(&cpup->cpu_lock);
			COUNT(cpup->cpu_stats, ncs_revhits);
			mutex_exit(&cpup->cpu_lock);
			nlen = ncp->nc_nlen;

			if (bufp) {
				bp = *bpp;
				bp -= nlen;
				if (bp <= bufp) {
					*dvpp = NULL;
					mutex_exit(&ncp->nc_lock);
					mutex_exit(namecache_lock);
					return (ERANGE);
				}
				memcpy(bp, ncp->nc_name, nlen);
				*bpp = bp;
			}

			mutex_enter(dvp->v_interlock);
			mutex_exit(&ncp->nc_lock); 
			mutex_exit(namecache_lock);
			error = vget(dvp, LK_NOWAIT);
			if (error) {
				KASSERT(error == EBUSY);
				if (bufp)
					(*bpp) += nlen;
				*dvpp = NULL;
				return -1;
			}
			*dvpp = dvp;
			return (0);
		}
		mutex_exit(&ncp->nc_lock);
	}
	mutex_enter(&cpup->cpu_lock);
	COUNT(cpup->cpu_stats, ncs_revmiss);
	mutex_exit(&cpup->cpu_lock);
	mutex_exit(namecache_lock);
 out:
	*dvpp = NULL;
	return (-1);
}

/*
 * Add an entry to the cache
 */
void
cache_enter(struct vnode *dvp, struct vnode *vp,
	    const char *name, size_t namelen, uint32_t cnflags)
{
	struct namecache *ncp;
	struct namecache *oncp;
	struct nchashhead *ncpp;
	struct ncvhashhead *nvcpp;
	nchash_t hash;

	/* First, check whether we can/should add a cache entry. */
	if ((cnflags & MAKEENTRY) == 0 ||
	    __predict_false(namelen > NCHNAMLEN || !doingcache)) {
		return;
	}

	if (numcache > desiredvnodes) {
		mutex_enter(namecache_lock);
		cache_ev_forced.ev_count++;
		cache_reclaim();
		mutex_exit(namecache_lock);
	}

	ncp = pool_cache_get(namecache_cache, PR_WAITOK);
	mutex_enter(namecache_lock);
	numcache++;

	/*
	 * Concurrent lookups in the same directory may race for a
	 * cache entry.  if there's a duplicated entry, free it.
	 */
	oncp = cache_lookup_entry(dvp, name, namelen);
	if (oncp) {
		cache_invalidate(oncp);
		mutex_exit(&oncp->nc_lock);
	}

	/* Grab the vnode we just found. */
	mutex_enter(&ncp->nc_lock);
	ncp->nc_vp = vp;
	ncp->nc_flags = 0;
	ncp->nc_hittime = 0;
	ncp->nc_gcqueue = NULL;
	if (vp == NULL) {
		/*
		 * For negative hits, save the ISWHITEOUT flag so we can
		 * restore it later when the cache entry is used again.
		 */
		ncp->nc_flags = cnflags & ISWHITEOUT;
	}

	/* Fill in cache info. */
	ncp->nc_dvp = dvp;
	LIST_INSERT_HEAD(&dvp->v_dnclist, ncp, nc_dvlist);
	if (vp)
		LIST_INSERT_HEAD(&vp->v_nclist, ncp, nc_vlist);
	else {
		ncp->nc_vlist.le_prev = NULL;
		ncp->nc_vlist.le_next = NULL;
	}
	KASSERT(namelen <= NCHNAMLEN);
	ncp->nc_nlen = namelen;
	memcpy(ncp->nc_name, name, (unsigned)ncp->nc_nlen);
	TAILQ_INSERT_TAIL(&nclruhead, ncp, nc_lru);
	hash = cache_hash(name, namelen);
	ncpp = &nchashtbl[NCHASH2(hash, dvp)];

	/*
	 * Flush updates before making visible in table.  No need for a
	 * memory barrier on the other side: to see modifications the
	 * list must be followed, meaning a dependent pointer load.
	 * The below is LIST_INSERT_HEAD() inlined, with the memory
	 * barrier included in the correct place.
	 */
	if ((ncp->nc_hash.le_next = ncpp->lh_first) != NULL)
		ncpp->lh_first->nc_hash.le_prev = &ncp->nc_hash.le_next;
	ncp->nc_hash.le_prev = &ncpp->lh_first;
	membar_producer();
	ncpp->lh_first = ncp;

	ncp->nc_vhash.le_prev = NULL;
	ncp->nc_vhash.le_next = NULL;

	/*
	 * Create reverse-cache entries (used in getcwd) for directories.
	 * (and in linux procfs exe node)
	 */
	if (vp != NULL &&
	    vp != dvp &&
#ifndef NAMECACHE_ENTER_REVERSE
	    vp->v_type == VDIR &&
#endif
	    (ncp->nc_nlen > 2 ||
	    (ncp->nc_nlen > 1 && ncp->nc_name[1] != '.') ||
	    (/* ncp->nc_nlen > 0 && */ ncp->nc_name[0] != '.'))) {
		nvcpp = &ncvhashtbl[NCVHASH(vp)];
		LIST_INSERT_HEAD(nvcpp, ncp, nc_vhash);
	}
	mutex_exit(&ncp->nc_lock);
	mutex_exit(namecache_lock);
}

/*
 * Name cache initialization, from vfs_init() when we are booting
 */
void
nchinit(void)
{
	int error;

	TAILQ_INIT(&nclruhead);
	namecache_cache = pool_cache_init(sizeof(struct namecache), 
	    coherency_unit, 0, 0, "ncache", NULL, IPL_NONE, cache_ctor,
	    cache_dtor, NULL);
	KASSERT(namecache_cache != NULL);

	namecache_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	nchashtbl = hashinit(desiredvnodes, HASH_LIST, true, &nchash);
	ncvhashtbl =
#ifdef NAMECACHE_ENTER_REVERSE
	    hashinit(desiredvnodes, HASH_LIST, true, &ncvhash);
#else
	    hashinit(desiredvnodes/8, HASH_LIST, true, &ncvhash);
#endif

	error = kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL, cache_thread,
	    NULL, NULL, "cachegc");
	if (error != 0)
		panic("nchinit %d", error);

	evcnt_attach_dynamic(&cache_ev_scan, EVCNT_TYPE_MISC, NULL,
	   "namecache", "entries scanned");
	evcnt_attach_dynamic(&cache_ev_gc, EVCNT_TYPE_MISC, NULL,
	   "namecache", "entries collected");
	evcnt_attach_dynamic(&cache_ev_over, EVCNT_TYPE_MISC, NULL,
	   "namecache", "over scan target");
	evcnt_attach_dynamic(&cache_ev_under, EVCNT_TYPE_MISC, NULL,
	   "namecache", "under scan target");
	evcnt_attach_dynamic(&cache_ev_forced, EVCNT_TYPE_MISC, NULL,
	   "namecache", "forced reclaims");
}

static int
cache_ctor(void *arg, void *obj, int flag)
{
	struct namecache *ncp;

	ncp = obj;
	mutex_init(&ncp->nc_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

static void
cache_dtor(void *arg, void *obj)
{
	struct namecache *ncp;

	ncp = obj;
	mutex_destroy(&ncp->nc_lock);
}

/*
 * Called once for each CPU in the system as attached.
 */
void
cache_cpu_init(struct cpu_info *ci)
{
	struct nchcpu *cpup;
	size_t sz;

	sz = roundup2(sizeof(*cpup), coherency_unit) + coherency_unit;
	cpup = kmem_zalloc(sz, KM_SLEEP);
	cpup = (void *)roundup2((uintptr_t)cpup, coherency_unit);
	mutex_init(&cpup->cpu_lock, MUTEX_DEFAULT, IPL_NONE);
	ci->ci_data.cpu_nch = cpup;
}

/*
 * Name cache reinitialization, for when the maximum number of vnodes increases.
 */
void
nchreinit(void)
{
	struct namecache *ncp;
	struct nchashhead *oldhash1, *hash1;
	struct ncvhashhead *oldhash2, *hash2;
	u_long i, oldmask1, oldmask2, mask1, mask2;

	hash1 = hashinit(desiredvnodes, HASH_LIST, true, &mask1);
	hash2 =
#ifdef NAMECACHE_ENTER_REVERSE
	    hashinit(desiredvnodes, HASH_LIST, true, &mask2);
#else
	    hashinit(desiredvnodes/8, HASH_LIST, true, &mask2);
#endif
	mutex_enter(namecache_lock);
	cache_lock_cpus();
	oldhash1 = nchashtbl;
	oldmask1 = nchash;
	nchashtbl = hash1;
	nchash = mask1;
	oldhash2 = ncvhashtbl;
	oldmask2 = ncvhash;
	ncvhashtbl = hash2;
	ncvhash = mask2;
	for (i = 0; i <= oldmask1; i++) {
		while ((ncp = LIST_FIRST(&oldhash1[i])) != NULL) {
			LIST_REMOVE(ncp, nc_hash);
			ncp->nc_hash.le_prev = NULL;
		}
	}
	for (i = 0; i <= oldmask2; i++) {
		while ((ncp = LIST_FIRST(&oldhash2[i])) != NULL) {
			LIST_REMOVE(ncp, nc_vhash);
			ncp->nc_vhash.le_prev = NULL;
		}
	}
	cache_unlock_cpus();
	mutex_exit(namecache_lock);
	hashdone(oldhash1, HASH_LIST, oldmask1);
	hashdone(oldhash2, HASH_LIST, oldmask2);
}

/*
 * Cache flush, a particular vnode; called when a vnode is renamed to
 * hide entries that would now be invalid
 */
void
cache_purge1(struct vnode *vp, const char *name, size_t namelen, int flags)
{
	struct namecache *ncp, *ncnext;

	mutex_enter(namecache_lock);
	if (flags & PURGE_PARENTS) {
		for (ncp = LIST_FIRST(&vp->v_nclist); ncp != NULL;
		    ncp = ncnext) {
			ncnext = LIST_NEXT(ncp, nc_vlist);
			mutex_enter(&ncp->nc_lock);
			cache_invalidate(ncp);
			mutex_exit(&ncp->nc_lock);
			cache_disassociate(ncp);
		}
	}
	if (flags & PURGE_CHILDREN) {
		for (ncp = LIST_FIRST(&vp->v_dnclist); ncp != NULL;
		    ncp = ncnext) {
			ncnext = LIST_NEXT(ncp, nc_dvlist);
			mutex_enter(&ncp->nc_lock);
			cache_invalidate(ncp);
			mutex_exit(&ncp->nc_lock);
			cache_disassociate(ncp);
		}
	}
	if (name != NULL) {
		ncp = cache_lookup_entry(vp, name, namelen);
		if (ncp) {
			cache_invalidate(ncp);
			mutex_exit(&ncp->nc_lock);
			cache_disassociate(ncp);
		}
	}
	mutex_exit(namecache_lock);
}

/*
 * Cache flush, a whole filesystem; called when filesys is umounted to
 * remove entries that would now be invalid.
 */
void
cache_purgevfs(struct mount *mp)
{
	struct namecache *ncp, *nxtcp;

	mutex_enter(namecache_lock);
	for (ncp = TAILQ_FIRST(&nclruhead); ncp != NULL; ncp = nxtcp) {
		nxtcp = TAILQ_NEXT(ncp, nc_lru);
		mutex_enter(&ncp->nc_lock);
		if (ncp->nc_dvp != NULL && ncp->nc_dvp->v_mount == mp) {
			/* Free the resources we had. */
			cache_invalidate(ncp);
			cache_disassociate(ncp);
		}
		mutex_exit(&ncp->nc_lock);
	}
	cache_reclaim();
	mutex_exit(namecache_lock);
}

/*
 * Scan global list invalidating entries until we meet a preset target. 
 * Prefer to invalidate entries that have not scored a hit within
 * cache_hottime seconds.  We sort the LRU list only for this routine's
 * benefit.
 */
static void
cache_prune(int incache, int target)
{
	struct namecache *ncp, *nxtcp, *sentinel;
	int items, recent, tryharder;

	KASSERT(mutex_owned(namecache_lock));

	items = 0;
	tryharder = 0;
	recent = hardclock_ticks - hz * cache_hottime;
	sentinel = NULL;
	for (ncp = TAILQ_FIRST(&nclruhead); ncp != NULL; ncp = nxtcp) {
		if (incache <= target)
			break;
		items++;
		nxtcp = TAILQ_NEXT(ncp, nc_lru);
		if (ncp == sentinel) {
			/*
			 * If we looped back on ourself, then ignore
			 * recent entries and purge whatever we find.
			 */
			tryharder = 1;
		}
		if (ncp->nc_dvp == NULL)
			continue;
		if (!tryharder && (ncp->nc_hittime - recent) > 0) {
			if (sentinel == NULL)
				sentinel = ncp;
			TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
			TAILQ_INSERT_TAIL(&nclruhead, ncp, nc_lru);
			continue;
		}
		mutex_enter(&ncp->nc_lock);
		if (ncp->nc_dvp != NULL) {
			cache_invalidate(ncp);
			cache_disassociate(ncp);
			incache--;
		}
		mutex_exit(&ncp->nc_lock);
	}
	cache_ev_scan.ev_count += items;
}

/*
 * Collect dead cache entries from all CPUs and garbage collect.
 */
static void
cache_reclaim(void)
{
	struct namecache *ncp, *next;
	int items;

	KASSERT(mutex_owned(namecache_lock));

	/*
	 * If the number of extant entries not awaiting garbage collection
	 * exceeds the high water mark, then reclaim stale entries until we
	 * reach our low water mark.
	 */
	items = numcache - cache_gcpend;
	if (items > (uint64_t)desiredvnodes * cache_hiwat / 100) {
		cache_prune(items, (int)((uint64_t)desiredvnodes *
		    cache_lowat / 100));
		cache_ev_over.ev_count++;
	} else
		cache_ev_under.ev_count++;

	/*
	 * Stop forward lookup activity on all CPUs and garbage collect dead
	 * entries.
	 */
	cache_lock_cpus();
	ncp = cache_gcqueue;
	cache_gcqueue = NULL;
	items = cache_gcpend;
	cache_gcpend = 0;
	while (ncp != NULL) {
		next = ncp->nc_gcqueue;
		cache_disassociate(ncp);
		KASSERT(ncp->nc_dvp == NULL);
		if (ncp->nc_hash.le_prev != NULL) {
			LIST_REMOVE(ncp, nc_hash);
			ncp->nc_hash.le_prev = NULL;
		}
		pool_cache_put(namecache_cache, ncp);
		ncp = next;
	}
	cache_unlock_cpus();
	numcache -= items;
	cache_ev_gc.ev_count += items;
}

/*
 * Cache maintainence thread, awakening once per second to:
 *
 * => keep number of entries below the high water mark
 * => sort pseudo-LRU list
 * => garbage collect dead entries
 */
static void
cache_thread(void *arg)
{

	mutex_enter(namecache_lock);
	for (;;) {
		cache_reclaim();
		kpause("cachegc", false, hz, namecache_lock);
	}
}

#ifdef DDB
void
namecache_print(struct vnode *vp, void (*pr)(const char *, ...))
{
	struct vnode *dvp = NULL;
	struct namecache *ncp;

	TAILQ_FOREACH(ncp, &nclruhead, nc_lru) {
		if (ncp->nc_vp == vp && ncp->nc_dvp != NULL) {
			(*pr)("name %.*s\n", ncp->nc_nlen, ncp->nc_name);
			dvp = ncp->nc_dvp;
		}
	}
	if (dvp == NULL) {
		(*pr)("name not found\n");
		return;
	}
	vp = dvp;
	TAILQ_FOREACH(ncp, &nclruhead, nc_lru) {
		if (ncp->nc_vp == vp) {
			(*pr)("parent %.*s\n", ncp->nc_nlen, ncp->nc_name);
		}
	}
}
#endif

void
namecache_count_pass2(void)
{
	struct nchcpu *cpup = curcpu()->ci_data.cpu_nch;

	mutex_enter(&cpup->cpu_lock);
	COUNT(cpup->cpu_stats, ncs_pass2);
	mutex_exit(&cpup->cpu_lock);
}

void
namecache_count_2passes(void)
{
	struct nchcpu *cpup = curcpu()->ci_data.cpu_nch;

	mutex_enter(&cpup->cpu_lock);
	COUNT(cpup->cpu_stats, ncs_2passes);
	mutex_exit(&cpup->cpu_lock);
}

static int
cache_stat_sysctl(SYSCTLFN_ARGS)
{
	struct nchstats_sysctl stats;

	if (oldp == NULL) {
		*oldlenp = sizeof(stats);
		return 0;
	}

	if (*oldlenp < sizeof(stats)) {
		*oldlenp = 0;
		return 0;
	}

	memset(&stats, 0, sizeof(stats));

	sysctl_unlock();
	cache_lock_cpus();
	stats.ncs_goodhits = nchstats.ncs_goodhits;
	stats.ncs_neghits = nchstats.ncs_neghits;
	stats.ncs_badhits = nchstats.ncs_badhits;
	stats.ncs_falsehits = nchstats.ncs_falsehits;
	stats.ncs_miss = nchstats.ncs_miss;
	stats.ncs_long = nchstats.ncs_long;
	stats.ncs_pass2 = nchstats.ncs_pass2;
	stats.ncs_2passes = nchstats.ncs_2passes;
	stats.ncs_revhits = nchstats.ncs_revhits;
	stats.ncs_revmiss = nchstats.ncs_revmiss;
	cache_unlock_cpus();
	sysctl_relock();

	*oldlenp = sizeof(stats);
	return sysctl_copyout(l, &stats, oldp, sizeof(stats));
}

SYSCTL_SETUP(sysctl_cache_stat_setup, "vfs.namecache_stats subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "namecache_stats",
		       SYSCTL_DESCR("namecache statistics"),
		       cache_stat_sysctl, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE, CTL_EOL);
}
