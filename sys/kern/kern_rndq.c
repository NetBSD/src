/*	$NetBSD: kern_rndq.c,v 1.1.2.5 2013/02/08 20:28:07 riz Exp $	*/

/*-
 * Copyright (c) 1997-2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org> and Thor Lancelot Simon.
 * This code uses ideas and algorithms from the Linux driver written by
 * Ted Ts'o.
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
__KERNEL_RCSID(0, "$NetBSD: kern_rndq.c,v 1.1.2.5 2013/02/08 20:28:07 riz Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/rnd.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/once.h>
#include <sys/rngtest.h>
#include <sys/cpu.h>	/* XXX temporary, see rnd_detach_source */

#include <dev/rnd_private.h>

#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL) /* XXX: bad pooka */
#include <machine/cpu_counter.h>
#endif

#ifdef RND_DEBUG
#define	DPRINTF(l,x)      if (rnd_debug & (l)) printf x
int	rnd_debug = 0;
#else
#define	DPRINTF(l,x)
#endif

#define	RND_DEBUG_WRITE		0x0001
#define	RND_DEBUG_READ		0x0002
#define	RND_DEBUG_IOCTL		0x0004
#define	RND_DEBUG_SNOOZE	0x0008

/*
 * list devices attached
 */
#if 0
#define	RND_VERBOSE
#endif

/*
 * The size of a temporary buffer, kmem_alloc()ed when needed, and used for
 * reading and writing data.
 */
#define	RND_TEMP_BUFFER_SIZE	128

/*
 * This is a little bit of state information attached to each device that we
 * collect entropy from.  This is simply a collection buffer, and when it
 * is full it will be "detached" from the source and added to the entropy
 * pool after entropy is distilled as much as possible.
 */
#define	RND_SAMPLE_COUNT	64	/* collect N samples, then compress */
typedef struct _rnd_sample_t {
	SIMPLEQ_ENTRY(_rnd_sample_t) next;
	krndsource_t	*source;
	int		cursor;
	int		entropy;
	u_int32_t	ts[RND_SAMPLE_COUNT];
	u_int32_t	values[RND_SAMPLE_COUNT];
} rnd_sample_t;

/*
 * The event queue.  Fields are altered at an interrupt level.
 * All accesses must be protected with the mutex.
 */
volatile int			rnd_timeout_pending;
SIMPLEQ_HEAD(, _rnd_sample_t)	rnd_samples;
kmutex_t			rnd_mtx;


/*
 * Entropy sinks: usually other generators waiting to be rekeyed.
 *
 * A sink's callback MUST NOT re-add the sink to the list, or
 * list corruption will occur.  The list is protected by the
 * rndsink_mtx, which must be released before calling any sink's
 * callback.
 */
TAILQ_HEAD(, rndsink)		rnd_sinks;
kmutex_t			rndsink_mtx;

/*
 * Memory pool for sample buffers
 */
static pool_cache_t rnd_mempc;

/*
 * Our random pool.  This is defined here rather than using the general
 * purpose one defined in rndpool.c.
 *
 * Samples are collected and queued into a separate mutex-protected queue
 * (rnd_samples, see above), and processed in a timeout routine; therefore,
 * the mutex protecting the random pool is at IPL_SOFTCLOCK() as well.
 */
rndpool_t rnd_pool;
kmutex_t  rndpool_mtx;
kcondvar_t rndpool_cv;

/*
 * This source is used to easily "remove" queue entries when the source
 * which actually generated the events is going away.
 */
static krndsource_t rnd_source_no_collect = {
	/* LIST_ENTRY list */
	.name = { 'N', 'o', 'C', 'o', 'l', 'l', 'e', 'c', 't',
		   0, 0, 0, 0, 0, 0, 0 },
	.last_time = 0, .last_delta = 0, .last_delta2 = 0, .total = 0,
	.type = RND_TYPE_UNKNOWN,
	.flags = (RND_FLAG_NO_COLLECT |
		  RND_FLAG_NO_ESTIMATE |
		  RND_TYPE_UNKNOWN),
	.state = NULL,
	.test_cnt = 0,
	.test = NULL
};

struct callout rnd_callout, skew_callout;

