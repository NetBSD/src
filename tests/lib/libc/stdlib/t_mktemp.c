/* $NetBSD: t_mktemp.c,v 1.4 2021/01/11 20:31:34 christos Exp $ */

/*-
 * Copyright (c) 2013, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Jukka Ruohonen.
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
__RCSID("$NetBSD: t_mktemp.c,v 1.4 2021/01/11 20:31:34 christos Exp $");

#include <sys/stat.h>

#include <atf-c.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int
check_mode(struct stat sa, const int mode, const int dir)
{

	if (dir == 0 && S_ISREG(sa.st_mode) == 0)
		return -1;

	if (dir != 0 && S_ISDIR(sa.st_mode) == 0)
		return -1;

	if ((sa.st_mode & mode) == 0)
		return -1;

	return 0;
}

ATF_TC(mktemp_not_exist);
ATF_TC_HEAD(mktemp_not_exist, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that mktemp(3)"
	    " does not fail on a path that does not exist");
}

ATF_TC_BODY(mktemp_not_exist, tc)
{
	char template[] = "I will barf/XXXXXX";
	ATF_REQUIRE(mktemp(template) != NULL);
}

ATF_TC(mktemp_large_template);
ATF_TC_HEAD(mktemp_large_template, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that mktemp "
	    "accepts arbitrarily large templates (PR lib/55441)");
}

ATF_TC_BODY(mktemp_large_template, tc)
{
	const char *path = "/tmp/mktemp.XXXXXX";
	char *template;
	size_t i;
	long name_max;
	size_t tlen;

	name_max = pathconf("/tmp/", _PC_NAME_MAX);
	ATF_REQUIRE(name_max > 0);

	tlen = (size_t)name_max + sizeof("/tmp/");
	template = malloc(tlen);
	ATF_REQUIRE(template != NULL);

	ATF_REQUIRE(strcpy(template, path) != NULL);
	ATF_REQUIRE(mktemp(template) != NULL);
	ATF_REQUIRE(strlen(template) == strlen(path));
	ATF_REQUIRE(strcmp(template, path) != 0);
	ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);

	(void)memset(template, '\0', tlen);
	(void)strcpy(template, "/tmp/mktemp.");

	for (i = 12; i < tlen - 1; i++)
		template[i] = 'X';

	ATF_REQUIRE(mktemp(template) != NULL);
	ATF_REQUIRE(strlen(template) == tlen - 1);
	ATF_REQUIRE(strcmp(template, path) != 0);
	ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
	free(template);
}

ATF_TC(mkstemp_basic);
ATF_TC_HEAD(mkstemp_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mkstemp(3)");
}

ATF_TC_BODY(mkstemp_basic, tc)
{
	char template[] = "/tmp/mktemp.XXXXXX";
	struct stat sa;
	int fd;

	(void)memset(&sa, 0, sizeof(struct stat));

	fd = mkstemp(template);

	ATF_REQUIRE(fd != -1);
	ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
	ATF_REQUIRE(write(fd, "X", 1) == 1);
	ATF_REQUIRE(fstat(fd, &sa) == 0);
	ATF_REQUIRE(check_mode(sa, 0600, 0) == 0);
	ATF_REQUIRE(close(fd) == 0);
	ATF_REQUIRE(unlink(template) == 0);
}

ATF_TC(mkstemps_basic);
ATF_TC_HEAD(mkstemps_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mkstemps(3)");
}

ATF_TC_BODY(mkstemps_basic, tc)
{
	char template[] = "/tmp/mktemp.XXXyyy";
	char *suffix = strchr(template, 'y');
	struct stat sa;
	int fd;

	(void)memset(&sa, 0, sizeof(struct stat));

	fd = mkstemps(template, 3);

	ATF_REQUIRE(fd != -1);
	ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
	ATF_REQUIRE(strcmp(suffix, "yyy") == 0);
	ATF_REQUIRE(write(fd, "X", 1) == 1);
	ATF_REQUIRE(fstat(fd, &sa) == 0);
	ATF_REQUIRE(check_mode(sa, 0600, 0) == 0);
	ATF_REQUIRE(close(fd) == 0);
	ATF_REQUIRE(unlink(template) == 0);
}

ATF_TC(mkdtemp_basic);
ATF_TC_HEAD(mkdtemp_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mkdtemp(3)");
}

ATF_TC_BODY(mkdtemp_basic, tc)
{
	char template[] = "/tmp/mktemp.XXXXXX";
	char *path = mkdtemp(template);
	struct stat sa;

	(void)memset(&sa, 0, sizeof(struct stat));

	ATF_REQUIRE(path != NULL);
	ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
	ATF_REQUIRE(stat(path, &sa) == 0);
	ATF_REQUIRE(check_mode(sa, 0700, 1) == 0);
	ATF_REQUIRE(rmdir(path) == 0);
}

ATF_TC(mkostemp_basic);
ATF_TC_HEAD(mkostemp_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mkostemp(3)");
}

ATF_TC_BODY(mkostemp_basic, tc)
{
	static const int flags[] = {
		O_APPEND, O_DIRECT,
		O_SHLOCK, O_EXLOCK,
		O_SYNC,   O_CLOEXEC
	};

	char template[] = "/tmp/mktemp.XXXXXX";
	struct stat sa;
	size_t i;
	int fd;

	for (i = 0; i < __arraycount(flags); i++) {

		(void)memset(&sa, 0, sizeof(struct stat));

		fd = mkostemp(template, flags[i]);

		ATF_REQUIRE(fd != -1);
		ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
		ATF_REQUIRE(write(fd, "X", 1) == 1);
		ATF_REQUIRE(fstat(fd, &sa) == 0);
		ATF_REQUIRE(check_mode(sa, 0600 | flags[i], 0) == 0);
		ATF_REQUIRE(close(fd) == 0);
		ATF_REQUIRE(unlink(template) == 0);
	}
}

ATF_TC(mkostemps_basic);
ATF_TC_HEAD(mkostemps_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mkostemps(3)");
}

ATF_TC_BODY(mkostemps_basic, tc)
{
	static const int flags[] = {
		O_APPEND, O_DIRECT,
		O_SHLOCK, O_EXLOCK,
		O_SYNC,   O_CLOEXEC
	};

	char template[] = "/tmp/mktemp.XXXyyy";
	char *suffix = strchr(template, 'y');
	struct stat sa;
	size_t i;
	int fd;

	for (i = 0; i < __arraycount(flags); i++) {

		(void)memset(&sa, 0, sizeof(struct stat));

		fd = mkostemps(template, 3, flags[i]);

		ATF_REQUIRE(fd != -1);
		ATF_REQUIRE(strncmp(template, "/tmp/mktemp.", 12) == 0);
		ATF_REQUIRE(strcmp(suffix, "yyy") == 0);
		ATF_REQUIRE(write(fd, "X", 1) == 1);
		ATF_REQUIRE(fstat(fd, &sa) == 0);
		ATF_REQUIRE(check_mode(sa, 0600 | flags[i], 0) == 0);
		ATF_REQUIRE(close(fd) == 0);
		ATF_REQUIRE(unlink(template) == 0);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mktemp_not_exist);
	ATF_TP_ADD_TC(tp, mktemp_large_template);
	ATF_TP_ADD_TC(tp, mkstemp_basic);
	ATF_TP_ADD_TC(tp, mkstemps_basic);
	ATF_TP_ADD_TC(tp, mkdtemp_basic);
	ATF_TP_ADD_TC(tp, mkostemp_basic);
	ATF_TP_ADD_TC(tp, mkostemps_basic);

	return atf_no_error();
}
