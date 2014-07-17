/*	$NetBSD: kern_rndq.c,v 1.23.2.4 2014/07/17 14:03:33 tls Exp $	*/

/*-
 * Copyright (c) 1997-2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_rndq.c,v 1.23.2.4 2014/07/17 14:03:33 tls Exp $");

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
#include <sys/intr.h>
#include <sys/rnd.h>
#include <sys/rndsink.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/once.h>
#include <sys/rngtest.h>
#include <sys/cpu.h>	/* XXX temporary, see rnd_detach_source */

#include <dev/rnd_private.h>

#if defined(__HAVE_CPU_COUNTER)
#include <machine/cpu_counter.h>
#endif

#ifdef RND_DEBUG
#define	DPRINTF(l,x)      if (rnd_debug & (l)) rnd_printf x
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

#ifdef RND_VERBOSE
static unsigned int deltacnt;
#endif

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
	uint32_t	ts[RND_SAMPLE_COUNT];
	u_int32_t	values[RND_SAMPLE_COUNT];
} rnd_sample_t;

/*
 * The event queue.  Fields are altered at an interrupt level.
 * All accesses must be protected with the mutex.
 */
SIMPLEQ_HEAD(, _rnd_sample_t)	rnd_samples;
kmutex_t			rnd_mtx;

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
	.total = 0,
	.type = RND_TYPE_UNKNOWN,
	.flags = (RND_FLAG_NO_COLLECT |
		  RND_FLAG_NO_ESTIMATE),
	.state = NULL,
	.test_cnt = 0,
	.test = NULL
};

static krndsource_t rnd_source_anonymous = {
	/* LIST_ENTRY list */
	.name = { 'A', 'n', 'o', 'n', 'y', 'm', 'o', 'u', 's',
		  0, 0, 0, 0, 0, 0, 0 },
	.total = 0,
	.type = RND_TYPE_UNKNOWN,
        .flags = (RND_FLAG_COLLECT_TIME|
		  RND_FLAG_COLLECT_VALUE|
		  RND_FLAG_ESTIMATE_TIME),
	.state = NULL,
	.test_cnt = 0,
	.test = NULL
};

krndsource_t rnd_printf_source, rnd_autoconf_source;

void *rnd_process, *rnd_wakeup;
struct callout skew_callout;

void	      		rnd_wakeup_readers(void);
static inline uint32_t	rnd_counter(void);
static        void	rnd_intr(void *);
static	      void	rnd_wake(void *);
static	      void	rnd_process_events(void);
u_int32_t     rnd_extract_data_locked(void *, u_int32_t, u_int32_t); /* XXX */
static	      void	rnd_add_data_ts(krndsource_t *, const void *const,
					uint32_t, uint32_t, uint32_t);
static inline void	rnd_schedule_process(void);

int			rnd_ready = 0;
int			rnd_initial_entropy = 0;
int			rnd_printing = 0;

#ifdef DIAGNOSTIC
static int		rnd_tested = 0;
static rngtest_t	rnd_rt;
static uint8_t		rnd_testbits[sizeof(rnd_rt.rt_b)];
#endif

LIST_HEAD(, krndsource)	rnd_sources;

rndsave_t		*boot_rsp;

static inline void
rnd_printf(const char *fmt, ...)
{
	va_list ap;

	membar_consumer();
	if (rnd_printing) {
		return;
	}
	rnd_printing = 1;
	membar_producer();
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	rnd_printing = 0;
}

void
rnd_init_softint(void) {
	rnd_process = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    rnd_intr, NULL);
	rnd_wakeup = softint_establish(SOFTINT_CLOCK|SOFTINT_MPSAFE,
	    rnd_wake, NULL);
	rnd_schedule_process();
}

/*
 * Generate a 64-bit counter.
 */
static inline uint32_t
rnd_counter(void)
{
	struct timespec ts;
	uint32_t ret;

#if defined(__HAVE_CPU_COUNTER)
	return (cpu_counter32());
#endif
	if (rnd_ready) {
		nanouptime(&ts);
		ret = ts.tv_sec;
		ret *= (uint32_t)1000000000;
		ret += ts.tv_nsec;
		return ret;
	}
	/* when called from rnd_init, its too early to call nanotime safely */
	return (0);
}

/*
 * We may be called from low IPL -- protect our softint.
 */

static inline void
rnd_schedule_softint(void *softint)
{
	kpreempt_disable();
	softint_schedule(softint);
	kpreempt_enable();
}