void	      rnd_wakeup_readers(void);
static inline u_int32_t rnd_estimate_entropy(krndsource_t *, u_int32_t);
static inline u_int32_t rnd_counter(void);
static        void	rnd_timeout(void *);
static	      void	rnd_process_events(void *);
u_int32_t     rnd_extract_data_locked(void *, u_int32_t, u_int32_t); /* XXX */
static	      void	rnd_add_data_ts(krndsource_t *, const void *const,
					uint32_t, uint32_t, uint32_t);

int			rnd_ready = 0;
int			rnd_initial_entropy = 0;

#ifdef DIAGNOSTIC
static int		rnd_tested = 0;
static rngtest_t	rnd_rt;
static uint8_t		rnd_testbits[sizeof(rnd_rt.rt_b)];
#endif

LIST_HEAD(, krndsource)	rnd_sources;

rndsave_t		*boot_rsp;

/*
 * Generate a 32-bit counter.  This should be more machine dependent,
 * using cycle counters and the like when possible.
 */
static inline u_int32_t
rnd_counter(void)
{
	struct timeval tv;

#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL) /* XXX: bad pooka */
	if (cpu_hascounter())
		return (cpu_counter32());
#endif
	if (rnd_ready) {
		microtime(&tv);
		return (tv.tv_sec * 1000000 + tv.tv_usec);
	}
	/* when called from rnd_init, its too early to call microtime safely */
	return (0);
}

/*
 * Check to see if there are readers waiting on us.  If so, kick them.
 */
void
rnd_wakeup_readers(void)
{
	rndsink_t *sink, *tsink;
	TAILQ_HEAD(, rndsink) sunk = TAILQ_HEAD_INITIALIZER(sunk);

	mutex_spin_enter(&rndpool_mtx);
	if (rndpool_get_entropy_count(&rnd_pool) < RND_ENTROPY_THRESHOLD * 8) {
		mutex_spin_exit(&rndpool_mtx);
		return;
	}

	/*
	 * First, take care of in-kernel consumers needing rekeying.
	 */
	mutex_spin_enter(&rndsink_mtx);
	TAILQ_FOREACH_SAFE(sink, &rnd_sinks, tailq, tsink) {
		if (!mutex_tryenter(&sink->mtx)) {
#ifdef RND_VERBOSE
			printf("rnd_wakeup_readers: "
			       "skipping busy rndsink\n");
#endif
			continue;
		}

		KASSERT(RSTATE_PENDING == sink->state);
		
		if ((sink->len + RND_ENTROPY_THRESHOLD) * 8 <
			rndpool_get_entropy_count(&rnd_pool)) {
			/* We have enough entropy to sink some here. */
			if (rndpool_extract_data(&rnd_pool, sink->data,
						 sink->len, RND_EXTRACT_GOOD)
			    != sink->len) {
				panic("could not extract estimated "
				      "entropy from pool");
			}
			sink->state = RSTATE_HASBITS;
			/* Move this sink to the list of pending callbacks */
			TAILQ_REMOVE(&rnd_sinks, sink, tailq);
			TAILQ_INSERT_HEAD(&sunk, sink, tailq);
		} else {
			mutex_exit(&sink->mtx);
		}
	}
	mutex_spin_exit(&rndsink_mtx);
		
	/*
	 * If we still have enough new bits to do something, feed userspace.
	 */
	if (rndpool_get_entropy_count(&rnd_pool) > RND_ENTROPY_THRESHOLD * 8) {
#ifdef RND_VERBOSE
		if (!rnd_initial_entropy)
			printf("rnd: have initial entropy (%u)\n",
			       rndpool_get_entropy_count(&rnd_pool));
#endif
		rnd_initial_entropy = 1;
		mutex_spin_exit(&rndpool_mtx);
	} else {
		mutex_spin_exit(&rndpool_mtx);
	}

	/*
	 * Now that we have dropped the mutex, we can run sinks' callbacks.
	 * Since we have reused the "tailq" member of the sink structure for
	 * this temporary on-stack queue, the callback must NEVER re-add
	 * the sink to the main queue, or our on-stack queue will become
	 * corrupt.
	 */
	while ((sink = TAILQ_FIRST(&sunk))) {
#ifdef RND_VERBOSE
		printf("supplying %d bytes to entropy sink \"%s\""
		       " (cb %p, arg %p).\n",
		       (int)sink->len, sink->name, sink->cb, sink->arg);
#endif
		sink->state = RSTATE_HASBITS;
		sink->cb(sink->arg);
		TAILQ_REMOVE(&sunk, sink, tailq);
		mutex_spin_exit(&sink->mtx);
	}
}

