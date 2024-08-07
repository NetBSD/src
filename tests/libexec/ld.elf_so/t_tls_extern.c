/*	$NetBSD: t_tls_extern.c,v 1.12.2.3 2024/08/07 11:00:12 martin Exp $	*/

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
tls_extern(const char *libdef, const char *libuse, enum order order)
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
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so", USE_DEF);
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
	    USE_DEF_NOLOAD);
}

ATF_TC(dynamic_defabuse_eager);
ATF_TC_HEAD(dynamic_defabuse_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "dlopen refuses extern __thread for TLS,"
	    " loading dynamic def then static use eagerly");
}
ATF_TC_BODY(dynamic_defabuse_eager, tc)
{
	void *def;
	int *(*fdef)(void);

	ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", 0));
	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));
	(void)(*fdef)();
	ATF_CHECK_EQ_MSG(NULL, dlopen("libh_abuse_dynamic.so", 0),
	    "dlopen failed to detect static-then-dynamic abuse");
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
	    DEF_USE_LAZY);
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
	    DEF_USE_EAGER);
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
	    DEF_USE_LAZY);
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
	    USE_DEF);
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
	    USE_DEF_NOLOAD);
}

ATF_TC(static_abusedef);
ATF_TC_HEAD(static_abusedef, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " loading dynamic use then static def");
}
ATF_TC_BODY(static_abusedef, tc)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so", USE_DEF);
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
	    USE_DEF_NOLOAD);
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
	    DEF_USE_EAGER);
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
	    DEF_USE_LAZY);
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
	    DEF_USE_EAGER);
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
	    DEF_USE_LAZY);
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
	    USE_DEF);
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
	    USE_DEF_NOLOAD);
}

ATF_TC(onlydef_dynamic_static_ctor);
ATF_TC_HEAD(onlydef_dynamic_static_ctor, tc)
{
	atf_tc_set_md_var(tc, "descr", "definition-only library,"
	    " dynamic load and use in ctor, then static load fails");
}
ATF_TC_BODY(onlydef_dynamic_static_ctor, tc)
{

	ATF_REQUIRE_DL(dlopen("libh_onlydef.so", 0));
	ATF_REQUIRE_DL(dlopen("libh_onlyctor_dynamic.so", 0));
	ATF_CHECK_EQ_MSG(NULL, dlopen("libh_onlyuse_static.so", 0),
	    "dlopen failed to detect dynamic-then-static abuse");
}

ATF_TC(onlydef_dynamic_static_eager);
ATF_TC_HEAD(onlydef_dynamic_static_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "definition-only library,"
	    " dynamic load and use, then static load fails");
}
ATF_TC_BODY(onlydef_dynamic_static_eager, tc)
{
	void *use_dynamic;
	int *(*fdynamic)(void);

	ATF_REQUIRE_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	ATF_REQUIRE_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	(void)(*fdynamic)();
	ATF_CHECK_EQ_MSG(NULL, dlopen("libh_onlyuse_static.so", 0),
	    "dlopen failed to detect dynamic-then-static abuse");
}

ATF_TC(onlydef_dynamic_static_lazy);
ATF_TC_HEAD(onlydef_dynamic_static_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " with definition-only library, dynamic and static load and use");
}
ATF_TC_BODY(onlydef_dynamic_static_lazy, tc)
{
	void *use_dynamic, *use_static;
	int *(*fdynamic)(void), *(*fstatic)(void);
	int *pdynamic, *pstatic;

	ATF_REQUIRE_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	ATF_REQUIRE_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	ATF_REQUIRE_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	ATF_REQUIRE_DL(fstatic = dlsym(use_static, "fstatic"));
	pdynamic = (*fdynamic)();
	pstatic = (*fstatic)();
	ATF_CHECK_EQ_MSG(pdynamic, pstatic,
	    "%p in dynamic tls user != %p in static tls user",
	    pdynamic, pstatic);
}

