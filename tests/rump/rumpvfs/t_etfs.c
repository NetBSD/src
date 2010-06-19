/*	$NetBSD: t_etfs.c,v 1.4 2010/06/19 13:44:11 pooka Exp $	*/

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
#include <sys/sysctl.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <atf-c.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../h_macros.h"

ATF_TC(reregister_reg);
ATF_TC_HEAD(reregister_reg, tc)
{

	atf_tc_set_md_var(tc, "descr", "Tests register/unregister/register "
	    "for a regular file");
	atf_tc_set_md_var(tc, "use.fs", "true");
}

#define TESTSTR1 "hi, it's me again!"
#define TESTSTR1SZ (sizeof(TESTSTR1)-1)

#define TESTSTR2 "what about the old vulcan proverb?"
#define TESTSTR2SZ (sizeof(TESTSTR2)-1)

#define TESTPATH1 "/trip/to/the/moon"
#define TESTPATH2 "/but/not/the/dark/size"
ATF_TC_BODY(reregister_reg, tc)
{
	char buf[1024];
	int localfd, etcfd;
	ssize_t n;
	int tfd;

	etcfd = open("/etc/passwd", O_RDONLY);
	ATF_REQUIRE(etcfd != -1);

	localfd = open("./testfile", O_RDWR | O_CREAT, 0666);
	ATF_REQUIRE(localfd != -1);

	ATF_REQUIRE_EQ(write(localfd, TESTSTR1, TESTSTR1SZ), TESTSTR1SZ);
	/* testfile now contains test string */

	rump_init();

	ATF_REQUIRE_EQ(rump_pub_etfs_register(TESTPATH1, "/etc/passwd",
	    RUMP_ETFS_REG), 0);
	tfd = rump_sys_open(TESTPATH1, O_RDONLY);
	ATF_REQUIRE(tfd != -1);
	ATF_REQUIRE(rump_sys_read(tfd, buf, sizeof(buf)) > 0);
	rump_sys_close(tfd);
	rump_pub_etfs_remove(TESTPATH1);

	ATF_REQUIRE_EQ(rump_pub_etfs_register(TESTPATH2, "./testfile",
	    RUMP_ETFS_REG), 0);
	tfd = rump_sys_open(TESTPATH2, O_RDWR);
	ATF_REQUIRE(tfd != -1);
	memset(buf, 0, sizeof(buf));
	ATF_REQUIRE((n = rump_sys_read(tfd, buf, sizeof(buf))) > 0);

	/* check that we have what we expected */
	ATF_REQUIRE_STREQ(buf, TESTSTR1);

	/* ... while here, check that writing works too */
	ATF_REQUIRE_EQ(rump_sys_lseek(tfd, 0, SEEK_SET), 0);
	ATF_REQUIRE(TESTSTR1SZ <= TESTSTR2SZ);
	ATF_REQUIRE_EQ(rump_sys_write(tfd, TESTSTR2, TESTSTR2SZ), TESTSTR2SZ);

	memset(buf, 0, sizeof(buf));
	ATF_REQUIRE_EQ(lseek(localfd, 0, SEEK_SET), 0);
	ATF_REQUIRE(read(localfd, buf, sizeof(buf)) > 0);
	ATF_REQUIRE_STREQ(buf, TESTSTR2);
}

ATF_TC(reregister_blk);
ATF_TC_HEAD(reregister_blk, tc)
{

	atf_tc_set_md_var(tc, "descr", "Tests register/unregister/register "
	    "for a block device");
	atf_tc_set_md_var(tc, "use.fs", "true");
}

ATF_TC_BODY(reregister_blk, tc)
{
	char buf[512 * 128];
	char cmpbuf[512 * 128];
	int rv, tfd;

	/* first, create some image files */
	rv = system("dd if=/dev/zero bs=512 count=64 "
	    "| tr '\\0' '\\1' > disk1.img");
	ATF_REQUIRE_EQ(rv, 0);

	rv = system("dd if=/dev/zero bs=512 count=128 "
	    "| tr '\\0' '\\2' > disk2.img");
	ATF_REQUIRE_EQ(rv, 0);

	rump_init();

	ATF_REQUIRE_EQ(rump_pub_etfs_register(TESTPATH1, "./disk1.img",
	    RUMP_ETFS_BLK), 0);
	tfd = rump_sys_open(TESTPATH1, O_RDONLY);
	ATF_REQUIRE(tfd != -1);
	ATF_REQUIRE_EQ(rump_sys_read(tfd, buf, sizeof(buf)), 64*512);
	memset(cmpbuf, 1, sizeof(cmpbuf));
	ATF_REQUIRE_EQ(memcmp(buf, cmpbuf, 64*512), 0);
	ATF_REQUIRE_EQ(rump_sys_close(tfd), 0);
	ATF_REQUIRE_EQ(rump_pub_etfs_remove(TESTPATH1), 0);

	ATF_REQUIRE_EQ(rump_pub_etfs_register(TESTPATH2, "./disk2.img",
	    RUMP_ETFS_BLK), 0);
	tfd = rump_sys_open(TESTPATH2, O_RDONLY);
	ATF_REQUIRE(tfd != -1);
	ATF_REQUIRE_EQ(rump_sys_read(tfd, buf, sizeof(buf)), 128*512);
	memset(cmpbuf, 2, sizeof(cmpbuf));
	ATF_REQUIRE_EQ(memcmp(buf, cmpbuf, 128*512), 0);
	ATF_REQUIRE_EQ(rump_sys_close(tfd), 0);
	ATF_REQUIRE_EQ(rump_pub_etfs_remove(TESTPATH2), 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, reregister_reg);
	ATF_TP_ADD_TC(tp, reregister_blk);

	return atf_no_error();
}