/*
 * Use the timing of the event to estimate the entropy gathered.
 * If all the differentials (first, second, and third) are non-zero, return
 * non-zero.  If any of these are zero, return zero.
 */
static inline u_int32_t
rnd_estimate_entropy(krndsource_t *rs, u_int32_t t)
{
	int32_t delta, delta2, delta3;

	/*
	 * If the time counter has overflowed, calculate the real difference.
	 * If it has not, it is simplier.
	 */
	if (t < rs->last_time)
		delta = UINT_MAX - rs->last_time + t;
	else
		delta = rs->last_time - t;

	if (delta < 0)
		delta = -delta;

	/*
	 * Calculate the second and third order differentials
	 */
	delta2 = rs->last_delta - delta;
	if (delta2 < 0)
		delta2 = -delta2;

	delta3 = rs->last_delta2 - delta2;
	if (delta3 < 0)
		delta3 = -delta3;

	rs->last_time = t;
	rs->last_delta = delta;
	rs->last_delta2 = delta2;

	/*
	 * If any delta is 0, we got no entropy.  If all are non-zero, we
	 * might have something.
	 */
	if (delta == 0 || delta2 == 0 || delta3 == 0)
		return (0);

	return (1);
}

#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL)
static void
rnd_skew(void *arg)
{
	static krndsource_t skewsrc;
	static int live, flipflop;

	/*
	 * Only one instance of this callout will ever be scheduled
	 * at a time (it is only ever scheduled by itself).  So no
	 * locking is required here.
	 */

	/*
	 * Even on systems with seemingly stable clocks, the
	 * entropy estimator seems to think we get 1 bit here
	 * about every 2 calls.  That seems like too much.  Set
	 * NO_ESTIMATE on this source until we can better analyze
	 * the entropy of its output.
	 */
	if (__predict_false(!live)) {
		rnd_attach_source(&skewsrc, "callout", RND_TYPE_SKEW,
				  RND_FLAG_NO_ESTIMATE);
		live = 1;
	}

	flipflop = !flipflop;

	if (flipflop) {
		rnd_add_uint32(&skewsrc, rnd_counter());
		callout_schedule(&skew_callout, hz);
	} else {
		callout_schedule(&skew_callout, 1);
	}
}
#endif

/*
 * initialize the global random pool for our use.
 * rnd_init() must be called very early on in the boot process, so
 * the pool is ready for other devices to attach as sources.
 */
void
rnd_init(void)
{
	u_int32_t c;

	if (rnd_ready)
		return;

	mutex_init(&rnd_mtx, MUTEX_DEFAULT, IPL_VM);
	mutex_init(&rndsink_mtx, MUTEX_DEFAULT, IPL_VM);

	callout_init(&rnd_callout, CALLOUT_MPSAFE);
	callout_setfunc(&rnd_callout, rnd_timeout, NULL);

	/*
	 * take a counter early, hoping that there's some variance in
	 * the following operations
	 */
	c = rnd_counter();

	LIST_INIT(&rnd_sources);
	SIMPLEQ_INIT(&rnd_samples);
	TAILQ_INIT(&rnd_sinks);

	rndpool_init(&rnd_pool);
	mutex_init(&rndpool_mtx, MUTEX_DEFAULT, IPL_VM);
	cv_init(&rndpool_cv, "rndread");

	rnd_mempc = pool_cache_init(sizeof(rnd_sample_t), 0, 0, 0,
				    "rndsample", NULL, IPL_VM,
				    NULL, NULL, NULL);

	/*
	 * Set resource limit. The rnd_process_events() function
	 * is called every tick and process the sample queue.
	 * Without limitation, if a lot of rnd_add_*() are called,
	 * all kernel memory may be eaten up.
	 */
	pool_cache_sethardlimit(rnd_mempc, RND_POOLBITS, NULL, 0);

	/*
	 * Mix *something*, *anything* into the pool to help it get started.
	 * However, it's not safe for rnd_counter() to call microtime() yet,
	 * so on some platforms we might just end up with zeros anyway.
	 * XXX more things to add would be nice.
	 */
	if (c) {
		mutex_spin_enter(&rndpool_mtx);
		rndpool_add_data(&rnd_pool, &c, sizeof(c), 1);
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(c), 1);
		mutex_spin_exit(&rndpool_mtx);
	}

	rnd_ready = 1;

	/*
	 * If we have a cycle counter, take its error with respect
	 * to the callout mechanism as a source of entropy, ala
	 * TrueRand.
 	 *
	 * XXX This will do little when the cycle counter *is* what's
	 * XXX clocking the callout mechanism.  How to get this right
	 * XXX without unsightly spelunking in the timecounter code?
	 */
