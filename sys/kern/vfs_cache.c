/*	$NetBSD: vfs_cache.c,v 1.126.2.5 2020/01/14 11:07:40 ad Exp $	*/

/*-
 * Copyright (c) 2008, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Name caching works as follows:
 *
 * 	Names found by directory scans are retained in a cache for future
 * 	reference.  It is managed pseudo-LRU, so frequently used names will
 * 	hang around.  Cache is indexed by directory.
 *
 *	Upon reaching the last segment of a path, if the reference is for
 *	DELETE, or NOCACHE is set (rewrite), and name is located in the
 *	cache, it will be dropped.
 *
 * Background:
 *
 *	XXX add a bit of history
 *
 * Data structures:
 *
 *	The original BSD implementation used a global hash table, which
 *	works very well on a uniprocessor system but causes performance
 *	difficulties on a multiprocessor system.  The global hash table is
 *	also difficult to size dynamically, and can become very large.  To
 *	try and address these problems, the focus of interest in this
 *	implementation is the directory itself.  A per-directory structure
 *	is used to look up names.
 *
 *	XXX Currently this structure is an rbtree, but rbtrees touch many
 *	cache lines during a lookup and so perform badly.  The intent is to
 *	utimately make use of some other data structure, perhaps a Robin
 *	Hood hash.  Insert blurb here when that happens.
 *
 * Replacement:
 *
 *	XXX LRU blurb.
 *
 * Concurrency:
 *
 *	XXX need new blurb here
 *
 *	See definition of "struct namecache" in src/sys/namei.src for the
 *	particulars.
 *
 *	Per-CPU statistics, and "numcache" are read unlocked, since an
 *	approximate value is OK.  We maintain uintptr_t sized per-CPU
 *	counters and 64-bit global counters under the theory that uintptr_t
 *	sized counters are less likely to be hosed by nonatomic increment.
 *
 * Lock order:
 *
 *	1) nc_dvp->vi_ncdlock
 *	2) cache_list_lock
 *	3) vp->v_interlock
 *
 * Ugly ASCII diagram:
 *
 *	XXX replace tabs with spaces, make less ugly
 *
 *          ...
 *	     |
 *	-----o-----
 *	|  VDIR   |
 *	|  vnode  |
 *	-----------
 *           ^
 *           |- nd_tree
 *	     |		  	       	 		 
 *	+----+----+	  	  -----------		    -----------
 *	|  VDIR   |		  |  VCHR   |		    |  VREG   |
 *	|  vnode  o-----+	  |  vnode  o-----+	    |  vnode  o------+
 *	+---------+     |	  -----------	  |	    -----------      |
 *	     ^	        |	       ^	  |		 ^           |
 *	     |- nc_vp   |- vi_nclist   |- nc_vp	  |- vi_nclist	 |- nc_vp    |
 *	     |	        |	       |	  |		 |           |
 *	-----o-----     |	  -----o-----	  |	    -----o-----      |
 *  +---onamecache|<----+     +---onamecache|<----+	+---onamecache|<-----+
 *  |	-----------	      |	  -----------		|   -----------
 *  |        ^		      |	       ^   		|	 ^
 *  |        |                |        |                |        |
 *  |        |  +----------------------+   		|	 |
 *  |-nc_dvp | +-------------------------------------------------+
 *  |	     |/- nd_tree      |		   		|
 *  |	     |		      |- nc_dvp		        |
 *  |	-----o-----	      |		  		|
 *  +-->|  VDIR   |<----------+			        |
 *	|  vnode  |<------------------------------------+
 *	-----------
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cache.c,v 1.126.2.5 2020/01/14 11:07:40 ad Exp $");

#define __NAMECACHE_PRIVATE
#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_dtrace.h"
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/pool.h>
#include <sys/sdt.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode_impl.h>

/* Per-CPU counters. */
struct nchstats_percpu _NAMEI_CACHE_STATS(uintptr_t);

/* Global pool cache. */
static pool_cache_t cache_pool __read_mostly;

/* LRU replacement state. */
enum cache_lru_id {
	LRU_ACTIVE,
	LRU_INACTIVE,
	LRU_COUNT
};

static struct {
	TAILQ_HEAD(, namecache)	list[LRU_COUNT];
	u_int			count[LRU_COUNT];
} cache_lru __cacheline_aligned;

static kmutex_t cache_lru_lock __cacheline_aligned;

/* Cache effectiveness statistics.  This holds total from per-cpu stats */
struct nchstats	nchstats __cacheline_aligned;

/* Macro to count an event. */
#define	COUNT(f)	do { \
	kpreempt_disable(); \
	((struct nchstats_percpu *)curcpu()->ci_data.cpu_nch)->f++; \
	kpreempt_enable(); \
} while (/* CONSTCOND */ 0);

