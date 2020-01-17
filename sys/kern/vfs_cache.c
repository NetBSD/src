/*	$NetBSD: vfs_cache.c,v 1.126.2.6 2020/01/17 22:26:25 ad Exp $	*/

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
 * Name caching:
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
 *	works very well but can cause difficulties for the CPU cache on
 *	modern CPUs, and concurrency headaches for the kernel hacker on
 *	multiprocessor systems.  The global hash table is also difficult to
 *	size dynamically.  To try and address these concerns, the focus of
 *	interest in this implementation is the directory itself: a
 *	per-directory red-black tree is used to look up names.  Other than
 *	the choice of data structure, it works largely the same way as the
 *	BSD implementation.
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
 *	2) nc_dvp->vi_ncvlock
 *	3) cache_lru_lock, vp->v_interlock
 *
 * Ugly ASCII diagram:
 *
 *	XXX replace tabs with spaces, make less ugly
 *
 *          ...
 *           ^
 *           |
 *      -----o-----
 *      |  VDIR   |
 *      | nchnode |
 *      -----------
 *           ^
 *           |- nd_tree
 *           |                                                           
 *      -----o-----               -----------               -----------
 *      |  VDIR   |               |  VCHR   |               |  VREG   |
 *      | nchnode o-----+         | nchnode o-----+         | nchnode o------+
 *      -----------     |         -----------     |         -----------      |
 *           ^          |              ^          |              ^           |
 *           |- nc_nn   |- nn_list     |- nc_nn   |- nn_list     |- nc_nn    |
 *           |          |              |          |              |           |
 *      -----o-----     |         -----o-----     |         -----o-----      |
 *  +---onamecache|<----+     +---onamecache|<----+     +---onamecache|<-----+
 *  |   -----------           |   -----------           |   -----------
 *  |        ^                |        ^                |        ^
 *  |        |                |        |                |        |
 *  |        |  +----------------------+                |        |
 *  |-nc_dnn | +-------------------------------------------------+
 *  |        |/- nd_tree      |                         |
 *  |        |                |- nc_dnn                 |- nc_dnn
 *  |   -----o-----           |                         |
 *  +-->|  VDIR   |<----------+                         |
 *      | nchnode |<------------------------------------+
 *      -----------
 *
 *      START HERE
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cache.c,v 1.126.2.6 2020/01/17 22:26:25 ad Exp $");

#define __NAMECACHE_PRIVATE
#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_dtrace.h"
#endif

#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/hash.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/param.h>
#include <sys/pool.h>
#include <sys/sdt.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode_impl.h>

#include <miscfs/genfs/genfs.h>

/*
 * Per-vnode state for the namecache.  This is allocated apart from struct
 * vnode to make the best use of memory, and best use of CPU cache.  Field
 * markings and corresponding locks:
 *
 *	-	stable throught the lifetime of the vnode
 *	n	protected by nn_lock
 *	l	protected by nn_listlock
 */
struct nchnode {
	/* First cache line: frequently used stuff. */
	rb_tree_t	nn_tree;	/* n  namecache tree */
	TAILQ_HEAD(,namecache) nn_list;	/* l  namecaches (parent) */
	mode_t		nn_mode;	/* n  cached mode or VNOVAL */
	uid_t		nn_uid;		/* n  cached UID or VNOVAL */
	gid_t		nn_gid;		/* n  cached GID or VNOVAL */
	uint32_t	nn_spare;	/* -  spare (padding) */

	/* Second cache line: locks and infrequenly used stuff. */
	krwlock_t	nn_lock		/* -  lock on node */
	    __aligned(COHERENCY_UNIT);
	krwlock_t	nn_listlock;	/* -  lock on nn_list */
	struct vnode	*nn_vp;		/* -  backpointer to vnode */
};

static void	cache_activate(struct namecache *);
static int	cache_compare_key(void *, const void *, const void *);
static int	cache_compare_nodes(void *, const void *, const void *);
static void	cache_deactivate(void);
static void	cache_reclaim(void);
static int	cache_stat_sysctl(SYSCTLFN_ARGS);

/* Per-CPU counters. */
struct nchstats_percpu _NAMEI_CACHE_STATS(uintptr_t);

/* Global pool cache. */
static pool_cache_t cache_pool __read_mostly;
static pool_cache_t cache_node_pool __read_mostly;

/* LRU replacement. */
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

#define	COUNT(f)	do { \
	kpreempt_disable(); \
	((struct nchstats_percpu *)curcpu()->ci_data.cpu_nch)->f++; \
	kpreempt_enable(); \
} while (/* CONSTCOND */ 0);

