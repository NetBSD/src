/*	$NetBSD: h_pthread_weak.c,v 1.1 2025/10/06 13:16:44 riastradh Exp $	*/

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
__RCSID("$NetBSD: h_pthread_weak.c,v 1.1 2025/10/06 13:16:44 riastradh Exp $");

#define	_NETBSD_PTHREAD_CREATE_WEAK

#include "h_pthread_weak.h"

#include <atf-c.h>
#include <pthread.h>

#include "h_macros.h"

static void *
start(void *cookie)
{
	return cookie;
}

void
test_mutex(void)
{
	pthread_mutex_t mtx;

	RZ(pthread_mutex_init(&mtx, NULL));
	RZ(pthread_mutex_lock(&mtx));
	RZ(pthread_mutex_unlock(&mtx));
	RZ(pthread_mutex_destroy(&mtx));
}

void
test_thread_creation(void)
{
	int cookie = 123;
	pthread_attr_t attr;
	pthread_t t;
	void *result;

	RZ(pthread_attr_init(&attr));
	RZ(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE));
	RZ(pthread_create(&t, NULL, &start, &cookie));
	RZ(pthread_attr_destroy(&attr));
	RZ(pthread_join(t, &result));
	ATF_CHECK_EQ(result, &cookie);
}

void
test_thread_creation_failure(void)
{
	int cookie = 123;
	pthread_t t;
	int error;

	error = pthread_create(&t, NULL, &start, &cookie);
	ATF_CHECK_MSG(error != 0, "pthread_create unexpectedly succeeded");
}