#if defined(__HAVE_CPU_COUNTER) && !defined(_RUMPKERNEL) /* XXX: bad pooka */
	callout_init(&skew_callout, CALLOUT_MPSAFE);
	callout_setfunc(&skew_callout, rnd_skew, NULL);
	rnd_skew(NULL);
#endif

#ifdef RND_VERBOSE
	printf("rnd: initialised (%u)%s", RND_POOLBITS,
	       c ? " with counter\n" : "\n");
#endif
	if (boot_rsp != NULL) {
		mutex_spin_enter(&rndpool_mtx);
			rndpool_add_data(&rnd_pool, boot_rsp->data,
					 sizeof(boot_rsp->data),
					 MIN(boot_rsp->entropy,
					     RND_POOLBITS / 2));
		if (rndpool_get_entropy_count(&rnd_pool) >
		    RND_ENTROPY_THRESHOLD * 8) {
                	rnd_initial_entropy = 1;
		}
                mutex_spin_exit(&rndpool_mtx);
#ifdef RND_VERBOSE
		printf("rnd: seeded with %d bits\n",
		       MIN(boot_rsp->entropy, RND_POOLBITS / 2));
#endif
		memset(boot_rsp, 0, sizeof(*boot_rsp));
	}
}

static rnd_sample_t *
rnd_sample_allocate(krndsource_t *source)
{
	rnd_sample_t *c;

	c = pool_cache_get(rnd_mempc, PR_WAITOK);
	if (c == NULL)
		return (NULL);

	c->source = source;
	c->cursor = 0;
	c->entropy = 0;

	return (c);
}

/*
 * Don't wait on allocation.  To be used in an interrupt context.
 */
static rnd_sample_t *
rnd_sample_allocate_isr(krndsource_t *source)
{
	rnd_sample_t *c;

	c = pool_cache_get(rnd_mempc, PR_NOWAIT);
	if (c == NULL)
		return (NULL);

	c->source = source;
	c->cursor = 0;
	c->entropy = 0;

	return (c);
}

static void
rnd_sample_free(rnd_sample_t *c)
{
	memset(c, 0, sizeof(*c));
	pool_cache_put(rnd_mempc, c);
}

/*
 * Add a source to our list of sources.
 */
void
rnd_attach_source(krndsource_t *rs, const char *name, u_int32_t type,
    u_int32_t flags)
{
	u_int32_t ts;

	ts = rnd_counter();

	strlcpy(rs->name, name, sizeof(rs->name));
	rs->last_time = ts;
	rs->last_delta = 0;
	rs->last_delta2 = 0;
	rs->total = 0;

	/*
	 * Force network devices to not collect any entropy by
	 * default.
	 */
	if (type == RND_TYPE_NET)
		flags |= (RND_FLAG_NO_COLLECT | RND_FLAG_NO_ESTIMATE);

	/*
 	 * Hardware RNGs get extra space for statistical testing.
	 */
	if (type == RND_TYPE_RNG) {
		rs->test = kmem_alloc(sizeof(rngtest_t), KM_NOSLEEP);
		rs->test_cnt = 0;
	} else {
		rs->test = NULL;
		rs->test_cnt = -1;
	}

	rs->type = type;
	rs->flags = flags;

	rs->state = rnd_sample_allocate(rs);

	mutex_spin_enter(&rndpool_mtx);
	LIST_INSERT_HEAD(&rnd_sources, rs, list);

#ifdef RND_VERBOSE
	printf("rnd: %s attached as an entropy source (", rs->name);
	if (!(flags & RND_FLAG_NO_COLLECT)) {
		printf("collecting");
		if (flags & RND_FLAG_NO_ESTIMATE)
			printf(" without estimation");
	}
	else
		printf("off");
	printf(")\n");
#endif

	/*
	 * Again, put some more initial junk in the pool.
	 * XXX Bogus, but harder to guess than zeros.
	 */
	rndpool_add_data(&rnd_pool, &ts, sizeof(u_int32_t), 1);
	mutex_spin_exit(&rndpool_mtx);
}

