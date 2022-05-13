/*	$NetBSD: subr_cprng.c,v 1.43 2022/05/13 09:40:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

/*
 * cprng_strong
 *
 *	Per-CPU NIST Hash_DRBG, reseeded automatically from the entropy
 *	pool when we transition to full entropy, never blocking.  This
 *	is slightly different from the old cprng_strong API, but the
 *	only users of the old one fell into three categories:
 *
 *	1. never-blocking, oughta-be-per-CPU (kern_cprng, sysctl_prng)
 *	2. never-blocking, used per-CPU anyway (/dev/urandom short reads)
 *	3. /dev/random
 *
 *	This code serves the first two categories without having extra
 *	logic for /dev/random.
 *
 *	kern_cprng - available at IPL_SOFTSERIAL or lower
 *	user_cprng - available only at IPL_NONE in thread context
 *
 *	The name kern_cprng is for hysterical raisins.  The name
 *	user_cprng serves only to contrast with kern_cprng.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_cprng.c,v 1.43 2022/05/13 09:40:25 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/entropy.h>
#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/percpu.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <crypto/nist_hash_drbg/nist_hash_drbg.h>

/*
 * struct cprng_strong
 */
struct cprng_strong {
	struct percpu		*cs_percpu; /* struct cprng_cpu */
	ipl_cookie_t		cs_iplcookie;
};

/*
 * struct cprng_cpu
 *
 *	Per-CPU state for a cprng_strong.  The DRBG and evcnt are
 *	allocated separately because percpu(9) sometimes moves per-CPU
 *	objects around without zeroing them.
 */
struct cprng_cpu {
	struct nist_hash_drbg	*cc_drbg;
	struct {
		struct evcnt	reseed;
	}			*cc_evcnt;
	unsigned		cc_epoch;
};

static int	sysctl_kern_urandom(SYSCTLFN_ARGS);
static int	sysctl_kern_arandom(SYSCTLFN_ARGS);
static void	cprng_init_cpu(void *, void *, struct cpu_info *);
static void	cprng_fini_cpu(void *, void *, struct cpu_info *);

/* Well-known CPRNG instances */
struct cprng_strong *kern_cprng __read_mostly; /* IPL_SOFTSERIAL */
struct cprng_strong *user_cprng __read_mostly; /* IPL_NONE */

static struct sysctllog *cprng_sysctllog __read_mostly;

void
cprng_init(void)
{

	if (__predict_false(nist_hash_drbg_initialize() != 0))
		panic("NIST Hash_DRBG failed self-test");

	/*
	 * Create CPRNG instances at two IPLs: IPL_SOFTSERIAL for
	 * kernel use that may occur inside soft interrupt handlers,
	 * and IPL_NONE for userland use which need not block
	 * interrupts.
	 */
	kern_cprng = cprng_strong_create("kern", IPL_SOFTSERIAL, 0);
	user_cprng = cprng_strong_create("user", IPL_NONE, 0);

	/* Create kern.urandom and kern.arandom sysctl nodes.  */
	sysctl_createv(&cprng_sysctllog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_INT, "urandom",
	    SYSCTL_DESCR("Independent uniform random 32-bit integer"),
	    sysctl_kern_urandom, 0, NULL, 0, CTL_KERN, KERN_URND, CTL_EOL);
	sysctl_createv(&cprng_sysctllog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_INT /*lie*/, "arandom",
	    SYSCTL_DESCR("Independent uniform random bytes, up to 256 bytes"),
	    sysctl_kern_arandom, 0, NULL, 0, CTL_KERN, KERN_ARND, CTL_EOL);
}

/*
 * sysctl kern.urandom
 *
 *	Independent uniform random 32-bit integer.  Read-only.
 */
static int
sysctl_kern_urandom(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int v;
	int error;

	/* Generate an int's worth of data.  */
	cprng_strong(user_cprng, &v, sizeof v, 0);

	/* Do the sysctl dance.  */
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/* Clear the buffer before returning the sysctl error.  */
	explicit_memset(&v, 0, sizeof v);
	return error;
}

/*
 * sysctl kern.arandom
 *
 *	Independent uniform random bytes, up to 256 bytes.  Read-only.
 */
static int
sysctl_kern_arandom(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	uint8_t buf[256];
	int error;

	/*
	 * Clamp to a reasonably small size.  256 bytes is kind of
	 * arbitrary; 32 would be more reasonable, but we used 256 in
	 * the past, so let's not break compatibility.
	 */
	if (*oldlenp > 256)	/* size_t, so never negative */
		*oldlenp = 256;

	/* Generate data.  */
	cprng_strong(user_cprng, buf, *oldlenp, 0);

	/* Do the sysctl dance.  */
	node.sysctl_data = buf;
	node.sysctl_size = *oldlenp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/* Clear the buffer before returning the sysctl error.  */
	explicit_memset(buf, 0, sizeof buf);
	return error;
}

struct cprng_strong *
cprng_strong_create(const char *name, int ipl, int flags)
{
	struct cprng_strong *cprng;

	cprng = kmem_alloc(sizeof(*cprng), KM_SLEEP);
	cprng->cs_iplcookie = makeiplcookie(ipl);
	cprng->cs_percpu = percpu_create(sizeof(struct cprng_cpu),
	    cprng_init_cpu, cprng_fini_cpu, __UNCONST(name));

	return cprng;
}

