/*	$NetBSD: argon2_utils.c,v 1.1 2021/11/22 14:34:35 nia Exp $ */
/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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

#include <sys/resource.h>
#include <sys/sysctl.h>
#include <argon2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <util.h>
#include <err.h>

#include "argon2_utils.h"

#ifndef lint
__RCSID("$NetBSD: argon2_utils.c,v 1.1 2021/11/22 14:34:35 nia Exp $");
#endif

static size_t
get_cpucount(void)
{
	const int mib[] = { CTL_HW, HW_NCPUONLINE };
	int ncpuonline = 1;
	size_t ncpuonline_len = sizeof(ncpuonline);

	if (sysctl(mib, __arraycount(mib),
	    &ncpuonline, &ncpuonline_len, NULL, 0) == -1) {
		return 1;
	}
	return ncpuonline;
}

static uint64_t
get_usermem(void)
{
	const int mib[] = { CTL_HW, HW_USERMEM64 };
	uint64_t usermem64 = 0;
	size_t usermem64_len = sizeof(usermem64);
	struct rlimit rlim;

	if (sysctl(mib, __arraycount(mib),
	    &usermem64, &usermem64_len, NULL, 0) == -1) {
		return 0;
	}

	if (getrlimit(RLIMIT_AS, &rlim) == -1)
		return usermem64;
	if (usermem64 > rlim.rlim_cur && rlim.rlim_cur != RLIM_INFINITY)
		usermem64 = rlim.rlim_cur;
	return usermem64; /* bytes */
}

void
argon2id_calibrate(size_t keylen, size_t saltlen,
    size_t *iterations, size_t *memory, size_t *parallelism)
{
	size_t mem = ARGON2_MIN_MEMORY; /* kilobytes */
	size_t time;
	const size_t ncpus = get_cpucount();
	const uint64_t usermem = get_usermem(); /* bytes */
	struct timespec tp1, tp2;
	struct timespec delta;
	unsigned int limit = 0;
	uint8_t *key = NULL, *salt = NULL;
	uint8_t tmp_pwd[17]; /* just random data for testing */
	int ret = ARGON2_OK;

	key = emalloc(keylen);
	salt = emalloc(saltlen);

	arc4random_buf(tmp_pwd, sizeof(tmp_pwd));
	arc4random_buf(salt, saltlen);

	/* 1kb to argon2 per 100kb of user memory */
	mem = usermem / 100000;

	/* 256k: reasonable lower bound from the argon2 test suite */
	if (mem < 256)
		mem = 256;

	fprintf(stderr, "calibrating argon2id parameters...");

	/* Decrease 'mem' if it slows down computation too much */

	do {
		if (clock_gettime(CLOCK_MONOTONIC, &tp1) == -1)
			goto error;
		if ((ret = argon2_hash(ARGON2_MIN_TIME, mem, ncpus,
		    tmp_pwd, sizeof(tmp_pwd),
		    salt, saltlen,
		    key, keylen,
		    NULL, 0,
		    Argon2_id, ARGON2_VERSION_NUMBER)) != ARGON2_OK) {
			goto error_argon2;
		}
		fprintf(stderr, ".");
		if (clock_gettime(CLOCK_MONOTONIC, &tp2) == -1)
			goto error;
		if (timespeccmp(&tp1, &tp2, >))
			goto error_clock;
		timespecsub(&tp2, &tp1, &delta);
		if (delta.tv_sec >= 1)
			mem /= 2;
		if (mem < ARGON2_MIN_MEMORY) {
			mem = ARGON2_MIN_MEMORY;
			break;
		}
	} while (delta.tv_sec >= 1 && (limit++) < 3);

	delta.tv_sec = 0;
	delta.tv_nsec = 0;

	/* Increase 'time' until we reach a second */

	for (time = ARGON2_MIN_TIME; delta.tv_sec < 1 &&
	    time < ARGON2_MAX_TIME; time <<= 1) {
		if (clock_gettime(CLOCK_MONOTONIC, &tp1) == -1)
			goto error;
		if ((ret = argon2_hash(time, mem, ncpus,
		    tmp_pwd, sizeof(tmp_pwd),
		    salt, saltlen,
		    key, keylen,
		    NULL, 0,
		    Argon2_id, ARGON2_VERSION_NUMBER)) != ARGON2_OK) {
			goto error_argon2;
		}
		fprintf(stderr, ".");
		if (clock_gettime(CLOCK_MONOTONIC, &tp2) == -1)
			goto error;
		if (timespeccmp(&tp1, &tp2, >))
			goto error_clock;
		timespecsub(&tp2, &tp1, &delta);
	}

	if (time > ARGON2_MIN_TIME)
		time >>= 1;

	fprintf(stderr, " done\n");

	free(key);
	free(salt);
	*iterations = time;
	*memory = mem;
	*parallelism = ncpus;
	return;

error_argon2:
	errx(EXIT_FAILURE,
	    " failed to calculate Argon2 hash, error code %d", ret);
error_clock:
	errx(EXIT_FAILURE,
	    " failed to calibrate hash parameters: broken monotonic clock?");
error:
	err(EXIT_FAILURE, " failed to calibrate hash parameters");
}
