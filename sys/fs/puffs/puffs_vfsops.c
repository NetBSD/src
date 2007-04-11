/*	$NetBSD: puffs_vfsops.c,v 1.33 2007/04/11 21:03:05 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: puffs_vfsops.c,v 1.33 2007/04/11 21:03:05 pooka Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/extattr.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>

#include <lib/libkern/libkern.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

VFS_PROTOS(puffs);

MALLOC_JUSTDEFINE(M_PUFFS, "puffs", "Pass-to-Userspace Framework File System");

#ifndef PUFFS_PNODEBUCKETS
#define PUFFS_PNODEBUCKETS 256
#endif
#ifndef PUFFS_MAXPNODEBUCKETS
#define PUFFS_MAXPNODEBUCKETS 65536
#endif
int puffs_pnodebuckets = PUFFS_PNODEBUCKETS;

int
puffs_mount(struct mount *mp, const char *path, void *data,
	    struct nameidata *ndp, struct lwp *l)
{
	struct puffs_mount *pmp = NULL;
	struct puffs_args *args;
	char namebuf[PUFFSNAMESIZE+sizeof(PUFFS_NAMEPREFIX)+1]; /* spooky */
	int error = 0, i;

	if (mp->mnt_flag & MNT_GETARGS) {
		pmp = MPTOPUFFSMP(mp);
		return copyout(&pmp->pmp_args, data, sizeof(struct puffs_args));
	}

	/* update is not supported currently */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	/*
	 * We need the file system name
	 */
	if (!data)
		return EINVAL;

	MALLOC(args, struct puffs_args *, sizeof(struct puffs_args),
	    M_PUFFS, M_WAITOK);

	error = copyin(data, args, sizeof(struct puffs_args));
	if (error)
		goto out;

	/* devel phase */
	if (args->pa_vers != (PUFFSVERSION | PUFFSDEVELVERS)) {
		printf("puffs_mount: development version mismatch\n");
		error = EINVAL;
		goto out;
	}

	/* nuke spy bits */
	args->pa_flags &= PUFFS_KFLAG_MASK;

	/* build real name */
	(void)strlcpy(namebuf, PUFFS_NAMEPREFIX, sizeof(namebuf));
	(void)strlcat(namebuf, args->pa_name, sizeof(namebuf));

	/* inform user server if it got the max request size it wanted */
	if (args->pa_maxreqlen == 0 || args->pa_maxreqlen > PUFFS_REQ_MAXSIZE)
		args->pa_maxreqlen = PUFFS_REQ_MAXSIZE;
	else if (args->pa_maxreqlen < PUFFS_REQSTRUCT_MAX)
		args->pa_maxreqlen = PUFFS_REQSTRUCT_MAX;
	(void)strlcpy(args->pa_name, namebuf, sizeof(args->pa_name));

	error = copyout(args, data, sizeof(struct puffs_args)); 
	if (error)
		goto out;

	error = set_statvfs_info(path, UIO_USERSPACE, namebuf,
	    UIO_SYSSPACE, mp, l);
	if (error)
		goto out;
	mp->mnt_stat.f_iosize = DEV_BSIZE;

	MALLOC(pmp, struct puffs_mount *, sizeof(struct puffs_mount),
	    M_PUFFS, M_WAITOK | M_ZERO);

	mp->mnt_fs_bshift = DEV_BSHIFT;
	mp->mnt_dev_bshift = DEV_BSHIFT;
	mp->mnt_flag &= ~MNT_LOCAL; /* we don't really know, so ... */
	mp->mnt_data = pmp;
	mp->mnt_iflag |= IMNT_HAS_TRANS;

	pmp->pmp_status = PUFFSTAT_MOUNTING;
	pmp->pmp_nextreq = 0;
	pmp->pmp_mp = mp;
	pmp->pmp_req_maxsize = args->pa_maxreqlen;
	pmp->pmp_args = *args;

	/* puffs_node hash buckets */
	pmp->pmp_npnodehash = puffs_pnodebuckets;
	if (pmp->pmp_npnodehash < 1)
		pmp->pmp_npnodehash = 1;
	if (pmp->pmp_npnodehash > PUFFS_MAXPNODEBUCKETS)
		pmp->pmp_npnodehash = PUFFS_MAXPNODEBUCKETS;
	pmp->pmp_pnodehash = malloc
	    (sizeof(struct puffs_pnode_hashlist *) * pmp->pmp_npnodehash,
	    M_PUFFS, M_WAITOK);
	for (i = 0; i < pmp->pmp_npnodehash; i++)
		LIST_INIT(&pmp->pmp_pnodehash[i]);

	/*
	 * Inform the fileops processing code that we have a mountpoint.
	 * If it doesn't know about anyone with our pid/fd having the
	 * device open, punt
	 */
	if (puffs_setpmp(l->l_proc->p_pid, args->pa_fd, pmp)) {
		error = ENOENT;
		goto out;
	}

	mutex_init(&pmp->pmp_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&pmp->pmp_req_waiter_cv, "puffsget");
	cv_init(&pmp->pmp_req_waitersink_cv, "puffsink");
	cv_init(&pmp->pmp_unmounting_cv, "puffsum");
	cv_init(&pmp->pmp_suspend_cv, "pufsusum");
	TAILQ_INIT(&pmp->pmp_req_touser);
	TAILQ_INIT(&pmp->pmp_req_replywait);
	TAILQ_INIT(&pmp->pmp_req_sizepark);

	DPRINTF(("puffs_mount: mount point at %p, puffs specific at %p\n",
	    mp, MPTOPUFFSMP(mp)));

	vfs_getnewfsid(mp);

 out:
	if (error && pmp && pmp->pmp_pnodehash)
		free(pmp->pmp_pnodehash, M_PUFFS);
	if (error && pmp)
		FREE(pmp, M_PUFFS);
	FREE(args, M_PUFFS);
	return error;
}

