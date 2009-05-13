/* $NetBSD: t_mkdir.c,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $ */

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
__RCSID("$NetBSD: t_mkdir.c,v 1.1.2.2 2009/05/13 19:19:33 jym Exp $");

#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>

#include <atf-c.h>

#include "../h_macros.h"

ATF_TC(mkdir);
ATF_TC_HEAD(mkdir, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks mkdir(2)");
}
ATF_TC_BODY(mkdir, tc)
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

		RL(mkdir(*test, 0777));
		RL(rename(*test, "foo"));
		RL(rename("foo/", *test));
		RL(rmdir(*test));
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mkdir);

	return atf_no_error();
}
