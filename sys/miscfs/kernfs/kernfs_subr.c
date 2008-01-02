/*	$NetBSD: kernfs_subr.c,v 1.14 2008/01/02 11:49:00 ad Exp $	*/

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)kernfs_subr.c	8.6 (Berkeley) 5/14/95
 */

/*
 * Copyright (c) 1994 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1993 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)kernfs_subr.c	8.6 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kernfs_subr.c,v 1.14 2008/01/02 11:49:00 ad Exp $");

#ifdef _KERNEL_OPT
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mount.h>

#include <miscfs/kernfs/kernfs.h>

#ifdef IPSEC
#include <sys/mbuf.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>
#include <netkey/keydb.h>
#include <netkey/key.h>
#endif

void kernfs_hashins(struct kernfs_node *);
void kernfs_hashrem(struct kernfs_node *);
struct vnode *kernfs_hashget(kfstype, struct mount *,
    const struct kern_target *, u_int32_t);

static LIST_HEAD(kfs_hashhead, kernfs_node) *kfs_hashtbl;
static u_long	kfs_ihash;	/* size of hash table - 1 */
#define KFSVALUEHASH(v)	((v) & kfs_ihash)

static kmutex_t kfs_hashlock;
static kmutex_t kfs_ihash_lock;

#define	ISSET(t, f)	((t) & (f))

/*
 * allocate a kfsnode/vnode pair.  the vnode is
 * referenced, and locked.
 *
 * the kfs_type, kfs_value and mount point uniquely
 * identify a kfsnode.  the mount point is needed
 * because someone might mount this filesystem
 * twice.
 *
 * all kfsnodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one process trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
kernfs_allocvp(mp, vpp, kfs_type, kt, value)
	struct mount *mp;
	struct vnode **vpp;
	kfstype kfs_type;
	const struct kern_target *kt;
	u_int32_t value;
{
	struct kernfs_node *kfs = NULL, *kfsp;
	struct vnode *vp = NULL;
	int error;
	long *cookie;

	if ((*vpp = kernfs_hashget(kfs_type, mp, kt, value)) != NULL)
		return (0);

	mutex_enter(&kfs_hashlock);
	if ((*vpp = kernfs_hashget(kfs_type, mp, kt, value)) != NULL) {
		mutex_exit(&kfs_hashlock);
		return (0);
	}

	if (kfs_type == KFSdevice) {
			/* /kern/rootdev = look for device and obey */
			/* /kern/rrootdev = look for device and obey */
		dev_t *dp;
		struct vnode *fvp;

#ifdef DIAGNOSTIC
		if (!kt)
			panic("kernfs: kt == NULL for KFSdevice");
#endif
		dp = kt->kt_data;
	loop:
		if (*dp == NODEV || !vfinddev(*dp, kt->kt_vtype, &fvp)) {
			mutex_exit(&kfs_hashlock);
			return (ENOENT);
		}
		vp = fvp;
		if (vget(fvp, LK_EXCLUSIVE))
			goto loop;
		*vpp = vp;
		mutex_exit(&kfs_hashlock);
		return (0);
	}

	if ((error = getnewvnode(VT_KERNFS, mp, kernfs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		mutex_exit(&kfs_hashlock);
		return (error);
	}

	MALLOC(kfs, void *, sizeof(struct kernfs_node), M_TEMP, M_WAITOK);
	memset(kfs, 0, sizeof(*kfs));
	vp->v_data = kfs;
	cookie = &(VFSTOKERNFS(mp)->fileno_cookie);
again:
	TAILQ_FOREACH(kfsp, &VFSTOKERNFS(mp)->nodelist, kfs_list) {
		if (kfsp->kfs_cookie == *cookie) {
			(*cookie) ++;
			goto again;
		}
		if (TAILQ_NEXT(kfsp, kfs_list)) {
			if (kfsp->kfs_cookie < *cookie &&
			    *cookie < TAILQ_NEXT(kfsp, kfs_list)->kfs_cookie)
				break;
			if (kfsp->kfs_cookie + 1 <
			    TAILQ_NEXT(kfsp, kfs_list)->kfs_cookie) {
				*cookie = kfsp->kfs_cookie + 1;
				break;
			}
		}
	}

	kfs->kfs_cookie = *cookie;

	if (kfsp)
		TAILQ_INSERT_AFTER(&VFSTOKERNFS(mp)->nodelist, kfsp, kfs,
		    kfs_list);
	else
		TAILQ_INSERT_TAIL(&VFSTOKERNFS(mp)->nodelist, kfs, kfs_list);

	kfs->kfs_type = kfs_type;
	kfs->kfs_vnode = vp;
	kfs->kfs_fileno = KERNFS_FILENO(kt, kfs_type, kfs->kfs_cookie);
	kfs->kfs_value = value;
	kfs->kfs_kt = kt;
	kfs->kfs_mode = kt->kt_mode;
	vp->v_type = kt->kt_vtype;

	if (kfs_type == KFSkern)
		vp->v_vflag = VV_ROOT;

	kernfs_hashins(kfs);
	uvm_vnp_setsize(vp, 0);
	mutex_exit(&kfs_hashlock);

	*vpp = vp;
	return (0);
}

int
kernfs_freevp(vp)
	struct vnode *vp;
{
	struct kernfs_node *kfs = VTOKERN(vp);

	kernfs_hashrem(kfs);
	TAILQ_REMOVE(&VFSTOKERNFS(vp->v_mount)->nodelist, kfs, kfs_list);

	FREE(vp->v_data, M_TEMP);
	vp->v_data = 0;
	return (0);
}

/*
 * Initialize kfsnode hash table.
 */
void
kernfs_hashinit()
{

	mutex_init(&kfs_hashlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&kfs_ihash_lock, MUTEX_DEFAULT, IPL_NONE);
	kfs_hashtbl = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT,
	    M_WAITOK, &kfs_ihash);
}