/*
 * This is called from the first "Hello, I'm alive" ioctl
 * from userspace.
 */
int
puffs_start2(struct puffs_mount *pmp, struct puffs_startreq *sreq)
{
	struct puffs_node *pn;
	struct mount *mp;

	mp = PMPTOMP(pmp);

	mutex_enter(&pmp->pmp_lock);

	/*
	 * if someone has issued a VFS_ROOT() already, fill in the
	 * vnode cookie.
	 */
	pn = NULL;
	if (pmp->pmp_root) {
		pn = VPTOPP(pmp->pmp_root);
		pn->pn_cookie = sreq->psr_cookie;
	}

	/* We're good to fly */
	pmp->pmp_rootcookie = sreq->psr_cookie;
	pmp->pmp_status = PUFFSTAT_RUNNING;
	mutex_exit(&pmp->pmp_lock);

	/* do the VFS_STATVFS() we missed out on in sys_mount() */
	copy_statvfs_info(&sreq->psr_sb, mp);
	(void)memcpy(&mp->mnt_stat, &sreq->psr_sb, sizeof(mp->mnt_stat));
	mp->mnt_stat.f_iosize = DEV_BSIZE;

	DPRINTF(("puffs_start2: root vp %p, cur root pnode %p, cookie %p\n",
	    pmp->pmp_root, pn, sreq->psr_cookie));

	return 0;
}

int
puffs_start(struct mount *mp, int flags, struct lwp *l)
{

	/*
	 * This cannot travel to userspace, as this is called from
	 * the kernel context of the process doing mount(2).  But
	 * it's probably a safe bet that the process doing mount(2)
	 * realizes it needs to start the filesystem also...
	 */
	return 0;
}

