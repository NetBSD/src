/*	$NetBSD: kern_rndsink.c,v 1.9 2014/09/05 05:57:21 matt Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_rndsink.c,v 1.9 2014/09/05 05:57:21 matt Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/rnd.h>
#include <sys/rndsink.h>

#include <dev/rnd_private.h>	/* XXX provisional, for rnd_extract_data */

enum rsink_state {
	RNDSINK_IDLE,		/* no callback in progress */
	RNDSINK_QUEUED,		/* queued for callback */
	RNDSINK_IN_FLIGHT,	/* callback called */
	RNDSINK_REQUEUED,	/* queued again before callback done */
	RNDSINK_DEAD,		/* destroyed */
};

struct rndsink {
	/* Callback state.  */
	enum rsink_state	rsink_state;

	/* Entry on the queue of rndsinks, iff in the RNDSINK_QUEUED state.  */
	TAILQ_ENTRY(rndsink)	rsink_entry;

	/*
	 * Notifies rndsink_destroy when rsink_state transitions to
	 * RNDSINK_IDLE or RNDSINK_QUEUED.
	 */
	kcondvar_t		rsink_cv;

	/* rndsink_create parameters.  */
	unsigned int		rsink_bytes;
	rndsink_callback_t	*rsink_callback;
	void			*rsink_arg;
};

static kmutex_t 		rndsinks_lock __cacheline_aligned;
static TAILQ_HEAD(, rndsink)	rndsinks = TAILQ_HEAD_INITIALIZER(rndsinks);

void
rndsinks_init(void)
{

	/*
	 * This mutex must be at an ipl as high as the highest ipl of
	 * anyone who wants to call rndsink_request.
	 *
	 * XXX Call this IPL_RND, perhaps.
	 */
	mutex_init(&rndsinks_lock, MUTEX_DEFAULT, IPL_VM);
}

/*
 * XXX Provisional -- rndpool_extract and rndpool_maybe_extract should
 * move into kern_rndpool.c.
 */
extern rndpool_t rnd_pool;
extern kmutex_t rndpool_mtx;

/*
 * Fill the buffer with as much entropy as we can.  Return true if it
 * has full entropy and false if not.
 */
static bool
rndpool_extract(void *buffer, size_t bytes)
{
	const size_t extracted = rnd_extract_data(buffer, bytes,
	    RND_EXTRACT_GOOD);

	if (extracted < bytes) {
		(void)rnd_extract_data((uint8_t *)buffer + extracted,
		    bytes - extracted, RND_EXTRACT_ANY);
		mutex_spin_enter(&rndpool_mtx);
		rnd_getmore(bytes - extracted);
		mutex_spin_exit(&rndpool_mtx);
		return false;
	}

	return true;
}

/*
 * If we have as much entropy as is requested, fill the buffer with it
 * and return true.  Otherwise, leave the buffer alone and return
 * false.
 */

CTASSERT(RND_ENTROPY_THRESHOLD <= 0xffffffffUL);
CTASSERT(RNDSINK_MAX_BYTES <= (0xffffffffUL - RND_ENTROPY_THRESHOLD));
CTASSERT((RNDSINK_MAX_BYTES + RND_ENTROPY_THRESHOLD) <=
	    (0xffffffffUL / NBBY));

static bool
rndpool_maybe_extract(void *buffer, size_t bytes)
{
	bool ok;

	KASSERT(bytes <= RNDSINK_MAX_BYTES);

	const uint32_t bits_needed = ((bytes + RND_ENTROPY_THRESHOLD) * NBBY);

	mutex_spin_enter(&rndpool_mtx);
	if (bits_needed <= rndpool_get_entropy_count(&rnd_pool)) {
		const uint32_t extracted __diagused =
		    rndpool_extract_data(&rnd_pool, buffer, bytes,
			RND_EXTRACT_GOOD);

		KASSERT(extracted == bytes);

		ok = true;
	} else {
		ok = false;
		rnd_getmore(howmany(bits_needed -
			rndpool_get_entropy_count(&rnd_pool), NBBY));
	}
	mutex_spin_exit(&rndpool_mtx);

	return ok;
}

void
rndsinks_distribute(void)
{
	uint8_t buffer[RNDSINK_MAX_BYTES];
	struct rndsink *rndsink;

	explicit_memset(buffer, 0, sizeof(buffer)); /* paranoia */

	mutex_spin_enter(&rndsinks_lock);
	while ((rndsink = TAILQ_FIRST(&rndsinks)) != NULL) {
		KASSERT(rndsink->rsink_state == RNDSINK_QUEUED);

		/* Bail if we can't get some entropy for this rndsink.  */
		if (!rndpool_maybe_extract(buffer, rndsink->rsink_bytes))
			break;

		/*
		 * Got some entropy.  Take the sink off the queue and
		 * feed the entropy to the callback, with rndsinks_lock
		 * dropped.  While running the callback, lock out
		 * rndsink_destroy by marking the sink in flight.
		 */
		TAILQ_REMOVE(&rndsinks, rndsink, rsink_entry);
		rndsink->rsink_state = RNDSINK_IN_FLIGHT;
		mutex_spin_exit(&rndsinks_lock);

		(*rndsink->rsink_callback)(rndsink->rsink_arg, buffer,
		    rndsink->rsink_bytes);
		explicit_memset(buffer, 0, rndsink->rsink_bytes);

		mutex_spin_enter(&rndsinks_lock);

		/*
		 * If, while the callback was running, anyone requested
		 * it be queued up again, do so now.  Otherwise, idle.
		 * Either way, it is now safe to destroy, so wake the
		 * pending rndsink_destroy, if there is one.
		 */
		if (rndsink->rsink_state == RNDSINK_REQUEUED) {
			TAILQ_INSERT_TAIL(&rndsinks, rndsink, rsink_entry);
			rndsink->rsink_state = RNDSINK_QUEUED;
		} else {
			KASSERT(rndsink->rsink_state == RNDSINK_IN_FLIGHT);
			rndsink->rsink_state = RNDSINK_IDLE;
		}
		cv_broadcast(&rndsink->rsink_cv);
	}
	mutex_spin_exit(&rndsinks_lock);

	explicit_memset(buffer, 0, sizeof(buffer));	/* paranoia */
}

