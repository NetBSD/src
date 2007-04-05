/*	$NetBSD: tmpfs_vfsops.c,v 1.20.4.1 2007/04/05 21:57:49 ad Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
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
 * tmpfs is a file system that uses NetBSD's virtual memory sub-system
 * (the well-known UVM) to store file data and metadata in an efficient
 * way.  This means that it does not follow the structure of an on-disk
 * file system because it simply does not need to.  Instead, it uses
 * memory-specific data structures and algorithms to automatically
 * allocate and release resources.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_vfsops.c,v 1.20.4.1 2007/04/05 21:57:49 ad Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/proc.h>

#include <fs/tmpfs/tmpfs.h>

MALLOC_DEFINE(M_TMPFSMNT, "tmpfs mount", "tmpfs mount structures");

/* --------------------------------------------------------------------- */

static int	tmpfs_mount(struct mount *, const char *, void *,
		    struct nameidata *, struct lwp *);
static int	tmpfs_start(struct mount *, int, struct lwp *);
static int	tmpfs_unmount(struct mount *, int, struct lwp *);
static int	tmpfs_root(struct mount *, struct vnode **);
static int	tmpfs_quotactl(struct mount *, int, uid_t, void *,
		    struct lwp *);
static int	tmpfs_vget(struct mount *, ino_t, struct vnode **);
static int	tmpfs_fhtovp(struct mount *, struct fid *, struct vnode **);
static int	tmpfs_vptofh(struct vnode *, struct fid *, size_t *);
static int	tmpfs_statvfs(struct mount *, struct statvfs *, struct lwp *);
static int	tmpfs_sync(struct mount *, int, kauth_cred_t, struct lwp *);
static void	tmpfs_init(void);
static void	tmpfs_done(void);
static int	tmpfs_snapshot(struct mount *, struct vnode *,
		    struct timespec *);

/* --------------------------------------------------------------------- */

static int
tmpfs_mount(struct mount *mp, const char *path, void *data,
    struct nameidata *ndp, struct lwp *l)
{
	int error;
	ino_t nodes;
	size_t pages;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *root;
	struct tmpfs_args args;

	/* Handle retrieval of mount point arguments. */
	if (mp->mnt_flag & MNT_GETARGS) {
		if (mp->mnt_data == NULL)
			return EIO;
		tmp = VFS_TO_TMPFS(mp);

		args.ta_version = TMPFS_ARGS_VERSION;
		args.ta_nodes_max = tmp->tm_nodes_max;
		args.ta_size_max = tmp->tm_pages_max * PAGE_SIZE;

		root = tmp->tm_root;
		args.ta_root_uid = root->tn_uid;
		args.ta_root_gid = root->tn_gid;
		args.ta_root_mode = root->tn_mode;

		return copyout(&args, data, sizeof(args));
	}

	/* Verify that we have parameters, as they are required. */
	if (data == NULL)
		return EINVAL;

	/* Get the mount parameters. */
	error = copyin(data, &args, sizeof(args));
	if (error != 0)
		return error;

	if (mp->mnt_flag & MNT_UPDATE) {
		/* XXX: There is no support yet to update file system
		 * settings.  Should be added. */

		return EOPNOTSUPP;
	}

	if (args.ta_version != TMPFS_ARGS_VERSION)
		return EINVAL;

	/* Do not allow mounts if we do not have enough memory to preserve
	 * the minimum reserved pages. */
	if (tmpfs_mem_info(true) < TMPFS_PAGES_RESERVED)
		return EINVAL;

	/* Get the maximum number of memory pages this file system is
	 * allowed to use, based on the maximum size the user passed in
	 * the mount structure.  A value of zero is treated as if the
	 * maximum available space was requested. */
	if (args.ta_size_max < PAGE_SIZE || args.ta_size_max >= SIZE_MAX)
		pages = SIZE_MAX;
	else
		pages = args.ta_size_max / PAGE_SIZE +
		    (args.ta_size_max % PAGE_SIZE == 0 ? 0 : 1);
	KASSERT(pages > 0);

	if (args.ta_nodes_max <= 3)
		nodes = 3 + pages * PAGE_SIZE / 1024;
	else
		nodes = args.ta_nodes_max;
	KASSERT(nodes >= 3);

	/* Allocate the tmpfs mount structure and fill it. */
	tmp = (struct tmpfs_mount *)malloc(sizeof(struct tmpfs_mount),
	    M_TMPFSMNT, M_WAITOK);
	KASSERT(tmp != NULL);

	tmp->tm_nodes_max = nodes;
	tmp->tm_nodes_last = 2;
	LIST_INIT(&tmp->tm_nodes_used);
	LIST_INIT(&tmp->tm_nodes_avail);

