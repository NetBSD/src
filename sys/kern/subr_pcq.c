/*	$NetBSD: subr_pcq.c,v 1.20 2023/02/24 11:02:27 riastradh Exp $	*/

/*-
 * Copyright (c) 2009, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Lockless producer/consumer queue.
 *
 * Summary of the producer algorithm in pcq_put (may run many in
 * parallel with each other and with a consumer):
 *
 *	P1. initialize an item
 *
 *	P2. atomic_cas(&pcq->pcq_pc) loop to advance the producer
 *	    pointer, reserving a space at c (fails if not enough space)
 *
 *	P3. atomic_store_release(&pcq->pcq_items[c], item) to publish
 *	    the item in the space it reserved
 *
 * Summary of the consumer algorithm in pcq_get (must be serialized by
 * caller with other consumers, may run in parallel with any number of
 * producers):
 *
 *	C1. atomic_load_relaxed(&pcq->pcq_pc) to get the consumer
 *	    pointer and a snapshot of the producer pointer, which may
 *	    point to null items or point to initialized items (fails if
 *	    no space reserved for published items yet)
 *
 *	C2. atomic_load_consume(&pcq->pcq_items[c]) to get the next
 *	    unconsumed but potentially published item (fails if item
 *	    not published yet)
 *
 *	C3. pcq->pcq_items[c] = NULL to consume the next unconsumed but
 *	    published item
 *
 *	C4. membar_producer
 *
 *	C5. atomic_cas(&pcq->pcq_pc) loop to advance the consumer
 *	    pointer
 *
 *	C6. use the item
 *
 * Note that there is a weird bare membar_producer which is not matched
 * by membar_consumer.  This is one of the rare cases of a memory
 * barrier on one side that is not matched by a memory barrier on
 * another side, but the ordering works out, with a somewhat more
 * involved proof.
 *
 * Some properties that need to be proved:
 *
 *	Theorem 1.  For pcq_put call that leads into pcq_get:
 *	Initializing item at P1 is dependency-ordered before usage of
 *	item at C6, so items placed by pcq_put can be safely used by
 *	the caller of pcq_get.
 *
 *	Proof sketch.
 *
 *		Assume load/store P2 synchronizes with load/store C1
 *		(if not, pcq_get fails in `if (p == c) return NULL').
 *
 *		Assume store-release P3 synchronizes with load-consume
 *		C2 (if not, pcq_get fails in `if (item == NULL) return
 *		NULL').
 *
 *		Then:
 *
 *		- P1 is sequenced before store-release P3
 *		- store-release P3 synchronizes with load-consume C2
 *		- load-consume C2 is dependency-ordered before C6
 *
 *		Hence transitively, P1 is dependency-ordered before C6,
 *		QED.
 *
 *	Theorem 2.  For pcq_get call followed by pcq_put: Nulling out
 *	location at store C3 happens before placing a new item in the
 *	same location at store P3, so items are not lost.
 *
 *	Proof sketch.
 *
 *		Assume load/store C5 synchronizes with load/store P2
 *		(otherwise pcq_peek starts over the CAS loop or fails).
 *
 *		Then:
 *
 *		- store C3 is sequenced before membar_producer C4
 *		- membar_producer C4 is sequenced before load/store C5
 *		- load/store C5 synchronizes with load/store P2 at &pcq->pcq_pc
 *		- P2 is sequenced before store-release P3
 *
 *		Hence transitively, store C3 happens before
 *		store-release P3, QED.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_pcq.c,v 1.20 2023/02/24 11:02:27 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <sys/pcq.h>

/*
 * Internal producer-consumer queue structure.  Note: providing a separate
 * cache-line both for pcq_t::pcq_pc and pcq_t::pcq_items.
 */
struct pcq {
	u_int			pcq_nitems;
	uint8_t			pcq_pad1[COHERENCY_UNIT - sizeof(u_int)];
	volatile uint32_t	pcq_pc;
	uint8_t			pcq_pad2[COHERENCY_UNIT - sizeof(uint32_t)];
	void * volatile		pcq_items[];
};

/*
 * Producer (p) - stored in the lower 16 bits of pcq_t::pcq_pc.
 * Consumer (c) - in the higher 16 bits.
 *
 * We have a limitation of 16 bits i.e. 0xffff items in the queue.
 * The PCQ_MAXLEN constant is set accordingly.
 */

static inline void
pcq_split(uint32_t v, u_int *p, u_int *c)
{

	*p = v & 0xffff;
	*c = v >> 16;
}

