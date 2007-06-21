/*	$NetBSD: puffs_subr.c,v 1.34 2007/06/21 14:54:49 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_subr.c,v 1.34 2007/06/21 14:54:49 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/hash.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/poll.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/genfs/genfs_node.h>
#include <miscfs/specfs/specdev.h>

struct pool puffs_pnpool;

#ifdef PUFFSDEBUG
int puffsdebug;
#endif

static __inline struct puffs_node_hashlist
	*puffs_cookie2hashlist(struct puffs_mount *, void *);
static struct puffs_node *puffs_cookie2pnode(struct puffs_mount *, void *);

static void puffs_gop_size(struct vnode *, off_t, off_t *, int);
static void puffs_gop_markupdate(struct vnode *, int);

static const struct genfs_ops puffs_genfsops = {
	.gop_size = puffs_gop_size,
	.gop_write = genfs_gop_write,
	.gop_markupdate = puffs_gop_markupdate,
#if 0
	.gop_alloc, should ask userspace
#endif
};

/*
 * Grab a vnode, intialize all the puffs-dependant stuff.
 */
int
puffs_getvnode(struct mount *mp, void *cookie, enum vtype type,
	voff_t vsize, dev_t rdev, struct vnode **vpp)
{
	struct puffs_mount *pmp;
	struct vnode *vp, *nvp;
	struct puffs_node *pnode;
	struct puffs_node_hashlist *plist;
	int error;

	pmp = MPTOPUFFSMP(mp);

	/*
	 * XXX: there is a deadlock condition between vfs_busy() and
	 * vnode locks.  For an unmounting file system the mountpoint
	 * is frozen, but in unmount(FORCE) vflush() wants to access all
	 * of the vnodes.  If we are here waiting for the mountpoint
	 * lock while holding on to a vnode lock, well, we ain't
	 * just pining for the fjords anymore.  If we release the
	 * vnode lock, we will be in the situation "mount point
	 * is dying" and panic() will ensue in insmntque.  So as a
	 * temporary workaround, get a vnode without putting it on
	 * the mount point list, check if mount point is still alive
	 * and kicking and only then add the vnode to the list.
	 */
	error = getnewvnode(VT_PUFFS, NULL, puffs_vnodeop_p, &vp);
	if (error)
		return error;
	vp->v_vnlock = NULL;
	vp->v_type = type;

	/*
	 * Check what mount point isn't going away.  This will work
	 * until we decide to remove biglock or make the kernel
	 * preemptive.  But hopefully the real problem will be fixed
	 * by then.
	 *
	 * XXX: yes, should call vfs_busy(), but thar be rabbits with
	 * vicious streaks a mile wide ...
	 */
	if (mp->mnt_iflag & IMNT_UNMOUNT) {
		DPRINTF(("puffs_getvnode: mp %p unmount, unable to create "
		    "vnode for cookie %p\n", mp, cookie));
		ungetnewvnode(vp);
		return ENXIO;
	}

	/* So it's not dead yet.. good.. inform new vnode of its master */
	simple_lock(&mntvnode_slock);
	TAILQ_INSERT_TAIL(&mp->mnt_vnodelist, vp, v_mntvnodes);
	simple_unlock(&mntvnode_slock);
	vp->v_mount = mp;

	/*
	 * clerical tasks & footwork
	 */

	/* default size */
	uvm_vnp_setsize(vp, 0);

	/* dances based on vnode type. almost ufs_vinit(), but not quite */
	switch (type) {
	case VCHR:
	case VBLK:
		/*
		 * replace vnode operation vector with the specops vector.
		 * our user server has very little control over the node
		 * if it decides its a character or block special file
		 */
		vp->v_op = puffs_specop_p;

		/* do the standard checkalias-dance */
		if ((nvp = checkalias(vp, rdev, mp)) != NULL) {
			/*
			 * found: release & unallocate aliased
			 * old (well, actually, new) node
			 */
			vp->v_op = spec_vnodeop_p;
			vp->v_flag &= ~VLOCKSWORK;
			vrele(vp);
			vgone(vp); /* cya */

			/* init "new" vnode */
			vp = nvp;
			vp->v_vnlock = NULL;
			vp->v_mount = mp;
		}
		break;

	case VFIFO:
		vp->v_op = puffs_fifoop_p;
		break;

	case VREG:
		uvm_vnp_setsize(vp, vsize);
		break;

	case VDIR:
	case VLNK:
	case VSOCK:
		break;
	default:
#ifdef DIAGNOSTIC
		panic("puffs_getvnode: invalid vtype %d", type);
#endif
		break;
	}

	pnode = pool_get(&puffs_pnpool, PR_WAITOK);
	pnode->pn_cookie = cookie;
	pnode->pn_stat = 0;
	pnode->pn_refcount = 1;

	mutex_init(&pnode->pn_mtx, MUTEX_DEFAULT, IPL_NONE);
	SLIST_INIT(&pnode->pn_sel.sel_klist);
	pnode->pn_revents = 0;

	plist = puffs_cookie2hashlist(pmp, cookie);
	LIST_INSERT_HEAD(plist, pnode, pn_hashent);
	vp->v_data = pnode;
	vp->v_type = type;
	pnode->pn_vp = vp;

	genfs_node_init(vp, &puffs_genfsops);
	*vpp = vp;

	DPRINTF(("new vnode at %p, pnode %p, cookie %p\n", vp,
	    pnode, pnode->pn_cookie));

	return 0;
}

