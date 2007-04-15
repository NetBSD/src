/*	$NetBSD: union_vfsops.c,v 1.43.2.1 2007/04/15 16:03:48 yamt Exp $	*/

/*
 * Copyright (c) 1994 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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
 *	@(#)union_vfsops.c	8.20 (Berkeley) 5/20/95
 */

/*
 * Copyright (c) 1994 Jan-Simon Pendry.
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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
 *	@(#)union_vfsops.c	8.20 (Berkeley) 5/20/95
 */

/*
 * Union Layer
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: union_vfsops.c,v 1.43.2.1 2007/04/15 16:03:48 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/kauth.h>

#include <fs/union/union.h>

int union_mount(struct mount *, const char *, void *, struct nameidata *,
		     struct lwp *);
int union_start(struct mount *, int, struct lwp *);
int union_unmount(struct mount *, int, struct lwp *);
int union_root(struct mount *, struct vnode **);
int union_quotactl(struct mount *, int, uid_t, void *, struct lwp *);
int union_statvfs(struct mount *, struct statvfs *, struct lwp *);
int union_sync(struct mount *, int, kauth_cred_t, struct lwp *);
int union_vget(struct mount *, ino_t, struct vnode **);

/*
 * Mount union filesystem
 */
int
union_mount(mp, path, data, ndp, l)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct lwp *l;
{
	int error = 0;
	struct union_args args;
	struct vnode *lowerrootvp = NULLVP;
	struct vnode *upperrootvp = NULLVP;
	struct union_mount *um = 0;
	const char *cp;
	char *xp;
	int len;
	size_t size;

#ifdef UNION_DIAGNOSTIC
	printf("union_mount(mp = %p)\n", mp);
#endif

	if (mp->mnt_flag & MNT_GETARGS) {
		um = MOUNTTOUNIONMOUNT(mp);
		if (um == NULL)
			return EIO;
		args.target = NULL;
		args.mntflags = um->um_op;
		return copyout(&args, data, sizeof(args));
	}
	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		/*
		 * Need to provide.
		 * 1. a way to convert between rdonly and rdwr mounts.
		 * 2. support for nfs exports.
		 */
		error = EOPNOTSUPP;
		goto bad;
	}

	/*
	 * Get argument
	 */
	error = copyin(data, &args, sizeof(struct union_args));
	if (error)
		goto bad;

	lowerrootvp = mp->mnt_vnodecovered;
	VREF(lowerrootvp);

	/*
	 * Find upper node.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW,
	       UIO_USERSPACE, args.target, l);

	if ((error = namei(ndp)) != 0)
		goto bad;

	upperrootvp = ndp->ni_vp;

	if (upperrootvp->v_type != VDIR) {
		error = EINVAL;
		goto bad;
	}

	um = (struct union_mount *) malloc(sizeof(struct union_mount),
				M_UFSMNT, M_WAITOK);	/* XXX */

	/*
	 * Keep a held reference to the target vnodes.
	 * They are vrele'd in union_unmount.
	 *
	 * Depending on the _BELOW flag, the filesystems are
	 * viewed in a different order.  In effect, this is the
	 * same as providing a mount under option to the mount syscall.
	 */

	um->um_op = args.mntflags & UNMNT_OPMASK;
	switch (um->um_op) {
	case UNMNT_ABOVE:
		um->um_lowervp = lowerrootvp;
		um->um_uppervp = upperrootvp;
		break;

	case UNMNT_BELOW:
		um->um_lowervp = upperrootvp;
		um->um_uppervp = lowerrootvp;
		break;

	case UNMNT_REPLACE:
		vrele(lowerrootvp);
		lowerrootvp = NULLVP;
		um->um_uppervp = upperrootvp;
		um->um_lowervp = lowerrootvp;
		break;

	default:
		error = EINVAL;
		goto bad;
	}

	/*
	 * Unless the mount is readonly, ensure that the top layer
	 * supports whiteout operations
	 */
	if ((mp->mnt_flag & MNT_RDONLY) == 0) {
		error = VOP_WHITEOUT(um->um_uppervp, (struct componentname *) 0, LOOKUP);
		if (error)
			goto bad;
	}

	um->um_cred = l->l_cred;
	kauth_cred_hold(um->um_cred);
	um->um_cmode = UN_DIRMODE &~ l->l_proc->p_cwdi->cwdi_cmask;

	/*
	 * Depending on what you think the MNT_LOCAL flag might mean,
	 * you may want the && to be || on the conditional below.
	 * At the moment it has been defined that the filesystem is
	 * only local if it is all local, ie the MNT_LOCAL flag implies
	 * that the entire namespace is local.  If you think the MNT_LOCAL
	 * flag implies that some of the files might be stored locally
	 * then you will want to change the conditional.
	 */
	if (um->um_op == UNMNT_ABOVE) {
		if (((um->um_lowervp == NULLVP) ||
		     (um->um_lowervp->v_mount->mnt_flag & MNT_LOCAL)) &&
		    (um->um_uppervp->v_mount->mnt_flag & MNT_LOCAL))
			mp->mnt_flag |= MNT_LOCAL;
	}

	/*
	 * Copy in the upper layer's RDONLY flag.  This is for the benefit
	 * of lookup() which explicitly checks the flag, rather than asking
	 * the filesystem for it's own opinion.  This means, that an update
	 * mount of the underlying filesystem to go from rdonly to rdwr
	 * will leave the unioned view as read-only.
	 */
	mp->mnt_flag |= (um->um_uppervp->v_mount->mnt_flag & MNT_RDONLY);

	mp->mnt_data = um;
	vfs_getnewfsid(mp);

	error = set_statvfs_info( path, UIO_USERSPACE, NULL, UIO_USERSPACE,
	    mp, l);
	if (error)
		goto bad;

	switch (um->um_op) {
	case UNMNT_ABOVE:
		cp = "<above>:";
		break;
	case UNMNT_BELOW:
		cp = "<below>:";
		break;
	case UNMNT_REPLACE:
		cp = "";
		break;
	default:
		cp = "<invalid>:";
#ifdef DIAGNOSTIC
		panic("union_mount: bad um_op");
#endif
		break;
	}
	len = strlen(cp);
	memcpy(mp->mnt_stat.f_mntfromname, cp, len);

	xp = mp->mnt_stat.f_mntfromname + len;
	len = MNAMELEN - len;

	(void) copyinstr(args.target, xp, len - 1, &size);
	memset(xp + size, 0, len - size);

