/*	$NetBSD: efs_vfsops.c,v 1.5 2007/07/17 11:19:32 pooka Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efs_vfsops.c,v 1.5 2007/07/17 11:19:32 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/fstypes.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/kauth.h>

#include <miscfs/genfs/genfs_node.h>

#include <miscfs/specfs/specdev.h>

#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>
#include <fs/efs/efs_dir.h>
#include <fs/efs/efs_genfs.h>
#include <fs/efs/efs_mount.h>
#include <fs/efs/efs_extent.h>
#include <fs/efs/efs_dinode.h>
#include <fs/efs/efs_inode.h>
#include <fs/efs/efs_subr.h>
#include <fs/efs/efs_ihash.h>

MALLOC_JUSTDEFINE(M_EFSMNT, "efsmnt", "efs mount structure");
MALLOC_JUSTDEFINE(M_EFSINO, "efsino", "efs in-core inode structure");
MALLOC_JUSTDEFINE(M_EFSTMP, "efstmp", "efs temporary allocations");

extern int (**efs_vnodeop_p)(void *); 	/* for getnewvnode() */
static int efs_statvfs(struct mount *, struct statvfs *, struct lwp *);

/*
 * efs_mount and efs_mountroot common functions.
 */
static int
efs_mount_common(struct mount *mp, const char *path, struct vnode *devvp,
    struct efs_args *args, struct lwp *l)
{
	int err;
	struct buf *bp;
	const char *why;
	struct efs_mount *emp;

	emp = malloc(sizeof(*emp), M_EFSMNT, M_WAITOK);
	emp->em_dev = devvp->v_rdev;
	emp->em_devvp = devvp;
	emp->em_mnt = mp;

	/* read in the superblock */
	err = efs_bread(emp, EFS_BB_SB, l, &bp);
	if (err) {
		EFS_DPRINTF(("superblock read failed\n"));
		free(emp, M_EFSMNT);
		brelse(bp);
		return (err);
	}
	memcpy(&emp->em_sb, bp->b_data, sizeof(emp->em_sb));
	brelse(bp);

	/* validate the superblock */
	if (efs_sb_validate(&emp->em_sb, &why)) {
		printf("efs: invalid superblock: %s\n", why);
		if (!(mp->mnt_flag & MNT_FORCE)) {
			free(emp, M_EFSMNT);
			return (EIO);
		}
	}

	/* check that it's clean */
	if (be16toh(emp->em_sb.sb_dirty) != EFS_SB_CLEAN) {
		printf("efs: filesystem is dirty (sb_dirty = 0x%x); please "
		    "run fsck_efs(8)\n", be16toh(emp->em_sb.sb_dirty));
		/* XXX - default to readonly unless forced?? */
	}

	/* if the superblock was replicated, verify that it is the same */
	if (be32toh(emp->em_sb.sb_replsb) != 0) {
		struct buf *rbp;
		bool skip = false;

		err = efs_bread(emp, be32toh(emp->em_sb.sb_replsb), l, &rbp);
		if (err) {
			printf("efs: read of superblock replicant failed; "
			    "please run fsck_efs(8)\n");
			if (mp->mnt_flag & MNT_FORCE) {
				skip = true;
			} else {
				free(emp, M_EFSMNT);
				brelse(rbp);
				return (err);
			}
		}

		if (!skip) {
			if (memcmp(rbp->b_data, &emp->em_sb,
			    sizeof(emp->em_sb))) {
				printf("efs: superblock differs from "
				    "replicant; please run fsck_efs(8)\n");
				if (!(mp->mnt_flag & MNT_FORCE)) {
					brelse(rbp);
					free(emp, M_EFSMNT);
					return (EIO);
				}
			}
		}
		brelse(rbp);
	}

	/* ensure we can read last block */
	err = efs_bread(emp, be32toh(emp->em_sb.sb_size) - 1, l, &bp);
	if (err) {
		printf("efs: cannot access all filesystem blocks; please run "
		    "fsck_efs(8)\n");
		if (!(mp->mnt_flag & MNT_FORCE)) {
			free(emp, M_EFSMNT);
			brelse(bp);
			return (err);
		}
	}
	brelse(bp);

