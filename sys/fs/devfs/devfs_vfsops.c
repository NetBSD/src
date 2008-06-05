/* 	$NetBSD: devfs_vfsops.c,v 1.1.14.5 2008/06/05 19:14:36 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

/*
 * Copyright (c) 2005, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Efficient memory file system.
 *
 * devfs is a file system that uses NetBSD's virtual memory sub-system
 * (the well-known UVM) to store file data and metadata in an efficient
 * way.  This means that it does not follow the structure of an on-disk
 * file system because it simply does not need to.  Instead, it uses
 * memory-specific data structures and algorithms to automatically
 * allocate and release resources.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devfs_vfsops.c,v 1.1.14.5 2008/06/05 19:14:36 mjf Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/namei.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>
#include <fs/devfs/devfs.h>
#include <dev/devfsctl/devfsctl.h>

/* --------------------------------------------------------------------- */

static int	devfs_mount(struct mount *, const char *, void *, size_t *);
static int	devfs_start(struct mount *, int);
static int	devfs_unmount(struct mount *, int);
static int	devfs_root(struct mount *, struct vnode **);
static int	devfs_vget(struct mount *, ino_t, struct vnode **);
static int	devfs_fhtovp(struct mount *, struct fid *, struct vnode **);
static int	devfs_vptofh(struct vnode *, struct fid *, size_t *);
static int	devfs_statvfs(struct mount *, struct statvfs *);
static int	devfs_sync(struct mount *, int, kauth_cred_t);
static void	devfs_init(void);
static void	devfs_done(void);
static int	devfs_snapshot(struct mount *, struct vnode *,
		    struct timespec *);

/* --------------------------------------------------------------------- */

static int
devfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	int error;
	ino_t nodes;
	size_t pages;
	struct devfs_mount *tmp;
	struct devfs_node *root;
	struct devfs_args *args = data;

	if (*data_len < sizeof *args)
		return EINVAL;

	/* Handle retrieval of mount point arguments. */
	if (mp->mnt_flag & MNT_GETARGS) {
		if (mp->mnt_data == NULL)
			return EIO;
		tmp = VFS_TO_DEVFS(mp);

		args->ta_version = DEVFS_ARGS_VERSION;
		args->ta_size_max = tmp->tm_pages_max * PAGE_SIZE;

		root = tmp->tm_root;
		args->ta_root_uid = root->tn_uid;
		args->ta_root_gid = root->tn_gid;
		args->ta_root_mode = root->tn_mode;

		args->ta_visible = root->tn_visible;

		*data_len = sizeof *args;
		return 0;
	}

	if (args->ta_version != DEVFS_ARGS_VERSION)
		return EINVAL;

	/* Do not allow mounts if we do not have enough memory to preserve
	 * the minimum reserved pages. */
	if (devfs_mem_info(true) < DEVFS_PAGES_RESERVED)
		return EINVAL;

	/*
	 * TODO:
	 * Ensure that if a devfs mount already exists on this
	 * path, we do not try to mount another.
	 * 
	 * Check vfs_lookup.c:777
	 */

	/* Get the maximum number of memory pages this file system is
	 * allowed to use, based on the maximum size the user passed in
	 * the mount structure.  A value of zero is treated as if the
	 * maximum available space was requested. */
	if (args->ta_size_max < PAGE_SIZE || args->ta_size_max >= SIZE_MAX)
		pages = SIZE_MAX;
	else
		pages = args->ta_size_max / PAGE_SIZE +
		    (args->ta_size_max % PAGE_SIZE == 0 ? 0 : 1);
	KASSERT(pages > 0);

	nodes = 3 + pages * PAGE_SIZE / 1024;
	KASSERT(nodes >= 3);

	/* Allocate the devfs mount structure and fill it. */
	tmp = kmem_alloc(sizeof(struct devfs_mount), KM_SLEEP);
	if (tmp == NULL)
		return ENOMEM;

	tmp->tm_nodes_max = nodes;
	tmp->tm_nodes_cnt = 0;
	LIST_INIT(&tmp->tm_nodes);

	mutex_init(&tmp->tm_lock, MUTEX_DEFAULT, IPL_NONE);

	tmp->tm_pages_max = pages;
	tmp->tm_pages_used = 0;
	devfs_pool_init(&tmp->tm_dirent_pool, sizeof(struct devfs_dirent),
	    "dirent", tmp);
	devfs_pool_init(&tmp->tm_node_pool, sizeof(struct devfs_node),
	    "node", tmp);
	devfs_str_pool_init(&tmp->tm_str_pool, tmp);

	tmp->tm_visible = args->ta_visible;

	/* Allocate the root node. */
	error = devfs_alloc_node(tmp, VDIR, args->ta_root_uid,
	    args->ta_root_gid, args->ta_root_mode & ALLPERMS, NULL, NULL,
	    VNOVAL, &root, tmp->tm_visible);
	KASSERT(error == 0 && root != NULL);
	root->tn_links++;
	tmp->tm_root = root;

	tmp->tm_init = args->ta_init;

	mp->mnt_data = tmp;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_fs_bshift = PAGE_SHIFT;
	mp->mnt_dev_bshift = DEV_BSHIFT;
	mp->mnt_iflag |= IMNT_MPSAFE;
	vfs_getnewfsid(mp);

	/*
	 * Create a console device and devfsctl(4) device special file
	 * in this new devfs.
	 */
	error = devfs_init_nodes(tmp, mp, tmp->tm_init);
	if (error != 0)
		return error;

	error = devfsctl_mount_msg(path, 
	    mp->mnt_stat.f_fsidx.__fsid_val[0], tmp->tm_visible);
	if (error != 0)
		return error;

	/* We've been called from within the kernel */
	if (tmp->tm_init)
		return set_statvfs_info(path, UIO_SYSSPACE, "devfs", 
		    UIO_SYSSPACE, mp->mnt_op->vfs_name, mp, l);

	return set_statvfs_info(path, UIO_USERSPACE, "devfs", 
	    UIO_SYSSPACE, mp->mnt_op->vfs_name, mp, l);
}

