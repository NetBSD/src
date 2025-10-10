/*	$NetBSD: t_aio_suspend.c,v 1.1 2025/10/10 15:53:55 christos Exp $	*/

/*
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <atf-c.h>

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int	mktemp_file(char *, size_t);
static void	fill_pattern(uint8_t *, size_t, uint8_t);
static void	wait_cb(struct aiocb *);

static int
mktemp_file(char *path, size_t pathlen)
{
	int fd, n;

	n = snprintf(path, pathlen, "t_aio_suspend.XXXXXX");
	ATF_REQUIRE(n > 0 && (size_t)n < pathlen);

	fd = mkstemp(path);
	ATF_REQUIRE(fd >= 0);

	return fd;
}

static void
fill_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
	size_t i;

	for (i = 0; i < len; i++) {
		buf[i] = (uint8_t)(seed + (i & 0xff));
	}
}

static void
wait_cb(struct aiocb *cb)
{
	const struct aiocb *one[1];
	int rv;

	one[0] = cb;
	while (aio_error(cb) == EINPROGRESS) {
		rv = aio_suspend(one, 1, NULL);
		ATF_REQUIRE_EQ(0, rv);
	}
	if (aio_error(cb) == 0) {
		aio_return(cb);
	}
}

ATF_TC_WITHOUT_HEAD(suspend_any);
ATF_TC_BODY(suspend_any, tc)
{
	char path[64];
	int fd, rv;
	const size_t blksz = 4096;
	uint8_t *buf0, *buf1;
	struct aiocb cb0, cb1;
	const struct aiocb *list[2];
	int done;

	fd = mktemp_file(path, sizeof(path));

	buf0 = malloc(blksz);
	buf1 = malloc(blksz);
	ATF_REQUIRE(buf0 != NULL && buf1 != NULL);
	fill_pattern(buf0, blksz, 0x20);
	fill_pattern(buf1, blksz, 0x40);

	memset(&cb0, 0, sizeof(cb0));
	cb0.aio_fildes = fd;
	cb0.aio_buf = buf0;
	cb0.aio_nbytes = blksz;
	cb0.aio_offset = 0;

	memset(&cb1, 0, sizeof(cb1));
	cb1.aio_fildes = fd;
	cb1.aio_buf = buf1;
	cb1.aio_nbytes = blksz;
	cb1.aio_offset = blksz;

	ATF_REQUIRE_EQ(0, aio_write(&cb0));
	ATF_REQUIRE_EQ(0, aio_write(&cb1));

	list[0] = &cb0;
	list[1] = &cb1;

	rv = aio_suspend(list, 2, NULL);
	ATF_REQUIRE_EQ(0, rv);

	done = 0;
	if (aio_error(&cb0) != EINPROGRESS) {
		done++;
		if (aio_error(&cb0) == 0) {
			ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&cb0));
		} else {
			ATF_REQUIRE_EQ(ECANCELED, aio_error(&cb0));
			ATF_REQUIRE_EQ(-1, aio_return(&cb0));
		}
	}
	if (aio_error(&cb1) != EINPROGRESS) {
		done++;
		if (aio_error(&cb1) == 0) {
			ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&cb1));
		} else {
			ATF_REQUIRE_EQ(ECANCELED, aio_error(&cb1));
			ATF_REQUIRE_EQ(-1, aio_return(&cb1));
		}
	}
	ATF_REQUIRE(done >= 1);

	if (aio_error(&cb0) == EINPROGRESS) {
		wait_cb(&cb0);
	}
	if (aio_error(&cb1) == EINPROGRESS) {
		wait_cb(&cb1);
	}

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);

	free(buf0);
	free(buf1);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, suspend_any);
	return atf_no_error();
}