ATF_TC(onlydef_static_dynamic_eager);
ATF_TC_HEAD(onlydef_static_dynamic_eager, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " with definition-only library,"
	    " static load and use, then dynamic load and use");
}
ATF_TC_BODY(onlydef_static_dynamic_eager, tc)
{
	void *use_static, *use_dynamic;
	int *(*fstatic)(void), *(*fdynamic)(void);
	int *pstatic, *pdynamic;

	ATF_REQUIRE_DL(dlopen("libh_onlydef.so", 0));
	ATF_REQUIRE_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	ATF_REQUIRE_DL(fstatic = dlsym(use_static, "fstatic"));
	pstatic = (*fstatic)();
	ATF_REQUIRE_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	ATF_REQUIRE_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	pdynamic = (*fdynamic)();
	ATF_CHECK_EQ_MSG(pstatic, pdynamic,
	    "%p in static tls user != %p in dynamic tls user",
	    pstatic, pdynamic);
}

ATF_TC(onlydef_static_dynamic_lazy);
ATF_TC_HEAD(onlydef_static_dynamic_lazy, tc)
{
	atf_tc_set_md_var(tc, "descr", "extern __thread for TLS works,"
	    " with definition-only library, static and dynamic load and use");
}
ATF_TC_BODY(onlydef_static_dynamic_lazy, tc)
{
	void *use_static, *use_dynamic;
	int *(*fstatic)(void), *(*fdynamic)(void);
	int *pstatic, *pdynamic;

	ATF_REQUIRE_DL(dlopen("libh_onlydef.so", 0));
	ATF_REQUIRE_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	ATF_REQUIRE_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	ATF_REQUIRE_DL(fstatic = dlsym(use_static, "fstatic"));
	ATF_REQUIRE_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	pstatic = (*fstatic)();
	pdynamic = (*fdynamic)();
	ATF_CHECK_EQ_MSG(pstatic, pdynamic,
	    "%p in static tls user != %p in dynamic tls user",
	    pstatic, pdynamic);
}

ATF_TC(opencloseloop_use);
ATF_TC_HEAD(opencloseloop_use, tc)
{
	atf_tc_set_md_var(tc, "descr", "Testing opening and closing in a loop,"
	    " then opening and using dynamic TLS");
}
ATF_TC_BODY(opencloseloop_use, tc)
{
	unsigned i;
	void *def, *use;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	/*
	 * Open and close the definition library repeatedly.  This
	 * should trigger allocation of many DTV offsets, which are
	 * (currently) not recycled, so the required DTV offsets should
	 * become very long -- pages past what is actually allocated
	 * before we attempt to use it.
	 *
	 * This way, we will exercise the wrong-way-conditional fast
	 * path of PR lib/58154.
	 */
	for (i = sysconf(_SC_PAGESIZE); i --> 0;) {
		ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", 0));
		ATF_REQUIRE_EQ_MSG(dlclose(def), 0,
		    "dlclose(def): %s", dlerror());
	}

	/*
	 * Now open the definition library and keep it open.
	 */
	ATF_REQUIRE_DL(def = dlopen("libh_def_dynamic.so", 0));
	ATF_REQUIRE_DL(fdef = dlsym(def, "fdef"));

	/*
	 * Open libraries that use the definition and verify they
	 * observe the same pointer.
	 */
	ATF_REQUIRE_DL(use = dlopen("libh_use_dynamic.so", 0));
	ATF_REQUIRE_DL(fuse = dlsym(use, "fuse"));
	pdef = (*fdef)();
	puse = (*fuse)();
	ATF_CHECK_EQ_MSG(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);

	/*
	 * Also verify the pointer can be used.
	 */
	*pdef = 123;
	*puse = 456;
	ATF_CHECK_EQ_MSG(*pdef, *puse,
	    "%d in defining library != %d in using library",
	    *pdef, *puse);
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
	ATF_TP_ADD_TC(tp, onlydef_dynamic_static_ctor);
	ATF_TP_ADD_TC(tp, onlydef_dynamic_static_eager);
	ATF_TP_ADD_TC(tp, onlydef_dynamic_static_lazy);
	ATF_TP_ADD_TC(tp, onlydef_static_dynamic_eager);
	ATF_TP_ADD_TC(tp, onlydef_static_dynamic_lazy);
	ATF_TP_ADD_TC(tp, opencloseloop_use);
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
