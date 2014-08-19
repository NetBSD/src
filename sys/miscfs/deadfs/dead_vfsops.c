/*	$NetBSD: dead_vfsops.c,v 1.2.10.2 2014/08/20 00:04:30 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
__KERNEL_RCSID(0, "$NetBSD: dead_vfsops.c,v 1.2.10.2 2014/08/20 00:04:30 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>

extern const struct vnodeopv_desc dead_vnodeop_opv_desc;

static const struct vnodeopv_desc * const dead_vnodeopv_descs[] = {
	&dead_vnodeop_opv_desc,
	NULL
};

static void
dead_panic(void)
{

	panic("dead fs operation used");
}

struct vfsops dead_vfsops = {
	.vfs_name = "dead",
	.vfs_min_mount_data = 0,
	.vfs_mount = (void *)dead_panic,
	.vfs_start = (void *)dead_panic,
	.vfs_unmount = (void *)dead_panic,
	.vfs_root = (void *)dead_panic,
	.vfs_quotactl = (void *)dead_panic,
	.vfs_statvfs = (void *)dead_panic,
	.vfs_sync = (void *)dead_panic,
	.vfs_vget = (void *)dead_panic,
	.vfs_fhtovp = (void *)dead_panic,
	.vfs_vptofh = (void *)dead_panic,
	.vfs_init = (void *)dead_panic,
	.vfs_reinit = (void *)dead_panic,
	.vfs_done = (void *)dead_panic,
	.vfs_mountroot = (void *)dead_panic,
	.vfs_snapshot = (void *)dead_panic,
	.vfs_extattrctl = (void *)dead_panic,
	.vfs_suspendctl = (void *)dead_panic,
	.vfs_renamelock_enter = (void *)dead_panic,
	.vfs_renamelock_exit = (void *)dead_panic,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = dead_vnodeopv_descs
};
