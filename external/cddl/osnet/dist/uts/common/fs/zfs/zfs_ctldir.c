/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright 2015, OmniTI Computer Consulting, Inc. All rights reserved.
 */

/*
 * ZFS control directory (a.k.a. ".zfs")
 *
 * This directory provides a common location for all ZFS meta-objects.
 * Currently, this is only the 'snapshot' directory, but this may expand in the
 * future.  The elements are built using the GFS primitives, as the hierarchy
 * does not actually exist on disk.
 *
 * For 'snapshot', we don't want to have all snapshots always mounted, because
 * this would take up a huge amount of space in /etc/mnttab.  We have three
 * types of objects:
 *
 * 	ctldir ------> snapshotdir -------> snapshot
 *                                             |
 *                                             |
 *                                             V
 *                                         mounted fs
 *
 * The 'snapshot' node contains just enough information to lookup '..' and act
 * as a mountpoint for the snapshot.  Whenever we lookup a specific snapshot, we
 * perform an automount of the underlying filesystem and return the
 * corresponding vnode.
 *
 * All mounts are handled automatically by the kernel, but unmounts are
 * (currently) handled from user land.  The main reason is that there is no
 * reliable way to auto-unmount the filesystem when it's "no longer in use".
 * When the user unmounts a filesystem, we call zfsctl_unmount(), which
 * unmounts any snapshots within the snapshot directory.
 *
 * The '.zfs', '.zfs/snapshot', and all directories created under
 * '.zfs/snapshot' (ie: '.zfs/snapshot/<snapname>') are all GFS nodes and
 * share the same vfs_t as the head filesystem (what '.zfs' lives under).
 *
 * File systems mounted ontop of the GFS nodes '.zfs/snapshot/<snapname>'
 * (ie: snapshots) are ZFS nodes and have their own unique vfs_t.
 * However, vnodes within these mounted on file systems have their v_vfsp
 * fields set to the head filesystem to make NFS happy (see
 * zfsctl_snapdir_lookup()). We VFS_HOLD the head filesystem's vfs_t
 * so that it cannot be freed until all snapshots have been unmounted.
 */

#ifdef __FreeBSD__

#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_vfsops.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/dmu.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_destroy.h>
#include <sys/dsl_deleg.h>
#include <sys/mount.h>
#include <sys/zap.h>

#include "zfs_namecheck.h"

/*
 * "Synthetic" filesystem implementation.
 */

/*
 * Assert that A implies B.
 */
#define KASSERT_IMPLY(A, B, msg)	KASSERT(!(A) || (B), (msg));

static MALLOC_DEFINE(M_SFSNODES, "sfs_nodes", "synthetic-fs nodes");

typedef struct sfs_node {
	char		sn_name[ZFS_MAX_DATASET_NAME_LEN];
	uint64_t	sn_parent_id;
	uint64_t	sn_id;
} sfs_node_t;

/*
 * Check the parent's ID as well as the node's to account for a chance
 * that IDs originating from different domains (snapshot IDs, artifical
 * IDs, znode IDs) may clash.
 */
static int
sfs_compare_ids(struct vnode *vp, void *arg)
{
	sfs_node_t *n1 = vp->v_data;
	sfs_node_t *n2 = arg;
	bool equal;

	equal = n1->sn_id == n2->sn_id &&
	    n1->sn_parent_id == n2->sn_parent_id;

	/* Zero means equality. */
	return (!equal);
}

static int
sfs_vnode_get(const struct mount *mp, int flags, uint64_t parent_id,
   uint64_t id, struct vnode **vpp)
{
	sfs_node_t search;
	int err;

	search.sn_id = id;
	search.sn_parent_id = parent_id;
	err = vfs_hash_get(mp, (u_int)id, flags, curthread, vpp,
	    sfs_compare_ids, &search);
	return (err);
}

static int
sfs_vnode_insert(struct vnode *vp, int flags, uint64_t parent_id,
   uint64_t id, struct vnode **vpp)
{
	int err;

	KASSERT(vp->v_data != NULL, ("sfs_vnode_insert with NULL v_data"));
	err = vfs_hash_insert(vp, (u_int)id, flags, curthread, vpp,
	    sfs_compare_ids, vp->v_data);
	return (err);
}

static void
sfs_vnode_remove(struct vnode *vp)
{
	vfs_hash_remove(vp);
}

typedef void sfs_vnode_setup_fn(vnode_t *vp, void *arg);

static int
sfs_vgetx(struct mount *mp, int flags, uint64_t parent_id, uint64_t id,
    const char *tag, struct vop_vector *vops,
    sfs_vnode_setup_fn setup, void *arg,
    struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	error = sfs_vnode_get(mp, flags, parent_id, id, vpp);
	if (error != 0 || *vpp != NULL) {
		KASSERT_IMPLY(error == 0, (*vpp)->v_data != NULL,
		    "sfs vnode with no data");
		return (error);
	}

	/* Allocate a new vnode/inode. */
	error = getnewvnode(tag, mp, vops, &vp);
	if (error != 0) {
		*vpp = NULL;
		return (error);
	}

	/*
	 * Exclusively lock the vnode vnode while it's being constructed.
	 */
	lockmgr(vp->v_vnlock, LK_EXCLUSIVE, NULL);
	error = insmntque(vp, mp);
	if (error != 0) {
		*vpp = NULL;
		return (error);
	}

	setup(vp, arg);

	error = sfs_vnode_insert(vp, flags, parent_id, id, vpp);
	if (error != 0 || *vpp != NULL) {
		KASSERT_IMPLY(error == 0, (*vpp)->v_data != NULL,
		    "sfs vnode with no data");
		return (error);
	}

	*vpp = vp;
	return (0);
}

static void
sfs_print_node(sfs_node_t *node)
{
	printf("\tname = %s\n", node->sn_name);
	printf("\tparent_id = %ju\n", (uintmax_t)node->sn_parent_id);
	printf("\tid = %ju\n", (uintmax_t)node->sn_id);
}

static sfs_node_t *
sfs_alloc_node(size_t size, const char *name, uint64_t parent_id, uint64_t id)
{
	struct sfs_node *node;

	KASSERT(strlen(name) < sizeof(node->sn_name),
	    ("sfs node name is too long"));
	KASSERT(size >= sizeof(*node), ("sfs node size is too small"));
	node = malloc(size, M_SFSNODES, M_WAITOK | M_ZERO);
	strlcpy(node->sn_name, name, sizeof(node->sn_name));
	node->sn_parent_id = parent_id;
	node->sn_id = id;

	return (node);
}

static void
sfs_destroy_node(sfs_node_t *node)
{
	free(node, M_SFSNODES);
}

static void *
sfs_reclaim_vnode(vnode_t *vp)
{
	sfs_node_t *node;
	void *data;

	sfs_vnode_remove(vp);
	data = vp->v_data;
	vp->v_data = NULL;
	return (data);
}

static int
sfs_readdir_common(uint64_t parent_id, uint64_t id, struct vop_readdir_args *ap,
    uio_t *uio, off_t *offp)
{
	struct dirent entry;
	int error;

	/* Reset ncookies for subsequent use of vfs_read_dirent. */
	if (ap->a_ncookies != NULL)
		*ap->a_ncookies = 0;

	if (uio->uio_resid < sizeof(entry))
		return (SET_ERROR(EINVAL));

	if (uio->uio_offset < 0)
		return (SET_ERROR(EINVAL));
	if (uio->uio_offset == 0) {
		entry.d_fileno = id;
		entry.d_type = DT_DIR;
		entry.d_name[0] = '.';
		entry.d_name[1] = '\0';
		entry.d_namlen = 1;
		entry.d_reclen = sizeof(entry);
		error = vfs_read_dirent(ap, &entry, uio->uio_offset);
		if (error != 0)
			return (SET_ERROR(error));
	}

	if (uio->uio_offset < sizeof(entry))
		return (SET_ERROR(EINVAL));
	if (uio->uio_offset == sizeof(entry)) {
		entry.d_fileno = parent_id;
		entry.d_type = DT_DIR;
		entry.d_name[0] = '.';
		entry.d_name[1] = '.';
		entry.d_name[2] = '\0';
		entry.d_namlen = 2;
		entry.d_reclen = sizeof(entry);
		error = vfs_read_dirent(ap, &entry, uio->uio_offset);
		if (error != 0)
			return (SET_ERROR(error));
	}

	if (offp != NULL)
		*offp = 2 * sizeof(entry);
	return (0);
}


