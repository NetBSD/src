/* 	$NetBSD: devfs_specops.c,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $ */

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
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
 * devfs vnode interface for special devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devfs_specops.c,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $");

#include <sys/param.h>
#include <sys/vnode.h>

#include <fs/devfs/devfs.h>
#include <fs/devfs/devfs_specops.h>

/* --------------------------------------------------------------------- */

/*
 * vnode operations vector used for special devices stored in a devfs
 * file system.
 */
int (**devfs_specop_p)(void *);
const struct vnodeopv_entry_desc devfs_specop_entries[] = {
	{ &vop_default_desc,		vn_default_error },
	{ &vop_lookup_desc,		devfs_spec_lookup },
	{ &vop_create_desc,		devfs_spec_create },
	{ &vop_mknod_desc,		devfs_spec_mknod },
	{ &vop_open_desc,		devfs_spec_open },
	{ &vop_close_desc,		devfs_spec_close },
	{ &vop_access_desc,		devfs_spec_access },
	{ &vop_getattr_desc,		devfs_spec_getattr },
	{ &vop_setattr_desc,		devfs_spec_setattr },
	{ &vop_read_desc,		devfs_spec_read },
	{ &vop_write_desc,		devfs_spec_write },
	{ &vop_ioctl_desc,		devfs_spec_ioctl },
	{ &vop_fcntl_desc,		devfs_spec_fcntl },
	{ &vop_poll_desc,		devfs_spec_poll },
	{ &vop_kqfilter_desc,		devfs_spec_kqfilter },
	{ &vop_revoke_desc,		devfs_spec_revoke },
	{ &vop_mmap_desc,		devfs_spec_mmap },
	{ &vop_fsync_desc,		devfs_spec_fsync },
	{ &vop_seek_desc,		devfs_spec_seek },
	{ &vop_remove_desc,		devfs_spec_remove },
	{ &vop_link_desc,		devfs_spec_link },
	{ &vop_rename_desc,		devfs_spec_rename },
	{ &vop_mkdir_desc,		devfs_spec_mkdir },
	{ &vop_rmdir_desc,		devfs_spec_rmdir },
	{ &vop_symlink_desc,		devfs_spec_symlink },
	{ &vop_readdir_desc,		devfs_spec_readdir },
	{ &vop_readlink_desc,		devfs_spec_readlink },
	{ &vop_abortop_desc,		devfs_spec_abortop },
	{ &vop_inactive_desc,		devfs_spec_inactive },
	{ &vop_reclaim_desc,		devfs_spec_reclaim },
	{ &vop_lock_desc,		devfs_spec_lock },
	{ &vop_unlock_desc,		devfs_spec_unlock },
	{ &vop_bmap_desc,		devfs_spec_bmap },
	{ &vop_strategy_desc,		devfs_spec_strategy },
	{ &vop_print_desc,		devfs_spec_print },
	{ &vop_pathconf_desc,		devfs_spec_pathconf },
	{ &vop_islocked_desc,		devfs_spec_islocked },
	{ &vop_advlock_desc,		devfs_spec_advlock },
	{ &vop_bwrite_desc,		devfs_spec_bwrite },
	{ &vop_getpages_desc,		devfs_spec_getpages },
	{ &vop_putpages_desc,		devfs_spec_putpages },
	{ NULL, NULL }
};
const struct vnodeopv_desc devfs_specop_opv_desc =
	{ &devfs_specop_p, devfs_specop_entries };

/* --------------------------------------------------------------------- */

int
devfs_spec_close(void *v)
{
	struct vnode *vp = ((struct vop_close_args *)v)->a_vp;

	int error;

	devfs_update(vp, NULL, NULL, UPDATE_CLOSE);
	error = VOCALL(spec_vnodeop_p, VOFFSET(vop_close), v);

	return error;
}

/* --------------------------------------------------------------------- */

int
devfs_spec_read(void *v)
{
	struct vnode *vp = ((struct vop_read_args *)v)->a_vp;

	VP_TO_DEVFS_NODE(vp)->tn_status |= DEVFS_NODE_ACCESSED;
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_read), v);
}

/* --------------------------------------------------------------------- */

int
devfs_spec_write(void *v)
{
	struct vnode *vp = ((struct vop_write_args *)v)->a_vp;

	VP_TO_DEVFS_NODE(vp)->tn_status |= DEVFS_NODE_MODIFIED;
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_write), v);
}