/* Tunables */
static const int cache_lru_maxdeact = 2;	/* max # to deactivate */
static const int cache_lru_maxscan = 128;	/* max # to scan/reclaim */
static int doingcache = 1;			/* 1 => enable the cache */

/* sysctl */
static struct	sysctllog *cache_sysctllog;

/* Read-black tree */
static rb_tree_ops_t cache_rbtree_ops __read_mostly = {
	.rbto_compare_nodes = cache_compare_nodes,
	.rbto_compare_key = cache_compare_key,
	.rbto_node_offset = offsetof(struct namecache, nc_tree),
	.rbto_context = NULL
};

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
	const struct namecache *nc1 = n1;
	const struct namecache *nc2 = n2;

	if (nc1->nc_key < nc2->nc_key) {
		return -1;
	}
	if (nc1->nc_key > nc2->nc_key) {
		return 1;
	}
	return 0;
}

/*
 * rbtree: compare a node and a key.
 */
static int
cache_compare_key(void *context, const void *n, const void *k)
{
	const struct namecache *nc = n;
	const int64_t key = *(const int64_t *)k;

	if (nc->nc_key < key) {
		return -1;
	}
	if (nc->nc_key > key) {
		return 1;
	}
	return 0;
}

/*
 * Compute a (with luck) unique key value for the given name.  The name
 * length is encoded in the key value to try and improve uniqueness, and so
 * that length doesn't need to be compared separately for subsequent string
 * comparisons.
 */
static int64_t
cache_key(const char *name, size_t nlen)
{
	int64_t key;

	KASSERT(nlen <= USHRT_MAX);

	key = hash32_buf(name, nlen, HASH32_STR_INIT);
	return (key << 16) | nlen;
}

/*
 * Remove an entry from the cache.  The directory lock must be held, and if
 * "dir2node" is true, then we're locking in the conventional direction and
 * the list lock will be acquired when removing the entry from the vnode
 * list.
 */
static void
cache_remove(struct namecache *nc, const bool dir2node)
{
	struct nchnode *dnn = nc->nc_dnn;
	struct nchnode *nn;

	KASSERT(rw_write_held(&dnn->nn_lock));
	KASSERT(cache_key(nc->nc_name, nc->nc_nlen) == nc->nc_key);
	KASSERT(rb_tree_find_node(&dnn->nn_tree, &nc->nc_key) == nc);

	SDT_PROBE(vfs, namecache, invalidate, done, nc,
	    0, 0, 0, 0);

	/* First remove from the directory's rbtree. */
	rb_tree_remove_node(&dnn->nn_tree, nc);

	/* Then remove from the LRU lists. */
	mutex_enter(&cache_lru_lock);
	TAILQ_REMOVE(&cache_lru.list[nc->nc_lrulist], nc, nc_lru);
	cache_lru.count[nc->nc_lrulist]--;
	mutex_exit(&cache_lru_lock);

	/* Then remove from the node's list. */
	if ((nn = nc->nc_nn) != NULL) {
		if (__predict_true(dir2node)) {
			rw_enter(&nn->nn_listlock, RW_WRITER);
			TAILQ_REMOVE(&nn->nn_list, nc, nc_list);
			rw_exit(&nn->nn_listlock);
		} else {
			TAILQ_REMOVE(&nn->nn_list, nc, nc_list);
		}
	}

	/* Finally, free it. */
	if (nc->nc_nlen > NCHNAMLEN) {
		size_t sz = offsetof(struct namecache, nc_name[nc->nc_nlen]);
		kmem_free(nc, sz);
	} else {
		pool_cache_put(cache_pool, nc);
	}
}

/*
 * Find a single cache entry and return it locked.  The directory lock must
 * be held.
 *
 * Marked __noinline, and with everything unnecessary excluded, the compiler
 * (gcc 8.3.0 x86_64) does a great job on this.
 */
