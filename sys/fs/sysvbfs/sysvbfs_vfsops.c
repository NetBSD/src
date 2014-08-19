/*	$NetBSD: sysvbfs_vfsops.c,v 1.41.2.1 2014/08/20 00:04:28 tls Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: sysvbfs_vfsops.c,v 1.41.2.1 2014/08/20 00:04:28 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/proc.h>

/* v-node */
#include <sys/namei.h>
#include <sys/vnode.h>
/* devsw */
#include <sys/conf.h>

#include <fs/sysvbfs/sysvbfs.h>	/* external interface */
#include <fs/sysvbfs/bfs.h>	/* internal interface */

#ifdef SYSVBFS_VNOPS_DEBUG
#define	DPRINTF(fmt, args...)	printf(fmt, ##args)
#else
#define	DPRINTF(arg...)		((void)0)
#endif

MALLOC_JUSTDEFINE(M_SYSVBFS_VFS, "sysvbfs vfs", "sysvbfs vfs structures");

struct pool sysvbfs_node_pool;

int sysvbfs_mountfs(struct vnode *, struct mount *, struct lwp *);

int
sysvbfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	struct sysvbfs_args *args = data;
	struct sysvbfs_mount *bmp = NULL;
	struct vnode *devvp = NULL;
	int error = 0;
	bool update;

	DPRINTF("%s: mnt_flag=%x\n", __func__, mp->mnt_flag);

	if (args == NULL)
		return EINVAL;
	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		if ((bmp = (struct sysvbfs_mount *)mp->mnt_data) == NULL)
			return EIO;
		args->fspec = NULL;
		*data_len = sizeof *args;
		return 0;
	}


	DPRINTF("%s: args->fspec=%s\n", __func__, args->fspec);
	update = mp->mnt_flag & MNT_UPDATE;
	if (args->fspec == NULL) {
		/* nothing to do. */
		return EINVAL;
	}

	if (args->fspec != NULL) {
		/* Look up the name and verify that it's sane. */
		error = namei_simple_user(args->fspec,
					NSM_FOLLOW_NOEMULROOT, &devvp);
		if (error != 0)
			return (error);

		if (!update) {
			/*
			 * Be sure this is a valid block device
			 */
			if (devvp->v_type != VBLK)
				error = ENOTBLK;
			else if (bdevsw_lookup(devvp->v_rdev) == NULL)
				error = ENXIO;
		} else {
			/*
			 * Be sure we're still naming the same device
			 * used for our initial mount
			 */
			bmp = (struct sysvbfs_mount *)mp->mnt_data;
			if (devvp != bmp->devvp)
				error = EINVAL;
		}
	}

	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 *
	 * Permission to update a mount is checked higher, so here we presume
	 * updating the mount is okay (for example, as far as securelevel goes)
	 * which leaves us with the normal check.
	 */
	if (error == 0) {
		int accessmode = VREAD;
		if (update ?
		    (mp->mnt_iflag & IMNT_WANTRDWR) != 0 :
		    (mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;

		error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_MOUNT,
		    KAUTH_REQ_SYSTEM_MOUNT_DEVICE, mp, devvp,
		    KAUTH_ARG(accessmode));
	}

	if (error) {
		vrele(devvp);
		return error;
	}

	if (!update) {
		if ((error = sysvbfs_mountfs(devvp, mp, l)) != 0) {
			vrele(devvp);
			return error;
		}
	} else 	if (mp->mnt_flag & MNT_RDONLY) {
		/* XXX: r/w -> read only */
	}

	return set_statvfs_info(path, UIO_USERSPACE, args->fspec, UIO_USERSPACE,
	    mp->mnt_op->vfs_name, mp, l);
}