/* new node creating for creative vop ops (create, symlink, mkdir, mknod) */
int
puffs_newnode(struct mount *mp, struct vnode *dvp, struct vnode **vpp,
	void *cookie, struct componentname *cnp, enum vtype type, dev_t rdev)
{
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	struct vnode *vp;
	int error;

	/* userspace probably has this as a NULL op */
	if (cookie == NULL) {
		error = EOPNOTSUPP;
		return error;
	}

	/*
	 * Check for previous node with the same designation.
	 * Explicitly check the root node cookie, since it might be
	 * reclaimed from the kernel when this check is made.
	 *
	 * XXX: technically this error check should punish the fs,
	 * not the caller.
	 */
	mutex_enter(&pmp->pmp_lock);
	if (cookie == pmp->pmp_root_cookie
	    || puffs_cookie2pnode(pmp, cookie) != NULL) {
		mutex_exit(&pmp->pmp_lock);
		error = EEXIST;
		return error;
	}
	mutex_exit(&pmp->pmp_lock);

	error = puffs_getvnode(dvp->v_mount, cookie, type, 0, rdev, &vp);
	if (error)
		return error;

	vp->v_type = type;
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	*vpp = vp;

	if ((cnp->cn_flags & MAKEENTRY) && PUFFS_DOCACHE(pmp))
		cache_enter(dvp, vp, cnp);

	return 0;
}

/*
 * Release pnode structure which dealing with references to the
 * puffs_node instead of the vnode.  Can't use vref()/vrele() on
 * the vnode there, since that causes the lovely VOP_INACTIVE(),
 * which in turn causes the lovely deadlock when called by the one
 * who is supposed to handle it.
 */
void
puffs_releasenode(struct puffs_node *pn)
{

	mutex_enter(&pn->pn_mtx);
	if (--pn->pn_refcount == 0) {
		mutex_exit(&pn->pn_mtx);
		mutex_destroy(&pn->pn_mtx);
		pool_put(&puffs_pnpool, pn);
	} else {
		mutex_exit(&pn->pn_mtx);
	}
}

/*
 * Add reference to node.
 *  mutex held on entry and return
 */
void
puffs_referencenode(struct puffs_node *pn)
{

	KASSERT(mutex_owned(&pn->pn_mtx));
	pn->pn_refcount++;
}

void
puffs_putvnode(struct vnode *vp)
{
	struct puffs_mount *pmp;
	struct puffs_node *pnode;

	pmp = VPTOPUFFSMP(vp);
	pnode = VPTOPP(vp);

#ifdef DIAGNOSTIC
	if (vp->v_tag != VT_PUFFS)
		panic("puffs_putvnode: %p not a puffs vnode", vp);
#endif

	LIST_REMOVE(pnode, pn_hashent);
	genfs_node_destroy(vp);
	puffs_releasenode(pnode);
	vp->v_data = NULL;

	return;
}

static __inline struct puffs_node_hashlist *
puffs_cookie2hashlist(struct puffs_mount *pmp, void *cookie)
{
	uint32_t hash;

	hash = hash32_buf(&cookie, sizeof(void *), HASH32_BUF_INIT);
	return &pmp->pmp_pnodehash[hash % pmp->pmp_npnodehash];
}

/*
 * Translate cookie to puffs_node.  Caller must hold mountpoint
 * lock and it will be held upon return.
 */