static struct namecache * __noinline
cache_lookup_entry(struct nchnode *dnn, const char *name, size_t namelen,
    int64_t key)
{
	struct rb_node *node = dnn->nn_tree.rbt_root;
	struct namecache *nc;

	KASSERT(rw_lock_held(&dnn->nn_lock));

	/*
	 * Search the RB tree for the key.  This is one of the most
	 * performance sensitive code paths in the system, so here is an
	 * inlined version of rb_tree_find_node() tailored for exactly
	 * what's needed here (64-bit key and so on).  Elsewhere during
	 * entry/removal the generic functions are used as it doesn't
	 * matter so much there.
	 */
	for (;;) {
		if (__predict_false(RB_SENTINEL_P(node))) {
			return NULL;
		}
		KASSERT((void *)&nc->nc_tree == (void *)nc);
		nc = (struct namecache *)node;
		KASSERT(nc->nc_dnn == dnn);
		if (nc->nc_key == key) {
			break;
		}
		node = node->rb_nodes[nc->nc_key < key];
	}

	/* Exclude collisions. */
	KASSERT(nc->nc_nlen == namelen);
	if (__predict_false(memcmp(nc->nc_name, name, namelen) != 0)) {
		return NULL;
	}

	/*
	 * If the entry is on the wrong LRU list, requeue it.  This is an
	 * unlocked check, but it will rarely be wrong and even then there
	 * will be no harm caused.
	 */
	if (__predict_false(nc->nc_lrulist != LRU_ACTIVE)) {
		cache_activate(nc);
	}
	return nc;
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
	struct nchnode *dnn = VNODE_TO_VIMPL(dvp)->vi_ncache;
	struct namecache *nc;
	struct vnode *vp;
	int64_t key;
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
		return false;
	}

	/* Could the entry be purged below? */
	if ((cnflags & ISLASTCN) != 0 &&
	    ((cnflags & MAKEENTRY) == 0 || nameiop == CREATE)) {
	    	op = RW_WRITER;
	} else {
		op = RW_READER;
	}

	/* Compute the key up front - don't need the lock. */
	key = cache_key(name, namelen);

	/* Now look for the name. */
	rw_enter(&dnn->nn_lock, op);
	nc = cache_lookup_entry(dnn, name, namelen, key);
	if (__predict_false(nc == NULL)) {
		rw_exit(&dnn->nn_lock);
		COUNT(ncs_miss);
		SDT_PROBE(vfs, namecache, lookup, miss, dvp,
		    name, namelen, 0, 0);
		return false;
	}
	if (__predict_false((cnflags & MAKEENTRY) == 0)) {
		/*
		 * Last component and we are renaming or deleting,
		 * the cache entry is invalid, or otherwise don't
		 * want cache entry to exist.
		 */
		KASSERT((cnflags & ISLASTCN) != 0);
		cache_remove(nc, true);
		rw_exit(&dnn->nn_lock);
		COUNT(ncs_badhits);
		return false;
	}
	if (nc->nc_nn == NULL) {
		if (nameiop == CREATE && (cnflags & ISLASTCN) != 0) {
			/*
			 * Last component and we are preparing to create
			 * the named object, so flush the negative cache
			 * entry.
			 */
			COUNT(ncs_badhits);
			cache_remove(nc, true);
			hit = false;
		} else {
			COUNT(ncs_neghits);
			SDT_PROBE(vfs, namecache, lookup, hit, dvp, name,
			    namelen, 0, 0);
			/* found neg entry; vn is already null from above */
			hit = true;
		}
		if (iswht_ret != NULL) {
			/*
			 * Restore the ISWHITEOUT flag saved earlier.
			 */
			*iswht_ret = nc->nc_whiteout;
		} else {
			KASSERT(!nc->nc_whiteout);
		}
		rw_exit(&dnn->nn_lock);
		return hit;
	}
	vp = nc->nc_vp;
	mutex_enter(vp->v_interlock);
	rw_exit(&dnn->nn_lock);

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
		return false;
	}

	COUNT(ncs_goodhits);
	SDT_PROBE(vfs, namecache, lookup, hit, dvp, name, namelen, 0, 0);
	/* found it */
	*vn_ret = vp;
	return true;
}

/*
 * Version of the above without the nameiop argument, for NFS.
 */
bool
cache_lookup_raw(struct vnode *dvp, const char *name, size_t namelen,
		 uint32_t cnflags,
		 int *iswht_ret, struct vnode **vn_ret)
{

	return cache_lookup(dvp, name, namelen, LOOKUP, cnflags | MAKEENTRY,
	    iswht_ret, vn_ret);
}

/*
 * Used by namei() to walk down a path, component by component by looking up
 * names in the cache.  The node locks are chained along the way: a parent's
 * lock is not dropped until the child's is acquired.
 */