/*
 * .zfs inode namespace
 *
 * We need to generate unique inode numbers for all files and directories
 * within the .zfs pseudo-filesystem.  We use the following scheme:
 *
 * 	ENTRY			ZFSCTL_INODE
 * 	.zfs			1
 * 	.zfs/snapshot		2
 * 	.zfs/snapshot/<snap>	objectid(snap)
 */
#define	ZFSCTL_INO_SNAP(id)	(id)

static struct vop_vector zfsctl_ops_root;
static struct vop_vector zfsctl_ops_snapdir;
static struct vop_vector zfsctl_ops_snapshot;
static struct vop_vector zfsctl_ops_shares_dir;

void
zfsctl_init(void)
{
}

void
zfsctl_fini(void)
{
}

boolean_t
zfsctl_is_node(vnode_t *vp)
{
	return (vn_matchops(vp, zfsctl_ops_root) ||
	    vn_matchops(vp, zfsctl_ops_snapdir) ||
	    vn_matchops(vp, zfsctl_ops_snapshot) ||
	    vn_matchops(vp, zfsctl_ops_shares_dir));

}

typedef struct zfsctl_root {
	sfs_node_t	node;
	sfs_node_t	*snapdir;
	timestruc_t	cmtime;
} zfsctl_root_t;


/*
 * Create the '.zfs' directory.
 */
void
zfsctl_create(zfsvfs_t *zfsvfs)
{
	zfsctl_root_t *dot_zfs;
	sfs_node_t *snapdir;
	vnode_t *rvp;
	uint64_t crtime[2];

	ASSERT(zfsvfs->z_ctldir == NULL);

	snapdir = sfs_alloc_node(sizeof(*snapdir), "snapshot", ZFSCTL_INO_ROOT,
	    ZFSCTL_INO_SNAPDIR);
	dot_zfs = (zfsctl_root_t *)sfs_alloc_node(sizeof(*dot_zfs), ".zfs", 0,
	    ZFSCTL_INO_ROOT);
	dot_zfs->snapdir = snapdir;

	VERIFY(VFS_ROOT(zfsvfs->z_vfs, LK_EXCLUSIVE, &rvp) == 0);
	VERIFY(0 == sa_lookup(VTOZ(rvp)->z_sa_hdl, SA_ZPL_CRTIME(zfsvfs),
	    &crtime, sizeof(crtime)));
	ZFS_TIME_DECODE(&dot_zfs->cmtime, crtime);
	vput(rvp);

	zfsvfs->z_ctldir = dot_zfs;
}

/*
 * Destroy the '.zfs' directory.  Only called when the filesystem is unmounted.
 * The nodes must not have any associated vnodes by now as they should be
 * vflush-ed.
 */
void
zfsctl_destroy(zfsvfs_t *zfsvfs)
{
	sfs_destroy_node(zfsvfs->z_ctldir->snapdir);
	sfs_destroy_node((sfs_node_t *)zfsvfs->z_ctldir);
	zfsvfs->z_ctldir = NULL;
}

static int
zfsctl_fs_root_vnode(struct mount *mp, void *arg __unused, int flags,
    struct vnode **vpp)
{
	return (VFS_ROOT(mp, flags, vpp));
}

static void
zfsctl_common_vnode_setup(vnode_t *vp, void *arg)
{
	ASSERT_VOP_ELOCKED(vp, __func__);

	/* We support shared locking. */
	VN_LOCK_ASHARE(vp);
	vp->v_type = VDIR;
	vp->v_data = arg;
}

static int
zfsctl_root_vnode(struct mount *mp, void *arg __unused, int flags,
    struct vnode **vpp)
{
	void *node;
	int err;

	node = ((zfsvfs_t*)mp->mnt_data)->z_ctldir;
	err = sfs_vgetx(mp, flags, 0, ZFSCTL_INO_ROOT, "zfs", &zfsctl_ops_root,
	    zfsctl_common_vnode_setup, node, vpp);
	return (err);
}

static int
zfsctl_snapdir_vnode(struct mount *mp, void *arg __unused, int flags,
    struct vnode **vpp)
{
	void *node;
	int err;

	node = ((zfsvfs_t*)mp->mnt_data)->z_ctldir->snapdir;
	err = sfs_vgetx(mp, flags, ZFSCTL_INO_ROOT, ZFSCTL_INO_SNAPDIR, "zfs",
	   &zfsctl_ops_snapdir, zfsctl_common_vnode_setup, node, vpp);
	return (err);
}

/*
 * Given a root znode, retrieve the associated .zfs directory.
 * Add a hold to the vnode and return it.
 */
int
zfsctl_root(zfsvfs_t *zfsvfs, int flags, vnode_t **vpp)
{
	vnode_t *vp;
	int error;

	error = zfsctl_root_vnode(zfsvfs->z_vfs, NULL, flags, vpp);
	return (error);
}

/*
 * Common open routine.  Disallow any write access.
 */
/* ARGSUSED */
static int
zfsctl_common_open(struct vop_open_args *ap)
{
	int flags = ap->a_mode;

	if (flags & FWRITE)
		return (SET_ERROR(EACCES));

	return (0);
}

/*
 * Common close routine.  Nothing to do here.
 */
/* ARGSUSED */
static int
zfsctl_common_close(struct vop_close_args *ap)
{
	return (0);
}

/*
 * Common access routine.  Disallow writes.
 */
/* ARGSUSED */
static int
zfsctl_common_access(ap)
	struct vop_access_args /* {
		struct vnode *a_vp;
		accmode_t a_accmode;
		struct ucred *a_cred;
		struct thread *a_td;
	} */ *ap;
{
	accmode_t accmode = ap->a_accmode;

	if (accmode & VWRITE)
		return (SET_ERROR(EACCES));
	return (0);
}

/*
 * Common getattr function.  Fill in basic information.
 */
static void
zfsctl_common_getattr(vnode_t *vp, vattr_t *vap)
{
	timestruc_t	now;
	sfs_node_t *node;

	node = vp->v_data;

	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_rdev = 0;
	/*
	 * We are a purely virtual object, so we have no
	 * blocksize or allocated blocks.
	 */
	vap->va_blksize = 0;
	vap->va_nblocks = 0;
	vap->va_seq = 0;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	vap->va_mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
	    S_IROTH | S_IXOTH;
	vap->va_type = VDIR;
	/*
	 * We live in the now (for atime).
	 */
	gethrestime(&now);
	vap->va_atime = now;
	/* FreeBSD: Reset chflags(2) flags. */
	vap->va_flags = 0;

	vap->va_nodeid = node->sn_id;

	/* At least '.' and '..'. */
	vap->va_nlink = 2;
}

