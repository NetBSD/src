/*	$NetBSD: kernfs_vfsops.c,v 1.70.6.1 2006/06/01 22:38:29 kardel Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kernfs_vfsops.c	8.10 (Berkeley) 5/14/95
 */

/*
 * Kernel params Filesystem
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kernfs_vfsops.c,v 1.70.6.1 2006/06/01 22:38:29 kardel Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>

MALLOC_DEFINE(M_KERNFSMNT, "kernfs mount", "kernfs mount structures");

dev_t rrootdev = NODEV;

void	kernfs_init(void);
void	kernfs_reinit(void);
void	kernfs_done(void);
void	kernfs_get_rrootdev(void);
int	kernfs_mount(struct mount *, const char *, void *,
	    struct nameidata *, struct lwp *);
int	kernfs_start(struct mount *, int, struct lwp *);
int	kernfs_unmount(struct mount *, int, struct lwp *);
int	kernfs_statvfs(struct mount *, struct statvfs *, struct lwp *);
int	kernfs_quotactl(struct mount *, int, uid_t, void *,
			     struct lwp *);
int	kernfs_sync(struct mount *, int, kauth_cred_t, struct lwp *);
int	kernfs_vget(struct mount *, ino_t, struct vnode **);

void
kernfs_init()
{
#ifdef _LKM
	malloc_type_attach(M_KERNFSMNT);
#endif
	kernfs_hashinit();
}

void
kernfs_reinit()
{
	kernfs_hashreinit();
}

void
kernfs_done()
{
#ifdef _LKM
	malloc_type_detach(M_KERNFSMNT);
#endif
	kernfs_hashdone();
}

void
kernfs_get_rrootdev()
{
	static int tried = 0;

	if (tried) {
		/* Already did it once. */
		return;
	}
	tried = 1;

	if (rootdev == NODEV)
		return;
	rrootdev = devsw_blk2chr(rootdev);
	if (rrootdev != NODEV)
		return;
	rrootdev = NODEV;
	printf("kernfs_get_rrootdev: no raw root device\n");
}

/*
 * Mount the Kernel params filesystem
 */
int
kernfs_mount(mp, path, data, ndp, l)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct lwp *l;
{
	int error = 0;
	struct kernfs_mount *fmp;

	if (UIO_MX & (UIO_MX - 1)) {
		log(LOG_ERR, "kernfs: invalid directory entry size");
		return (EINVAL);
	}

	if (mp->mnt_flag & MNT_GETARGS)
		return 0;
	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE)
		return (EOPNOTSUPP);

	MALLOC(fmp, struct kernfs_mount *, sizeof(struct kernfs_mount),
	    M_KERNFSMNT, M_WAITOK);
	memset(fmp, 0, sizeof(*fmp));
	TAILQ_INIT(&fmp->nodelist);

	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_data = fmp;
	vfs_getnewfsid(mp);

	if ((error = set_statvfs_info(path, UIO_USERSPACE, "kernfs",
	    UIO_SYSSPACE, mp, l)) != 0) {
		free(fmp, M_KERNFSMNT);
		return error;
	}

	kernfs_get_rrootdev();
	return 0;
}

int
kernfs_start(mp, flags, l)
	struct mount *mp;
	int flags;
	struct lwp *l;
{

	return (0);
}

int
kernfs_unmount(mp, mntflags, l)
	struct mount *mp;
	int mntflags;
	struct lwp *l;
{
	int error;
	int flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if ((error = vflush(mp, 0, flags)) != 0)
		return (error);

	/*
	 * Finally, throw away the kernfs_mount structure
	 */
	free(mp->mnt_data, M_KERNFSMNT);
	mp->mnt_data = NULL;
	return (0);
}

int
kernfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{

	/* setup "." */
	return (kernfs_allocvp(mp, vpp, KFSkern, &kern_targets[0], 0));
}

int
kernfs_quotactl(mp, cmd, uid, arg, l)
	struct mount *mp;
	int cmd;
	uid_t uid;
	void *arg;
	struct lwp *l;
{

	return (EOPNOTSUPP);
}

int
kernfs_statvfs(mp, sbp, l)
	struct mount *mp;
	struct statvfs *sbp;
	struct lwp *l;
{

	sbp->f_bsize = DEV_BSIZE;
	sbp->f_frsize = DEV_BSIZE;
	sbp->f_iosize = DEV_BSIZE;
	sbp->f_blocks = 2;		/* 1K to keep df happy */
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_bresvd = 0;
	sbp->f_files = 1024;	/* XXX lie */
	sbp->f_ffree = 128;	/* XXX lie */
	sbp->f_favail = 128;	/* XXX lie */
	sbp->f_fresvd = 0;
	copy_statvfs_info(sbp, mp);
	return (0);
}

/*ARGSUSED*/
int
kernfs_sync(mp, waitfor, uc, l)
	struct mount *mp;
	int waitfor;
	kauth_cred_t uc;
	struct lwp *l;
{

	return (0);
}

/*
 * Kernfs flat namespace lookup.
 * Currently unsupported.
 */
int
kernfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{

	return (EOPNOTSUPP);
}

SYSCTL_SETUP(sysctl_vfs_kernfs_setup, "sysctl vfs.kern subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kernfs",
		       SYSCTL_DESCR("/kern file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 11, CTL_EOL);
	/*
	 * XXX the "11" above could be dynamic, thereby eliminating one
	 * more instance of the "number to vfs" mapping problem, but
	 * "11" is the order as taken from sys/mount.h
	 */
}

extern const struct vnodeopv_desc kernfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const kernfs_vnodeopv_descs[] = {
	&kernfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops kernfs_vfsops = {
	MOUNT_KERNFS,
	kernfs_mount,
	kernfs_start,
	kernfs_unmount,
	kernfs_root,
	kernfs_quotactl,
	kernfs_statvfs,
	kernfs_sync,
	kernfs_vget,
	NULL,				/* vfs_fhtovp */
	NULL,				/* vfs_vptofh */
	kernfs_init,
	kernfs_reinit,
	kernfs_done,
	NULL,				/* vfs_mountroot */
	(int (*)(struct mount *, struct vnode *, struct timespec *)) eopnotsupp,
	vfs_stdextattrctl,
	kernfs_vnodeopv_descs,
};
VFS_ATTACH(kernfs_vfsops);
