/*	$NetBSD: vfs_cache.c,v 1.42 2003/05/21 09:36:06 fvdl Exp $	*/

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
 *	@(#)vfs_cache.c	8.3 (Berkeley) 8/22/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cache.c,v 1.42 2003/05/21 09:36:06 fvdl Exp $");

#include "opt_ddb.h"
#include "opt_revcache.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/lock.h>

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
 * Structures associated with name cacheing.
 */
LIST_HEAD(nchashhead, namecache) *nchashtbl;
u_long	nchash;				/* size of hash table - 1 */
long	numcache;			/* number of cache entries allocated */
#define	NCHASH(cnp, dvp)	(((cnp)->cn_hash ^ (dvp)->v_id) & nchash)

LIST_HEAD(ncvhashhead, namecache) *ncvhashtbl;
u_long	ncvhash;			/* size of hash table - 1 */
#define	NCVHASH(vp)		((vp)->v_id & ncvhash)

TAILQ_HEAD(, namecache) nclruhead;		/* LRU chain */
struct	nchstats nchstats;		/* cache effectiveness statistics */

struct pool namecache_pool;

MALLOC_DEFINE(M_CACHE, "namecache", "Dynamically allocated cache entries");

int doingcache = 1;			/* 1 => enable the cache */

/* A single lock to protect cache insertion, removal and lookup */
static struct simplelock namecache_slock = SIMPLELOCK_INITIALIZER;

/*
 * Look for a the name in the cache. We don't do this
 * if the segment name is long, simply so the cache can avoid
 * holding long names (which would either waste space, or
 * add greatly to the complexity).
 *
 * Lookup is called with ni_dvp pointing to the directory to search,
 * ni_ptr pointing to the name of the entry being sought, ni_namelen
 * tells the length of the name, and ni_hash contains a hash of
 * the name. If the lookup succeeds, the vnode is locked, stored in ni_vp
 * and a status of zero is returned. If the locking fails for whatever
 * reason, the vnode is unlocked and the error is returned to caller.
 * If the lookup determines that the name does not exist (negative cacheing),
 * a status of ENOENT is returned. If the lookup fails, a status of -1
 * is returned.
 */
int
cache_lookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp)
{
	struct namecache *ncp;
	struct nchashhead *ncpp;
	struct vnode *vp;
	u_long vpid;
	int error;

	if (!doingcache) {
		cnp->cn_flags &= ~MAKEENTRY;
		*vpp = NULL;
		return (-1);
	}

	simple_lock(&namecache_slock);
	if (cnp->cn_namelen > NCHNAMLEN) {
		nchstats.ncs_long++;
		cnp->cn_flags &= ~MAKEENTRY;
		goto fail_wlock;
	}
	ncpp = &nchashtbl[NCHASH(cnp, dvp)];
	LIST_FOREACH(ncp, ncpp, nc_hash) {
		if (ncp->nc_dvp == dvp &&
		    ncp->nc_dvpid == dvp->v_id &&
		    ncp->nc_nlen == cnp->cn_namelen &&
		    !memcmp(ncp->nc_name, cnp->cn_nameptr, (u_int)ncp->nc_nlen))
			break;
	}
	if (ncp == 0) {
		nchstats.ncs_miss++;
		goto fail_wlock;
	}
	if ((cnp->cn_flags & MAKEENTRY) == 0) {
		nchstats.ncs_badhits++;
		goto remove;
	} else if (ncp->nc_vp == NULL) {
		/*
		 * Restore the ISWHITEOUT flag saved earlier.
		 */
		cnp->cn_flags |= ncp->nc_vpid;
		if (cnp->cn_nameiop != CREATE ||
		    (cnp->cn_flags & ISLASTCN) == 0) {
			nchstats.ncs_neghits++;
			/*
			 * Move this slot to end of LRU chain,
			 * if not already there.
			 */
			if (TAILQ_NEXT(ncp, nc_lru) != 0) {
				TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
				TAILQ_INSERT_TAIL(&nclruhead, ncp, nc_lru);
			}
			simple_unlock(&namecache_slock);
			return (ENOENT);
		} else {
			nchstats.ncs_badhits++;
			goto remove;
		}
	} else if (ncp->nc_vpid != ncp->nc_vp->v_id) {
		nchstats.ncs_falsehits++;
		goto remove;
	}

	vp = ncp->nc_vp;
	vpid = vp->v_id;
	/* Release the name cache mutex while we acquire vnode locks */
	simple_unlock(&namecache_slock);

	if (vp == dvp) {	/* lookup on "." */
		VREF(dvp);
		error = 0;
	} else if (cnp->cn_flags & ISDOTDOT) {
		VOP_UNLOCK(dvp, 0);
		cnp->cn_flags |= PDIRUNLOCK;
		error = vget(vp, LK_EXCLUSIVE);
		/*
		 * If the above vget() succeeded and both LOCKPARENT and
		 * ISLASTCN is set, lock the directory vnode as well.
		 */
		if (!error && (~cnp->cn_flags & (LOCKPARENT|ISLASTCN)) == 0) {
			if ((error = vn_lock(dvp, LK_EXCLUSIVE)) != 0) {
				vput(vp);
				return (error);
			}
			cnp->cn_flags &= ~PDIRUNLOCK;
		}
	} else {
		error = vget(vp, LK_EXCLUSIVE);
		/*
		 * If the above vget() failed or either of LOCKPARENT or
		 * ISLASTCN is set, unlock the directory vnode.
		 */
		if (error || (~cnp->cn_flags & (LOCKPARENT|ISLASTCN)) != 0) {
			VOP_UNLOCK(dvp, 0);
			cnp->cn_flags |= PDIRUNLOCK;
		}
	}

	/*
	 * Check that the lock succeeded, and that the capability number did
	 * not change while we were waiting for the lock.
	 */
	if (error || vpid != vp->v_id) {
		/* XXXSMP - updating stats without lock; do we care? */
		if (!error) {
			vput(vp);
			nchstats.ncs_falsehits++;
		} else
			nchstats.ncs_badhits++;

		/*
		 * The parent needs to be locked when we return to VOP_LOOKUP().
		 * The `.' case here should be extremely rare (if it can happen
		 * at all), so we don't bother optimizing out the unlock/relock.
		 */
		if (vp == dvp ||
		    error || (~cnp->cn_flags & (LOCKPARENT|ISLASTCN)) != 0) {
			if ((error = vn_lock(dvp, LK_EXCLUSIVE)) != 0)
				return (error);
			cnp->cn_flags &= ~PDIRUNLOCK;
		}
		*vpp = NULL;
		return (-1);
	}

	simple_lock(&namecache_slock);
	nchstats.ncs_goodhits++;
	/*
	 * Move this slot to end of LRU chain, if not already there.
	 */
	if (TAILQ_NEXT(ncp, nc_lru) != 0) {
		TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
		TAILQ_INSERT_TAIL(&nclruhead, ncp, nc_lru);
	}

	simple_unlock(&namecache_slock);
	*vpp = vp;
	return (0);

remove:
	/*
	 * Last component and we are renaming or deleting,
	 * the cache entry is invalid, or otherwise don't
	 * want cache entry to exist.
	 */
	TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
	LIST_REMOVE(ncp, nc_hash);
	ncp->nc_hash.le_prev = NULL;
	if (ncp->nc_vhash.le_prev != NULL) {
		LIST_REMOVE(ncp, nc_vhash);
		ncp->nc_vhash.le_prev = NULL;
	}
	TAILQ_INSERT_HEAD(&nclruhead, ncp, nc_lru);

fail_wlock:
	simple_unlock(&namecache_slock);
	*vpp = NULL;
	return (-1);
}