/*ARGSUSED*/
static int
zfsctl_common_fid(ap)
	struct vop_fid_args /* {
		struct vnode *a_vp;
		struct fid *a_fid;
	} */ *ap;
{
	vnode_t		*vp = ap->a_vp;
	fid_t		*fidp = (void *)ap->a_fid;
	sfs_node_t	*node = vp->v_data;
	uint64_t	object = node->sn_id;
	zfid_short_t	*zfid;
	int		i;

	zfid = (zfid_short_t *)fidp;
	zfid->zf_len = SHORT_FID_LEN;

	for (i = 0; i < sizeof(zfid->zf_object); i++)
		zfid->zf_object[i] = (uint8_t)(object >> (8 * i));

	/* .zfs nodes always have a generation number of 0 */
	for (i = 0; i < sizeof(zfid->zf_gen); i++)
		zfid->zf_gen[i] = 0;

	return (0);
}

static int
zfsctl_common_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct thread *a_td;
	} */ *ap;
{
	vnode_t *vp = ap->a_vp;

	(void) sfs_reclaim_vnode(vp);
	return (0);
}

static int
zfsctl_common_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	sfs_print_node(ap->a_vp->v_data);
	return (0);
}

/*
 * Get root directory attributes.
 */
/* ARGSUSED */
static int
zfsctl_root_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	zfsctl_root_t *node = vp->v_data;

	zfsctl_common_getattr(vp, vap);
	vap->va_ctime = node->cmtime;
	vap->va_mtime = vap->va_ctime;
	vap->va_birthtime = vap->va_ctime;
	vap->va_nlink += 1; /* snapdir */
	vap->va_size = vap->va_nlink;
	return (0);
}

/*
 * When we lookup "." we still can be asked to lock it
 * differently, can't we?
 */
int
zfsctl_relock_dot(vnode_t *dvp, int ltype)
{
	vref(dvp);
	if (ltype != VOP_ISLOCKED(dvp)) {
		if (ltype == LK_EXCLUSIVE)
			vn_lock(dvp, LK_UPGRADE | LK_RETRY);
		else /* if (ltype == LK_SHARED) */
			vn_lock(dvp, LK_DOWNGRADE | LK_RETRY);

		/* Relock for the "." case may left us with reclaimed vnode. */
		if ((dvp->v_iflag & VI_DOOMED) != 0) {
			vrele(dvp);
			return (SET_ERROR(ENOENT));
		}
	}
	return (0);
}

/*
 * Special case the handling of "..".
 */
int
zfsctl_root_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap;
{
	struct componentname *cnp = ap->a_cnp;
	vnode_t *dvp = ap->a_dvp;
	vnode_t **vpp = ap->a_vpp;
	cred_t *cr = ap->a_cnp->cn_cred;
	int flags = ap->a_cnp->cn_flags;
	int lkflags = ap->a_cnp->cn_lkflags;
	int nameiop = ap->a_cnp->cn_nameiop;
	int err;
	int ltype;

	ASSERT(dvp->v_type == VDIR);

	if ((flags & ISLASTCN) != 0 && nameiop != LOOKUP)
		return (SET_ERROR(ENOTSUP));

	if (cnp->cn_namelen == 1 && *cnp->cn_nameptr == '.') {
		err = zfsctl_relock_dot(dvp, lkflags & LK_TYPE_MASK);
		if (err == 0)
			*vpp = dvp;
	} else if ((flags & ISDOTDOT) != 0) {
		err = vn_vget_ino_gen(dvp, zfsctl_fs_root_vnode, NULL,
		    lkflags, vpp);
	} else if (strncmp(cnp->cn_nameptr, "snapshot", cnp->cn_namelen) == 0) {
		err = zfsctl_snapdir_vnode(dvp->v_mount, NULL, lkflags, vpp);
	} else {
		err = SET_ERROR(ENOENT);
	}
	if (err != 0)
		*vpp = NULL;
	return (err);
}

static int
zfsctl_root_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
		int *a_eofflag;
		int *ncookies;
		u_long **a_cookies;
	} */ *ap;
{
	struct dirent entry;
	vnode_t *vp = ap->a_vp;
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	zfsctl_root_t *node = vp->v_data;
	uio_t *uio = ap->a_uio;
	int *eofp = ap->a_eofflag;
	off_t dots_offset;
	int error;

	ASSERT(vp->v_type == VDIR);

	error = sfs_readdir_common(zfsvfs->z_root, ZFSCTL_INO_ROOT, ap, uio,
	    &dots_offset);
	if (error != 0) {
		if (error == ENAMETOOLONG) /* ran out of destination space */
			error = 0;
		return (error);
	}
	if (uio->uio_offset != dots_offset)
		return (SET_ERROR(EINVAL));

	CTASSERT(sizeof(node->snapdir->sn_name) <= sizeof(entry.d_name));
	entry.d_fileno = node->snapdir->sn_id;
	entry.d_type = DT_DIR;
	strcpy(entry.d_name, node->snapdir->sn_name);
	entry.d_namlen = strlen(entry.d_name);
	entry.d_reclen = sizeof(entry);
	error = vfs_read_dirent(ap, &entry, uio->uio_offset);
	if (error != 0) {
		if (error == ENAMETOOLONG)
			error = 0;
		return (SET_ERROR(error));
	}
	if (eofp != NULL)
		*eofp = 1;
	return (0);
}

static int
zfsctl_root_vptocnp(struct vop_vptocnp_args *ap)
{
	static const char dotzfs_name[4] = ".zfs";
	vnode_t *dvp;
	int error;

	if (*ap->a_buflen < sizeof (dotzfs_name))
		return (SET_ERROR(ENOMEM));

	error = vn_vget_ino_gen(ap->a_vp, zfsctl_fs_root_vnode, NULL,
	    LK_SHARED, &dvp);
	if (error != 0)
		return (SET_ERROR(error));

	VOP_UNLOCK(dvp, 0);
	*ap->a_vpp = dvp;
	*ap->a_buflen -= sizeof (dotzfs_name);
	bcopy(dotzfs_name, ap->a_buf + *ap->a_buflen, sizeof (dotzfs_name));
	return (0);
}

static struct vop_vector zfsctl_ops_root = {
	.vop_default =	&default_vnodeops,
	.vop_open =	zfsctl_common_open,
	.vop_close =	zfsctl_common_close,
	.vop_ioctl =	VOP_EINVAL,
	.vop_getattr =	zfsctl_root_getattr,
	.vop_access =	zfsctl_common_access,
	.vop_readdir =	zfsctl_root_readdir,
	.vop_lookup =	zfsctl_root_lookup,
	.vop_inactive =	VOP_NULL,
	.vop_reclaim =	zfsctl_common_reclaim,
	.vop_fid =	zfsctl_common_fid,
	.vop_print =	zfsctl_common_print,
	.vop_vptocnp =	zfsctl_root_vptocnp,
};

static int
zfsctl_snapshot_zname(vnode_t *vp, const char *name, int len, char *zname)
{
	objset_t *os = ((zfsvfs_t *)((vp)->v_vfsp->vfs_data))->z_os;

	dmu_objset_name(os, zname);
	if (strlen(zname) + 1 + strlen(name) >= len)
		return (SET_ERROR(ENAMETOOLONG));
	(void) strcat(zname, "@");
	(void) strcat(zname, name);
	return (0);
}

static int
zfsctl_snapshot_lookup(vnode_t *vp, const char *name, uint64_t *id)
{
	objset_t *os = ((zfsvfs_t *)((vp)->v_vfsp->vfs_data))->z_os;
	int err;

	err = dsl_dataset_snap_lookup(dmu_objset_ds(os), name, id);
	return (err);
}

/*
 * Given a vnode get a root vnode of a filesystem mounted on top of
 * the vnode, if any.  The root vnode is referenced and locked.
 * If no filesystem is mounted then the orinal vnode remains referenced
 * and locked.  If any error happens the orinal vnode is unlocked and
 * released.
 */
