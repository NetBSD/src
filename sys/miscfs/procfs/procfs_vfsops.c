/*
 * Copyright (c) 1993 Paul Kranenburg
 * All rights reserved.
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
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: procfs_vfsops.c,v 1.7.2.1 1993/11/14 22:35:05 mycroft Exp $
 */

/*
 * PROCFS VFS interface routines
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>

#include <miscfs/procfs/pfsnode.h>

extern struct vnodeops pfs_vnodeops;

/*
 * mfs vfs operations.
 */
int pfs_mount();
int pfs_start();
int pfs_unmount();
int pfs_root();
int pfs_quotactl();
int pfs_statfs();
int pfs_sync();
int pfs_fhtovp();
int pfs_vptofh();
int pfs_init();

struct vfsops procfs_vfsops = {
	pfs_mount,
	pfs_start,
	pfs_unmount,
	pfs_root,
	pfs_quotactl,
	pfs_statfs,
	pfs_sync,
	pfs_fhtovp,
	pfs_vptofh,
	pfs_init,
};

/*
 * VFS Operations.
 *
 * mount system call
 */
/* ARGSUSED */
pfs_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
#if 0
	struct pfs_args args;
#endif
	struct vnode *pvp;
	u_int size;
	int error;

	if (mp->mnt_flag & MNT_UPDATE) {
		return (0);
	}

#if 0
	if (error = copyin(data, (caddr_t)&args, sizeof (struct pfs_args)))
		return (error);
#endif
	(void) copyinstr(path, (caddr_t)mp->mnt_stat.f_mntonname, MNAMELEN, &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);

	size = sizeof("proc") - 1;
	bcopy("proc", mp->mnt_stat.f_mntfromname, size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);

	(void) pfs_statfs(mp, &mp->mnt_stat, p);
	return (0);
}

/*
 * unmount system call
 */
pfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	return (0);
}

pfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *vp;
	struct pfsnode *pfsp, **pp;
	int error;

	/* Look in "cache" first */
	for (pfsp = pfshead; pfsp != NULL; pfsp = pfsp->pfs_next) {
		if (pfsp->pfs_vnode->v_flag & VROOT) {
			*vpp = pfsp->pfs_vnode;
			vref(*vpp);
			return 0;
		}
	}

	/* Not on list, allocate new vnode */
	error = getnewvnode(VT_PROCFS, mp, &pfs_vnodeops, &vp);
	if (error)
		return error;

	vp->v_type = VDIR;
	vp->v_flag = VROOT;
	pfsp = VTOPFS(vp);
	pfsp->pfs_next = NULL;
	pfsp->pfs_pid = 0;
	pfsp->pfs_vnode = vp;
	pfsp->pfs_flags = 0;
	pfsp->pfs_vflags = 0;
	pfsp->pfs_uid = 0;
	pfsp->pfs_gid = 0;
	pfsp->pfs_mode = 0755;	/* /proc = drwxr-xr-x */

	/* Append to pfs node list */
	for (pp = &pfshead; *pp; pp = &(*pp)->pfs_next);
	*pp = pfsp;

	*vpp = vp;
	return 0;
}

/*
 */
/* ARGSUSED */
pfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return 0;
}

/*
 * Get file system statistics.
 */
pfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	sbp->f_type = MOUNT_PROCFS;
	sbp->f_fsize = 0;
	sbp->f_bsize = 0;
	sbp->f_blocks = 0;
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_files = maxproc + 3;
	sbp->f_ffree = maxproc - nprocs;

	return 0;
}


pfs_quotactl(mp, cmds, uid, arg, p)
	struct mount *mp;
	int cmds;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	return EOPNOTSUPP;
}

pfs_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
	return 0;
}

pfs_fhtovp(mp, fhp, vpp)
	register struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
	return EINVAL;
}

pfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	return EINVAL;
}

pfs_init()
{
	return 0;
}