/* Tunables */
static const int cache_lru_maxdeact = 2;	/* max # to deactivate */
static const int cache_lru_maxscan = 128;	/* max # to scan/reclaim */
static int doingcache = 1;			/* 1 => enable the cache */

static void	cache_reclaim(void);
static void	cache_remove(struct namecache *, const bool);

/* sysctl */
static struct	sysctllog *sysctllog;
static void	sysctl_cache_stat_setup(void);

/* dtrace hooks */
SDT_PROVIDER_DEFINE(vfs);

SDT_PROBE_DEFINE1(vfs, namecache, invalidate, done, "struct vnode *");
SDT_PROBE_DEFINE1(vfs, namecache, purge, parents, "struct vnode *");
SDT_PROBE_DEFINE1(vfs, namecache, purge, children, "struct vnode *");
SDT_PROBE_DEFINE2(vfs, namecache, purge, name, "char *", "size_t");
SDT_PROBE_DEFINE1(vfs, namecache, purge, vfs, "struct mount *");
SDT_PROBE_DEFINE3(vfs, namecache, lookup, hit, "struct vnode *",
    "char *", "size_t");
SDT_PROBE_DEFINE3(vfs, namecache, lookup, miss, "struct vnode *",
    "char *", "size_t");
SDT_PROBE_DEFINE3(vfs, namecache, lookup, toolong, "struct vnode *",
    "char *", "size_t");
SDT_PROBE_DEFINE2(vfs, namecache, revlookup, success, "struct vnode *",
     "struct vnode *");
SDT_PROBE_DEFINE2(vfs, namecache, revlookup, fail, "struct vnode *",
     "int");
SDT_PROBE_DEFINE2(vfs, namecache, prune, done, "int", "int");
SDT_PROBE_DEFINE3(vfs, namecache, enter, toolong, "struct vnode *",
    "char *", "size_t");
SDT_PROBE_DEFINE3(vfs, namecache, enter, done, "struct vnode *",
    "char *", "size_t");

/*
 * rbtree: compare two nodes.
 */
static int
cache_compare_nodes(void *context, const void *n1, const void *n2)
{
	const struct namecache *ncp1 = n1;
	const struct namecache *ncp2 = n2;

	if (ncp1->nc_nlen != ncp2->nc_nlen) {
		return (int)ncp1->nc_nlen - (int)ncp2->nc_nlen;
	}
	return memcmp(ncp1->nc_name, ncp2->nc_name, ncp1->nc_nlen);
}

/*
 * rbtree: compare a node and a key.
 */
static int
cache_compare_key(void *context, const void *n, const void *k)
{
	const struct namecache *ncp = n;
	const struct iovec *iov = k;

	if (ncp->nc_nlen != iov->iov_len) {
		return (int)ncp->nc_nlen - (int)iov->iov_len;
	}
	return memcmp(ncp->nc_name, iov->iov_base, ncp->nc_nlen);
}

/*
 * rbtree definition.
 */
static rb_tree_ops_t cache_rbtree_ops __read_mostly = {
	.rbto_compare_nodes = cache_compare_nodes,
	.rbto_compare_key = cache_compare_key,
	.rbto_node_offset = offsetof(struct namecache, nc_node),
	.rbto_context = NULL
};

/*
 * Remove an entry from the cache.  The directory lock must be held, and if
 * "dirtovnode" is true (it usually will be), the vnode's cache lock will be
 * acquired when removing the entry from the vnode list.
 */
static void
cache_remove(struct namecache *ncp, const bool dirtovnode)
{
	krwlock_t *vlock;

	KASSERT(rw_write_held(VNODE_TO_VIMPL(ncp->nc_dvp)->vi_ncdlock));

	SDT_PROBE(vfs, namecache, invalidate, done, ncp->nc_dvp,
	    0, 0, 0, 0);

	/* First remove from the directory's rbtree. */
	rb_tree_remove_node(&VNODE_TO_VIMPL(ncp->nc_dvp)->vi_nctree, ncp);

	/* Then remove from the LRU lists. */
	mutex_enter(&cache_lru_lock);
	TAILQ_REMOVE(&cache_lru.list[ncp->nc_lrulist], ncp, nc_lru);
	cache_lru.count[ncp->nc_lrulist]--;
	mutex_exit(&cache_lru_lock);

	/* Then remove from the vnode list. */
	if (ncp->nc_vp != NULL) {
		vlock = VNODE_TO_VIMPL(ncp->nc_vp)->vi_ncvlock;
		if (__predict_true(dirtovnode)) {
			rw_enter(vlock, RW_WRITER);
		}
		TAILQ_REMOVE(&VNODE_TO_VIMPL(ncp->nc_vp)->vi_nclist, ncp,
		    nc_vlist);
		if (__predict_true(dirtovnode)) {
			rw_exit(vlock);
		}
		ncp->nc_vp = NULL;
	}

	/* Finally, free it. */
	if (ncp->nc_nlen > NCHNAMLEN) {
		size_t sz = offsetof(struct namecache, nc_name[ncp->nc_nlen]);
		kmem_free(ncp, sz);
	} else {
		pool_cache_put(cache_pool, ncp);
	}
}

