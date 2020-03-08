/* $NetBSD: t_catalog.c,v 1.1 2020/03/08 22:08:46 mgorny Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_catalog.c,v 1.1 2020/03/08 22:08:46 mgorny Exp $");

#include <atf-c.h>
#include <errno.h>
#include <stdio.h>	/* Needed for sys_nerr on FreeBSD */
#include <limits.h>
#include <locale.h>
#include <nl_types.h>
#include <signal.h>
#include <string.h>

ATF_TC(catalog_errno);
ATF_TC_HEAD(catalog_errno, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test whether C catalog covers all "
	    "errno values");
}

ATF_TC_BODY(catalog_errno, tc)
{
	int i;
	nl_catd catd = catopen("libc", NL_CAT_LOCALE);
	ATF_REQUIRE(catd);

	for (i = 1; i < sys_nerr; i++) {
		const char *strerr = sys_errlist[i];
		const char *caterr = catgets(catd, 1, i, "");
		ATF_CHECK_MSG(!strcmp(strerr, caterr),
		    "Catalog message mismatch for errno=%d (sys_errlist: '%s', "
		    "catalog: '%s')\n", i, strerr, caterr);
	}

	catclose(catd);
}

ATF_TC(catalog_signal);
ATF_TC_HEAD(catalog_signal, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test whether C catalog covers all "
	    "signal values");
}

ATF_TC_BODY(catalog_signal, tc)
{
	int i;
	nl_catd catd = catopen("libc", NL_CAT_LOCALE);
	ATF_REQUIRE(catd);

	for (i = 1; i < SIGRTMIN-1; i++) {
		const char *strerr = sys_siglist[i];
		const char *caterr = catgets(catd, 2, i, "");
		ATF_CHECK_MSG(!strcmp(strerr, caterr),
		    "Catalog message mismatch for signal=%d (sys_siglist: '%s', "
		    "catalog: '%s')\n", i, strerr, caterr);
	}

	catclose(catd);
}

ATF_TP_ADD_TCS(tp)
{
	(void)setlocale(LC_ALL, "C");

	ATF_TP_ADD_TC(tp, catalog_errno);
	ATF_TP_ADD_TC(tp, catalog_signal);

	return atf_no_error();
}