int
puffs_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	struct puffs_mount *pmp;
	int error, force;

	PUFFS_VFSREQ(unmount);

	error = 0;
	force = mntflags & MNT_FORCE;
	pmp = MPTOPUFFSMP(mp);

	DPRINTF(("puffs_unmount: detach filesystem from vfs, current "
	    "status 0x%x\n", pmp->pmp_status));

	/*
	 * flush all the vnodes.  VOP_RECLAIM() takes care that the
	 * root vnode does not get flushed until unmount.  The
	 * userspace root node cookie is stored in the mount
	 * structure, so we can always re-instantiate a root vnode,
	 * should userspace unmount decide it doesn't want to
	 * cooperate.
	 */
	error = vflush(mp, NULLVP, force ? FORCECLOSE : 0);
	if (error)
		goto out;

	/*
	 * If we are not DYING, we should ask userspace's opinion
	 * about the situation
	 */
	mutex_enter(&pmp->pmp_lock);
	if (pmp->pmp_status != PUFFSTAT_DYING) {
		pmp->pmp_unmounting = 1;
		mutex_exit(&pmp->pmp_lock);

		unmount_arg.pvfsr_flags = mntflags;
		unmount_arg.pvfsr_pid = puffs_lwp2pid(l);

		error = puffs_vfstouser(pmp, PUFFS_VFS_UNMOUNT,
		     &unmount_arg, sizeof(unmount_arg));
		DPRINTF(("puffs_unmount: error %d force %d\n", error, force));

		mutex_enter(&pmp->pmp_lock);
		pmp->pmp_unmounting = 0;
		cv_broadcast(&pmp->pmp_unmounting_cv);
	}

	/*
	 * if userspace cooperated or we really need to die,
	 * screw what userland thinks and just die.
	 */
	if (error == 0 || force) {
		/* tell waiters & other resources to go unwait themselves */
		puffs_userdead(pmp);
		puffs_nukebypmp(pmp);

		/*
		 * Sink waiters.  This is still not perfect, since the
		 * draining is done after userret, not when they really
		 * exit the file system.  It will probably work as almost
		 * no call will block and therefore cause a context switch
		 * and therefore will protected by the biglock after
		 * exiting userspace.  But ... it's an imperfect world.
		 */
		while (pmp->pmp_req_waiters != 0)
			cv_wait(&pmp->pmp_req_waitersink_cv, &pmp->pmp_lock);
		mutex_exit(&pmp->pmp_lock);

		/* free resources now that we hopefully have no waiters left */
		cv_destroy(&pmp->pmp_req_waiter_cv);
		cv_destroy(&pmp->pmp_req_waitersink_cv);
		cv_destroy(&pmp->pmp_unmounting_cv);
		cv_destroy(&pmp->pmp_suspend_cv);
		mutex_destroy(&pmp->pmp_lock);

		free(pmp->pmp_pnodehash, M_PUFFS);
		FREE(pmp, M_PUFFS);
		error = 0;
	} else {
		mutex_exit(&pmp->pmp_lock);
	}

 out:
	DPRINTF(("puffs_unmount: return %d\n", error));
	return error;
}

/*
 * This doesn't need to travel to userspace
 */
int
puffs_root(struct mount *mp, struct vnode **vpp)
{
	struct puffs_mount *pmp;
	struct puffs_node *pn;
	struct vnode *vp;

	pmp = MPTOPUFFSMP(mp);

	/*
	 * pmp_lock must be held if vref()'ing or vrele()'ing the
	 * root vnode.  the latter is controlled by puffs_inactive().
	 */
	mutex_enter(&pmp->pmp_lock);
	vp = pmp->pmp_root;
	if (vp) {
		simple_lock(&vp->v_interlock);
		mutex_exit(&pmp->pmp_lock);
		pn = VPTOPP(vp);
		if (vget(vp, LK_EXCLUSIVE | LK_RETRY | LK_INTERLOCK))
			goto grabnew;
		*vpp = vp;
		return 0;
	} else
		mutex_exit(&pmp->pmp_lock);

	/* XXX: this is wrong, so FIXME */
 grabnew:

	/*
	 * So, didn't have the magic root vnode available.
	 * No matter, grab another an stuff it with the cookie.
	 */
	if (puffs_getvnode(mp, pmp->pmp_rootcookie, VDIR, 0, 0, &vp))
		panic("sloppy programming");

	mutex_enter(&pmp->pmp_lock);
	/*
	 * check if by mysterious force someone else created a root
	 * vnode while we were executing.
	 */
	if (pmp->pmp_root) {
		vref(pmp->pmp_root);
		mutex_exit(&pmp->pmp_lock);
		puffs_putvnode(vp);
		vn_lock(pmp->pmp_root, LK_EXCLUSIVE | LK_RETRY);
		*vpp = pmp->pmp_root;
		return 0;
	} 

	/* store cache */
	vp->v_flag = VROOT;
	pmp->pmp_root = vp;
	mutex_exit(&pmp->pmp_lock);

	vn_lock(pmp->pmp_root, LK_EXCLUSIVE | LK_RETRY);

	*vpp = vp;
	return 0;
}