/*
 * Try to balance the LRU lists.  Pick some victim entries, and re-queue
 * them from the head of the ACTIVE list to the tail of the INACTIVE list. 
 */
static void __noinline
cache_deactivate(void)
{
	struct namecache *ncp;
	int total, i;

	KASSERT(mutex_owned(&cache_lru_lock));

	/* If we're nowhere near budget yet, don't bother. */
	total = cache_lru.count[LRU_ACTIVE] + cache_lru.count[LRU_INACTIVE];
	if (total < (desiredvnodes >> 1)) {
	    	return;
	}

	/* If not out of balance, don't bother. */
	if (cache_lru.count[LRU_ACTIVE] < cache_lru.count[LRU_INACTIVE]) {
		return;
	}

	/* Move victim from head of ACTIVE list, to tail of INACTIVE. */
	for (i = 0; i < cache_lru_maxdeact; i++) {
		ncp = TAILQ_FIRST(&cache_lru.list[LRU_ACTIVE]);
		if (ncp == NULL) {
			break;
		}
		KASSERT(ncp->nc_lrulist == LRU_ACTIVE);
		ncp->nc_lrulist = LRU_INACTIVE;
		TAILQ_REMOVE(&cache_lru.list[LRU_ACTIVE], ncp, nc_lru);
		TAILQ_INSERT_TAIL(&cache_lru.list[LRU_INACTIVE], ncp, nc_lru);
		cache_lru.count[LRU_ACTIVE]--;
		cache_lru.count[LRU_INACTIVE]++;
	}
}

/*
 * Re-queue an entry onto the correct LRU list, after it has scored a hit.
 */
static void __noinline
cache_activate(struct namecache *ncp)
{

	mutex_enter(&cache_lru_lock);
	/* Put on tail of ACTIVE queue, since it just scored a hit. */
	TAILQ_REMOVE(&cache_lru.list[ncp->nc_lrulist], ncp, nc_lru);
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], ncp, nc_lru);
	cache_lru.count[ncp->nc_lrulist]--;
	cache_lru.count[LRU_ACTIVE]++;
	ncp->nc_lrulist = LRU_ACTIVE;
	mutex_exit(&cache_lru_lock);
}

/*
 * Find a single cache entry and return it locked.  The directory lock must
 * be held.
 */
