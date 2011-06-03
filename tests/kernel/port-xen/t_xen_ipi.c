/* $NetBSD: t_xen_ipi.c,v 1.1.2.1 2011/06/03 13:27:44 cherry Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@NetBSD.org>
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

__RCSID("$NetBSD: t_xen_ipi.c,v 1.1.2.1 2011/06/03 13:27:44 cherry Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <stdbool.h>

#include <atf-c.h>


/* 
 * Please keep the below type declarations in sync with
 * sys/arch/xen/x86/xen_ipi.c
 */
enum {
	TNAMEMAX = 100 /* Totally arbitrary */
};

struct xen_ipi_test_payload {
	union {
		int id;
	} in;
	const struct {
		bool testresult;
		char testname[TNAMEMAX];
	} out;
};

/* End sync declarations. */

/* Test #1 */
ATF_TC(xc_send);
ATF_TC_HEAD(xc_send, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "Tests xcall normal priority");
}

ATF_TC_BODY(xc_send, tc)
{
	int error;
	size_t tpsz;

	struct xen_ipi_test_payload tp = {
		.in.id = 0
	};

	error = sysctlbyname("machdep.tests.xen_ipi", &tp, &tpsz,
			     &tp, sizeof tp);

	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(tpsz, sizeof tp);

	ATF_CHECK(tp.out.testresult == true);

}

/* Test #2 */
ATF_TC(xc_send_high);
ATF_TC_HEAD(xc_send_high, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "Tests xcall on higher priority");
}

ATF_TC_BODY(xc_send_high, tc)
{
	int error;
	size_t tpsz;

	struct xen_ipi_test_payload tp = {
		.in.id = 1
	};

	error = sysctlbyname("machdep.tests.xen_ipi", &tp, &tpsz,
			     &tp, sizeof tp);

	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(tpsz, sizeof tp);

	ATF_CHECK(tp.out.testresult == true);

}

/* Test #3 */
ATF_TC(callout);
ATF_TC_HEAD(callout, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "Tests callout(9) on all cpus");
}

ATF_TC_BODY(callout, tc)
{
	int error;
	size_t tpsz;

	struct xen_ipi_test_payload tp = {
		.in.id = 2
	};

	error = sysctlbyname("machdep.tests.xen_ipi", &tp, &tpsz,
			     &tp, sizeof tp);

	ATF_REQUIRE_EQ(error, 0);
	ATF_REQUIRE_EQ(tpsz, sizeof tp);

	ATF_CHECK(tp.out.testresult == true);

}



ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, xc_send);
	ATF_TP_ADD_TC(tp, xc_send_high);
	ATF_TP_ADD_TC(tp, callout);
	return atf_no_error();
}

