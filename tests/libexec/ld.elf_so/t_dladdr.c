/*	$NetBSD: t_dladdr.c,v 1.1 2025/12/14 23:26:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_dladdr.c,v 1.1 2025/12/14 23:26:12 riastradh Exp $");

#include <sys/mman.h>

#include <atf-c.h>
#include <dlfcn.h>
#include <unistd.h>

#include "h_macros.h"

#define	SELF	"t_dladdr"

/*
 * Note: the symbols foo, bar, and baz must be exposed to dlfcn(3) by
 * linking this with -export-dynamic.
 */
int foo;			/* something in this ELF object */
int bar;			/* something in this ELF object */
int baz;			/* something in this ELF object */

extern char _end[];		/* one past last byte of this ELF object */

ATF_TC(dladdr_self);
ATF_TC_HEAD(dladdr_self, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify dladdr of data in this object returns self");
}
ATF_TC_BODY(dladdr_self, tc)
{
	Dl_info info;
	const char *ptr = (char *)&bar + 1;
	const char *p;

	/*
	 * If we're statically linked, dladdr just fails (XXX is that
	 * right?).  But we're not statically linked because these are
	 * the ld.elf_so tests (XXX should migrate this to
	 * tests/lib/libc/dlfcn/ and handle it there).
	 *
	 * If we're dynamically linked, then foo, bar, and baz should
	 * live in self.
	 */
	ATF_CHECK_MSG(dladdr(ptr, &info) != 0,
	    "[bar @ %p + [0,%zu)] dladdr(%p) failed: %s",
	    &bar, sizeof(bar), ptr, dlerror());
	p = strrchr(info.dli_fname, '/');
	if (p == NULL)
		p = info.dli_fname;
	else
		p++;
	ATF_CHECK_MSG(strcmp(p, SELF) == 0,
	    "[bar @ %p + [0,%zu)] dladdr found %p in %s, not self=%s",
	    &bar, sizeof(bar), ptr, info.dli_fname, SELF);
	ATF_CHECK_MSG(strcmp(info.dli_sname, "bar") == 0,
	    "[bar @ %p + [0,%zu)] dladdr found %p in %s at %p, not bar",
	    &bar, sizeof(bar), ptr, info.dli_sname, info.dli_saddr);
}

ATF_TC(dladdr_errno);
ATF_TC_HEAD(dladdr_errno, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify dladdr(errno) returns libc.so (or self if static)");
}
ATF_TC_BODY(dladdr_errno, tc)
{
	Dl_info info;
	const char *p;

	/*
	 * If we're statically linked, dladdr just fails (XXX is that
	 * right?).  But we're not statically linked because these are
	 * the ld.elf_so tests (XXX should migrate this to
	 * tests/lib/libc/dlfcn/ and handle it there).
	 *
	 * If we're dynamically linked and single-threaded (no
	 * libpthread.so), &errno will be in libc.
	 *
	 * If we're dynamically linked and multi-threaded, &errno would
	 * be in a pthread_t object -- but we're not multithreaded, so
	 * it's not.  Hence only two cases: static vs dynamic.
	 */
	ATF_CHECK_MSG(dladdr(&errno, &info) != 0,
	    "[errno @ %p] dladdr failed: %s", &errno, dlerror());
	p = strrchr(info.dli_fname, '/');
	if (p == NULL)
		p = info.dli_fname;
	else
		p++;
	ATF_CHECK_MSG(strcmp(p, SELF) != 0,
	    "[errno @ %p] dladdr found errno in self=%s, not in libc.so",
	    &errno, info.dli_fname);
}

ATF_TC(dladdr_after__end);
ATF_TC_HEAD(dladdr_after__end, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Verify dladdr doesn't claim pages past _end for self");
}
ATF_TC_BODY(dladdr_after__end, tc)
{
	const size_t pagesize = sysconf(_SC_PAGESIZE);
	uintptr_t endp, nextp;
	void *page;
	Dl_info info;

	/*
	 * Round up to a page start.
	 */
	endp = (uintptr_t)(void *)_end;
	nextp = pagesize*((endp + pagesize - 1)/pagesize);

	/*
	 * Map the next page, or something after it.  It just has to be
	 * not mapped by an existing object, but for the sake of
	 * testing for past bugs, we would like to make it as close to
	 * a real object as we can.
	 */
	REQUIRE_LIBC(page = mmap((void *)nextp, pagesize, PROT_NONE, MAP_ANON,
		/*fd*/-1, /*off*/0),
	    MAP_FAILED);

	/*
	 * Verify dladdr doesn't return anything for this page.
	 */
	atf_tc_expect_fail("PR lib/59567: dladdr(3) doesn't work properly"
	    " especially when main executable is loaded at"
	    " high memory address");
	ATF_CHECK_MSG(dladdr(page, &info) == 0,
	    "dladdr returned %s @ %p (symbol %s @ %p) for bogus address %p",
	    info.dli_fname, info.dli_fbase,
	    info.dli_sname, info.dli_saddr,
	    page);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, dladdr_after__end);
	ATF_TP_ADD_TC(tp, dladdr_errno);
	ATF_TP_ADD_TC(tp, dladdr_self);
	return atf_no_error();
}