void
kernfs_hashreinit()
{
	struct kernfs_node *pp;
	struct kfs_hashhead *oldhash, *hash;
	u_long i, oldmask, mask, val;

	hash = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT, M_WAITOK,
	    &mask);

	mutex_enter(&kfs_ihash_lock);
	oldhash = kfs_hashtbl;
	oldmask = kfs_ihash;
	kfs_hashtbl = hash;
	kfs_ihash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((pp = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(pp, kfs_hash);
			val = KFSVALUEHASH(pp->kfs_value);
			LIST_INSERT_HEAD(&hash[val], pp, kfs_hash);
		}
	}
	mutex_exit(&kfs_ihash_lock);
	hashdone(oldhash, M_UFSMNT);
}

/*
 * Free kfsnode hash table.
 */
void
kernfs_hashdone()
{

	hashdone(kfs_hashtbl, M_UFSMNT);
	mutex_destroy(&kfs_hashlock);
	mutex_destroy(&kfs_ihash_lock);
}

struct vnode *
kernfs_hashget(type, mp, kt, value)
	kfstype type;
	struct mount *mp;
	const struct kern_target *kt;
	u_int32_t value;
{
	struct kfs_hashhead *ppp;
	struct kernfs_node *pp;
	struct vnode *vp;

 loop:
	mutex_enter(&kfs_ihash_lock);
	ppp = &kfs_hashtbl[KFSVALUEHASH(value)];
	LIST_FOREACH(pp, ppp, kfs_hash) {
		vp = KERNFSTOV(pp);
		if (pp->kfs_type == type && vp->v_mount == mp &&
		    pp->kfs_kt == kt && pp->kfs_value == value) {
			mutex_enter(&vp->v_interlock);
			mutex_exit(&kfs_ihash_lock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
				goto loop;
			return (vp);
		}
	}
	mutex_exit(&kfs_ihash_lock);
	return (NULL);
}

/*
 * Insert the kfsnode into the hash table and lock it.
 */
void
kernfs_hashins(pp)
	struct kernfs_node *pp;
{
	struct kfs_hashhead *ppp;

	/* lock the kfsnode, then put it on the appropriate hash list */
	lockmgr(&pp->kfs_vnode->v_lock, LK_EXCLUSIVE, NULL);

	mutex_enter(&kfs_ihash_lock);
	ppp = &kfs_hashtbl[KFSVALUEHASH(pp->kfs_value)];
	LIST_INSERT_HEAD(ppp, pp, kfs_hash);
	mutex_exit(&kfs_ihash_lock);
}

/*
 * Remove the kfsnode from the hash table.
 */
void
kernfs_hashrem(pp)
	struct kernfs_node *pp;
{
	mutex_enter(&kfs_ihash_lock);
	LIST_REMOVE(pp, kfs_hash);
	mutex_exit(&kfs_ihash_lock);
}

#ifdef IPSEC
void
kernfs_revoke_sa(sav)
	struct secasvar *sav;
{
	struct kernfs_node *kfs, *pnext;
	struct vnode *vp;
	struct kfs_hashhead *ppp;
	struct mbuf *m;

	ppp = &kfs_hashtbl[KFSVALUEHASH(ntohl(sav->spi))];
	for (kfs = LIST_FIRST(ppp); kfs; kfs = pnext) {
		vp = KERNFSTOV(kfs);
		pnext = LIST_NEXT(kfs, kfs_hash);
		if (vp->v_usecount > 0 && kfs->kfs_type == KFSipsecsa &&
		    kfs->kfs_value == ntohl(sav->spi)) {
			m = key_setdumpsa_spi(sav->spi);
			if (!m)
				VOP_REVOKE(vp, REVOKEALL);
			else
				m_freem(m);
			break;
		}
	}
}

void
kernfs_revoke_sp(sp)
	struct secpolicy *sp;
{
	struct kernfs_node *kfs, *pnext;
	struct vnode *vp;
	struct kfs_hashhead *ppp;

	ppp = &kfs_hashtbl[KFSVALUEHASH(sp->id)];
	for (kfs = LIST_FIRST(ppp); kfs; kfs = pnext) {
		vp = KERNFSTOV(kfs);
		pnext = LIST_NEXT(kfs, kfs_hash);
		if (vp->v_usecount > 0 && kfs->kfs_type == KFSipsecsa &&
		    kfs->kfs_value == sp->id)
			VOP_REVOKE(vp, REVOKEALL);
	}
}
#endif