/*
 * Scan cache looking for name of directory entry pointing at vp.
 *
 * Fill in dvpp.
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
	struct ncvhashhead *nvcpp;
	char *bp;

	if (!doingcache)
		goto out;

	nvcpp = &ncvhashtbl[NCVHASH(vp)];

	simple_lock(&namecache_slock);
	LIST_FOREACH(ncp, nvcpp, nc_vhash) {
		if (ncp->nc_vp == vp &&
		    ncp->nc_vpid == vp->v_id &&
		    (dvp = ncp->nc_dvp) != NULL &&
		    dvp != vp && 		/* avoid pesky . entries.. */
		    dvp->v_id == ncp->nc_dvpid) {

#ifdef DIAGNOSTIC
			if (ncp->nc_nlen == 1 &&
			    ncp->nc_name[0] == '.')
				panic("cache_revlookup: found entry for .");

			if (ncp->nc_nlen == 2 &&
			    ncp->nc_name[0] == '.' &&
			    ncp->nc_name[1] == '.')
				panic("cache_revlookup: found entry for ..");
#endif
			nchstats.ncs_revhits++;

			if (bufp) {
				bp = *bpp;
				bp -= ncp->nc_nlen;
				if (bp <= bufp) {
					*dvpp = NULL;
					simple_unlock(&namecache_slock);
					return (ERANGE);
				}
				memcpy(bp, ncp->nc_name, ncp->nc_nlen);
				*bpp = bp;
			}

			/* XXX MP: how do we know dvp won't evaporate? */
			*dvpp = dvp;
			simple_unlock(&namecache_slock);
			return (0);
		}
	}
	nchstats.ncs_revmiss++;
	simple_unlock(&namecache_slock);
 out:
	*dvpp = NULL;
	return (-1);
}

