/*	$NetBSD: ptyfs_subr.c,v 1.2.14.6 2008/02/04 09:23:57 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ptyfs_subr.c,v 1.2.14.6 2008/02/04 09:23:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/stat.h>
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

static void ptyfs_getinfo(struct ptyfsnode *, struct lwp *);

static void ptyfs_hashins(struct ptyfsnode *);
static void ptyfs_hashrem(struct ptyfsnode *);

static struct vnode *ptyfs_used_get(ptyfstype, int, struct mount *, int);
static struct ptyfsnode *ptyfs_free_get(ptyfstype, int, struct lwp *);

static void ptyfs_rehash(kmutex_t *, struct ptyfs_hashhead **,
    u_long *);

#define PTYHASH(type, pty, mask) (PTYFS_FILENO(type, pty) % (mask + 1))


static void
ptyfs_getinfo(struct ptyfsnode *ptyfs, struct lwp *l)
{
	extern struct ptm_pty *ptyfs_save_ptm, ptm_ptyfspty;

	if (ptyfs->ptyfs_type == PTYFSroot) {
		ptyfs->ptyfs_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|
		    S_IROTH|S_IXOTH;
		goto out;
	} else
		ptyfs->ptyfs_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|
		    S_IROTH|S_IWOTH;

	if (ptyfs_save_ptm != NULL && ptyfs_save_ptm != &ptm_ptyfspty) {
		int error;
		struct nameidata nd;
		char ttyname[64];
		kauth_cred_t cred;
		struct vattr va;
		/*
		 * We support traditional ptys, so we copy the info
		 * from the inode
		 */
		if ((error = (*ptyfs_save_ptm->makename)(
			ptyfs_save_ptm, l, ttyname, sizeof(ttyname),
			ptyfs->ptyfs_pty, ptyfs->ptyfs_type == PTYFSpts ? 't'
			: 'p')) != 0)
				goto out;
		NDINIT(&nd, LOOKUP, NOFOLLOW|LOCKLEAF, UIO_SYSSPACE, ttyname);
		if ((error = namei(&nd)) != 0)
			goto out;
		cred = kauth_cred_alloc();
		error = VOP_GETATTR(nd.ni_vp, &va, cred);
		kauth_cred_free(cred);
		VOP_UNLOCK(nd.ni_vp, 0);
		vrele(nd.ni_vp);
		if (error)
			goto out;
		ptyfs->ptyfs_uid = va.va_uid;
		ptyfs->ptyfs_gid = va.va_gid;
		ptyfs->ptyfs_mode = va.va_mode;
		ptyfs->ptyfs_flags = va.va_flags;
		ptyfs->ptyfs_birthtime = va.va_birthtime;
		ptyfs->ptyfs_ctime = va.va_ctime;
		ptyfs->ptyfs_mtime = va.va_mtime;
		ptyfs->ptyfs_atime = va.va_atime;
		return;
	}
out:
	ptyfs->ptyfs_uid = ptyfs->ptyfs_gid = 0;
	ptyfs->ptyfs_flags |= PTYFS_CHANGE;
	PTYFS_ITIMES(ptyfs, NULL, NULL, NULL);
	ptyfs->ptyfs_birthtime = ptyfs->ptyfs_mtime =
	    ptyfs->ptyfs_atime = ptyfs->ptyfs_ctime;
	ptyfs->ptyfs_flags = 0;
}


/*
 * allocate a ptyfsnode/vnode pair.  the vnode is
 * referenced, and locked.
 *
 * the pid, ptyfs_type, and mount point uniquely
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
ptyfs_allocvp(struct mount *mp, struct vnode **vpp, ptyfstype type, int pty,
    struct lwp *l)
{
	struct ptyfsnode *ptyfs;
	struct vnode *vp;
	int error;

 retry:
	if ((*vpp = ptyfs_used_get(type, pty, mp, LK_EXCLUSIVE)) != NULL)
		return 0;

	if ((error = getnewvnode(VT_PTYFS, mp, ptyfs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		return error;
	}

	mutex_enter(&ptyfs_hashlock);
	if (ptyfs_used_get(type, pty, mp, 0) != NULL) {
		mutex_exit(&ptyfs_hashlock);
		ungetnewvnode(vp);
		goto retry;
	}

	vp->v_data = ptyfs = ptyfs_free_get(type, pty, l);
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
	ptyfs_used_tbl = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT,
	    M_WAITOK, &ptyfs_used_mask);
	ptyfs_free_tbl = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT,
	    M_WAITOK, &ptyfs_free_mask);
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

	hash = hashinit(desiredvnodes / 4, HASH_LIST, M_UFSMNT, M_WAITOK,
	    &mask);

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
	hashdone(oldhash, M_UFSMNT);
}

/*
 * Free ptyfsnode hash table.
 */
void
ptyfs_hashdone(void)
{
	hashdone(ptyfs_used_tbl, M_UFSMNT);
	hashdone(ptyfs_free_tbl, M_UFSMNT);
}

/*
 * Get a ptyfsnode from the free table, or allocate one.
 * Removes the node from the free table.
 */
struct ptyfsnode *
ptyfs_free_get(ptyfstype type, int pty, struct lwp *l)
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

	MALLOC(pp, void *, sizeof(struct ptyfsnode), M_TEMP, M_WAITOK);
	pp->ptyfs_pty = pty;
	pp->ptyfs_type = type;
	pp->ptyfs_fileno = PTYFS_FILENO(pty, type);
	ptyfs_getinfo(pp, l);
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
				mutex_enter(&vp->v_interlock);
				mutex_exit(&ptyfs_used_slock);
				if (vget(vp, flags | LK_INTERLOCK))
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

	/* lock the ptyfsnode, then put it on the appropriate hash list */
	vlockmgr(&pp->ptyfs_vnode->v_lock, LK_EXCLUSIVE);

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
