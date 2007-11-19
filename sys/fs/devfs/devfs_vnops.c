/*	$NetBSD: devfs_vnops.c,v 1.1.2.1 2007/11/19 00:34:33 mjf Exp $	*/

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
 * devfs vnode interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devfs_vnops.c,v 1.1.2.1 2007/11/19 00:34:33 mjf Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/event.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <sys/vnode.h>
#include <sys/lockf.h>
#include <sys/kauth.h>

#include <uvm/uvm.h>

#include <fs/devfs/devfs.h>

#include <miscfs/fifofs/fifo.h>
#include <fs/tmpfs/tmpfs_vnops.h>
#include <fs/tmpfs/tmpfs_specops.h>
#include <fs/tmpfs/tmpfs_fifoops.h>
#include <fs/tmpfs/tmpfs.h>

/*
 * vnode operations vector used for files stored in a devfs file system.
 */
int (**devfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc devfs_vnodeop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		tmpfs_lookup },
	{ &vop_create_desc,		tmpfs_create },
	{ &vop_mknod_desc,		tmpfs_mknod },
	{ &vop_open_desc,		tmpfs_open },
	{ &vop_close_desc,		tmpfs_close },
	{ &vop_access_desc,		tmpfs_access },
	{ &vop_getattr_desc,		tmpfs_getattr },
	{ &vop_setattr_desc,		tmpfs_setattr },
	{ &vop_read_desc,		tmpfs_read },
	{ &vop_write_desc,		tmpfs_write },
	{ &vop_ioctl_desc,		tmpfs_ioctl },
	{ &vop_fcntl_desc,		tmpfs_fcntl },
	{ &vop_poll_desc,		tmpfs_poll },
	{ &vop_kqfilter_desc,		tmpfs_kqfilter },
	{ &vop_revoke_desc,		tmpfs_revoke },
	{ &vop_mmap_desc,		tmpfs_mmap },
	{ &vop_fsync_desc,		tmpfs_fsync },
	{ &vop_seek_desc,		tmpfs_seek },
	{ &vop_remove_desc,		tmpfs_remove },
	{ &vop_link_desc,		tmpfs_link },
	{ &vop_rename_desc,		tmpfs_rename },
	{ &vop_mkdir_desc,		tmpfs_mkdir },
	{ &vop_rmdir_desc,		tmpfs_rmdir },
	{ &vop_symlink_desc,		tmpfs_symlink },
	{ &vop_readdir_desc,		tmpfs_readdir },
	{ &vop_readlink_desc,		tmpfs_readlink },
	{ &vop_abortop_desc,		tmpfs_abortop },
	{ &vop_inactive_desc,		tmpfs_inactive },
	{ &vop_reclaim_desc,		tmpfs_reclaim },
	{ &vop_lock_desc,		tmpfs_lock },
	{ &vop_unlock_desc,		tmpfs_unlock },
	{ &vop_bmap_desc,		tmpfs_bmap },
	{ &vop_strategy_desc,		tmpfs_strategy },
	{ &vop_print_desc,		tmpfs_print },
	{ &vop_pathconf_desc,		tmpfs_pathconf },
	{ &vop_islocked_desc,		tmpfs_islocked },
	{ &vop_advlock_desc,		tmpfs_advlock },
	{ &vop_lease_desc,		tmpfs_lease },
	{ &vop_bwrite_desc,		tmpfs_bwrite },
	{ &vop_getpages_desc,		tmpfs_getpages },
	{ &vop_putpages_desc,		tmpfs_putpages },
	{ NULL, NULL }
};
const struct vnodeopv_desc devfs_vnodeop_opv_desc =
	{ &devfs_vnodeop_p, devfs_vnodeop_entries };

/*
 * vnode operations vector used for special devices stored in a tmpfs
 * file system.
 */