static int
zfsctl_mounted_here(vnode_t **vpp, int flags)
{
	struct mount *mp;
	int err;

	ASSERT_VOP_LOCKED(*vpp, __func__);
	ASSERT3S((*vpp)->v_type, ==, VDIR);

	if ((mp = (*vpp)->v_mountedhere) != NULL) {
		err = vfs_busy(mp, 0);
		KASSERT(err == 0, ("vfs_busy(mp, 0) failed with %d", err));
		KASSERT(vrefcnt(*vpp) > 1, ("unreferenced mountpoint"));
		vput(*vpp);
		err = VFS_ROOT(mp, flags, vpp);
		vfs_unbusy(mp);
		return (err);
	}
	return (EJUSTRETURN);
}

typedef struct {
	const char *snap_name;
	uint64_t    snap_id;
} snapshot_setup_arg_t;

static void
zfsctl_snapshot_vnode_setup(vnode_t *vp, void *arg)
{
	snapshot_setup_arg_t *ssa = arg;
	sfs_node_t *node;

	ASSERT_VOP_ELOCKED(vp, __func__);

	node = sfs_alloc_node(sizeof(sfs_node_t),
	    ssa->snap_name, ZFSCTL_INO_SNAPDIR, ssa->snap_id);
	zfsctl_common_vnode_setup(vp, node);

	/* We have to support recursive locking. */
	VN_LOCK_AREC(vp);
}

/*
 * Lookup entry point for the 'snapshot' directory.  Try to open the
 * snapshot if it exist, creating the pseudo filesystem vnode as necessary.
 * Perform a mount of the associated dataset on top of the vnode.
 */
/* ARGSUSED */
int
zfsctl_snapdir_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap;
{
	vnode_t *dvp = ap->a_dvp;
	vnode_t **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	char name[NAME_MAX + 1];
	char fullname[ZFS_MAX_DATASET_NAME_LEN];
	char *mountpoint;
	size_t mountpoint_len;
	zfsvfs_t *zfsvfs = dvp->v_vfsp->vfs_data;
	uint64_t snap_id;
	int nameiop = cnp->cn_nameiop;
	int lkflags = cnp->cn_lkflags;
	int flags = cnp->cn_flags;
	int err;

	ASSERT(dvp->v_type == VDIR);

	if ((flags & ISLASTCN) != 0 && nameiop != LOOKUP)
		return (SET_ERROR(ENOTSUP));

	if (cnp->cn_namelen == 1 && *cnp->cn_nameptr == '.') {
		err = zfsctl_relock_dot(dvp, lkflags & LK_TYPE_MASK);
		if (err == 0)
			*vpp = dvp;
		return (err);
	}
	if (flags & ISDOTDOT) {
		err = vn_vget_ino_gen(dvp, zfsctl_root_vnode, NULL, lkflags,
		    vpp);
		return (err);
	}

	if (cnp->cn_namelen >= sizeof(name))
		return (SET_ERROR(ENAMETOOLONG));

	strlcpy(name, ap->a_cnp->cn_nameptr, ap->a_cnp->cn_namelen + 1);
	err = zfsctl_snapshot_lookup(dvp, name, &snap_id);
	if (err != 0)
		return (SET_ERROR(ENOENT));

	for (;;) {
		snapshot_setup_arg_t ssa;

		ssa.snap_name = name;
		ssa.snap_id = snap_id;
		err = sfs_vgetx(dvp->v_mount, LK_SHARED, ZFSCTL_INO_SNAPDIR,
		   snap_id, "zfs", &zfsctl_ops_snapshot,
		   zfsctl_snapshot_vnode_setup, &ssa, vpp);
		if (err != 0)
			return (err);

		/* Check if a new vnode has just been created. */
		if (VOP_ISLOCKED(*vpp) == LK_EXCLUSIVE)
			break;

		/*
		 * The vnode must be referenced at least by this thread and
		 * the mounted snapshot or the thread doing the mounting.
		 * There can be more references from concurrent lookups.
		 */
		KASSERT(vrefcnt(*vpp) > 1, ("found unreferenced mountpoint"));

		/*
		 * Check if a snapshot is already mounted on top of the vnode.
		 */
		err = zfsctl_mounted_here(vpp, lkflags);
		if (err != EJUSTRETURN)
			return (err);

#ifdef INVARIANTS
		/*
		 * If the vnode not covered yet, then the mount operation
		 * must be in progress.
		 */
		VI_LOCK(*vpp);
		KASSERT(((*vpp)->v_iflag & VI_MOUNT) != 0,
		    ("snapshot vnode not covered"));
		VI_UNLOCK(*vpp);
#endif
		vput(*vpp);

		/*
		 * In this situation we can loop on uncontested locks and starve
		 * the thread doing the lengthy, non-trivial mount operation.
		 */
		kern_yield(PRI_USER);
	}

	VERIFY0(zfsctl_snapshot_zname(dvp, name, sizeof(fullname), fullname));

	mountpoint_len = strlen(dvp->v_vfsp->mnt_stat.f_mntonname) +
	    strlen("/" ZFS_CTLDIR_NAME "/snapshot/") + strlen(name) + 1;
	mountpoint = kmem_alloc(mountpoint_len, KM_SLEEP);
	(void) snprintf(mountpoint, mountpoint_len,
	    "%s/" ZFS_CTLDIR_NAME "/snapshot/%s",
	    dvp->v_vfsp->mnt_stat.f_mntonname, name);

	err = mount_snapshot(curthread, vpp, "zfs", mountpoint, fullname, 0);
	kmem_free(mountpoint, mountpoint_len);
	if (err == 0) {
		/*
		 * Fix up the root vnode mounted on .zfs/snapshot/<snapname>.
		 *
		 * This is where we lie about our v_vfsp in order to
		 * make .zfs/snapshot/<snapname> accessible over NFS
		 * without requiring manual mounts of <snapname>.
		 */
		ASSERT(VTOZ(*vpp)->z_zfsvfs != zfsvfs);
		VTOZ(*vpp)->z_zfsvfs->z_parent = zfsvfs;

		/* Clear the root flag (set via VFS_ROOT) as well. */
		(*vpp)->v_vflag &= ~VV_ROOT;
	}

	if (err != 0)
		*vpp = NULL;
	return (err);
}

static int
zfsctl_snapdir_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
		int *a_eofflag;
		int *ncookies;
		u_long **a_cookies;
	} */ *ap;
{
	char snapname[ZFS_MAX_DATASET_NAME_LEN];
	struct dirent entry;
	vnode_t *vp = ap->a_vp;
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	uio_t *uio = ap->a_uio;
	int *eofp = ap->a_eofflag;
	off_t dots_offset;
	int error;

	ASSERT(vp->v_type == VDIR);

	error = sfs_readdir_common(ZFSCTL_INO_ROOT, ZFSCTL_INO_SNAPDIR, ap, uio,
	    &dots_offset);
	if (error != 0) {
		if (error == ENAMETOOLONG) /* ran out of destination space */
			error = 0;
		return (error);
	}

	for (;;) {
		uint64_t cookie;
		uint64_t id;

		cookie = uio->uio_offset - dots_offset;

		dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
		error = dmu_snapshot_list_next(zfsvfs->z_os, sizeof(snapname),
		    snapname, &id, &cookie, NULL);
		dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
		if (error != 0) {
			if (error == ENOENT) {
				if (eofp != NULL)
					*eofp = 1;
				error = 0;
			}
			return (error);
		}

		entry.d_fileno = id;
		entry.d_type = DT_DIR;
		strcpy(entry.d_name, snapname);
		entry.d_namlen = strlen(entry.d_name);
		entry.d_reclen = sizeof(entry);
		error = vfs_read_dirent(ap, &entry, uio->uio_offset);
		if (error != 0) {
			if (error == ENAMETOOLONG)
				error = 0;
			return (SET_ERROR(error));
		}
		uio->uio_offset = cookie + dots_offset;
	}
	/* NOTREACHED */
}