bool
cache_lookup_linked(struct vnode *dvp, const char *name, size_t namelen,
		    struct vnode **vn_ret, krwlock_t **plock,
		    kauth_cred_t cred)
{
	struct nchnode *dnn = VNODE_TO_VIMPL(dvp)->vi_ncache;
	struct namecache *nc;
	int64_t key;
	int error;

	/* Establish default results. */
	*vn_ret = NULL;

	/*
	 * If disabled, or FS doesn't support us, bail out.  This is an
	 * unlocked check of dnn->nn_mode, but an incorrect answer doesn't
	 * have any ill consequences.
	 */
	if (__predict_false(!doingcache || dnn->nn_mode == VNOVAL)) {
		return false;
	}

	if (__predict_false(namelen > USHRT_MAX)) {
		COUNT(ncs_long);
		return false;
	}

	/* Compute the key up front - don't need the lock. */
	key = cache_key(name, namelen);

	/*
	 * Acquire the directory lock.  Once we have that, we can drop the
	 * previous one (if any).
	 *
	 * The two lock holds mean that the directory can't go away while
	 * here: the directory must be purged first, and both parent &
	 * child's locks must be taken to do that.
	 *
	 * However if there's no previous lock, like at the root of the
	 * chain, then "dvp" must be referenced to prevent it going away
	 * before we get its lock.
	 *
	 * Note that the two locks can be the same if looking up a dot, for
	 * example: /usr/bin/.
	 */
	if (*plock != &dnn->nn_lock) {
		rw_enter(&dnn->nn_lock, RW_READER);
		if (*plock != NULL) {
			rw_exit(*plock);
		}
		*plock = &dnn->nn_lock;
	} else {
		KASSERT(dvp->v_usecount > 0);
	}

	/*
	 * First up check if the user is allowed to look up files in this
	 * directory.
	 */
	error = kauth_authorize_vnode(cred, KAUTH_ACCESS_ACTION(VEXEC,
	    dvp->v_type, dnn->nn_mode & ALLPERMS), dvp, NULL,
	    genfs_can_access(dvp->v_type, dnn->nn_mode & ALLPERMS,
	    dnn->nn_uid, dnn->nn_gid, VEXEC, cred));
	if (error != 0) {
		COUNT(ncs_denied);
		return false;
	}

	/*
	 * Now look for a matching cache entry.
	 */
	nc = cache_lookup_entry(dnn, name, namelen, key);
	if (__predict_false(nc == NULL)) {
		COUNT(ncs_miss);
		SDT_PROBE(vfs, namecache, lookup, miss, dvp,
		    name, namelen, 0, 0);
		return false;
	}
	if (nc->nc_nn == NULL) {
		/* found negative entry; vn is already null from above */
		COUNT(ncs_neghits);
		SDT_PROBE(vfs, namecache, lookup, hit, dvp, name, namelen, 0, 0);
		return true;
	}

	COUNT(ncs_goodhits); /* XXX can be "badhits" */
	SDT_PROBE(vfs, namecache, lookup, hit, dvp, name, namelen, 0, 0);

	/*
	 * Return with the directory lock still held.  It will either be
	 * returned to us with another call to cache_lookup_linked() when
	 * looking up the next component, or the caller will release it
	 * manually when finished.
	 */
	*vn_ret = nc->nc_vp;
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
	struct nchnode *nn = VNODE_TO_VIMPL(vp)->vi_ncache;
	struct namecache *nc;
	struct vnode *dvp;
	char *bp;
	int error, nlen;

	KASSERT(vp != NULL);

	if (!doingcache)
		goto out;

	rw_enter(&nn->nn_listlock, RW_READER);
	TAILQ_FOREACH(nc, &nn->nn_list, nc_list) {
		KASSERT(nc->nc_nn == nn);
		KASSERT(nc->nc_dnn != NULL);
		nlen = nc->nc_nlen;
		/*
		 * The queue is partially sorted.  Once we hit dots, nothing
		 * else remains but dots and dotdots, so bail out.
		 */
		if (nc->nc_name[0] == '.') {
			if (nlen == 1 ||
			    (nlen == 2 && nc->nc_name[1] == '.')) {
			    	break;
			}
		}

		/* Record a hit on the entry.  This is an unlocked read. */
		if (nc->nc_lrulist != LRU_ACTIVE) {
			cache_activate(nc);
		}

		if (bufp) {
			bp = *bpp;
			bp -= nlen;
			if (bp <= bufp) {
				*dvpp = NULL;
				rw_exit(&nn->nn_listlock);
				SDT_PROBE(vfs, namecache, revlookup,
				    fail, vp, ERANGE, 0, 0, 0);
				return (ERANGE);
			}
			memcpy(bp, nc->nc_name, nlen);
			*bpp = bp;
		}

		dvp = nc->nc_dnn->nn_vp;
		mutex_enter(dvp->v_interlock);
		rw_exit(&nn->nn_listlock);
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
	rw_exit(&nn->nn_listlock);
	COUNT(ncs_revmiss);
 out:
	*dvpp = NULL;
	return (-1);
}

/*
 * Add an entry to the cache.
 */
void
cache_enter(struct vnode *dvp, struct vnode *vp,
	    const char *name, size_t namelen, uint32_t cnflags)
{
	struct nchnode *dnn = VNODE_TO_VIMPL(dvp)->vi_ncache;
	struct nchnode *nn;
	struct namecache *nc;
	struct namecache *onc;
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
	if (__predict_true(namelen <= NCHNAMLEN)) {
		nc = pool_cache_get(cache_pool, PR_WAITOK);
	} else {
		size_t sz = offsetof(struct namecache, nc_name[namelen]);
		nc = kmem_alloc(sz, KM_SLEEP);
	}

	/* Fill in cache info. */
	nc->nc_dnn = dnn;
	nc->nc_key = cache_key(name, namelen);
	nc->nc_nlen = namelen;
	memcpy(nc->nc_name, name, namelen);

	/*
	 * Insert to the directory.  Concurrent lookups in the same
	 * directory may race for a cache entry.  There can also be hash
	 * value collisions.  If there's a entry there already, free it.
	 */
	rw_enter(&dnn->nn_lock, RW_WRITER);
	onc = rb_tree_find_node(&dnn->nn_tree, &nc->nc_key);
	if (onc) {
		KASSERT(onc->nc_nlen == nc->nc_nlen);
		if (memcmp(onc->nc_name, nc->nc_name, nc->nc_nlen) != 0) {
			COUNT(ncs_collisions);
		}
		cache_remove(onc, true);
	}
	rb_tree_insert_node(&dnn->nn_tree, nc);

	/* Then insert to the vnode. */
	if (vp == NULL) {
		/*
		 * For negative hits, save the ISWHITEOUT flag so we can
		 * restore it later when the cache entry is used again.
		 */
		nc->nc_vp = NULL;
		nc->nc_nn = NULL;
		nc->nc_whiteout = ((cnflags & ISWHITEOUT) != 0);
	} else {
		nn = VNODE_TO_VIMPL(vp)->vi_ncache;
		/* Partially sort the per-vnode list: dots go to back. */
		rw_enter(&nn->nn_listlock, RW_WRITER);
		if ((namelen == 1 && name[0] == '.') ||
		    (namelen == 2 && name[0] == '.' && name[1] == '.')) {
			TAILQ_INSERT_TAIL(&nn->nn_list, nc, nc_list);
		} else {
			TAILQ_INSERT_HEAD(&nn->nn_list, nc, nc_list);
		}
		rw_exit(&nn->nn_listlock);
		nc->nc_vp = vp;
		nc->nc_nn = nn;
		nc->nc_whiteout = false;
	}

	/*
	 * Finally, insert to the tail of the ACTIVE LRU list (new) and with
	 * the LRU lock held take the to opportunity to incrementally
	 * balance the lists.
	 */
	mutex_enter(&cache_lru_lock);
	nc->nc_lrulist = LRU_ACTIVE;
	cache_lru.count[LRU_ACTIVE]++;
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], nc, nc_lru);
	cache_deactivate();
	mutex_exit(&cache_lru_lock);
	rw_exit(&dnn->nn_lock);
}

