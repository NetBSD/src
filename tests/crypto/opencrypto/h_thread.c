/* $NetBSD: h_thread.c,v 1.2.2.2 2026/06/27 15:23:23 martin Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <crypto/cryptodev.h>

#define	atomic_load_acquire(p)	atomic_load_explicit(p, memory_order_acquire)
#define	atomic_load_relaxed(p)	atomic_load_explicit(p, memory_order_relaxed)
#define	atomic_store_relaxed(p, v)					      \
	atomic_store_explicit(p, v, memory_order_relaxed)
#define	atomic_store_release(p, v)					      \
	atomic_store_explicit(p, v, memory_order_release)

/* Test vectors from RFC1321 */

const struct {
	size_t len;
	unsigned char plaintx[80];
	unsigned char digest[16];
} tests[] = {
	{ 0, "",
	  { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
	    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e } },
	{ 1, "a",
	  { 0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
	    0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 } },
	{ 3, "abc",
	  { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
	    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 } },
	{ 14, "message digest",
	  { 0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
	    0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0 } },
	{ 26, "abcdefghijklmnopqrstuvwxyz",
	  { 0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
	    0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b } },
	{ 62, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
	  { 0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
	    0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f } },
	{ 80, "123456789012345678901234567890123456789012345678901234567890"
		"12345678901234567890",
	  { 0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
	    0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a } },
};

struct arg {
	char msg;
};

static int fd;
static atomic_bool done = false;
static atomic_uint_fast32_t ses = 0;
static timer_t timer;

static void *
t_openclose(void *v)
{
	struct arg *a = v;

	while (!atomic_load_relaxed(&done)) {
		struct session_op cs;

		memset(&cs, 0, sizeof(cs));
		cs.mac = CRYPTO_MD5;
		if (ioctl(fd, CIOCGSESSION, &cs) == -1)
			err(EXIT_FAILURE, "CIOCGSESSION");

		atomic_store_release(&ses, cs.ses);
		sched_yield();
		atomic_store_relaxed(&ses, 0);

		while (ioctl(fd, CIOCFSESSION, &cs.ses) == -1) {
			if (errno != EBUSY)
				err(EXIT_FAILURE, "CIOCFSESSION");
		}

		fprintf(stderr, "%c", a->msg);
	}

	return NULL;
}

static void *
t_encrypt(void *v)
{
	struct crypt_op co;
	unsigned char buf[16];
	struct arg *a = v;

	while (!atomic_load_relaxed(&done)) {
	for (size_t i = 0; i < __arraycount(tests); i++) {
		memset(&co, 0, sizeof(co));
		memset(&buf, 0, sizeof(buf));
		while ((co.ses = atomic_load_acquire(&ses)) == 0) {
			if (atomic_load_relaxed(&done))
				goto out;
			continue;
		}
		co.op = COP_ENCRYPT;
		co.len = tests[i].len;
		co.src = __UNCONST(&tests[i].plaintx);
		co.mac = buf;
		if (ioctl(fd, CIOCCRYPT, &co) == -1) {
			if (atomic_load_relaxed(&ses) == co.ses)
				err(EXIT_FAILURE, "CIOCCRYPT");
			continue;
		}
		if (memcmp(co.mac, tests[i].digest, sizeof(tests[i].digest)))
			errx(1, "verification failed test %zu", i);
		fprintf(stderr, "%c", a->msg);
	}
	}
out:
	return NULL;
}

static void
abortalarm(unsigned sec)
{
	struct sigevent sigev;

	memset(&sigev, 0, sizeof(sigev));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = SIGABRT;

	if (timer_create(CLOCK_MONOTONIC, &sigev, &timer) == -1)
		err(EXIT_FAILURE, "timer_create");
	if (timer_settime(timer, TIMER_RELTIME,
		&(const struct itimerspec) { .it_value = {sec, 0} },
		NULL) == -1)
		err(EXIT_FAILURE, "timer_settime");
}

int
main(void)
{
	pthread_t ta, tb, tc;
	struct arg a, b, c;
	int error;

	if ((fd = open("/dev/crypto", O_RDWR, 0)) == -1)
		err(EXIT_FAILURE, "open /dev/crypto");

	a.msg = '/';
	error = pthread_create(&ta, NULL, t_openclose, &a);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_create A");

	b = a;
	b.msg = '-';
	error = pthread_create(&tb, NULL, t_encrypt, &b);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_create B");
	c = a;
	c.msg = '+';
	error = pthread_create(&tc, NULL, t_encrypt, &c);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_create C");

	sleep(5);
	fprintf(stderr, "|");
	atomic_store_relaxed(&done, true);
	abortalarm(1);
	error = pthread_join(ta, NULL);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_join A");
	error = pthread_join(tb, NULL);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_join B");
	error = pthread_join(tc, NULL);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_join C");

	if (ioctl(fd, CIOCFSESSION, &ses) != -1)
		errx(EXIT_FAILURE, "CIOCFSESSION failed to fail");
	if (errno != EINVAL)
		err(EXIT_FAILURE, "CIOCFSESSION");
	return EXIT_SUCCESS;
}