static struct puffs_node *
puffs_cookie2pnode(struct puffs_mount *pmp, void *cookie)
{
	struct puffs_node_hashlist *plist;
	struct puffs_node *pnode;

	plist = puffs_cookie2hashlist(pmp, cookie);
	LIST_FOREACH(pnode, plist, pn_hashent) {
		if (pnode->pn_cookie == cookie)
			break;
	}

	return pnode;
}

/*
 * Make sure root vnode exists and reference it.  Does NOT lock.
 */
static int
puffs_makeroot(struct puffs_mount *pmp)
{
	struct vnode *vp;
	int rv;

	/*
	 * pmp_lock must be held if vref()'ing or vrele()'ing the
	 * root vnode.  the latter is controlled by puffs_inactive().
	 *
	 * pmp_root is set here and cleared in puffs_reclaim().
	 */
 retry:
	mutex_enter(&pmp->pmp_lock);
	vp = pmp->pmp_root;
	if (vp) {
		simple_lock(&vp->v_interlock);
		mutex_exit(&pmp->pmp_lock);
		if (vget(vp, LK_INTERLOCK) == 0)
			return 0;
	} else
		mutex_exit(&pmp->pmp_lock);

	/*
	 * So, didn't have the magic root vnode available.
	 * No matter, grab another an stuff it with the cookie.
	 */
	if ((rv = puffs_getvnode(pmp->pmp_mp, pmp->pmp_root_cookie,
	    pmp->pmp_root_vtype, pmp->pmp_root_vsize, pmp->pmp_root_rdev, &vp)))
		return rv;

	/*
	 * Someone magically managed to race us into puffs_getvnode?
	 * Put our previous new vnode back and retry.
	 */
	mutex_enter(&pmp->pmp_lock);
	if (pmp->pmp_root) {
		mutex_exit(&pmp->pmp_lock);
		puffs_putvnode(vp);
		goto retry;
	} 

	/* store cache */
	vp->v_flag = VROOT;
	pmp->pmp_root = vp;
	mutex_exit(&pmp->pmp_lock);

	return 0;
}

/*
 * Locate the in-kernel vnode based on the cookie received given
 * from userspace.  Returns a vnode, if found, NULL otherwise.
 * The parameter "lock" control whether to lock the possible or
 * not.  Locking always might cause us to lock against ourselves
 * in situations where we want the vnode but don't care for the
 * vnode lock, e.g. file server issued putpages.
 */
int
puffs_pnode2vnode(struct puffs_mount *pmp, void *cookie, int lock,
	struct vnode **vpp)
{
	struct puffs_node *pnode;
	struct vnode *vp;
	int vgetflags, rv;

	/*
	 * Handle root in a special manner, since we want to make sure
	 * pmp_root is properly set.
	 */
	if (cookie == pmp->pmp_root_cookie) {
		if ((rv = puffs_makeroot(pmp)))
			return rv;
		if (lock)
			vn_lock(pmp->pmp_root, LK_EXCLUSIVE | LK_RETRY);

		*vpp = pmp->pmp_root;
		return 0;
	}

	mutex_enter(&pmp->pmp_lock);
	pnode = puffs_cookie2pnode(pmp, cookie);

	if (pnode == NULL) {
		mutex_exit(&pmp->pmp_lock);
		return ENOENT;
	}

	vp = pnode->pn_vp;
	simple_lock(&vp->v_interlock);
	mutex_exit(&pmp->pmp_lock);

	vgetflags = LK_INTERLOCK;
	if (lock)
		vgetflags |= LK_EXCLUSIVE | LK_RETRY;
	if ((rv = vget(vp, vgetflags)))
		return rv;

	*vpp = vp;
	return 0;
}

void
puffs_makecn(struct puffs_kcn *pkcn, const struct componentname *cn)
{

	pkcn->pkcn_nameiop = cn->cn_nameiop;
	pkcn->pkcn_flags = cn->cn_flags;
	pkcn->pkcn_pid = cn->cn_lwp->l_proc->p_pid;
	puffs_credcvt(&pkcn->pkcn_cred, cn->cn_cred);

	(void)memcpy(&pkcn->pkcn_name, cn->cn_nameptr, cn->cn_namelen);
	pkcn->pkcn_name[cn->cn_namelen] = '\0';
	pkcn->pkcn_namelen = cn->cn_namelen;
}