#ifdef UNION_DIAGNOSTIC
	printf("union_mount: from %s, on %s\n",
	    mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif

	/* Setup the readdir hook if it's not set already */
	if (!vn_union_readdir_hook)
		vn_union_readdir_hook = union_readdirhook;

	return (0);

bad:
	if (um)
		free(um, M_UFSMNT);
	if (upperrootvp)
		vrele(upperrootvp);
	if (lowerrootvp)
		vrele(lowerrootvp);
	return (error);
}

/*
 * VFS start.  Nothing needed here - the start routine
 * on the underlying filesystem(s) will have been called
 * when that filesystem was mounted.
 */
 /*ARGSUSED*/
int
union_start(struct mount *mp, int flags,
    struct lwp *l)
{

	return (0);
}

/*
 * Free reference to union layer
 */
int
union_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	int freeing;
	int error;

#ifdef UNION_DIAGNOSTIC
	printf("union_unmount(mp = %p)\n", mp);
#endif

	/*
	 * Keep flushing vnodes from the mount list.
	 * This is needed because of the un_pvp held
	 * reference to the parent vnode.
	 * If more vnodes have been freed on a given pass,
	 * the try again.  The loop will iterate at most
	 * (d) times, where (d) is the maximum tree depth
	 * in the filesystem.
	 */
	for (freeing = 0; (error = vflush(mp, NULL, 0)) != 0;) {
		struct vnode *vp;
		int n;

		/* count #vnodes held on mount list */
		n = 0;
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes)
			n++;

		/* if this is unchanged then stop */
		if (n == freeing)
			break;

		/* otherwise try once more time */
		freeing = n;
	}

	/*
	 * Ok, now that we've tried doing it gently, get out the hammer.
	 */

	if (mntflags & MNT_FORCE)
		error = vflush(mp, NULL, FORCECLOSE);

	if (error)
		return error;

	/*
	 * Discard references to upper and lower target vnodes.
	 */
	if (um->um_lowervp)
		vrele(um->um_lowervp);
	vrele(um->um_uppervp);
	kauth_cred_free(um->um_cred);
	/*
	 * Finally, throw away the union_mount structure
	 */
	free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return (0);
}

