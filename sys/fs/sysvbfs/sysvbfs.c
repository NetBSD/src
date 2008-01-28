/*	$NetBSD: sysvbfs.c,v 1.9 2008/01/28 14:31:17 dholland Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysvbfs.c,v 1.9 2008/01/28 14:31:17 dholland Exp $");

#include <sys/resource.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>
#include <fs/sysvbfs/sysvbfs.h>

/* External interfaces */

int (**sysvbfs_vnodeop_p)(void *);	/* filled by getnewvnode (vnode.h) */

const struct vnodeopv_entry_desc sysvbfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, sysvbfs_lookup },		/* lookup */
	{ &vop_create_desc, sysvbfs_create },		/* create */
	{ &vop_mknod_desc, genfs_eopnotsupp },		/* mknod */
	{ &vop_open_desc, sysvbfs_open },		/* open */
	{ &vop_close_desc, sysvbfs_close },		/* close */
	{ &vop_access_desc, sysvbfs_access },		/* access */
	{ &vop_getattr_desc, sysvbfs_getattr },		/* getattr */
	{ &vop_setattr_desc, sysvbfs_setattr },		/* setattr */
	{ &vop_read_desc, sysvbfs_read },		/* read */
	{ &vop_write_desc, sysvbfs_write },		/* write */
	{ &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, genfs_enoioctl },		/* ioctl */
	{ &vop_poll_desc, genfs_poll },			/* poll */
	{ &vop_kqfilter_desc, genfs_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, genfs_revoke },		/* revoke */
	{ &vop_mmap_desc, genfs_mmap },			/* mmap */
	{ &vop_fsync_desc, sysvbfs_fsync },		/* fsync */
	{ &vop_seek_desc, genfs_seek },			/* seek */
	{ &vop_remove_desc, sysvbfs_remove },		/* remove */
	{ &vop_link_desc, genfs_eopnotsupp },		/* link */
	{ &vop_rename_desc, sysvbfs_rename },		/* rename */
	{ &vop_mkdir_desc, genfs_eopnotsupp },		/* mkdir */
	{ &vop_rmdir_desc, genfs_eopnotsupp },		/* rmdir */
	{ &vop_symlink_desc, genfs_eopnotsupp },	/* symlink */
	{ &vop_readdir_desc, sysvbfs_readdir },		/* readdir */
	{ &vop_readlink_desc, genfs_eopnotsupp },	/* readlink */
	{ &vop_abortop_desc, genfs_abortop },		/* abortop */
	{ &vop_inactive_desc, sysvbfs_inactive },	/* inactive */
	{ &vop_reclaim_desc, sysvbfs_reclaim },		/* reclaim */
	{ &vop_lock_desc, genfs_lock },			/* lock */
	{ &vop_unlock_desc, genfs_unlock },		/* unlock */
	{ &vop_bmap_desc, sysvbfs_bmap },		/* bmap */
	{ &vop_strategy_desc, sysvbfs_strategy },	/* strategy */
	{ &vop_print_desc, sysvbfs_print },		/* print */
	{ &vop_islocked_desc, genfs_islocked },		/* islocked */
	{ &vop_pathconf_desc, sysvbfs_pathconf },	/* pathconf */
	{ &vop_advlock_desc, sysvbfs_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, genfs_getpages },		/* getpages */
	{ &vop_putpages_desc, genfs_putpages },		/* putpages */
	{ NULL, NULL }
};

const struct vnodeopv_desc sysvbfs_vnodeop_opv_desc = {
	&sysvbfs_vnodeop_p,
	sysvbfs_vnodeop_entries
};

const struct vnodeopv_desc *sysvbfs_vnodeopv_descs[] = {
	&sysvbfs_vnodeop_opv_desc,
	NULL,
};

const struct genfs_ops sysvbfs_genfsops = {
	.gop_size = genfs_size,
	.gop_alloc = sysvbfs_gop_alloc,
	.gop_write = genfs_gop_write,
};

struct vfsops sysvbfs_vfsops = {
	MOUNT_SYSVBFS,
	sizeof (struct sysvbfs_args),
	sysvbfs_mount,
	sysvbfs_start,
	sysvbfs_unmount,
	sysvbfs_root,
	(void *)eopnotsupp,	/* vfs_quotactl */
	sysvbfs_statvfs,
	sysvbfs_sync,
	sysvbfs_vget,
	sysvbfs_fhtovp,
	sysvbfs_vptofh,
	sysvbfs_init,
	sysvbfs_reinit,
	sysvbfs_done,
	NULL,			/* vfs_mountroot */
	(int (*)(struct mount *, struct vnode *, struct timespec *))
	    eopnotsupp,		/* snapshot */
	vfs_stdextattrctl,
	(void *)eopnotsupp,	/* vfs_suspendctl */
	genfs_renamelock_enter,
	genfs_renamelock_exit,
	sysvbfs_vnodeopv_descs,
	0,
	{ NULL, NULL }
};
VFS_ATTACH(sysvbfs_vfsops);