static inline void
rnd_schedule_process(void)
{
	if (__predict_true(rnd_process)) {
		rnd_schedule_softint(rnd_process);
		return;
	} 
	rnd_process_events();
}

static inline void
rnd_schedule_wakeup(void)
{
	if (__predict_true(rnd_wakeup)) {
		rnd_schedule_softint(rnd_wakeup);
		return;
	}
	rnd_wakeup_readers();
}

/*
 * Tell any sources with "feed me" callbacks that we are hungry.
 */
void
rnd_getmore(size_t byteswanted)
{
	krndsource_t *rs; 

	KASSERT(mutex_owned(&rndpool_mtx));

	LIST_FOREACH(rs, &rnd_sources, list) {
		if (rs->flags & RND_FLAG_HASCB) {
			KASSERT(rs->get != NULL);
			KASSERT(rs->getarg != NULL);
			rs->get(byteswanted, rs->getarg);
#ifdef RND_VERBOSE
			rnd_printf("rnd: asking source %s for %zu bytes\n",
			       rs->name, byteswanted);
#endif
		}    
	}    
}

/*
 * Check to see if there are readers waiting on us.  If so, kick them.
 */
void
rnd_wakeup_readers(void)
{

	/*
	 * XXX This bookkeeping shouldn't be here -- this is not where
	 * the rnd_empty/rnd_initial_entropy state change actually
	 * happens.
	 */
	mutex_spin_enter(&rndpool_mtx);
	const size_t entropy_count = rndpool_get_entropy_count(&rnd_pool);
	if (entropy_count < RND_ENTROPY_THRESHOLD * 8) {
		rnd_empty = 1;
		mutex_spin_exit(&rndpool_mtx);
		return;
	} else {
#ifdef RND_VERBOSE
		if (__predict_false(!rnd_initial_entropy))
			rnd_printf("rnd: have initial entropy (%zu)\n",
			    entropy_count);
#endif
		rnd_empty = 0;
		rnd_initial_entropy = 1;
	}
	mutex_spin_exit(&rndpool_mtx);

	rndsinks_distribute();
}

/*
 * Use the timing/value of the event to estimate the entropy gathered.
 * If all the differentials (first, second, and third) are non-zero, return
 * non-zero.  If any of these are zero, return zero.
 */
static inline uint32_t
rnd_delta_estimate(rnd_delta_t *d, uint32_t v, int32_t delta)
{
	int32_t delta2, delta3;

	d->insamples++;

	/*
	 * Calculate the second and third order differentials
	 */
	delta2 = d->dx - delta;
	if (delta2 < 0)
		delta2 = -delta2;

	delta3 = d->d2x - delta2;
	if (delta3 < 0)
		delta3 = -delta3;

	d->x = v;
	d->dx = delta;
	d->d2x = delta2;

	/*
	 * If any delta is 0, we got no entropy.  If all are non-zero, we
	 * might have something.
	 */
	if (delta == 0 || delta2 == 0 || delta3 == 0)
		return (0);

	d->outbits++;
	return (1);
}

/*
 * Delta estimator for 32-bit timeestamps.  Must handle wrap.
 */
static inline uint32_t
rnd_dt_estimate(krndsource_t *rs, uint32_t t)
{
	int32_t delta;
	uint32_t ret;
	rnd_delta_t *d = &rs->time_delta;

	if (t < d->x) {
		delta = UINT32_MAX - d->x + t;
	} else {
		delta = d->x - t;
	}

	if (delta < 0) {
		delta = -delta;
	}

	ret = rnd_delta_estimate(d, t, delta);

	KASSERT(d->x == t);
	KASSERT(d->dx == delta);
#ifdef RND_VERBOSE
	if (deltacnt++ % 1151 == 0) {
		rnd_printf("rnd_dt_estimate: %s x = %lld, dx = %lld, "
		       "d2x = %lld\n", rs->name,
		       (int)d->x, (int)d->dx, (int)d->d2x);
	}
#endif
	return ret;
}

/*
 * Delta estimator for 32 or bit values.  "Wrap" isn't.
 */