/*
 * Remove a source from our list of sources.
 */
void
rnd_detach_source(krndsource_t *source)
{
	rnd_sample_t *sample;

	mutex_spin_enter(&rnd_mtx);

	LIST_REMOVE(source, list);

	/*
	 * If there are samples queued up "remove" them from the sample queue
	 * by setting the source to the no-collect pseudosource.
	 */
	sample = SIMPLEQ_FIRST(&rnd_samples);
	while (sample != NULL) {
		if (sample->source == source)
			sample->source = &rnd_source_no_collect;

		sample = SIMPLEQ_NEXT(sample, next);
	}

	mutex_spin_exit(&rnd_mtx);

	if (!cpu_softintr_p()) {	/* XXX XXX very temporary "fix" */
		if (source->state) {
			rnd_sample_free(source->state);
			source->state = NULL;
		}

		if (source->test) {
			kmem_free(source->test, sizeof(rngtest_t));
		}
	}

#ifdef RND_VERBOSE
	printf("rnd: %s detached as an entropy source\n", source->name);
#endif
}

/*
 * Add a 32-bit value to the entropy pool.  The rs parameter should point to
 * the source-specific source structure.
 */
void
_rnd_add_uint32(krndsource_t *rs, u_int32_t val)
{
	u_int32_t ts;
	u_int32_t entropy = 0;

	if (rs->flags & RND_FLAG_NO_COLLECT)
		return;

	/*
	 * Sample the counter as soon as possible to avoid
	 * entropy overestimation.
	 */
	ts = rnd_counter();

	/*
	 * If we are estimating entropy on this source,
	 * calculate differentials.
	 */

	if ((rs->flags & RND_FLAG_NO_ESTIMATE) == 0) {
		entropy = rnd_estimate_entropy(rs, ts);
	}

	rnd_add_data_ts(rs, &val, sizeof(val), entropy, ts);
}

void
rnd_add_data(krndsource_t *rs, const void *const data, uint32_t len,
	     uint32_t entropy)
{
	/*
	 * This interface is meant for feeding data which is,
	 * itself, random.  Don't estimate entropy based on
	 * timestamp, just directly add the data.
	 */
	rnd_add_data_ts(rs, data, len, entropy, rnd_counter());
}

static void
rnd_add_data_ts(krndsource_t *rs, const void *const data, u_int32_t len,
		u_int32_t entropy, uint32_t ts)
{
	rnd_sample_t *state = NULL;
	const uint32_t *dint = data;
	int todo, done, filled = 0;
	SIMPLEQ_HEAD(, _rnd_sample_t) tmp_samples =
	    		SIMPLEQ_HEAD_INITIALIZER(tmp_samples);

	if (rs->flags & RND_FLAG_NO_COLLECT) {
		return;
	}

	/*
	 * Loop over data packaging it into sample buffers.
	 * If a sample buffer allocation fails, drop all data.
	 */
	todo = len / sizeof(*dint);
	for (done = 0; done < todo ; done++) {
		state = rs->state;
		if (state == NULL) {
			state = rnd_sample_allocate_isr(rs);
			if (__predict_false(state == NULL)) {
				break;
			}
			rs->state = state;
		}

		state->ts[state->cursor] = ts;
		state->values[state->cursor] = dint[done];
		state->cursor++;

		if (state->cursor == RND_SAMPLE_COUNT) {
			SIMPLEQ_INSERT_HEAD(&tmp_samples, state, next);
			filled++;
			rs->state = NULL;
		}
	}

	if (__predict_false(state == NULL)) {
		while ((state = SIMPLEQ_FIRST(&tmp_samples))) {
			SIMPLEQ_REMOVE_HEAD(&tmp_samples, next);
			rnd_sample_free(state);
		}
		return;
	}

	/*
	 * Claim all the entropy on the last one we send to
	 * the pool, so we don't rely on it being evenly distributed
	 * in the supplied data.
	 *
	 * XXX The rndpool code must accept samples with more
	 * XXX claimed entropy than bits for this to work right.
	 */
	state->entropy += entropy;
	rs->total += entropy;

	/*
	 * If we didn't finish any sample buffers, we're done.
	 */
	if (!filled) {
		return;
	}

	mutex_spin_enter(&rnd_mtx);
	while ((state = SIMPLEQ_FIRST(&tmp_samples))) {
		SIMPLEQ_REMOVE_HEAD(&tmp_samples, next);
		SIMPLEQ_INSERT_HEAD(&rnd_samples, state, next);
	}

	/*
	 * If we are still starting up, cause immediate processing of
	 * the queued samples.  Otherwise, if the timeout isn't
	 * pending, have it run in the near future.
	 */
	if (__predict_false(cold)) {
#ifdef RND_VERBOSE
		printf("rnd: directly processing boot-time events.\n");
#endif
		rnd_process_events(NULL);	/* Drops lock! */
		return;
	}
	if (rnd_timeout_pending == 0) {
		rnd_timeout_pending = 1;
		mutex_spin_exit(&rnd_mtx);
		callout_schedule(&rnd_callout, 1);
		return;
	}
	mutex_spin_exit(&rnd_mtx);
}