int
sysvbfs_mountfs(struct vnode *devvp, struct mount *mp, struct lwp *l)
{
	kauth_cred_t cred = l->l_cred;
	struct sysvbfs_mount *bmp;
	int error, oflags;
	bool devopen = false;

	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, V_SAVE, cred, l, 0, 0);
	if (error)
		goto out;

	/* Open block device */
	oflags = FREAD;
	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		oflags |= FWRITE;
	if ((error = VOP_OPEN(devvp, oflags, NOCRED)) != 0)
		goto out;
	devopen = true;

	bmp = malloc(sizeof(*bmp), M_SYSVBFS_VFS, M_WAITOK | M_ZERO);
	bmp->devvp = devvp;
	bmp->mountp = mp;
	if ((error = sysvbfs_bfs_init(&bmp->bfs, devvp)) != 0) {
		free(bmp, M_SYSVBFS_VFS);
		goto out;
	}
	LIST_INIT(&bmp->bnode_head);

	mp->mnt_data = bmp;
	mp->mnt_stat.f_fsidx.__fsid_val[0] = (long)devvp->v_rdev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_SYSVBFS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = BFS_FILENAME_MAXLEN;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_dev_bshift = BFS_BSHIFT;
	mp->mnt_fs_bshift = BFS_BSHIFT;

 out:
	if (devopen && error)
		VOP_CLOSE(devvp, oflags, NOCRED);
	VOP_UNLOCK(devvp);
	return error;
}

int
sysvbfs_start(struct mount *mp, int flags)
{

	DPRINTF("%s:\n", __func__);
	/* Nothing to do. */
	return 0;
}

int
sysvbfs_unmount(struct mount *mp, int mntflags)
{
	struct sysvbfs_mount *bmp = (void *)mp->mnt_data;
	int error;

	DPRINTF("%s: %p\n", __func__, bmp);

	if ((error = vflush(mp, NULLVP,
	    mntflags & MNT_FORCE ? FORCECLOSE : 0)) != 0)
		return error;

	vn_lock(bmp->devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(bmp->devvp, FREAD, NOCRED);
	vput(bmp->devvp);

	sysvbfs_bfs_fini(bmp->bfs);

	free(bmp, M_SYSVBFS_VFS);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;

	return 0;
}

int
sysvbfs_root(struct mount *mp, struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	DPRINTF("%s:\n", __func__);
	if ((error = VFS_VGET(mp, BFS_ROOT_INODE, &vp)) != 0)
		return error;
	*vpp = vp;

	return 0;
}

int
sysvbfs_statvfs(struct mount *mp, struct statvfs *f)
{
	struct sysvbfs_mount *bmp = mp->mnt_data;
	struct bfs *bfs = bmp->bfs;
	int free_block;
	size_t data_block;

	data_block = (bfs->data_end - bfs->data_start) >> BFS_BSHIFT;
	if (bfs_inode_alloc(bfs, 0, 0, &free_block) != 0)
		free_block = 0;
	else
		free_block = (bfs->data_end >> BFS_BSHIFT) - free_block;

	DPRINTF("%s: %d %d %d\n", __func__, bfs->data_start,
	    bfs->data_end, free_block);

	f->f_bsize = BFS_BSIZE;
	f->f_frsize = BFS_BSIZE;
	f->f_iosize = BFS_BSIZE;
	f->f_blocks = data_block;
	f->f_bfree = free_block;
	f->f_bavail = f->f_bfree;
	f->f_bresvd = 0;
	f->f_files = bfs->max_inode;
	f->f_ffree = bfs->max_inode - bfs->n_inode;
	f->f_favail = f->f_ffree;
	f->f_fresvd = 0;
	copy_statvfs_info(f, mp);

	return 0;
}

int
sysvbfs_sync(struct mount *mp, int waitfor, kauth_cred_t cred)
{
	struct sysvbfs_mount *bmp = mp->mnt_data;
	struct sysvbfs_node *bnode;
	struct vnode *v;
	int err, error;

	DPRINTF("%s:\n", __func__);
	error = 0;
	mutex_enter(&mntvnode_lock);
	for (bnode = LIST_FIRST(&bmp->bnode_head); bnode != NULL;
	    bnode = LIST_NEXT(bnode, link)) {
		v = bnode->vnode;
	    	mutex_enter(v->v_interlock);
		mutex_exit(&mntvnode_lock);
		err = vget(v, LK_EXCLUSIVE | LK_NOWAIT);
		if (err == 0) {
			err = VOP_FSYNC(v, cred, FSYNC_WAIT, 0, 0);
			vput(v);
		}
		if (err != 0)
			error = err;
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);

	return error;
}

int
sysvbfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	struct sysvbfs_mount *bmp = mp->mnt_data;
	struct bfs *bfs = bmp->bfs;
	struct vnode *vp;
	struct sysvbfs_node *bnode;
	struct bfs_inode *inode;
	int error;

	DPRINTF("%s: i-node=%lld\n", __func__, (long long)ino);
	/* Lookup requested i-node */
	if (!bfs_inode_lookup(bfs, ino, &inode)) {
		DPRINTF("%s: bfs_inode_lookup failed.\n", __func__);
		return ENOENT;
	}

 retry:
	mutex_enter(&mntvnode_lock);
	for (bnode = LIST_FIRST(&bmp->bnode_head); bnode != NULL;
	    bnode = LIST_NEXT(bnode, link)) {
		if (bnode->inode->number == ino) {
			vp = bnode->vnode;
			mutex_enter(vp->v_interlock);
			mutex_exit(&mntvnode_lock);
			if (vget(vp, LK_EXCLUSIVE) == 0) {
				*vpp = vp;
				return 0;
			} else {
				goto retry;
			}
		}
	}
	mutex_exit(&mntvnode_lock);

	/* Allocate v-node. */
	error = getnewvnode(VT_SYSVBFS, mp, sysvbfs_vnodeop_p, NULL, &vp);
	if (error) {
		DPRINTF("%s: getnewvnode error.\n", __func__);
		return error;
	}
	/* Lock vnode here */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	/* Allocate i-node */
	vp->v_data = pool_get(&sysvbfs_node_pool, PR_WAITOK);
	memset(vp->v_data, 0, sizeof(struct sysvbfs_node));
	bnode = vp->v_data;
	mutex_enter(&mntvnode_lock);
	LIST_INSERT_HEAD(&bmp->bnode_head, bnode, link);
	mutex_exit(&mntvnode_lock);
	bnode->vnode = vp;
	bnode->bmp = bmp;
	bnode->inode = inode;
	bnode->lockf = NULL; /* advlock */

	if (ino == BFS_ROOT_INODE) {	/* BFS is flat filesystem */
		vp->v_type = VDIR;
		vp->v_vflag |= VV_ROOT;
	} else {
		vp->v_type = VREG;
	}

	genfs_node_init(vp, &sysvbfs_genfsops);
	uvm_vnp_setsize(vp, bfs_file_size(inode));
	*vpp = vp;

	return 0;
}