/*
 * First set of identity info in cache for a vnode.  If a file system calls
 * this, it means the FS supports in-cache lookup.  We only care about
 * directories so ignore other updates.
 */
void
cache_set_id(struct vnode *dvp, mode_t mode, uid_t uid, gid_t gid)
{
	struct nchnode *nn = VNODE_TO_VIMPL(dvp)->vi_ncache;

	if (dvp->v_type == VDIR) {
		rw_enter(&nn->nn_lock, RW_WRITER);
		KASSERT(nn->nn_mode == VNOVAL);
		KASSERT(nn->nn_uid == VNOVAL);
		KASSERT(nn->nn_gid == VNOVAL);
		nn->nn_mode = mode;
		nn->nn_uid = uid;
		nn->nn_gid = gid;
		rw_exit(&nn->nn_lock);
	}
}

/*
 * Update of identity info in cache for a vnode.  If we didn't get ID info
 * to begin with, then the file system doesn't support in-cache lookup,
 * so ignore the update.  We only care about directories.
 */
void
cache_update_id(struct vnode *dvp, mode_t mode, uid_t uid, gid_t gid)
{
	struct nchnode *nn = VNODE_TO_VIMPL(dvp)->vi_ncache;

	if (dvp->v_type == VDIR) {
		rw_enter(&nn->nn_lock, RW_WRITER);
		if (nn->nn_mode != VNOVAL) {
			nn->nn_mode = mode;
			nn->nn_uid = uid;
			nn->nn_gid = gid;
		}
		rw_exit(&nn->nn_lock);
	}
}