static int
rnd_hwrng_test(rnd_sample_t *sample)
{
	krndsource_t *source = sample->source;
	size_t cmplen;
	uint8_t *v1, *v2;
	size_t resid, totest;

	KASSERT(source->type = RND_TYPE_RNG);

	/*
	 * Continuous-output test: compare two halves of the
	 * sample buffer to each other.  The sample buffer (64 ints,
	 * so either 256 or 512 bytes on any modern machine) should be
	 * much larger than a typical hardware RNG output, so this seems
	 * a reasonable way to do it without retaining extra data.
	 */
	cmplen = sizeof(sample->values) / 2;
	v1 = (uint8_t *)sample->values;
	v2 = (uint8_t *)sample->values + cmplen;

	if (__predict_false(!memcmp(v1, v2, cmplen))) {
		printf("rnd: source \"%s\" failed continuous-output test.\n",
		       source->name);
		return 1;
	}

	/*
	 * FIPS 140 statistical RNG test.  We must accumulate 20,000 bits.
	 */
	if (__predict_true(source->test_cnt == -1)) {
		/* already passed the test */
		return 0;
	}
	resid = FIPS140_RNG_TEST_BYTES - source->test_cnt;
	totest = MIN(RND_SAMPLE_COUNT * 4, resid);
	memcpy(source->test->rt_b + source->test_cnt, sample->values, totest);
	resid -= totest;
	source->test_cnt += totest;
	if (resid == 0) {
		strlcpy(source->test->rt_name, source->name,
			sizeof(source->test->rt_name));
		if (rngtest(source->test)) {
			printf("rnd: source \"%s\" failed statistical test.",
			       source->name);
			return 1;
		}
		source->test_cnt = -1;
		memset(source->test, 0, sizeof(*source->test));
	}
	return 0;
}

/*
 * Process the events in the ring buffer.  Called by rnd_timeout or
 * by the add routines directly if the callout has never fired (that
 * is, if we are "cold" -- just booted).
 *
 * Call with rnd_mtx held -- WILL RELEASE IT.
 */
