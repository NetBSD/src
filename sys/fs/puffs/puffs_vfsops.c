/*	$NetBSD: puffs_vfsops.c,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: puffs_vfsops.c,v 1.1 2006/10/22 22:43:23 pooka Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/extattr.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/kauth.h>

#include <lib/libkern/libkern.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

VFS_PROTOS(puffs);

MALLOC_DEFINE(M_PUFFS, "puffs", "pass-to-userspace file system structures");

int
puffs_mount(struct mount *mp, const char *path, void *data,
	    struct nameidata *ndp, struct lwp *l)
{
	struct puffs_mount *pmp;
	struct puffs_args args;
	char namebuf[PUFFSNAMESIZE+sizeof("puffs:")+1]; /* do I get a prize? */
	int error;

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

	error = copyin(data, &args, sizeof(struct puffs_args));
	if (error)
		return error;

	/* build real name */
	(void)strlcpy(namebuf, "puffs:", sizeof(namebuf));
	(void)strlcat(namebuf, args.pa_name, sizeof(namebuf));

	/* inform user server if it got the max request size it wanted */
	if (args.pa_maxreqlen == 0 || args.pa_maxreqlen > PUFFS_REQ_MAXSIZE)
		args.pa_maxreqlen = PUFFS_REQ_MAXSIZE;
	else if (args.pa_maxreqlen < PUFFS_REQSTRUCT_MAX)
		args.pa_maxreqlen = PUFFS_REQSTRUCT_MAX;
	(void)strlcpy(args.pa_name, namebuf, sizeof(args.pa_name));

	error = copyout(&args, data, sizeof(struct puffs_args)); 
	if (error)
		return error;

	error = set_statvfs_info(path, UIO_USERSPACE, namebuf,
	    UIO_SYSSPACE, mp, l);
	if (error)
		return error;

	MALLOC(pmp, struct puffs_mount *, sizeof(struct puffs_mount),
	    M_PUFFS, M_WAITOK | M_ZERO);

	mp->mnt_data = pmp;
	pmp->pmp_status = PUFFSTAT_MOUNTING;
	pmp->pmp_nextreq = 0;
	pmp->pmp_mp = mp;
	pmp->pmp_req_maxsize = args.pa_maxreqlen;
	pmp->pmp_args = args;

	/*
	 * Inform the fileops processing code that we have a mountpoint.
	 * If it doesn't know about anyone with our pid/fd having the
	 * device open, punt
	 */
	if (puffs_setpmp(l->l_proc->p_pid, args.pa_fd, pmp)) {
		FREE(pmp, M_PUFFS);
		return ENOENT;
	}

	simple_lock_init(&pmp->pmp_lock);
	TAILQ_INIT(&pmp->pmp_req_touser);
	TAILQ_INIT(&pmp->pmp_req_replywait);
	TAILQ_INIT(&pmp->pmp_req_sizepark);

	DPRINTF(("puffs_mount: mount point at %p, puffs specific at %p\n",
	    mp, MPTOPUFFSMP(mp)));

	vfs_getnewfsid(mp);

	return 0;
}

/*
 * This is called from the first "Hello, I'm alive" ioctl
 * from userspace.
 */