/* --------------------------------------------------------------------- */

static int
devfs_start(struct mount *mp, int flags)
{

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED2 */
static int
devfs_unmount(struct mount *mp, int mntflags)
{
	int error;
	int flags = 0;
	struct devfs_mount *tmp;
	struct devfs_node *node;

	/* Handle forced unmounts. */
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	tmp = VFS_TO_DEVFS(mp);

	/* 
	 * If this file system was mounted by init(8) the root vnode
	 * has an extra reference to it. Release this reference now.
	 */
	if (tmp->tm_init > 0) {
		if (!(mntflags & MNT_FORCE))
			return EBUSY;
		else
			vref(tmp->tm_root->tn_vnode);
	}

	error = devfsctl_unmount_msg(mp->mnt_stat.f_fsidx.__fsid_val[0],
	    tmp->tm_visible);

	/* I have no idea what to do here */
	if (error != 0)
		error = 0;

	/* Finalize all pending I/O. */
	error = vflush(mp, NULL, flags);
	if (error != 0)
		return error;

	/* Free all associated data.  The loop iterates over the linked list
	 * we have containing all used nodes.  For each of them that is
	 * a directory, we free all its directory entries.  Note that after
	 * freeing a node, it will automatically go to the available list,
	 * so we will later have to iterate over it to release its items. */
	node = LIST_FIRST(&tmp->tm_nodes);
	while (node != NULL) {
		struct devfs_node *next;

		if (node->tn_type == VDIR) {
			struct devfs_dirent *de;

			de = TAILQ_FIRST(&node->tn_spec.tn_dir.tn_dir);
			while (de != NULL) {
				struct devfs_dirent *nde;

				nde = TAILQ_NEXT(de, td_entries);
				devfs_free_dirent(tmp, de, false);
				de = nde;
				node->tn_size -= sizeof(struct devfs_dirent);
			}
		}

		next = LIST_NEXT(node, tn_entries);
		devfs_free_node(tmp, node);
		node = next;
	}

	devfs_pool_destroy(&tmp->tm_dirent_pool);
	devfs_pool_destroy(&tmp->tm_node_pool);
	devfs_str_pool_destroy(&tmp->tm_str_pool);

	KASSERT(tmp->tm_pages_used == 0);

	/* Throw away the devfs_mount structure. */
	mutex_destroy(&tmp->tm_lock);
	kmem_free(tmp, sizeof(*tmp));
	mp->mnt_data = NULL;

	return 0;
}

/* --------------------------------------------------------------------- */

static int
devfs_root(struct mount *mp, struct vnode **vpp)
{

	return devfs_alloc_vp(mp, VFS_TO_DEVFS(mp)->tm_root, vpp);
}

/* --------------------------------------------------------------------- */

static int
devfs_vget(struct mount *mp, ino_t ino,
    struct vnode **vpp) 
{

	printf("devfs_vget called; need for it unknown yet\n");
	return EOPNOTSUPP;
}

/* --------------------------------------------------------------------- */

static int
devfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	bool found;
	struct devfs_fid tfh;
	struct devfs_mount *tmp;
	struct devfs_node *node;

	tmp = VFS_TO_DEVFS(mp);

	if (fhp->fid_len != sizeof(struct devfs_fid))
		return EINVAL;

	memcpy(&tfh, fhp, sizeof(struct devfs_fid));

	if (tfh.tf_id >= tmp->tm_nodes_max)
		return EINVAL;

	found = false;
	mutex_enter(&tmp->tm_lock);
	LIST_FOREACH(node, &tmp->tm_nodes, tn_entries) {
		if (node->tn_id == tfh.tf_id &&
		    node->tn_gen == tfh.tf_gen) {
			found = true;
			break;
		}
	}
	mutex_exit(&tmp->tm_lock);

	/* XXXAD nothing to prevent 'node' from being removed. */
	return found ? devfs_alloc_vp(mp, node, vpp) : EINVAL;
}

