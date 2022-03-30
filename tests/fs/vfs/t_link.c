/*	$NetBSD: t_link.c,v 1.3 2022/03/30 13:43:42 christos Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <atf-c.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>

#include <rump/rump_syscalls.h>
#include <rump/rump.h>

#include "../common/h_fsmacros.h"
#include "h_macros.h"

#define USES_OWNER							\
	if (FSTYPE_MSDOS(tc))						\
	    atf_tc_skip("owner not supported by file system")
#define USES_USERLEVEL							\
	if (FSTYPE_PUFFS(tc) || FSTYPE_P2K_FFS(tc))			\
	    atf_tc_skip("userlevel pass not supported, "		\
		"since sysctl might not be set in underlying system")
#define USES_OWNCHECK							\
	if (FSTYPE_ZFS(tc))						\
	    atf_tc_skip("zfs not supported since it has its "		\
		"own rules for hardlinks")


static void
hardlink(const atf_tc_t *tc, const char *mp, uid_t u1, uid_t u2,
    bool sysctl, bool allowed)
{
	const char name[] = "foo";
	const char link[] = "bar";
	int one = 1, fd;

	USES_OWNER;
	USES_USERLEVEL;
	USES_OWNCHECK;

	FSTEST_ENTER();

	if (sysctl) {
		if (sysctlbyname(
		    "security.models.extensions.hardlink_check_uid",
		    NULL, 0, &one, sizeof(one)) == -1)
			atf_tc_fail_errno("sysctlbyname");
	}

	rump_pub_lwproc_rfork(RUMP_RFCFDG);
	if (rump_sys_chmod(".", 0777) == -1)
		atf_tc_fail_errno("chmod");
	if (rump_sys_setuid(u1) == -1)
		atf_tc_fail_errno("setuid");
        if ((fd = rump_sys_open(name, O_RDWR|O_CREAT, 0666)) == -1)
		atf_tc_fail_errno("open");
	if (rump_sys_close(fd) == -1)
		atf_tc_fail_errno("close");
	rump_pub_lwproc_releaselwp();

	rump_pub_lwproc_rfork(RUMP_RFCFDG);
	if (rump_sys_setuid(u2) == -1)
		atf_tc_fail_errno("setuid");
        if (rump_sys_link(name, link) == -1) {
		if (errno != EOPNOTSUPP && allowed)
			atf_tc_fail_errno("link");
	} else {
		if (!allowed)
			atf_tc_fail("failed to disallow hard link");
	}
	rump_pub_lwproc_releaselwp();

	FSTEST_EXIT();
}


static void
hardlink_sameuser(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 1, false, true);
}

static void
hardlink_sameuser_sysctl(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 1, true, true);
}

static void
hardlink_otheruser(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 2, false, true);
}

static void
hardlink_otheruser_sysctl(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 2, true, false);
}

static void
hardlink_rootuser(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 0, false, true);
}

static void
hardlink_rootuser_sysctl(const atf_tc_t *tc, const char *mp)
{
	hardlink(tc, mp, 1, 0, true, true);
}

ATF_TC_FSAPPLY(hardlink_sameuser, "hardlink same user allowed");
ATF_TC_FSAPPLY(hardlink_sameuser_sysctl, "hardlink same user sysctl allowed");
ATF_TC_FSAPPLY(hardlink_otheruser, "hardlink other user allowed");
ATF_TC_FSAPPLY(hardlink_otheruser_sysctl, "hardlink other user sysctl denied");
ATF_TC_FSAPPLY(hardlink_rootuser, "hardlink root user allowed");
ATF_TC_FSAPPLY(hardlink_rootuser_sysctl, "hardlink root user sysctl allowed");

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_FSAPPLY(hardlink_sameuser);
	ATF_TP_FSAPPLY(hardlink_sameuser_sysctl);
	ATF_TP_FSAPPLY(hardlink_otheruser);
	ATF_TP_FSAPPLY(hardlink_otheruser_sysctl);
	ATF_TP_FSAPPLY(hardlink_rootuser);
	ATF_TP_FSAPPLY(hardlink_rootuser_sysctl);

	return atf_no_error();
}
