/*	$NetBSD$	*/

/*
 * Full file-system.
 *
 * Implemented using layerfs, see layer_vnops.c for a description.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <sys/cprng.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/layer_extern.h>
#include <miscfs/fullfs/full.h>
#include <miscfs/fullfs/full_ioctl.h>

static int
fullfs_should_fail(struct mount *mp, int opmask_bit, int64_t nbytes)
{
	struct full_mount *nmp = MOUNTTOFULLMOUNT(mp);
	int error;

	mutex_enter(&nmp->fullm_lock);
	if ((nmp->fullm_opmask & opmask_bit) == 0) {
		mutex_exit(&nmp->fullm_lock);
		return 0;
	}

	switch (nmp->fullm_mode) {
	case FULLFS_MODE_PASS:
		error = 0;
		break;
	case FULLFS_MODE_FAIL:
		error = nmp->fullm_error;
		break;
	case FULLFS_MODE_COUNT:
		if (nmp->fullm_doom == 0)
			error = nmp->fullm_error;
		else {
			nmp->fullm_doom -= 1;
			error = 0;
		}
		break;
	case FULLFS_MODE_BYTES:
		if (nbytes == 0) {
			error = 0;
			break;
		}
		if (nbytes > (int64_t)nmp->fullm_doom)
			error = nmp->fullm_error;
		else {
			nmp->fullm_doom -= (unsigned int)nbytes;
			error = 0;
		}
		break;
	case FULLFS_MODE_RANDOM:
		if (cprng_fast32() % 100 < nmp->fullm_rate)
			error = nmp->fullm_error;
		else
			error = 0;
		break;
	default:
		KASSERTMSG(0, "%s: invalid fullm_mode %d", __func__, nmp->fullm_mode);
		error = nmp->fullm_error;
		break;
	}

	mutex_exit(&nmp->fullm_lock);
	return error;
}

static int
fullfs_write(void *v)
{
	struct vop_write_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_vp->v_mount, FULLFS_OP_WRITE,
		ap->a_uio->uio_resid);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_create(void *v)
{
	struct vop_create_v3_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_dvp->v_mount, FULLFS_OP_CREATE, 0);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_mkdir(void *v)
{
	struct vop_mkdir_v3_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_dvp->v_mount, FULLFS_OP_MKDIR, 0);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_mknod(void *v)
{
	struct vop_mknod_v3_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_dvp->v_mount, FULLFS_OP_MKNOD, 0);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_symlink(void *v)
{
	struct vop_symlink_v3_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_dvp->v_mount, FULLFS_OP_SYMLINK, 0);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_link(void *v)
{
	struct vop_link_v2_args *ap = v;
	int error;

	error = fullfs_should_fail(ap->a_dvp->v_mount, FULLFS_OP_LINK, 0);
	if (error)
		return error;
	return layer_bypass(v);
}

static int
fullfs_ioctl(void *v)
{
	struct vop_ioctl_args *ap = v;
	struct full_mount *nmp = MOUNTTOFULLMOUNT(ap->a_vp->v_mount);
	struct fullfs_ctl *fc = (struct fullfs_ctl *)ap->a_data;

	switch (ap->a_command) {
	case FULLFS_GETSTATE:
		mutex_enter(&nmp->fullm_lock);
		fc->fc_mode = nmp->fullm_mode;
		fc->fc_opmask = nmp->fullm_opmask;
		fc->fc_error = nmp->fullm_error;
		fc->fc_rate = nmp->fullm_rate;
		fc->fc_doom = nmp->fullm_doom;
		mutex_exit(&nmp->fullm_lock);
		return 0;
	case FULLFS_SETSTATE:
		if (fc->fc_mode < FULLFS_MODE_PASS ||
			fc->fc_mode > FULLFS_MODE_RANDOM)
			return EINVAL;
		if (fc->fc_error <= 0)
			return EINVAL;
		if (fc->fc_opmask & ~FULLFS_OP_ALL)
			return EINVAL;
		if (fc->fc_mode == FULLFS_MODE_COUNT && fc->fc_doom == 0)
			return EINVAL;
		if (fc->fc_mode == FULLFS_MODE_BYTES && fc->fc_doom == 0)
			return EINVAL;
		if (fc->fc_mode == FULLFS_MODE_RANDOM &&
			(fc->fc_rate == 0 || fc->fc_rate > 99))
			return EINVAL;

		mutex_enter(&nmp->fullm_lock);
		nmp->fullm_mode = fc->fc_mode;
		nmp->fullm_opmask = fc->fc_opmask;
		nmp->fullm_error = fc->fc_error;
		nmp->fullm_rate = fc->fc_rate;
		nmp->fullm_doom = fc->fc_doom;
		mutex_exit(&nmp->fullm_lock);
		return 0;
	default:
		return layer_bypass(v);
	}
}

/*
 * Global VFS data structures.
 */

int (**full_vnodeop_p)(void *);

const struct vnodeopv_entry_desc full_vnodeop_entries[] = {
	{ &vop_default_desc,	layer_bypass },

	{ &vop_lookup_desc,	layer_lookup },
	{ &vop_setattr_desc,	layer_setattr },
	{ &vop_getattr_desc,	layer_getattr },
	{ &vop_access_desc,	layer_access },
	{ &vop_accessx_desc,	genfs_accessx },
	{ &vop_fsync_desc,	layer_fsync },
	{ &vop_inactive_desc,	layer_inactive },
	{ &vop_reclaim_desc,	layer_reclaim },
	{ &vop_print_desc,	layer_print },
	{ &vop_remove_desc,	layer_remove },
	{ &vop_rename_desc,	layer_rename },
	{ &vop_revoke_desc,	layer_revoke },
	{ &vop_rmdir_desc,	layer_rmdir },

	{ &vop_open_desc,	layer_open },
	{ &vop_close_desc,	layer_close },

	{ &vop_bmap_desc,	layer_bmap },
	{ &vop_getpages_desc,	layer_getpages },
	{ &vop_putpages_desc,	layer_putpages },

	{ &vop_write_desc,	fullfs_write },
	{ &vop_create_desc,	fullfs_create },
	{ &vop_mkdir_desc,	fullfs_mkdir },
	{ &vop_mknod_desc,	fullfs_mknod },
	{ &vop_symlink_desc,	fullfs_symlink },
	{ &vop_link_desc,	fullfs_link },
	{ &vop_ioctl_desc,	fullfs_ioctl },

	{ NULL, NULL }
};

const struct vnodeopv_desc full_vnodeop_opv_desc = {
	&full_vnodeop_p, full_vnodeop_entries
};
