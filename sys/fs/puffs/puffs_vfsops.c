/*	$NetBSD: puffs_vfsops.c,v 1.53.4.2 2007/07/31 21:14:19 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: puffs_vfsops.c,v 1.53.4.2 2007/07/31 21:14:19 pooka Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/extattr.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>
#include <sys/proc.h>

#include <lib/libkern/libkern.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <nfs/nfsproto.h> /* for fh sizes */

VFS_PROTOS(puffs);

MALLOC_JUSTDEFINE(M_PUFFS, "puffs", "Pass-to-Userspace Framework File System");

#ifndef PUFFS_PNODEBUCKETS
#define PUFFS_PNODEBUCKETS 256
#endif
#ifndef PUFFS_MAXPNODEBUCKETS
#define PUFFS_MAXPNODEBUCKETS 8192
#endif
int puffs_pnodebuckets_default = PUFFS_PNODEBUCKETS;
int puffs_maxpnodebuckets = PUFFS_MAXPNODEBUCKETS;

int
puffs_mount(struct mount *mp, const char *path, void *data, size_t *data_len,
	    struct lwp *l)
{
	struct puffs_mount *pmp = NULL;
	struct puffs_kargs *args;
	char fstype[_VFS_NAMELEN];
	char *p;
	int error = 0, i;

	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		pmp = MPTOPUFFSMP(mp);
		*(struct puffs_kargs *)data = pmp->pmp_args;
		*data_len = sizeof *args;
		return 0;
	}

	/* update is not supported currently */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	/*
	 * We need the file system name
	 */
	if (!data)
		return EINVAL;

	MALLOC(args, struct puffs_kargs *, sizeof(struct puffs_kargs),
	    M_PUFFS, M_WAITOK);

	*args = *(struct puffs_kargs *)data;

	/* devel phase */
	if (args->pa_vers != (PUFFSVERSION | PUFFSDEVELVERS)) {
		printf("puffs_mount: development version mismatch\n");
		error = EINVAL;
		goto out;
	}

	/* nuke spy bits */
	args->pa_flags &= PUFFS_KFLAG_MASK;

	/* sanitize file handle length */
	if (PUFFS_TOFHSIZE(args->pa_fhsize) > FHANDLE_SIZE_MAX) {
		printf("puffs_mount: handle size %zu too large\n",
		    args->pa_fhsize);
		error = EINVAL;
		goto out;
	}
	/* sanity check file handle max sizes */
	if (args->pa_fhsize && args->pa_fhflags & PUFFS_FHFLAG_PROTOMASK) {
		size_t kfhsize = PUFFS_TOFHSIZE(args->pa_fhsize);

		if (args->pa_fhflags & PUFFS_FHFLAG_NFSV2) {
			if (NFSX_FHTOOBIG_P(kfhsize, 0)) {
				printf("puffs_mount: fhsize larger than "
				    "NFSv2 max %d\n",
				    PUFFS_FROMFHSIZE(NFSX_V2FH));
				error = EINVAL;
				goto out;
			}
		}

		if (args->pa_fhflags & PUFFS_FHFLAG_NFSV3) {
			if (NFSX_FHTOOBIG_P(kfhsize, 1)) {
				printf("puffs_mount: fhsize larger than "
				    "NFSv3 max %d\n",
				    PUFFS_FROMFHSIZE(NFSX_V3FHMAX));
				error = EINVAL;
				goto out;
			}
		}
	}

	/* don't allow non-printing characters (like my sweet umlauts.. snif) */
	args->pa_typename[sizeof(args->pa_typename)-1] = '\0';
	for (p = args->pa_typename; *p; p++)
		if (*p < ' ' || *p > '~')
			*p = '.';

	args->pa_mntfromname[sizeof(args->pa_mntfromname)-1] = '\0';
	for (p = args->pa_mntfromname; *p; p++)
		if (*p < ' ' || *p > '~')
			*p = '.';

	/* build real name */
	(void)strlcpy(fstype, PUFFS_TYPEPREFIX, sizeof(fstype));
	(void)strlcat(fstype, args->pa_typename, sizeof(fstype));

	/* inform user server if it got the max request size it wanted */
	if (args->pa_maxreqlen == 0 || args->pa_maxreqlen > PUFFS_REQ_MAXSIZE)
		args->pa_maxreqlen = PUFFS_REQ_MAXSIZE;
	else if (args->pa_maxreqlen < 2*PUFFS_REQSTRUCT_MAX)
		args->pa_maxreqlen = 2*PUFFS_REQSTRUCT_MAX;
	(void)strlcpy(args->pa_typename, fstype, sizeof(args->pa_typename));

	if (args->pa_nhashbuckets == 0)
		args->pa_nhashbuckets = puffs_pnodebuckets_default;
	if (args->pa_nhashbuckets < 1)
		args->pa_nhashbuckets = 1;
	if (args->pa_nhashbuckets > PUFFS_MAXPNODEBUCKETS) {
		args->pa_nhashbuckets = puffs_maxpnodebuckets;
		printf("puffs_mount: using %d hash buckets. "
		    "adjust puffs_maxpnodebuckets for more\n",
		    puffs_maxpnodebuckets);
	}

	error = set_statvfs_info(path, UIO_USERSPACE, args->pa_mntfromname,
	    UIO_SYSSPACE, fstype, mp, l);
	if (error)
		goto out;
	mp->mnt_stat.f_iosize = DEV_BSIZE;

	/*
	 * We can't handle the VFS_STATVFS() mount_domount() does
	 * after VFS_MOUNT() because we'd deadlock, so handle it
	 * here already.
	 */
	copy_statvfs_info(&args->pa_svfsb, mp);
	(void)memcpy(&mp->mnt_stat, &args->pa_svfsb, sizeof(mp->mnt_stat));

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

	pmp->pmp_npnodehash = args->pa_nhashbuckets;
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

	/* XXX: check parameters */
	pmp->pmp_root_cookie = args->pa_root_cookie;
	pmp->pmp_root_vtype = args->pa_root_vtype;
	pmp->pmp_root_vsize = args->pa_root_vsize;
	pmp->pmp_root_rdev = args->pa_root_rdev;

	mutex_init(&pmp->pmp_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&pmp->pmp_req_waiter_cv, "puffsget");
	cv_init(&pmp->pmp_refcount_cv, "puffsref");
	cv_init(&pmp->pmp_unmounting_cv, "puffsum");
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