int (**devfs_specop_p)(void *);
const struct vnodeopv_entry_desc devfs_specop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		tmpfs_spec_lookup },
	{ &vop_create_desc,		tmpfs_spec_create },
	{ &vop_mknod_desc,		tmpfs_spec_mknod },
	{ &vop_open_desc,		tmpfs_spec_open },
	{ &vop_close_desc,		tmpfs_spec_close },
	{ &vop_access_desc,		tmpfs_spec_access },
	{ &vop_getattr_desc,		tmpfs_spec_getattr },
	{ &vop_setattr_desc,		tmpfs_spec_setattr },
	{ &vop_read_desc,		tmpfs_spec_read },
	{ &vop_write_desc,		tmpfs_spec_write },
	{ &vop_ioctl_desc,		tmpfs_spec_ioctl },
	{ &vop_fcntl_desc,		tmpfs_spec_fcntl },
	{ &vop_poll_desc,		tmpfs_spec_poll },
	{ &vop_kqfilter_desc,		tmpfs_spec_kqfilter },
	{ &vop_revoke_desc,		tmpfs_spec_revoke },
	{ &vop_mmap_desc,		tmpfs_spec_mmap },
	{ &vop_fsync_desc,		tmpfs_spec_fsync },
	{ &vop_seek_desc,		tmpfs_spec_seek },
	{ &vop_remove_desc,		tmpfs_spec_remove },
	{ &vop_link_desc,		tmpfs_spec_link },
	{ &vop_rename_desc,		tmpfs_spec_rename },
	{ &vop_mkdir_desc,		tmpfs_spec_mkdir },
	{ &vop_rmdir_desc,		tmpfs_spec_rmdir },
	{ &vop_symlink_desc,		tmpfs_spec_symlink },
	{ &vop_readdir_desc,		tmpfs_spec_readdir },
	{ &vop_readlink_desc,		tmpfs_spec_readlink },
	{ &vop_abortop_desc,		tmpfs_spec_abortop },
	{ &vop_inactive_desc,		tmpfs_spec_inactive },
	{ &vop_reclaim_desc,		tmpfs_spec_reclaim },
	{ &vop_lock_desc,		tmpfs_spec_lock },
	{ &vop_unlock_desc,		tmpfs_spec_unlock },
	{ &vop_bmap_desc,		tmpfs_spec_bmap },
	{ &vop_strategy_desc,		tmpfs_spec_strategy },
	{ &vop_print_desc,		tmpfs_spec_print },
	{ &vop_pathconf_desc,		tmpfs_spec_pathconf },
	{ &vop_islocked_desc,		tmpfs_spec_islocked },
	{ &vop_advlock_desc,		tmpfs_spec_advlock },
	{ &vop_lease_desc,		tmpfs_spec_lease },
	{ &vop_bwrite_desc,		tmpfs_spec_bwrite },
	{ &vop_getpages_desc,		tmpfs_spec_getpages },
	{ &vop_putpages_desc,		tmpfs_spec_putpages },
	{ NULL, NULL }
};
const struct vnodeopv_desc devfs_specop_opv_desc =
	{ &devfs_specop_p, devfs_specop_entries };

/*
 * vnode operations vector used for fifos stored in a tmpfs file system.
 */
int (**devfs_fifoop_p)(void *);
const struct vnodeopv_entry_desc devfs_fifoop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		tmpfs_fifo_lookup },
	{ &vop_create_desc,		tmpfs_fifo_create },
	{ &vop_mknod_desc,		tmpfs_fifo_mknod },
	{ &vop_open_desc,		tmpfs_fifo_open },
	{ &vop_close_desc,		tmpfs_fifo_close },
	{ &vop_access_desc,		tmpfs_fifo_access },
	{ &vop_getattr_desc,		tmpfs_fifo_getattr },
	{ &vop_setattr_desc,		tmpfs_fifo_setattr },
	{ &vop_read_desc,		tmpfs_fifo_read },
	{ &vop_write_desc,		tmpfs_fifo_write },
	{ &vop_ioctl_desc,		tmpfs_fifo_ioctl },
	{ &vop_fcntl_desc,		tmpfs_fifo_fcntl },
	{ &vop_poll_desc,		tmpfs_fifo_poll },
	{ &vop_kqfilter_desc,		tmpfs_fifo_kqfilter },
	{ &vop_revoke_desc,		tmpfs_fifo_revoke },
	{ &vop_mmap_desc,		tmpfs_fifo_mmap },
	{ &vop_fsync_desc,		tmpfs_fifo_fsync },
	{ &vop_seek_desc,		tmpfs_fifo_seek },
	{ &vop_remove_desc,		tmpfs_fifo_remove },
	{ &vop_link_desc,		tmpfs_fifo_link },
	{ &vop_rename_desc,		tmpfs_fifo_rename },
	{ &vop_mkdir_desc,		tmpfs_fifo_mkdir },
	{ &vop_rmdir_desc,		tmpfs_fifo_rmdir },
	{ &vop_symlink_desc,		tmpfs_fifo_symlink },
	{ &vop_readdir_desc,		tmpfs_fifo_readdir },
	{ &vop_readlink_desc,		tmpfs_fifo_readlink },
	{ &vop_abortop_desc,		tmpfs_fifo_abortop },
	{ &vop_inactive_desc,		tmpfs_fifo_inactive },
	{ &vop_reclaim_desc,		tmpfs_fifo_reclaim },
	{ &vop_lock_desc,		tmpfs_fifo_lock },
	{ &vop_unlock_desc,		tmpfs_fifo_unlock },
	{ &vop_bmap_desc,		tmpfs_fifo_bmap },
	{ &vop_strategy_desc,		tmpfs_fifo_strategy },
	{ &vop_print_desc,		tmpfs_fifo_print },
	{ &vop_pathconf_desc,		tmpfs_fifo_pathconf },
	{ &vop_islocked_desc,		tmpfs_fifo_islocked },
	{ &vop_advlock_desc,		tmpfs_fifo_advlock },
	{ &vop_lease_desc,		tmpfs_fifo_lease },
	{ &vop_bwrite_desc,		tmpfs_fifo_bwrite },
	{ &vop_getpages_desc,		tmpfs_fifo_getpages },
	{ &vop_putpages_desc,		tmpfs_fifo_putpages },
	{ NULL, NULL }
};
const struct vnodeopv_desc devfs_fifoop_opv_desc =
	{ &devfs_fifoop_p, devfs_fifoop_entries };
