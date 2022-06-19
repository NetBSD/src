/*	$NetBSD: zfs.c,v 1.1 2022/06/19 11:31:19 simonb Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
__RCSID("$NetBSD: zfs.c,v 1.1 2022/06/19 11:31:19 simonb Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#define __EXPOSE_MOUNT
#include <sys/mount.h>

#include "zfs_znode.h"

#include <kvm.h>
#include <err.h>
#include "fstat.h"

int
zfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	znode_t inode;
	struct mount mt;

	if (!KVM_READ(VTOZ(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOZ(vp), Pid);
		return 0;
	}
	if (!KVM_READ(vp->v_mount, &mt, sizeof(mt))) {
		dprintf("can't read mount at %p for pid %d",
		    vp->v_mount, Pid);
		return 0;
	}
	/*
	 * XXX: fsid should be something like
	 *    ((struct zfsvfs *)vp->v_mount->mnt_data)->
	 *    z_os->os_ds_dataset->ds_fsid_guid
	 * but ZFS headers are pretty much impossible to include
	 * from userland.
	 */
	fsp->fsid = mt.mnt_stat.f_fsidx.__fsid_val[0];
	fsp->fileid = inode.z_id;
	fsp->mode = inode.z_mode;
	fsp->size = inode.z_size;
	fsp->rdev = 0;
	return 1;
}
