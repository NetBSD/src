/*	$NetBSD: t_o_search.c,v 1.3.2.2 2013/01/16 05:33:59 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__RCSID("$NetBSD: t_o_search.c,v 1.3.2.2 2013/01/16 05:33:59 yamt Exp $");

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/param.h>

#define DIR "dir"
#define FILE "dir/o_search"
#define BASEFILE "o_search"

ATF_TC_WITH_CLEANUP(o_search_perm1);
ATF_TC_HEAD(o_search_perm1, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that openat enforce search permission");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(o_search_perm1, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY, 0)) != -1);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fchmod(dfd, 644) == 0);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) == -1);
	ATF_REQUIRE(errno == EACCES);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_perm1, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_root_flag1);
ATF_TC_HEAD(o_search_root_flag1, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that openat honours O_SEARCH");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(o_search_root_flag1, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY|O_SEARCH, 0)) != -1);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fchmod(dfd, 644) == 0);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fchmod(dfd, 444) == 0);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_root_flag1, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_unpriv_flag1);
ATF_TC_HEAD(o_search_unpriv_flag1, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that openat honours O_SEARCH");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(o_search_unpriv_flag1, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY|O_SEARCH, 0)) != -1);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fchmod(dfd, 744) == 0);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE(fchmod(dfd, 444) == 0);

	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) == -1);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_unpriv_flag1, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_perm2);
ATF_TC_HEAD(o_search_perm2, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that fstatat enforce search permission");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(o_search_perm2, tc)
{
	int dfd;
	int fd;
	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY, 0)) != -1);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(fchmod(dfd, 644) == 0);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == -1);
	ATF_REQUIRE(errno == EACCES);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_perm2, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_root_flag2);
ATF_TC_HEAD(o_search_root_flag2, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that fstatat honours O_SEARCH");
	atf_tc_set_md_var(tc, "require.user", "root");
}

ATF_TC_BODY(o_search_root_flag2, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY|O_SEARCH, 0)) != -1);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(fchmod(dfd, 644) == 0);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(fchmod(dfd, 444) == 0);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_root_flag2, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_unpriv_flag2);
ATF_TC_HEAD(o_search_unpriv_flag2, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that fstatat honours O_SEARCH");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}

ATF_TC_BODY(o_search_unpriv_flag2, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((fd = open(FILE, O_CREAT|O_RDWR, 0644)) != -1);
	ATF_REQUIRE(close(fd) == 0);

	ATF_REQUIRE((dfd = open(DIR, O_RDONLY|O_SEARCH, 0)) != -1);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(fchmod(dfd, 744) == 0);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(fchmod(dfd, 444) == 0);

	ATF_REQUIRE(faccessat(dfd, BASEFILE, W_OK, 0) == 0);

	ATF_REQUIRE(close(dfd) == 0);
}

ATF_TC_CLEANUP(o_search_unpriv_flag2, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 

ATF_TC_WITH_CLEANUP(o_search_notdir);
ATF_TC_HEAD(o_search_notdir, tc)
{
	atf_tc_set_md_var(tc, "descr", "See that openat fails with non dir fd");
}

ATF_TC_BODY(o_search_notdir, tc)
{
	int dfd;
	int fd;

	ATF_REQUIRE(mkdir(DIR, 0755) == 0);
	ATF_REQUIRE((dfd = open(FILE, O_CREAT|O_RDWR|O_SEARCH, 0644)) != -1);
	ATF_REQUIRE((fd = openat(dfd, BASEFILE, O_RDWR, 0)) == -1);
	ATF_REQUIRE(errno == ENOTDIR);
}

ATF_TC_CLEANUP(o_search_notdir, tc)
{
	(void)unlink(FILE);
	(void)rmdir(DIR);
} 



ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, o_search_perm1);
	ATF_TP_ADD_TC(tp, o_search_root_flag1);
	ATF_TP_ADD_TC(tp, o_search_unpriv_flag1);
	ATF_TP_ADD_TC(tp, o_search_perm2);
	ATF_TP_ADD_TC(tp, o_search_root_flag2);
	ATF_TP_ADD_TC(tp, o_search_unpriv_flag2);
	ATF_TP_ADD_TC(tp, o_search_notdir);

	return atf_no_error();
}