static inline uint32_t
pcq_combine(u_int p, u_int c)
{

	return p | (c << 16);
}

static inline u_int
pcq_advance(pcq_t *pcq, u_int pc)
{

	if (__predict_false(++pc == pcq->pcq_nitems)) {
		return 0;
	}
	return pc;
}

/*
 * pcq_put: place an item at the end of the queue.
 */
bool
pcq_put(pcq_t *pcq, void *item)
{
	uint32_t v, nv;
	u_int op, p, c;

	KASSERT(item != NULL);

	do {
		v = atomic_load_relaxed(&pcq->pcq_pc);
		pcq_split(v, &op, &c);
		p = pcq_advance(pcq, op);
		if (p == c) {
			/* Queue is full. */
			return false;
		}
		nv = pcq_combine(p, c);
	} while (atomic_cas_32(&pcq->pcq_pc, v, nv) != v);

	/*
	 * Ensure that the update to pcq_pc is globally visible before the
	 * data item.  See pcq_get().  This also ensures that any changes
	 * that the caller made to the data item are globally visible
	 * before we put it onto the list.
	 */
	atomic_store_release(&pcq->pcq_items[op], item);

	/*
	 * Synchronization activity to wake up the consumer will ensure
	 * that the update to pcq_items[] is visible before the wakeup
	 * arrives.  So, we do not need an additional memory barrier here.
	 */
	return true;
}

/*
 * pcq_peek: return the next item from the queue without removal.
 */
void *
pcq_peek(pcq_t *pcq)
{
	const uint32_t v = atomic_load_relaxed(&pcq->pcq_pc);
	u_int p, c;

	pcq_split(v, &p, &c);

	/* See comment on race below in pcq_get(). */
	return (p == c) ? NULL : atomic_load_consume(&pcq->pcq_items[c]);
}

/*
 * pcq_get: remove and return the next item for consumption or NULL if empty.
 *
 * => The caller must prevent concurrent gets from occurring.
 */
void *
pcq_get(pcq_t *pcq)
{
	uint32_t v, nv;
	u_int p, c;
	void *item;

	v = atomic_load_relaxed(&pcq->pcq_pc);
	pcq_split(v, &p, &c);
	if (p == c) {
		/* Queue is empty: nothing to return. */
		return NULL;
	}
	item = atomic_load_consume(&pcq->pcq_items[c]);
	if (item == NULL) {
		/*
		 * Raced with sender: we rely on a notification (e.g. softint
		 * or wakeup) being generated after the producer's pcq_put(),
		 * causing us to retry pcq_get() later.
		 */
		return NULL;
	}
	/*
	 * We have exclusive access to this slot, so no need for
	 * atomic_store_*.
	 */
	pcq->pcq_items[c] = NULL;
	c = pcq_advance(pcq, c);
	nv = pcq_combine(p, c);

	/*
	 * Ensure that update to pcq_items[c] becomes globally visible
	 * before the update to pcq_pc.  If it were reordered to occur
	 * after it, we could in theory wipe out a modification made
	 * to pcq_items[c] by pcq_put().
	 *
	 * No need for load-before-store ordering of membar_release
	 * because the only load we need to ensure happens first is the
	 * load of pcq->pcq_items[c], but that necessarily happens
	 * before the store to pcq->pcq_items[c] to null it out because
	 * it is at the same memory location.  Yes, this is a bare
	 * membar_producer with no matching membar_consumer.
	 */
	membar_producer();
	while (__predict_false(atomic_cas_32(&pcq->pcq_pc, v, nv) != v)) {
		v = atomic_load_relaxed(&pcq->pcq_pc);
		pcq_split(v, &p, &c);
		c = pcq_advance(pcq, c);
		nv = pcq_combine(p, c);
	}
	return item;
}

pcq_t *
pcq_create(size_t nitems, km_flag_t kmflags)
{
	pcq_t *pcq;

	KASSERT(nitems > 0);
	KASSERT(nitems <= PCQ_MAXLEN);

	pcq = kmem_zalloc(offsetof(pcq_t, pcq_items[nitems]), kmflags);
	if (pcq != NULL) {
		pcq->pcq_nitems = nitems;
	}
	return pcq;
}

void
pcq_destroy(pcq_t *pcq)
{

	kmem_free(pcq, offsetof(pcq_t, pcq_items[pcq->pcq_nitems]));
}

size_t
pcq_maxitems(pcq_t *pcq)
{

	return pcq->pcq_nitems;
}