static void
rnd_process_events(void *arg)
{
	rnd_sample_t *sample;
	krndsource_t *source, *badsource = NULL;
	u_int32_t entropy;
	SIMPLEQ_HEAD(, _rnd_sample_t) dq_samples =
			SIMPLEQ_HEAD_INITIALIZER(dq_samples);
	SIMPLEQ_HEAD(, _rnd_sample_t) df_samples =
			SIMPLEQ_HEAD_INITIALIZER(df_samples);
        TAILQ_HEAD(, rndsink) sunk = TAILQ_HEAD_INITIALIZER(sunk);

	/*
	 * Sample queue is protected by rnd_mtx, drain to onstack queue
	 * and drop lock.
	 */

	while ((sample = SIMPLEQ_FIRST(&rnd_samples))) {
		SIMPLEQ_REMOVE_HEAD(&rnd_samples, next);
		/*
		 * We repeat this check here, since it is possible
		 * the source was disabled before we were called, but
		 * after the entry was queued.
		 */
		if (__predict_false(sample->source->flags
				    & RND_FLAG_NO_COLLECT)) {
			SIMPLEQ_INSERT_TAIL(&df_samples, sample, next);
		} else {
			SIMPLEQ_INSERT_TAIL(&dq_samples, sample, next);
		}
	}
	mutex_spin_exit(&rnd_mtx);

	/* Don't thrash the rndpool mtx either.  Hold, add all samples. */
	mutex_spin_enter(&rndpool_mtx);
	while ((sample = SIMPLEQ_FIRST(&dq_samples))) {
		SIMPLEQ_REMOVE_HEAD(&dq_samples, next);
		source = sample->source;
		entropy = sample->entropy;

		/*
		 * Hardware generators are great but sometimes they
		 * have...hardware issues.  Don't use any data from
		 * them unless it passes some tests.
		 */
		if (source->type == RND_TYPE_RNG) {
			if (__predict_false(rnd_hwrng_test(sample))) {
				/*
				 * Detach the bad source.  See below.
				 */
				badsource = source;
				printf("rnd: detaching source \"%s\".",
				       badsource->name);
				break;
			}
		}
		rndpool_add_data(&rnd_pool, sample->values,
		    RND_SAMPLE_COUNT * 4, 0);

		rndpool_add_data(&rnd_pool, sample->ts,
		    RND_SAMPLE_COUNT * 4, entropy);

		source->total += sample->entropy;
		SIMPLEQ_INSERT_TAIL(&df_samples, sample, next);
	}
	mutex_spin_exit(&rndpool_mtx);

	/* Now we hold no locks: clean up. */
	if (__predict_false(badsource)) {
		/*
		 * The detach routine frees any samples we have not
		 * dequeued ourselves.  For sanity's sake, we simply
		 * free (without using) all dequeued samples from the
		 * point at which we detected a problem onwards.
		 */
		rnd_detach_source(badsource);
		while ((sample = SIMPLEQ_FIRST(&dq_samples))) {
			SIMPLEQ_REMOVE_HEAD(&dq_samples, next);
			rnd_sample_free(sample);
		}
	}
	while ((sample = SIMPLEQ_FIRST(&df_samples))) {
		SIMPLEQ_REMOVE_HEAD(&df_samples, next);
		rnd_sample_free(sample);
	}

	/*
	 * Wake up any potential readers waiting.
	 */
	rnd_wakeup_readers();
}

/*
 * Timeout, run to process the events in the ring buffer.
 */
static void
rnd_timeout(void *arg)
{
        mutex_spin_enter(&rnd_mtx);
        rnd_timeout_pending = 0;
        rnd_process_events(arg);
}