static struct namecache *
cache_lookup_entry(struct vnode *dvp, const char *name, size_t namelen)
{
	struct namecache *ncp;
	struct iovec iov;

	KASSERT(rw_lock_held(VNODE_TO_VIMPL(dvp)->vi_ncdlock));

	iov.iov_base = __UNCONST(name);
	iov.iov_len = namelen;
	ncp = rb_tree_find_node(&VNODE_TO_VIMPL(dvp)->vi_nctree, &iov);

	if (ncp != NULL) {
		KASSERT(ncp->nc_dvp == dvp);
		/* If the entry is on the wrong LRU list, requeue it. */
		if (__predict_false(ncp->nc_lrulist != LRU_ACTIVE)) {
			cache_activate(ncp);
		}
		SDT_PROBE(vfs, namecache, lookup, hit, dvp,
		    name, namelen, 0, 0);
	} else {
		SDT_PROBE(vfs, namecache, lookup, miss, dvp,
		    name, namelen, 0, 0);
	}
	return ncp;
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
bool
cache_lookup(struct vnode *dvp, const char *name, size_t namelen,
	     uint32_t nameiop, uint32_t cnflags,
	     int *iswht_ret, struct vnode **vn_ret)
{
	struct namecache *ncp;
	struct vnode *vp;
	krwlock_t *dlock;
	int error;
	bool hit;
	krw_t op;

	/* Establish default result values */
	if (iswht_ret != NULL) {
		*iswht_ret = 0;
	}
	*vn_ret = NULL;

	if (__predict_false(!doingcache)) {
		return false;
	}

	if (__predict_false(namelen > USHRT_MAX)) {
		SDT_PROBE(vfs, namecache, lookup, toolong, dvp,
		    name, namelen, 0, 0);
		COUNT(ncs_long);
		/* found nothing */
		return false;
	}

	/* Could the entry be purged below? */
	if ((cnflags & ISLASTCN) != 0 &&
	    ((cnflags & MAKEENTRY) == 0 || nameiop == CREATE)) {
	    	op = RW_WRITER;
	} else {
		op = RW_READER;
	}

	/* If no directory lock yet, there's nothing in cache. */
	dlock = VNODE_TO_VIMPL(dvp)->vi_ncdlock;
	if (__predict_false(dlock == NULL)) {
		return false;
	}

	rw_enter(dlock, op);
	ncp = cache_lookup_entry(dvp, name, namelen);
	if (__predict_false(ncp == NULL)) {
		rw_exit(dlock);
		/* found nothing */
		COUNT(ncs_miss);
		return false;
	}
	if (__predict_false((cnflags & MAKEENTRY) == 0)) {
		/*
		 * Last component and we are renaming or deleting,
		 * the cache entry is invalid, or otherwise don't
		 * want cache entry to exist.
		 */
		cache_remove(ncp, true);
		rw_exit(dlock);
		/* found nothing */
		COUNT(ncs_badhits);
		return false;
	}
	if (__predict_false(ncp->nc_vp == NULL)) {
		if (__predict_true(nameiop != CREATE ||
		    (cnflags & ISLASTCN) == 0)) {
			COUNT(ncs_neghits);
			/* found neg entry; vn is already null from above */
			hit = true;
		} else {
			/*
			 * Last component and we are preparing to create
			 * the named object, so flush the negative cache
			 * entry.
			 */
			COUNT(ncs_badhits);
			cache_remove(ncp, true);
			/* found nothing */
			hit = false;
		}
		if (iswht_ret != NULL) {
			/*
			 * Restore the ISWHITEOUT flag saved earlier.
			 */
			*iswht_ret = ncp->nc_whiteout;
		} else {
			KASSERT(!ncp->nc_whiteout);
		}
		rw_exit(dlock);
		return hit;
	}
	vp = ncp->nc_vp;
	mutex_enter(vp->v_interlock);
	rw_exit(dlock);

	/*
	 * Unlocked except for the vnode interlock.  Call vcache_tryvget().
	 */
	error = vcache_tryvget(vp);
	if (error) {
		KASSERT(error == EBUSY);
		/*
		 * This vnode is being cleaned out.
		 * XXX badhits?
		 */
		COUNT(ncs_falsehits);
		/* found nothing */
		return false;
	}

	COUNT(ncs_goodhits);
	/* found it */
	*vn_ret = vp;
	return true;
}

/*
 * Cut-'n-pasted version of the above without the nameiop argument.
 *
 * This could also be used for namei's LOOKUP case, i.e. when the directory
 * is not being modified.
 */
bool
cache_lookup_raw(struct vnode *dvp, const char *name, size_t namelen,
		 uint32_t cnflags,
		 int *iswht_ret, struct vnode **vn_ret)
{
	struct namecache *ncp;
	struct vnode *vp;
	krwlock_t *dlock;
	int error;

	/* Establish default results. */
	if (iswht_ret != NULL) {
		*iswht_ret = 0;
	}
	*vn_ret = NULL;

	if (__predict_false(!doingcache)) {
		/* found nothing */
		return false;
	}

	/* If no directory lock yet, there's nothing in cache. */
	dlock = VNODE_TO_VIMPL(dvp)->vi_ncdlock;
	if (__predict_false(dlock == NULL)) {
		return false;
	}

	rw_enter(dlock, RW_READER);
	if (__predict_false(namelen > USHRT_MAX)) {
		rw_exit(dlock);
		/* found nothing */
		COUNT(ncs_long);
		return false;
	}
	ncp = cache_lookup_entry(dvp, name, namelen);
	if (__predict_false(ncp == NULL)) {
		rw_exit(dlock);
		/* found nothing */
		COUNT(ncs_miss);
		return false;
	}
	vp = ncp->nc_vp;
	if (vp == NULL) {
		/*
		 * Restore the ISWHITEOUT flag saved earlier.
		 */
		if (iswht_ret != NULL) {
			/*cnp->cn_flags |= ncp->nc_flags;*/
			*iswht_ret = ncp->nc_whiteout;
		}
		rw_exit(dlock);
		/* found negative entry; vn is already null from above */
		COUNT(ncs_neghits);
		return true;
	}
	mutex_enter(vp->v_interlock);
	rw_exit(dlock);

	/*
	 * Unlocked except for the vnode interlock.  Call vcache_tryvget().
	 */
	error = vcache_tryvget(vp);
	if (error) {
		KASSERT(error == EBUSY);
		/*
		 * This vnode is being cleaned out.
		 * XXX badhits?
		 */
		COUNT(ncs_falsehits);
		/* found nothing */
		return false;
	}

	COUNT(ncs_goodhits); /* XXX can be "badhits" */
	/* found it */
	*vn_ret = vp;
	return true;
}

/*
 * Scan cache looking for name of directory entry pointing at vp.
 * Will not search for "." or "..".
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
	krwlock_t *vlock;
	char *bp;
	int error, nlen;

	KASSERT(vp != NULL);

	if (!doingcache)
		goto out;

	vlock = VNODE_TO_VIMPL(vp)->vi_ncvlock;
	rw_enter(vlock, RW_READER);
	TAILQ_FOREACH(ncp, &VNODE_TO_VIMPL(vp)->vi_nclist, nc_vlist) {
		KASSERT(ncp->nc_vp == vp);
		KASSERT(ncp->nc_dvp != NULL);
		nlen = ncp->nc_nlen;
		/*
		 * The queue is partially sorted.  Once we hit dots, nothing
		 * else remains but dots and dotdots, so bail out.
		 */
		if (ncp->nc_name[0] == '.') {
			if (nlen == 1 ||
			    (nlen == 2 && ncp->nc_name[1] == '.')) {
			    	break;
			}
		}

		/* Record a hit on the entry. */
		if (ncp->nc_lrulist != LRU_ACTIVE) {
			cache_activate(ncp);
		}

		if (bufp) {
			bp = *bpp;
			bp -= nlen;
			if (bp <= bufp) {
				*dvpp = NULL;
				rw_exit(vlock);
				SDT_PROBE(vfs, namecache, revlookup,
				    fail, vp, ERANGE, 0, 0, 0);
				return (ERANGE);
			}
			memcpy(bp, ncp->nc_name, nlen);
			*bpp = bp;
		}

		dvp = ncp->nc_dvp;
		mutex_enter(dvp->v_interlock);
		rw_exit(vlock);
		error = vcache_tryvget(dvp);
		if (error) {
			KASSERT(error == EBUSY);
			if (bufp)
				(*bpp) += nlen;
			*dvpp = NULL;
			SDT_PROBE(vfs, namecache, revlookup, fail, vp,
			    error, 0, 0, 0);
			return -1;
		}
		*dvpp = dvp;
		SDT_PROBE(vfs, namecache, revlookup, success, vp, dvp,
		    0, 0, 0);
		COUNT(ncs_revhits);
		return (0);
	}
	rw_exit(vlock);
	COUNT(ncs_revmiss);
 out:
	*dvpp = NULL;
	return (-1);
}

