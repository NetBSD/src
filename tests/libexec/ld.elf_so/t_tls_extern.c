/*	$NetBSD: t_tls_extern.c,v 1.6 2023/06/01 22:26:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#include <sys/types.h>

#include <atf-c.h>
#include <dlfcn.h>

#define	ATF_REQUIRE_DL(x)	ATF_REQUIRE_MSG(x, "%s: %s", #x, dlerror())

enum order {
	DEF_USE,
	USE_DEF,
	USE_DEF_NOLOAD,
};

static void
tls_extern(const char *libdef, const char *libuse, enum order order,
    bool xfail)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	switch (order) {
	case DEF_USE:
		ATF_REQUIRE_DL(def = dlopen(libdef, 0));
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		break;
	case USE_DEF:
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		ATF_REQUIRE_DL(def = dlopen(libdef, 0));
		break;
	case USE_DEF_NOLOAD:
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		ATF_REQUIRE_DL(def = dlopen(libdef, RTLD_NOLOAD));
		break;
	}

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	if (xfail) {
		atf_tc_expect_fail("PR toolchain/50277:"
		    " rtld relocation bug with thread local storage");
	}
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_dynamic_abusedef);
ATF_TC_HEAD(tls_extern_dynamic_abusedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static use than dynamic def");
}
ATF_TC_BODY(tls_extern_dynamic_abusedef, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(tls_extern_dynamic_abusedefnoload);
ATF_TC_HEAD(tls_extern_dynamic_abusedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static use then dynamic def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_dynamic_abusedefnoload, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TC(tls_extern_dynamic_defabuse);
ATF_TC_HEAD(tls_extern_dynamic_defabuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic def then static use");
}
ATF_TC_BODY(tls_extern_dynamic_defabuse, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    DEF_USE, /*xfail*/true);
}

ATF_TC(tls_extern_dynamic_defuse);
ATF_TC_HEAD(tls_extern_dynamic_defuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading def then use");
}
ATF_TC_BODY(tls_extern_dynamic_defuse, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    DEF_USE, /*xfail*/false);
}

ATF_TC(tls_extern_dynamic_usedef);
ATF_TC_HEAD(tls_extern_dynamic_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(tls_extern_dynamic_usedef, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF, /*xfail*/false);
}

ATF_TC(tls_extern_dynamic_usedefnoload);
ATF_TC_HEAD(tls_extern_dynamic_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_dynamic_usedefnoload, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF_NOLOAD, /*xfail*/false);
}

ATF_TC(tls_extern_static_abusedef);
ATF_TC_HEAD(tls_extern_static_abusedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic use then static def");
}
ATF_TC_BODY(tls_extern_static_abusedef, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(tls_extern_static_abusedefnoload);
ATF_TC_HEAD(tls_extern_static_abusedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic use then static def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_static_abusedefnoload, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TC(tls_extern_static_defabuse);
ATF_TC_HEAD(tls_extern_static_defabuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static def then dynamic use");
}
ATF_TC_BODY(tls_extern_static_defabuse, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    DEF_USE, /*xfail*/true);
}

ATF_TC(tls_extern_static_defuse);
ATF_TC_HEAD(tls_extern_static_defuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading def then use");
}
ATF_TC_BODY(tls_extern_static_defuse, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    DEF_USE, /*xfail*/false);
}

ATF_TC(tls_extern_static_usedef);
ATF_TC_HEAD(tls_extern_static_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(tls_extern_static_usedef, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(tls_extern_static_usedefnoload);
ATF_TC_HEAD(tls_extern_static_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_static_usedefnoload, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, tls_extern_dynamic_abusedef);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_abusedefnoload);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_defabuse);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_defuse);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_usedef);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_usedefnoload);
	ATF_TP_ADD_TC(tp, tls_extern_static_abusedef);
	ATF_TP_ADD_TC(tp, tls_extern_static_abusedefnoload);
	ATF_TP_ADD_TC(tp, tls_extern_static_defabuse);
	ATF_TP_ADD_TC(tp, tls_extern_static_defuse);
	ATF_TP_ADD_TC(tp, tls_extern_static_usedef);
	ATF_TP_ADD_TC(tp, tls_extern_static_usedefnoload);
	return atf_no_error();
}