u_int32_t
rnd_extract_data_locked(void *p, u_int32_t len, u_int32_t flags)
{
	static int timed_in;

	KASSERT(mutex_owned(&rndpool_mtx));
	if (__predict_false(!timed_in)) {
		if (boottime.tv_sec) {
			rndpool_add_data(&rnd_pool, &boottime,
					 sizeof(boottime), 0);
		}
		timed_in++;
	}
	if (__predict_false(!rnd_initial_entropy)) {
		u_int32_t c;

#ifdef RND_VERBOSE
		printf("rnd: WARNING! initial entropy low (%u).\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif
		/* Try once again to put something in the pool */
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
	}

#ifdef DIAGNOSTIC
	while (!rnd_tested) {
		int entropy_count;

		entropy_count = rndpool_get_entropy_count(&rnd_pool);
#ifdef RND_VERBOSE
		printf("rnd: starting statistical RNG test, entropy = %d.\n",
			entropy_count);
#endif
		if (rndpool_extract_data(&rnd_pool, rnd_rt.rt_b,
		    sizeof(rnd_rt.rt_b), RND_EXTRACT_ANY)
		    != sizeof(rnd_rt.rt_b)) {
			panic("rnd: could not get bits for statistical test");
		}
		/*
		 * Stash the tested bits so we can put them back in the
		 * pool, restoring the entropy count.  DO NOT rely on
		 * rngtest to maintain the bits pristine -- we could end
		 * up adding back non-random data claiming it were pure
		 * entropy.
		 */
		memcpy(rnd_testbits, rnd_rt.rt_b, sizeof(rnd_rt.rt_b));
		strlcpy(rnd_rt.rt_name, "entropy pool", sizeof(rnd_rt.rt_name));
		if (rngtest(&rnd_rt)) {
			/*
			 * The probabiliity of a Type I error is 3/10000,
			 * but note this can only happen at boot time.
			 * The relevant standard says to reset the module,
			 * but developers objected...
			 */
			printf("rnd: WARNING, ENTROPY POOL FAILED "
			       "STATISTICAL TEST!\n");
			continue;
		}
		memset(&rnd_rt, 0, sizeof(rnd_rt));
		rndpool_add_data(&rnd_pool, rnd_testbits, sizeof(rnd_testbits),
				 entropy_count);
		memset(rnd_testbits, 0, sizeof(rnd_testbits));
#ifdef RND_VERBOSE
		printf("rnd: statistical RNG test done, entropy = %d.\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif
		rnd_tested++;
	}
#endif
	return rndpool_extract_data(&rnd_pool, p, len, flags);
}

u_int32_t
rnd_extract_data(void *p, u_int32_t len, u_int32_t flags)
{
	uint32_t retval;

	mutex_spin_enter(&rndpool_mtx);
	retval = rnd_extract_data_locked(p, len, flags);
	mutex_spin_exit(&rndpool_mtx);
	return retval;
}

void
rndsink_attach(rndsink_t *rs)
{
#ifdef RND_VERBOSE
	printf("rnd: entropy sink \"%s\" wants %d bytes of data.\n",
	       rs->name, (int)rs->len);
#endif

	KASSERT(mutex_owned(&rs->mtx));
	KASSERT(rs->state = RSTATE_PENDING);

	mutex_spin_enter(&rndsink_mtx);
	TAILQ_INSERT_TAIL(&rnd_sinks, rs, tailq);
	mutex_spin_exit(&rndsink_mtx);

	mutex_spin_enter(&rnd_mtx);
	if (rnd_timeout_pending == 0) {
		rnd_timeout_pending = 1;
		callout_schedule(&rnd_callout, 1);
	}
	mutex_spin_exit(&rnd_mtx);

}

void
rndsink_detach(rndsink_t *rs)
{
	rndsink_t *sink, *tsink;
#ifdef RND_VERBOSE
	printf("rnd: entropy sink \"%s\" no longer wants data.\n", rs->name);
#endif
	KASSERT(mutex_owned(&rs->mtx));

	mutex_spin_enter(&rndsink_mtx);
	TAILQ_FOREACH_SAFE(sink, &rnd_sinks, tailq, tsink) {
		if (sink == rs) {
			TAILQ_REMOVE(&rnd_sinks, rs, tailq);
		}
	}
	mutex_spin_exit(&rndsink_mtx);
}

void
rnd_seed(void *base, size_t len)
{
	SHA1_CTX s;
	uint8_t digest[SHA1_DIGEST_LENGTH];

	if (len != sizeof(*boot_rsp)) {
		aprint_error("rnd: bad seed length %d\n", (int)len);
		return;
	}

	boot_rsp = (rndsave_t *)base;
	SHA1Init(&s);
	SHA1Update(&s, (uint8_t *)&boot_rsp->entropy,
		   sizeof(boot_rsp->entropy));
	SHA1Update(&s, boot_rsp->data, sizeof(boot_rsp->data));
	SHA1Final(digest, &s);

	if (memcmp(digest, boot_rsp->digest, sizeof(digest))) {
		aprint_error("rnd: bad seed checksum\n");
		return;
	}

	/*
	 * It's not really well-defined whether bootloader-supplied
	 * modules run before or after rnd_init().  Handle both cases.
	 */
	if (rnd_ready) {
#ifdef RND_VERBOSE
		printf("rnd: ready, feeding in seed data directly.\n");
#endif
		mutex_spin_enter(&rndpool_mtx);
		rndpool_add_data(&rnd_pool, boot_rsp->data,
				 sizeof(boot_rsp->data),
				 MIN(boot_rsp->entropy, RND_POOLBITS / 2));
		memset(boot_rsp, 0, sizeof(*boot_rsp));
		mutex_spin_exit(&rndpool_mtx);
	} else {
#ifdef RND_VERBOSE
		printf("rnd: not ready, deferring seed feed.\n");
#endif
	}
}