/* --------------------------------------------------------------------- */

static int
devfs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct devfs_fid tfh;
	struct devfs_node *node;

	if (*fh_size < sizeof(struct devfs_fid)) {
		*fh_size = sizeof(struct devfs_fid);
		return E2BIG;
	}
	*fh_size = sizeof(struct devfs_fid);
	node = VP_TO_DEVFS_NODE(vp);

	memset(&tfh, 0, sizeof(tfh));
	tfh.tf_len = sizeof(struct devfs_fid);
	tfh.tf_gen = node->tn_gen;
	tfh.tf_id = node->tn_id;
	memcpy(fhp, &tfh, sizeof(tfh));

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED2 */
static int
devfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	fsfilcnt_t freenodes;
	struct devfs_mount *tmp;

	tmp = VFS_TO_DEVFS(mp);

	sbp->f_iosize = sbp->f_frsize = sbp->f_bsize = PAGE_SIZE;

	sbp->f_blocks = DEVFS_PAGES_MAX(tmp);
	sbp->f_bavail = sbp->f_bfree = DEVFS_PAGES_AVAIL(tmp);
	sbp->f_bresvd = 0;

	freenodes = MIN(tmp->tm_nodes_max - tmp->tm_nodes_cnt,
	    DEVFS_PAGES_AVAIL(tmp) * PAGE_SIZE / sizeof(struct devfs_node));

	sbp->f_files = tmp->tm_nodes_cnt + freenodes;
	sbp->f_favail = sbp->f_ffree = freenodes;
	sbp->f_fresvd = 0;

	copy_statvfs_info(sbp, mp);

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED0 */
static int
devfs_sync(struct mount *mp, int waitfor,
    kauth_cred_t uc)
{

	return 0;
}

/* --------------------------------------------------------------------- */

static void
devfs_init(void)
{

}

/* --------------------------------------------------------------------- */

static void
devfs_done(void)
{

}

/* --------------------------------------------------------------------- */

static int
devfs_snapshot(struct mount *mp, struct vnode *vp,
    struct timespec *ctime)
{

	return EOPNOTSUPP;
}

/* --------------------------------------------------------------------- */

/*
 * devfs vfs operations.
 */

