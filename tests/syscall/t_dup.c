/* $NetBSD: t_dup.c,v 1.2 2011/04/03 16:22:15 jruoho Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
__RCSID("$NetBSD: t_dup.c,v 1.2 2011/04/03 16:22:15 jruoho Exp $");

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atf-c.h>

static char	 path[] = "/tmp/dup";

ATF_TC(dup_err);
ATF_TC_HEAD(dup_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test error conditions of dup(2)");
}

ATF_TC_BODY(dup_err, tc)
{
	ATF_REQUIRE(dup(-1) != 0);
}

ATF_TC_WITH_CLEANUP(dup_max);
ATF_TC_HEAD(dup_max, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test dup(2) against limits");
}

ATF_TC_BODY(dup_max, tc)
{
	int current, fd, *buf, serrno;
	long i, maxfd;

	/*
	 * Open a temporary file until the
	 * maximum number of open files is
	 * reached. Ater that dup(2) should
	 * fail with EMFILE.
	 */
	maxfd = sysconf(_SC_OPEN_MAX);
	ATF_REQUIRE(maxfd >= 0);

	buf = calloc(maxfd, sizeof(int));

	if (buf == NULL)
		return;

	buf[0] = mkstemp(path);
	ATF_REQUIRE(buf[0] != -1);

	current = fcntl(0, F_MAXFD);
	ATF_REQUIRE(current != -1);

	fd = -1;
	serrno = EMFILE;

	for (i = current; i <= maxfd; i++) {

		buf[i] = open(path, O_RDONLY);

		if (buf[i] < 0)
			goto out;
	}

	errno = 0;
	fd = dup(buf[0]);
	serrno = errno;

out:
	for (i = 0; i <= maxfd; i++)
		(void)close(buf[i]);

	free(buf);

	if (fd != -1 || serrno != EMFILE)
		atf_tc_fail("dup(2) dupped more than _SC_OPEN_MAX");
}

ATF_TC_CLEANUP(dup_max, tc)
{
	(void)unlink(path);
}

ATF_TC_WITH_CLEANUP(dup_mode);
ATF_TC_HEAD(dup_mode, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of dup(2)");
}

ATF_TC_BODY(dup_mode, tc)
{
	int mode[3] = { O_RDONLY, O_WRONLY, O_RDWR   };
	int perm[5] = { 0700, 0400, 0600, 0444, 0666 };
	struct stat st1, st2;
	int fd1, fd2;
	size_t i, j;

	/*
	 * Check that a duplicated descriptor
	 * retains the mode of the original file.
	 */
	for (i = 0; i < __arraycount(mode); i++) {

		for (j = 0; j < __arraycount(perm); j++) {

			fd1 = open(path, mode[i] | O_CREAT, perm[j]);

			if (fd1 < 0)
				return;

			fd2 = dup(fd1);
			ATF_REQUIRE(fd2 >= 0);

			(void)memset(&st1, 0, sizeof(struct stat));
			(void)memset(&st2, 0, sizeof(struct stat));

			ATF_REQUIRE(fstat(fd1, &st1) == 0);
			ATF_REQUIRE(fstat(fd2, &st2) == 0);

			if (st1.st_mode != st2.st_mode)
				atf_tc_fail("invalid mode");

			ATF_REQUIRE(close(fd1) == 0);
			ATF_REQUIRE(close(fd2) == 0);
			ATF_REQUIRE(unlink(path) == 0);
		}
	}
}

ATF_TC_CLEANUP(dup_mode, tc)
{
	(void)unlink(path);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dup_err);
	ATF_TP_ADD_TC(tp, dup_max);
	ATF_TP_ADD_TC(tp, dup_mode);

	return atf_no_error();
}
