/*	$NetBSD: t_aio_cancel.c,v 1.1 2025/10/10 15:53:55 christos Exp $	*/

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

	n = snprintf(path, pathlen, "t_aio_cancel.XXXXXX");
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
	int pending, rv;

	for (;;) {
		pending = 0;

		for (i = 0; i < nent; i++) {
			int err;

			if (list[i] == NULL) {
				continue;
			}

			err = aio_error(list[i]);
			if (err == EINPROGRESS) {
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

ATF_TC_WITHOUT_HEAD(cancel_active_write);
ATF_TC_BODY(cancel_active_write, tc)
{
	char path[64];
	int fd, rv, crv, err;
	const size_t blksz = 0x1000;
	uint8_t *wbuf;
	struct aiocb cb;
	const struct aiocb *list[1];

	fd = mktemp_file(path, sizeof(path));

	wbuf = malloc(blksz);
	ATF_REQUIRE(wbuf != NULL);
	fill_pattern(wbuf, blksz, 0x33);

	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = wbuf;
	cb.aio_nbytes = blksz;
	cb.aio_offset = 0;

	rv = aio_write(&cb);
	ATF_REQUIRE_EQ(0, rv);

	crv = aio_cancel(fd, &cb);
	ATF_REQUIRE(crv == AIO_CANCELED || crv == AIO_NOTCANCELED
	    || crv == AIO_ALLDONE);

	if (crv == AIO_CANCELED) {
		do {
			err = aio_error(&cb);
		} while (err == EINPROGRESS);
		ATF_REQUIRE_EQ(ECANCELED, err);
		ATF_REQUIRE_EQ(-1, aio_return(&cb));
	} else if (crv == AIO_NOTCANCELED) {
		list[0] = &cb;
		wait_all(list, 1);
		ATF_REQUIRE_EQ(0, aio_error(&cb));
		ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&cb));
	} else {
		do {
			err = aio_error(&cb);
		} while (err == EINPROGRESS);
		ATF_REQUIRE_EQ(0, err);
		ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&cb));
	}

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);

	free(wbuf);
}

ATF_TC_WITHOUT_HEAD(cancel_completed_request);
ATF_TC_BODY(cancel_completed_request, tc)
{
	char path[64];
	int fd, rv, crv;
	const size_t blksz = 4096;
	uint8_t *wbuf;
	struct aiocb cb;
	const struct aiocb *list[1];

	fd = mktemp_file(path, sizeof(path));

	wbuf = malloc(blksz);
	ATF_REQUIRE(wbuf != NULL);
	memset(wbuf, 0x7E, blksz);

	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = wbuf;
	cb.aio_nbytes = blksz;
	cb.aio_offset = 0;

	rv = aio_write(&cb);
	ATF_REQUIRE_EQ(0, rv);

	list[0] = &cb;
	wait_all(list, 1);
	ATF_REQUIRE_EQ(0, aio_error(&cb));
	ATF_REQUIRE_EQ((ssize_t)blksz, aio_return(&cb));

	crv = aio_cancel(fd, &cb);
	ATF_REQUIRE_EQ(AIO_ALLDONE, crv);

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);

	free(wbuf);
}

ATF_TC_WITHOUT_HEAD(cancel_invalid_fd);
ATF_TC_BODY(cancel_invalid_fd, tc)
{
	struct aiocb cb;
	int crv;

	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = -1;

	errno = 0;
	crv = aio_cancel(-1, &cb);
	ATF_REQUIRE_EQ(-1, crv);
	ATF_REQUIRE_EQ(EBADF, errno);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cancel_active_write);
	ATF_TP_ADD_TC(tp, cancel_completed_request);
	ATF_TP_ADD_TC(tp, cancel_invalid_fd);
	return atf_no_error();
}
