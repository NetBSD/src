/*	$NetBSD: t_environment.c,v 1.9 2010/11/13 21:08:36 tron Exp $	*/
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
__RCSID("$NetBSD: t_environment.c,v 1.9 2010/11/13 21:08:36 tron Exp $");

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ATF_TC(t_setenv);
ATF_TC(t_putenv);
ATF_TC(t_clearenv);
ATF_TC(t_mixed);
ATF_TC(t_getenv);

ATF_TC_HEAD(t_setenv, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test setenv(3), getenv(3), unsetenv(3)");
	atf_tc_set_md_var(tc, "timeout", "300");
}

ATF_TC_HEAD(t_putenv, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test putenv(3), getenv(3), unsetenv(3)");
}

ATF_TC_HEAD(t_clearenv, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test user clearing environment directly");
}

ATF_TC_HEAD(t_mixed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test mixing setenv(3), unsetenv(3) and putenv(3)");
}

ATF_TC_HEAD(t_getenv, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test setenv(3), getenv(3)");
}

ATF_TC_BODY(t_setenv, tc)
{
	size_t i;
	char name[1024];
	char value[1024];
	for (i = 0; i < 10240; i++) {
		snprintf(name, sizeof(name), "var%zu", i);
		snprintf(value, sizeof(value), "value%ld", lrand48());
		ATF_CHECK(setenv(name, value, 1) != -1);
		ATF_CHECK(setenv(name, "foo", 0) != -1);
		ATF_CHECK_STREQ(getenv(name), value);
	}

	for (i = 0; i < 10240; i++) {
		snprintf(name, sizeof(name), "var%zu", i);
		ATF_CHECK(unsetenv(name) != -1);
		ATF_CHECK(getenv(name) == NULL);
	}

	ATF_CHECK_ERRNO(EINVAL, setenv(NULL, "val", 1) == -1);
	ATF_CHECK_ERRNO(EINVAL, setenv("", "val", 1) == -1);
	ATF_CHECK_ERRNO(EINVAL, setenv("v=r", "val", 1) == -1);
	ATF_CHECK_ERRNO(EINVAL, setenv("var", NULL, 1) == -1);

	ATF_CHECK(setenv("var", "=val", 1) == 0);
	ATF_CHECK_STREQ(getenv("var"), "=val");
}

ATF_TC_BODY(t_putenv, tc)
{
	char string[1024];

	snprintf(string, sizeof(string), "crap=true");
	ATF_CHECK(putenv(string) != -1);
	ATF_CHECK_STREQ(getenv("crap"), "true");
	string[1] = 'l';
	ATF_CHECK_STREQ(getenv("clap"), "true");
	ATF_CHECK(getenv("crap") == NULL);
	string[1] = 'r';
	ATF_CHECK(unsetenv("crap") != -1);
	ATF_CHECK(getenv("crap") == NULL);

	ATF_CHECK_ERRNO(EINVAL, putenv(NULL) == -1);
	ATF_CHECK_ERRNO(EINVAL, putenv(__UNCONST("val")) == -1);
	ATF_CHECK_ERRNO(EINVAL, putenv(__UNCONST("=val")) == -1);
}

extern char **environ;

ATF_TC_BODY(t_clearenv, tc)
{
	char name[1024], value[1024];

	for (size_t i = 0; i < 1024; i++) {
		snprintf(name, sizeof(name), "crap%zu", i);
		snprintf(value, sizeof(value), "%zu", i);
		ATF_CHECK(setenv(name, value, 1) != -1);
	}

	*environ = NULL;

	for (size_t i = 0; i < 1; i++) {
		snprintf(name, sizeof(name), "crap%zu", i);
		snprintf(value, sizeof(value), "%zu", i);
		ATF_CHECK(setenv(name, value, 1) != -1);
	}

	ATF_CHECK_STREQ(getenv("crap0"), "0");
	ATF_CHECK(getenv("crap1") == NULL);
	ATF_CHECK(getenv("crap2") == NULL);
}

ATF_TC_BODY(t_mixed, tc)
{
	char string[32];

	(void)strcpy(string, "mixedcrap=putenv");

	ATF_CHECK(setenv("mixedcrap", "setenv", 1) != -1);
	ATF_CHECK_STREQ(getenv("mixedcrap"), "setenv");
	ATF_CHECK(putenv(string) != -1);
	ATF_CHECK_STREQ(getenv("mixedcrap"), "putenv");
	ATF_CHECK(unsetenv("mixedcrap") != -1);
	ATF_CHECK(getenv("mixedcrap") == NULL);

	ATF_CHECK(putenv(string) != -1);
	ATF_CHECK_STREQ(getenv("mixedcrap"), "putenv");
	ATF_CHECK(setenv("mixedcrap", "setenv", 1) != -1);
	ATF_CHECK_STREQ(getenv("mixedcrap"), "setenv");
	ATF_CHECK(unsetenv("mixedcrap") != -1);
	ATF_CHECK(getenv("mixedcrap") == NULL);
}

ATF_TC_BODY(t_getenv, tc)
{
	ATF_CHECK(setenv("EVIL", "very=bad", 1) != -1);
	ATF_CHECK_STREQ(getenv("EVIL"), "very=bad");

	atf_tc_expect_fail("getenv(3) doesn't check variable names properly");
	ATF_CHECK(getenv("EVIL=very") == NULL);

	atf_tc_expect_pass();
	ATF_CHECK(unsetenv("EVIL") != -1);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_setenv);
	ATF_TP_ADD_TC(tp, t_putenv);
	ATF_TP_ADD_TC(tp, t_clearenv);
	ATF_TP_ADD_TC(tp, t_mixed);
	ATF_TP_ADD_TC(tp, t_getenv);

	return atf_no_error();
}