static inline uint32_t
rnd_dv_estimate(krndsource_t *rs, uint32_t v)
{
	int32_t delta;
	uint32_t ret;
	rnd_delta_t *d = &rs->value_delta;

	delta = d->x - v;

	if (delta < 0) {
		delta = -delta;
	}
	ret = rnd_delta_estimate(d, v, (uint32_t)delta);

	KASSERT(d->x == v);
	KASSERT(d->dx == delta);
#ifdef RND_VERBOSE
	if (deltacnt++ % 1151 == 0) {
		rnd_printf("rnd_dv_estimate: %s x = %lld, dx = %lld, "
		       " d2x = %lld\n", rs->name,
		       (long long int)d->x,
		       (long long int)d->dx,
		       (long long int)d->d2x);
	}
#endif
	return ret;
}

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
	 * delta-time entropy estimator seems to think we get 1 bit here
	 * about every 2 calls.  That seems like too much.  Instead,
	 * we feed the rnd_counter() value to the value estimator as well,
	 * to take advantage of the additional LZ test on estimated values.
	 *
	 */
	if (__predict_false(!live)) {
		rnd_attach_source(&skewsrc, "callout", RND_TYPE_SKEW,
				  RND_FLAG_COLLECT_VALUE|
				  RND_FLAG_ESTIMATE_VALUE);
		live = 1;
	}

	flipflop = !flipflop;

	if (flipflop) {
		rnd_add_uint32(&skewsrc, rnd_counter());
		callout_schedule(&skew_callout, hz / 10);
	} else {
		callout_schedule(&skew_callout, 1);
	}
}

/*
 * initialize the global random pool for our use.
 * rnd_init() must be called very early on in the boot process, so
 * the pool is ready for other devices to attach as sources.
 */
