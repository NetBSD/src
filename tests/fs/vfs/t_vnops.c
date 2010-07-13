/*	$NetBSD: t_vnops.c,v 1.1 2010/07/13 18:13:10 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/stat.h>
#include <sys/statvfs.h>

#include <atf-c.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rump.h>

#include "../common/h_fsmacros.h"
#include "../../h_macros.h"

/* make work when run with 5.0 userland */
#if defined(__NetBSD_Version__) && (__NetBSD_Version__ < 599000700)
#define rump_sys_stat rump_pub_sys___stat30
#endif

#define USES_DIRS \
    if (FSTYPE_SYSVBFS(tc)) atf_tc_skip("dirs not supported by file system")

static void
lookup_simple(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN], final[MAXPATHLEN];
	struct stat sb1, sb2;

	strcpy(final, mountpath);
	sprintf(pb, "%s/../%s", mountpath, basename(final));
	if (rump_sys_stat(pb, &sb1) == -1)
		atf_tc_fail_errno("stat 1");

	sprintf(pb, "%s/./../%s", mountpath, basename(final));
	if (rump_sys_stat(pb, &sb2) == -1)
		atf_tc_fail_errno("stat 2");

	ATF_REQUIRE(memcmp(&sb1, &sb2, sizeof(sb1)) == 0);
}

static void
lookup_complex(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN];
	struct stat sb1, sb2;

	USES_DIRS;

	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");
	if (rump_sys_stat(pb, &sb1) == -1)
		atf_tc_fail_errno("stat 1");

	sprintf(pb, "%s/./dir/../././dir/.", mountpath);
	if (rump_sys_stat(pb, &sb2) == -1)
		atf_tc_fail_errno("stat 2");

	ATF_REQUIRE(memcmp(&sb1, &sb2, sizeof(sb1)) == 0);
}

static void
dir_simple(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN];
	struct stat sb;

	USES_DIRS;

	/* check we can create directories */
	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");
	if (rump_sys_stat(pb, &sb) == -1)
		atf_tc_fail_errno("stat new directory");

	/* check we can remove then and that it makes them unreachable */
	if (rump_sys_rmdir(pb) == -1)
		atf_tc_fail_errno("rmdir");
	if (rump_sys_stat(pb, &sb) != -1 || errno != ENOENT)
		atf_tc_fail("ENOENT expected from stat");
}

static void
dir_notempty(const atf_tc_t *tc, const char *mountpath)
{
	char pb[MAXPATHLEN], pb2[MAXPATHLEN];
	int fd, rv;

	USES_DIRS;

	/* check we can create directories */
	sprintf(pb, "%s/dir", mountpath);
	if (rump_sys_mkdir(pb, 0777) == -1)
		atf_tc_fail_errno("mkdir");

	sprintf(pb2, "%s/dir/file", mountpath);
	fd = rump_sys_open(pb2, O_RDWR | O_CREAT, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create file");
	rump_sys_close(fd);

	rv = rump_sys_rmdir(pb);
	if (rv != -1 || errno != ENOTEMPTY)
		atf_tc_fail("non-empty directory removed succesfully");

	if (rump_sys_unlink(pb2) == -1)
		atf_tc_fail_errno("cannot remove dir/file");

	if (rump_sys_rmdir(pb) == -1)
		atf_tc_fail_errno("remove directory");
}

ATF_TC_FSAPPLY(lookup_simple, "simple lookup (./.. on root)");
ATF_TC_FSAPPLY(lookup_complex, "lookup of non-dot entries");
ATF_TC_FSAPPLY(dir_simple, "mkdir/rmdir");
ATF_TC_FSAPPLY(dir_notempty, "non-empty directories cannot be removed");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(lookup_simple);
	ATF_TP_FSAPPLY(lookup_complex);
	ATF_TP_FSAPPLY(dir_simple);
	ATF_TP_FSAPPLY(dir_notempty);

	return atf_no_error();
}
