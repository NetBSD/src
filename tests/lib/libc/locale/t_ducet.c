/* $NetBSD: t_ducet.c,v 1.2 2017/07/23 18:51:21 perseant Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2011\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_ducet.c,v 1.2 2017/07/23 18:51:21 perseant Exp $");

#include <sys/param.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ducet_test.h"

#include <atf-c.h>

ATF_TC(wcscoll_ducet);
ATF_TC_HEAD(wcscoll_ducet, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test collation algorithm against DUCET test data");
}

ATF_TC_BODY(wcscoll_ducet, tc)
{
#ifndef __STDC_ISO_10646__
	atf_tc_skip("Cannot test DUCET without __STDC_ISO_10646__");
#else
	wchar_t *oline = NULL, *line;
	int i, lineno = 0;

	setlocale(LC_COLLATE, "en_US.UTF-8"); /* should be "vanilla" DUCET, but en_US will do */
	ATF_REQUIRE_STREQ("en_US.UTF-8", setlocale(LC_COLLATE, NULL));

	for (line = &ducet_test_data[0][0]; line[0]; line += MAX_TS_LEN * sizeof(*line)) {
		++lineno;

		if (oline == NULL) {
			oline = line;
			continue;
		}

		printf(" line %d : ", lineno);
		for (i = 0; oline[i] != 0; i++)
			printf("0x%lx ", (long)oline[i]);
		printf(" < ");
		for (i = 0; line[i] != 0; i++)
			printf("0x%lx ", (long)line[i]);
		printf("\n");
		
		/* Compare, expect oline <= line, a non-positive result */
		ATF_CHECK(wcscoll(oline, line) <= 0);

		oline = line;
	}
#endif
}

ATF_TC(wcsxfrm_ducet);
ATF_TC_HEAD(wcsxfrm_ducet, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test collation algorithm against DUCET test data");
}

#define BUFLEN 1024

ATF_TC_BODY(wcsxfrm_ducet, tc)
{
#ifndef __STDC_ISO_10646__
	atf_tc_skip("Cannot test DUCET without __STDC_ISO_10646__");
#else
	wchar_t *tmp, *oline = NULL, *line;
	wchar_t xfb1[BUFLEN], xfb2[BUFLEN]; /* Gross overestimates */
	wchar_t *oxf = xfb1, *xf = xfb2;
	int i, lineno = 0, result;

	setlocale(LC_COLLATE, "en_US.UTF-8"); /* should be "vanilla" DUCET, but en_US will do */
	ATF_REQUIRE_STREQ("en_US.UTF-8", setlocale(LC_COLLATE, NULL));

	memset(xfb1, 0, BUFLEN);
	memset(xfb2, 0, BUFLEN);
	for (line = &ducet_test_data[0][0]; line[0]; line += MAX_TS_LEN * sizeof(*line)) {
		++lineno;

		if (oline == NULL) {
			oline = line;
			continue;
		}

		printf(" line %d : ", lineno);
		for (i = 0; oline[i] != 0; i++)
			printf("0x%lx ", (long)oline[i]);
		printf(" < ");
		for (i = 0; line[i] != 0; i++)
			printf("0x%lx ", (long)line[i]);
		printf("\n");
		
		wcsxfrm(xf, line, BUFLEN);
		result = wcscmp(oxf, xf);
		if (result > 0) {
			printf("FAILED result was %d\nweights were ", result);
			for (i = 0; oxf[i] != 0; i++)
				printf("0x%lx ", (long)oline[i]);
			printf(" and ");
			for (i = 0; xf[i] != 0; i++)
				printf("0x%lx ", (long)line[i]);
			printf("\n");
			
		}
		ATF_CHECK(result <= 0);

		/* Swap buffers */
		tmp = oxf;
		oxf = xf;
		xf = tmp;
		oline = line;
	}
#endif
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, wcscoll_ducet);
	ATF_TP_ADD_TC(tp, wcsxfrm_ducet);

	return atf_no_error();
}