int
puffs_start(struct mount *mp, int flags, struct lwp *l)
{
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);

	KASSERT(pmp->pmp_status == PUFFSTAT_MOUNTING);
	pmp->pmp_status = PUFFSTAT_RUNNING;

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
		puffs_cidcvt(&unmount_arg.pvfsr_cid, l);

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
		 * Wait until there are no more users for the mount resource.
		 * Notice that this is hooked against transport_close
		 * and return from touser.  In an ideal world, it would
		 * be hooked against final return from all operations.
		 * But currently it works well enough, since nobody
		 * does weird blocking voodoo after return from touser().
		 */
		while (pmp->pmp_refcount != 0)
			cv_wait(&pmp->pmp_refcount_cv, &pmp->pmp_lock);
		mutex_exit(&pmp->pmp_lock);

		/* free resources now that we hopefully have no waiters left */
		cv_destroy(&pmp->pmp_unmounting_cv);
		cv_destroy(&pmp->pmp_refcount_cv);
		cv_destroy(&pmp->pmp_req_waiter_cv);
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
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);

	return puffs_pnode2vnode(pmp, pmp->pmp_root_cookie, 1, vpp);
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
	puffs_cidcvt(&statvfs_arg->pvfsr_cid, l);

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
	puffs_cidcvt(&sync_arg.pvfsr_cid, l);

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
	struct puffs_vfsreq_fhtonode *fhtonode_argp;
	struct vnode *vp;
	size_t argsize;
	int error;

	if (pmp->pmp_args.pa_fhsize == 0)
		return EOPNOTSUPP;

	if (pmp->pmp_args.pa_fhflags & PUFFS_FHFLAG_DYNAMIC) {
		if (pmp->pmp_args.pa_fhsize < PUFFS_FROMFHSIZE(fhp->fid_len))
			return EINVAL;
	} else {
		if (pmp->pmp_args.pa_fhsize != PUFFS_FROMFHSIZE(fhp->fid_len))
			return EINVAL;
	}

	argsize = sizeof(struct puffs_vfsreq_fhtonode)
	    + PUFFS_FROMFHSIZE(fhp->fid_len);
	fhtonode_argp = malloc(argsize, M_PUFFS, M_ZERO | M_WAITOK);
	fhtonode_argp->pvfsr_dsize = PUFFS_FROMFHSIZE(fhp->fid_len);
	memcpy(fhtonode_argp->pvfsr_data, fhp->fid_data,
	    PUFFS_FROMFHSIZE(fhp->fid_len));

	error = puffs_vfstouser(pmp, PUFFS_VFS_FHTOVP, fhtonode_argp, argsize);
	if (error)
		goto out;

	error = puffs_pnode2vnode(pmp, fhtonode_argp->pvfsr_fhcookie, 1, &vp);
	DPRINTF(("puffs_fhtovp: got cookie %p, existing vnode %p\n",
	    fhtonode_argp->pvfsr_fhcookie, vp));
	if (error) {
		error = puffs_getvnode(mp, fhtonode_argp->pvfsr_fhcookie,
		    fhtonode_argp->pvfsr_vtype, fhtonode_argp->pvfsr_size,
		    fhtonode_argp->pvfsr_rdev, &vp);
		if (error)
			goto out;
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}

	*vpp = vp;
 out:
	free(fhtonode_argp, M_PUFFS);
	return error;
}

