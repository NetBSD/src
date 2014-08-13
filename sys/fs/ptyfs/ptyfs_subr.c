/*	$NetBSD: ptyfs_subr.c,v 1.30 2014/08/13 14:10:00 hannken Exp $	*/

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
 *	@(#)ptyfs_subr.c	8.6 (Berkeley) 5/14/95
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
 *	@(#)procfs_subr.c	8.6 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ptyfs_subr.c,v 1.30 2014/08/13 14:10:00 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/pty.h>
#include <sys/kauth.h>
#include <sys/lwp.h>

#include <fs/ptyfs/ptyfs.h>
#include <miscfs/specfs/specdev.h>

static kmutex_t ptyfs_hashlock;

static LIST_HEAD(ptyfs_hashhead, ptyfsnode) *ptyfs_used_tbl, *ptyfs_free_tbl;
static u_long ptyfs_used_mask, ptyfs_free_mask; /* size of hash table - 1 */
static kmutex_t ptyfs_used_slock, ptyfs_free_slock;

static void ptyfs_hashins(struct ptyfsnode *);
static void ptyfs_hashrem(struct ptyfsnode *);

static struct vnode *ptyfs_used_get(ptyfstype, int, struct mount *, int);
static struct ptyfsnode *ptyfs_free_get(ptyfstype, int);

static void ptyfs_rehash(kmutex_t *, struct ptyfs_hashhead **,
    u_long *);

#define PTYHASH(type, pty, mask) (PTYFS_FILENO(type, pty) % (mask + 1))


/*
 * allocate a ptyfsnode/vnode pair.  the vnode is
 * referenced, and locked.
 *
 * the pty, ptyfs_type, and mount point uniquely
 * identify a ptyfsnode.  the mount point is needed
 * because someone might mount this filesystem
 * twice.
 *
 * all ptyfsnodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one ptyess trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
ptyfs_allocvp(struct mount *mp, struct vnode **vpp, ptyfstype type, int pty)
{
	struct ptyfsnode *ptyfs;
	struct vnode *vp;
	int error;

 retry:
	if ((*vpp = ptyfs_used_get(type, pty, mp, LK_EXCLUSIVE)) != NULL)
		return 0;

	error = getnewvnode(VT_PTYFS, mp, ptyfs_vnodeop_p, NULL, &vp);
	if (error) {
		*vpp = NULL;
		return error;
	}

	mutex_enter(&ptyfs_hashlock);
	if (ptyfs_used_get(type, pty, mp, 0) != NULL) {
		mutex_exit(&ptyfs_hashlock);
		ungetnewvnode(vp);
		goto retry;
	}

	vp->v_data = ptyfs = ptyfs_free_get(type, pty);
	ptyfs->ptyfs_vnode = vp;

	switch (type) {
	case PTYFSroot:	/* /pts = dr-xr-xr-x */
		vp->v_type = VDIR;
		vp->v_vflag = VV_ROOT;
		break;

	case PTYFSpts:	/* /pts/N = cxxxxxxxxx */
	case PTYFSptc:	/* controlling side = cxxxxxxxxx */
		vp->v_type = VCHR;
		spec_node_init(vp, PTYFS_MAKEDEV(ptyfs));
		break;
	default:
		panic("ptyfs_allocvp");
	}

	ptyfs_hashins(ptyfs);
	uvm_vnp_setsize(vp, 0);
	mutex_exit(&ptyfs_hashlock);

	*vpp = vp;
	return 0;
}

int
ptyfs_freevp(struct vnode *vp)
{
	struct ptyfsnode *ptyfs = VTOPTYFS(vp);

	ptyfs_hashrem(ptyfs);
	vp->v_data = NULL;
	return 0;
}

/*
 * Initialize ptyfsnode hash table.
 */
void
ptyfs_hashinit(void)
{
	ptyfs_used_tbl = hashinit(desiredvnodes / 4, HASH_LIST, true,
	    &ptyfs_used_mask);
	ptyfs_free_tbl = hashinit(desiredvnodes / 4, HASH_LIST, true,
	    &ptyfs_free_mask);
	mutex_init(&ptyfs_hashlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&ptyfs_used_slock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&ptyfs_free_slock, MUTEX_DEFAULT, IPL_NONE);
}

