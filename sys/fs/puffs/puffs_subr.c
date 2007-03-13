/*	$NetBSD: puffs_subr.c,v 1.22.2.1 2007/03/13 16:51:36 ad Exp $	*/

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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: puffs_subr.c,v 1.22.2.1 2007/03/13 16:51:36 ad Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/hash.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/namei.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/genfs/genfs_node.h>
#include <miscfs/specfs/specdev.h>

POOL_INIT(puffs_pnpool, sizeof(struct puffs_node), 0, 0, 0, "puffspnpl",
    &pool_allocator_nointr, IPL_NONE);

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
	simple_lock(&pmp->pmp_lock);
	if (cookie == pmp->pmp_rootcookie
	    || puffs_cookie2pnode(pmp, cookie) != NULL) {
		simple_unlock(&pmp->pmp_lock);
		error = EEXIST;
		return error;
	}
	simple_unlock(&pmp->pmp_lock);

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
	pool_put(&puffs_pnpool, vp->v_data);
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
 * Locate the in-kernel vnode based on the cookie received given
 * from userspace.  Returns a vnode, if found, NULL otherwise.
 * The parameter "lock" control whether to lock the possible or
 * not.  Locking always might cause us to lock against ourselves
 * in situations where we want the vnode but don't care for the
 * vnode lock, e.g. file server issued putpages.
 */
struct vnode *
puffs_pnode2vnode(struct puffs_mount *pmp, void *cookie, int lock)
{
	struct puffs_node *pnode;
	struct vnode *vp;
	int vgetflags;

	/*
	 * If we're trying to get the root vnode, return it through
	 * puffs_root() to get all the right things set.  Lock must
	 * be set, since VFS_ROOT() always locks the returned vnode.
	 */
	if (cookie == pmp->pmp_rootcookie) {
		if (!lock)
			return NULL;
		if (VFS_ROOT(pmp->pmp_mp, &vp))
			return NULL;

		return vp;
	}

	vgetflags = LK_INTERLOCK;
	if (lock)
		vgetflags |= LK_EXCLUSIVE | LK_RETRY;

	simple_lock(&pmp->pmp_lock);
	pnode = puffs_cookie2pnode(pmp, cookie);

	if (pnode == NULL) {
		simple_unlock(&pmp->pmp_lock);
		return NULL;
	}
	vp = pnode->pn_vp;

	simple_lock(&vp->v_interlock);
	simple_unlock(&pmp->pmp_lock);

	if (vget(vp, vgetflags))
		return NULL;

	return vp;
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
	struct timespec ts;
	struct puffs_vnreq_setattr *setattr_arg;

	if (flags == 0)
		return;

	setattr_arg = malloc(sizeof(struct puffs_vnreq_setattr), M_PUFFS,
	    M_NOWAIT | M_ZERO);
	if (setattr_arg == NULL)
		return; /* 2bad */

	nanotime(&ts);

	VATTR_NULL(&setattr_arg->pvnr_va);
	if (flags & PUFFS_UPDATEATIME)
		setattr_arg->pvnr_va.va_atime = ts;
	if (flags & PUFFS_UPDATECTIME)
		setattr_arg->pvnr_va.va_ctime = ts;
	if (flags & PUFFS_UPDATEMTIME)
		setattr_arg->pvnr_va.va_mtime = ts;
	if (flags & PUFFS_UPDATESIZE)
		setattr_arg->pvnr_va.va_size = vp->v_size;

	setattr_arg->pvnr_pid = 0;
	puffs_credcvt(&setattr_arg->pvnr_cred, NOCRED);

	/* setattr_arg ownership shifted to callee */
	puffs_vntouser_faf(MPTOPUFFSMP(vp->v_mount), PUFFS_VN_SETATTR,
	    setattr_arg, sizeof(struct puffs_vnreq_setattr), VPTOPNC(vp));
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

/*
 * We're dead, kaput, RIP, slightly more than merely pining for the
 * fjords, belly-up, fallen, lifeless, finished, expired, gone to meet
 * our maker, ceased to be, etcetc.  YASD.  It's a dead FS!
 *
 * Caller must hold puffs spinlock.
 */
void
puffs_userdead(struct puffs_mount *pmp)
{
	struct puffs_park *park;

	/*
	 * Mark filesystem status as dying so that operations don't
	 * attempt to march to userspace any longer.
	 */
	pmp->pmp_status = PUFFSTAT_DYING;

	/* and wakeup processes waiting for a reply from userspace */
	TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
		if (park->park_preq)
			park->park_preq->preq_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_replywait, park, park_entries);
		wakeup(park);
	}

	/* wakeup waiters for completion of vfs/vnode requests */
	TAILQ_FOREACH(park, &pmp->pmp_req_touser, park_entries) {
		if (park->park_preq)
			park->park_preq->preq_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);
		wakeup(park);
	}
}

/*
 * Converts a non-FAF op to a FAF.  This simply involves making copies
 * of the park and request structures and tagging the request as a FAF.
 * It is safe to block here, since the original op is not a FAF.
 */
struct puffs_park *
puffs_reqtofaf(struct puffs_park *ppark)
{
	struct puffs_park *newpark;
	struct puffs_req *newpreq;

	KASSERT((ppark->park_preq->preq_opclass & PUFFSOPFLAG_FAF) == 0);

	MALLOC(newpark, struct puffs_park *, sizeof(struct puffs_park),
	    M_PUFFS, M_ZERO | M_WAITOK);
	MALLOC(newpreq, struct puffs_req *, sizeof(struct puffs_req),
	    M_PUFFS, M_ZERO | M_WAITOK);

	memcpy(newpark, ppark, sizeof(struct puffs_park));
	memcpy(newpreq, ppark->park_preq, sizeof(struct puffs_req));

	newpark->park_preq = newpreq;
	newpark->park_preq->preq_opclass |= PUFFSOPFLAG_FAF;

	return newpark;
}
