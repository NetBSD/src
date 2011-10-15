/* $NetBSD: t_mkdir.c,v 1.1 2011/10/15 06:26:33 jruoho Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_mkdir.c,v 1.1 2011/10/15 06:26:33 jruoho Exp $");

#include <sys/stat.h>

#include <atf-c.h>
#include <stdio.h>
#include <unistd.h>

ATF_TC(mkdir_trail);
ATF_TC_HEAD(mkdir_trail, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mkdir(2) for trailing slashes");
}

ATF_TC_BODY(mkdir_trail, tc)
{
	const char *tests[] = {
		/*
		 * IEEE 1003.1 second ed. 2.2.2.78:
		 *
		 * If the pathname refers to a directory, it may also have
		 * one or more trailing slashes.  Multiple successive slashes
		 * are considered to be the same as one slash.
		 */
		"dir1/",
		"dir2//",

		NULL,
	};

	const char **test;

	for (test = &tests[0]; *test != NULL; ++test) {

		(void)printf("Checking \"%s\"\n", *test);
		(void)rmdir(*test);

		ATF_REQUIRE(mkdir(*test, 0777) == 0);
		ATF_REQUIRE(rename(*test, "foo") == 0);
		ATF_REQUIRE(rename("foo/", *test) == 0);
		ATF_REQUIRE(rmdir(*test) == 0);
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mkdir_trail);

	return atf_no_error();
}
