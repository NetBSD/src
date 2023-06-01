/*	$NetBSD: t_tls_extern.c,v 1.8 2023/06/01 23:47:24 riastradh Exp $	*/

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
	DEF_USE_EAGER,
	DEF_USE_LAZY,
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
	case DEF_USE_EAGER:
		ATF_REQUIRE_DL(def = dlopen(libdef, 0));
		ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
		pdef = (*fdef)();
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));
		puse = (*fuse)();
		break;
	case DEF_USE_LAZY:
		ATF_REQUIRE_DL(def = dlopen(libdef, 0));
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		goto lazy;
	case USE_DEF:
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		ATF_REQUIRE_DL(def = dlopen(libdef, 0));
		goto lazy;
	case USE_DEF_NOLOAD:
		ATF_REQUIRE_DL(use = dlopen(libuse, 0));
		ATF_REQUIRE_DL(def = dlopen(libdef, RTLD_NOLOAD));
lazy:		ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
		ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));
		pdef = (*fdef)();
		puse = (*fuse)();
		break;
	}

	if (xfail) {
		atf_tc_expect_fail("PR toolchain/50277:"
		    " rtld relocation bug with thread local storage");
	}
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(dynamic_abusedef);
ATF_TC_HEAD(dynamic_abusedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static use than dynamic def");
}
ATF_TC_BODY(dynamic_abusedef, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(dynamic_abusedefnoload);
ATF_TC_HEAD(dynamic_abusedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static use then dynamic def with RTLD_NOLOAD");
}
ATF_TC_BODY(dynamic_abusedefnoload, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TC(dynamic_defabuse_eager);
ATF_TC_HEAD(dynamic_defabuse_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic def then static use eagerly");
}
ATF_TC_BODY(dynamic_defabuse_eager, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    DEF_USE_EAGER, /*xfail*/true);
}

ATF_TC(dynamic_defabuse_lazy);
ATF_TC_HEAD(dynamic_defabuse_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic def then static use lazily");
}
ATF_TC_BODY(dynamic_defabuse_lazy, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    DEF_USE_LAZY, /*xfail*/true);
}

ATF_TC(dynamic_defuse_eager);
ATF_TC_HEAD(dynamic_defuse_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading def then use eagerly");
}
ATF_TC_BODY(dynamic_defuse_eager, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    DEF_USE_EAGER, /*xfail*/false);
}

ATF_TC(dynamic_defuse_lazy);
ATF_TC_HEAD(dynamic_defuse_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading def then use lazyly");
}
ATF_TC_BODY(dynamic_defuse_lazy, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    DEF_USE_LAZY, /*xfail*/false);
}

ATF_TC(dynamic_usedef);
ATF_TC_HEAD(dynamic_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(dynamic_usedef, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF, /*xfail*/false);
}

ATF_TC(dynamic_usedefnoload);
ATF_TC_HEAD(dynamic_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(dynamic_usedefnoload, tc)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF_NOLOAD, /*xfail*/false);
}

ATF_TC(static_abusedef);
ATF_TC_HEAD(static_abusedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic use then static def");
}
ATF_TC_BODY(static_abusedef, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(static_abusedefnoload);
ATF_TC_HEAD(static_abusedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic use then static def with RTLD_NOLOAD");
}
ATF_TC_BODY(static_abusedefnoload, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TC(static_defabuse_eager);
ATF_TC_HEAD(static_defabuse_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static def then dynamic use eagerly");
}
ATF_TC_BODY(static_defabuse_eager, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    DEF_USE_EAGER, /*xfail*/true);
}

ATF_TC(static_defabuse_lazy);
ATF_TC_HEAD(static_defabuse_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading static def then dynamic use lazyly");
}
ATF_TC_BODY(static_defabuse_lazy, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    DEF_USE_LAZY, /*xfail*/true);
}

ATF_TC(static_defuse_eager);
ATF_TC_HEAD(static_defuse_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading def then use eagerly");
}
ATF_TC_BODY(static_defuse_eager, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    DEF_USE_EAGER, /*xfail*/false);
}

ATF_TC(static_defuse_lazy);
ATF_TC_HEAD(static_defuse_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading def then use lazyly");
}
ATF_TC_BODY(static_defuse_lazy, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    DEF_USE_LAZY, /*xfail*/false);
}

ATF_TC(static_usedef);
ATF_TC_HEAD(static_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(static_usedef, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF, /*xfail*/true);
}

ATF_TC(static_usedefnoload);
ATF_TC_HEAD(static_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(static_usedefnoload, tc)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF_NOLOAD, /*xfail*/true);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dynamic_abusedef);
	ATF_TP_ADD_TC(tp, dynamic_abusedefnoload);
	ATF_TP_ADD_TC(tp, dynamic_defabuse_eager);
	ATF_TP_ADD_TC(tp, dynamic_defabuse_lazy);
	ATF_TP_ADD_TC(tp, dynamic_defuse_eager);
	ATF_TP_ADD_TC(tp, dynamic_defuse_lazy);
	ATF_TP_ADD_TC(tp, dynamic_usedef);
	ATF_TP_ADD_TC(tp, dynamic_usedefnoload);
	ATF_TP_ADD_TC(tp, static_abusedef);
	ATF_TP_ADD_TC(tp, static_abusedefnoload);
	ATF_TP_ADD_TC(tp, static_defabuse_eager);
	ATF_TP_ADD_TC(tp, static_defabuse_lazy);
	ATF_TP_ADD_TC(tp, static_defuse_eager);
	ATF_TP_ADD_TC(tp, static_defuse_lazy);
	ATF_TP_ADD_TC(tp, static_usedef);
	ATF_TP_ADD_TC(tp, static_usedefnoload);
	return atf_no_error();
}
