/*	$NetBSD: t_unmount.c,v 1.4 2024/10/02 17:16:32 bad Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2024\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_unmount.c,v 1.4 2024/10/02 17:16:32 bad Exp $");

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/vnode.h>

#include <rump/rump.h>
#include <rump/rumpvnode_if.h>
#include <rump/rump_syscalls.h>

#include <fs/tmpfs/tmpfs_args.h>

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>

#include "h_macros.h"

ATF_TC(async);
ATF_TC_HEAD(async, tc)
{
	atf_tc_set_md_var(tc,
	    "descr", "failed unmount of async fs should stay async");
}

#define MP "/mnt"

ATF_TC_BODY(async, tc)
{
	struct tmpfs_args args;
	struct vnode *vp;
	extern struct vnode *rumpns_rootvnode;

	RZ(rump_init());

	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;

	/* create mount point and mount a tmpfs on it */
	RL(rump_sys_mkdir(MP, 0777));
	RL(rump_sys_mount(MOUNT_TMPFS, MP, MNT_ASYNC, &args, sizeof(args)));

	/* make sure the tmpfs is busy */
	RL(rump_sys_chdir(MP));

	/* need a stable lwp for componentname */
	RZ(rump_pub_lwproc_rfork(RUMP_RFCFDG));

	/* get vnode of MP, unlocked */
	RZ(rump_pub_namei(RUMP_NAMEI_LOOKUP, 0, MP, NULL, &vp, NULL));

	/* make sure we didn't just get the root vnode */
	ATF_REQUIRE_MSG((rumpns_rootvnode != vp), "drat! got the root vnode");

	printf("mnt_iflag & IMNT_ONWORKLIST == %d\n",
	    (vp->v_mount->mnt_iflag & IMNT_ONWORKLIST) != 0);

	/* can't unmount a busy file system */
	ATF_REQUIRE_ERRNO(EBUSY, rump_sys_unmount(MP, 0) == -1);

	printf("mnt_iflag & IMNT_ONWORKLIST == %d\n",
	    (vp->v_mount->mnt_iflag & IMNT_ONWORKLIST) != 0);


	ATF_REQUIRE_MSG(((vp->v_mount->mnt_iflag & IMNT_ONWORKLIST) == 0),
	    "mount point on syncer work list");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, async);

	return atf_no_error();
}