/* ARGSUSED */
static int
zfsctl_snapdir_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
	} */ *ap;
{
	vnode_t *vp = ap->a_vp;
	vattr_t *vap = ap->a_vap;
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	dsl_dataset_t *ds = dmu_objset_ds(zfsvfs->z_os);
	sfs_node_t *node = vp->v_data;
	uint64_t snap_count;
	int err;

	zfsctl_common_getattr(vp, vap);
	vap->va_ctime = dmu_objset_snap_cmtime(zfsvfs->z_os);
	vap->va_mtime = vap->va_ctime;
	vap->va_birthtime = vap->va_ctime;
	if (dsl_dataset_phys(ds)->ds_snapnames_zapobj != 0) {
		err = zap_count(dmu_objset_pool(ds->ds_objset)->dp_meta_objset,
		    dsl_dataset_phys(ds)->ds_snapnames_zapobj, &snap_count);
		if (err != 0)
			return (err);
		vap->va_nlink += snap_count;
	}
	vap->va_size = vap->va_nlink;

	return (0);
}

static struct vop_vector zfsctl_ops_snapdir = {
	.vop_default =	&default_vnodeops,
	.vop_open =	zfsctl_common_open,
	.vop_close =	zfsctl_common_close,
	.vop_getattr =	zfsctl_snapdir_getattr,
	.vop_access =	zfsctl_common_access,
	.vop_readdir =	zfsctl_snapdir_readdir,
	.vop_lookup =	zfsctl_snapdir_lookup,
	.vop_reclaim =	zfsctl_common_reclaim,
	.vop_fid =	zfsctl_common_fid,
	.vop_print =	zfsctl_common_print,
};

static int
zfsctl_snapshot_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct thread *a_td;
	} */ *ap;
{
	vnode_t *vp = ap->a_vp;

	VERIFY(vrecycle(vp) == 1);
	return (0);
}

static int
zfsctl_snapshot_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct thread *a_td;
	} */ *ap;
{
	vnode_t *vp = ap->a_vp;
	void *data = vp->v_data;

	sfs_reclaim_vnode(vp);
	sfs_destroy_node(data);
	return (0);
}

static int
zfsctl_snapshot_vptocnp(struct vop_vptocnp_args *ap)
{
	struct mount *mp;
	vnode_t *dvp;
	vnode_t *vp;
	sfs_node_t *node;
	size_t len;
	int locked;
	int error;

	vp = ap->a_vp;
	node = vp->v_data;
	len = strlen(node->sn_name);
	if (*ap->a_buflen < len)
		return (SET_ERROR(ENOMEM));

	/*
	 * Prevent unmounting of the snapshot while the vnode lock
	 * is not held.  That is not strictly required, but allows
	 * us to assert that an uncovered snapshot vnode is never
	 * "leaked".
	 */
	mp = vp->v_mountedhere;
	if (mp == NULL)
		return (SET_ERROR(ENOENT));
	error = vfs_busy(mp, 0);
	KASSERT(error == 0, ("vfs_busy(mp, 0) failed with %d", error));

	/*
	 * We can vput the vnode as we can now depend on the reference owned
	 * by the busied mp.  But we also need to hold the vnode, because
	 * the reference may go after vfs_unbusy() which has to be called
	 * before we can lock the vnode again.
	 */
	locked = VOP_ISLOCKED(vp);
	vhold(vp);
	vput(vp);

	/* Look up .zfs/snapshot, our parent. */
	error = zfsctl_snapdir_vnode(vp->v_mount, NULL, LK_SHARED, &dvp);
	if (error == 0) {
		VOP_UNLOCK(dvp, 0);
		*ap->a_vpp = dvp;
		*ap->a_buflen -= len;
		bcopy(node->sn_name, ap->a_buf + *ap->a_buflen, len);
	}
	vfs_unbusy(mp);
	vget(vp, locked | LK_VNHELD | LK_RETRY, curthread);
	return (error);
}

/*
 * These VP's should never see the light of day.  They should always
 * be covered.
 */
static struct vop_vector zfsctl_ops_snapshot = {
	.vop_default =		NULL, /* ensure very restricted access */
	.vop_inactive =		zfsctl_snapshot_inactive,
	.vop_reclaim =		zfsctl_snapshot_reclaim,
	.vop_vptocnp =		zfsctl_snapshot_vptocnp,
	.vop_lock1 =		vop_stdlock,
	.vop_unlock =		vop_stdunlock,
	.vop_islocked =		vop_stdislocked,
	.vop_advlockpurge =	vop_stdadvlockpurge, /* called by vgone */
	.vop_print =		zfsctl_common_print,
};

int
zfsctl_lookup_objset(vfs_t *vfsp, uint64_t objsetid, zfsvfs_t **zfsvfsp)
{
	struct mount *mp;
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	vnode_t *vp;
	int error;

	ASSERT(zfsvfs->z_ctldir != NULL);
	*zfsvfsp = NULL;
	error = sfs_vnode_get(vfsp, LK_EXCLUSIVE,
	    ZFSCTL_INO_SNAPDIR, objsetid, &vp);
	if (error == 0 && vp != NULL) {
		/*
		 * XXX Probably need to at least reference, if not busy, the mp.
		 */
		if (vp->v_mountedhere != NULL)
			*zfsvfsp = vp->v_mountedhere->mnt_data;
		vput(vp);
	}
	if (*zfsvfsp == NULL)
		return (SET_ERROR(EINVAL));
	return (0);
}

/*
 * Unmount any snapshots for the given filesystem.  This is called from
 * zfs_umount() - if we have a ctldir, then go through and unmount all the
 * snapshots.
 */
int
zfsctl_umount_snapshots(vfs_t *vfsp, int fflags, cred_t *cr)
{
	char snapname[ZFS_MAX_DATASET_NAME_LEN];
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	struct mount *mp;
	vnode_t *dvp;
	vnode_t *vp;
	sfs_node_t *node;
	sfs_node_t *snap;
	uint64_t cookie;
	int error;

	ASSERT(zfsvfs->z_ctldir != NULL);

	cookie = 0;
	for (;;) {
		uint64_t id;

		dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
		error = dmu_snapshot_list_next(zfsvfs->z_os, sizeof(snapname),
		    snapname, &id, &cookie, NULL);
		dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
		if (error != 0) {
			if (error == ENOENT)
				error = 0;
			break;
		}

		for (;;) {
			error = sfs_vnode_get(vfsp, LK_EXCLUSIVE,
			    ZFSCTL_INO_SNAPDIR, id, &vp);
			if (error != 0 || vp == NULL)
				break;

			mp = vp->v_mountedhere;

			/*
			 * v_mountedhere being NULL means that the
			 * (uncovered) vnode is in a transient state
			 * (mounting or unmounting), so loop until it
			 * settles down.
			 */
			if (mp != NULL)
				break;
			vput(vp);
		}
		if (error != 0)
			break;
		if (vp == NULL)
			continue;	/* no mountpoint, nothing to do */

		/*
		 * The mount-point vnode is kept locked to avoid spurious EBUSY
		 * from a concurrent umount.
		 * The vnode lock must have recursive locking enabled.
		 */
		vfs_ref(mp);
		error = dounmount(mp, fflags, curthread);
		KASSERT_IMPLY(error == 0, vrefcnt(vp) == 1,
		    ("extra references after unmount"));
		vput(vp);
		if (error != 0)
			break;
	}
	KASSERT_IMPLY((fflags & MS_FORCE) != 0, error == 0,
	    ("force unmounting failed"));
	return (error);
}

#endif /* __FreeBSD__ */

#ifdef __NetBSD__

