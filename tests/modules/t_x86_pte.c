/*	$NetBSD: t_x86_pte.c,v 1.1 2020/04/26 09:08:40 maxv Exp $	*/

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
__RCSID("$NetBSD: t_x86_pte.c,v 1.1 2020/04/26 09:08:40 maxv Exp $");

#include <sys/types.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <err.h>
#include <errno.h>
#include <atf-c.h>

#define mib_name	"kern.x86_pte_test.test"
#define MODULE_PATH	"/usr/tests/modules/x86_pte_tester/x86_pte_tester.kmod"
#define MODULE_NAME	"x86_pte_tester"

static bool module_loaded;

static void
load_module(void)
{
#ifndef SKIP_MODULE
	if (module_loaded)
		return;

	modctl_load_t params = {
		.ml_filename = MODULE_PATH,
		.ml_flags = MODCTL_NO_PROP,
	};

	if (modctl(MODCTL_LOAD, &params) != 0) {
		warn("failed to load module '%s'", MODULE_PATH);
	} else {
		module_loaded = true;
	}
#else
	module_loaded = true;
#endif /* ! SKIP_MODULE */
}

static void
unload_module(void)
{
#ifndef SKIP_MODULE
	char module_name[] = MODULE_NAME;

	if (modctl(MODCTL_UNLOAD, module_name) != 0) {
		warn("failed to unload module '%s'", MODULE_NAME);
	} else {
		module_loaded = false;
	}
#endif /* ! SKIP_MODULE */
}

ATF_TC_WITH_CLEANUP(x86_pte);
ATF_TC_HEAD(x86_pte, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "testing the PTEs for correctness");
}
ATF_TC_BODY(x86_pte, tc)
{
	struct {
		size_t n_rwx;
		bool kernel_map_with_low_ptes;
		bool pte_is_user_accessible;
		size_t n_user_space_is_kernel;
		size_t n_kernel_space_is_user;
		size_t n_svs_g_bit_set;
	} results;
	size_t len = sizeof(results);
	int rv;

	load_module();
	if (!module_loaded) {
		atf_tc_skip("loading '%s' module failed.", MODULE_NAME);
	}

	rv = sysctlbyname(mib_name, &results, &len, 0, 0);
	ATF_REQUIRE_EQ(rv, 0);

	ATF_REQUIRE_EQ(results.n_rwx, 0);
	ATF_REQUIRE_EQ(results.kernel_map_with_low_ptes, false);
	ATF_REQUIRE_EQ(results.pte_is_user_accessible, false);
	ATF_REQUIRE_EQ(results.n_user_space_is_kernel, 0);
	ATF_REQUIRE_EQ(results.n_kernel_space_is_user, 0);
	if (results.n_svs_g_bit_set != (size_t)-1) {
		ATF_REQUIRE_EQ(results.n_svs_g_bit_set, 0);
	}
}
ATF_TC_CLEANUP(x86_pte, tc)
{
	unload_module();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, x86_pte);

	return atf_no_error();
}