	mp->mnt_data = emp;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_fs_bshift = EFS_BB_SHFT;
	mp->mnt_dev_bshift = DEV_BSHIFT;
	vfs_getnewfsid(mp);
	efs_statvfs(mp, &mp->mnt_stat, l);

	err = set_statvfs_info(path, UIO_USERSPACE, args->fspec,
	    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, l);
	if (err)
		free(emp, M_EFSMNT);

	return (err);
}

/*
 * mount syscall vfsop.
 *
 * Returns 0 on success.
 */
static int
efs_mount(struct mount *mp, const char *path, void *data, size_t *data_len,
    struct nameidata *ndp, struct lwp *l)
{
	struct efs_args *args = data;
	struct nameidata devndp;
	struct efs_mount *emp; 
	struct vnode *devvp;
	int err, mode;

	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		if ((emp = VFSTOEFS(mp)) == NULL)
			return (EIO);
		args->fspec = NULL;
		args->version = EFS_MNT_VERSION;
		*data_len = sizeof *args;
		return 0;
	}

	if (mp->mnt_flag & MNT_UPDATE)
		return (EOPNOTSUPP);	/* XXX read-only */

	/* look up our device's vnode. it is returned locked */
	NDINIT(&devndp, LOOKUP, FOLLOW | LOCKLEAF,
	    UIO_USERSPACE, args->fspec, l);
	if ((err = namei(&devndp)))
		return (err);

	devvp = devndp.ni_vp;
	if (devvp->v_type != VBLK) {
		vput(devvp);
		return (ENOTBLK);
	}

	/* XXX - rdonly */
	mode = FREAD;

	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL)) {
		err = VOP_ACCESS(devvp, mode, l->l_cred, l);
		if (err) {
			vput(devvp);
			return (err);
		}
	}

	if ((err = VOP_OPEN(devvp, mode, l->l_cred, l))) {
		vput(devvp);
		return (err);
	}

	err = efs_mount_common(mp, path, devvp, args, l);
	if (err) {
		VOP_CLOSE(devvp, mode, l->l_cred, l);
		vput(devvp);
		return (err);
	}

	VOP_UNLOCK(devvp, 0);

	return (0);
}

/*
 * Initialisation routine.
 *
 * Returns 0 on success.
 */
static int
efs_start(struct mount *mp, int flags, struct lwp *l)
{

	return (0);
}

/*
 * unmount syscall vfsop. 
 *
 * Returns 0 on success.
 */
static int
efs_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	struct efs_mount *emp;
	int err;

	(void)l;

	emp = VFSTOEFS(mp);

	err = vflush(mp, NULL, (mntflags & MNT_FORCE) ? FORCECLOSE : 0);
	if (err)
		return (err);

	cache_purgevfs(mp);

	vn_lock(emp->em_devvp, LK_EXCLUSIVE | LK_RETRY);
        err = VOP_CLOSE(emp->em_devvp, FREAD, l->l_cred, l);
	vput(emp->em_devvp);

	free(mp->mnt_data, M_EFSMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;

	return (err);
}

/*
 * Return the root vnode.
 *
 * Returns 0 on success.
 */
static int
efs_root(struct mount *mp, struct vnode **vpp)
{
	int err;
	struct vnode *vp;
	
	if ((err = VFS_VGET(mp, EFS_ROOTINO, &vp)))
		return (err);

	*vpp = vp;
	return (0);
}

/*
 * statvfs syscall vfsop.
 *
 * Returns 0 on success.
 */