/*
 * Allocate and install a directory lock for a vnode.
 * XXX Be better to do this when setting vnode type to VDIR.
 * XXX In the future this could be an "nchdir" - to try other data structs.
 */
static krwlock_t *
cache_alloc_dirlock(struct vnode *dvp)
{
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	krwlock_t *dlock;

	/*
	 * File systems are not generally expected to make concurrent calls
	 * to cache_enter(), but don't needlessly impose that limitation on
	 * them.
	 */
	for (;;) {
		dlock = dvi->vi_ncdlock;
		if (dlock != NULL) {
			return dlock;
		}
		dlock = rw_obj_alloc();
		if (atomic_cas_ptr(&dvi->vi_ncdlock, NULL, dlock) == NULL) {
			return dlock;
		}
		rw_obj_free(dlock);
	}
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
	vnode_impl_t *vi, *dvi;
	krwlock_t *dlock, *vlock;
	int total;

	/* First, check whether we can/should add a cache entry. */
	if ((cnflags & MAKEENTRY) == 0 ||
	    __predict_false(namelen > USHRT_MAX || !doingcache)) {
		SDT_PROBE(vfs, namecache, enter, toolong, vp, name, namelen,
		    0, 0);
		return;
	}

	SDT_PROBE(vfs, namecache, enter, done, vp, name, namelen, 0, 0);

	/*
	 * Reclaim some entries if over budget.  This is an unlocked check,
	 * but it doesn't matter.  Just need to catch up with things
	 * eventually: it doesn't matter if we go over temporarily.
	 */
	total = cache_lru.count[LRU_ACTIVE] + cache_lru.count[LRU_INACTIVE];
	if (__predict_false(total > desiredvnodes)) {
		cache_reclaim();
	}

	/* Now allocate a fresh entry. */
	if (namelen > NCHNAMLEN) {
		size_t sz = offsetof(struct namecache, nc_name[namelen]);
		ncp = kmem_alloc(sz, KM_SLEEP);
	} else {
		ncp = pool_cache_get(cache_pool, PR_WAITOK);
	}

	/* If there is no directory lock yet, then allocate one. */
	dvi = VNODE_TO_VIMPL(dvp);
	dlock = dvi->vi_ncdlock;
	if (__predict_false(dlock == NULL)) {
		dlock = cache_alloc_dirlock(dvp);
	}

	/*
	 * Concurrent lookups in the same directory may race for a cache
	 * entry.  If there's a duplicated entry, free it.
	 */
	rw_enter(dlock, RW_WRITER);
	oncp = cache_lookup_entry(dvp, name, namelen);
	if (oncp) {
		cache_remove(oncp, true);
	}

	/* Fill in cache info and insert to directory. */
	ncp->nc_dvp = dvp;
	KASSERT(namelen <= USHRT_MAX);
	ncp->nc_nlen = namelen;
	memcpy(ncp->nc_name, name, (unsigned)ncp->nc_nlen);
	rb_tree_insert_node(&dvi->vi_nctree, ncp);
	ncp->nc_vp = vp;

	/* Insert to the vnode. */
	if (vp == NULL) {
		/*
		 * For negative hits, save the ISWHITEOUT flag so we can
		 * restore it later when the cache entry is used again.
		 */
		ncp->nc_whiteout = ((cnflags & ISWHITEOUT) != 0);
	} else {
		ncp->nc_whiteout = false;
		vi = VNODE_TO_VIMPL(vp);
		vlock = vi->vi_ncvlock;
		/* Partially sort the per-vnode list: dots go to back. */
		rw_enter(vlock, RW_WRITER);
		if ((namelen == 1 && name[0] == '.') ||
		    (namelen == 2 && name[0] == '.' && name[1] == '.')) {
			TAILQ_INSERT_TAIL(&vi->vi_nclist, ncp, nc_vlist);
		} else {
			TAILQ_INSERT_HEAD(&vi->vi_nclist, ncp, nc_vlist);
		}
		rw_exit(vlock);
	}

	/*
	 * Finally, insert to the tail of the ACTIVE LRU list (new) and with
	 * the LRU lock held take the to opportunity to incrementally
	 * balance the lists.
	 */
	mutex_enter(&cache_lru_lock);
	ncp->nc_lrulist = LRU_ACTIVE;
	cache_lru.count[LRU_ACTIVE]++;
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], ncp, nc_lru);
	cache_deactivate();
	mutex_exit(&cache_lru_lock);
	rw_exit(dlock);
}

