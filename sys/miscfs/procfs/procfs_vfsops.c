/*
 *	%W% (Erasmus) %G%	- pk@cs.few.eur.nl
 */

#include "param.h"
#include "time.h"
#include "kernel.h"
#include "proc.h"
#include "buf.h"
#include "mount.h"
#include "signalvar.h"
#include "vnode.h"

#include "pfsnode.h"

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
	struct pfsnode *pfsp;
	int error;

	error = getnewvnode(VT_PROCFS, mp, &pfs_vnodeops, &vp);
	if (error)
		return error;

	vp->v_type = VDIR;
	vp->v_flag = VROOT;
	pfsp = VTOPFS(vp);
	pfsp->pfs_vnode = vp;
	pfsp->pfs_pid = 0;

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
	sbp->f_fsize = nprocs;
	sbp->f_bsize = 0;
	sbp->f_blocks = 0;
	sbp->f_bfree = maxproc - nprocs;
	sbp->f_bavail = 0;
	sbp->f_files =  0;
	sbp->f_ffree = 0;

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
