/*	$NetBSD: t_tls_extern.c,v 1.4 2023/06/01 20:50:18 riastradh Exp $	*/

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

ATF_TC(tls_extern_dynamic_defuse);
ATF_TC_HEAD(tls_extern_dynamic_defuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading def then use");
}
ATF_TC_BODY(tls_extern_dynamic_defuse, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", 0));
	ATF_REQUIRE_DL(use = dlopen("libh_use_dynamic.so", 0));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_dynamic_usedef);
ATF_TC_HEAD(tls_extern_dynamic_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(tls_extern_dynamic_usedef, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(use = dlopen("libh_use_dynamic.so", 0));
	ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", 0));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_dynamic_usedefnoload);
ATF_TC_HEAD(tls_extern_dynamic_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for dynamic TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_dynamic_usedefnoload, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(use = dlopen("libh_use_dynamic.so", 0));
	ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", RTLD_NOLOAD));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_static_defuse);
ATF_TC_HEAD(tls_extern_static_defuse, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading def then use");
}
ATF_TC_BODY(tls_extern_static_defuse, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(def = dlopen("libh_def_static.so", 0));
	ATF_REQUIRE_DL(use = dlopen("libh_use_static.so", 0));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_static_usedef);
ATF_TC_HEAD(tls_extern_static_usedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def");
}
ATF_TC_BODY(tls_extern_static_usedef, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(use = dlopen("libh_use_static.so", 0));
	ATF_REQUIRE_DL(def = dlopen("libh_def_static.so", 0));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	atf_tc_expect_fail("PR toolchain/50277:"
	    " rtld relocation bug with thread local storage");
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TC(tls_extern_static_usedefnoload);
ATF_TC_HEAD(tls_extern_static_usedefnoload, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for static TLS works,"
	    " loading use then def with RTLD_NOLOAD");
}
ATF_TC_BODY(tls_extern_static_usedefnoload, tc)
{
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	ATF_REQUIRE_DL(use = dlopen("libh_use_static.so", 0));
	ATF_REQUIRE_DL(def = dlopen("libh_def_static.so", RTLD_NOLOAD));

	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));

	pdef = (*fdef)();
	puse = (*fuse)();
	atf_tc_expect_fail("PR toolchain/50277:"
	    " rtld relocation bug with thread local storage");
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, tls_extern_dynamic_defuse);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_usedef);
	ATF_TP_ADD_TC(tp, tls_extern_dynamic_usedefnoload);
	ATF_TP_ADD_TC(tp, tls_extern_static_defuse);
	ATF_TP_ADD_TC(tp, tls_extern_static_usedef);
	ATF_TP_ADD_TC(tp, tls_extern_static_usedefnoload);
	return atf_no_error();
}