int
puffs_statvfs(struct mount *mp, struct statvfs *sbp, struct lwp *l)
{
	struct puffs_vfsreq_statvfs *statvfs_arg; /* too big for stack */
	struct puffs_mount *pmp;
	int error = 0;

	pmp = MPTOPUFFSMP(mp);

	/*
	 * If we are mounting, it means that the userspace counterpart
	 * is calling mount(2), but mount(2) also calls statvfs.  So
	 * requesting statvfs from userspace would mean a deadlock.
	 * Compensate.
	 */
	if (pmp->pmp_status == PUFFSTAT_MOUNTING)
		return EINPROGRESS;

	/* too big for stack */
	MALLOC(statvfs_arg, struct puffs_vfsreq_statvfs *,
	    sizeof(struct puffs_vfsreq_statvfs), M_PUFFS, M_WAITOK | M_ZERO);
	statvfs_arg->pvfsr_pid = puffs_lwp2pid(l);

	error = puffs_vfstouser(pmp, PUFFS_VFS_STATVFS,
	    statvfs_arg, sizeof(*statvfs_arg));
	statvfs_arg->pvfsr_sb.f_iosize = DEV_BSIZE;

	/*
	 * Try to produce a sensible result even in the event
	 * of userspace error.
	 *
	 * XXX: cache the copy in non-error case
	 */
	if (!error) {
		copy_statvfs_info(&statvfs_arg->pvfsr_sb, mp);
		(void)memcpy(sbp, &statvfs_arg->pvfsr_sb,
		    sizeof(struct statvfs));
	} else {
		copy_statvfs_info(sbp, mp);
	}

	FREE(statvfs_arg, M_PUFFS);
	return error;
}

static int
pageflush(struct mount *mp, kauth_cred_t cred,
	int waitfor, int suspending, struct lwp *l)
{
	struct puffs_node *pn;
	struct vnode *vp, *nvp;
	int error, rv;

	KASSERT(((waitfor == MNT_WAIT) && suspending) == 0);
	KASSERT((suspending == 0)
	    || (fstrans_is_owner(mp)
	      && fstrans_getstate(mp) == FSTRANS_SUSPENDING));

	error = 0;

	/*
	 * Sync all cached data from regular vnodes (which are not
	 * currently locked, see below).  After this we call VFS_SYNC
	 * for the fs server, which should handle data and metadata for
	 * all the nodes it knows to exist.
	 */
	simple_lock(&mntvnode_slock);
 loop:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		/* check if we're on the right list */
		if (vp->v_mount != mp)
			goto loop;

		simple_lock(&vp->v_interlock);
		pn = VPTOPP(vp);
		nvp = TAILQ_NEXT(vp, v_mntvnodes);

		if (vp->v_type != VREG || UVM_OBJ_IS_CLEAN(&vp->v_uobj)) {
			simple_unlock(&vp->v_interlock);
			continue;
		}

		simple_unlock(&mntvnode_slock);

		/*
		 * Here we try to get a reference to the vnode and to
		 * lock it.  This is mostly cargo-culted, but I will
		 * offer an explanation to why I believe this might
		 * actually do the right thing.
		 *
		 * If the vnode is a goner, we quite obviously don't need
		 * to sync it.
		 *
		 * If the vnode was busy, we don't need to sync it because
		 * this is never called with MNT_WAIT except from
		 * dounmount(), when we are wait-flushing all the dirty
		 * vnodes through other routes in any case.  So there,
		 * sync() doesn't actually sync.  Happy now?
		 *
		 * NOTE: if we're suspending, vget() does NOT lock.
		 * See puffs_lock() for details.
		 */
		rv = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK);
		if (rv) {
			simple_lock(&mntvnode_slock);
			if (rv == ENOENT)
				goto loop;
			continue;
		}

		/*
		 * Thread information to puffs_strategy() through the
		 * pnode flags: we want to issue the putpages operations
		 * as FAF if we're suspending, since it's very probable
		 * that our execution context is that of the userspace
		 * daemon.  We can do this because:
		 *   + we send the "going to suspend" prior to this part
		 *   + if any of the writes fails in userspace, it's the
		 *     file system server's problem to decide if this was a
		 *     failed snapshot when it gets the "snapshot complete"
		 *     notification.
		 *   + if any of the writes fail in the kernel already, we
		 *     immediately fail *and* notify the user server of
		 *     failure.
		 *
		 * We also do FAFs if we're called from the syncer.  This
		 * is just general optimization for trickle sync: no need
		 * to really guarantee that the stuff ended on backing
		 * storage.
		 * TODO: Maybe also hint the user server of this twist?
		 */
		if (suspending || waitfor == MNT_LAZY) {
			simple_lock(&vp->v_interlock);
			pn->pn_stat |= PNODE_SUSPEND;
			simple_unlock(&vp->v_interlock);
		}
		rv = VOP_FSYNC(vp, cred, waitfor, 0, 0, l);
		if (suspending || waitfor == MNT_LAZY) {
			simple_lock(&vp->v_interlock);
			pn->pn_stat &= ~PNODE_SUSPEND;
			simple_unlock(&vp->v_interlock);
		}
		if (rv)
			error = rv;
		vput(vp);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);

	return error;
}

