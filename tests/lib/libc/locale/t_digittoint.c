/* $NetBSD: t_digittoint.c,v 1.3 2022/05/24 20:50:20 andvar Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad Schroder
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
__COPYRIGHT("@(#) Copyright (c) 2017\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_digittoint.c,v 1.3 2022/05/24 20:50:20 andvar Exp $");

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <vis.h>
#include <ctype.h>

#include <atf-c.h>

/* Use this until we have a better way to tell if it is defined */
#ifdef digittoint
# define DIGITTOINT_DEFINED
#endif

static struct test {
	const char *locale;
	const char *digits;
} tests[] = {
	{
		"C",
		"0123456789AbcDeF",
	}, {
		"en_US.UTF-8",
		"0123456789AbcDeF",
	}, {
		"ru_RU.KOI-8",
		"0123456789AbcDeF",
	}, {
		NULL,
		NULL,
	}
};

#ifdef DIGITTOINT_DEFINED
static void
h_digittoint(const struct test *t)
{
	int i;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	printf("Trying locale %s...\n", t->locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, t->locale) != NULL);

	for (i = 0; i < 16; i++) {
		printf(" char %2.2x in position %d\n", t->digits[i], i);
		ATF_REQUIRE_EQ(digittoint(t->digits[i]), i);
	}
}
#endif /* DIGITTOINT_DEFINED */

ATF_TC(digittoint);

ATF_TC_HEAD(digittoint, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Checks digittoint under different locales");
}

ATF_TC_BODY(digittoint, tc)
{
	struct test *t;

#ifdef DIGITTOINT_DEFINED
	for (t = &tests[0]; t->locale != NULL; ++t)
		h_digittoint(t);
#else /* ! DIGITTOINT_DEFINED */
	atf_tc_skip("digittoint(3) not present to test");
#endif /* DIGITTOINT_DEFINED */
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, digittoint);

	return atf_no_error();
}
