/*	$NetBSD: kernfs_subr.c,v 1.28 2014/07/17 08:21:34 hannken Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kernfs_subr.c,v 1.28 2014/07/17 08:21:34 hannken Exp $");

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
#include <miscfs/specfs/specdev.h>

void kernfs_hashins(struct kernfs_node *);
void kernfs_hashrem(struct kernfs_node *);
struct vnode *kernfs_hashget(kfstype, struct mount *,
    const struct kern_target *);

static LIST_HEAD(kfs_hashhead, kernfs_node) *kfs_hashtbl;
static u_long	kfs_ihash;	/* size of hash table - 1 */
#define KFSHASH(v)	((v) & kfs_ihash)

static kmutex_t kfs_hashlock;
static kmutex_t kfs_ihash_lock;

#define	ISSET(t, f)	((t) & (f))

/*
 * allocate a kfsnode/vnode pair.  the vnode is
 * referenced, and locked.
 *
 * the kfs_type and mount point uniquely
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
kernfs_allocvp(struct mount *mp, struct vnode **vpp,
    const struct kern_target *kt)
{
	struct kernfs_node *kfs = NULL, *kfsp;
	struct vnode *vp = NULL;
	int error;
	long *cookie;

	if ((*vpp = kernfs_hashget(kt->kt_tag, mp, kt)) != NULL)
		return (0);

	mutex_enter(&kfs_hashlock);
	if ((*vpp = kernfs_hashget(kt->kt_tag, mp, kt)) != NULL) {
		mutex_exit(&kfs_hashlock);
		return (0);
	}

	error = getnewvnode(VT_KERNFS, mp, kernfs_vnodeop_p, NULL, &vp);
	if (error) {
		*vpp = NULL;
		mutex_exit(&kfs_hashlock);
		return (error);
	}

	kfs = malloc(sizeof(struct kernfs_node), M_TEMP, M_WAITOK|M_ZERO);
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

	kfs->kfs_type = kt->kt_tag;
	kfs->kfs_vnode = vp;
	kfs->kfs_fileno = KERNFS_FILENO(kt, kt->kt_tag, kfs->kfs_cookie);
	kfs->kfs_kt = kt;
	kfs->kfs_mode = kt->kt_mode;
	vp->v_type = kt->kt_vtype;

	if (kt->kt_tag == KFSkern)
		vp->v_vflag = VV_ROOT;

	if (kt->kt_tag == KFSdevice) {
		spec_node_init(vp, *(dev_t *)kt->kt_data);
	}

	kernfs_hashins(kfs);
	uvm_vnp_setsize(vp, 0);
	mutex_exit(&kfs_hashlock);

	*vpp = vp;
	return (0);
}

int
kernfs_freevp(struct vnode *vp)
{
	struct kernfs_node *kfs = VTOKERN(vp);

	kernfs_hashrem(kfs);
	TAILQ_REMOVE(&VFSTOKERNFS(vp->v_mount)->nodelist, kfs, kfs_list);

	free(vp->v_data, M_TEMP);
	vp->v_data = 0;
	return (0);
}

/*
 * Initialize kfsnode hash table.
 */
void
kernfs_hashinit(void)
{

	mutex_init(&kfs_hashlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&kfs_ihash_lock, MUTEX_DEFAULT, IPL_NONE);
	kfs_hashtbl = hashinit(desiredvnodes / 4, HASH_LIST, true, &kfs_ihash);
}

void
kernfs_hashreinit(void)
{
	struct kernfs_node *pp;
	struct kfs_hashhead *oldhash, *hash;
	u_long i, oldmask, mask, val;

	hash = hashinit(desiredvnodes / 4, HASH_LIST, true, &mask);

	mutex_enter(&kfs_ihash_lock);
	oldhash = kfs_hashtbl;
	oldmask = kfs_ihash;
	kfs_hashtbl = hash;
	kfs_ihash = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((pp = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(pp, kfs_hash);
			val = KFSHASH(pp->kfs_type);
			LIST_INSERT_HEAD(&hash[val], pp, kfs_hash);
		}
	}
	mutex_exit(&kfs_ihash_lock);
	hashdone(oldhash, HASH_LIST, oldmask);
}

/*
 * Free kfsnode hash table.
 */
void
kernfs_hashdone(void)
{

	hashdone(kfs_hashtbl, HASH_LIST, kfs_ihash);
	mutex_destroy(&kfs_hashlock);
	mutex_destroy(&kfs_ihash_lock);
}

struct vnode *
kernfs_hashget(kfstype type, struct mount *mp, const struct kern_target *kt)
{
	struct kfs_hashhead *ppp;
	struct kernfs_node *pp;
	struct vnode *vp;

 loop:
	mutex_enter(&kfs_ihash_lock);
	ppp = &kfs_hashtbl[KFSHASH(type)];
	LIST_FOREACH(pp, ppp, kfs_hash) {
		vp = KERNFSTOV(pp);
		if (pp->kfs_type == type && vp->v_mount == mp &&
		    pp->kfs_kt == kt) {
			mutex_enter(vp->v_interlock);
			mutex_exit(&kfs_ihash_lock);
			if (vget(vp, LK_EXCLUSIVE))
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
kernfs_hashins(struct kernfs_node *pp)
{
	struct kfs_hashhead *ppp;
	int error __diagused;

	/* lock the kfsnode, then put it on the appropriate hash list */
	error = VOP_LOCK(KERNFSTOV(pp), LK_EXCLUSIVE);
	KASSERT(error == 0);

	mutex_enter(&kfs_ihash_lock);
	ppp = &kfs_hashtbl[KFSHASH(pp->kfs_type)];
	LIST_INSERT_HEAD(ppp, pp, kfs_hash);
	mutex_exit(&kfs_ihash_lock);
}

/*
 * Remove the kfsnode from the hash table.
 */
void
kernfs_hashrem(struct kernfs_node *pp)
{
	mutex_enter(&kfs_ihash_lock);
	LIST_REMOVE(pp, kfs_hash);
	mutex_exit(&kfs_ihash_lock);
}
