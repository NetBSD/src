/*	$NetBSD: vfs_cache.c,v 1.154 2023/04/29 10:07:22 riastradh Exp $	*/

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
 *	Names found by directory scans are retained in a cache for future
 *	reference.  It is managed LRU, so frequently used names will hang
 *	around.  The cache is indexed by hash value obtained from the name.
 *
 *	The name cache is the brainchild of Robert Elz and was introduced in
 *	4.3BSD.  See "Using gprof to Tune the 4.2BSD Kernel", Marshall Kirk
 *	McKusick, May 21 1984.
 *
 * Data structures:
 *
 *	Most Unix namecaches very sensibly use a global hash table to index
 *	names.  The global hash table works well, but can cause concurrency
 *	headaches for the kernel hacker.  In the NetBSD 10.0 implementation
 *	we are not sensible, and use a per-directory data structure to index
 *	names, but the cache otherwise functions the same.
 *
 *	The index is a red-black tree.  There are no special concurrency
 *	requirements placed on it, because it's per-directory and protected
 *	by the namecache's per-directory locks.  It should therefore not be
 *	difficult to experiment with other types of index.
 *
 *	Each cached name is stored in a struct namecache, along with a
 *	pointer to the associated vnode (nc_vp).  Names longer than a
 *	maximum length of NCHNAMLEN are allocated with kmem_alloc(); they
 *	occur infrequently, and names shorter than this are stored directly
 *	in struct namecache.  If it is a "negative" entry, (i.e. for a name
 *	that is known NOT to exist) the vnode pointer will be NULL.
 *
 *	For a directory with 3 cached names for 3 distinct vnodes, the
 *	various vnodes and namecache structs would be connected like this
 *	(the root is at the bottom of the diagram):
 *
 *          ...
 *           ^
 *           |- vi_nc_tree
 *           |
 *      +----o----+               +---------+               +---------+
 *      |  VDIR   |               |  VCHR   |               |  VREG   |
 *      |  vnode  o-----+         |  vnode  o-----+         |  vnode  o------+
 *      +---------+     |         +---------+     |         +---------+      |
 *           ^          |              ^          |              ^           |
 *           |- nc_vp   |- vi_nc_list  |- nc_vp   |- vi_nc_list  |- nc_vp    |
 *           |          |              |          |              |           |
 *      +----o----+     |         +----o----+     |         +----o----+      |
 *  +---onamecache|<----+     +---onamecache|<----+     +---onamecache|<-----+
 *  |   +---------+           |   +---------+           |   +---------+
 *  |        ^                |        ^                |        ^
 *  |        |                |        |                |        |
 *  |        |  +----------------------+                |        |
 *  |-nc_dvp | +-------------------------------------------------+
 *  |        |/- vi_nc_tree   |                         |
 *  |        |                |- nc_dvp                 |- nc_dvp
 *  |   +----o----+           |                         |
 *  +-->|  VDIR   |<----------+                         |
 *      |  vnode  |<------------------------------------+
 *      +---------+
 *
 *      START HERE
 *
 * Replacement:
 *
 *	As the cache becomes full, old and unused entries are purged as new
 *	entries are added.  The synchronization overhead in maintaining a
 *	strict ordering would be prohibitive, so the VM system's "clock" or
 *	"second chance" page replacement algorithm is aped here.  New
 *	entries go to the tail of the active list.  After they age out and
 *	reach the head of the list, they are moved to the tail of the
 *	inactive list.  Any use of the deactivated cache entry reactivates
 *	it, saving it from impending doom; if not reactivated, the entry
 *	eventually reaches the head of the inactive list and is purged.
 *
 * Concurrency:
 *
 *	From a performance perspective, cache_lookup(nameiop == LOOKUP) is
 *	what really matters; insertion of new entries with cache_enter() is
 *	comparatively infrequent, and overshadowed by the cost of expensive
 *	file system metadata operations (which may involve disk I/O).  We
 *	therefore want to make everything simplest in the lookup path.
 *
 *	struct namecache is mostly stable except for list and tree related
 *	entries, changes to which don't affect the cached name or vnode. 
 *	For changes to name+vnode, entries are purged in preference to
 *	modifying them.
 *
 *	Read access to namecache entries is made via tree, list, or LRU
 *	list.  A lock corresponding to the direction of access should be
 *	held.  See definition of "struct namecache" in src/sys/namei.src,
 *	and the definition of "struct vnode" for the particulars.
 *
 *	Per-CPU statistics, and LRU list totals are read unlocked, since
 *	an approximate value is OK.  We maintain 32-bit sized per-CPU
 *	counters and 64-bit global counters under the theory that 32-bit
 *	sized counters are less likely to be hosed by nonatomic increment
 *	(on 32-bit platforms).
 *
 *	The lock order is:
 *
 *	1) vi->vi_nc_lock	(tree or parent -> child direction,
 *				 used during forward lookup)
 *
 *	2) vi->vi_nc_listlock	(list or child -> parent direction,
 *				 used during reverse lookup)
 *
 *	3) cache_lru_lock	(LRU list direction, used during reclaim)
 *
 *	4) vp->v_interlock	(what the cache entry points to)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cache.c,v 1.154 2023/04/29 10:07:22 riastradh Exp $");

