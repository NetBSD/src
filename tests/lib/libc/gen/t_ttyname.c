/*	$NetBSD: t_ttyname.c,v 1.1 2011/04/05 06:15:31 jruoho Exp $ */

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
__RCSID("$NetBSD: t_ttyname.c,v 1.1 2011/04/05 06:15:31 jruoho Exp $");

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

ATF_TC(ttyname_err);
ATF_TC_HEAD(ttyname_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors in ttyname(3)");
}

ATF_TC_BODY(ttyname_err, tc)
{
	char buf[0];
	int fd, rv;

	fd = open("XXX", O_RDONLY);

	if (fd < 0) {

		errno = 0;

		ATF_REQUIRE(isatty(fd) != -1);
		ATF_REQUIRE(errno == EBADF);

		errno = 0;

		ATF_REQUIRE(ttyname(fd) == NULL);
		ATF_REQUIRE(errno == EBADF);
	}

	fd = open("/etc/passwd", O_RDONLY);

	if (fd >= 0) {

		errno = 0;

		ATF_REQUIRE(isatty(fd) != -1);
		ATF_REQUIRE(errno == ENOTTY);

		errno = 0;

		ATF_REQUIRE(ttyname(fd) == NULL);
		ATF_REQUIRE(errno == ENOTTY);
	}

	if (isatty(STDIN_FILENO) != 0) {

		rv = ttyname_r(STDIN_FILENO, buf, sizeof(buf));
		ATF_REQUIRE(rv == ERANGE);
	}
}

ATF_TC(ttyname_stdin);
ATF_TC_HEAD(ttyname_stdin, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test ttyname(3) with stdin(3)");
}

ATF_TC_BODY(ttyname_stdin, tc)
{

	ATF_REQUIRE(isatty(STDIN_FILENO) == 1);
	ATF_REQUIRE(ttyname(STDIN_FILENO) != NULL);

	ATF_REQUIRE(close(STDIN_FILENO) == 0);

	ATF_REQUIRE(isatty(STDIN_FILENO) != 1);
	ATF_REQUIRE(ttyname(STDIN_FILENO) == NULL);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ttyname_err);
	ATF_TP_ADD_TC(tp, ttyname_stdin);

	return atf_no_error();
}