	tmp->tm_pages_max = pages;
	tmp->tm_pages_used = 0;
	tmpfs_pool_init(&tmp->tm_dirent_pool, sizeof(struct tmpfs_dirent),
	    "dirent", tmp);
	tmpfs_pool_init(&tmp->tm_node_pool, sizeof(struct tmpfs_node),
	    "node", tmp);
	tmpfs_str_pool_init(&tmp->tm_str_pool, tmp);

	/* Allocate the root node. */
	error = tmpfs_alloc_node(tmp, VDIR, args.ta_root_uid,
	    args.ta_root_gid, args.ta_root_mode & ALLPERMS, NULL, NULL,
	    VNOVAL, l->l_proc, &root);
	KASSERT(error == 0 && root != NULL);
	tmp->tm_root = root;

	mp->mnt_data = tmp;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	vfs_getnewfsid(mp);

	return set_statvfs_info(path, UIO_USERSPACE, "tmpfs", UIO_SYSSPACE,
	    mp, l);
}

/* --------------------------------------------------------------------- */

static int
tmpfs_start(struct mount *mp, int flags,
    struct lwp *l)
{

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED2 */
static int
tmpfs_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	int error;
	int flags = 0;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *node;

	/* Handle forced unmounts. */
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	/* Finalize all pending I/O. */
	error = vflush(mp, NULL, flags);
	if (error != 0)
		return error;

	tmp = VFS_TO_TMPFS(mp);

	/* Free all associated data.  The loop iterates over the linked list
	 * we have containing all used nodes.  For each of them that is
	 * a directory, we free all its directory entries.  Note that after
	 * freeing a node, it will automatically go to the available list,
	 * so we will later have to iterate over it to release its items. */
	node = LIST_FIRST(&tmp->tm_nodes_used);
	while (node != NULL) {
		struct tmpfs_node *next;

		if (node->tn_type == VDIR) {
			struct tmpfs_dirent *de;

			de = TAILQ_FIRST(&node->tn_spec.tn_dir.tn_dir);
			while (de != NULL) {
				struct tmpfs_dirent *nde;

				nde = TAILQ_NEXT(de, td_entries);
				KASSERT(de->td_node->tn_vnode == NULL);
				tmpfs_free_dirent(tmp, de, false);
				de = nde;
				node->tn_size -= sizeof(struct tmpfs_dirent);
			}
		}

		next = LIST_NEXT(node, tn_entries);
		tmpfs_free_node(tmp, node);
		node = next;
	}
	node = LIST_FIRST(&tmp->tm_nodes_avail);
	while (node != NULL) {
		struct tmpfs_node *next;

		next = LIST_NEXT(node, tn_entries);
		LIST_REMOVE(node, tn_entries);
		TMPFS_POOL_PUT(&tmp->tm_node_pool, node);
		node = next;
	}

	tmpfs_pool_destroy(&tmp->tm_dirent_pool);
	tmpfs_pool_destroy(&tmp->tm_node_pool);
	tmpfs_str_pool_destroy(&tmp->tm_str_pool);

	KASSERT(tmp->tm_pages_used == 0);

	/* Throw away the tmpfs_mount structure. */
	free(mp->mnt_data, M_TMPFSMNT);
	mp->mnt_data = NULL;

	return 0;
}

/* --------------------------------------------------------------------- */

static int
tmpfs_root(struct mount *mp, struct vnode **vpp)
{

	return tmpfs_alloc_vp(mp, VFS_TO_TMPFS(mp)->tm_root, vpp);
}

/* --------------------------------------------------------------------- */

static int
tmpfs_quotactl(struct mount *mp, int cmd, uid_t uid,
    void *arg, struct lwp *l)
{

	printf("tmpfs_quotactl called; need for it unknown yet\n");
	return EOPNOTSUPP;
}

/* --------------------------------------------------------------------- */

static int
tmpfs_vget(struct mount *mp, ino_t ino,
    struct vnode **vpp) 
{

	printf("tmpfs_vget called; need for it unknown yet\n");
	return EOPNOTSUPP;
}

/* --------------------------------------------------------------------- */

static int
tmpfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	bool found;
	struct tmpfs_fid tfh;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *node;

	tmp = VFS_TO_TMPFS(mp);

	if (fhp->fid_len != sizeof(struct tmpfs_fid))
		return EINVAL;

	memcpy(&tfh, fhp, sizeof(struct tmpfs_fid));

	if (tfh.tf_id >= tmp->tm_nodes_max)
		return EINVAL;

	found = false;
	LIST_FOREACH(node, &tmp->tm_nodes_used, tn_entries) {
		if (node->tn_id == tfh.tf_id &&
		    node->tn_gen == tfh.tf_gen) {
			found = true;
			break;
		}
	}

	return found ? tmpfs_alloc_vp(mp, node, vpp) : EINVAL;
}