#define __NAMECACHE_PRIVATE
#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_dtrace.h"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/callout.h>
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

static void	cache_activate(struct namecache *);
static void	cache_update_stats(void *);
static int	cache_compare_nodes(void *, const void *, const void *);
static void	cache_deactivate(void);
static void	cache_reclaim(void);
static int	cache_stat_sysctl(SYSCTLFN_ARGS);

/*
 * Global pool cache.
 */
static pool_cache_t cache_pool __read_mostly;

/*
 * LRU replacement.
 */
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

/*
 * Cache effectiveness statistics.  nchstats holds system-wide total.
 */
struct nchstats	nchstats;
struct nchstats_percpu _NAMEI_CACHE_STATS(uint32_t);
struct nchcpu {
	struct nchstats_percpu cur;
	struct nchstats_percpu last;
};
static callout_t cache_stat_callout;
static kmutex_t cache_stat_lock __cacheline_aligned;

#define	COUNT(f) do { \
	lwp_t *l = curlwp; \
	KPREEMPT_DISABLE(l); \
	struct nchcpu *nchcpu = curcpu()->ci_data.cpu_nch; \
	nchcpu->cur.f++; \
	KPREEMPT_ENABLE(l); \
} while (/* CONSTCOND */ 0);

#define	UPDATE(nchcpu, f) do { \
	uint32_t cur = atomic_load_relaxed(&nchcpu->cur.f); \
	nchstats.f += (uint32_t)(cur - nchcpu->last.f); \
	nchcpu->last.f = cur; \
} while (/* CONSTCOND */ 0)

/*
 * Tunables.  cache_maxlen replaces the historical doingcache:
 * set it zero to disable caching for debugging purposes.
 */
int cache_lru_maxdeact __read_mostly = 2;	/* max # to deactivate */
int cache_lru_maxscan __read_mostly = 64;	/* max # to scan/reclaim */
int cache_maxlen __read_mostly = USHRT_MAX;	/* max name length to cache */
int cache_stat_interval __read_mostly = 300;	/* in seconds */

/*
 * sysctl stuff.
 */
static struct	sysctllog *cache_sysctllog;

/*
 * This is a dummy name that cannot usually occur anywhere in the cache nor
 * file system.  It's used when caching the root vnode of mounted file
 * systems.  The name is attached to the directory that the file system is
 * mounted on.
 */
static const char cache_mp_name[] = "";
static const int cache_mp_nlen = sizeof(cache_mp_name) - 1;

/*
 * Red-black tree stuff.
 */
static const rb_tree_ops_t cache_rbtree_ops = {
	.rbto_compare_nodes = cache_compare_nodes,
	.rbto_compare_key = cache_compare_nodes,
	.rbto_node_offset = offsetof(struct namecache, nc_tree),
	.rbto_context = NULL
};

/*
 * dtrace probes.
 */
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
	KASSERT(nc1->nc_nlen == nc2->nc_nlen);
	return memcmp(nc1->nc_name, nc2->nc_name, nc1->nc_nlen);
}

/*
 * Compute a key value for the given name.  The name length is encoded in
 * the key value to try and improve uniqueness, and so that length doesn't
 * need to be compared separately for string comparisons.
 */
static inline uint64_t
cache_key(const char *name, size_t nlen)
{
	uint64_t key;

	KASSERT(nlen <= USHRT_MAX);

	key = hash32_buf(name, nlen, HASH32_STR_INIT);
	return (key << 32) | nlen;
}

/*
 * Remove an entry from the cache.  vi_nc_lock must be held, and if dir2node
 * is true, then we're locking in the conventional direction and the list
 * lock will be acquired when removing the entry from the vnode list.
 */
static void
cache_remove(struct namecache *ncp, const bool dir2node)
{
	struct vnode *vp, *dvp = ncp->nc_dvp;
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);

	KASSERT(rw_write_held(&dvi->vi_nc_lock));
	KASSERT(cache_key(ncp->nc_name, ncp->nc_nlen) == ncp->nc_key);
	KASSERT(rb_tree_find_node(&dvi->vi_nc_tree, ncp) == ncp);

	SDT_PROBE(vfs, namecache, invalidate, done, ncp,
	    0, 0, 0, 0);

	/*
	 * Remove from the vnode's list.  This excludes cache_revlookup(),
	 * and then it's safe to remove from the LRU lists.
	 */
	if ((vp = ncp->nc_vp) != NULL) {
		vnode_impl_t *vi = VNODE_TO_VIMPL(vp);
		if (__predict_true(dir2node)) {
			rw_enter(&vi->vi_nc_listlock, RW_WRITER);
			TAILQ_REMOVE(&vi->vi_nc_list, ncp, nc_list);
			rw_exit(&vi->vi_nc_listlock);
		} else {
			TAILQ_REMOVE(&vi->vi_nc_list, ncp, nc_list);
		}
	}

	/* Remove from the directory's rbtree. */
	rb_tree_remove_node(&dvi->vi_nc_tree, ncp);

	/* Remove from the LRU lists. */
	mutex_enter(&cache_lru_lock);
	TAILQ_REMOVE(&cache_lru.list[ncp->nc_lrulist], ncp, nc_lru);
	cache_lru.count[ncp->nc_lrulist]--;
	mutex_exit(&cache_lru_lock);

	/* Finally, free it. */
	if (ncp->nc_nlen > NCHNAMLEN) {
		size_t sz = offsetof(struct namecache, nc_name[ncp->nc_nlen]);
		kmem_free(ncp, sz);
	} else {
		pool_cache_put(cache_pool, ncp);
	}
}

