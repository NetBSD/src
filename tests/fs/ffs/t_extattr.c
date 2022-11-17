/*	$NetBSD: t_extattr.c,v 1.3 2022/11/17 06:40:40 chs Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: t_extattr.c,v 1.3 2022/11/17 06:40:40 chs Exp $");

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/extattr.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <ufs/ufs/ufsmount.h>

#include "h_macros.h"

#define IMGNAME "extattr.img"

#define G "/garage"
#define M "mercedes"
#define C "carburator"
#define E "engine"
#define T "trunk"
#define S "suitcase"

static const char *attrs[] = { E, T };

static void
check_list(const char *buf, ssize_t nr)
{
	size_t p = 0;
	for (size_t i = 0; i < __arraycount(attrs); i++) {
		size_t l = buf[p++];
		if (l != strlen(attrs[i]))
		    atf_tc_fail("got invalid len %zu expected %zu", l,
			strlen(attrs[i]));
		if (strncmp(&buf[p], attrs[i], l) != 0)
			atf_tc_fail("got invalid str %.*s expected %s", (int)l,
			    &buf[p], attrs[i]);
		p += l;
	}
}

// Make it ffsv2 with extattrs
const char *newfs = "newfs -O 2ea -F -s 10000 " IMGNAME;
#define FAKEBLK "/dev/formula1"

static void
start(void) {
	struct ufs_args args;

	if (system(newfs) == -1)
		atf_tc_fail_errno("newfs failed");

	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(FAKEBLK);

	rump_init();
	if (rump_sys_mkdir(G, 0777) == -1)
		atf_tc_fail_errno("cannot create mountpoint");
	rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);
	if (rump_sys_mount(MOUNT_FFS, G, 0, &args, sizeof(args))==-1)
		atf_tc_fail_errno("rump_sys_mount failed");

	/* create extattr */
	if (rump_sys_chdir(G) == 1)
		atf_tc_fail_errno("chdir");
}

static void
finish(void) {
	if (rump_sys_chdir("/") == 1)
		atf_tc_fail_errno("chdir");
	if (rump_sys_unmount(G, 0) == -1)
		atf_tc_fail_errno("unmount failed");
}


ATF_TC_WITH_CLEANUP(extattr_simple);
ATF_TC_HEAD(extattr_simple, tc)
{
	atf_tc_set_md_var(tc, "descr", "test extended attribute creation"
	    " and removal ffsv2");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(extattr_simple, tc)
{
	ssize_t nr;
	int fd;
	char buf[512];

	start();

	if ((fd = rump_sys_open(M, O_RDWR | O_CREAT, 0600)) == -1)
		atf_tc_fail_errno("open");
	if (rump_sys_write(fd, "hi mom\n", 7) != 7)
		atf_tc_fail_errno("write");

	if (rump_sys_extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, E, C, 11) == -1)
		atf_tc_fail_errno("exattr_set_fd");
	if ((nr = rump_sys_extattr_get_file(M, EXTATTR_NAMESPACE_USER,
	    E, buf, sizeof(buf))) != 11)
		atf_tc_fail_errno("extattr_get_file");
	if (strcmp(buf, C) != 0)
		atf_tc_fail("got invalid str %s expected %s", buf, C);

	if (rump_sys_extattr_set_file(M, EXTATTR_NAMESPACE_USER, T, S, 9) == -1)
		atf_tc_fail_errno("extattr_set_file");
	if ((nr = rump_sys_extattr_get_fd(fd, EXTATTR_NAMESPACE_USER,
	    T, buf, sizeof(buf))) != 9)
		atf_tc_fail_errno("extattr_get_fd");
	if (strcmp(buf, S) != 0)
		atf_tc_fail("got invalid str %s expected %s", buf, S);

	if ((nr = rump_sys_extattr_list_fd(fd, EXTATTR_NAMESPACE_USER,
	    buf, sizeof(buf))) != 13)
		atf_tc_fail_errno("extattr_list_fd %zd", nr);
	check_list(buf, __arraycount(attrs));

	if ((nr = rump_sys_extattr_list_file(M, EXTATTR_NAMESPACE_USER,
	    buf, sizeof(buf))) != 13)
		atf_tc_fail_errno("extattr_list_file %zd", nr);
	check_list(buf, __arraycount(attrs));

	if (rump_sys_extattr_delete_fd(fd, EXTATTR_NAMESPACE_USER, T) == -1)
		atf_tc_fail_errno("extattr_delete_fd");
	if ((nr = rump_sys_extattr_list_fd(fd, EXTATTR_NAMESPACE_USER,
	    buf, sizeof(buf))) != 7)
		atf_tc_fail_errno("extattr_list_fd %zd", nr);
	check_list(buf, 1);

	if (rump_sys_extattr_delete_file(M, EXTATTR_NAMESPACE_USER, E) == -1)
		atf_tc_fail_errno("extattr_delete_file");
	if ((nr = rump_sys_extattr_list_fd(fd, EXTATTR_NAMESPACE_USER,
	    buf, sizeof(buf))) != 0)
		atf_tc_fail_errno("extattr_list_fd %zd", nr);

	if (rump_sys_close(fd) == -1)
		atf_tc_fail_errno("close");
	if (rump_sys_unlink(M) == -1)
		atf_tc_fail_errno("unlink");

	finish();
}

ATF_TC_CLEANUP(extattr_simple, tc)
{

	unlink(IMGNAME);
}

ATF_TC_WITH_CLEANUP(extattr_create_unlink);
ATF_TC_HEAD(extattr_create_unlink, tc)
{
	atf_tc_set_md_var(tc, "descr", "test extended attribute creation"
	    " and unlinking file with ACLs");
	atf_tc_set_md_var(tc, "timeout", "5");
}

ATF_TC_BODY(extattr_create_unlink, tc)
{
	int fd;

	start();
	if ((fd = rump_sys_open(M, O_RDWR | O_CREAT, 0600)) == -1)
		atf_tc_fail_errno("open");

	if (rump_sys_close(fd) == -1)
		atf_tc_fail_errno("close");

	/* create extattr */
	if (rump_sys_extattr_set_file(M, EXTATTR_NAMESPACE_USER, T, S, 9) == -1)
		atf_tc_fail_errno("extattr_set_file");

	if (rump_sys_unlink(M) == -1)
		atf_tc_fail_errno("unlink");
	finish();

}

ATF_TC_CLEANUP(extattr_create_unlink, tc)
{

	unlink(IMGNAME);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, extattr_simple);
	ATF_TP_ADD_TC(tp, extattr_create_unlink);
	return atf_no_error();
}