/*
 * Convert given credentials to struct puffs_cred for userspace.
 */
void
puffs_credcvt(struct puffs_cred *pcr, const kauth_cred_t cred)
{

	memset(pcr, 0, sizeof(struct puffs_cred));

	if (cred == NOCRED || cred == FSCRED) {
		pcr->pcr_type = PUFFCRED_TYPE_INTERNAL;
		if (cred == NOCRED)
			pcr->pcr_internal = PUFFCRED_CRED_NOCRED;
		if (cred == FSCRED)
			pcr->pcr_internal = PUFFCRED_CRED_FSCRED;
 	} else {
		pcr->pcr_type = PUFFCRED_TYPE_UUC;
		kauth_cred_to_uucred(&pcr->pcr_uuc, cred);
	}
}

/*
 * Return pid.  In case the operation is coming from within the
 * kernel without any process context, borrow the swapper's pid.
 */
pid_t
puffs_lwp2pid(struct lwp *l)
{

	return l ? l->l_proc->p_pid : 0;
}


static void
puffs_gop_size(struct vnode *vp, off_t size, off_t *eobp,
	int flags)
{

	*eobp = size;
}

static void
puffs_gop_markupdate(struct vnode *vp, int flags)
{
	int uflags = 0;

	if (flags & GOP_UPDATE_ACCESSED)
		uflags |= PUFFS_UPDATEATIME;
	if (flags & GOP_UPDATE_MODIFIED)
		uflags |= PUFFS_UPDATEMTIME;

	puffs_updatenode(vp, uflags);
}

void
puffs_updatenode(struct vnode *vp, int flags)
{
	struct puffs_node *pn;
	struct timespec ts;

	if (flags == 0)
		return;

	pn = VPTOPP(vp);
	nanotime(&ts);

	if (flags & PUFFS_UPDATEATIME) {
		pn->pn_mc_atime = ts;
		pn->pn_stat |= PNODE_METACACHE_ATIME;
	}
	if (flags & PUFFS_UPDATECTIME) {
		pn->pn_mc_ctime = ts;
		pn->pn_stat |= PNODE_METACACHE_CTIME;
	}
	if (flags & PUFFS_UPDATEMTIME) {
		pn->pn_mc_mtime = ts;
		pn->pn_stat |= PNODE_METACACHE_MTIME;
	}
	if (flags & PUFFS_UPDATESIZE) {
		pn->pn_mc_size = vp->v_size;
		pn->pn_stat |= PNODE_METACACHE_SIZE;
	}
}

void
puffs_updatevpsize(struct vnode *vp)
{
	struct vattr va;

	if (VOP_GETATTR(vp, &va, FSCRED, NULL))
		return;

	if (va.va_size != VNOVAL)
		vp->v_size = va.va_size;
}

void
puffs_parkdone_asyncbioread(struct puffs_req *preq, void *arg)
{
	struct puffs_vnreq_read *read_argp = (void *)preq;
	struct buf *bp = arg;
	size_t moved;

	bp->b_error = preq->preq_rv;
	if (bp->b_error == 0) {
		moved = bp->b_bcount - read_argp->pvnr_resid;
		bp->b_resid = read_argp->pvnr_resid;

		memcpy(bp->b_data, read_argp->pvnr_data, moved);
	} else {
		bp->b_flags |= B_ERROR;
	}

	biodone(bp);
	free(preq, M_PUFFS);
}

void
puffs_parkdone_poll(struct puffs_req *preq, void *arg)
{
	struct puffs_vnreq_poll *poll_argp = (void *)preq;
	struct puffs_node *pn = arg;
	int revents;

	if (preq->preq_rv == 0)
		revents = poll_argp->pvnr_events;
	else
		revents = POLLERR;

	mutex_enter(&pn->pn_mtx);
	pn->pn_revents |= revents;
	mutex_exit(&pn->pn_mtx);

	selnotify(&pn->pn_sel, 0);
	free(preq, M_PUFFS);

	puffs_releasenode(pn);
}

void
puffs_mp_reference(struct puffs_mount *pmp)
{

	KASSERT(mutex_owned(&pmp->pmp_lock));
	pmp->pmp_refcount++;
}

void
puffs_mp_release(struct puffs_mount *pmp)
{

	KASSERT(mutex_owned(&pmp->pmp_lock));
	if (--pmp->pmp_refcount == 0)
		cv_broadcast(&pmp->pmp_refcount_cv);
}