/* --------------------------------------------------------------------- */

static int
tmpfs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct tmpfs_fid tfh;
	struct tmpfs_node *node;

	if (*fh_size < sizeof(struct tmpfs_fid)) {
		*fh_size = sizeof(struct tmpfs_fid);
		return E2BIG;
	}
	*fh_size = sizeof(struct tmpfs_fid);
	node = VP_TO_TMPFS_NODE(vp);

	memset(&tfh, 0, sizeof(tfh));
	tfh.tf_len = sizeof(struct tmpfs_fid);
	tfh.tf_gen = node->tn_gen;
	tfh.tf_id = node->tn_id;
	memcpy(fhp, &tfh, sizeof(tfh));

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED2 */
static int
tmpfs_statvfs(struct mount *mp, struct statvfs *sbp, struct lwp *l)
{
	fsfilcnt_t freenodes, usednodes;
	struct tmpfs_mount *tmp;
	struct tmpfs_node *dummy;

	tmp = VFS_TO_TMPFS(mp);

	sbp->f_iosize = sbp->f_frsize = sbp->f_bsize = PAGE_SIZE;

	sbp->f_blocks = TMPFS_PAGES_MAX(tmp);
	sbp->f_bavail = sbp->f_bfree = TMPFS_PAGES_AVAIL(tmp);
	sbp->f_bresvd = 0;

	freenodes = MIN(tmp->tm_nodes_max - tmp->tm_nodes_last,
	    TMPFS_PAGES_AVAIL(tmp) * PAGE_SIZE / sizeof(struct tmpfs_node));
	LIST_FOREACH(dummy, &tmp->tm_nodes_avail, tn_entries)
		freenodes++;

	usednodes = 0;
	LIST_FOREACH(dummy, &tmp->tm_nodes_used, tn_entries)
		usednodes++;

	sbp->f_files = freenodes + usednodes;
	sbp->f_favail = sbp->f_ffree = freenodes;
	sbp->f_fresvd = 0;

	copy_statvfs_info(sbp, mp);

	return 0;
}

/* --------------------------------------------------------------------- */

/* ARGSUSED0 */
static int
tmpfs_sync(struct mount *mp, int waitfor,
    kauth_cred_t uc, struct lwp *l)
{

	return 0;
}

/* --------------------------------------------------------------------- */

static void
tmpfs_init(void)
{

#ifdef _LKM
	malloc_type_attach(M_TMPFSMNT);
#endif
}

/* --------------------------------------------------------------------- */

static void
tmpfs_done(void)
{

#ifdef _LKM
	malloc_type_detach(M_TMPFSMNT);
#endif
}

/* --------------------------------------------------------------------- */

static int
tmpfs_snapshot(struct mount *mp, struct vnode *vp,
    struct timespec *ctime)
{

	return EOPNOTSUPP;
}

/* --------------------------------------------------------------------- */

/*
 * tmpfs vfs operations.
 */

extern const struct vnodeopv_desc tmpfs_fifoop_opv_desc;
extern const struct vnodeopv_desc tmpfs_specop_opv_desc;
extern const struct vnodeopv_desc tmpfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const tmpfs_vnodeopv_descs[] = {
	&tmpfs_fifoop_opv_desc,
	&tmpfs_specop_opv_desc,
	&tmpfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops tmpfs_vfsops = {
	MOUNT_TMPFS,			/* vfs_name */
	tmpfs_mount,			/* vfs_mount */
	tmpfs_start,			/* vfs_start */
	tmpfs_unmount,			/* vfs_unmount */
	tmpfs_root,			/* vfs_root */
	tmpfs_quotactl,			/* vfs_quotactl */
	tmpfs_statvfs,			/* vfs_statvfs */
	tmpfs_sync,			/* vfs_sync */
	tmpfs_vget,			/* vfs_vget */
	tmpfs_fhtovp,			/* vfs_fhtovp */
	tmpfs_vptofh,			/* vfs_vptofh */
	tmpfs_init,			/* vfs_init */
	NULL,				/* vfs_reinit */
	tmpfs_done,			/* vfs_done */
	NULL,				/* vfs_mountroot */
	tmpfs_snapshot,			/* vfs_snapshot */
	vfs_stdextattrctl,		/* vfs_extattrctl */
	vfs_stdsuspendctl,		/* vfs_suspendctl */
	tmpfs_vnodeopv_descs,
	0,				/* vfs_refcount */
	{ NULL, NULL },
};
VFS_ATTACH(tmpfs_vfsops);