void
cprng_strong_destroy(struct cprng_strong *cprng)
{

	percpu_free(cprng->cs_percpu, sizeof(struct cprng_cpu));
	kmem_free(cprng, sizeof(*cprng));
}

static void
cprng_init_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct cprng_cpu *cc = ptr;
	const char *name = cookie;
	const char *cpuname;
	uint8_t zero[NIST_HASH_DRBG_SEEDLEN_BYTES] = {0};
	char namebuf[64];	/* XXX size? */

	/*
	 * Format the name as, e.g., kern/8 if we're on cpu8.  This
	 * doesn't get displayed anywhere; it just ensures that if
	 * there were a bug causing us to use the same otherwise secure
	 * seed on multiple CPUs, we would still get independent output
	 * from the NIST Hash_DRBG.
	 */
	snprintf(namebuf, sizeof namebuf, "%s/%u", name, cpu_index(ci));

	/*
	 * Allocate the struct nist_hash_drbg and struct evcnt
	 * separately, since percpu(9) may move objects around in
	 * memory without zeroing.
	 */
	cc->cc_drbg = kmem_zalloc(sizeof(*cc->cc_drbg), KM_SLEEP);
	cc->cc_evcnt = kmem_alloc(sizeof(*cc->cc_evcnt), KM_SLEEP);

	/*
	 * Initialize the DRBG with no seed.  We do this in order to
	 * defer reading from the entropy pool as long as possible.
	 */
	if (__predict_false(nist_hash_drbg_instantiate(cc->cc_drbg,
		    zero, sizeof zero, NULL, 0, namebuf, strlen(namebuf))))
		panic("nist_hash_drbg_instantiate");

	/* Attach the event counters.  */
	/* XXX ci_cpuname may not be initialized early enough.  */
	cpuname = ci->ci_cpuname[0] == '\0' ? "cpu0" : ci->ci_cpuname;
	evcnt_attach_dynamic(&cc->cc_evcnt->reseed, EVCNT_TYPE_MISC, NULL,
	    cpuname, "cprng_strong reseed");

	/* Set the epoch uninitialized so we reseed on first use.  */
	cc->cc_epoch = 0;
}

static void
cprng_fini_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct cprng_cpu *cc = ptr;

	evcnt_detach(&cc->cc_evcnt->reseed);
	if (__predict_false(nist_hash_drbg_destroy(cc->cc_drbg)))
		panic("nist_hash_drbg_destroy");

	kmem_free(cc->cc_evcnt, sizeof(*cc->cc_evcnt));
	kmem_free(cc->cc_drbg, sizeof(*cc->cc_drbg));
}

size_t
cprng_strong(struct cprng_strong *cprng, void *buf, size_t len, int flags)
{
	uint8_t seed[NIST_HASH_DRBG_SEEDLEN_BYTES];
	struct cprng_cpu *cc;
	unsigned epoch;
	int s;

	/* Not allowed in hard interrupt context.  */
	KASSERT(!cpu_intr_p());

	/*
	 * Verify maximum request length.  Caller should really limit
	 * their requests to 32 bytes to avoid spending much time with
	 * preemption disabled -- use the 32 bytes to seed a private
	 * DRBG instance if you need more data.
	 */
	KASSERT(len <= CPRNG_MAX_LEN);

	/* Verify legacy API use.  */
	KASSERT(flags == 0);

	/* Acquire per-CPU state and block interrupts.  */
	cc = percpu_getref(cprng->cs_percpu);
	s = splraiseipl(cprng->cs_iplcookie);

	/* If the entropy epoch has changed, (re)seed.  */
	epoch = entropy_epoch();
	if (__predict_false(epoch != cc->cc_epoch)) {
		entropy_extract(seed, sizeof seed, 0);
		cc->cc_evcnt->reseed.ev_count++;
		if (__predict_false(nist_hash_drbg_reseed(cc->cc_drbg,
			    seed, sizeof seed, NULL, 0)))
			panic("nist_hash_drbg_reseed");
		explicit_memset(seed, 0, sizeof seed);
		cc->cc_epoch = epoch;
	}

	/* Generate data.  Failure here means it's time to reseed.  */
	if (__predict_false(nist_hash_drbg_generate(cc->cc_drbg, buf, len,
		    NULL, 0))) {
		entropy_extract(seed, sizeof seed, 0);
		cc->cc_evcnt->reseed.ev_count++;
		if (__predict_false(nist_hash_drbg_reseed(cc->cc_drbg,
			    seed, sizeof seed, NULL, 0)))
			panic("nist_hash_drbg_reseed");
		explicit_memset(seed, 0, sizeof seed);
		if (__predict_false(nist_hash_drbg_generate(cc->cc_drbg,
			    buf, len, NULL, 0)))
			panic("nist_hash_drbg_generate");
	}

	/* Release state and interrupts.  */
	splx(s);
	percpu_putref(cprng->cs_percpu);

	/* Return the number of bytes generated, for hysterical raisins.  */
	return len;
}

uint32_t
cprng_strong32(void)
{
	uint32_t r;
	cprng_strong(kern_cprng, &r, sizeof(r), 0);
	return r;
}

uint64_t
cprng_strong64(void)
{
	uint64_t r;
	cprng_strong(kern_cprng, &r, sizeof(r), 0);
	return r;
}