/*
 * Add an entry to the cache
 */
void
cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp)
{
	struct namecache *ncp;
	struct nchashhead *ncpp;
	struct ncvhashhead *nvcpp;

#ifdef DIAGNOSTIC
	if (cnp->cn_namelen > NCHNAMLEN)
		panic("cache_enter: name too long");
#endif
	if (!doingcache)
		return;
	/*
	 * Free the cache slot at head of lru chain.
	 */
	simple_lock(&namecache_slock);
	if (numcache < numvnodes) {
		numcache++;
		simple_unlock(&namecache_slock);
		ncp = pool_get(&namecache_pool, PR_WAITOK);
		memset(ncp, 0, sizeof(*ncp));
		simple_lock(&namecache_slock);
	} else if ((ncp = TAILQ_FIRST(&nclruhead)) != NULL) {
		TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
		if (ncp->nc_hash.le_prev != NULL) {
			LIST_REMOVE(ncp, nc_hash);
			ncp->nc_hash.le_prev = NULL;
		}
		if (ncp->nc_vhash.le_prev != NULL) {
			LIST_REMOVE(ncp, nc_vhash);
			ncp->nc_vhash.le_prev = NULL;
		}
	} else {
		simple_unlock(&namecache_slock);
		return;
	}
	/* Grab the vnode we just found. */
	ncp->nc_vp = vp;
	if (vp)
		ncp->nc_vpid = vp->v_id;
	else {
		/*
		 * For negative hits, save the ISWHITEOUT flag so we can
		 * restore it later when the cache entry is used again.
		 */
		ncp->nc_vpid = cnp->cn_flags & ISWHITEOUT;
	}
	/* Fill in cache info. */
	ncp->nc_dvp = dvp;
	ncp->nc_dvpid = dvp->v_id;
	ncp->nc_nlen = cnp->cn_namelen;
	memcpy(ncp->nc_name, cnp->cn_nameptr, (unsigned)ncp->nc_nlen);
	TAILQ_INSERT_TAIL(&nclruhead, ncp, nc_lru);
	ncpp = &nchashtbl[NCHASH(cnp, dvp)];
	LIST_INSERT_HEAD(ncpp, ncp, nc_hash);

	ncp->nc_vhash.le_prev = NULL;
	ncp->nc_vhash.le_next = NULL;

	/*
	 * Create reverse-cache entries (used in getcwd) for directories.
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
	simple_unlock(&namecache_slock);
}

/*
 * Name cache initialization, from vfs_init() when we are booting
 */
void
nchinit(void)
{

	TAILQ_INIT(&nclruhead);
	nchashtbl =
	    hashinit(desiredvnodes, HASH_LIST, M_CACHE, M_WAITOK, &nchash);
	ncvhashtbl =
#ifdef NAMECACHE_ENTER_REVERSE
	    hashinit(desiredvnodes, HASH_LIST, M_CACHE, M_WAITOK, &ncvhash);
#else
	    hashinit(desiredvnodes/8, HASH_LIST, M_CACHE, M_WAITOK, &ncvhash);
#endif
	pool_init(&namecache_pool, sizeof(struct namecache), 0, 0, 0,
	    "ncachepl", &pool_allocator_nointr);
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

	hash1 = hashinit(desiredvnodes, HASH_LIST, M_CACHE, M_WAITOK, &mask1);
	hash2 =
#ifdef NAMECACHE_ENTER_REVERSE
	    hashinit(desiredvnodes, HASH_LIST, M_CACHE, M_WAITOK, &mask2);
#else
	    hashinit(desiredvnodes/8, HASH_LIST, M_CACHE, M_WAITOK, &mask2);
#endif
	simple_lock(&namecache_slock);
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
	simple_unlock(&namecache_slock);
	hashdone(oldhash1, M_CACHE);
	hashdone(oldhash2, M_CACHE);
}

/*
 * Cache flush, a particular vnode; called when a vnode is renamed to
 * hide entries that would now be invalid
 */
void
cache_purge(struct vnode *vp)
{
	struct namecache *ncp;
	struct nchashhead *ncpp;
	static u_long nextvnodeid;

	simple_lock(&namecache_slock);
	vp->v_id = ++nextvnodeid;
	if (nextvnodeid != 0)
		goto out;
	for (ncpp = &nchashtbl[nchash]; ncpp >= nchashtbl; ncpp--) {
		LIST_FOREACH(ncp, ncpp, nc_hash) {
			ncp->nc_vpid = 0;
			ncp->nc_dvpid = 0;
		}
	}
	vp->v_id = ++nextvnodeid;
out:
	simple_unlock(&namecache_slock);
}

/*
 * Cache flush, a whole filesystem; called when filesys is umounted to
 * remove entries that would now be invalid.
 */
void
cache_purgevfs(struct mount *mp)
{
	struct namecache *ncp, *nxtcp;

	simple_lock(&namecache_slock);
	for (ncp = TAILQ_FIRST(&nclruhead); ncp != NULL; ncp = nxtcp) {
		nxtcp = TAILQ_NEXT(ncp, nc_lru);
		if (ncp->nc_dvp == NULL || ncp->nc_dvp->v_mount != mp) {
			continue;
		}
		/* Free the resources we had. */
		ncp->nc_vp = NULL;
		ncp->nc_dvp = NULL;
		TAILQ_REMOVE(&nclruhead, ncp, nc_lru);
		if (ncp->nc_hash.le_prev != NULL) {
			LIST_REMOVE(ncp, nc_hash);
			ncp->nc_hash.le_prev = NULL;
		}
		if (ncp->nc_vhash.le_prev != NULL) {
			LIST_REMOVE(ncp, nc_vhash);
			ncp->nc_vhash.le_prev = NULL;
		}
		TAILQ_INSERT_HEAD(&nclruhead, ncp, nc_lru);
	}
	simple_unlock(&namecache_slock);
}

#ifdef DDB
void
namecache_print(struct vnode *vp, void (*pr)(const char *, ...))
{
	struct vnode *dvp = NULL;
	struct namecache *ncp;

	TAILQ_FOREACH(ncp, &nclruhead, nc_lru) {
		if (ncp->nc_vp == vp && ncp->nc_vpid == vp->v_id) {
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
		if (ncp->nc_vp == vp && ncp->nc_vpid == vp->v_id) {
			(*pr)("parent %.*s\n", ncp->nc_nlen, ncp->nc_name);
		}
	}
}
#endif