#include <sys/malloc.h>
#include <sys/pathname.h>
#include <miscfs/genfs/genfs.h>
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/dsl_dataset.h>
#include <sys/zap.h>

struct zfsctl_root {
	timestruc_t zc_cmtime;
};

struct sfs_node_key {
	uint64_t parent_id;
	uint64_t id;
};
struct sfs_node {
	struct sfs_node_key sn_key;
#define sn_parent_id sn_key.parent_id
#define sn_id sn_key.id
	lwp_t *sn_mounting;
};

#define ZFS_SNAPDIR_NAME "snapshot"

#define VTOSFS(vp) ((struct sfs_node *)((vp)->v_data))

#define SFS_NODE_ASSERT(vp) \
	do { \
		struct sfs_node *np = VTOSFS(vp); \
		ASSERT((vp)->v_op == zfs_sfsop_p); \
		ASSERT((vp)->v_type == VDIR); \
	} while (/*CONSTCOND*/ 0)

static int (**zfs_sfsop_p)(void *);

/*
 * Mount a snapshot.  Cannot use do_sys_umount() as it
 * doesn't allow its "path" argument from SYSSPACE.
 */
static int
sfs_snapshot_mount(vnode_t *vp, const char *snapname)
{
	struct sfs_node *node = VTOSFS(vp);
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	vfs_t *vfsp;
	char *path, *osname;
	int error;
	extern int zfs_domount(vfs_t *, char *);

	path = PNBUF_GET();
	osname = PNBUF_GET();

	dmu_objset_name(zfsvfs->z_os, path);
	snprintf(osname, MAXPATHLEN, "%s@%s", path, snapname);
	snprintf(path, MAXPATHLEN,
	    "%s/" ZFS_CTLDIR_NAME "/" ZFS_SNAPDIR_NAME "/%s",
	    vp->v_vfsp->mnt_stat.f_mntonname, snapname);

	vfsp = vfs_mountalloc(vp->v_vfsp->mnt_op, vp);
	if (vfsp == NULL) {
		error = ENOMEM;
		goto out;
	}
	vfsp->mnt_op->vfs_refcount++;
	vfsp->mnt_stat.f_owner = 0;
	vfsp->mnt_flag = MNT_RDONLY | MNT_NOSUID | MNT_IGNORE;

	mutex_enter(&vfsp->mnt_updating);

	error = zfs_domount(vfsp, osname);
	if (error)
		goto out;

	/* Set f_fsidx from parent to cheat NFSD. */
	vfsp->mnt_stat.f_fsidx = vp->v_vfsp->mnt_stat.f_fsidx;

	strlcpy(vfsp->mnt_stat.f_mntfromname, osname,
	    sizeof(vfsp->mnt_stat.f_mntfromname));
	set_statvfs_info(path, UIO_SYSSPACE, vfsp->mnt_stat.f_mntfromname,
	    UIO_SYSSPACE, vfsp->mnt_op->vfs_name, vfsp, curlwp);

	vfsp->mnt_lower = vp->v_vfsp;

	mountlist_append(vfsp);
	vref(vp);
	vp->v_mountedhere = vfsp;

	mutex_exit(&vfsp->mnt_updating);
	(void) VFS_STATVFS(vfsp, &vfsp->mnt_stat);

out:;
	if (error && vfsp) {
		mutex_exit(&vfsp->mnt_updating);
		vfs_rele(vfsp);
	}
	PNBUF_PUT(osname);
	PNBUF_PUT(path);

	return error;
}

static int
sfs_lookup_snapshot(vnode_t *dvp, struct componentname *cnp, vnode_t **vpp)
{
	zfsvfs_t *zfsvfs = dvp->v_vfsp->vfs_data;
	vnode_t *vp;
	struct sfs_node *node;
	struct sfs_node_key key;
	char snapname[ZFS_MAX_DATASET_NAME_LEN];
	int error;

	/* Retrieve the snapshot object id and the to be mounted on vnode. */
	if (cnp->cn_namelen >= sizeof(snapname))
		return ENOENT;

	strlcpy(snapname, cnp->cn_nameptr, cnp->cn_namelen + 1);
	error = dsl_dataset_snap_lookup( dmu_objset_ds(zfsvfs->z_os),
	    snapname, &key.id);
	if (error)
		return error;
	key.parent_id = ZFSCTL_INO_SNAPDIR;
	error = vcache_get(zfsvfs->z_vfs, &key, sizeof(key), vpp);
	if (error)
		return error;

	/* Handle case where the vnode is currently mounting. */
	vp = *vpp;
	mutex_enter(vp->v_interlock);
	node = VTOSFS(vp);
	if (node->sn_mounting) {
		if (node->sn_mounting == curlwp)
			error = 0;
		else
			error = ERESTART;
		mutex_exit(vp->v_interlock);
		if (error)
			yield();
		return error;
	}

	/* If not yet mounted mount the snapshot. */
	if (vp->v_mountedhere == NULL) {
		ASSERT(node->sn_mounting == NULL);
		node->sn_mounting = curlwp;
		mutex_exit(vp->v_interlock);

		VOP_UNLOCK(dvp, 0);
		error = sfs_snapshot_mount(vp, snapname);
		if (vn_lock(dvp, LK_EXCLUSIVE) != 0) {
			vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			error = ENOENT;
		}

		mutex_enter(vp->v_interlock);
		if ((node = VTOSFS(vp)))
			node->sn_mounting = NULL;
		mutex_exit(vp->v_interlock);

		if (error) {
			vrele(vp);
			*vpp = NULL;
			return error;
		}
	} else
		mutex_exit(vp->v_interlock);

	/* Return the mounted root rather than the covered mount point.  */
	ASSERT(vp->v_mountedhere);
	error = VFS_ROOT(vp->v_mountedhere, vpp);
	vrele(vp);
	if (error)
		return error;

	/*
	 * Fix up the root vnode mounted on .zfs/snapshot/<snapname>
	 *
	 * Here we make .zfs/snapshot/<snapname> accessible over NFS
	 * without requiring manual mounts of <snapname>.
	 */
	if (((*vpp)->v_vflag & VV_ROOT)) {
		ASSERT(VTOZ(*vpp)->z_zfsvfs != zfsvfs);
		VTOZ(*vpp)->z_zfsvfs->z_parent = zfsvfs;
		(*vpp)->v_vflag &= ~VV_ROOT;
	}
	VOP_UNLOCK(*vpp, 0);

	return 0;
}

static int
sfs_lookup(void *v)
{
	struct vop_lookup_v2_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap = v;
	vnode_t *dvp = ap->a_dvp;
	vnode_t **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	zfsvfs_t *zfsvfs = dvp->v_vfsp->vfs_data;
	struct sfs_node *dnode = VTOSFS(dvp);
	int error;

	SFS_NODE_ASSERT(dvp);
	ZFS_ENTER(zfsvfs);

	/*
	 * No CREATE, DELETE or RENAME.
	 */
	if ((cnp->cn_flags & ISLASTCN) && cnp->cn_nameiop != LOOKUP) {
		ZFS_EXIT(zfsvfs);

		return ENOTSUP;
	}

	/*
	 * Handle DOT and DOTDOT.
	 */
	if (cnp->cn_namelen == 1 && cnp->cn_nameptr[0] == '.') {
		vref(dvp);
		*vpp = dvp;
		ZFS_EXIT(zfsvfs);

		return 0;
	}
	if ((cnp->cn_flags & ISDOTDOT)) {
		if (dnode->sn_parent_id == 0) {
			error = vcache_get(zfsvfs->z_vfs,
			    &zfsvfs->z_root, sizeof(zfsvfs->z_root), vpp);
		} else if (dnode->sn_parent_id == ZFSCTL_INO_ROOT) {
			error = zfsctl_root(zfsvfs, vpp);
		} else if (dnode->sn_parent_id == ZFSCTL_INO_SNAPDIR) {
			error = zfsctl_snapshot(zfsvfs, vpp);
		} else {
			error = ENOENT;
		}
		ZFS_EXIT(zfsvfs);

		return error;
	}

	/*
	 * Lookup in ".zfs".
	 */
	if (dnode->sn_id == ZFSCTL_INO_ROOT) {
		if (cnp->cn_namelen == strlen(ZFS_SNAPDIR_NAME) &&
		    strncmp(cnp->cn_nameptr, ZFS_SNAPDIR_NAME,
		    cnp->cn_namelen) == 0) {
			error = zfsctl_snapshot(zfsvfs, vpp);
		} else {
			error = ENOENT;
		}
		ZFS_EXIT(zfsvfs);

		return error;
	}

	/*
	 * Lookup in ".zfs/snapshot".
	 */
	if (dnode->sn_id == ZFSCTL_INO_SNAPDIR) {
		error = sfs_lookup_snapshot(dvp, cnp, vpp);
		ZFS_EXIT(zfsvfs);

		return error;
	}

	vprint("sfs_lookup: unexpected node for lookup", dvp);
	ZFS_EXIT(zfsvfs);

	return ENOENT;
}