/*
 * Name cache initialization, from vfs_init() when the system is booting.
 */
void
nchinit(void)
{

	cache_pool = pool_cache_init(sizeof(struct namecache),
	    coherency_unit, 0, 0, "nchentry", NULL, IPL_NONE, NULL,
	    NULL, NULL);
	KASSERT(cache_pool != NULL);

	cache_node_pool = pool_cache_init(sizeof(struct nchnode),
	    coherency_unit, 0, 0, "nchnode", NULL, IPL_NONE, NULL,
	    NULL, NULL);
	KASSERT(cache_node_pool != NULL);

	mutex_init(&cache_lru_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&cache_lru.list[LRU_ACTIVE]);
	TAILQ_INIT(&cache_lru.list[LRU_INACTIVE]);

	KASSERT(cache_sysctllog == NULL);
	sysctl_createv(&cache_sysctllog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "namecache_stats",
		       SYSCTL_DESCR("namecache statistics"),
		       cache_stat_sysctl, 0, NULL, 0,
		       CTL_VFS, CTL_CREATE, CTL_EOL);
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
	struct nchnode *nn;

	nn = pool_cache_get(cache_node_pool, PR_WAITOK);
	rw_init(&nn->nn_lock);
	rw_init(&nn->nn_listlock);
	rb_tree_init(&nn->nn_tree, &cache_rbtree_ops);
	TAILQ_INIT(&nn->nn_list);
	nn->nn_mode = VNOVAL;
	nn->nn_uid = VNOVAL;
	nn->nn_gid = VNOVAL;
	nn->nn_vp = vp;

	VNODE_TO_VIMPL(vp)->vi_ncache = nn;
}

/*
 * A vnode is being freed: finish cache structures.
 */
void
cache_vnode_fini(struct vnode *vp)
{
	struct nchnode *nn = VNODE_TO_VIMPL(vp)->vi_ncache;

	KASSERT(nn != NULL);
	KASSERT(nn->nn_vp == vp);
	KASSERT(RB_TREE_MIN(&nn->nn_tree) == NULL);
	KASSERT(TAILQ_EMPTY(&nn->nn_list));

	rw_destroy(&nn->nn_lock);
	rw_destroy(&nn->nn_listlock);
	pool_cache_put(cache_node_pool, nn);
}

/*
 * Helper for cache_purge1(): purge cache entries for the given vnode from
 * all directories that the vnode is cached in.
 */
static void
cache_purge_parents(struct vnode *vp)
{
	struct nchnode *nn = VNODE_TO_VIMPL(vp)->vi_ncache;
	struct nchnode *dnn, *blocked;
	struct namecache *nc;
	struct vnode *dvp;

	SDT_PROBE(vfs, namecache, purge, parents, vp, 0, 0, 0, 0);

	blocked = NULL;

	rw_enter(&nn->nn_listlock, RW_WRITER);
	while ((nc = TAILQ_FIRST(&nn->nn_list)) != NULL) {
		/*
		 * Locking in the wrong direction.  Try for a hold on the
		 * directory node's lock, and if we get it then all good,
		 * nuke the entry and move on to the next.
		 */
		dnn = nc->nc_dnn;
		if (rw_tryenter(&dnn->nn_lock, RW_WRITER)) {
			cache_remove(nc, false);
			rw_exit(&dnn->nn_lock);
			blocked = NULL;
			continue;
		}

		/*
		 * We can't wait on the directory node's lock with our list
		 * lock held or the system could deadlock.
		 *
		 * Take a hold on the directory vnode to prevent it from
		 * being freed (taking the nchnode & lock with it).  Then
		 * wait for the lock to become available with no other locks
		 * held, and retry.
		 *
		 * If this happens twice in a row, give the other side a
		 * breather; we can do nothing until it lets go.
		 */
		dvp = dnn->nn_vp;
		vhold(dvp);
		rw_exit(&nn->nn_listlock);
		rw_enter(&dnn->nn_lock, RW_WRITER);
		/* Do nothing. */
		rw_exit(&dnn->nn_lock);
		holdrele(dvp);
		if (blocked == dnn) {
			kpause("ncpurge", false, 1, NULL);
		}
		rw_enter(&nn->nn_listlock, RW_WRITER);
		blocked = dnn;
	}
	rw_exit(&nn->nn_listlock);
}