/*
 * Name cache initialization, from vfs_init() when we are booting.
 */
void
nchinit(void)
{

	cache_pool = pool_cache_init(sizeof(struct namecache),
	    coherency_unit, 0, 0, "ncache", NULL, IPL_NONE, NULL,
	    NULL, NULL);
	KASSERT(cache_pool != NULL);

	mutex_init(&cache_lru_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&cache_lru.list[LRU_ACTIVE]);
	TAILQ_INIT(&cache_lru.list[LRU_INACTIVE]);

	sysctl_cache_stat_setup();
}

/*
 * Called once for each CPU in the system as attached.
 */
void
cache_cpu_init(struct cpu_info *ci)
{
	void *p;
	size_t sz;

	sz = roundup2(sizeof(struct nchstats_percpu), coherency_unit) +
	    coherency_unit;
	p = kmem_zalloc(sz, KM_SLEEP);
	ci->ci_data.cpu_nch = (void *)roundup2((uintptr_t)p, coherency_unit);
}

/*
 * A vnode is being allocated: set up cache structures.
 */
void
cache_vnode_init(struct vnode *vp)
{
	vnode_impl_t *vi = VNODE_TO_VIMPL(vp);

	vi->vi_ncdlock = NULL;
	vi->vi_ncvlock = rw_obj_alloc();
	rb_tree_init(&vi->vi_nctree, &cache_rbtree_ops);
	TAILQ_INIT(&vi->vi_nclist);
}

/*
 * A vnode is being freed: finish cache structures.
 */
void
cache_vnode_fini(struct vnode *vp)
{
	vnode_impl_t *vi = VNODE_TO_VIMPL(vp);

	KASSERT(RB_TREE_MIN(&vi->vi_nctree) == NULL);
	KASSERT(TAILQ_EMPTY(&vi->vi_nclist));
	rw_obj_free(vi->vi_ncvlock);
	if (vi->vi_ncdlock != NULL) {
		rw_obj_free(vi->vi_ncdlock);
	}
}

/*
 * Cache flush, a particular vnode; called when a vnode is renamed to
 * hide entries that would now be invalid
 */