void
rnd_init(void)
{
	uint32_t c;

	if (rnd_ready)
		return;

	mutex_init(&rnd_mtx, MUTEX_DEFAULT, IPL_VM);
	rndsinks_init();

	/*
	 * take a counter early, hoping that there's some variance in
	 * the following operations
	 */
	c = rnd_counter();

	LIST_INIT(&rnd_sources);
	SIMPLEQ_INIT(&rnd_samples);

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

	/*
	 * If we have a cycle counter, take its error with respect
	 * to the callout mechanism as a source of entropy, ala
	 * TrueRand.
 	 *
	 */
#if defined(__HAVE_CPU_COUNTER)
	callout_init(&skew_callout, CALLOUT_MPSAFE);
	callout_setfunc(&skew_callout, rnd_skew, NULL);
	rnd_skew(NULL);
#endif

#ifdef RND_VERBOSE
	rnd_printf("rnd: initialised (%u)%s", RND_POOLBITS,
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
		rnd_printf("rnd: seeded with %d bits\n",
		       MIN(boot_rsp->entropy, RND_POOLBITS / 2));
#endif
		memset(boot_rsp, 0, sizeof(*boot_rsp));
	}
	rnd_attach_source(&rnd_source_anonymous, "Anonymous",
			  RND_TYPE_UNKNOWN,
			  RND_FLAG_COLLECT_TIME|RND_FLAG_COLLECT_VALUE|
			  RND_FLAG_ESTIMATE_TIME);
	rnd_attach_source(&rnd_printf_source, "printf", RND_TYPE_UNKNOWN,
			  RND_FLAG_NO_ESTIMATE);
	rnd_attach_source(&rnd_autoconf_source, "autoconf",
			  RND_TYPE_UNKNOWN,
			  RND_FLAG_COLLECT_TIME|RND_FLAG_ESTIMATE_TIME);
	rnd_ready = 1;
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
rnd_attach_source(krndsource_t *rs, const char *name, uint32_t type,
    uint32_t flags)
{
	uint32_t ts;

	ts = rnd_counter();

	strlcpy(rs->name, name, sizeof(rs->name));
	memset(&rs->time_delta, 0, sizeof(rs->time_delta));
	rs->time_delta.x = ts;
	memset(&rs->value_delta, 0, sizeof(rs->value_delta));
	rs->total = 0;

	/*
	 * Some source setup, by type
	 */
	rs->test = NULL;
	rs->test_cnt = -1;

	if (flags == 0) {
		flags = RND_FLAG_DEFAULT;
	}

	switch (type) {
	    case RND_TYPE_NET:		/* Don't collect by default */
		flags |= (RND_FLAG_NO_COLLECT | RND_FLAG_NO_ESTIMATE);
		break;
	    case RND_TYPE_RNG:		/* Space for statistical testing */
		rs->test = kmem_alloc(sizeof(rngtest_t), KM_NOSLEEP);
		rs->test_cnt = 0;
		/* FALLTHRU */
	    case RND_TYPE_VM:		/* Process samples in bulk always */
		flags |= RND_FLAG_FAST;
		break;
	    default:
		break;
	}

	rs->type = type;
	rs->flags = flags;

	rs->state = rnd_sample_allocate(rs);

	mutex_spin_enter(&rndpool_mtx);
	LIST_INSERT_HEAD(&rnd_sources, rs, list);

#ifdef RND_VERBOSE
	rnd_printf("rnd: %s attached as an entropy source (", rs->name);
	if (!(flags & RND_FLAG_NO_COLLECT)) {
		rnd_printf("collecting");
		if (flags & RND_FLAG_NO_ESTIMATE)
			rnd_printf(" without estimation");
	}
	else
		rnd_printf("off");
	rnd_printf(")\n");
#endif

	/*
	 * Again, put some more initial junk in the pool.
	 * FreeBSD claim to have an analysis that show 4 bits of
	 * entropy per source-attach timestamp.  I am skeptical,
	 * but we count 1 bit per source here.
	 */
	rndpool_add_data(&rnd_pool, &ts, sizeof(ts), 1);
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
	rnd_printf("rnd: %s detached as an entropy source\n", source->name);
#endif
}

static inline uint32_t
rnd_estimate(krndsource_t *rs, uint32_t ts, uint32_t val)
{
	uint32_t entropy = 0, dt_est, dv_est;

	dt_est = rnd_dt_estimate(rs, ts);
	dv_est = rnd_dv_estimate(rs, val);

	if (!(rs->flags & RND_FLAG_NO_ESTIMATE)) {
		if (rs->flags & RND_FLAG_ESTIMATE_TIME) {
			entropy += dt_est;
		}

                if (rs->flags & RND_FLAG_ESTIMATE_VALUE) {
			entropy += dv_est;
		}

	}
	return entropy;
}

/*
 * Add a 32-bit value to the entropy pool.  The rs parameter should point to
 * the source-specific source structure.
 */
void
_rnd_add_uint32(krndsource_t *rs, uint32_t val)
{
	uint32_t ts;	
	uint32_t entropy = 0;

	if (rs->flags & RND_FLAG_NO_COLLECT)
		return;

	/*
	 * Sample the counter as soon as possible to avoid
	 * entropy overestimation.
	 */
	ts = rnd_counter();

	/*
	 * Calculate estimates - we may not use them, but if we do
	 * not calculate them, the estimators' history becomes invalid.
	 */
	entropy = rnd_estimate(rs, ts, val);

	rnd_add_data_ts(rs, &val, sizeof(val), entropy, ts);
}

void
_rnd_add_uint64(krndsource_t *rs, uint64_t val)
{
	uint32_t ts;   
	uint32_t entropy = 0;

	if (rs->flags & RND_FLAG_NO_COLLECT)
                return;

	/*
	 * Sample the counter as soon as possible to avoid
	 * entropy overestimation.
	 */
	ts = rnd_counter();

	/*
	 * Calculate estimates - we may not use them, but if we do
	 * not calculate them, the estimators' history becomes invalid.
	 */
	entropy = rnd_estimate(rs, ts, (uint32_t)(val & (uint64_t)0xffffffff));

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
	mutex_spin_enter(&rndpool_mtx);
	if (__predict_false(rs == NULL)) {
		rs = &rnd_source_anonymous;
	}
	rndpool_add_data(&rnd_pool, data, len, entropy);
	mutex_spin_exit(&rndpool_mtx);
}

static void
rnd_add_data_ts(krndsource_t *rs, const void *const data, u_int32_t len,
		u_int32_t entropy, uint32_t ts)
{
	rnd_sample_t *state = NULL;
	const uint32_t *dint = data;
	int todo, done, filled = 0;
	int sample_count;
	SIMPLEQ_HEAD(, _rnd_sample_t) tmp_samples =
	    		SIMPLEQ_HEAD_INITIALIZER(tmp_samples);

	if (rs && (rs->flags & RND_FLAG_NO_COLLECT ||
	    __predict_false(!(rs->flags & 
			     (RND_FLAG_COLLECT_TIME|
			     RND_FLAG_COLLECT_VALUE))))) {
		return;
	}
	todo = len / sizeof(*dint);
	/*
	 * Let's try to be efficient: if we are warm, and a source
	 * is adding entropy at a rate of at least 1 bit every 10 seconds,
	 * mark it as "fast" and add its samples in bulk.
	 */
	if (__predict_true(rs->flags & RND_FLAG_FAST)) {
		sample_count = RND_SAMPLE_COUNT;
	} else {
		if (!cold && rnd_initial_entropy) {
			struct timeval upt;

			getmicrouptime(&upt);
			if ((todo >= RND_SAMPLE_COUNT) ||
			    (upt.tv_sec > 0  && rs->total > upt.tv_sec * 10) ||
			    (upt.tv_sec > 10 && rs->total > upt.tv_sec) ||
			    (upt.tv_sec > 100 &&
			      rs->total > upt.tv_sec / 10)) {
#ifdef RND_VERBOSE
				rnd_printf("rnd: source %s is fast (%d samples "
				       "at once, %d bits in %lld seconds), "
				       "processing samples in bulk.\n",
				       rs->name, todo, rs->total,
				       (long long int)upt.tv_sec);
#endif
				rs->flags |= RND_FLAG_FAST;
			}
		}
		sample_count = 2;
	}

	/*
	 * Loop over data packaging it into sample buffers.
	 * If a sample buffer allocation fails, drop all data.
	 */
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

		if (state->cursor == sample_count) {
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
	mutex_spin_exit(&rnd_mtx);

	/* Cause processing of queued samples */
	rnd_schedule_process();
}

static int
rnd_hwrng_test(rnd_sample_t *sample)
{
	krndsource_t *source = sample->source;
	size_t cmplen;
	uint8_t *v1, *v2;
	size_t resid, totest;

	KASSERT(source->type == RND_TYPE_RNG);

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
		rnd_printf("rnd: source \"%s\" failed continuous-output test.\n",
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
			rnd_printf("rnd: source \"%s\" failed statistical test.",
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
 */
static void
rnd_process_events(void)
{
	rnd_sample_t *sample = NULL;
	krndsource_t *source, *badsource = NULL;
	static krndsource_t *last_source;
	u_int32_t entropy;
	size_t pool_entropy;
	int found = 0, wake = 0;
	SIMPLEQ_HEAD(, _rnd_sample_t) dq_samples =
			SIMPLEQ_HEAD_INITIALIZER(dq_samples);
	SIMPLEQ_HEAD(, _rnd_sample_t) df_samples =
			SIMPLEQ_HEAD_INITIALIZER(df_samples);

	/*
	 * Sample queue is protected by rnd_mtx, drain to onstack queue
	 * and drop lock.
	 */

	mutex_spin_enter(&rnd_mtx);
	while ((sample = SIMPLEQ_FIRST(&rnd_samples))) {
		found++;
		SIMPLEQ_REMOVE_HEAD(&rnd_samples, next);
		/*
		 * We repeat this check here, since it is possible
		 * the source was disabled before we were called, but
		 * after the entry was queued.
		 */
		if (__predict_false(!(sample->source->flags &
				    (RND_FLAG_COLLECT_TIME|
				     RND_FLAG_COLLECT_VALUE)))) {
			SIMPLEQ_INSERT_TAIL(&df_samples, sample, next);
		} else {
			SIMPLEQ_INSERT_TAIL(&dq_samples, sample, next);
		}
	}
	mutex_spin_exit(&rnd_mtx);

	/* Don't thrash the rndpool mtx either.  Hold, add all samples. */
	mutex_spin_enter(&rndpool_mtx);

	pool_entropy = rndpool_get_entropy_count(&rnd_pool);

	while ((sample = SIMPLEQ_FIRST(&dq_samples))) {
		int sample_count;

		SIMPLEQ_REMOVE_HEAD(&dq_samples, next);
		source = sample->source;
		entropy = sample->entropy;
		sample_count = sample->cursor + 1;

		/*
		 * Don't provide a side channel for timing attacks on
		 * low-rate sources: require mixing with some other
		 * source before we schedule a wakeup.
		 */
		if (!wake &&
		    (source != last_source || source->flags & RND_FLAG_FAST)) {
			wake++;
		}
		last_source = source;

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
				rnd_printf("rnd: detaching source \"%s\".",
				       badsource->name);
				break;
			}
		}

		if (source->flags & RND_FLAG_COLLECT_VALUE) {
			rndpool_add_data(&rnd_pool, sample->values,
					 sample_count *
					     sizeof(sample->values[1]),
					 0);
		}
		if (source->flags & RND_FLAG_COLLECT_TIME) {
			rndpool_add_data(&rnd_pool, sample->ts,
					 sample_count *
					     sizeof(sample->ts[1]),
					 0);
		}

		pool_entropy += entropy;
		source->total += sample->entropy;
		SIMPLEQ_INSERT_TAIL(&df_samples, sample, next);
	}
	rndpool_set_entropy_count(&rnd_pool, pool_entropy);
	if (pool_entropy > RND_ENTROPY_THRESHOLD * 8) {
		wake++;
	} else {
		rnd_empty = 1;
		rnd_getmore((RND_POOLBITS - pool_entropy) / 8);
#ifdef RND_VERBOSE
		rnd_printf("rnd: empty, asking for %d bits\n",
		       (int)((RND_POOLBITS - pool_entropy) / 8));
#endif
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
	if (wake) {
		rnd_schedule_wakeup();
	}
}

static void
rnd_intr(void *arg)
{
	rnd_process_events();
}

static void
rnd_wake(void *arg)
{
	rnd_wakeup_readers();
}

u_int32_t
rnd_extract_data_locked(void *p, u_int32_t len, u_int32_t flags)
{
	static int timed_in;
	int entropy_count;

	KASSERT(mutex_owned(&rndpool_mtx));
	if (__predict_false(!timed_in)) {
		if (boottime.tv_sec) {
			rndpool_add_data(&rnd_pool, &boottime,
					 sizeof(boottime), 0);
		}
		timed_in++;
	}
	if (__predict_false(!rnd_initial_entropy)) {
		uint32_t c;

#ifdef RND_VERBOSE
		rnd_printf("rnd: WARNING! initial entropy low (%u).\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif
		/* Try once again to put something in the pool */
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(c), 1);
	}

#ifdef DIAGNOSTIC
	while (!rnd_tested) {
		entropy_count = rndpool_get_entropy_count(&rnd_pool);
#ifdef RND_VERBOSE
		rnd_printf("rnd: starting statistical RNG test, entropy = %d.\n",
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
			rnd_printf("rnd: WARNING, ENTROPY POOL FAILED "
			       "STATISTICAL TEST!\n");
			continue;
		}
		memset(&rnd_rt, 0, sizeof(rnd_rt));
		rndpool_add_data(&rnd_pool, rnd_testbits, sizeof(rnd_testbits),
				 entropy_count);
		memset(rnd_testbits, 0, sizeof(rnd_testbits));
#ifdef RND_VERBOSE
		rnd_printf("rnd: statistical RNG test done, entropy = %d.\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif
		rnd_tested++;
	}
#endif
	entropy_count = rndpool_get_entropy_count(&rnd_pool);
	if (entropy_count < (RND_ENTROPY_THRESHOLD * 2 + len) * NBBY) {
		rnd_getmore(howmany((RND_POOLBITS - entropy_count), NBBY));
	}
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
rnd_seed(void *base, size_t len)
{
	SHA1_CTX s;
	uint8_t digest[SHA1_DIGEST_LENGTH];

	if (len != sizeof(*boot_rsp)) {
		rnd_printf("rnd: bad seed length %d\n", (int)len);
		return;
	}

	boot_rsp = (rndsave_t *)base;
	SHA1Init(&s);
	SHA1Update(&s, (uint8_t *)&boot_rsp->entropy,
		   sizeof(boot_rsp->entropy));
	SHA1Update(&s, boot_rsp->data, sizeof(boot_rsp->data));
	SHA1Final(digest, &s);

	if (memcmp(digest, boot_rsp->digest, sizeof(digest))) {
		rnd_printf("rnd: bad seed checksum\n");
		return;
	}

	/*
	 * It's not really well-defined whether bootloader-supplied
	 * modules run before or after rnd_init().  Handle both cases.
	 */
	if (rnd_ready) {
#ifdef RND_VERBOSE
		rnd_printf("rnd: ready, feeding in seed data directly.\n");
#endif
		mutex_spin_enter(&rndpool_mtx);
		rndpool_add_data(&rnd_pool, boot_rsp->data,
				 sizeof(boot_rsp->data),
				 MIN(boot_rsp->entropy, RND_POOLBITS / 2));
		memset(boot_rsp, 0, sizeof(*boot_rsp));
		mutex_spin_exit(&rndpool_mtx);
	} else {
#ifdef RND_VERBOSE
		rnd_printf("rnd: not ready, deferring seed feed.\n");
#endif
	}
}
