/* $NetBSD: t_mincore.c,v 1.2.2.2 2011/06/23 14:20:41 cherry Exp $ */

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
__RCSID("$NetBSD: t_mincore.c,v 1.2.2.2 2011/06/23 14:20:41 cherry Exp $");

#include <sys/mman.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static long		page = 0;
static const char	path[] = "mincore";

ATF_TC(mincore_err);
ATF_TC_HEAD(mincore_err, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test errors from mincore(2)");
}

ATF_TC_BODY(mincore_err, tc)
{
	char *map, *vec;

	map = mmap(NULL, page, PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	vec = malloc(page);

	ATF_REQUIRE(vec != NULL);
	ATF_REQUIRE(map != MAP_FAILED);

	errno = 0;
	ATF_REQUIRE_ERRNO(EINVAL, mincore(map, 0, vec) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(ENOMEM, mincore(0, page, vec) == -1);

	errno = 0;
	ATF_REQUIRE_ERRNO(EFAULT, mincore(map, page, (void *)-1) == -1);

	free(vec);
	ATF_REQUIRE(munmap(map, page) == 0);
}

ATF_TC_WITH_CLEANUP(mincore_incore);
ATF_TC_HEAD(mincore_incore, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test that mincore(2) works");
}

ATF_TC_BODY(mincore_incore, tc)
{
	char *buf, *vec, *map = MAP_FAILED;
	const char *str = NULL;
	const size_t n = 3;
	ssize_t tot;
	int fd, rv;
	size_t i, j;

	/*
	 * Create a temporary file, write
	 * few pages to it, and map the file.
	 */
	buf = calloc(n, page);
	vec = calloc(n, page);

	if (buf == NULL || vec == NULL)
		return;

	for (i = 0; i < (size_t)page * n; i++)
		buf[i] = 'x';

	fd = open(path, O_RDWR | O_CREAT, 0600);

	if (fd < 0) {
		str = "failed to open";
		goto out;
	}

	tot = 0;

	while (tot < page * (long)n) {

		rv = write(fd, buf, sizeof(buf));

		if (rv < 0) {
			str = "failed to write";
			goto out;
		}

		tot += rv;
	}

	map = mmap(NULL, page * n, PROT_READ | PROT_WRITE,
	    MAP_FILE | MAP_PRIVATE, fd, 0);

	if (map == MAP_FAILED) {
		str = "failed to map";
		goto out;
	}

	/*
	 * Lock the mapping such that only
	 * in-core page status is returned.
	 */
	if (mlock(map, page * n) != 0) {
		str = "failed to lock";
		goto out;
	}

	if (mincore(map, page * n, vec) != 0) {
		str = "mincore failed";
		goto out;
	}

	/*
	 * Check that the in-core pages
	 * match the locked pages.
	 */
	for (i = j = 0; i < (size_t)page * n; i++) {

		if (vec[i] != 0)
			j++;
	}

	if (j != n)
		str = "mismatch of in-core pages";

out:
	free(buf);
	free(vec);

	(void)close(fd);
	(void)unlink(path);

	if (map != MAP_FAILED) {
		(void)munlock(map, page);
		(void)munmap(map, page);
	}

	if (str != NULL)
		atf_tc_fail("%s", str);
}

ATF_TC_CLEANUP(mincore_incore, tc)
{
	(void)unlink(path);
}

ATF_TP_ADD_TCS(tp)
{

	page = sysconf(_SC_PAGESIZE);
	ATF_REQUIRE(page >= 0);

	ATF_TP_ADD_TC(tp, mincore_err);
	ATF_TP_ADD_TC(tp, mincore_incore);

	return atf_no_error();
}