void
cache_purge1(struct vnode *vp, const char *name, size_t namelen, int flags)
{
	struct namecache *ncp;
	krwlock_t *dlock, *vlock, *blocked;
	rb_tree_t *tree;

	if (flags & PURGE_PARENTS) {
		SDT_PROBE(vfs, namecache, purge, parents, vp, 0, 0, 0, 0);
		blocked = NULL;
		vlock = VNODE_TO_VIMPL(vp)->vi_ncvlock;
		rw_enter(vlock, RW_WRITER);
		while ((ncp = TAILQ_FIRST(&VNODE_TO_VIMPL(vp)->vi_nclist))
		    != NULL) {
			dlock = VNODE_TO_VIMPL(ncp->nc_dvp)->vi_ncdlock;
			KASSERT(dlock != NULL);
			/*
			 * We can't wait on the directory lock with the 
			 * list lock held or the system could
			 * deadlock.  In the unlikely event that the
			 * directory lock is unavailable, take a hold on it
			 * and wait for it to become available with no other
			 * locks held.  Then retry.  If this happens twice
			 * in a row, we'll give the other side a breather to
			 * prevent livelock.
			 */
			if (!rw_tryenter(dlock, RW_WRITER)) {
				rw_obj_hold(dlock);
				rw_exit(vlock);
				rw_enter(dlock, RW_WRITER);
				/* Do nothing. */
				rw_exit(dlock);
				rw_obj_free(dlock);
				if (blocked == dlock) {
					kpause("livelock", false, 1, NULL);
				}
				rw_enter(dlock, RW_WRITER);
				blocked = dlock;
			} else {
				cache_remove(ncp, false);
				rw_exit(dlock);
				blocked = NULL;
			}
		}
		rw_exit(vlock);
	}
	if (flags & PURGE_CHILDREN) {
		SDT_PROBE(vfs, namecache, purge, children, vp, 0, 0, 0, 0);
		dlock = VNODE_TO_VIMPL(vp)->vi_ncdlock;
		if (dlock != NULL) {
			rw_enter(dlock, RW_WRITER);
			tree = &VNODE_TO_VIMPL(vp)->vi_nctree;
			while ((ncp = rb_tree_iterate(tree, NULL,
			    RB_DIR_RIGHT)) != NULL) {
				cache_remove(ncp, true);
			}
			rw_exit(dlock);
		}
	}
	if (name != NULL) {
		SDT_PROBE(vfs, namecache, purge, name, name, namelen, 0, 0, 0);
		dlock = VNODE_TO_VIMPL(vp)->vi_ncdlock;
		if (dlock != NULL) {
			rw_enter(dlock, RW_WRITER);
			ncp = cache_lookup_entry(vp, name, namelen);
			if (ncp) {
				cache_remove(ncp, true);
			}
			rw_exit(dlock);
		}
	}
}

/*
 * Cache flush, a whole filesystem; called when filesys is umounted to
 * remove entries that would now be invalid.
 */
void
cache_purgevfs(struct mount *mp)
{
	struct vnode_iterator *marker;
	struct namecache *ncp;
	vnode_impl_t *vi;
	krwlock_t *dlock;
	vnode_t *vp;

	/*
	 * In the original BSD implementation this used to traverse the LRU
	 * list.  To avoid locking difficulties, and avoid scanning a ton of
	 * namecache entries that we have no interest in, scan the mount
	 * list instead.
	 */
	vfs_vnode_iterator_init(mp, &marker);
	while ((vp = vfs_vnode_iterator_next(marker, NULL, NULL))) {
		vi = VNODE_TO_VIMPL(vp);
		if (vp->v_type != VDIR) {
			KASSERT(RB_TREE_MIN(&vi->vi_nctree) == NULL);
			continue;
		}
		if ((dlock = vi->vi_ncdlock) == NULL) {
			continue;
		}
		rw_enter(dlock, RW_WRITER);
		while ((ncp = rb_tree_iterate(&vi->vi_nctree, NULL,
		    RB_DIR_RIGHT)) != NULL) {
			cache_remove(ncp, true);
		}
		rw_exit(dlock);
	}
	vfs_vnode_iterator_destroy(marker);
}

/*
 * Free some entries from the cache, when we have gone over budget.
 *
 * We don't want to cause too much work for any individual caller, and it
 * doesn't matter if we temporarily go over budget.  This is also "just a
 * cache" so it's not a big deal if we screw up and throw out something we
 * shouldn't.  So we take a relaxed attitude to this process to reduce its
 * impact.
 */