int
puffs_sync(struct mount *mp, int waitfor, struct kauth_cred *cred,
	struct lwp *l)
{
	int error, rv;

	PUFFS_VFSREQ(sync);

	error = pageflush(mp, cred, waitfor, 0, l);

	/* sync fs */
	sync_arg.pvfsr_waitfor = waitfor;
	puffs_credcvt(&sync_arg.pvfsr_cred, cred);
	sync_arg.pvfsr_pid = puffs_lwp2pid(l);

	rv = puffs_vfstouser(MPTOPUFFSMP(mp), PUFFS_VFS_SYNC,
	    &sync_arg, sizeof(sync_arg));
	if (rv)
		error = rv;

	return error;
}

int
puffs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	struct vnode *vp;
	int error;

	PUFFS_VFSREQ(fhtonode);

	if ((pmp->pmp_flags & PUFFS_KFLAG_CANEXPORT) == 0)
		return EOPNOTSUPP;

	if (fhp->fid_len < PUFFS_FHSIZE + 4)
		return EINVAL;

	fhtonode_arg.pvfsr_dsize = PUFFS_FHSIZE;
	memcpy(fhtonode_arg.pvfsr_data, fhp->fid_data, PUFFS_FHSIZE);

	error = puffs_vfstouser(pmp, PUFFS_VFS_FHTOVP,
	    &fhtonode_arg, sizeof(fhtonode_arg));
	if (error)
		return error;

	vp = puffs_pnode2vnode(pmp, fhtonode_arg.pvfsr_fhcookie, 1);
	DPRINTF(("puffs_fhtovp: got cookie %p, existing vnode %p\n",
	    fhtonode_arg.pvfsr_fhcookie, vp));
	if (!vp) {
		error = puffs_getvnode(mp, fhtonode_arg.pvfsr_fhcookie,
		    fhtonode_arg.pvfsr_vtype, fhtonode_arg.pvfsr_size,
		    fhtonode_arg.pvfsr_rdev, &vp);
		if (error)
			return error;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}

	*vpp = vp;
	return 0;
}

