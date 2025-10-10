/* $NetBSD: t_aio_rw.c,v 1.1 2025/10/10 15:53:55 christos Exp $ */

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
static void	wait_all(const struct aiocb * const [], size_t);

static int
mktemp_file(char *path, size_t pathlen)
{
	int fd, n;

	n = snprintf(path, pathlen, "t_aio_rw.XXXXXX");
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
wait_all(const struct aiocb * const list[], size_t nent)
{
	size_t i;
	int pending, rv, error;

	for (;;) {
		pending = 0;

		for (i = 0; i < nent; i++) {
			if (list[i] == NULL) {
				continue;
			}

			error = aio_error(list[i]);
			if (error == EINPROGRESS) {
				pending = 1;
			}
		}

		if (!pending) {
			break;
		}

		rv = aio_suspend(list, (int)nent, NULL);
		ATF_REQUIRE_EQ_MSG(0, rv, "aio_suspend failed: %s",
			strerror(errno));
	}
}

/*
 * write_then_read_back
 * Write a block then read it back asynchronously and compare.
 */
ATF_TC_WITHOUT_HEAD(write_then_read_back);
ATF_TC_BODY(write_then_read_back, tc)
{
	char path[64];
	int fd, rv;
	const size_t blksz = 0x2000;
	uint8_t *wbuf, *rbuf;
	struct aiocb wcb, rcb;
	const struct aiocb *wlist[1], *rlist[1];

	fd = mktemp_file(path, sizeof(path));

	wbuf = malloc(blksz);
	rbuf = calloc(1, blksz);
	ATF_REQUIRE(wbuf != NULL && rbuf != NULL);

	fill_pattern(wbuf, blksz, 0xA0);

	memset(&wcb, 0, sizeof(wcb));
	wcb.aio_fildes = fd;
	wcb.aio_buf = wbuf;
	wcb.aio_nbytes = blksz;
	wcb.aio_offset = 0;

	rv = aio_write(&wcb);
	ATF_REQUIRE_EQ(0, rv);
	wlist[0] = &wcb;
	wait_all(wlist, 1);

	ATF_REQUIRE_EQ(0, aio_error(&wcb));
	ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&wcb));

	memset(&rcb, 0, sizeof(rcb));
	rcb.aio_fildes = fd;
	rcb.aio_buf = rbuf;
	rcb.aio_nbytes = blksz;
	rcb.aio_offset = 0;

	rv = aio_read(&rcb);
	ATF_REQUIRE_EQ(0, rv);
	rlist[0] = &rcb;
	wait_all(rlist, 1);

	ATF_REQUIRE_EQ(0, aio_error(&rcb));
	ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&rcb));
	ATF_REQUIRE_EQ(0, memcmp(wbuf, rbuf, blksz));

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);

	free(wbuf);
	free(rbuf);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, write_then_read_back);
	return atf_no_error();
}