static int
efs_statvfs(struct mount *mp, struct statvfs *sbp, struct lwp *l)
{
	struct efs_mount *emp;
	
	emp = VFSTOEFS(mp);
	sbp->f_bsize	= EFS_BB_SIZE;
	sbp->f_frsize	= EFS_BB_SIZE;
	sbp->f_iosize	= EFS_BB_SIZE;
	sbp->f_blocks	= be32toh(emp->em_sb.sb_size);
	sbp->f_bfree	= be32toh(emp->em_sb.sb_tfree);
	sbp->f_bavail	= sbp->f_bfree; // XXX same?? 
	sbp->f_bresvd	= 0;
	sbp->f_files	= be32toh(emp->em_sb.sb_tinode);
	sbp->f_ffree	= be16toh(emp->em_sb.sb_cgisize) *
			  be16toh(emp->em_sb.sb_ncg) *
			  EFS_DINODES_PER_BB;
	sbp->f_favail	= sbp->f_ffree; // XXX same??
	sbp->f_fresvd	= 0;
	sbp->f_namemax	= EFS_DIRENT_NAMELEN_MAX;
	copy_statvfs_info(sbp, mp);

	return (0);
}

/*
 * Obtain a locked vnode for the given on-disk inode number.
 *
 * We currently allocate a new vnode from getnewnode(), tack it with
 * our in-core inode structure (efs_inode), and read in the inode from
 * disk. The returned inode must be locked.
 *
 * Returns 0 on success.
 */
static int
efs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	int err;
	struct vnode *vp;
	struct efs_inode *eip;
	struct efs_mount *emp;

	emp = VFSTOEFS(mp);

	while (true) {
		*vpp = efs_ihashget(emp->em_dev, ino, LK_EXCLUSIVE);
		if (*vpp != NULL)
			return (0);

		err = getnewvnode(VT_EFS, mp, efs_vnodeop_p, &vp);
		if (err)
			return (err);
		
		eip = pool_get(&efs_inode_pool, PR_WAITOK);

		/*
		 * See if anybody has raced us here.  If not, continue
		 * setting up the new inode, otherwise start over.
		 */
		efs_ihashlock();

		if (efs_ihashget(emp->em_dev, ino, 0) == NULL)
			break;

		efs_ihashunlock();
		ungetnewvnode(vp);
		pool_put(&efs_inode_pool, eip);
	}

	vp->v_flag |= VLOCKSWORK;
	eip->ei_mode = 0;
	eip->ei_lockf = NULL;
	eip->ei_number = ino;
	eip->ei_dev = emp->em_dev;
	eip->ei_vp = vp;
	vp->v_data = eip;
	vp->v_mount = mp;

	/*
	 * Place the vnode on the hash chain. Doing so will lock the
	 * vnode, so it's okay to drop the global lock and read in
	 * the inode from disk.
	 */
	efs_ihashins(eip);
	efs_ihashunlock();

	/*
	 * Init genfs early, otherwise we'll trip up on genfs_node_destroy
	 * in efs_reclaim when vput()ing in an error branch here.
	 */
	genfs_node_init(vp, &efs_genfsops);

	err = efs_read_inode(emp, ino, NULL, &eip->ei_di);
	if (err) {
		vput(vp);
		*vpp = NULL;
		return (err);
	}

	efs_sync_dinode_to_inode(eip);
	vp->v_size = eip->ei_size;

	if (ino == EFS_ROOTINO && !S_ISDIR(eip->ei_mode)) {
		printf("efs: root inode (%lu) is not a directory!\n",
		    (ulong)EFS_ROOTINO);
		vput(vp);
		*vpp = NULL;
		return (EIO);
	}

	switch (eip->ei_mode & S_IFMT) {
	case S_IFIFO:
		vp->v_type = VFIFO;
		break;
	case S_IFCHR:
		vp->v_type = VCHR;
		break;
	case S_IFDIR:
		vp->v_type = VDIR;
		if (ino == EFS_ROOTINO)
			vp->v_flag |= VROOT;
		break;
	case S_IFBLK:
		vp->v_type = VBLK;
		break;
	case S_IFREG:
		vp->v_type = VREG;
		break;
	case S_IFLNK:
		vp->v_type = VLNK;
		break;
	case S_IFSOCK:
		vp->v_type = VSOCK;
		break;
	default:
		printf("efs: invalid mode 0x%x in inode %lu on mount %s\n",
		    eip->ei_mode, (ulong)ino, mp->mnt_stat.f_mntonname);
		vput(vp);
		*vpp = NULL;
		return (EIO);
	}

	uvm_vnp_setsize(vp, vp->v_size);
	*vpp = vp;

	KASSERT(VOP_ISLOCKED(vp));

	return (0);
}

