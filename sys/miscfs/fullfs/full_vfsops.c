/*	$NetBSD$	*/

/*
 * Full file-system: VFS operations.
 *
 * See full_vnops.c for a description.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/module.h>

#include <miscfs/fullfs/full.h>
#include <miscfs/fullfs/full_ioctl.h>
#include <miscfs/genfs/layer_extern.h>

MODULE(MODULE_CLASS_VFS, full, "layerfs");

VFS_PROTOS(fullfs);

int
fullfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct vnode *lowerrootvp, *vp;
	struct full_args *args = data;
	struct full_mount *nmp;
	struct layer_mount *lmp;
	struct pathbuf *pb;
	struct nameidata nd;
	int error;

	if (args == NULL)
		return EINVAL;
	if (*data_len < sizeof(*args))
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		lmp = MOUNTTOLAYERMOUNT(mp);
		if (lmp == NULL)
			return EIO;
		args->la.target = NULL;
		*data_len = sizeof(*args);
		return 0;
	}

	/* Update is not supported. */
	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	/* Find the lower vnode and lock it. */
	error = pathbuf_copyin(args->la.target, &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW|LOCKLEAF, pb);
	if ((error = namei(&nd)) != 0) {
		pathbuf_destroy(pb);
		return error;
	}
	lowerrootvp = nd.ni_vp;
	pathbuf_destroy(pb);

	/* Create the mount point. */
	nmp = kmem_zalloc(sizeof(struct full_mount), KM_SLEEP);
	mp->mnt_data = nmp;
	mp->mnt_iflag |= lowerrootvp->v_mount->mnt_iflag & IMNT_MPSAFE;
	mp->mnt_iflag |= lowerrootvp->v_mount->mnt_iflag & IMNT_SHRLOOKUP;

	/*
	 * Make sure that the mount point is sufficiently initialized
	 * that the node create call will work.
	 */
	vfs_getnewfsid(mp);
	error = vfs_set_lowermount(mp, lowerrootvp->v_mount);
	if (error) {
		vput(lowerrootvp);
		kmem_free(nmp, sizeof(struct full_mount));
		return error;
	}

	mutex_init(&nmp->fullm_lock, MUTEX_DEFAULT, IPL_NONE);
	nmp->fullm_mode = FULLFS_MODE_FAIL;
	nmp->fullm_error = ENOSPC;
	nmp->fullm_opmask = FULLFS_OP_ALL;
	nmp->fullm_rate = 0;
	nmp->fullm_doom = 0;

	nmp->fullm_size = sizeof(struct full_node);
	nmp->fullm_tag = VT_FULL;
	nmp->fullm_bypass = layer_bypass;
	nmp->fullm_vnodeop_p = full_vnodeop_p;

	/* Setup a full node for root vnode. */
	VOP_UNLOCK(lowerrootvp);
	error = layer_node_create(mp, lowerrootvp, &vp);
	if (error) {
		vrele(lowerrootvp);
		mutex_destroy(&nmp->fullm_lock);
		kmem_free(nmp, sizeof(struct full_mount));
		return error;
	}
	/*
	 * Keep a held reference to the root vnode.  It will be released on
	 * umount.  Note: fullfs is MP-safe.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vp->v_vflag |= VV_ROOT;
	nmp->fullm_rootvp = vp;
	VOP_UNLOCK(vp);

	error = set_statvfs_info(path, UIO_USERSPACE, args->la.target,
	    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, curlwp);
	if (error) {
		vgone(nmp->fullm_rootvp);
		mutex_destroy(&nmp->fullm_lock);
		kmem_free(nmp, sizeof(struct full_mount));
		return error;
	}

	if (mp->mnt_lower->mnt_flag & MNT_LOCAL)
		mp->mnt_flag |= MNT_LOCAL;
	return 0;
}

int
fullfs_unmount(struct mount *mp, int mntflags)
{
	struct full_mount *nmp = MOUNTTOFULLMOUNT(mp);
	struct vnode *full_rootvp = nmp->fullm_rootvp;
	int error, flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if (vrefcnt(full_rootvp) > 1 && (mntflags & MNT_FORCE) == 0)
		return EBUSY;

	if ((error = vflush(mp, full_rootvp, flags)) != 0)
		return error;

	/* Eliminate all activity and release the vnode. */
	vgone(full_rootvp);

	/* Finally, destroy the mount point structures. */
	mutex_destroy(&nmp->fullm_lock);
	kmem_free(mp->mnt_data, sizeof(struct full_mount));
	mp->mnt_data = NULL;
	return 0;
}

int
fullfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	struct full_mount *nmp = MOUNTTOFULLMOUNT(mp);
	int error;

	error = layerfs_statvfs(mp, sbp);
	if (error)
		return error;

	/* A full filesystem has no free space. */
	mutex_enter(&nmp->fullm_lock);
	if (nmp->fullm_mode != FULLFS_MODE_PASS) {
		sbp->f_bfree = 0;
		sbp->f_bavail = 0;
		sbp->f_bresvd = 0;
		sbp->f_ffree = 0;
		sbp->f_favail = 0;
		sbp->f_fresvd = 0;
	}

	mutex_exit(&nmp->fullm_lock);
	return 0;
}

extern const struct vnodeopv_desc full_vnodeop_opv_desc;

const struct vnodeopv_desc * const fullfs_vnodeopv_descs[] = {
	&full_vnodeop_opv_desc,
	NULL,
};

struct vfsops fullfs_vfsops = {
	.vfs_name = MOUNT_FULL,
	.vfs_min_mount_data = sizeof (struct full_args),
	.vfs_mount = fullfs_mount,
	.vfs_start = layerfs_start,
	.vfs_unmount = fullfs_unmount,
	.vfs_root = layerfs_root,
	.vfs_quotactl = layerfs_quotactl,
	.vfs_statvfs = fullfs_statvfs,
	.vfs_sync = layerfs_sync,
	.vfs_loadvnode = layerfs_loadvnode,
	.vfs_vget = layerfs_vget,
	.vfs_fhtovp = layerfs_fhtovp,
	.vfs_vptofh = layerfs_vptofh,
	.vfs_init = layerfs_init,
	.vfs_done = layerfs_done,
	.vfs_snapshot = layerfs_snapshot,
	.vfs_extattrctl = vfs_stdextattrctl,
	.vfs_suspendctl = layerfs_suspendctl,
	.vfs_renamelock_enter = layerfs_renamelock_enter,
	.vfs_renamelock_exit = layerfs_renamelock_exit,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = fullfs_vnodeopv_descs
};

static int
full_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = vfs_attach(&fullfs_vfsops);
		break;
	case MODULE_CMD_FINI:
		error = vfs_detach(&fullfs_vfsops);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
