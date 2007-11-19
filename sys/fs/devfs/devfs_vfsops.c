/*	$NetBSD: devfs_vfsops.c,v 1.1.2.1 2007/11/19 00:34:33 mjf Exp $	*/

/*
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 * Device Filesystem
 * 
 * This implementation is inspired by the FreeBSD devfs, although we have
 * made some significant changes:
 *
 *	- The entire rule code is written in userland.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devfs_vfsops.c,v 1.1.2.1 2007/11/19 00:34:33 mjf Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/proc.h>

#include <fs/devfs/devfs.h>
#include <fs/tmpfs/tmpfs.h>

static int	devfs_mount(struct mount *, const char *, void *, size_t *,
		    struct lwp *);
static int	devfs_root(struct mount *, struct vnode **);
static void	devfs_init(void);
static void	devfs_done(void);

MALLOC_JUSTDEFINE(M_DEVFSMNT, "devfs mount", "devfs mount structure");

int
devfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len,
    struct lwp *l)
{
        struct devfs_args *args = data;
        struct vnode *devvp;
        struct devfs_mount *dmp;
	struct tmpfs_mount *tmp;
	struct vnode *vp;

        if (*data_len < sizeof *args)
                return EINVAL;

        if (mp->mnt_flag & MNT_GETARGS) {
                dmp = VFSTODEVFS(mp);
                if (dmp == NULL)
                        return EIO;
                args->fspec = NULL;
                *data_len = sizeof *args;
                return 0;
        }

        if (data == NULL)
                return EINVAL;

        if (mnt->mnt_flag & (MNT_UPDATE | MNT_ROOTFS)) {
                return EOPNOTSUPP;
	
        /*
         * We are a tmpfs except that we identify as a devfs.
         */
        if ((error = tmpfs_mount(mp, path, data, data_len, l)) != 0)
		return error;

	tmp = (struct tmpfs_mount *)mp->mnt_data;

	/* Copy over all the data from the struct tmpfs_mount */
	dmp = (struct devfs_mount *)malloc(sizeof *dmp, M_DEVFSMNT,
	    M_WAITOK | M_ZERO);
	memcpy(dmp, tmp, sizeof *tmp);

	/* We're done with this space now? */
	free(tmp, M_TMPFSTMP);

	mp->mnt_data = dmp;

	dmp->dm_mount = mp;

	if ((error = set_statvfs_info(path, UIO_USERSPACE, "devfs",
	    UIO_SYSSPACE, mp->mnt_op->vfs_name, mp, l)) != 0)
	    	return error;

	return 0;
}

static void
devfs_init(void)
{
	malloc_type_attach(M_DEVFSMNT);
}

static void
devfs_done(void)
{
	malloc_type_detach(M_DEVFSMNT);
}

/*
 * devfs vfs operations.
 */

extern const struct vnodeopv_desc tmpfs_fifoop_opv_desc;
extern const struct vnodeopv_desc tmpfs_specop_opv_desc;
extern const struct vnodeopv_desc devfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const devfs_vnodeopv_descs[] = {
	&tmpfs_fifoop_opv_desc,
	&tmpfs_specop_opv_desc,
	&devfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops devfs_vfsops = {
	MOUNT_DEVFS,			/* vfs_name */
	sizeof (struct devfs_args),
	devfs_mount,			/* vfs_mount */
	tmpfs_start,			/* vfs_start */
	tmpfs_unmount,			/* vfs_unmount */
	tmpfs_root,			/* vfs_root */
	tmpfs_quotactl,			/* vfs_quotactl */
	tmpfs_statvfs,			/* vfs_statvfs */
	tmpfs_sync,			/* vfs_sync */
	tmpfs_vget,			/* vfs_vget */
	tmpfs_fhtovp,			/* vfs_fhtovp */
	tmpfs_vptofh,			/* vfs_vptofh */
	devfs_init,			/* vfs_init */
	NULL,				/* vfs_reinit */
	devfs_done,			/* vfs_done */
	NULL,				/* vfs_mountroot */
	tmpfs_snapshot,			/* vfs_snapshot */
	vfs_stdextattrctl,		/* vfs_extattrctl */
	(void *)eopnotsupp,		/* vfs_suspendctl */
	devfs_vnodeopv_descs,
	0,				/* vfs_refcount */
	{ NULL, NULL },
};
VFS_ATTACH(devfs_vfsops);