/*
 * Convert the provided opaque, unique file handle into a vnode.
 *
 * Returns 0 on success.
 */
static int
efs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	int err;
	struct vnode *vp;
	struct efs_fid *efp;
	struct efs_inode *eip;

	if (fhp->fid_len != sizeof(struct efs_fid)) 
		return (EINVAL);

	efp = (struct efs_fid *)fhp;

	if ((err = VFS_VGET(mp, efp->ef_ino, &vp))) {
		*vpp = NULL;
		return (err);
	}

	eip = EFS_VTOI(vp);
	if (eip->ei_mode == 0 || eip->ei_gen != efp->ef_gen) {
		vput(vp);
		*vpp = NULL;
		return (ESTALE);
	}

	*vpp = vp;
	return (0);
}

/*
 * Convert the provided vnode into an opaque, unique file handle.
 *
 * Returns 0 on success.
 */
static int
efs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct efs_fid *efp;
	struct efs_inode *eip;

	if (*fh_size < sizeof(struct efs_fid)) {
		*fh_size = sizeof(struct efs_fid);
		return (E2BIG);
	}
	*fh_size = sizeof(struct efs_fid);

	eip = EFS_VTOI(vp); 
	efp = (struct efs_fid *)fhp;

	fhp->fid_len = sizeof(struct efs_fid);
	efp->ef_ino = eip->ei_number;
	efp->ef_gen = eip->ei_gen;

	return (0);
}

/*
 * Globally initialise the filesystem.
 */
static void 
efs_init(void)
{

	malloc_type_attach(M_EFSMNT);
	malloc_type_attach(M_EFSINO);
	malloc_type_attach(M_EFSTMP);
	efs_ihashinit();
	pool_init(&efs_inode_pool, sizeof(struct efs_inode), 0, 0, 0,
	    "efsinopl", &pool_allocator_nointr, IPL_NONE);
}

/*
 * Globally reinitialise the filesystem.
 */
static void 
efs_reinit(void)
{
	
	efs_ihashreinit();
}

/*
 * Globally clean up the filesystem.
 */
static void 
efs_done(void)
{
	
	pool_destroy(&efs_inode_pool);
	efs_ihashdone();
	malloc_type_detach(M_EFSMNT);
	malloc_type_detach(M_EFSINO);
	malloc_type_detach(M_EFSTMP);
}

extern const struct vnodeopv_desc efs_vnodeop_opv_desc;
//extern const struct vnodeopv_desc efs_specop_opv_desc;
//extern const struct vnodeopv_desc efs_fifoop_opv_desc;

const struct vnodeopv_desc * const efs_vnodeopv_descs[] = {
	&efs_vnodeop_opv_desc,
//	&efs_specop_opv_desc,
//	&efs_fifoop_opv_desc,
	NULL
};

struct vfsops efs_vfsops = {
	.vfs_name	= MOUNT_EFS,
	.vfs_min_mount_data = sizeof (struct efs_args),
	.vfs_mount	= efs_mount,
	.vfs_start	= efs_start,
	.vfs_unmount	= efs_unmount,
	.vfs_root	= efs_root,
	.vfs_quotactl	= (void *)eopnotsupp,
	.vfs_statvfs	= efs_statvfs,
	.vfs_sync	= (void *)eopnotsupp,
	.vfs_vget	= efs_vget,
	.vfs_fhtovp	= efs_fhtovp,
	.vfs_vptofh	= efs_vptofh,
	.vfs_init	= efs_init,
	.vfs_reinit	= efs_reinit,
	.vfs_done	= efs_done,
	.vfs_mountroot	= (void *)eopnotsupp,
	.vfs_snapshot	= (void *)eopnotsupp,
	.vfs_extattrctl	= vfs_stdextattrctl,
	.vfs_opv_descs	= efs_vnodeopv_descs
/*	.vfs_refcount */
/*	.vfs_list */
};
VFS_ATTACH(efs_vfsops);