int
sysvbfs_fhtovp(struct mount *mp, struct fid *fid, struct vnode **vpp)
{

	DPRINTF("%s:\n", __func__);
	/* notyet */
	return EOPNOTSUPP;
}

int
sysvbfs_vptofh(struct vnode *vpp, struct fid *fid, size_t *fh_size)
{

	DPRINTF("%s:\n", __func__);
	/* notyet */
	return EOPNOTSUPP;
}

MALLOC_DECLARE(M_BFS);
MALLOC_DECLARE(M_SYSVBFS_VNODE);

void
sysvbfs_init(void)
{

	DPRINTF("%s:\n", __func__);
	malloc_type_attach(M_SYSVBFS_VFS);
	malloc_type_attach(M_BFS);
	malloc_type_attach(M_SYSVBFS_VNODE);
	pool_init(&sysvbfs_node_pool, sizeof(struct sysvbfs_node), 0, 0, 0,
	    "sysvbfs_node_pool", &pool_allocator_nointr, IPL_NONE);
}

void
sysvbfs_reinit(void)
{

	/* Nothing to do. */
	DPRINTF("%s:\n", __func__);
}

void
sysvbfs_done(void)
{

	DPRINTF("%s:\n", __func__);
	pool_destroy(&sysvbfs_node_pool);
	malloc_type_detach(M_BFS);
	malloc_type_detach(M_SYSVBFS_VFS);
	malloc_type_detach(M_SYSVBFS_VNODE);
}

int
sysvbfs_gop_alloc(struct vnode *vp, off_t off, off_t len, int flags,
    kauth_cred_t cred)
{

	return 0;
}
