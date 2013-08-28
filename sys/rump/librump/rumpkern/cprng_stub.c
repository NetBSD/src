/*	$NetBSD: cprng_stub.c,v 1.7.2.1 2013/08/28 23:59:37 rmind Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thor Lancelot Simon.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cprng.h>
#include <sys/event.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/rngtest.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <rump/rumpuser.h>

/*
 * This is all stubbed out because of rump build dependency issues I
 * cannot fix.  One is more-or-less caused by the longstanding bogosity
 * that sys/dev/rnd.c implements *both* the in-kernel interface *and*
 * the pseudodevice.  The other, by the fact that I am not smart enough
 * to understand how to deal with code in rumpkern that depends on code
 * that lives in sys/crypto.  Sigh.
 */

cprng_strong_t *kern_cprng = NULL;

void
cprng_init(void)
{
	return;
}

cprng_strong_t *
cprng_strong_create(const char *const name __unused, int ipl __unused,
    int flags __unused)
{
	return NULL;
}

size_t
cprng_strong(cprng_strong_t *c __unused, void *p, size_t len,
    int blocking __unused)
{
	KASSERT(c == NULL);
	cprng_fast(p, len);		/* XXX! */
	return len;
}

int
cprng_strong_kqfilter(cprng_strong_t *c __unused, struct knote *kn __unused)
{
	KASSERT(c == NULL);
	kn->kn_data = CPRNG_MAX_LEN;
	return 1;
}

int
cprng_strong_poll(cprng_strong_t *c __unused, int events)
{
	KASSERT(c == NULL);
	return (events & (POLLIN | POLLRDNORM));
}

void
cprng_strong_destroy(cprng_strong_t *c __unused)
{
	KASSERT(c == NULL);
}

size_t
cprng_fast(void *p, size_t len)
{
	size_t randlen;

	rumpuser_getrandom(p, len, 0, &randlen);
	KASSERT(randlen == len);
	return len;
}

uint32_t
cprng_fast32(void)
{
	size_t randlen;
	uint32_t ret;

	rumpuser_getrandom(&ret, sizeof(ret), 0, &randlen);
	KASSERT(randlen == sizeof(ret));
	return ret;
}

uint64_t
cprng_fast64(void)
{
	uint64_t ret;

	size_t randlen;
	rumpuser_getrandom(&ret, sizeof(ret), 0, &randlen);
	KASSERT(randlen == sizeof(ret));
	return ret;
}
