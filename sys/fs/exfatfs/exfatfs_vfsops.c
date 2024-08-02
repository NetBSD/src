/* $NetBSD: exfatfs_vfsops.c,v 1.1.2.6 2024/08/02 00:16:55 perseant Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_vfsops.c,v 1.1.2.6 2024/08/02 00:16:55 perseant Exp $");

struct vm_page;

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_balloc.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_mount.h>
#include <fs/exfatfs/exfatfs_tables.h>
#include <fs/exfatfs/exfatfs_vfsops.h>

/* #define EXFATFS_VFSOPS_DEBUG */
#ifdef EXFATFS_VFSOPS_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

static int update_mp(struct mount *, struct exfatfs_args *);
static int exfatfs_gop_alloc(struct vnode *vp, off_t off,
			     off_t len, int flags, kauth_cred_t cred);
static void exfatfs_gop_markupdate(struct vnode *vp, int flags);
static int exfatfs_modcmd(modcmd_t cmd, void *arg);
static void exfatfs_reinit(void);
static void exfatfs_done(void);
static int exfatfs_newvnode(struct mount *mp, struct vnode *dvp,
			    struct vnode *vp, struct vattr *vap,
			    kauth_cred_t cred, void *extra,
			    size_t *key_len, const void **new_key);

MODULE(MODULE_CLASS_VFS, exfatfs, NULL);

MALLOC_JUSTDEFINE(M_EXFATFSMNT, "EXFATFS mount", "EXFAT FS mount structure");
MALLOC_JUSTDEFINE(M_EXFATFSBOOT, "EXFATFS boot", "EXFAT FS boot block");
MALLOC_JUSTDEFINE(M_EXFATFSFAT, "EXFATFS FAT", "EXFAT FS FAT table");
MALLOC_JUSTDEFINE(M_EXFATFSTMP, "EXFATFS temp", "EXFAT FS temp. structures");