void
ptyfs_hashreinit(void)
{
	ptyfs_rehash(&ptyfs_used_slock, &ptyfs_used_tbl, &ptyfs_used_mask);
	ptyfs_rehash(&ptyfs_free_slock, &ptyfs_free_tbl, &ptyfs_free_mask);
}

static void
ptyfs_rehash(kmutex_t *hlock, struct ptyfs_hashhead **hhead,
    u_long *hmask)
{
	struct ptyfsnode *pp;
	struct ptyfs_hashhead *oldhash, *hash;
	u_long i, oldmask, mask, val;

	hash = hashinit(desiredvnodes / 4, HASH_LIST, true, &mask);

	mutex_enter(hlock);
	oldhash = *hhead;
	oldmask = *hmask;
	*hhead = hash;
	*hmask = mask;
	for (i = 0; i <= oldmask; i++) {
		while ((pp = LIST_FIRST(&oldhash[i])) != NULL) {
			LIST_REMOVE(pp, ptyfs_hash);
			val = PTYHASH(pp->ptyfs_type, pp->ptyfs_pty,
			    ptyfs_used_mask);
			LIST_INSERT_HEAD(&hash[val], pp, ptyfs_hash);
		}
	}
	mutex_exit(hlock);
	hashdone(oldhash, HASH_LIST, oldmask);
}

/*
 * Free ptyfsnode hash table.
 */
void
ptyfs_hashdone(void)
{
	
	mutex_destroy(&ptyfs_hashlock);
	mutex_destroy(&ptyfs_used_slock);
	mutex_destroy(&ptyfs_free_slock);
	hashdone(ptyfs_used_tbl, HASH_LIST, ptyfs_used_mask);
	hashdone(ptyfs_free_tbl, HASH_LIST, ptyfs_free_mask);
}

/*
 * Get a ptyfsnode from the free table, or allocate one.
 * Removes the node from the free table.
 */
struct ptyfsnode *
ptyfs_free_get(ptyfstype type, int pty)
{
	struct ptyfs_hashhead *ppp;
	struct ptyfsnode *pp;

	mutex_enter(&ptyfs_free_slock);
	ppp = &ptyfs_free_tbl[PTYHASH(type, pty, ptyfs_free_mask)];
	LIST_FOREACH(pp, ppp, ptyfs_hash) {
		if (pty == pp->ptyfs_pty && pp->ptyfs_type == type) {
			LIST_REMOVE(pp, ptyfs_hash);
			mutex_exit(&ptyfs_free_slock);
			return pp;
		}
	}
	mutex_exit(&ptyfs_free_slock);

	pp = malloc(sizeof(struct ptyfsnode), M_TEMP, M_WAITOK);
	pp->ptyfs_pty = pty;
	pp->ptyfs_type = type;
	pp->ptyfs_fileno = PTYFS_FILENO(pty, type);
	if (pp->ptyfs_type == PTYFSroot)
		pp->ptyfs_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|
		    S_IROTH|S_IXOTH;
	else
		pp->ptyfs_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|
		    S_IROTH|S_IWOTH;

	pp->ptyfs_uid = pp->ptyfs_gid = 0;
	pp->ptyfs_status = PTYFS_CHANGE;
	PTYFS_ITIMES(pp, NULL, NULL, NULL);
	pp->ptyfs_birthtime = pp->ptyfs_mtime =
	    pp->ptyfs_atime = pp->ptyfs_ctime;
	pp->ptyfs_flags = 0;
	return pp;
}

struct vnode *
ptyfs_used_get(ptyfstype type, int pty, struct mount *mp, int flags)
{
	struct ptyfs_hashhead *ppp;
	struct ptyfsnode *pp;
	struct vnode *vp;

loop:
	mutex_enter(&ptyfs_used_slock);
	ppp = &ptyfs_used_tbl[PTYHASH(type, pty, ptyfs_used_mask)];
	LIST_FOREACH(pp, ppp, ptyfs_hash) {
		vp = PTYFSTOV(pp);
		if (pty == pp->ptyfs_pty && pp->ptyfs_type == type &&
		    vp->v_mount == mp) {
		    	if (flags == 0) {
				mutex_exit(&ptyfs_used_slock);
			} else {
				mutex_enter(vp->v_interlock);
				mutex_exit(&ptyfs_used_slock);
				if (vget(vp, flags))
					goto loop;
			}
			return vp;
		}
	}
	mutex_exit(&ptyfs_used_slock);
	return NULL;
}

