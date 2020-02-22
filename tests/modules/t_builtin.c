/*	$NetBSD: t_builtin.c,v 1.5 2020/02/22 00:18:55 kamil Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.  All rights reserved.
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
#include <sys/module.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <fcntl.h>
#include <stdbool.h>

#include <miscfs/kernfs/kernfs.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

#define MYMP "/mnt"
#define HZFILE MYMP "/hz"

static char kernfs[] = "kernfs";

static bool
check_kernfs(void)
{
	char buf[16];
	bool rv = true;
	int fd;

	fd = rump_sys_open(HZFILE, O_RDONLY);
	if (fd == -1)
		return false;
	if (rump_sys_read(fd, buf, sizeof(buf)) < 1)
		rv = false;
	RL(rump_sys_close(fd));

	return rv;
}

ATF_TC(disable);
ATF_TC_HEAD(disable, tc)
{

	atf_tc_set_md_var(tc, "descr", "Tests that builtin modules can "
	    "be disabled");
}

ATF_TC_BODY(disable, tc)
{

	rump_init();
	RL(rump_sys_mkdir(MYMP, 0777));
	RL(rump_sys_mount(MOUNT_KERNFS, MYMP, 0, NULL, 0));
	ATF_REQUIRE(check_kernfs());
	RL(rump_sys_unmount(MYMP, 0));
	RL(rump_sys_modctl(MODCTL_UNLOAD, kernfs));
}

ATF_TC(noauto);
ATF_TC_HEAD(noauto, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests that disabled builtin modules "
	    "will not autoload");
}

ATF_TC_BODY(noauto, tc)
{

	rump_init();
	RL(rump_sys_mkdir(MYMP, 0777));

	RL(rump_sys_modctl(MODCTL_UNLOAD, kernfs));

	ATF_REQUIRE_ERRNO(ENODEV,
	    rump_sys_mount(MOUNT_KERNFS, MYMP, 0, NULL, 0) == -1);
}

ATF_TC(forcereload);
ATF_TC_HEAD(forcereload, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests that disabled builtin modules "
	    "can be force-reloaded");
}

ATF_TC_BODY(forcereload, tc)
{
	struct modctl_load mod;

	rump_init();
	RL(rump_sys_mkdir(MYMP, 0777));

	RL(rump_sys_modctl(MODCTL_UNLOAD, kernfs));
	ATF_REQUIRE_ERRNO(ENODEV,
	    rump_sys_mount(MOUNT_KERNFS, MYMP, 0, NULL, 0) == -1);

	memset(&mod, 0, sizeof(mod));
	mod.ml_filename = kernfs;
	mod.ml_flags = MODCTL_LOAD_FORCE;

	RL(rump_sys_modctl(MODCTL_LOAD, &mod));

	RL(rump_sys_mount(MOUNT_KERNFS, MYMP, 0, NULL, 0));
	ATF_REQUIRE(check_kernfs());
	RL(rump_sys_unmount(MYMP, 0));
}

ATF_TC(disabledstat);
ATF_TC_HEAD(disabledstat, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests that disabled builtin modules "
	    "show up in modstat with refcount -1");
}

ATF_TC_BODY(disabledstat, tc)
{
	modstat_t *ms;
	modstat_t m;
	struct iovec iov;
	size_t len;
	int count;
	bool found = false;

	rump_init();
	RL(rump_sys_mkdir(MYMP, 0777));

	RL(rump_sys_modctl(MODCTL_UNLOAD, kernfs));

	for (len = 8192; ;) {
		iov.iov_base = malloc(len);
		iov.iov_len = len;

		errno = 0;

		if (rump_sys_modctl(MODCTL_STAT, &iov) != 0) {
			int err = errno;
			fprintf(stderr, "modctl(MODCTL_STAT) failed: %s\n",
			    strerror(err));
			atf_tc_fail("Failed to query module status");
		}
		if (len >= iov.iov_len)
			break;
		free(iov.iov_base);
		len = iov.iov_len;
	}

	count = *(int *)iov.iov_base;
	ms = (modstat_t *)((char *)iov.iov_base + sizeof(int));
	while ( count ) {
		memcpy(&m, ms, sizeof(m));		
		if (strcmp(m.ms_name, kernfs) == 0) {
			ATF_REQUIRE_EQ(m.ms_refcnt, (u_int)-1);
			found = true;
			break;
		}
		ms++;
		count--;

	}
	ATF_REQUIRE(found);
}

ATF_TC(busydisable);
ATF_TC_HEAD(busydisable, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests that busy builtin modules "
	    "cannot be disabled");
}

ATF_TC_BODY(busydisable, tc)
{

	rump_init();
	RL(rump_sys_mkdir(MYMP, 0777));
	RL(rump_sys_mount(MOUNT_KERNFS, MYMP, 0, NULL, 0));
	ATF_REQUIRE(check_kernfs());
	ATF_REQUIRE_ERRNO(EBUSY,
	    rump_sys_modctl(MODCTL_UNLOAD, kernfs) == -1);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, disable);
	ATF_TP_ADD_TC(tp, noauto);
	ATF_TP_ADD_TC(tp, forcereload);
	ATF_TP_ADD_TC(tp, disabledstat);
	ATF_TP_ADD_TC(tp, busydisable);

	return atf_no_error();
}