static struct sysctllog *exfatfs_sysctl_log;
struct pool exfatfs_xfinode_pool;
struct pool exfatfs_bitmap_pool;
struct pool exfatfs_dirent_pool;
extern const struct vnodeopv_desc exfatfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const exfatfs_vnodeopv_descs[] = {
	&exfatfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops exfatfs_vfsops = {
	.vfs_name = MOUNT_EXFATFS,
	.vfs_min_mount_data = sizeof (struct exfatfs_args),
	.vfs_mount = exfatfs_mount,
	.vfs_start = exfatfs_start,
	.vfs_unmount = exfatfs_unmount,
	.vfs_root = exfatfs_root,
	.vfs_quotactl = (void *)eopnotsupp,
	.vfs_statvfs = exfatfs_statvfs,
	.vfs_sync = exfatfs_sync,
	.vfs_vget = exfatfs_vget,
	.vfs_loadvnode = exfatfs_loadvnode,
	.vfs_fhtovp = (void *)eopnotsupp,
	.vfs_vptofh = (void *)eopnotsupp,
	.vfs_init = exfatfs_init,
	.vfs_reinit = exfatfs_reinit,
	.vfs_done = exfatfs_done,
	.vfs_mountroot = exfatfs_mountroot,
	.vfs_snapshot = (void *)eopnotsupp,
	.vfs_extattrctl = vfs_stdextattrctl,
	.vfs_suspendctl = genfs_suspendctl,
	.vfs_renamelock_enter = genfs_renamelock_enter,
	.vfs_renamelock_exit = genfs_renamelock_exit,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = exfatfs_vnodeopv_descs,
	.vfs_newvnode = exfatfs_newvnode
};

const struct genfs_ops exfatfs_genfsops = {
        .gop_size = genfs_size,
        .gop_alloc = exfatfs_gop_alloc,
        .gop_write = genfs_gop_write,
        .gop_markupdate = exfatfs_gop_markupdate,
        .gop_putrange = genfs_gop_putrange,
};

static int
exfatfs_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = vfs_attach(&exfatfs_vfsops);
		if (error != 0)
			break;
		sysctl_createv(&exfatfs_sysctl_log, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "exfatfs",
			       SYSCTL_DESCR("EXFAT file system"),
			       NULL, 0, NULL, 0,
			       CTL_VFS, 35, CTL_EOL);
		/*
		 * XXX the "35" above could be dynamic, thereby eliminating one
		 * more instance of the "number to vfs" mapping problem, but
		 * "35" is the order as taken from sys/mount.h
		 */
		break;
	case MODULE_CMD_FINI:
		error = vfs_detach(&exfatfs_vfsops);
		if (error != 0)
			break;
		sysctl_teardown(&exfatfs_sysctl_log);
		break;
	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

static int
update_mp(struct mount *mp, struct exfatfs_args *argp)
{
	struct exfatfs_mount *xmp = MPTOXMP(mp);
	struct exfatfs *fs = xmp->xm_fs;
	
	xmp->xm_flags |= argp->flags & EXFATFSMNT_MNTOPT;
	mp->mnt_stat.f_namemax = EXFATFS_NAMEMAX;
	
	fs->xf_gid = argp->gid;
	fs->xf_uid = argp->uid;
	fs->xf_mask = argp->mask & ALLPERMS;
	fs->xf_dirmask = argp->dirmask & ALLPERMS;
	fs->xf_gmtoff = argp->gmtoff;

	return 0;
}

void
exfatfs_init(void)
{
  	pool_init(&exfatfs_xfinode_pool, sizeof(struct xfinode), 0, 0, 0,
	    "exfatfsinopl", &pool_allocator_nointr, IPL_NONE);
	pool_sethiwat(&exfatfs_xfinode_pool, 1);
	pool_sethiwat(&exfatfs_bitmap_pool, 1);
  	pool_init(&exfatfs_dirent_pool, sizeof(struct exfatfs_dirent), 0, 0, 0,
	    "exfatfsdirentpl", &pool_allocator_nointr, IPL_NONE);
	pool_sethiwat(&exfatfs_dirent_pool, 1);
}

static void exfatfs_reinit(void)
{
}

static void exfatfs_done(void)
{
}

int
exfatfs_mountroot(void)
{
	struct mount *mp;
	struct lwp *l = curlwp;	/* XXX */
	int error;
	struct exfatfs_args args;

	if (device_class(root_device) != DV_DISK)
		return (ENODEV);

	if ((error = vfs_rootmountalloc(MOUNT_EXFATFS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}

	args.flags = 0;
	args.uid = 0;
	args.gid = 0;
	args.mask = 0777;
	args.version = EXFATFSMNT_VERSION;
	args.dirmask = 0777;
	args.gmtoff = 0x80;

	if ((error = exfatfs_mountfs(rootvp, mp, l, &args)) != 0) {
		vfs_unbusy(mp);
		vfs_rele(mp);
		return (error);
	}

	if ((error = update_mp(mp, &args)) != 0) {
		(void)exfatfs_unmount(mp, 0);
		vfs_unbusy(mp);
		vfs_rele(mp);
		vrele(rootvp);
		return (error);
	}

	mountlist_append(mp);
	(void)exfatfs_statvfs(mp, &mp->mnt_stat);
	vfs_unbusy(mp);
	return (0);
}

/*
 * mp - path - addr in user space of mount point (ie /usr or whatever)
 * data - addr in user space of mount params including the name of the block
 * special file to treat as a filesystem.
 */
int
exfatfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	struct vnode *devvp;	  /* vnode for blk device to mount */
	struct exfatfs_args *args = data; /* holds data from mount request */
	/* exfatfs specific mount control block */
	struct exfatfs_mount *xmp = NULL;
	struct exfatfs *fs = NULL;
	int error, flags;
	mode_t accessmode;

	DPRINTF(("exfatfs_mount(%p, %p, ., .)\n", mp, path));
	
	if (args == NULL) {
		printf("mount_exfatfs: args == NULL\n");
		return EINVAL;
	}
	if (*data_len < sizeof *args) {
		printf("mount_exfatfs: *data_len = %zd < sizeof *args = %zd\n",
			*data_len, sizeof(*args));
		return EINVAL;
	}

	if (mp->mnt_flag & MNT_GETARGS) {
		xmp = MPTOXMP(mp);
		if (xmp == NULL)
			return EIO;
		fs = xmp->xm_fs;
		args->fspec = NULL;
		args->uid = fs->xf_uid;
		args->gid = fs->xf_gid;
		args->mask = fs->xf_mask;
		args->flags = xmp->xm_flags;
		args->version = EXFATFSMNT_VERSION;
		args->dirmask = fs->xf_dirmask;
		args->gmtoff = fs->xf_gmtoff;
		*data_len = sizeof *args;
		return 0;
	}

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		xmp = MPTOXMP(mp);
		fs = xmp->xm_fs;
		error = 0;
		if (!(xmp->xm_flags & EXFATFSMNT_RONLY) &&
		    (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = vflush(mp, NULLVP, flags);
		}
		if (!error && (mp->mnt_flag & MNT_RELOAD))
			/* not yet implemented */
			error = EOPNOTSUPP;
		if (error) {
			DPRINTF(("vflush %d", error));
			return (error);
		}
		if ((xmp->xm_flags & EXFATFSMNT_RONLY) &&
		    (mp->mnt_iflag & IMNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 *
			 * Permission to update a mount is checked higher, so
			 * here we presume updating the mount is okay (for
			 * example, as far as securelevel goes) which leaves us
			 * with the normal check.
			 */
			devvp = fs->xf_devvp;
			vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
			error = kauth_authorize_system(l->l_cred,
			    KAUTH_SYSTEM_MOUNT, KAUTH_REQ_SYSTEM_MOUNT_DEVICE,
			    mp, devvp, KAUTH_ARG(VREAD | VWRITE));
			VOP_UNLOCK(devvp);
			DPRINTF(("KAUTH_REQ_SYSTEM_MOUNT_DEVICE %d", error));
			if (error)
				return (error);

			xmp->xm_flags &= ~EXFATFSMNT_RONLY;
		}
		if (args->fspec == NULL) {
			printf("mount_exfatfs: args->fspec == NULL\n");
			return EINVAL;
		}
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	error = namei_simple_user(args->fspec,
				NSM_FOLLOW_NOEMULROOT, &devvp);
	if (error != 0) {
		DPRINTF(("namei %d", error));
		return (error);
	}

	if (devvp->v_type != VBLK) {
		DPRINTF(("not block"));
		vrele(devvp);
		return (ENOTBLK);
	}
	if (bdevsw_lookup(devvp->v_rdev) == NULL) {
		DPRINTF(("no block switch"));
		vrele(devvp);
		return (ENXIO);
	}
	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	accessmode = VREAD;
	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		accessmode |= VWRITE;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_DEVICE, mp, devvp, KAUTH_ARG(accessmode));
	VOP_UNLOCK(devvp);
	if (error) {
		DPRINTF(("KAUTH_REQ_SYSTEM_MOUNT_DEVICE %d", error));
		vrele(devvp);
		return (error);
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0) {
		int xflags;

		if (mp->mnt_flag & MNT_RDONLY)
			xflags = FREAD;
		else
			xflags = FREAD|FWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_OPEN(devvp, xflags, FSCRED);
		VOP_UNLOCK(devvp);
		if (error) {
			DPRINTF(("VOP_OPEN %d", error));
			goto fail;
		}
		error = exfatfs_mountfs(devvp, mp, l, args);
		if (error) {
			DPRINTF(("exfatfs_mountfs %d", error));
			vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
			(void) VOP_CLOSE(devvp, xflags, NOCRED);
			VOP_UNLOCK(devvp);
			goto fail;
		}
	} else {
		vrele(devvp);
		if (devvp != fs->xf_devvp) {
			DPRINTF(("devvp %p xmp %p", devvp, fs->xf_devvp));
			printf("mount_exfatfs: devvp=%p but fs->xf_devvp=%p",
				devvp, fs->xf_devvp);
			return (EINVAL);	/* needs translation */
		}
	}
	if ((error = update_mp(mp, args)) != 0) {
		exfatfs_unmount(mp, MNT_FORCE);
		DPRINTF(("update_mp %d", error));
		return error;
	}

	return set_statvfs_info(path, UIO_USERSPACE, args->fspec, UIO_USERSPACE,
	    mp->mnt_op->vfs_name, mp, l);

fail:
	vrele(devvp);
	return (error);
}

int
exfatfs_mountfs(struct vnode *devvp, struct mount *mp, struct lwp *l,
		struct exfatfs_args *argp)
{
	struct exfatfs_mount *xmp;
	uint64_t psize;
	unsigned secsize;
	int error;

	DPRINTF(("exfatfs_mountfs(%p, %p, %p, %p)\n",
		 devvp, mp, l, argp));

	/* Flush out any old buffers remaining from a previous use. */
	if ((error = vinvalbuf(devvp, V_SAVE, l->l_cred, l, 0, 0)) != 0)
		return (error);

	xmp = NULL;

	if ((error = getdisksize(devvp, &psize, &secsize)) != 0) {
		/* Use default values */
		secsize = DEV_BSIZE;
		psize = 0;
		error = 0;
	}
	xmp = malloc(sizeof(*xmp), M_EXFATFSMNT, M_WAITOK|M_ZERO);
	xmp->xm_mp = mp;

	if ((error = exfatfs_mountfs_shared(devvp, xmp, secsize, &xmp->xm_fs)) != 0)
		goto error_exit;

	mutex_init(&xmp->xm_fs->xf_lock, MUTEX_DEFAULT, IPL_NONE);
	return 0;

error_exit:
	if (xmp) {
		free(xmp, M_EXFATFSMNT);
		mp->mnt_data = NULL;
	}
	return (error);
}

int
exfatfs_finish_mountfs(struct exfatfs *fs)
{
	struct exfatfs_mount *xmp;
	struct mount *mp;
	vnode_t *devvp;

	DPRINTF(("exfatfs_finish_mountfs(%p)\n", fs));
	xmp = fs->xf_mp;
	DPRINTF(("  fs->xf_mp = %p\n", xmp));
	mp = xmp->xm_mp;
	DPRINTF(("  xmp->xm_mp = %p\n", mp));
	devvp = fs->xf_devvp;
	DPRINTF(("  fs->devvp = %p\n", devvp));

	/*
	 * Finish up.
	 */
	if (mp->mnt_flag & MNT_SYNCHRONOUS)
		xmp->xm_flags |= EXFATFSMNT_SYNC;
	if (mp->mnt_flag & MNT_RDONLY)
		xmp->xm_flags |= EXFATFSMNT_RONLY;

	mp->mnt_data = xmp;
	mp->mnt_stat.f_fsidx.__fsid_val[0] = (long)devvp->v_rdev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_EXFATFS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = EXFATFS_NAMEMAX;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_dev_bshift = DEV_BSHIFT;
	mp->mnt_fs_bshift = EXFATFS_LSHIFT(fs);

	spec_node_setmountedfs(devvp, mp);

	return 0;
}

int
exfatfs_start(struct mount *mp, int flags)
{

	return (0);
}

struct exfatfs_unmount_ctx {
	struct exfatfs *fs;
	unsigned flag;
#define CTX_ALL    0x0000
#define CTX_PARENT 0x0001
};

static bool
exfatfs_unmount_selector(void *cl, struct vnode *vp)
{
	struct exfatfs_unmount_ctx *c = cl;

	KASSERT(mutex_owned(vp->v_interlock));

	if (c->flag == CTX_ALL)
		return true;
	if (c->flag & CTX_PARENT)
		if (VTOXI(vp)->xi_parentvp != NULL)
			return true;
	return false;
}

/*
 * Unmount the filesystem described by mp.
 */
int
exfatfs_unmount(struct mount *mp, int mntflags)
{
	struct exfatfs_mount *xmp = MPTOXMP(mp);
	struct exfatfs *fs = xmp->xm_fs;
	struct vnode_iterator *marker;
	struct vnode *vp;
	struct xfinode *xip;
	struct exfatfs_unmount_ctx ctx;
	int error, flags;

	DPRINTF(("exfatfs_unmount(%p, 0x%x)\n", mp, mntflags));
	DPRINTF(("  bitmapvp = %p/%u/%u, refcount %d\n", fs->xf_bitmapvp,
	       VTOXI(fs->xf_bitmapvp)->xi_dirclust,
	       VTOXI(fs->xf_bitmapvp)->xi_diroffset,
		 (int)vrefcnt(fs->xf_bitmapvp)));
	DPRINTF(("  upcasevp = %p/%u/%u, refcount %d\n", fs->xf_upcasevp,
	       VTOXI(fs->xf_upcasevp)->xi_dirclust,
	       VTOXI(fs->xf_upcasevp)->xi_diroffset,
		 (int)vrefcnt(fs->xf_upcasevp)));
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	DPRINTF((" vrele parent...\n"));
	vfs_vnode_iterator_init(mp, &marker);
	ctx.flag = CTX_PARENT;
	while ((vp = vfs_vnode_iterator_next(marker,
					     exfatfs_unmount_selector, &ctx)) != NULL) {
		xip = VTOXI(vp);
		KASSERT(xip->xi_parentvp != NULL);
#ifdef EXFATFS_VFSOPS_DEBUG
		vprint("parent", xip->xi_parentvp);
#endif
		PUTPARENT(xip);
		vrele(vp);
	}
	vfs_vnode_iterator_destroy(marker);


	/*
	 * Flush all but the system vnodes.  Then rootvp should
	 * have only the single reference we gave it at mount.
	 */
	vflush(mp, NULLVP, flags);
	if (vrefcnt(fs->xf_rootvp) > 1)
		return EBUSY;
	
	DPRINTF((" vrele rootvp...\n"));
	vrele(fs->xf_rootvp);   fs->xf_rootvp = NULL;
	DPRINTF((" vrele upcasevp...\n"));
	vrele(fs->xf_upcasevp); fs->xf_upcasevp = NULL;
	DPRINTF((" vrele bitmapvp...\n"));
	vrele(fs->xf_bitmapvp); fs->xf_bitmapvp = NULL;

	DPRINTF((" vflush...\n"));
	if ((error = vflush(mp, NULLVP, flags)) != 0) {
		printf(" vflush returned %d\n", error);

		vfs_vnode_iterator_init(mp, &marker);
		ctx.flag = CTX_ALL;
		while ((vp = vfs_vnode_iterator_next(marker,
						     exfatfs_unmount_selector, &ctx)) != NULL) {
			vprint("not flushed", vp);
			vrele(vp);
		}
		vfs_vnode_iterator_destroy(marker);
		return error;
	}
	
	xmp = MPTOXMP(mp);
	DPRINTF((" spec_node_setmountedfs...\n"));
	if (fs->xf_devvp->v_type != VBAD)
		spec_node_setmountedfs(fs->xf_devvp, NULL);
	DPRINTF((" clear dirty and update free percent...\n"));
	fs->xf_VolumeFlags &= ~EXFATFS_VOLUME_DIRTY;
	fs->xf_PercentInUse = (fs->xf_ClusterCount - fs->xf_FreeClusterCount)
		/ fs->xf_ClusterCount;
	exfatfs_write_sb(fs, 0);
	DPRINTF((" lock devvp...\n"));
	vn_lock(fs->xf_devvp, LK_EXCLUSIVE | LK_RETRY);
	DPRINTF((" close devvp...\n"));
	(void) VOP_CLOSE(fs->xf_devvp,
	    xmp->xm_flags & EXFATFSMNT_RONLY ? FREAD : FREAD|FWRITE, NOCRED);
	DPRINTF((" vput devvp...\n"));
	vput(fs->xf_devvp);
	DPRINTF((" free upcase table...\n"));
	exfatfs_destroy_uctable(fs);
	DPRINTF((" free bitmap...\n"));
	exfatfs_bitmap_destroy(fs);
	DPRINTF((" destroy lock...\n"));
	mutex_destroy(&xmp->xm_fs->xf_lock);
	DPRINTF((" free xmp...\n"));
	free(xmp, M_EXFATFSMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	return 0;
}

int
exfatfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	struct exfatfs *fs = MPTOXMP(mp)->xm_fs;
	
	sbp->f_bsize = EXFATFS_LSIZE(fs);
	sbp->f_frsize = EXFATFS_LSIZE(fs);
	sbp->f_iosize = EXFATFS_LSIZE(fs);
	sbp->f_blocks = EXFATFS_C2L(fs, fs->xf_ClusterCount);
	sbp->f_bfree =  EXFATFS_C2L(fs, fs->xf_FreeClusterCount);
	sbp->f_bavail = EXFATFS_C2L(fs, fs->xf_FreeClusterCount);
	sbp->f_bresvd = 0;
	sbp->f_files = 0; /* XXX How can we know the number of files? */
	sbp->f_ffree = 0; /* Could compute from #files if we had that */
	sbp->f_favail = 0;
	sbp->f_fresvd = 0;
	copy_statvfs_info(sbp, mp);
	return (0);
}

int
exfatfs_root(struct mount *mp, int lktype, struct vnode **vpp)
{
	struct xfinode *xip, *tmpxip;
	struct exfatfs *fs = MPTOXMP(mp)->xm_fs;
	struct timespec now;
	uint32_t dosnow;
	struct buf *bp;
	uint32_t clust;
	int error;

	if (fs->xf_rootvp != NULL) {
		*vpp = fs->xf_rootvp;
		DPRINTF(("later root call with vrefcnt %d\n",
			 vrefcnt(*vpp)));
		vref(*vpp);
		vn_lock(*vpp, lktype);
		return 0;
	}

	xip = exfatfs_newxfinode(fs, ROOTDIRCLUST, ROOTDIRENTRY);

	/*
	 * This cannot be a real directory entry; it is
	 * the root directory.  Fill it in with bogus
	 * contents.
	 */
	getnanotime(&now);
	xip->xi_direntp[0] = exfatfs_newdirent();
	xip->xi_direntp[1] = exfatfs_newdirent();

	SET_DFE_FILE_ATTRIBUTES(xip, XD_FILEATTR_DIRECTORY);
	exfatfs_unix2dostime(&now, 0, &dosnow, NULL);
	SET_DFE_CREATE(xip, dosnow);
	SET_DFE_LAST_MODIFIED(xip, dosnow);
	SET_DFE_LAST_ACCESSED(xip, dosnow);
	SET_DFE_CREATE_UTCOFF(xip, 0x80);
	SET_DSE_FIRSTCLUSTER(xip, fs->xf_FirstClusterOfRootDirectory);
	SET_DSE_ALLOCPOSSIBLE(xip);
	CLR_DSE_NOFATCHAIN(xip);

	/*
	 * The root directory doesn't store its length anywhere.
	 * Walk its FAT to find out how long the file is.
	 */
	SET_DSE_DATALENGTH(xip, 0);
	clust = fs->xf_FirstClusterOfRootDirectory;
	while (clust != 0xFFFFFFFF) {
		/* Read the FAT to find the next cluster */
		bread(fs->xf_devvp, EXFATFS_FATBLK(fs, clust),
		      FATBSIZE(fs), 0, &bp);
		clust = ((uint32_t *)bp->b_data)
			[EXFATFS_FATOFF(clust)];
		brelse(bp, 0);
		SET_DSE_DATALENGTH(xip, GET_DSE_DATALENGTH(xip)
				   + EXFATFS_CSIZE(fs));
	}
	SET_DSE_VALIDDATALENGTH(xip, GET_DSE_DATALENGTH(xip));
	/* GETPARENT(xip, dvp); */ /* Root has no parent */
	
	/* Store xip where loadvnode can find it (via vcache_get) */
	mutex_enter(&fs->xf_lock);
	LIST_INSERT_HEAD(&fs->xf_newxip, xip, xi_newxip);
	mutex_exit(&fs->xf_lock);

	DPRINTF(("exfatfs_root fs=%p\n", MPTOXMP(mp)->xm_fs));
	error = vcache_get(mp, &xip->xi_key, sizeof(xip->xi_key), vpp);
	if (!error) {
		DPRINTF(("first root assignment with vrefcnt %d\n",
			 vrefcnt(*vpp)));
		vref(*vpp); /* Extra reference */
		fs->xf_rootvp = *vpp;
		vn_lock(*vpp, lktype);
	}

	/* Either way, our new xip should not be on the list. */
	mutex_enter(&fs->xf_lock);
	LIST_FOREACH(tmpxip, &fs->xf_newxip, xi_newxip) {
		if (tmpxip == xip) {
			printf("warning: xip still on list\n");
			LIST_REMOVE(xip, xi_newxip);
			break;
		}
	}
	mutex_exit(&fs->xf_lock);

	if (error)
		exfatfs_freexfinode(xip);
	
	return error;
}

struct exfatfs_sync_ctx {
	int waitfor;
};

static bool
exfatfs_sync_selector(void *cl, struct vnode *vp)
{
	struct exfatfs_sync_ctx *c = cl;
	struct xfinode *xip;

	KASSERT(mutex_owned(vp->v_interlock));

	xip = VTOXI(vp);
	if (c->waitfor == MNT_LAZY || vp->v_type == VNON ||
	    xip == NULL || (((xip->xi_flag &
	    (XI_ACCESS | XI_CREATE | XI_UPDATE | XI_MODIFIED)) == 0) &&
	     (LIST_EMPTY(&vp->v_dirtyblkhd) &&
	      (vp->v_iflag & VI_ONWORKLST) == 0)))
		return false;
	return true;
}

int
exfatfs_sync(struct mount *mp, int waitfor, kauth_cred_t cred)
{
	struct vnode *vp;
	struct vnode_iterator *marker;
	struct exfatfs_mount *xmp = MPTOXMP(mp);
	struct exfatfs *fs = xmp->xm_fs;
	int error, allerror = 0;
	struct exfatfs_sync_ctx ctx;

	/*
	 * Write back each (modified) xfinode.
	 */
	vfs_vnode_iterator_init(mp, &marker);
	ctx.waitfor = waitfor;
	while ((vp = vfs_vnode_iterator_next(marker, exfatfs_sync_selector,
	    &ctx)))
	{
		error = vn_lock(vp, LK_EXCLUSIVE);
		if (error) {
			vrele(vp);
			continue;
		}
		if ((error = VOP_FSYNC(vp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0)) != 0)
			allerror = error;
		vput(vp);
	}
	vfs_vnode_iterator_destroy(marker);

	/*
	 * Force stale file system control information to be flushed.
	 */
	vn_lock(fs->xf_devvp, LK_EXCLUSIVE | LK_RETRY);
	if ((error = VOP_FSYNC(fs->xf_devvp, cred,
	    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0)) != 0)
		allerror = error;
	VOP_UNLOCK(fs->xf_devvp);
	return (allerror);
}

int
exfatfs_vget(struct mount *mp, ino_t ino, int unused, struct vnode **vpp)
{
	return (EOPNOTSUPP);
}

extern int (**exfatfs_vnodeop_p)(void *);

static int
exfatfs_newvnode(struct mount *mp, struct vnode *dvp, struct vnode *vp,
		 struct vattr *vap, kauth_cred_t cred, void *extra,
		 size_t *key_len, const void **new_key)
{
        struct exfatfs *fs;
	struct xfinode *xip;

	DPRINTF(("newvnode(mp=%p dvp=%p vp=%p vap=%p . extra=%p len=%p keyp=%p)\n",
		 mp, dvp, vp, vap, extra, key_len, new_key));

        KASSERT(dvp != NULL || vap->va_fileid > 0);
        KASSERT(dvp == NULL || dvp->v_mount == mp);
        KASSERT(vap->va_type != VNON);
	KASSERT(extra != NULL);

	DPRINTF(("newvnode xmp=%p\n", MPTOXMP(mp)));
	DPRINTF(("newvnode fs=%p\n", MPTOXMP(mp)->xm_fs));
        fs = MPTOXMP(mp)->xm_fs;

	/* Copy mode from vap */
	vp->v_type = vap->va_type;

	/* If we are given xinode data, use it */
	if (extra) {
		DPRINTF(("         using given xip=%p\n", extra));
		vp->v_data = xip = extra;
		xip->xi_vnode = vp;
	} else {
		/* Set up new xfinode */
		DPRINTF(("         alloc xip\n"));
		xip = exfatfs_newxfinode(fs, INO2CLUST(vap->va_fileid),
					 INO2ENTRY(vap->va_fileid));
		DPRINTF(("         xip=%p\n", xip));
		vp->v_data = xip;
		xip->xi_vnode = vp;
		SET_DFE_CREATE(xip, DOS_EPOCH_DOS_FMT);
		SET_DFE_LAST_MODIFIED(xip, DOS_EPOCH_DOS_FMT);
		SET_DFE_LAST_ACCESSED(xip, DOS_EPOCH_DOS_FMT);
		GETPARENT(xip, dvp);
	}
	xip->xi_dirgen = 0; /* vap->va_gen; */

	vp->v_op = exfatfs_vnodeop_p;
	vp->v_tag = VT_EXFATFS;
	/* vp->v_mount = mp; */

        *key_len = sizeof(xip->xi_key);
	*new_key = &xip->xi_key;
	
	/* vprint("newvnode exit", vp); */ /* Can't print here */
	DPRINTF(("newvnode exit inum 0x%lx vp=%p vp->v_data=%p vp->v_data->xi_fs=%p\n",
		 (unsigned long)INUM(xip), vp, vp->v_data, VTOXI(vp)->xi_fs));
	
	return 0;
}
	
/*
 * Get the matching vnode, creating one if it does not already exist.
 */
int exfatfs_getnewvnode(struct exfatfs *fs, struct vnode *dvp,
			uint32_t clust, uint32_t off, unsigned type,
			void *xip, struct vnode **vpp) {
	struct vattr va;

	KASSERT(fs != NULL);
	/* KASSERT(dvp != NULL); */ /* when called from system_loadvnode */
	KASSERT(xip != NULL);
	KASSERT(vpp != NULL);
	
	DPRINTF(("getnewvnode(fs=%p dvp=%p clust=%u off=%u type=%u vpp=%p)\n",
		 fs, dvp, clust, off, type, vpp));

	va.va_type = type;
	va.va_fileid = CE2INO(fs, clust, off);
	return vcache_new(XMPTOMP(fs->xf_mp), dvp,
			  &va, NOCRED, xip, vpp);
}

/*
 * Load a file entry given the cluster number of the file entry
 * and byte offset into the cluster.
 */
int
exfatfs_loadvnode(struct mount *mp, struct vnode *vp,
    const void *key, size_t key_len, const void **new_key)
{
	extern int (**exfatfs_vnodeop_p)(void *);
	struct exfatfs_mount *xmp;
	struct exfatfs *fs;
	struct xfinode *xip;
	struct exfatfs_dirent_key dkey;
	int found = 0;

	KASSERT(key_len == sizeof(dkey));
	memcpy(&dkey, key, key_len);
	KASSERT(dkey.dk_dirgen == NULL);

	xmp = MPTOXMP(mp);
	fs = xmp->xm_fs;

	/* If we have an xip stored, use it */
	mutex_enter(&fs->xf_lock);
	LIST_FOREACH(xip, &fs->xf_newxip, xi_newxip) {
		if (memcmp(&xip->xi_key, &dkey, sizeof(dkey)) == 0) {
			LIST_REMOVE(xip, xi_newxip);
			++found;
			break;
		}
	}
	mutex_exit(&fs->xf_lock);
	assert(xip != NULL);
	assert(found);
#ifdef TRACE_INUM
	if (INUM(xip) == TRACE_INUM)
		printf("loadvnode found 0x%x with xip=%p gen %p\n",
		       TRACE_INUM, xip,	xip->xi_dirgen);
#endif /* TRACE_INUM */

	/*
	 * Fill in a few fields of the vnode and finish filling in the
	 * xfinode.
	 */
	vp->v_data = xip;
	xip->xi_vnode = vp;
	if (ISDIRECTORY(xip))
		vp->v_type = VDIR;
	else if (ISSYMLINK(xip))
		vp->v_type = VLNK;
	else
		vp->v_type = VREG;
	vref(xip->xi_devvp);
	if (dkey.dk_dirclust == ROOTDIRCLUST
	    && dkey.dk_diroffset == ROOTDIRENTRY)
		vp->v_vflag |= VV_ROOT;
	vp->v_tag = VT_EXFATFS;

	vp->v_op = exfatfs_vnodeop_p;
	if (vp->v_type == VREG)
		genfs_node_init(vp, &exfatfs_genfsops);
	uvm_vnp_setsize(vp, GET_DSE_VALIDDATALENGTH(VTOXI(vp)));
	*new_key = &VTOXI(vp)->xi_key;

	return 0;
}

int
exfatfs_gop_alloc(struct vnode *vp, off_t off,
		  off_t len, int flags, kauth_cred_t cred)
{
        return 0;
}

void
exfatfs_gop_markupdate(struct vnode *vp, int flags)
{
        u_long mask = 0;

        if ((flags & GOP_UPDATE_ACCESSED) != 0) {
                mask = XI_ACCESS;
        }
        if ((flags & GOP_UPDATE_MODIFIED) != 0) {
                mask |= XI_UPDATE;
        }
        if (mask) {
		VTOXI(vp)->xi_flag |= mask;
        }
}