/*
 * Find a single cache entry and return it.  vi_nc_lock must be held.
 */
static struct namecache * __noinline
cache_lookup_entry(struct vnode *dvp, const char *name, size_t namelen,
    uint64_t key)
{
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct rb_node *node = dvi->vi_nc_tree.rbt_root;
	struct namecache *ncp;
	int lrulist, diff;

	KASSERT(rw_lock_held(&dvi->vi_nc_lock));

	/*
	 * Search the RB tree for the key.  This is an inlined lookup
	 * tailored for exactly what's needed here (64-bit key and so on)
	 * that is quite a bit faster than using rb_tree_find_node().
	 *
	 * For a matching key memcmp() needs to be called once to confirm
	 * that the correct name has been found.  Very rarely there will be
	 * a key value collision and the search will continue.
	 */
	for (;;) {
		if (__predict_false(RB_SENTINEL_P(node))) {
			return NULL;
		}
		ncp = (struct namecache *)node;
		KASSERT((void *)&ncp->nc_tree == (void *)ncp);
		KASSERT(ncp->nc_dvp == dvp);
		if (ncp->nc_key == key) {
			KASSERT(ncp->nc_nlen == namelen);
			diff = memcmp(ncp->nc_name, name, namelen);
			if (__predict_true(diff == 0)) {
				break;
			}
			node = node->rb_nodes[diff < 0];
		} else {
			node = node->rb_nodes[ncp->nc_key < key];
		}
	}

	/*
	 * If the entry is on the wrong LRU list, requeue it.  This is an
	 * unlocked check, but it will rarely be wrong and even then there
	 * will be no harm caused.
	 */
	lrulist = atomic_load_relaxed(&ncp->nc_lrulist);
	if (__predict_false(lrulist != LRU_ACTIVE)) {
		cache_activate(ncp);
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
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct namecache *ncp;
	struct vnode *vp;
	uint64_t key;
	int error;
	bool hit;
	krw_t op;

	KASSERT(namelen != cache_mp_nlen || name == cache_mp_name);

	/* Establish default result values */
	if (iswht_ret != NULL) {
		*iswht_ret = 0;
	}
	*vn_ret = NULL;

	if (__predict_false(namelen > cache_maxlen)) {
		SDT_PROBE(vfs, namecache, lookup, toolong, dvp,
		    name, namelen, 0, 0);
		COUNT(ncs_long);
		return false;
	}

	/* Compute the key up front - don't need the lock. */
	key = cache_key(name, namelen);

	/* Could the entry be purged below? */
	if ((cnflags & ISLASTCN) != 0 &&
	    ((cnflags & MAKEENTRY) == 0 || nameiop == CREATE)) {
	    	op = RW_WRITER;
	} else {
		op = RW_READER;
	}

	/* Now look for the name. */
	rw_enter(&dvi->vi_nc_lock, op);
	ncp = cache_lookup_entry(dvp, name, namelen, key);
	if (__predict_false(ncp == NULL)) {
		rw_exit(&dvi->vi_nc_lock);
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
		cache_remove(ncp, true);
		rw_exit(&dvi->vi_nc_lock);
		COUNT(ncs_badhits);
		return false;
	}
	if (ncp->nc_vp == NULL) {
		if (iswht_ret != NULL) {
			/*
			 * Restore the ISWHITEOUT flag saved earlier.
			 */
			*iswht_ret = ncp->nc_whiteout;
		} else {
			KASSERT(!ncp->nc_whiteout);
		}
		if (nameiop == CREATE && (cnflags & ISLASTCN) != 0) {
			/*
			 * Last component and we are preparing to create
			 * the named object, so flush the negative cache
			 * entry.
			 */
			COUNT(ncs_badhits);
			cache_remove(ncp, true);
			hit = false;
		} else {
			COUNT(ncs_neghits);
			SDT_PROBE(vfs, namecache, lookup, hit, dvp, name,
			    namelen, 0, 0);
			/* found neg entry; vn is already null from above */
			hit = true;
		}
		rw_exit(&dvi->vi_nc_lock);
		return hit;
	}
	vp = ncp->nc_vp;
	error = vcache_tryvget(vp);
	rw_exit(&dvi->vi_nc_lock);
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
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct namecache *ncp;
	krwlock_t *oldlock, *newlock;
	uint64_t key;
	int error;

	KASSERT(namelen != cache_mp_nlen || name == cache_mp_name);

	/* If disabled, or file system doesn't support this, bail out. */
	if (__predict_false((dvp->v_mount->mnt_iflag & IMNT_NCLOOKUP) == 0)) {
		return false;
	}

	if (__predict_false(namelen > cache_maxlen)) {
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
	 * here: the directory must be purged with cache_purge() before
	 * being freed, and both parent & child's vi_nc_lock must be taken
	 * before that point is passed.
	 *
	 * However if there's no previous lock, like at the root of the
	 * chain, then "dvp" must be referenced to prevent dvp going away
	 * before we get its lock.
	 *
	 * Note that the two locks can be the same if looking up a dot, for
	 * example: /usr/bin/.  If looking up the parent (..) we can't wait
	 * on the lock as child -> parent is the wrong direction.
	 */
	if (*plock != &dvi->vi_nc_lock) {
		oldlock = *plock;
		newlock = &dvi->vi_nc_lock;
		if (!rw_tryenter(&dvi->vi_nc_lock, RW_READER)) {
			return false;
		}
	} else {
		oldlock = NULL;
		newlock = NULL;
		if (*plock == NULL) {
			KASSERT(vrefcnt(dvp) > 0);
		}
	}

	/*
	 * First up check if the user is allowed to look up files in this
	 * directory.
	 */
	if (cred != FSCRED) {
		if (dvi->vi_nc_mode == VNOVAL) {
			if (newlock != NULL) {
				rw_exit(newlock);
			}
			return false;
		}
		KASSERT(dvi->vi_nc_uid != VNOVAL);
		KASSERT(dvi->vi_nc_gid != VNOVAL);
		error = kauth_authorize_vnode(cred,
		    KAUTH_ACCESS_ACTION(VEXEC,
		    dvp->v_type, dvi->vi_nc_mode & ALLPERMS), dvp, NULL,
		    genfs_can_access(dvp, cred, dvi->vi_nc_uid, dvi->vi_nc_gid,
		    dvi->vi_nc_mode & ALLPERMS, NULL, VEXEC));
		if (error != 0) {
			if (newlock != NULL) {
				rw_exit(newlock);
			}
			COUNT(ncs_denied);
			return false;
		}
	}

	/*
	 * Now look for a matching cache entry.
	 */
	ncp = cache_lookup_entry(dvp, name, namelen, key);
	if (__predict_false(ncp == NULL)) {
		if (newlock != NULL) {
			rw_exit(newlock);
		}
		COUNT(ncs_miss);
		SDT_PROBE(vfs, namecache, lookup, miss, dvp,
		    name, namelen, 0, 0);
		return false;
	}
	if (ncp->nc_vp == NULL) {
		/* found negative entry; vn is already null from above */
		KASSERT(namelen != cache_mp_nlen);
		KASSERT(name != cache_mp_name);
		COUNT(ncs_neghits);
	} else {
		COUNT(ncs_goodhits); /* XXX can be "badhits" */
	}
	SDT_PROBE(vfs, namecache, lookup, hit, dvp, name, namelen, 0, 0);

	/*
	 * Return with the directory lock still held.  It will either be
	 * returned to us with another call to cache_lookup_linked() when
	 * looking up the next component, or the caller will release it
	 * manually when finished.
	 */
	if (oldlock) {
		rw_exit(oldlock);
	}
	if (newlock) {
		*plock = newlock;
	}
	*vn_ret = ncp->nc_vp;
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
cache_revlookup(struct vnode *vp, struct vnode **dvpp, char **bpp, char *bufp,
    bool checkaccess, accmode_t accmode)
{
	vnode_impl_t *vi = VNODE_TO_VIMPL(vp);
	struct namecache *ncp;
	struct vnode *dvp;
	int error, nlen, lrulist;
	char *bp;

	KASSERT(vp != NULL);

	if (cache_maxlen == 0)
		goto out;

	rw_enter(&vi->vi_nc_listlock, RW_READER);
	if (checkaccess) {
		/*
		 * Check if the user is allowed to see.  NOTE: this is
		 * checking for access on the "wrong" directory.  getcwd()
		 * wants to see that there is access on every component
		 * along the way, not that there is access to any individual
		 * component.  Don't use this to check you can look in vp.
		 *
		 * I don't like it, I didn't come up with it, don't blame me!
		 */
		if (vi->vi_nc_mode == VNOVAL) {
			rw_exit(&vi->vi_nc_listlock);
			return -1;
		}
		KASSERT(vi->vi_nc_uid != VNOVAL);
		KASSERT(vi->vi_nc_gid != VNOVAL);
		error = kauth_authorize_vnode(kauth_cred_get(),
		    KAUTH_ACCESS_ACTION(VEXEC, vp->v_type, vi->vi_nc_mode &
		    ALLPERMS), vp, NULL, genfs_can_access(vp, curlwp->l_cred,
		    vi->vi_nc_uid, vi->vi_nc_gid, vi->vi_nc_mode & ALLPERMS,
		    NULL, accmode));
		    if (error != 0) {
		    	rw_exit(&vi->vi_nc_listlock);
			COUNT(ncs_denied);
			return EACCES;
		}
	}
	TAILQ_FOREACH(ncp, &vi->vi_nc_list, nc_list) {
		KASSERT(ncp->nc_vp == vp);
		KASSERT(ncp->nc_dvp != NULL);
		nlen = ncp->nc_nlen;

		/*
		 * Ignore mountpoint entries.
		 */
		if (ncp->nc_nlen == cache_mp_nlen) {
			continue;
		}

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

		/*
		 * Record a hit on the entry.  This is an unlocked read but
		 * even if wrong it doesn't matter too much.
		 */
		lrulist = atomic_load_relaxed(&ncp->nc_lrulist);
		if (lrulist != LRU_ACTIVE) {
			cache_activate(ncp);
		}

		if (bufp) {
			bp = *bpp;
			bp -= nlen;
			if (bp <= bufp) {
				*dvpp = NULL;
				rw_exit(&vi->vi_nc_listlock);
				SDT_PROBE(vfs, namecache, revlookup,
				    fail, vp, ERANGE, 0, 0, 0);
				return (ERANGE);
			}
			memcpy(bp, ncp->nc_name, nlen);
			*bpp = bp;
		}

		dvp = ncp->nc_dvp;
		error = vcache_tryvget(dvp);
		rw_exit(&vi->vi_nc_listlock);
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
	rw_exit(&vi->vi_nc_listlock);
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
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct namecache *ncp, *oncp;
	int total;

	KASSERT(namelen != cache_mp_nlen || name == cache_mp_name);

	/* First, check whether we can/should add a cache entry. */
	if ((cnflags & MAKEENTRY) == 0 ||
	    __predict_false(namelen > cache_maxlen)) {
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
	total = atomic_load_relaxed(&cache_lru.count[LRU_ACTIVE]);
	total += atomic_load_relaxed(&cache_lru.count[LRU_INACTIVE]);
	if (__predict_false(total > desiredvnodes)) {
		cache_reclaim();
	}

	/* Now allocate a fresh entry. */
	if (__predict_true(namelen <= NCHNAMLEN)) {
		ncp = pool_cache_get(cache_pool, PR_WAITOK);
	} else {
		size_t sz = offsetof(struct namecache, nc_name[namelen]);
		ncp = kmem_alloc(sz, KM_SLEEP);
	}

	/*
	 * Fill in cache info.  For negative hits, save the ISWHITEOUT flag
	 * so we can restore it later when the cache entry is used again.
	 */
	ncp->nc_vp = vp;
	ncp->nc_dvp = dvp;
	ncp->nc_key = cache_key(name, namelen);
	ncp->nc_nlen = namelen;
	ncp->nc_whiteout = ((cnflags & ISWHITEOUT) != 0);
	memcpy(ncp->nc_name, name, namelen);

	/*
	 * Insert to the directory.  Concurrent lookups may race for a cache
	 * entry.  If there's a entry there already, purge it.
	 */
	rw_enter(&dvi->vi_nc_lock, RW_WRITER);
	oncp = rb_tree_insert_node(&dvi->vi_nc_tree, ncp);
	if (oncp != ncp) {
		KASSERT(oncp->nc_key == ncp->nc_key);
		KASSERT(oncp->nc_nlen == ncp->nc_nlen);
		KASSERT(memcmp(oncp->nc_name, name, namelen) == 0);
		cache_remove(oncp, true);
		oncp = rb_tree_insert_node(&dvi->vi_nc_tree, ncp);
		KASSERT(oncp == ncp);
	}

	/*
	 * With the directory lock still held, insert to the tail of the
	 * ACTIVE LRU list (new) and take the opportunity to incrementally
	 * balance the lists.
	 */
	mutex_enter(&cache_lru_lock);
	ncp->nc_lrulist = LRU_ACTIVE;
	cache_lru.count[LRU_ACTIVE]++;
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], ncp, nc_lru);
	cache_deactivate();
	mutex_exit(&cache_lru_lock);

	/*
	 * Finally, insert to the vnode and unlock.  With everything set up
	 * it's safe to let cache_revlookup() see the entry.  Partially sort
	 * the per-vnode list: dots go to back so cache_revlookup() doesn't
	 * have to consider them.
	 */
	if (vp != NULL) {
		vnode_impl_t *vi = VNODE_TO_VIMPL(vp);
		rw_enter(&vi->vi_nc_listlock, RW_WRITER);
		if ((namelen == 1 && name[0] == '.') ||
		    (namelen == 2 && name[0] == '.' && name[1] == '.')) {
			TAILQ_INSERT_TAIL(&vi->vi_nc_list, ncp, nc_list);
		} else {
			TAILQ_INSERT_HEAD(&vi->vi_nc_list, ncp, nc_list);
		}
		rw_exit(&vi->vi_nc_listlock);
	}
	rw_exit(&dvi->vi_nc_lock);
}

/*
 * Set identity info in cache for a vnode.  We only care about directories
 * so ignore other updates.  The cached info may be marked invalid if the
 * inode has an ACL.
 */
void
cache_enter_id(struct vnode *vp, mode_t mode, uid_t uid, gid_t gid, bool valid)
{
	vnode_impl_t *vi = VNODE_TO_VIMPL(vp);

	if (vp->v_type == VDIR) {
		/* Grab both locks, for forward & reverse lookup. */
		rw_enter(&vi->vi_nc_lock, RW_WRITER);
		rw_enter(&vi->vi_nc_listlock, RW_WRITER);
		if (valid) {
			vi->vi_nc_mode = mode;
			vi->vi_nc_uid = uid;
			vi->vi_nc_gid = gid;
		} else {
			vi->vi_nc_mode = VNOVAL;
			vi->vi_nc_uid = VNOVAL;
			vi->vi_nc_gid = VNOVAL;
		}
		rw_exit(&vi->vi_nc_listlock);
		rw_exit(&vi->vi_nc_lock);
	}
}

/*
 * Return true if we have identity for the given vnode, and use as an
 * opportunity to confirm that everything squares up.
 *
 * Because of shared code, some file systems could provide partial
 * information, missing some updates, so check the mount flag too.
 */
bool
cache_have_id(struct vnode *vp)
{

	if (vp->v_type == VDIR &&
	    (vp->v_mount->mnt_iflag & IMNT_NCLOOKUP) != 0 &&
	    atomic_load_relaxed(&VNODE_TO_VIMPL(vp)->vi_nc_mode) != VNOVAL) {
		return true;
	} else {
		return false;
	}
}

/*
 * Enter a mount point.  cvp is the covered vnode, and rvp is the root of
 * the mounted file system.
 */
void
cache_enter_mount(struct vnode *cvp, struct vnode *rvp)
{

	KASSERT(vrefcnt(cvp) > 0);
	KASSERT(vrefcnt(rvp) > 0);
	KASSERT(cvp->v_type == VDIR);
	KASSERT((rvp->v_vflag & VV_ROOT) != 0);

	if (rvp->v_type == VDIR) {
		cache_enter(cvp, rvp, cache_mp_name, cache_mp_nlen, MAKEENTRY);
	}
}

/*
 * Look up a cached mount point.  Used in the strongly locked path.
 */
bool
cache_lookup_mount(struct vnode *dvp, struct vnode **vn_ret)
{
	bool ret;

	ret = cache_lookup(dvp, cache_mp_name, cache_mp_nlen, LOOKUP,
	    MAKEENTRY, NULL, vn_ret);
	KASSERT((*vn_ret != NULL) == ret);
	return ret;
}

/*
 * Try to cross a mount point.  For use with cache_lookup_linked().
 */
bool
cache_cross_mount(struct vnode **dvp, krwlock_t **plock)
{

	return cache_lookup_linked(*dvp, cache_mp_name, cache_mp_nlen,
	   dvp, plock, FSCRED);
}

/*
 * Name cache initialization, from vfs_init() when the system is booting.
 */
void
nchinit(void)
{

	cache_pool = pool_cache_init(sizeof(struct namecache),
	    coherency_unit, 0, 0, "namecache", NULL, IPL_NONE, NULL,
	    NULL, NULL);
	KASSERT(cache_pool != NULL);

	mutex_init(&cache_lru_lock, MUTEX_DEFAULT, IPL_NONE);
	TAILQ_INIT(&cache_lru.list[LRU_ACTIVE]);
	TAILQ_INIT(&cache_lru.list[LRU_INACTIVE]);

	mutex_init(&cache_stat_lock, MUTEX_DEFAULT, IPL_NONE);
	callout_init(&cache_stat_callout, CALLOUT_MPSAFE);
	callout_setfunc(&cache_stat_callout, cache_update_stats, NULL);
	callout_schedule(&cache_stat_callout, cache_stat_interval * hz);

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

	sz = roundup2(sizeof(struct nchcpu), coherency_unit) + coherency_unit;
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

	rw_init(&vi->vi_nc_lock);
	rw_init(&vi->vi_nc_listlock);
	rb_tree_init(&vi->vi_nc_tree, &cache_rbtree_ops);
	TAILQ_INIT(&vi->vi_nc_list);
	vi->vi_nc_mode = VNOVAL;
	vi->vi_nc_uid = VNOVAL;
	vi->vi_nc_gid = VNOVAL;
}

/*
 * A vnode is being freed: finish cache structures.
 */
void
cache_vnode_fini(struct vnode *vp)
{
	vnode_impl_t *vi = VNODE_TO_VIMPL(vp);

	KASSERT(RB_TREE_MIN(&vi->vi_nc_tree) == NULL);
	KASSERT(TAILQ_EMPTY(&vi->vi_nc_list));
	rw_destroy(&vi->vi_nc_lock);
	rw_destroy(&vi->vi_nc_listlock);
}

/*
 * Helper for cache_purge1(): purge cache entries for the given vnode from
 * all directories that the vnode is cached in.
 */
static void
cache_purge_parents(struct vnode *vp)
{
	vnode_impl_t *dvi, *vi = VNODE_TO_VIMPL(vp);
	struct vnode *dvp, *blocked;
	struct namecache *ncp;

	SDT_PROBE(vfs, namecache, purge, parents, vp, 0, 0, 0, 0);

	blocked = NULL;

	rw_enter(&vi->vi_nc_listlock, RW_WRITER);
	while ((ncp = TAILQ_FIRST(&vi->vi_nc_list)) != NULL) {
		/*
		 * Locking in the wrong direction.  Try for a hold on the
		 * directory node's lock, and if we get it then all good,
		 * nuke the entry and move on to the next.
		 */
		dvp = ncp->nc_dvp;
		dvi = VNODE_TO_VIMPL(dvp);
		if (rw_tryenter(&dvi->vi_nc_lock, RW_WRITER)) {
			cache_remove(ncp, false);
			rw_exit(&dvi->vi_nc_lock);
			blocked = NULL;
			continue;
		}

		/*
		 * We can't wait on the directory node's lock with our list
		 * lock held or the system could deadlock.
		 *
		 * Take a hold on the directory vnode to prevent it from
		 * being freed (taking the vnode & lock with it).  Then
		 * wait for the lock to become available with no other locks
		 * held, and retry.
		 *
		 * If this happens twice in a row, give the other side a
		 * breather; we can do nothing until it lets go.
		 */
		vhold(dvp);
		rw_exit(&vi->vi_nc_listlock);
		rw_enter(&dvi->vi_nc_lock, RW_WRITER);
		/* Do nothing. */
		rw_exit(&dvi->vi_nc_lock);
		holdrele(dvp);
		if (blocked == dvp) {
			kpause("ncpurge", false, 1, NULL);
		}
		rw_enter(&vi->vi_nc_listlock, RW_WRITER);
		blocked = dvp;
	}
	rw_exit(&vi->vi_nc_listlock);
}

/*
 * Helper for cache_purge1(): purge all cache entries hanging off the given
 * directory vnode.
 */
static void
cache_purge_children(struct vnode *dvp)
{
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct namecache *ncp;

	SDT_PROBE(vfs, namecache, purge, children, dvp, 0, 0, 0, 0);

	rw_enter(&dvi->vi_nc_lock, RW_WRITER);
	while ((ncp = RB_TREE_MIN(&dvi->vi_nc_tree)) != NULL) {
		cache_remove(ncp, true);
	}
	rw_exit(&dvi->vi_nc_lock);
}

/*
 * Helper for cache_purge1(): purge cache entry from the given vnode,
 * finding it by name.
 */
static void
cache_purge_name(struct vnode *dvp, const char *name, size_t namelen)
{
	vnode_impl_t *dvi = VNODE_TO_VIMPL(dvp);
	struct namecache *ncp;
	uint64_t key;

	SDT_PROBE(vfs, namecache, purge, name, name, namelen, 0, 0, 0);

	key = cache_key(name, namelen);
	rw_enter(&dvi->vi_nc_lock, RW_WRITER);
	ncp = cache_lookup_entry(dvp, name, namelen, key);
	if (ncp) {
		cache_remove(ncp, true);
	}
	rw_exit(&dvi->vi_nc_lock);
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
 * vnode filter for cache_purgevfs().
 */
static bool
cache_vdir_filter(void *cookie, vnode_t *vp)
{

	return vp->v_type == VDIR;
}

/*
 * Cache flush, a whole filesystem; called when filesys is umounted to
 * remove entries that would now be invalid.
 */
void
cache_purgevfs(struct mount *mp)
{
	struct vnode_iterator *iter;
	vnode_t *dvp;

	vfs_vnode_iterator_init(mp, &iter);
	for (;;) {
		dvp = vfs_vnode_iterator_next(iter, cache_vdir_filter, NULL);
		if (dvp == NULL) {
			break;
		}
		cache_purge_children(dvp);
		vrele(dvp);
	}
	vfs_vnode_iterator_destroy(iter);
}

/*
 * Re-queue an entry onto the tail of the active LRU list, after it has
 * scored a hit.
 */
static void
cache_activate(struct namecache *ncp)
{

	mutex_enter(&cache_lru_lock);
	TAILQ_REMOVE(&cache_lru.list[ncp->nc_lrulist], ncp, nc_lru);
	TAILQ_INSERT_TAIL(&cache_lru.list[LRU_ACTIVE], ncp, nc_lru);
	cache_lru.count[ncp->nc_lrulist]--;
	cache_lru.count[LRU_ACTIVE]++;
	ncp->nc_lrulist = LRU_ACTIVE;
	mutex_exit(&cache_lru_lock);
}

/*
 * Try to balance the LRU lists.  Pick some victim entries, and re-queue
 * them from the head of the active list to the tail of the inactive list.
 */
static void
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

	/*
	 * Aim for a 1:1 ratio of active to inactive.  This is to allow each
	 * potential victim a reasonable amount of time to cycle through the
	 * inactive list in order to score a hit and be reactivated, while
	 * trying not to cause reactivations too frequently.
	 */
	if (cache_lru.count[LRU_ACTIVE] < cache_lru.count[LRU_INACTIVE]) {
		return;
	}

	/* Move only a few at a time; will catch up eventually. */
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
	vnode_impl_t *dvi;
	int toscan;

	/*
	 * Scan up to a preset maximum number of entries, but no more than
	 * 0.8% of the total at once (to allow for very small systems).
	 *
	 * On bigger systems, do a larger chunk of work to reduce the number
	 * of times that cache_lru_lock is held for any length of time.
	 */
	mutex_enter(&cache_lru_lock);
	toscan = MIN(cache_lru_maxscan, desiredvnodes >> 7);
	toscan = MAX(toscan, 1);
	SDT_PROBE(vfs, namecache, prune, done, cache_lru.count[LRU_ACTIVE] +
	    cache_lru.count[LRU_INACTIVE], toscan, 0, 0, 0);
	while (toscan-- != 0) {
		/* First try to balance the lists. */
		cache_deactivate();

		/* Now look for a victim on head of inactive list (old). */
		ncp = TAILQ_FIRST(&cache_lru.list[LRU_INACTIVE]);
		if (ncp == NULL) {
			break;
		}
		dvi = VNODE_TO_VIMPL(ncp->nc_dvp);
		KASSERT(ncp->nc_lrulist == LRU_INACTIVE);
		KASSERT(dvi != NULL);

		/*
		 * Locking in the wrong direction.  If we can't get the
		 * lock, the directory is actively busy, and it could also
		 * cause problems for the next guy in here, so send the
		 * entry to the back of the list.
		 */
		if (!rw_tryenter(&dvi->vi_nc_lock, RW_WRITER)) {
			TAILQ_REMOVE(&cache_lru.list[LRU_INACTIVE],
			    ncp, nc_lru);
			TAILQ_INSERT_TAIL(&cache_lru.list[LRU_INACTIVE],
			    ncp, nc_lru);
			continue;
		}

		/*
		 * Now have the victim entry locked.  Drop the LRU list
		 * lock, purge the entry, and start over.  The hold on
		 * vi_nc_lock will prevent the vnode from vanishing until
		 * finished (cache_purge() will be called on dvp before it
		 * disappears, and that will wait on vi_nc_lock).
		 */
		mutex_exit(&cache_lru_lock);
		cache_remove(ncp, true);
		rw_exit(&dvi->vi_nc_lock);
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
 * Sum the stats from all CPUs into nchstats.  This needs to run at least
 * once within every window where a 32-bit counter could roll over.  It's
 * called regularly by timer to ensure this.
 */
static void
cache_update_stats(void *cookie)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	mutex_enter(&cache_stat_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct nchcpu *nchcpu = ci->ci_data.cpu_nch;
		UPDATE(nchcpu, ncs_goodhits);
		UPDATE(nchcpu, ncs_neghits);
		UPDATE(nchcpu, ncs_badhits);
		UPDATE(nchcpu, ncs_falsehits);
		UPDATE(nchcpu, ncs_miss);
		UPDATE(nchcpu, ncs_long);
		UPDATE(nchcpu, ncs_pass2);
		UPDATE(nchcpu, ncs_2passes);
		UPDATE(nchcpu, ncs_revhits);
		UPDATE(nchcpu, ncs_revmiss);
		UPDATE(nchcpu, ncs_denied);
	}
	if (cookie != NULL) {
		memcpy(cookie, &nchstats, sizeof(nchstats));
	}
	/* Reset the timer; arrive back here in N minutes at latest. */
	callout_schedule(&cache_stat_callout, cache_stat_interval * hz);
	mutex_exit(&cache_stat_lock);
}

/*
 * Fetch the current values of the stats for sysctl.
 */
static int
cache_stat_sysctl(SYSCTLFN_ARGS)
{
	struct nchstats stats;

	if (oldp == NULL) {
		*oldlenp = sizeof(nchstats);
		return 0;
	}

	if (*oldlenp <= 0) {
		*oldlenp = 0;
		return 0;
	}

	/* Refresh the global stats. */
	sysctl_unlock();
	cache_update_stats(&stats);
	sysctl_relock();

	*oldlenp = MIN(sizeof(stats), *oldlenp);
	return sysctl_copyout(l, &stats, oldp, *oldlenp);
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
	for (id = 0; id < LRU_COUNT; id++) {
		TAILQ_FOREACH(ncp, &cache_lru.list[id], nc_lru) {
			if (ncp->nc_vp == dvp) {
				(*pr)("parent %.*s\n", ncp->nc_nlen,
				    ncp->nc_name);
			}
		}
	}
}
#endif
