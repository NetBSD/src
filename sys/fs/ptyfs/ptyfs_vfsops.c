/*	$NetBSD: ptyfs_vfsops.c,v 1.1 2004/11/11 18:56:25 jdolecek Exp $	*/

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
 */

/*
 * Pseudo-tty Filesystem
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ptyfs_vfsops.c,v 1.1 2004/11/11 18:56:25 jdolecek Exp $");

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
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/pty.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/ptyfs/ptyfs.h>

MALLOC_DEFINE(M_PTYFSMNT, "ptyfs mount", "ptyfs mount structures");

void	ptyfs_init(void);
void	ptyfs_reinit(void);
void	ptyfs_done(void);
int	ptyfs_mount(struct mount *, const char *, void *, struct nameidata *,
    struct proc *);
int	ptyfs_start(struct mount *, int, struct proc *);
int	ptyfs_unmount(struct mount *, int, struct proc *);
int	ptyfs_statvfs(struct mount *, struct statvfs *, struct proc *);
int	ptyfs_quotactl(struct mount *, int, uid_t, void *, struct proc *);
int	ptyfs_sync(struct mount *, int, struct ucred *, struct proc *);
int	ptyfs_vget(struct mount *, ino_t, struct vnode **);
int	ptyfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int	ptyfs_checkexp(struct mount *, struct mbuf *, int *, struct ucred **);
int	ptyfs_vptofh(struct vnode *, struct fid *);

static int ptyfs__allocvp(struct ptm_pty *, struct proc *, struct vnode **,
    dev_t, char);
static int ptyfs__makename(struct ptm_pty *, char *, size_t, dev_t, char);

/*
 * ptm glue: When we mount, we make ptm point to us.
 */
struct ptm_pty *ptyfs_save_ptm;

struct ptm_pty ptm_ptyfspty = {
	ptyfs__allocvp,
	ptyfs__makename,
	NULL
};

static int
ptyfs__makename(struct ptm_pty *pt, char *buf, size_t bufsiz, dev_t dev,
    char ms)
{
	struct mount *mp = pt->arg;
	size_t len;

	switch (ms) {
	case 'p':
		/* We don't provide access to the master, should we? */
		len = snprintf(buf, bufsiz, "/dev/null");
		break;
	case 't':
		len = snprintf(buf, bufsiz, "%s/%d", mp->mnt_stat.f_mntonname,
		    minor(dev));
		break;
	default:
		return EINVAL;
	}

	return len >= bufsiz ? ENOSPC : 0;
}

static int
/*ARGSUSED*/
ptyfs__allocvp(struct ptm_pty *pt, struct proc *p, struct vnode **vpp,
    dev_t dev, char ms)
{
	struct mount *mp = pt->arg;
	ptyfstype type;

	switch (ms) {
	case 'p':
		type = PTYFSptc;
		break;
	case 't':
		type = PTYFSpts;
		break;
	default:
		return EINVAL;
	}

	return ptyfs_allocvp(mp, vpp, type, minor(dev), p);
}

void
ptyfs_init(void)
{
#ifdef _LKM
	malloc_type_attach(M_PTYFSMNT);
#endif
	ptyfs_hashinit();
}

void
ptyfs_reinit(void)
{
	ptyfs_hashreinit();
}

void
ptyfs_done(void)
{
#ifdef _LKM
	malloc_type_detach(M_PTYFSMNT);
#endif
	ptyfs_hashdone();
}

/*
 * Mount the Pseudo tty params filesystem
 */
int
ptyfs_mount(struct mount *mp, const char *path, void *data, 
    struct nameidata *ndp, struct proc *p)
{
	int error = 0;

	if (UIO_MX & (UIO_MX - 1)) {
		log(LOG_ERR, "ptyfs: invalid directory entry size");
		return EINVAL;
	}

	if (mp->mnt_flag & MNT_GETARGS)
		return 0;
	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	mp->mnt_data = NULL;
	mp->mnt_flag |= MNT_LOCAL;
	vfs_getnewfsid(mp);

	if ((error = set_statvfs_info(path, UIO_USERSPACE, "ptyfs",
	    UIO_SYSSPACE, mp, p)) != 0) {
		return error;
	}

	/* Point pty access to us */
	if (ptyfs_save_ptm != NULL)
		return EBUSY;

	ptm_ptyfspty.arg = mp;
	ptyfs_save_ptm = pty_sethandler(&ptm_ptyfspty);
	return 0;
}

/*ARGSUSED*/
int
ptyfs_start(struct mount *mp, int flags, struct proc *p)
{
	return 0;
}

/*ARGSUSED*/
int
ptyfs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	int error;
	int flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if ((error = vflush(mp, 0, flags)) != 0)
		return (error);

	/* Restore where pty access was pointing */
	ptm_ptyfspty.arg = NULL;
	(void)pty_sethandler(ptyfs_save_ptm);
	ptyfs_save_ptm = NULL;
	/*
	 * Finally, throw away the ptyfs_mount structure
	 */
	return 0;
}

int
ptyfs_root(struct mount *mp, struct vnode **vpp)
{
	/* setup "." */
	return ptyfs_allocvp(mp, vpp, PTYFSroot, 0, NULL);
}

/*ARGSUSED*/
int
ptyfs_quotactl(struct mount *mp, int cmd, uid_t uid, void *arg, struct proc *p)
{
	return EOPNOTSUPP;
}

/*ARGSUSED*/
int
ptyfs_statvfs(struct mount *mp, struct statvfs *sbp, struct proc *p)
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
	sbp->f_namemax = MAXNAMLEN;
	copy_statvfs_info(sbp, mp);
	return 0;
}

/*ARGSUSED*/
int
ptyfs_sync(struct mount *mp, int waitfor, struct ucred *uc, struct proc *p)
{
	return 0;
}

/*
 * Kernfs flat namespace lookup.
 * Currently unsupported.
 */
/*ARGSUSED*/
int
ptyfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	return EOPNOTSUPP;
}

/*ARGSUSED*/
int
ptyfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	return EOPNOTSUPP;
}

/*ARGSUSED*/
int
ptyfs_checkexp(struct mount *mp, struct mbuf *mb, int *wh, struct ucred **anon)
{
	return EOPNOTSUPP;
}

SYSCTL_SETUP(sysctl_vfs_ptyfs_setup, "sysctl vfs.ptyfs subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ptyfs",
		       SYSCTL_DESCR("Pty file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 23, CTL_EOL);
	/*
	 * XXX the "23" above could be dynamic, thereby eliminating
	 * one more instance of the "number to vfs" mapping problem,
	 * but "23" is the order as taken from sys/mount.h
	 */
}


/*ARGSUSED*/
int
ptyfs_vptofh(struct vnode *vp, struct fid *fhp)
{
	return EOPNOTSUPP;
}

extern const struct vnodeopv_desc ptyfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const ptyfs_vnodeopv_descs[] = {
	&ptyfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops ptyfs_vfsops = {
	MOUNT_PTYFS,
	ptyfs_mount,
	ptyfs_start,
	ptyfs_unmount,
	ptyfs_root,
	ptyfs_quotactl,
	ptyfs_statvfs,
	ptyfs_sync,
	ptyfs_vget,
	ptyfs_fhtovp,
	ptyfs_vptofh,
	ptyfs_init,
	ptyfs_reinit,
	ptyfs_done,
	NULL,
	NULL,				/* vfs_mountroot */
	ptyfs_checkexp,
	(int (*)(struct mount *, struct vnode *, struct timespec *))eopnotsupp,
	ptyfs_vnodeopv_descs,
};
