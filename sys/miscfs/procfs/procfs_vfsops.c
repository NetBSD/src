/*	$NetBSD: procfs_vfsops.c,v 1.39.4.1 2001/10/01 12:47:22 fvdl Exp $	*/

/*
 * Copyright (c) 1993 Jan-Simon Pendry
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
 *	@(#)procfs_vfsops.c	8.7 (Berkeley) 5/10/95
 */

/*
 * procfs VFS interface
 */

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <miscfs/procfs/procfs.h>

#include <uvm/uvm_extern.h>			/* for PAGE_SIZE */

void	procfs_init __P((void));
void	procfs_reinit __P((void));
void	procfs_done __P((void));
int	procfs_mount __P((struct mount *, const char *, void *,
			  struct nameidata *, struct proc *));
int	procfs_start __P((struct mount *, int, struct proc *));
int	procfs_unmount __P((struct mount *, int, struct proc *));
int	procfs_quotactl __P((struct mount *, int, uid_t, caddr_t,
			     struct proc *));
int	procfs_statfs __P((struct mount *, struct statfs *, struct proc *));
int	procfs_sync __P((struct mount *, int, struct ucred *, struct proc *));
int	procfs_vget __P((struct mount *, ino_t, struct vnode **));
int	procfs_fhtovp __P((struct mount *, struct fid *, struct vnode **));
int	procfs_checkexp __P((struct mount *, struct mbuf *, int *,
			   struct ucred **));
int	procfs_vptofh __P((struct vnode *, struct fid *));
int	procfs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
			   struct proc *));
/*
 * VFS Operations.
 *
 * mount system call
 */
/* ARGSUSED */
int
procfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	size_t size;
	struct procfsmount *pmnt;
	struct procfs_args args;
	int error;

	if (UIO_MX & (UIO_MX-1)) {
		log(LOG_ERR, "procfs: invalid directory entry size");
		return (EINVAL);
	}

	if (mp->mnt_flag & MNT_UPDATE)
		return (EOPNOTSUPP);

	if (data != NULL) {
		error = copyin(data, &args, sizeof args);
		if (error != 0)
			return error;

		if (args.version != PROCFS_ARGSVERSION)
			return EINVAL;
	} else
		args.flags = 0;

	mp->mnt_flag |= MNT_LOCAL;
	pmnt = (struct procfsmount *) malloc(sizeof(struct procfsmount),
	    M_UFSMNT, M_WAITOK);   /* XXX need new malloc type */

	mp->mnt_data = (qaddr_t)pmnt;
	vfs_getnewfsid(mp);

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN, &size);
	memset(mp->mnt_stat.f_mntonname + size, 0, MNAMELEN - size);
	memset(mp->mnt_stat.f_mntfromname, 0, MNAMELEN);
	memcpy(mp->mnt_stat.f_mntfromname, "procfs", sizeof("procfs"));

	pmnt->pmnt_exechook = exechook_establish(procfs_revoke_vnodes, mp);
	pmnt->pmnt_flags = args.flags;

	return (0);
}

/*
 * unmount system call
 */
int
procfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	int error;
	int flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if ((error = vflush(mp, 0, flags)) != 0)
		return (error);

	exechook_disestablish(VFSTOPROC(mp)->pmnt_exechook);

	free(mp->mnt_data, M_UFSMNT);
	mp->mnt_data = 0;

	return (0);
}

int
procfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{

	return (procfs_allocvp(mp, vpp, 0, Proot));
}

/* ARGSUSED */
int
procfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{

	return (0);
}

/*
 * Get file system statistics.
 */
int
procfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{

	sbp->f_bsize = PAGE_SIZE;
	sbp->f_iosize = PAGE_SIZE;
	sbp->f_blocks = 1;	/* avoid divide by zero in some df's */
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_files = maxproc;			/* approx */
	sbp->f_ffree = maxproc - nprocs;	/* approx */
#ifdef COMPAT_09
	sbp->f_type = 10;
#else
	sbp->f_type = 0;
#endif
	if (sbp != &mp->mnt_stat) {
		memcpy(&sbp->f_fsid, &mp->mnt_stat.f_fsid, sizeof(sbp->f_fsid));
		memcpy(sbp->f_mntonname, mp->mnt_stat.f_mntonname, MNAMELEN);
		memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname, MNAMELEN);
	}
	strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);
	return (0);
}

/*ARGSUSED*/
int
procfs_quotactl(mp, cmds, uid, arg, p)
	struct mount *mp;
	int cmds;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
procfs_sync(mp, waitfor, uc, p)
	struct mount *mp;
	int waitfor;
	struct ucred *uc;
	struct proc *p;
{

	return (0);
}

/*ARGSUSED*/
int
procfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
procfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{

	return (EINVAL);
}

/*ARGSUSED*/
int
procfs_checkexp(mp, mb, what, anon)
	struct mount *mp;
	struct mbuf *mb;
	int *what;
	struct ucred **anon;
{

	return (EINVAL);
}

/*ARGSUSED*/
int
procfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{

	return (EINVAL);
}

void
procfs_init()
{
	procfs_hashinit();
}

void
procfs_reinit()
{
	procfs_hashreinit();
}

void
procfs_done()
{
	procfs_hashdone();
}

int
procfs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	return (EOPNOTSUPP);
}

extern const struct vnodeopv_desc procfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const procfs_vnodeopv_descs[] = {
	&procfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops procfs_vfsops = {
	MOUNT_PROCFS,
	procfs_mount,
	procfs_start,
	procfs_unmount,
	procfs_root,
	procfs_quotactl,
	procfs_statfs,
	procfs_sync,
	procfs_vget,
	procfs_fhtovp,
	procfs_vptofh,
	procfs_init,
	procfs_reinit,
	procfs_done,
	procfs_sysctl,
	NULL,				/* vfs_mountroot */
	procfs_checkexp,
	procfs_vnodeopv_descs,
};
