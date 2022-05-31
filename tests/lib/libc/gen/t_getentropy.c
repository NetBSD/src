/*	$NetBSD: t_getentropy.c,v 1.1 2022/05/31 13:42:59 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_getentropy.c,v 1.1 2022/05/31 13:42:59 riastradh Exp $");

#include <sys/mman.h>

#include <atf-c.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static uint8_t buf[289];
static uint8_t zero[sizeof(buf)];

__CTASSERT(GETENTROPY_MAX == 256);

ATF_TC(getentropy_0);
ATF_TC_HEAD(getentropy_0, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy 0 bytes");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_0, tc)
{

	memset(buf, 0, sizeof(buf));
	if (getentropy(buf, 0) == -1)
		atf_tc_fail("getentropy: %d (%s)", errno, strerror(errno));
	ATF_CHECK(memcmp(buf, zero, sizeof(buf)) == 0);
}

ATF_TC(getentropy_32);
ATF_TC_HEAD(getentropy_32, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy 32 bytes");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_32, tc)
{

	memset(buf, 0, sizeof(buf));
	if (getentropy(buf + 1, 32) == -1)
		atf_tc_fail("getentropy: %d (%s)", errno, strerror(errno));
	ATF_CHECK(buf[0] == 0);
	ATF_CHECK(memcmp(buf + 1, zero, 32) != 0);
	ATF_CHECK(memcmp(buf + 1 + 32, zero, sizeof(buf) - 1 - 32) == 0);
}

ATF_TC(getentropy_256);
ATF_TC_HEAD(getentropy_256, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy 256 bytes");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_256, tc)
{

	memset(buf, 0, sizeof(buf));
	if (getentropy(buf + 1, 256) == -1)
		atf_tc_fail("getentropy: %d (%s)", errno, strerror(errno));
	ATF_CHECK(buf[0] == 0);
	ATF_CHECK(memcmp(buf + 1, zero, 256) != 0);
	ATF_CHECK(memcmp(buf + 1 + 256, zero, sizeof(buf) - 1 - 256) == 0);
}

ATF_TC(getentropy_257);
ATF_TC_HEAD(getentropy_257, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy 257 bytes (beyond max)");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_257, tc)
{

	memset(buf, 0, sizeof(buf));
	ATF_CHECK_ERRNO(EINVAL, getentropy(buf, 257) == -1);
	ATF_CHECK(memcmp(buf, zero, sizeof(buf)) == 0);
}

ATF_TC(getentropy_null);
ATF_TC_HEAD(getentropy_null, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy with null buffer");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_null, tc)
{

	ATF_CHECK_ERRNO(EFAULT, getentropy(NULL, 32) == -1);
}

ATF_TC(getentropy_nearnull);
ATF_TC_HEAD(getentropy_nearnull, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy with nearly null buffer");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_nearnull, tc)
{

	ATF_CHECK_ERRNO(EFAULT, getentropy((char *)(uintptr_t)1, 32) == -1);
}

ATF_TC(getentropy_badaddr);
ATF_TC_HEAD(getentropy_badaddr, tc)
{
	atf_tc_set_md_var(tc, "descr", "getentropy with bad address");
	atf_tc_set_md_var(tc, "timeout", "2");
}
ATF_TC_BODY(getentropy_badaddr, tc)
{
	size_t pagesize = sysconf(_SC_PAGESIZE);
	char *p;

	/*
	 * Allocate three consecutive pages and make the middle one
	 * nonwritable.
	 */
	p = mmap(NULL, 3*pagesize, PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0);
	if (p == MAP_FAILED)
		atf_tc_fail("mmap: %d (%s)", errno, strerror(errno));
	if (mprotect(p + pagesize, pagesize, PROT_READ) == -1)
		atf_tc_fail("mprotect: %d (%s)", errno, strerror(errno));

	/* Verify that writing to the end of the first page works.  */
	if (getentropy(p + pagesize - 25, 25) == -1)
		atf_tc_fail("getentropy 1: %d (%s)", errno, strerror(errno));
	ATF_CHECK(memcmp(p + pagesize - 25, zero, 25) != 0);

	/*
	 * Verify that writes into the middle page, whether straddling
	 * the surrounding pages or not, fail with EFAULT.
	 */
	ATF_CHECK_ERRNO(EFAULT, getentropy(p + pagesize - 1, 2) == -1);
	ATF_CHECK_ERRNO(EFAULT, getentropy(p + pagesize, 32) == -1);
	ATF_CHECK_ERRNO(EFAULT, getentropy(p + 2*pagesize - 1, 2) == -1);

	/* Verify that writing to the start of the last page works.  */
	if (getentropy(p + 2*pagesize, 25) == -1)
		atf_tc_fail("getentropy 2: %d (%s)", errno, strerror(errno));
	ATF_CHECK(memcmp(p + pagesize - 25, zero, 25) != 0);

	if (munmap(p, 3*pagesize) == -1)
		atf_tc_fail("munmap: %d (%s)", errno, strerror(errno));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, getentropy_0);
	ATF_TP_ADD_TC(tp, getentropy_256);
	ATF_TP_ADD_TC(tp, getentropy_257);
	ATF_TP_ADD_TC(tp, getentropy_32);
	ATF_TP_ADD_TC(tp, getentropy_badaddr);
	ATF_TP_ADD_TC(tp, getentropy_nearnull);
	ATF_TP_ADD_TC(tp, getentropy_null);

	return atf_no_error();
}