int
puffs_start2(struct puffs_mount *pmp, struct puffs_vfsreq_start *sreq)
{
	struct puffs_node *pn;
	struct mount *mp;

	mp = PMPTOMP(pmp);

	simple_lock(&pmp->pmp_lock);

	/*
	 * if someone has issued a VFS_ROOT() already, fill in the
	 * vnode cookie.
	 */
	if (pmp->pmp_root) {
		pn = VPTOPP(pmp->pmp_root);
		pn->pn_cookie = sreq->psr_cookie;
	}

	/* We're good to fly */
	pmp->pmp_rootcookie = sreq->psr_cookie;
	pmp->pmp_status = PUFFSTAT_RUNNING;
	sreq->psr_fsidx = mp->mnt_stat.f_fsidx; 

	simple_unlock(&pmp->pmp_lock);

	DPRINTF(("puffs_start2: root vp %p, root pnode %p, cookie %p\n",
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
	if (pmp->pmp_status != PUFFSTAT_DYING) {
		unmount_arg.pvfsr_flags = mntflags;
		unmount_arg.pvfsr_pid = puffs_lwp2pid(l);
		error = puffs_vfstouser(pmp, PUFFS_VFS_UNMOUNT,
		     &unmount_arg, sizeof(unmount_arg));
	}

	/*
	 * if userspace cooperated or we really need to die,
	 * screw what userland thinks and just die.
	 */
	DPRINTF(("puffs_umount: error %d force %d\n", error, force));
	if (error == 0 || force) {
		pmp->pmp_status = PUFFSTAT_DYING;
		puffs_nukebypmp(pmp);
		FREE(pmp, M_PUFFS);
		error = 0;
	}

 out:
	DPRINTF(("puffs_umount: return %d\n", error));
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
	 * root vnode.
	 */
	simple_lock(&pmp->pmp_lock);
	vp = pmp->pmp_root;
	if (vp) {
		pn = VPTOPP(vp);
		if (pn->pn_stat & PNODE_INACTIVE)  {
			if (vget(vp, LK_NOWAIT)) {
				pmp->pmp_root = NULL;
				goto grabnew;
			}
		} else
			vref(vp);
		simple_unlock(&pmp->pmp_lock);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		*vpp = vp;
		return 0;
	}
 grabnew:
	simple_unlock(&pmp->pmp_lock);

	/*
	 * So, didn't have the magic root vnode available.
	 * No matter, grab another an stuff it with the cookie.
	 */
	if (puffs_getvnode(mp, pmp->pmp_rootcookie, &vp))
		panic("sloppy programming");

	simple_lock(&pmp->pmp_lock);
	/*
	 * check if by mysterious force someone else created a root
	 * vnode while we were executing.
	 */
	if (pmp->pmp_root) {
		vref(pmp->pmp_root);
		simple_unlock(&pmp->pmp_lock);
		puffs_putvnode(vp);
		vn_lock(pmp->pmp_root, LK_EXCLUSIVE | LK_RETRY);
		*vpp = pmp->pmp_root;
		return 0;
	} 

	/* store cache */
	vp->v_type = VDIR;
	vp->v_flag = VROOT;
	pmp->pmp_root = vp;
	simple_unlock(&pmp->pmp_lock);

	vn_lock(pmp->pmp_root, LK_EXCLUSIVE | LK_RETRY);

	*vpp = vp;
	return 0;
}

int
puffs_quotactl(struct mount *mp, int cmd, uid_t uid, void *arg, struct lwp *l)
{

	return EOPNOTSUPP;
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

	return 0;
}

int
puffs_sync(struct mount *mp, int waitfor, struct kauth_cred *cred,
	struct lwp *l)
{
	int error;

	PUFFS_VFSREQ(sync);

	sync_arg.pvfsr_waitfor = waitfor;
	puffs_credcvt(&sync_arg.pvfsr_cred, cred);
	sync_arg.pvfsr_pid = puffs_lwp2pid(l);

	error = puffs_vfstouser(MPTOPUFFSMP(mp), PUFFS_VFS_SYNC,
	    &sync_arg, sizeof(sync_arg));

	return error;
}

int
puffs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{

	return EOPNOTSUPP;
}

#if 0
/*ARGSUSED*/
int
puffs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{

	return EOPNOTSUPP;
}

/*ARGSUSED*/
int
puffs_vptofh(struct vnode *vp, struct fid *fhp)
{

	return EOPNOTSUPP;
}
#endif

void
puffs_init()
{

	return;
}

void
puffs_done()
{

	return;
}

int
puffs_snapshot(struct mount *mp, struct vnode *vp, struct timespec *ts)
{

	return EOPNOTSUPP;
}

extern const struct vnodeopv_desc puffs_vnodeop_opv_desc;

const struct vnodeopv_desc * const puffs_vnodeopv_descs[] = {
	&puffs_vnodeop_opv_desc,
	NULL,
};

struct vfsops puffs_vfsops = {
	MOUNT_PUFFS,
	puffs_mount,		/* mount	*/
	puffs_start,		/* start	*/
	puffs_unmount,		/* unmount	*/
	puffs_root,		/* root		*/
	puffs_quotactl,		/* quotactl	*/
	puffs_statvfs,		/* statvfs	*/
	puffs_sync,		/* sync		*/
	puffs_vget,		/* vget		*/
	NULL,			/* fhtovp	*/
	NULL,			/* vptofh	*/
	puffs_init,		/* init		*/
	NULL,			/* reinit	*/
	puffs_done,		/* done		*/
	NULL,			/* mountroot	*/
	puffs_snapshot,		/* snapshot	*/
	vfs_stdextattrctl,	/* extattrctl	*/
	puffs_vnodeopv_descs,	/* vnodeops	*/
	0,			/* refcount	*/
	{ NULL, NULL }
};
VFS_ATTACH(puffs_vfsops);