static void
cache_reclaim(void)
{
	struct namecache *ncp;
	krwlock_t *dlock;
	int toscan, total;

	/* Scan up to a preset maxium number of entries. */
	mutex_enter(&cache_lru_lock);
	toscan = cache_lru_maxscan;
	total = cache_lru.count[LRU_ACTIVE] + cache_lru.count[LRU_INACTIVE];
	SDT_PROBE(vfs, namecache, prune, done, total, toscan, 0, 0, 0);
	while (toscan-- != 0) {
		/* First try to balance the lists. */
		cache_deactivate();

		/* Now look for a victim on head of inactive list (old). */
		ncp = TAILQ_FIRST(&cache_lru.list[LRU_INACTIVE]);
		if (ncp == NULL) {
			break;
		}
		dlock = VNODE_TO_VIMPL(ncp->nc_dvp)->vi_ncdlock;
		KASSERT(ncp->nc_lrulist == LRU_INACTIVE);
		KASSERT(dlock != NULL);

		/*
		 * Locking in the wrong direction.  If we can't get the
		 * lock, the directory is actively busy, and it could also
		 * cause problems for the next guy in here, so send the
		 * entry to the back of the list.
		 */
		if (!rw_tryenter(dlock, RW_WRITER)) {
			TAILQ_REMOVE(&cache_lru.list[LRU_INACTIVE],
			    ncp, nc_lru);
			TAILQ_INSERT_TAIL(&cache_lru.list[LRU_INACTIVE],
			    ncp, nc_lru);
			continue;
		}

		/*
		 * Now have the victim entry locked.  Drop the LRU list
		 * lock, purge the entry, and start over.  The hold on the
		 * directory lock will prevent the directory lock itself
		 * from vanishing until finished (the directory owner
		 * will call cache_purge() on dvp before it disappears).
		 */
		mutex_exit(&cache_lru_lock);
		cache_remove(ncp, true);
		rw_exit(dlock);
		mutex_enter(&cache_lru_lock);
	}
	mutex_exit(&cache_lru_lock);
}

#ifdef DDB
void
namecache_print(struct vnode *vp, void (*pr)(const char *, ...))
{
	struct vnode *dvp = NULL;
	struct namecache *ncp;
	enum cache_lru_id id;

	for (id = 0; id < LRU_COUNT; id++) {
		TAILQ_FOREACH(ncp, &cache_lru.list[id], nc_lru) {
			if (ncp->nc_vp == vp) {
				(*pr)("name %.*s\n", ncp->nc_nlen,
				    ncp->nc_name);
				dvp = ncp->nc_dvp;
			}
		}
	}
	if (dvp == NULL) {
		(*pr)("name not found\n");
		return;
	}
	vp = dvp;
	for (id = 0; id < LRU_COUNT; id++) {
		TAILQ_FOREACH(ncp, &cache_lru.list[id], nc_lru) {
			if (ncp->nc_vp == vp) {
				(*pr)("parent %.*s\n", ncp->nc_nlen,
				    ncp->nc_name);
			}
		}
	}
}
#endif

void
namecache_count_pass2(void)
{

	COUNT(ncs_pass2);
}

void
namecache_count_2passes(void)
{

	COUNT(ncs_2passes);
}

/*
 * Fetch the current values of the stats.  We return the most
 * recent values harvested into nchstats by cache_reclaim(), which
 * will be less than a second old.
 */
static int
cache_stat_sysctl(SYSCTLFN_ARGS)
{
	struct nchstats stats;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	if (oldp == NULL) {
		*oldlenp = sizeof(stats);
		return 0;
	}

	if (*oldlenp < sizeof(stats)) {
		*oldlenp = 0;
		return 0;
	}

	sysctl_unlock();
	memset(&stats, 0, sizeof(stats));
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct nchstats_percpu *np = ci->ci_data.cpu_nch;

		stats.ncs_goodhits += np->ncs_goodhits;
		stats.ncs_neghits += np->ncs_neghits;
		stats.ncs_badhits += np->ncs_badhits;
		stats.ncs_falsehits += np->ncs_falsehits;
		stats.ncs_miss += np->ncs_miss;
		stats.ncs_long += np->ncs_long;
		stats.ncs_pass2 += np->ncs_pass2;
		stats.ncs_2passes += np->ncs_2passes;
		stats.ncs_revhits += np->ncs_revhits;
		stats.ncs_revmiss += np->ncs_revmiss;
	}
	/* Serialize the update to nchstats, just because. */
	mutex_enter(&cache_lru_lock);
	nchstats = stats;
	mutex_exit(&cache_lru_lock);
	sysctl_relock();

	*oldlenp = sizeof(stats);
	return sysctl_copyout(l, &stats, oldp, sizeof(stats));
}

static void
sysctl_cache_stat_setup(void)
{

	KASSERT(sysctllog == NULL);
	sysctl_createv(&sysctllog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "namecache_stats",
		       SYSCTL_DESCR("namecache statistics"),
		       cache_stat_sysctl, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE, CTL_EOL);
}