extern const struct vnodeopv_desc devfs_fifoop_opv_desc;
extern const struct vnodeopv_desc devfs_specop_opv_desc;
extern const struct vnodeopv_desc devfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const devfs_vnodeopv_descs[] = {
	&devfs_fifoop_opv_desc,
	&devfs_specop_opv_desc,
	&devfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops devfs_vfsops = {
	MOUNT_DEVFS,			/* vfs_name */
	sizeof (struct devfs_args),
	devfs_mount,			/* vfs_mount */
	devfs_start,			/* vfs_start */
	devfs_unmount,			/* vfs_unmount */
	devfs_root,			/* vfs_root */
	(void *)eopnotsupp,		/* vfs_quotactl */
	devfs_statvfs,			/* vfs_statvfs */
	devfs_sync,			/* vfs_sync */
	devfs_vget,			/* vfs_vget */
	devfs_fhtovp,			/* vfs_fhtovp */
	devfs_vptofh,			/* vfs_vptofh */
	devfs_init,			/* vfs_init */
	NULL,				/* vfs_reinit */
	devfs_done,			/* vfs_done */
	NULL,				/* vfs_mountroot */
	devfs_snapshot,			/* vfs_snapshot */
	vfs_stdextattrctl,		/* vfs_extattrctl */
	(void *)eopnotsupp,		/* vfs_suspendctl */
	genfs_renamelock_enter,
	genfs_renamelock_exit,
	(void *)eopnotsupp,
	devfs_vnodeopv_descs,
	0,				/* vfs_refcount */
	{ NULL, NULL },
};

int
devfs_kernel_mount(void)
{
	int error;
	struct devfs_args dargs;
	struct mount *mp;
	struct nameidata nd;
	size_t len = sizeof(dargs);

	dargs.ta_init = dargs.ta_visible = 1;
	dargs.ta_version = DEVFS_ARGS_VERSION;

	NDINIT(&nd, LOOKUP, LOCKPARENT, UIO_SYSSPACE, "/dev");

	if ((error = namei(&nd)) != 0)
		goto out;

	if (nd.ni_vp == NULL) {
		error = ENOENT;
		goto out;
	}

	if ((mp = kmem_zalloc(sizeof(*mp), KM_NOSLEEP)) == NULL)
		panic("could not alloc memory for devfs mount");

	mp->mnt_op = &devfs_vfsops;
	mp->mnt_refcnt = 1;

	TAILQ_INIT(&mp->mnt_vnodelist);
	rw_init(&mp->mnt_unmounting);
	mutex_init(&mp->mnt_renamelock, 
	    MUTEX_DEFAULT, IPL_NONE);
	(void)vfs_busy(mp, NULL);

	mp->mnt_vnodecovered = nd.ni_vp;
	mp->mnt_stat.f_owner = kauth_cred_geteuid(curlwp->l_cred);
	mount_initspecific(mp);

	devfs_vfsops.vfs_refcount++;
	error = (devfs_vfsops.vfs_mount)(mp, "/dev", &dargs, &len);
	devfs_vfsops.vfs_refcount--;
	if (error != 0) {
		nd.ni_vp->v_mountedhere = NULL;
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp, false, NULL);
		vfs_destroy(mp);
		goto out;
	}

	VOP_UNLOCK(rootvnode, 0);
	mp->mnt_flag &= ~MNT_OP_FLAGS;

	/*
	 * Put the new filesystem on the mount list after root.
	 */
	cache_purge(nd.ni_vp);

	mp->mnt_iflag &= ~IMNT_WANTRDWR;
	mutex_enter(&mountlist_lock);
	nd.ni_vp->v_mountedhere = mp;
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);
	if ((mp->mnt_flag & (MNT_RDONLY | MNT_ASYNC)) == 0)
		error = vfs_allocate_syncvnode(mp);
	vfs_unbusy(mp, false, NULL);
	(void) VFS_STATVFS(mp, &mp->mnt_stat);
	error = VFS_START(mp, 0);
	if (error) {
		vrele(nd.ni_vp);
		vfs_destroy(mp);
	}
out:
	return error;
}
