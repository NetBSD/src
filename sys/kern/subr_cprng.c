/*	$NetBSD: subr_cprng.c,v 1.7.2.2 2012/04/17 00:08:27 yamt Exp $ */

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
#include <sys/rnd.h>
#include <dev/rnd_private.h>

#if defined(__HAVE_CPU_COUNTER)
#include <machine/cpu_counter.h>
#endif

#include <sys/cprng.h>

__KERNEL_RCSID(0, "$NetBSD: subr_cprng.c,v 1.7.2.2 2012/04/17 00:08:27 yamt Exp $");

void
cprng_init(void)
{
	nist_ctr_initialize();
}

static inline uint32_t
cprng_counter(void)
{
	struct timeval tv;

#if defined(__HAVE_CPU_COUNTER)
	if (cpu_hascounter())
		return cpu_counter32();
#endif
	if (__predict_false(cold)) {
		/* microtime unsafe if clock not running yet */
		return 0;
	}
	microtime(&tv);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
}

static void
cprng_strong_sched_reseed(cprng_strong_t *const c)
{
	KASSERT(mutex_owned(&c->mtx));
	if (!(c->reseed_pending) && mutex_tryenter(&c->reseed.mtx)) {
		c->reseed_pending = 1;
		c->reseed.len = NIST_BLOCK_KEYLEN_BYTES;
		rndsink_attach(&c->reseed);
		mutex_spin_exit(&c->reseed.mtx);
	}
}

static void
cprng_strong_reseed(void *const arg)
{
	cprng_strong_t *c = arg;
	uint8_t key[NIST_BLOCK_KEYLEN_BYTES];
	uint32_t cc = cprng_counter();

	if (!mutex_tryenter(&c->mtx)) {
	    return;
	}

	if (c->reseed.len != sizeof(key)) {
		panic("cprng_strong_reseed: bad entropy length %d "
		      " (expected %d)", (int)c->reseed.len, (int)sizeof(key));
	}
	if (nist_ctr_drbg_reseed(&c->drbg, c->reseed.data, c->reseed.len,
				 &cc, sizeof(cc))) {
		panic("cprng %s: nist_ctr_drbg_reseed failed.", c->name);
	}
	c->reseed_pending = 0;
	if (c->flags & CPRNG_USE_CV) {
		cv_broadcast(&c->cv);
	}
	selnotify(&c->selq, 0, 0);
	mutex_exit(&c->mtx);
}

cprng_strong_t *
cprng_strong_create(const char *const name, int ipl, int flags)
{
	cprng_strong_t *c;
	uint8_t key[NIST_BLOCK_KEYLEN_BYTES];
	int r, getmore = 0, hard = 0;
	uint32_t cc;

	c = kmem_alloc(sizeof(*c), KM_NOSLEEP);
	if (c == NULL) {
		return NULL;
	}
	c->flags = flags;
	strlcpy(c->name, name, sizeof(c->name));
	c->reseed_pending = 0;
	c->reseed.cb = cprng_strong_reseed;
	c->reseed.arg = c;
	mutex_init(&c->reseed.mtx, MUTEX_DEFAULT, IPL_VM);
	strlcpy(c->reseed.name, name, sizeof(c->reseed.name));

	mutex_init(&c->mtx, MUTEX_DEFAULT, ipl);

	if (c->flags & CPRNG_USE_CV) {
		cv_init(&c->cv, name);
	}

	selinit(&c->selq);

	r = rnd_extract_data(key, sizeof(key), RND_EXTRACT_GOOD);
	if (r != sizeof(key)) {
		if (c->flags & CPRNG_INIT_ANY) {
#ifdef DEBUG
			printf("cprng %s: WARNING insufficient "
			       "entropy at creation.\n", name);
#endif
			rnd_extract_data(key + r, sizeof(key - r),
					 RND_EXTRACT_ANY);
		} else {
			hard++;
		}
		getmore++;
	}
		
	if (nist_ctr_drbg_instantiate(&c->drbg, key, sizeof(key),
				      &cc, sizeof(cc), name, strlen(name))) {
		panic("cprng %s: instantiation failed.", name);
	}

	if (getmore) {
		/* Cause readers to wait for rekeying. */
		if (hard) {
			c->drbg.reseed_counter =
			    NIST_CTR_DRBG_RESEED_INTERVAL + 1;
		} else {
			c->drbg.reseed_counter =
			    (NIST_CTR_DRBG_RESEED_INTERVAL / 2) + 1;
		}
	}
	return c;
}

