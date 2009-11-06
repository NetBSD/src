/*	$NetBSD: t_modcmd.c,v 1.3 2009/11/06 15:25:52 pooka Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/mount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <fs/tmpfs/tmpfs_args.h>

#include <atf-c.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "../../h_macros.h"

/*
 * We verify that modules can be loaded and unloaded.
 * tmpfs was chosen because it does not depend on an image.
 */

ATF_TC(cmsg_modcmd);
ATF_TC_HEAD(cmsg_modcmd, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that loading and unloading "
	    "a module (vfs/tmpfs) is possible");
}

#define TMPFSMODULE "librumpfs_tmpfs.so"
ATF_TC_BODY(cmsg_modcmd, tc)
{
	struct tmpfs_args args;
	struct modinfo **mi;
	void *handle;
	int rv;

	rump_init();
	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;
	
	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("mkdir mountpoint");
	if (rump_sys_mount(MOUNT_TMPFS, "/mp", 0, &args, sizeof(args)) != -1)
		atf_tc_fail("mount unexpectedly succeeded");

	handle = dlopen(TMPFSMODULE, RTLD_GLOBAL);
	if (handle == NULL) {
		const char *dlmsg = dlerror();
		atf_tc_fail("cannot open %s: %s", TMPFSMODULE, dlmsg);
	}
	mi = dlsym(handle, "__start_link_set_modules");
	if (mi == NULL)
		atf_tc_fail("cannot find module info");
	if ((rv = rump_pub_module_init(*mi, NULL)) != 0)
		atf_tc_fail("module init failed: %d (%s)", rv, strerror(rv));

	if (rump_sys_mount(MOUNT_TMPFS, "/mp", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("still cannot mount");
	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail("cannot unmount");
	if ((rv = rump_pub_module_fini(*mi)) != 0)
		atf_tc_fail("module fini failed: %d (%s)", rv, strerror(rv));
	if (dlclose(handle)) {
		const char *dlmsg = dlerror();
		atf_tc_fail("cannot close %s: %s", TMPFSMODULE, dlmsg);
	}

	if (rump_sys_mount(MOUNT_TMPFS, "/mp", 0, &args, sizeof(args)) != -1)
		atf_tc_fail("mount unexpectedly succeeded");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cmsg_modcmd);
}
