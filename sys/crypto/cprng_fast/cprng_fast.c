/*	$NetBSD: cprng_fast.c,v 1.17 2022/06/01 15:44:37 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: cprng_fast.c,v 1.17 2022/06/01 15:44:37 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/entropy.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/percpu.h>

#include <crypto/chacha/chacha.h>

#define	CPRNG_FAST_SEED_BYTES	CHACHA_STREAM_KEYBYTES

struct cprng_fast {
	/* 128-bit vector unit generates 256 bytes at once */
	uint8_t		buf[256];
	uint8_t		key[CPRNG_FAST_SEED_BYTES];
	uint8_t		nonce[CHACHA_STREAM_NONCEBYTES];
	unsigned	i;
	struct evcnt	*reseed_evcnt;
	unsigned	epoch;
};

static void	cprng_fast_init_cpu(void *, void *, struct cpu_info *);
static void	cprng_fast_reseed(struct cprng_fast *);

static void	cprng_fast_seed(struct cprng_fast *, const void *);
static void	cprng_fast_buf(struct cprng_fast *, void *, unsigned);

static void	cprng_fast_buf_short(void *, size_t);
static void	cprng_fast_buf_long(void *, size_t);

static percpu_t	*cprng_fast_percpu	__read_mostly;

void
cprng_fast_init(void)
{

	cprng_fast_percpu = percpu_create(sizeof(struct cprng_fast),
	    cprng_fast_init_cpu, NULL, NULL);
}

static void
cprng_fast_init_cpu(void *p, void *arg __unused, struct cpu_info *ci)
{
	struct cprng_fast *const cprng = p;

	cprng->epoch = 0;

	cprng->reseed_evcnt = kmem_alloc(sizeof(*cprng->reseed_evcnt),
	    KM_SLEEP);
	evcnt_attach_dynamic(cprng->reseed_evcnt, EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "cprng_fast reseed");
}

static int
cprng_fast_get(struct cprng_fast **cprngp)
{
	struct cprng_fast *cprng;
	int s;

	KASSERT(!cpu_intr_p());

	*cprngp = cprng = percpu_getref(cprng_fast_percpu);
	s = splsoftserial();

	if (__predict_false(cprng->epoch != entropy_epoch())) {
		splx(s);
		cprng_fast_reseed(cprng);
		s = splsoftserial();
	}

	return s;
}

static void
cprng_fast_put(struct cprng_fast *cprng, int s)
{

	KASSERT((cprng == percpu_getref(cprng_fast_percpu)) &&
	    (percpu_putref(cprng_fast_percpu), true));
	splx(s);
	percpu_putref(cprng_fast_percpu);
}

static void
cprng_fast_reseed(struct cprng_fast *cprng)
{
	unsigned epoch = entropy_epoch();
	uint8_t seed[CPRNG_FAST_SEED_BYTES];
	int s;

	cprng_strong(kern_cprng, seed, sizeof(seed), 0);

	s = splsoftserial();
	cprng_fast_seed(cprng, seed);
	cprng->epoch = epoch;
	cprng->reseed_evcnt->ev_count++;
	splx(s);

	explicit_memset(seed, 0, sizeof(seed));
}

/* CPRNG algorithm */

static void
cprng_fast_seed(struct cprng_fast *cprng, const void *seed)
{

	(void)memset(cprng->buf, 0, sizeof cprng->buf);
	(void)memcpy(cprng->key, seed, sizeof cprng->key);
	(void)memset(cprng->nonce, 0, sizeof cprng->nonce);
	cprng->i = sizeof cprng->buf;
}

static void
cprng_fast_buf(struct cprng_fast *cprng, void *buf, unsigned len)
{
	uint8_t *p = buf;
	unsigned n = len, n0;

	KASSERT(cprng->i <= sizeof(cprng->buf));
	KASSERT(len <= sizeof(cprng->buf));

	n0 = MIN(n, sizeof(cprng->buf) - cprng->i);
	memcpy(p, &cprng->buf[cprng->i], n0);
	if ((n -= n0) == 0) {
		cprng->i += n0;
		KASSERT(cprng->i <= sizeof(cprng->buf));
		return;
	}
	p += n0;
	le64enc(cprng->nonce, 1 + le64dec(cprng->nonce));
	chacha_stream(cprng->buf, sizeof(cprng->buf), 0, cprng->nonce,
	    cprng->key, 8);
	memcpy(p, cprng->buf, n);
	cprng->i = n;
}

/* Public API */

static void
cprng_fast_buf_short(void *buf, size_t len)
{
	struct cprng_fast *cprng;
	int s;

	KASSERT(len <= sizeof(cprng->buf));

	s = cprng_fast_get(&cprng);
	cprng_fast_buf(cprng, buf, len);
	cprng_fast_put(cprng, s);
}

static void
cprng_fast_buf_long(void *buf, size_t len)
{
	uint8_t seed[CHACHA_STREAM_KEYBYTES];
	uint8_t nonce[CHACHA_STREAM_NONCEBYTES] = {0};

	CTASSERT(sizeof(seed) <= sizeof(((struct cprng_fast *)0)->buf));

#if SIZE_MAX >= 0x3fffffffff
	/* >=256 GB is not reasonable */
	KASSERT(len <= 0x3fffffffff);
#endif

	cprng_fast_buf_short(seed, sizeof seed);
	chacha_stream(buf, len, 0, nonce, seed, 8);

	(void)explicit_memset(seed, 0, sizeof seed);
}

uint32_t
cprng_fast32(void)
{
	uint32_t v;

	cprng_fast_buf_short(&v, sizeof v);

	return v;
}

uint64_t
cprng_fast64(void)
{
	uint64_t v;

	cprng_fast_buf_short(&v, sizeof v);

	return v;
}

size_t
cprng_fast(void *buf, size_t len)
{

	/*
	 * We don't want to hog the CPU, so we use the short version,
	 * to generate output without preemption, only if we can do it
	 * with at most one ChaCha call.
	 */
	if (len <= sizeof(((struct cprng_fast *)0)->buf))
		cprng_fast_buf_short(buf, len);
	else
		cprng_fast_buf_long(buf, len);

	return len;		/* hysterical raisins */
}