/*
 * Helper for cache_purge1(): purge all cache entries hanging off the given
 * directory vnode.
 */
static void
cache_purge_children(struct vnode *dvp)
{
	struct nchnode *dnn = VNODE_TO_VIMPL(dvp)->vi_ncache;
	struct namecache *nc;

	SDT_PROBE(vfs, namecache, purge, children, dvp, 0, 0, 0, 0);

	rw_enter(&dnn->nn_lock, RW_WRITER);
	while ((nc = rb_tree_iterate(&dnn->nn_tree, NULL, RB_DIR_RIGHT))
	    != NULL) {
		cache_remove(nc, true);
	}
	rw_exit(&dnn->nn_lock);
}

/*
 * Helper for cache_purge1(): purge cache entry from the given vnode,
 * finding it by name.
 */
static void
cache_purge_name(struct vnode *dvp, const char *name, size_t namelen)
{
	struct nchnode *dnn = VNODE_TO_VIMPL(dvp)->vi_ncache;
	struct namecache *nc;
	int64_t key;

	SDT_PROBE(vfs, namecache, purge, name, name, namelen, 0, 0, 0);

	key = cache_key(name, namelen);
	rw_enter(&dnn->nn_lock, RW_WRITER);
	nc = cache_lookup_entry(dnn, name, namelen, key);
	if (nc) {
		cache_remove(nc, true);
	}
	rw_exit(&dnn->nn_lock);
}

/*
 * Cache flush, a particular vnode; called when a vnode is renamed to
 * hide entries that would now be invalid.
 */
void
cache_purge1(struct vnode *vp, const char *name, size_t namelen, int flags)
{

	if (flags & PURGE_PARENTS) {
		cache_purge_parents(vp);
	}
	if (flags & PURGE_CHILDREN) {
		cache_purge_children(vp);
	}
	if (name != NULL) {
		cache_purge_name(vp, name, namelen);
	}
}

/*
 * Cache flush, a whole filesystem; called when filesys is umounted to
 * remove entries that would now be invalid.
 *
 * In the BSD implementation this traversed the LRU list.  To avoid locking
 * difficulties, and avoid scanning a ton of namecache entries that we have
 * no interest in, scan the mount's vnode list instead.
 */
void
cache_purgevfs(struct mount *mp)
{
	struct vnode_iterator *marker;
	vnode_t *vp;

	vfs_vnode_iterator_init(mp, &marker);
	while ((vp = vfs_vnode_iterator_next(marker, NULL, NULL))) {
		cache_purge_children(vp);
		if ((vp->v_vflag & VV_ROOT) != 0) {
			cache_purge_parents(vp);
		}
		vrele(vp);
	}
	vfs_vnode_iterator_destroy(marker);
}

/*
 * Re-queue an entry onto the correct LRU list, after it has scored a hit.
 */
static void
cache_activate(struct namecache *nc)
{

	mutex_enter(&cache_lru_lock);
	/* Put on tail of ACTIVE list, since it just scored a hit. */
	TAILQ_REMOVE(&cache_lru.list[nc->nc_lrulist], nc, nc_lru);
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], nc, nc_lru);
	cache_lru.count[nc->nc_lrulist]--;
	cache_lru.count[LRU_ACTIVE]++;
	nc->nc_lrulist = LRU_ACTIVE;
	mutex_exit(&cache_lru_lock);
}

/*
 * Try to balance the LRU lists.  Pick some victim entries, and re-queue
 * them from the head of the ACTIVE list to the tail of the INACTIVE list. 
 */