size_t
cprng_strong(cprng_strong_t *const c, void *const p, size_t len, int flags)
{
	uint32_t cc = cprng_counter();
#ifdef DEBUG
	int testfail = 0;
#endif
	if (len > CPRNG_MAX_LEN) {	/* XXX should we loop? */
		len = CPRNG_MAX_LEN;	/* let the caller loop if desired */
	}
	mutex_enter(&c->mtx);

	if (nist_ctr_drbg_generate(&c->drbg, p, len, &cc, sizeof(cc))) {
		/* A generator failure really means we hit the hard limit. */
		if (c->flags & CPRNG_REKEY_ANY) {
			uint8_t key[NIST_BLOCK_KEYLEN_BYTES];

 			printf("cprng %s: WARNING pseudorandom rekeying.\n",
			       c->name);
                        rnd_extract_data(key, sizeof(key), RND_EXTRACT_ANY);
			cc = cprng_counter();
			if (nist_ctr_drbg_reseed(&c->drbg, key, sizeof(key),
			    &cc, sizeof(cc))) {
				panic("cprng %s: nist_ctr_drbg_reseed "
				      "failed.", c->name);
			}
		} else {
			if (!(flags & FNONBLOCK) && 
			     (c->flags & CPRNG_USE_CV)) {
				int wr;

				cprng_strong_sched_reseed(c);
				do {
					wr = cv_wait_sig(&c->cv, &c->mtx);
					if (wr == ERESTART) {
						mutex_exit(&c->mtx);
						return 0;
					}
				} while (nist_ctr_drbg_generate(&c->drbg, p,
								len, &cc,
								sizeof(cc)));
			} else {
				len = 0;
			}
		}
	}

#ifdef DEBUG
	/*
	 * If the generator has just been keyed, perform
	 * the statistical RNG test.
	 */
	if (__predict_false(c->drbg.reseed_counter == 1)) {
		rngtest_t *rt = kmem_alloc(sizeof(*rt), KM_NOSLEEP);

		if (rt) {

			strncpy(rt->rt_name, c->name, sizeof(rt->rt_name));

			if (nist_ctr_drbg_generate(&c->drbg, rt->rt_b,
		  	    sizeof(rt->rt_b), NULL, 0)) {
				panic("cprng %s: nist_ctr_drbg_generate "
				      "failed!", c->name);
		
			}
			testfail = rngtest(rt);

			if (testfail) {
				printf("cprng %s: failed statistical RNG "
				       "test.\n", c->name);
				c->drbg.reseed_counter =
				    NIST_CTR_DRBG_RESEED_INTERVAL + 1;
				len = 0;
			}
			memset(rt, 0, sizeof(*rt));
			kmem_free(rt, sizeof(*rt));
		}
	}
#endif
	if (__predict_false(c->drbg.reseed_counter >
			     (NIST_CTR_DRBG_RESEED_INTERVAL / 2))) {
		cprng_strong_sched_reseed(c);
	}

	if (rnd_full) {
		if (!c->rekeyed_on_full) {
			c->rekeyed_on_full++;
			cprng_strong_sched_reseed(c);
		}
	} else {
		c->rekeyed_on_full = 0;
	}

	mutex_exit(&c->mtx);
	return len;
}

void
cprng_strong_destroy(cprng_strong_t *c)
{
	mutex_enter(&c->mtx);
	mutex_spin_enter(&c->reseed.mtx);

	if (c->flags & CPRNG_USE_CV) {
		KASSERT(!cv_has_waiters(&c->cv));
		cv_destroy(&c->cv);
	}
	seldestroy(&c->selq);

	if (c->reseed_pending) {
		rndsink_detach(&c->reseed);
	}
	mutex_spin_exit(&c->reseed.mtx);
	mutex_destroy(&c->reseed.mtx);

	nist_ctr_drbg_destroy(&c->drbg);

	mutex_exit(&c->mtx);
	mutex_destroy(&c->mtx);

	memset(c, 0, sizeof(*c));
	kmem_free(c, sizeof(*c));
}

int
cprng_strong_getflags(cprng_strong_t *const c)
{
	KASSERT(mutex_owned(&c->mtx));
	return c->flags;
}

void
cprng_strong_setflags(cprng_strong_t *const c, int flags)
{
	KASSERT(mutex_owned(&c->mtx));
	if (flags & CPRNG_USE_CV) {
		if (!(c->flags & CPRNG_USE_CV)) {
			cv_init(&c->cv, (const char *)c->name);
		}
	} else {
		if (c->flags & CPRNG_USE_CV) {
			KASSERT(!cv_has_waiters(&c->cv));
			cv_destroy(&c->cv);
		}
	}
	if (flags & CPRNG_REKEY_ANY) {
		if (!(c->flags & CPRNG_REKEY_ANY)) {
			if (c->flags & CPRNG_USE_CV) {
				cv_broadcast(&c->cv);
			}
			selnotify(&c->selq, 0, 0);
		}
	}
	c->flags = flags;
}