int
union_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	int error;

	/*
	 * Return locked reference to root.
	 */
	VREF(um->um_uppervp);
	vn_lock(um->um_uppervp, LK_EXCLUSIVE | LK_RETRY);
	if (um->um_lowervp)
		VREF(um->um_lowervp);
	error = union_allocvp(vpp, mp, NULL, NULL, NULL,
			      um->um_uppervp, um->um_lowervp, 1);

	if (error) {
		vput(um->um_uppervp);
		if (um->um_lowervp)
			vrele(um->um_lowervp);
	}

	return (error);
}

/*ARGSUSED*/
int
union_quotactl(struct mount *mp, int cmd, uid_t uid,
    void *arg, struct lwp *l)
{

	return (EOPNOTSUPP);
}

int
union_statvfs(mp, sbp, l)
	struct mount *mp;
	struct statvfs *sbp;
	struct lwp *l;
{
	int error;
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	struct statvfs *sbuf = malloc(sizeof(*sbuf), M_TEMP, M_WAITOK | M_ZERO);
	unsigned long lbsize;

#ifdef UNION_DIAGNOSTIC
	printf("union_statvfs(mp = %p, lvp = %p, uvp = %p)\n", mp,
	    um->um_lowervp, um->um_uppervp);
#endif

	if (um->um_lowervp) {
		error = VFS_STATVFS(um->um_lowervp->v_mount, sbuf, l);
		if (error)
			goto done;
	}

	/* now copy across the "interesting" information and fake the rest */
	lbsize = sbuf->f_bsize;
	sbp->f_blocks = sbuf->f_blocks - sbuf->f_bfree;
	sbp->f_files = sbuf->f_files - sbuf->f_ffree;

	error = VFS_STATVFS(um->um_uppervp->v_mount, sbuf, l);
	if (error)
		goto done;

	sbp->f_flag = sbuf->f_flag;
	sbp->f_bsize = sbuf->f_bsize;
	sbp->f_frsize = sbuf->f_frsize;
	sbp->f_iosize = sbuf->f_iosize;

	/*
	 * The "total" fields count total resources in all layers,
	 * the "free" fields count only those resources which are
	 * free in the upper layer (since only the upper layer
	 * is writable).
	 */

	if (sbuf->f_bsize != lbsize)
		sbp->f_blocks = sbp->f_blocks * lbsize / sbuf->f_bsize;
	sbp->f_blocks += sbuf->f_blocks;
	sbp->f_bfree = sbuf->f_bfree;
	sbp->f_bavail = sbuf->f_bavail;
	sbp->f_bresvd = sbuf->f_bresvd;
	sbp->f_files += sbuf->f_files;
	sbp->f_ffree = sbuf->f_ffree;
	sbp->f_favail = sbuf->f_favail;
	sbp->f_fresvd = sbuf->f_fresvd;

	copy_statvfs_info(sbp, mp);
done:
	free(sbuf, M_TEMP);
	return error;
}

/*ARGSUSED*/
int
union_sync(struct mount *mp, int waitfor,
    kauth_cred_t cred, struct lwp *l)
{

	/*
	 * XXX - Assumes no data cached at union layer.
	 */
	return (0);
}

/*ARGSUSED*/
int
union_vget(struct mount *mp, ino_t ino,
    struct vnode **vpp)
{

	return (EOPNOTSUPP);
}

SYSCTL_SETUP(sysctl_vfs_union_setup, "sysctl vfs.union subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "union",
		       SYSCTL_DESCR("Union file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 15, CTL_EOL);
	/*
	 * XXX the "15" above could be dynamic, thereby eliminating
	 * one more instance of the "number to vfs" mapping problem,
	 * but "15" is the order as taken from sys/mount.h
	 */
}

extern const struct vnodeopv_desc union_vnodeop_opv_desc;

const struct vnodeopv_desc * const union_vnodeopv_descs[] = {
	&union_vnodeop_opv_desc,
	NULL,
};

struct vfsops union_vfsops = {
	MOUNT_UNION,
	union_mount,
	union_start,
	union_unmount,
	union_root,
	union_quotactl,
	union_statvfs,
	union_sync,
	union_vget,
	(void *)eopnotsupp,		/* vfs_fhtovp */
	(void *)eopnotsupp,		/* vfs_vptofh */
	union_init,
	NULL,				/* vfs_reinit */
	union_done,
	NULL,				/* vfs_mountroot */
	(int (*)(struct mount *, struct vnode *, struct timespec *)) eopnotsupp,
	vfs_stdextattrctl,
	vfs_stdsuspendctl,
	union_vnodeopv_descs,
	0,				/* vfs_refcount */
	{ NULL, NULL },
};
VFS_ATTACH(union_vfsops);
