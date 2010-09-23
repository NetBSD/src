/*	$NetBSD: t_environment.c,v 1.1 2010/09/23 17:38:08 christos Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_environment.c,v 1.1 2010/09/23 17:38:08 christos Exp $");

#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ATF_TC(t_environment);

ATF_TC_HEAD(t_environment, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test setenv(3), getenv(3), unsetenv(3)");
}

ATF_TC_BODY(t_environment, tc)
{
	size_t i;
	char name[1024];
	char value[1024];
	for (i = 0; i < 10240; i++) {
		snprintf(name, sizeof(name), "var%zu", i);
		snprintf(value, sizeof(value), "value%ld", lrand48());
		setenv(name, value, 1);
		ATF_CHECK_STREQ(getenv(name), value);
	}

	for (i = 0; i < 10240; i++) {
		snprintf(name, sizeof(name), "var%zu", i);
		unsetenv(name);
		ATF_CHECK(getenv(name) == NULL);
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_environment);

	return atf_no_error();
}