static int
sfs_open(void *v)
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	zfsvfs_t *zfsvfs = ap->a_vp->v_vfsp->vfs_data;
	int error = 0;

	SFS_NODE_ASSERT(ap->a_vp);
	ZFS_ENTER(zfsvfs);

	if (ap->a_mode & FWRITE)
		error = EACCES;

	ZFS_EXIT(zfsvfs);

	return error;
}

static int
sfs_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	zfsvfs_t *zfsvfs = ap->a_vp->v_vfsp->vfs_data;

	SFS_NODE_ASSERT(ap->a_vp);
	ZFS_ENTER(zfsvfs);

	ZFS_EXIT(zfsvfs);

	return 0;
}

static int
sfs_access(void *v)
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	zfsvfs_t *zfsvfs = ap->a_vp->v_vfsp->vfs_data;
	int error = 0;

	SFS_NODE_ASSERT(ap->a_vp);
	ZFS_ENTER(zfsvfs);

	if (ap->a_mode & FWRITE)
		error = EACCES;

	ZFS_EXIT(zfsvfs);

	return error;
}

static int
sfs_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	struct sfs_node *node = VTOSFS(vp);
	struct vattr *vap = ap->a_vap;
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	dsl_dataset_t *ds = dmu_objset_ds(zfsvfs->z_os);
	timestruc_t now;
	uint64_t snap_count;
	int error;

	SFS_NODE_ASSERT(vp);
	ZFS_ENTER(zfsvfs);

	vap->va_type = VDIR;
	vap->va_mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
	    S_IROTH | S_IXOTH;
	vap->va_nlink = 2;
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = vp->v_vfsp->mnt_stat.f_fsid;
	vap->va_fileid = node->sn_id;
	vap->va_size = 0;
	vap->va_blocksize = 0;
	gethrestime(&now);
	vap->va_atime = now;
	vap->va_ctime = zfsvfs->z_ctldir->zc_cmtime;
	vap->va_mtime = vap->va_ctime;
	vap->va_birthtime = vap->va_ctime;
	vap->va_gen = 0;
	vap->va_flags = 0;
	vap->va_rdev = 0;
	vap->va_bytes = 0;
	vap->va_filerev = 0;

	switch (node->sn_id){
	case ZFSCTL_INO_ROOT:
		vap->va_nlink += 1; /* snapdir */
		vap->va_size = vap->va_nlink;
		break;
	case ZFSCTL_INO_SNAPDIR:
		if (dsl_dataset_phys(ds)->ds_snapnames_zapobj) {
			error = zap_count(
			    dmu_objset_pool(ds->ds_objset)->dp_meta_objset,
			    dsl_dataset_phys(ds)->ds_snapnames_zapobj,
			    &snap_count);
			if (error)
				return error;
			vap->va_nlink += snap_count;
		}
		vap->va_size = vap->va_nlink;
		break;
	}

	ZFS_EXIT(zfsvfs);

	return 0;
}

static int
sfs_readdir_one(struct vop_readdir_args *ap, struct dirent *dp,
    const char *name, ino_t ino, off_t *offp)
{
	int error;

	dp->d_fileno = ino;
	dp->d_type = DT_DIR;
	strlcpy(dp->d_name, name, sizeof(dp->d_name));
	dp->d_namlen = strlen(dp->d_name);
	dp->d_reclen = _DIRENT_SIZE(dp);

	if (ap->a_uio->uio_resid < dp->d_reclen)
		return ENAMETOOLONG;
	if (ap->a_uio->uio_offset > *offp) {
		*offp += dp->d_reclen;
		return 0;
	}

	error = uiomove(dp, dp->d_reclen, UIO_READ, ap->a_uio);
	if (error)
		return error;
	if (ap->a_ncookies)
		(*ap->a_cookies)[(*ap->a_ncookies)++] = *offp;
	*offp += dp->d_reclen;

	return 0;
}

static int
sfs_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	struct sfs_node *node = VTOSFS(vp);
	zfsvfs_t *zfsvfs = vp->v_vfsp->vfs_data;
	struct dirent *dp;
	uint64_t parent;
	off_t offset;
	int error, ncookies;

	SFS_NODE_ASSERT(ap->a_vp);
	ZFS_ENTER(zfsvfs);
 
	parent = node->sn_parent_id == 0 ? zfsvfs->z_root : node->sn_parent_id;
	dp = kmem_alloc(sizeof(*dp), KM_SLEEP);
	if (ap->a_ncookies) {
		ncookies = ap->a_uio->uio_resid / _DIRENT_MINSIZE(dp);
		*ap->a_ncookies = 0;
		*ap->a_cookies = malloc(ncookies * sizeof (off_t),
		    M_TEMP, M_WAITOK);
	}

	offset = 0;
	error = sfs_readdir_one(ap, dp, ".", node->sn_id, &offset);
	if (error == 0)
		error = sfs_readdir_one(ap, dp, "..", parent, &offset);
	if (error == 0 && node->sn_id == ZFSCTL_INO_ROOT) {
		error = sfs_readdir_one(ap, dp, ZFS_SNAPDIR_NAME,
		    ZFSCTL_INO_SNAPDIR, &offset);
	} else if (error == 0 && node->sn_id == ZFSCTL_INO_SNAPDIR) {
		char snapname[ZFS_MAX_DATASET_NAME_LEN];
		uint64_t cookie, id;

		cookie = 0;
		for (;;) {
			dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os),
			    FTAG);
			error = dmu_snapshot_list_next(zfsvfs->z_os,
			    sizeof(snapname), snapname, &id, &cookie, NULL);
			dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os),
			    FTAG);
			if (error) {
				if (error == ENOENT)
					error = 0;
				break;
			}
			error = sfs_readdir_one(ap, dp, snapname, id, &offset);
			if (error)
				break;
		}
	}

	if (ap->a_eofflag && error == 0)
		*ap->a_eofflag = 1;

	if (error == ENAMETOOLONG)
		error = 0;

	if (ap->a_ncookies && error) {
		free(*ap->a_cookies, M_TEMP);
		*ap->a_ncookies = 0;
		*ap->a_cookies = NULL;
	}
	kmem_free(dp, sizeof(*dp));

	ZFS_EXIT(zfsvfs);

	return error;
}

