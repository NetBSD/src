/*	$NetBSD: t_aio_lio.c,v 1.1 2025/10/10 15:53:55 christos Exp $	*/

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
#include <unistd.h>

static int	mktemp_file(char *, size_t);
static void	fill_pattern(uint8_t *, size_t, uint8_t);
static void	wait_all(const struct aiocb * const [], size_t);

static int
mktemp_file(char *path, size_t pathlen)
{
	int fd, n;

	n = snprintf(path, pathlen, "t_aio_lio.XXXXXX");
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

ATF_TC_WITHOUT_HEAD(lio_nowait);
ATF_TC_BODY(lio_nowait, tc)
{
	char path[64];
	int fd, rv;
#define	NW_REQS		8
#define	NW_BLKSIZ	8192
	uint8_t *bufs[NW_REQS];
	struct aiocb cbs[NW_REQS];
	struct aiocb *list[NW_REQS];
	off_t off;
	size_t i;

	fd = mktemp_file(path, sizeof(path));

	off = 0;
	for (i = 0; i < NW_REQS; i++) {
		bufs[i] = malloc(NW_BLKSIZ);
		ATF_REQUIRE(bufs[i] != NULL);

		fill_pattern(bufs[i], NW_BLKSIZ, (uint8_t)i);

		memset(&cbs[i], 0, sizeof(cbs[i]));
		cbs[i].aio_fildes = fd;
		cbs[i].aio_buf = bufs[i];
		cbs[i].aio_nbytes = NW_BLKSIZ;
		cbs[i].aio_offset = off;
		cbs[i].aio_lio_opcode = LIO_WRITE;

		list[i] = &cbs[i];
		off += (off_t)NW_BLKSIZ;
	}

	rv = lio_listio(LIO_NOWAIT, list, (int)NW_REQS, NULL);
	ATF_REQUIRE_EQ_MSG(0, rv, "lio_listio failed: %s",
		strerror(errno));

	wait_all((const struct aiocb * const *)list, NW_REQS);

	for (i = 0; i < NW_REQS; i++) {
		int err;
		ssize_t done;

		err = aio_error(&cbs[i]);
		ATF_REQUIRE_EQ(0, err);

		done = aio_return(&cbs[i]);
		ATF_REQUIRE_EQ(NW_BLKSIZ, done);

		free(bufs[i]);
	}

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);
}

ATF_TC_WITHOUT_HEAD(lio_wait_write_then_read);
ATF_TC_BODY(lio_wait_write_then_read, tc)
{
	char path[64];
	int fd, rv;
#define WWTR_REQS	4
#define WWTR_BLKSIZ	4096

	uint8_t *wbufs[WWTR_REQS];
	struct aiocb wcbs[WWTR_REQS];
	struct aiocb *wlist[WWTR_REQS];

	uint8_t *rbufs[WWTR_REQS];
	struct aiocb rcbs[WWTR_REQS];
	struct aiocb *rlist[WWTR_REQS];

	size_t i;
	off_t off;

	fd = mktemp_file(path, sizeof(path));

	off = 0;
	for (i = 0; i < WWTR_REQS; i++) {
		wbufs[i] = malloc(WWTR_BLKSIZ);
		ATF_REQUIRE(wbufs[i] != NULL);

		fill_pattern(wbufs[i], WWTR_BLKSIZ, (uint8_t)(0xA0 + i));

		memset(&wcbs[i], 0, sizeof(wcbs[i]));
		wcbs[i].aio_fildes = fd;
		wcbs[i].aio_buf = wbufs[i];
		wcbs[i].aio_nbytes = WWTR_BLKSIZ;
		wcbs[i].aio_offset = off;
		wcbs[i].aio_lio_opcode = LIO_WRITE;

		wlist[i] = &wcbs[i];
		off += WWTR_BLKSIZ;
	}

	rv = lio_listio(LIO_WAIT, wlist, (int)WWTR_REQS, NULL);
	ATF_REQUIRE_EQ_MSG(0, rv, "lio_listio write failed: %s",
		strerror(errno));

	for (i = 0; i < WWTR_REQS; i++) {
		int err;
		ssize_t done;

		err = aio_error(&wcbs[i]);
		ATF_REQUIRE_EQ(0, err);

		done = aio_return(&wcbs[i]);
		ATF_REQUIRE_EQ(WWTR_BLKSIZ, done);
	}

	for (i = 0; i < WWTR_REQS; i++) {
		rbufs[i] = calloc(1, WWTR_BLKSIZ);
		ATF_REQUIRE(rbufs[i] != NULL);

		memset(&rcbs[i], 0, sizeof(rcbs[i]));
		rcbs[i].aio_fildes = fd;
		rcbs[i].aio_buf = rbufs[i];
		rcbs[i].aio_nbytes = WWTR_BLKSIZ;
		rcbs[i].aio_offset = (off_t)i * WWTR_BLKSIZ;
		rcbs[i].aio_lio_opcode = LIO_READ;

		rlist[i] = &rcbs[i];
	}

	rv = lio_listio(LIO_NOWAIT, rlist, WWTR_REQS, NULL);
	ATF_REQUIRE_EQ_MSG(0, rv, "lio_listio read failed: %s",
		strerror(errno));

	wait_all((const struct aiocb * const *)rlist, WWTR_REQS);

	for (i = 0; i < WWTR_REQS; i++) {
		int err;
		ssize_t done;

		err = aio_error(&rcbs[i]);
		ATF_REQUIRE_EQ(0, err);

		done = aio_return(&rcbs[i]);
		ATF_REQUIRE_EQ(WWTR_BLKSIZ, done);

		ATF_REQUIRE_EQ(0, memcmp(wbufs[i], rbufs[i], WWTR_BLKSIZ));

		free(wbufs[i]);
		free(rbufs[i]);
	}

	rv = close(fd);
	ATF_REQUIRE_EQ(0, rv);
	rv = unlink(path);
	ATF_REQUIRE_EQ(0, rv);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, lio_nowait);
	ATF_TP_ADD_TC(tp, lio_wait_write_then_read);

	return atf_no_error();
}
