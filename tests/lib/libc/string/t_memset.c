/* $NetBSD: t_memset.c,v 1.5 2024/11/02 02:43:48 riastradh Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
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
__RCSID("$NetBSD: t_memset.c,v 1.5 2024/11/02 02:43:48 riastradh Exp $");

#include <sys/stat.h>

#include <atf-c.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static long	page = 0;
static void	fill(char *, size_t, char);
static bool	check(char *, size_t, char);

int zero;	/* always zero, but the compiler does not know */

static const struct {
	const char	*name;
	void		*(*fn)(void *, int, size_t);
} memsetfn[] = {
	{ "memset", &memset },
	{ "explicit_memset", &explicit_memset }, /* NetBSD extension */
	{ "memset_explicit", &memset_explicit }, /* C23 adopted name */
};

ATF_TC(memset_array);
ATF_TC_HEAD(memset_array, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test memset(3) with arrays");
}

ATF_TC_BODY(memset_array, tc)
{
	char buf[1024];
	unsigned i;

	for (i = 0; i < __arraycount(memsetfn); i++) {
		(void)(*memsetfn[i].fn)(buf, 0, sizeof(buf));
		ATF_CHECK_MSG(check(buf, sizeof(buf), 0),
		    "%s did not fill a static buffer",
		    memsetfn[i].name);

		(void)(*memsetfn[i].fn)(buf, 'x', sizeof(buf));
		ATF_CHECK_MSG(check(buf, sizeof(buf), 'x'),
		    "%s did not fill a static buffer",
		    memsetfn[i].name);
	}
}

ATF_TC(memset_return);
ATF_TC_HEAD(memset_return, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test memset(3) return value");
}

ATF_TC_BODY(memset_return, tc)
{
	char *b = (char *)0x1;
	char c[2];
	char *p;
	unsigned i;

	for (i = 0; i < __arraycount(memsetfn); i++) {
		ATF_CHECK_EQ_MSG((p = (*memsetfn[i].fn)(b, 0, 0)), b,
		    "%s: returned %p, expected %p", memsetfn[i].name, p, b);
		ATF_CHECK_EQ_MSG((p = (*memsetfn[i].fn)(c, 2, sizeof(c))), c,
		    "%s: returned %p, expected %p", memsetfn[i].name, p, c);
	}
}

ATF_TC(memset_basic);
ATF_TC_HEAD(memset_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of memset(3)");
}

ATF_TC_BODY(memset_basic, tc)
{
	char *buf, *ret;
	unsigned i;

	buf = malloc(page);
	ret = malloc(page);

	ATF_REQUIRE(buf != NULL);
	ATF_REQUIRE(ret != NULL);

	for (i = 0; i < __arraycount(memsetfn); i++) {
		fill(ret, page, 0);
		(*memsetfn[i].fn)(buf, 0, page);

		ATF_CHECK_EQ_MSG(memcmp(ret, buf, page), 0, "%s",
		    memsetfn[i].name);

		fill(ret, page, 'x');
		(*memsetfn[i].fn)(buf, 'x', page);

		ATF_CHECK_EQ_MSG(memcmp(ret, buf, page), 0, "%s",
		    memsetfn[i].name);
	}

	free(buf);
	free(ret);
}

ATF_TC(memset_nonzero);
ATF_TC_HEAD(memset_nonzero, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test memset(3) with non-zero params");
}

ATF_TC_BODY(memset_nonzero, tc)
{
	const size_t n = 0x7f;
	char *buf;
	size_t i, j;

	buf = malloc(page);
	ATF_REQUIRE(buf != NULL);

	for (i = 0x21; i < n; i++) {
		for (j = 0; j < __arraycount(memsetfn); j++) {
			(void)(*memsetfn[j].fn)(buf, i, page);
			ATF_CHECK_MSG(check(buf, page, i),
			    "%s did not fill properly with %zu",
			    memsetfn[j].name, i);
		}
	}

	free(buf);
}

ATF_TC(memset_zero_size);

ATF_TC_HEAD(memset_zero_size, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test memset(3) with zero size");
}

ATF_TC_BODY(memset_zero_size, tc)
{
	char buf[1024];
	unsigned i;

	for (i = 0; i < __arraycount(memsetfn); i++) {
		(void)(*memsetfn[i].fn)(buf, 'x', sizeof(buf));
		ATF_CHECK_MSG(check(buf, sizeof(buf), 'x'),
		    "%s did not fill a static buffer",
		    memsetfn[i].name);

		(void)memset(buf+sizeof(buf)/2, 0, zero);
		ATF_CHECK_MSG(check(buf, sizeof(buf), 'x'),
		    "%s with 0 size did change the buffer",
		    memsetfn[i].name);
	}
}

ATF_TC(bzero_zero_size);

ATF_TC_HEAD(bzero_zero_size, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test bzero(3) with zero size");
}

ATF_TC_BODY(bzero_zero_size, tc)
{
	char buf[1024];

	(void)memset(buf, 'x', sizeof(buf));

	if (check(buf, sizeof(buf), 'x') != true)
		atf_tc_fail("memset(3) did not fill a static buffer");

	(void)bzero(buf+sizeof(buf)/2, zero);

	if (check(buf, sizeof(buf), 'x') != true)
		atf_tc_fail("bzero(3) with 0 size did change the buffer");
}

ATF_TC(memset_struct);
ATF_TC_HEAD(memset_struct, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test memset(3) with a structure");
}

ATF_TC_BODY(memset_struct, tc)
{
	struct stat st;
	unsigned i;

	for (i = 0; i < __arraycount(memsetfn); i++) {
		st.st_dev = 0;
		st.st_ino = 1;
		st.st_mode = 2;
		st.st_nlink = 3;
		st.st_uid = 4;
		st.st_gid = 5;
		st.st_rdev = 6;
		st.st_size = 7;
		st.st_atime = 8;
		st.st_mtime = 9;

		(void)(*memsetfn[i].fn)(&st, 0, sizeof(struct stat));

		ATF_CHECK_MSG(st.st_dev == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_ino == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_mode == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_nlink == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_uid == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_gid == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_rdev == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_size == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_atime == 0, "%s", memsetfn[i].name);
		ATF_CHECK_MSG(st.st_mtime == 0, "%s", memsetfn[i].name);
	}
}

static void
fill(char *buf, size_t len, char x)
{
	size_t i;

	for (i = 0; i < len; i++)
		buf[i] = x;
}

static bool
check(char *buf, size_t len, char x)
{
	size_t i;

	for (i = 0; i < len; i++) {

		if (buf[i] != x)
			return false;
	}

	return true;
}

ATF_TP_ADD_TCS(tp)
{

	page = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(page >= 0);

	ATF_TP_ADD_TC(tp, memset_array);
	ATF_TP_ADD_TC(tp, memset_basic);
	ATF_TP_ADD_TC(tp, memset_nonzero);
	ATF_TP_ADD_TC(tp, memset_struct);
	ATF_TP_ADD_TC(tp, memset_return);
	ATF_TP_ADD_TC(tp, memset_zero_size);
	ATF_TP_ADD_TC(tp, bzero_zero_size);

	return atf_no_error();
}