/*
 * Insert the ptyfsnode into the used table and lock it.
 */
static void
ptyfs_hashins(struct ptyfsnode *pp)
{
	struct ptyfs_hashhead *ppp;
	int error __diagused;

	/* lock the ptyfsnode, then put it on the appropriate hash list */
	error = VOP_LOCK(PTYFSTOV(pp), LK_EXCLUSIVE);
	KASSERT(error == 0);

	mutex_enter(&ptyfs_used_slock);
	ppp = &ptyfs_used_tbl[PTYHASH(pp->ptyfs_type, pp->ptyfs_pty,
	    ptyfs_used_mask)];
	LIST_INSERT_HEAD(ppp, pp, ptyfs_hash);
	mutex_exit(&ptyfs_used_slock);
}

/*
 * Remove the ptyfsnode from the used table, and add it to the free table
 */
static void
ptyfs_hashrem(struct ptyfsnode *pp)
{
	struct ptyfs_hashhead *ppp;

	mutex_enter(&ptyfs_used_slock);
	LIST_REMOVE(pp, ptyfs_hash);
	mutex_exit(&ptyfs_used_slock);

	mutex_enter(&ptyfs_free_slock);
	ppp = &ptyfs_free_tbl[PTYHASH(pp->ptyfs_type, pp->ptyfs_pty,
	    ptyfs_free_mask)];
	LIST_INSERT_HEAD(ppp, pp, ptyfs_hash);
	mutex_exit(&ptyfs_free_slock);
}

/*
 * Mark this controlling pty as active.
 */
void
ptyfs_set_active(struct mount *mp, int pty)
{
	struct ptyfsmount *pmnt = VFSTOPTY(mp);

	KASSERT(pty >= 0);
	/* Reallocate map if needed. */
	if (pty >= pmnt->pmnt_bitmap_size * NBBY) {
		int osize, nsize;
		uint8_t *obitmap, *nbitmap;

		nsize = roundup(howmany(pty + 1, NBBY), 64);
		nbitmap = kmem_alloc(nsize, KM_SLEEP);
		mutex_enter(&pmnt->pmnt_lock);
		if (pty < pmnt->pmnt_bitmap_size * NBBY) {
			mutex_exit(&pmnt->pmnt_lock);
			kmem_free(nbitmap, nsize);
		} else {
			osize = pmnt->pmnt_bitmap_size;
			obitmap = pmnt->pmnt_bitmap;
			pmnt->pmnt_bitmap_size = nsize;
			pmnt->pmnt_bitmap = nbitmap;
			if (osize > 0)
				memcpy(pmnt->pmnt_bitmap, obitmap, osize);
			memset(pmnt->pmnt_bitmap + osize, 0, nsize - osize);
			mutex_exit(&pmnt->pmnt_lock);
			if (osize > 0)
				kmem_free(obitmap, osize);
		}
	}

	mutex_enter(&pmnt->pmnt_lock);
	setbit(pmnt->pmnt_bitmap, pty);
	mutex_exit(&pmnt->pmnt_lock);
}

/*
 * Mark this controlling pty as inactive.
 */
void
ptyfs_clr_active(struct mount *mp, int pty)
{
	struct ptyfsmount *pmnt = VFSTOPTY(mp);

	KASSERT(pty >= 0);
	mutex_enter(&pmnt->pmnt_lock);
	if (pty >= 0 && pty < pmnt->pmnt_bitmap_size * NBBY)
		clrbit(pmnt->pmnt_bitmap, pty);
	mutex_exit(&pmnt->pmnt_lock);
}

/*
 * Lookup the next active controlling pty greater or equal "pty".
 * Return -1 if not found.
 */
int
ptyfs_next_active(struct mount *mp, int pty)
{
	struct ptyfsmount *pmnt = VFSTOPTY(mp);

	KASSERT(pty >= 0);
	mutex_enter(&pmnt->pmnt_lock);
	while (pty < pmnt->pmnt_bitmap_size * NBBY) {
		if (isset(pmnt->pmnt_bitmap, pty)) {
			mutex_exit(&pmnt->pmnt_lock);
			return pty;
		}
		pty++;
	}
	mutex_exit(&pmnt->pmnt_lock);
	return -1;
}