static void
rndsinks_enqueue(struct rndsink *rndsink)
{

	KASSERT(mutex_owned(&rndsinks_lock));

	/*
	 * XXX This should request only rndsink->rs_bytes bytes of
	 * entropy, but that might get buffered up indefinitely because
	 * kern_rndq has no bound on the duration before it will
	 * process queued entropy samples.  For now, request refilling
	 * the pool altogether so that the buffer will fill up and get
	 * processed.  Later, we ought to (a) bound the duration before
	 * queued entropy samples get processed, and (b) add a target
	 * or something -- as soon as we get that much from the entropy
	 * sources, distribute it.
	 */
	mutex_spin_enter(&rndpool_mtx);
	rnd_getmore(RND_POOLBITS / NBBY);
	mutex_spin_exit(&rndpool_mtx);

	switch (rndsink->rsink_state) {
	case RNDSINK_IDLE:
		/* Not on the queue and nobody is handling it.  */
		TAILQ_INSERT_TAIL(&rndsinks, rndsink, rsink_entry);
		rndsink->rsink_state = RNDSINK_QUEUED;
		break;

	case RNDSINK_QUEUED:
		/* Already on the queue.  */
		break;

	case RNDSINK_IN_FLIGHT:
		/* Someone is handling it.  Ask to queue it up again.  */
		rndsink->rsink_state = RNDSINK_REQUEUED;
		break;

	case RNDSINK_REQUEUED:
		/* Already asked to queue it up again.  */
		break;

	case RNDSINK_DEAD:
		panic("requesting entropy from dead rndsink: %p", rndsink);

	default:
		panic("rndsink %p in unknown state: %d", rndsink,
		    (int)rndsink->rsink_state);
	}
}

struct rndsink *
rndsink_create(size_t bytes, rndsink_callback_t *callback, void *arg)
{
	struct rndsink *const rndsink = kmem_alloc(sizeof(*rndsink), KM_SLEEP);

	KASSERT(bytes <= RNDSINK_MAX_BYTES);

	rndsink->rsink_state = RNDSINK_IDLE;
	cv_init(&rndsink->rsink_cv, "rndsink");
	rndsink->rsink_bytes = bytes;
	rndsink->rsink_callback = callback;
	rndsink->rsink_arg = arg;

	return rndsink;
}

void
rndsink_destroy(struct rndsink *rndsink)
{

	/*
	 * Make sure the rndsink is off the queue, and if it's already
	 * in flight, wait for the callback to complete.
	 */
	mutex_spin_enter(&rndsinks_lock);
	while (rndsink->rsink_state != RNDSINK_IDLE) {
		switch (rndsink->rsink_state) {
		case RNDSINK_QUEUED:
			TAILQ_REMOVE(&rndsinks, rndsink, rsink_entry);
			rndsink->rsink_state = RNDSINK_IDLE;
			break;

		case RNDSINK_IN_FLIGHT:
		case RNDSINK_REQUEUED:
			cv_wait(&rndsink->rsink_cv, &rndsinks_lock);
			break;

		case RNDSINK_DEAD:
			panic("destroying dead rndsink: %p", rndsink);

		default:
			panic("rndsink %p in unknown state: %d", rndsink,
			    (int)rndsink->rsink_state);
		}
	}
	rndsink->rsink_state = RNDSINK_DEAD;
	mutex_spin_exit(&rndsinks_lock);

	cv_destroy(&rndsink->rsink_cv);

	kmem_free(rndsink, sizeof(*rndsink));
}

void
rndsink_schedule(struct rndsink *rndsink)
{

	/* Optimistically check without the lock whether we're queued.  */
	if ((rndsink->rsink_state != RNDSINK_QUEUED) &&
	    (rndsink->rsink_state != RNDSINK_REQUEUED)) {
		mutex_spin_enter(&rndsinks_lock);
		rndsinks_enqueue(rndsink);
		mutex_spin_exit(&rndsinks_lock);
	}
}

bool
rndsink_request(struct rndsink *rndsink, void *buffer, size_t bytes)
{

	KASSERT(bytes == rndsink->rsink_bytes);

	mutex_spin_enter(&rndsinks_lock);
	const bool full_entropy = rndpool_extract(buffer, bytes);
	if (!full_entropy)
		rndsinks_enqueue(rndsink);
	mutex_spin_exit(&rndsinks_lock);

	return full_entropy;
}
