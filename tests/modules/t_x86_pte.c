/*	$NetBSD: t_x86_pte.c,v 1.2 2020/04/26 11:56:38 maxv Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__COPYRIGHT("@(#) Copyright (c) 2020\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_x86_pte.c,v 1.2 2020/04/26 11:56:38 maxv Exp $");

#include <sys/types.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <err.h>
#include <errno.h>
#include <atf-c.h>

#define mib_name	"kern.x86_pte_test.test"
#define MODULE_PATH	"/usr/tests/modules/x86_pte_tester/x86_pte_tester.kmod"
#define MODULE_NAME	"x86_pte_tester"

static struct {
	size_t n_rwx;
	size_t n_shstk;
	bool kernel_map_with_low_ptes;
	bool pte_is_user_accessible;
	size_t n_user_space_is_kernel;
	size_t n_kernel_space_is_user;
	size_t n_svs_g_bit_set;
} x86_pte_results;

static void
fetch_results(void)
{
	static bool fetched = false;
	size_t len = sizeof(x86_pte_results);
	char module_name[] = MODULE_NAME;
	modctl_load_t params = {
		.ml_filename = MODULE_PATH,
		.ml_flags = MODCTL_NO_PROP,
	};
	int rv;

	if (fetched)
		return;

	if (modctl(MODCTL_LOAD, &params) != 0)
		atf_tc_skip("loading '%s' module failed.", MODULE_NAME);

	rv = sysctlbyname(mib_name, &x86_pte_results, &len, 0, 0);
	ATF_REQUIRE_EQ(rv, 0);

	if (modctl(MODCTL_UNLOAD, module_name) != 0)
		warn("failed to unload module '%s'", MODULE_NAME);

	fetched = true;
}

/* -------------------------------------------------------------------------- */

ATF_TC(rwx);
ATF_TC_HEAD(rwx, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure there is no RWX page in the kernel");
}
ATF_TC_BODY(rwx, tc)
{
	fetch_results();
	ATF_REQUIRE_EQ(x86_pte_results.n_rwx, 0);
}

/* -------------------------------------------------------------------------- */

ATF_TC(shstk);
ATF_TC_HEAD(shstk, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure there is no SHSTK page in the kernel");
}
ATF_TC_BODY(shstk, tc)
{
	fetch_results();
	atf_tc_expect_fail("there are %zu SHSTK pages",
	    x86_pte_results.n_shstk);
	ATF_REQUIRE_EQ(x86_pte_results.n_shstk, 0);
}

/* -------------------------------------------------------------------------- */

ATF_TC(kernel_map_with_low_ptes);
ATF_TC_HEAD(kernel_map_with_low_ptes, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure the kernel map has no user mapping");
}
ATF_TC_BODY(kernel_map_with_low_ptes, tc)
{
	fetch_results();
	ATF_REQUIRE_EQ(x86_pte_results.kernel_map_with_low_ptes, false);
}

/* -------------------------------------------------------------------------- */

ATF_TC(pte_is_user_accessible);
ATF_TC_HEAD(pte_is_user_accessible, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure the PTE space does not have user permissions");
}
ATF_TC_BODY(pte_is_user_accessible, tc)
{
	fetch_results();
	ATF_REQUIRE_EQ(x86_pte_results.pte_is_user_accessible, false);
}

/* -------------------------------------------------------------------------- */

ATF_TC(user_space_is_kernel);
ATF_TC_HEAD(user_space_is_kernel, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure no page in the user space has kernel permissions");
}
ATF_TC_BODY(user_space_is_kernel, tc)
{
	fetch_results();
	ATF_REQUIRE_EQ(x86_pte_results.n_user_space_is_kernel, 0);
}

/* -------------------------------------------------------------------------- */

ATF_TC(kernel_space_is_user);
ATF_TC_HEAD(kernel_space_is_user, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure no page in the kernel space has user permissions");
}
ATF_TC_BODY(kernel_space_is_user, tc)
{
	fetch_results();
	ATF_REQUIRE_EQ(x86_pte_results.n_kernel_space_is_user, 0);
}

/* -------------------------------------------------------------------------- */

ATF_TC(svs_g_bit_set);
ATF_TC_HEAD(svs_g_bit_set, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "ensure that no page in the SVS map has the G bit set");
}
ATF_TC_BODY(svs_g_bit_set, tc)
{
	fetch_results();
	if (x86_pte_results.n_svs_g_bit_set != (size_t)-1) {
		ATF_REQUIRE_EQ(x86_pte_results.n_svs_g_bit_set, 0);
	} else {
		atf_tc_skip("SVS is disabled.");
	}
}

/* -------------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, rwx);
	ATF_TP_ADD_TC(tp, shstk);
	ATF_TP_ADD_TC(tp, kernel_map_with_low_ptes);
	ATF_TP_ADD_TC(tp, pte_is_user_accessible);
	ATF_TP_ADD_TC(tp, user_space_is_kernel);
	ATF_TP_ADD_TC(tp, kernel_space_is_user);
	ATF_TP_ADD_TC(tp, svs_g_bit_set);

	return atf_no_error();
}