static void
cache_deactivate(void)
{
	struct namecache *nc;
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
		nc = TAILQ_FIRST(&cache_lru.list[LRU_ACTIVE]);
		if (nc == NULL) {
			break;
		}
		KASSERT(nc->nc_lrulist == LRU_ACTIVE);
		nc->nc_lrulist = LRU_INACTIVE;
		TAILQ_REMOVE(&cache_lru.list[LRU_ACTIVE], nc, nc_lru);
		TAILQ_INSERT_TAIL(&cache_lru.list[LRU_INACTIVE], nc, nc_lru);
		cache_lru.count[LRU_ACTIVE]--;
		cache_lru.count[LRU_INACTIVE]++;
	}
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
	struct namecache *nc;
	struct nchnode *nn;
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
		nc = TAILQ_FIRST(&cache_lru.list[LRU_INACTIVE]);
		if (nc == NULL) {
			break;
		}
		nn = nc->nc_nn;
		KASSERT(nc->nc_lrulist == LRU_INACTIVE);
		KASSERT(nn != NULL);

		/*
		 * Locking in the wrong direction.  If we can't get the
		 * lock, the directory is actively busy, and it could also
		 * cause problems for the next guy in here, so send the
		 * entry to the back of the list.
		 */
		if (!rw_tryenter(&nn->nn_lock, RW_WRITER)) {
			TAILQ_REMOVE(&cache_lru.list[LRU_INACTIVE],
			    nc, nc_lru);
			TAILQ_INSERT_TAIL(&cache_lru.list[LRU_INACTIVE],
			    nc, nc_lru);
			continue;
		}

		/*
		 * Now have the victim entry locked.  Drop the LRU list
		 * lock, purge the entry, and start over.  The hold on the
		 * directory lock will prevent the vnode from vanishing
		 * until finished (the vnode owner will call cache_purge()
		 * on dvp before it disappears).
		 */
		mutex_exit(&cache_lru_lock);
		cache_remove(nc, true);
		rw_exit(&nn->nn_lock);
		mutex_enter(&cache_lru_lock);
	}
	mutex_exit(&cache_lru_lock);
}

/*
 * For file system code: count a lookup that required a full re-scan of
 * directory metadata.
 */
void
namecache_count_pass2(void)
{

	COUNT(ncs_pass2);
}

/*
 * For file system code: count a lookup that scored a hit in the directory
 * metadata near the location of the last lookup.
 */
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
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	if (oldp == NULL) {
		*oldlenp = sizeof(nchstats);
		return 0;
	}

	if (*oldlenp <= 0) {
		*oldlenp = 0;
		return 0;
	}

	sysctl_unlock();
	mutex_enter(&cache_lru_lock);
	memset(&nchstats, 0, sizeof(nchstats));
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct nchstats_percpu *np = ci->ci_data.cpu_nch;

		nchstats.ncs_goodhits += np->ncs_goodhits;
		nchstats.ncs_neghits += np->ncs_neghits;
		nchstats.ncs_badhits += np->ncs_badhits;
		nchstats.ncs_falsehits += np->ncs_falsehits;
		nchstats.ncs_miss += np->ncs_miss;
		nchstats.ncs_long += np->ncs_long;
		nchstats.ncs_pass2 += np->ncs_pass2;
		nchstats.ncs_2passes += np->ncs_2passes;
		nchstats.ncs_revhits += np->ncs_revhits;
		nchstats.ncs_revmiss += np->ncs_revmiss;
		nchstats.ncs_collisions += np->ncs_collisions;
		nchstats.ncs_denied += np->ncs_denied;
	}
	nchstats.ncs_active = cache_lru.count[LRU_ACTIVE];
	nchstats.ncs_inactive = cache_lru.count[LRU_INACTIVE];
	mutex_exit(&cache_lru_lock);
	sysctl_relock();

	*oldlenp = MIN(sizeof(nchstats), *oldlenp);
	return sysctl_copyout(l, &nchstats, oldp, *oldlenp);
}

/*
 * For the debugger, given the address of a vnode, print all associated
 * names in the cache.
 */
#ifdef DDB
void
namecache_print(struct vnode *vp, void (*pr)(const char *, ...))
{
	struct vnode *dvp = NULL;
	struct namecache *nc;
	struct nchnode *dnn;
	enum cache_lru_id id;

	for (id = 0; id < LRU_COUNT; id++) {
		TAILQ_FOREACH(nc, &cache_lru.list[id], nc_lru) {
			if (nc->nc_vp == vp) {
				(*pr)("name %.*s\n", nc->nc_nlen,
				    nc->nc_name);
				dnn = nc->nc_dnn;
			}
		}
	}
	if (dvp == NULL) {
		(*pr)("name not found\n");
		return;
	}
	for (id = 0; id < LRU_COUNT; id++) {
		TAILQ_FOREACH(nc, &cache_lru.list[id], nc_lru) {
			if (nc->nc_nn == dnn) {
				(*pr)("parent %.*s\n", nc->nc_nlen,
				    nc->nc_name);
			}
		}
	}
}
#endif