int
puffs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_vfsreq_nodetofh *nodetofh_argp;
	size_t argsize;
	int error;

	if (pmp->pmp_args.pa_fhsize == 0)
		return EOPNOTSUPP;

	/* if file handles are static length, we can return immediately */
	if (((pmp->pmp_args.pa_fhflags & PUFFS_FHFLAG_DYNAMIC) == 0)
	    && (PUFFS_FROMFHSIZE(*fh_size) < pmp->pmp_args.pa_fhsize)) {
		*fh_size = PUFFS_TOFHSIZE(pmp->pmp_args.pa_fhsize);
		return E2BIG;
	}

	argsize = sizeof(struct puffs_vfsreq_nodetofh)
	    + PUFFS_FROMFHSIZE(*fh_size);
	nodetofh_argp = malloc(argsize, M_PUFFS, M_ZERO | M_WAITOK);
	nodetofh_argp->pvfsr_fhcookie = VPTOPNC(vp);
	nodetofh_argp->pvfsr_dsize = PUFFS_FROMFHSIZE(*fh_size);

	error = puffs_vfstouser(pmp, PUFFS_VFS_VPTOFH, nodetofh_argp, argsize);
	if (error) {
		if (error == E2BIG)
			*fh_size = PUFFS_TOFHSIZE(nodetofh_argp->pvfsr_dsize);
		goto out;
	}

	if (PUFFS_TOFHSIZE(nodetofh_argp->pvfsr_dsize) > FHANDLE_SIZE_MAX) {
		/* XXX: wrong direction */
		error = EINVAL;
		goto out;
	}

	if (*fh_size < PUFFS_TOFHSIZE(nodetofh_argp->pvfsr_dsize)) {
		*fh_size = PUFFS_TOFHSIZE(nodetofh_argp->pvfsr_dsize);
		error = E2BIG;
		goto out;
	}
	if (pmp->pmp_args.pa_fhflags & PUFFS_FHFLAG_DYNAMIC) {
		*fh_size = PUFFS_TOFHSIZE(nodetofh_argp->pvfsr_dsize);
	} else {
		*fh_size = PUFFS_TOFHSIZE(pmp->pmp_args.pa_fhsize);
	}

	if (fhp) {
		fhp->fid_len = *fh_size;
		memcpy(fhp->fid_data,
		    nodetofh_argp->pvfsr_data, nodetofh_argp->pvfsr_dsize);
	}

 out:
	free(nodetofh_argp, M_PUFFS);
	return error;
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
	sizeof (struct puffs_kargs),
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