int
puffs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int error;

	PUFFS_VFSREQ(nodetofh);

	if ((pmp->pmp_flags & PUFFS_KFLAG_CANEXPORT) == 0)
		return EOPNOTSUPP;

	if (*fh_size < PUFFS_FHSIZE + 4) {
		*fh_size = PUFFS_FHSIZE + 4;
		return E2BIG;
	}
	*fh_size = PUFFS_FHSIZE + 4;

	nodetofh_arg.pvfsr_fhcookie = VPTOPNC(vp);
	nodetofh_arg.pvfsr_dsize = PUFFS_FHSIZE;

	error = puffs_vfstouser(pmp, PUFFS_VFS_VPTOFH,
	    &nodetofh_arg, sizeof(nodetofh_arg));
	if (error)
		return error;

	fhp->fid_len = PUFFS_FHSIZE + 4;
	memcpy(fhp->fid_data,
	    nodetofh_arg.pvfsr_data, PUFFS_FHSIZE);

	return 0;
}

void
puffs_init()
{

	malloc_type_attach(M_PUFFS);

	pool_init(&puffs_pnpool, sizeof(struct puffs_node), 0, 0, 0,
	    "puffpnpl", &pool_allocator_nointr, IPL_NONE);
	puffs_transport_init();
	puffs_msgif_init();
}

void
puffs_done()
{

	puffs_msgif_destroy();
	puffs_transport_destroy();
	pool_destroy(&puffs_pnpool);

	malloc_type_detach(M_PUFFS);
}

int
puffs_snapshot(struct mount *mp, struct vnode *vp, struct timespec *ts)
{

	return EOPNOTSUPP;
}

int
puffs_suspendctl(struct mount *mp, int cmd)
{
	struct puffs_mount *pmp;
	int error;

	pmp = MPTOPUFFSMP(mp);
	switch (cmd) {
	case SUSPEND_SUSPEND:
		DPRINTF(("puffs_suspendctl: suspending\n"));
		if ((error = fstrans_setstate(mp, FSTRANS_SUSPENDING)) != 0)
			break;
		puffs_suspendtouser(pmp, PUFFS_SUSPEND_START);

		error = pageflush(mp, FSCRED, 0, 1, curlwp);
		if (error == 0)
			error = fstrans_setstate(mp, FSTRANS_SUSPENDED);

		if (error != 0) {
			puffs_suspendtouser(pmp, PUFFS_SUSPEND_ERROR);
			(void) fstrans_setstate(mp, FSTRANS_NORMAL);
			break;
		}

		puffs_suspendtouser(pmp, PUFFS_SUSPEND_SUSPENDED);

		break;

	case SUSPEND_RESUME:
		DPRINTF(("puffs_suspendctl: resume\n"));
		error = 0;
		(void) fstrans_setstate(mp, FSTRANS_NORMAL);
		puffs_suspendtouser(pmp, PUFFS_SUSPEND_RESUME);
		break;

	default:
		error = EINVAL;
		break;
	}

	DPRINTF(("puffs_suspendctl: return %d\n", error));
	return error;
}

const struct vnodeopv_desc * const puffs_vnodeopv_descs[] = {
	&puffs_vnodeop_opv_desc,
	&puffs_specop_opv_desc,
	&puffs_fifoop_opv_desc,
	&puffs_msgop_opv_desc,
	NULL,
};

struct vfsops puffs_vfsops = {
	MOUNT_PUFFS,
	puffs_mount,		/* mount	*/
	puffs_start,		/* start	*/
	puffs_unmount,		/* unmount	*/
	puffs_root,		/* root		*/
	(void *)eopnotsupp,	/* quotactl	*/
	puffs_statvfs,		/* statvfs	*/
	puffs_sync,		/* sync		*/
	(void *)eopnotsupp,	/* vget		*/
	puffs_fhtovp,		/* fhtovp	*/
	puffs_vptofh,		/* vptofh	*/
	puffs_init,		/* init		*/
	NULL,			/* reinit	*/
	puffs_done,		/* done		*/
	NULL,			/* mountroot	*/
	puffs_snapshot,		/* snapshot	*/
	vfs_stdextattrctl,	/* extattrctl	*/
	puffs_suspendctl,	/* suspendctl	*/
	puffs_vnodeopv_descs,	/* vnodeops	*/
	0,			/* refcount	*/
	{ NULL, NULL }
};
VFS_ATTACH(puffs_vfsops);