static int
sfs_inactive(void *v)
{
	struct vop_inactive_v2_args /* {
		struct vnode *a_vp;
		bool *a_recycle;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	struct sfs_node *node = VTOSFS(vp);

	SFS_NODE_ASSERT(vp);

	*ap->a_recycle = (node->sn_parent_id == ZFSCTL_INO_SNAPDIR);

	return 0;
}

static int
sfs_reclaim(void *v)
{
	struct vop_reclaim_v2_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp;
	struct sfs_node *node = VTOSFS(vp);

	SFS_NODE_ASSERT(ap->a_vp);

	vp->v_data = NULL;
	VOP_UNLOCK(vp, 0);

	kmem_free(node, sizeof(*node));

	return 0;
}

static int
sfs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct sfs_node *node = VTOSFS(ap->a_vp);

	SFS_NODE_ASSERT(ap->a_vp);

	printf("\tid %" PRIu64 ", parent %" PRIu64 "\n",
	    node->sn_id, node->sn_parent_id);

	return 0;
}

const struct vnodeopv_entry_desc zfs_sfsop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		sfs_lookup },
	{ &vop_open_desc,		sfs_open },
	{ &vop_close_desc,		sfs_close },
	{ &vop_access_desc,		sfs_access },
	{ &vop_getattr_desc,		sfs_getattr },
	{ &vop_lock_desc,		genfs_lock },
	{ &vop_unlock_desc,		genfs_unlock },
	{ &vop_readdir_desc,		sfs_readdir },
	{ &vop_inactive_desc,		sfs_inactive },
	{ &vop_reclaim_desc,		sfs_reclaim },
	{ &vop_seek_desc,		genfs_seek },
	{ &vop_putpages_desc,		genfs_null_putpages },
	{ &vop_islocked_desc,		genfs_islocked },
	{ &vop_print_desc,		sfs_print },
	{ NULL, NULL }
};

const struct vnodeopv_desc zfs_sfsop_opv_desc =
	{ &zfs_sfsop_p, zfs_sfsop_entries };

void
zfsctl_init(void)
{
}

void
zfsctl_fini(void)
{
}

int
zfsctl_loadvnode(vfs_t *vfsp, vnode_t *vp,
    const void *key, size_t key_len, const void **new_key)
{
	struct sfs_node_key node_key;
	struct sfs_node *node;

	if (key_len != sizeof(node_key))
		return EINVAL;
	if ((vfsp->mnt_iflag & IMNT_UNMOUNT))
		return ENOENT;

	memcpy(&node_key, key, key_len);

	node = kmem_alloc(sizeof(*node), KM_SLEEP);

	node->sn_mounting = NULL;
	node->sn_key = node_key;

	vp->v_data = node;
	vp->v_op = zfs_sfsop_p;
	vp->v_tag = VT_ZFS;
	vp->v_type = VDIR;
	uvm_vnp_setsize(vp, 0);

	*new_key = &node->sn_key;

	return 0;
}

int
zfsctl_vptofh(vnode_t *vp, fid_t *fidp, size_t *fh_size)
{
	struct sfs_node *node = VTOSFS(vp);
	uint64_t object = node->sn_id;
	zfid_short_t *zfid = (zfid_short_t *)fidp;
	int i;

	SFS_NODE_ASSERT(vp);

	if (*fh_size < SHORT_FID_LEN) {
		*fh_size = SHORT_FID_LEN;
		return SET_ERROR(E2BIG);
	}
	*fh_size = SHORT_FID_LEN;

	zfid->zf_len = SHORT_FID_LEN;
	for (i = 0; i < sizeof(zfid->zf_object); i++)
		zfid->zf_object[i] = (uint8_t)(object >> (8 * i));

	/* .zfs nodes always have a generation number of 0 */
	for (i = 0; i < sizeof(zfid->zf_gen); i++)
		zfid->zf_gen[i] = 0;

	return 0;
}

/*
 * Return the ".zfs" vnode.
 */
int
zfsctl_root(zfsvfs_t *zfsvfs, vnode_t **vpp)
{
	struct sfs_node_key key = {
		.parent_id = 0,
		.id = ZFSCTL_INO_ROOT
	};

	return vcache_get(zfsvfs->z_vfs, &key, sizeof(key), vpp);
}

/*
 * Return the ".zfs/snapshot" vnode.
 */
int
zfsctl_snapshot(zfsvfs_t *zfsvfs, vnode_t **vpp)
{
	struct sfs_node_key key = {
		.parent_id = ZFSCTL_INO_ROOT,
		.id = ZFSCTL_INO_SNAPDIR
	};

	return vcache_get(zfsvfs->z_vfs, &key, sizeof(key), vpp);
}

void
zfsctl_create(zfsvfs_t *zfsvfs)
{
	vnode_t *vp;
	struct zfsctl_root *zc;
	uint64_t crtime[2];

	zc = kmem_alloc(sizeof(*zc), KM_SLEEP);

	VERIFY(0 == VFS_ROOT(zfsvfs->z_vfs, &vp));
	VERIFY(0 == sa_lookup(VTOZ(vp)->z_sa_hdl, SA_ZPL_CRTIME(zfsvfs),
	    &crtime, sizeof(crtime)));
	vput(vp);

	ZFS_TIME_DECODE(&zc->zc_cmtime, crtime);

	ASSERT(zfsvfs->z_ctldir == NULL);
	zfsvfs->z_ctldir = zc;
}

void
zfsctl_destroy(zfsvfs_t *zfsvfs)
{
	struct zfsctl_root *zc = zfsvfs->z_ctldir;

	ASSERT(zfsvfs->z_ctldir);
 	zfsvfs->z_ctldir = NULL;
	kmem_free(zc, sizeof(*zc));
}

int
zfsctl_lookup_objset(vfs_t *vfsp, uint64_t objsetid, zfsvfs_t **zfsvfsp)
{
	struct sfs_node_key key = {
		.parent_id = ZFSCTL_INO_SNAPDIR,
		.id = objsetid
	};
	vnode_t *vp;
	int error;

	*zfsvfsp = NULL;
	error = vcache_get(vfsp, &key, sizeof(key), &vp);
	if (error == 0) {
		if (vp->v_mountedhere)
			*zfsvfsp = vp->v_mountedhere->mnt_data;
		vrele(vp);
	}
	if (*zfsvfsp == NULL)
		return SET_ERROR(EINVAL);
	return 0;
}

int
zfsctl_umount_snapshots(vfs_t *vfsp, int fflags, cred_t *cr)
{
	char snapname[ZFS_MAX_DATASET_NAME_LEN];
	zfsvfs_t *zfsvfs = vfsp->vfs_data;
	struct mount *mp;
	vnode_t *vp;
	struct sfs_node_key key;
	uint64_t cookie;
	int error;

	ASSERT(zfsvfs->z_ctldir);

	cookie = 0;
	key.parent_id = ZFSCTL_INO_SNAPDIR;
	for (;;) {
		dsl_pool_config_enter(dmu_objset_pool(zfsvfs->z_os), FTAG);
		error = dmu_snapshot_list_next(zfsvfs->z_os, sizeof(snapname),
		    snapname, &key.id, &cookie, NULL);
		dsl_pool_config_exit(dmu_objset_pool(zfsvfs->z_os), FTAG);
		if (error) {
			if (error == ENOENT)
				error = 0;
			break;
		}

		error = vcache_get(zfsvfs->z_vfs, &key, sizeof(key), &vp);
		if (error == ENOENT)
			continue;
		else if (error)
			break;

		mp = vp->v_mountedhere;
		if (mp == NULL) {
			vrele(vp);
			continue;
		}

		error = dounmount(mp, fflags, curthread);
		vrele(vp);
		if (error)
			break;
	}
	ASSERT((fflags & MS_FORCE) == 0 || error == 0);

	return (error);
}

boolean_t
zfsctl_is_node(vnode_t *vp)
{

	return (vp->v_op == zfs_sfsop_p);
}
#endif /* __NetBSD__ */
