/*	$NetBSD: cprng_stub.c,v 1.4 2011/12/17 20:05:40 tls Exp $ */

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rngtest.h>

#include <sys/cprng.h>

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
cprng_strong_create(const char *const name, int ipl, int flags)
{
	cprng_strong_t *c;

	/* zero struct to zero counters we won't ever set with no DRBG */
	c = kmem_zalloc(sizeof(*c), KM_NOSLEEP);
	if (c == NULL) {
		return NULL;
	}
	strlcpy(c->name, name, sizeof(c->name));
	mutex_init(&c->mtx, MUTEX_DEFAULT, ipl);
	if (c->flags & CPRNG_USE_CV) {
		cv_init(&c->cv, name);
	}
	selinit(&c->selq);
	return c;
}


size_t
cprng_strong(cprng_strong_t *c, void *p, size_t len, int blocking)
{
	mutex_enter(&c->mtx);
	cprng_fast(p, len);		/* XXX! */
	mutex_exit(&c->mtx);
	return len;
}

void
cprng_strong_destroy(cprng_strong_t *c)
{
	mutex_destroy(&c->mtx);
	cv_destroy(&c->cv);
	memset(c, 0, sizeof(*c));
	kmem_free(c, sizeof(*c));
}

size_t
cprng_fast(void *p, size_t len)
{
	uint8_t *resp, *pchar = (uint8_t *)p;
	uint32_t res;
	size_t i;

	do {
		res = rumpuser_arc4random();
		resp = (uint8_t *)&res;

		for (i = 0; i < sizeof(res); i++) {
		    *pchar++ = resp[i];
		    if (pchar == (uint8_t *)p + len) {
			return len;
		    }
		}
	} while(1);
}

uint32_t
cprng_fast32(void)
{
	return rumpuser_arc4random();
}

uint64_t
cprng_fast64(void)
{
	uint64_t ret;
	uint32_t *ret32;

	ret32 = (uint32_t *)&ret;
	ret32[0] = rumpuser_arc4random();
	ret32[1] = rumpuser_arc4random();
	return ret;
}
