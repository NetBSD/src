/*	$NetBSD: subr_pcq.c,v 1.3 2008/11/11 21:45:33 rmind Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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
__KERNEL_RCSID(0, "$NetBSD: subr_pcq.c,v 1.3 2008/11/11 21:45:33 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/kmem.h>

#include <sys/pcq.h>

typedef void * volatile pcq_entry_t;

struct pcq {
	pcq_entry_t *pcq_consumer;
	pcq_entry_t *pcq_producer;
	pcq_entry_t *pcq_limit;
	pcq_entry_t pcq_base[];
};

static inline pcq_entry_t *
pcq_advance(pcq_t *pcq, pcq_entry_t *ptr)
{

	if (__predict_false(++ptr == pcq->pcq_limit))
		return pcq->pcq_base;

	return ptr;
}

bool
pcq_put(pcq_t *pcq, void *item)
{
	pcq_entry_t *producer;

	KASSERT(item != NULL);

	/*
	 * Get our starting point,  While we are doing this, it is
	 * imperative that pcq->pcq_base/pcq->pcq_limit not change
	 * in value.  If you need to resize a pcq, init a new pcq
	 * with the right size and swap pointers to it.
	 */
	membar_consumer();	/* see updates to pcq_producer */
	producer = pcq->pcq_producer;
	for (;;) {
		/*
		 * Preadvance so we reduce the window on updates.
		 */
		pcq_entry_t * const new_producer = pcq_advance(pcq, producer);

		/*
		 * Try to fill an empty slot
		 */
		if (NULL == atomic_cas_ptr(producer, NULL, item)) {
			/*
			 * We need to use atomic_cas_ptr since another thread
			 * might have inserted between these two cas operations
			 * and we don't want to overwrite a producer that's
			 * more up-to-date.
			 */
			atomic_cas_ptr(&pcq->pcq_producer,
			    __UNVOLATILE(producer), 
			    __UNVOLATILE(new_producer));
			/*
			 * Tell them we were able to enqueue it.
			 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
			membar_producer();
#endif
			return true;
		}

		/*
		 * If we've reached the consumer, we've filled all the
		 * slots and there's no more room so return false.
		 */
#ifndef __HAVE_ATOMIC_AS_MEMBAR
		membar_consumer();	/* see updates to pcq_consumer */
#endif
		if (producer == pcq->pcq_consumer)
			return false;

		/*
		 * Let's see if the next slot is free...
		 */
		producer = new_producer;
	}
}

/*
 * It's assumed that the enclosing structure that contains the pcq will
 * provide appropriate locking to prevent concurrent gets from occurring.
 */
void *
pcq_get(pcq_t *pcq)
{
	pcq_entry_t * const consumer = pcq->pcq_consumer;
	void *item;

	/*
	 * Updates to pcq_consumer doesn't matter since we control it but we
	 * want to make sure that any stores to what it references have
	 * completed.
	 */
	membar_consumer();

	/*
	 * If there's nothing to return, just return.
	 */
	if ((item = *consumer) == NULL)
		return NULL;

	/*
	 * Update the consumer and free the slot.
	 * Update the consumer pointer first so when producer == consumer
	 * the right thing happens.
	 *
	 * 1) until the slot set to NULL, pcq_put will fail since
	 *    the slot != NULL && producer == consumer.
	 * 2) consumer is advanced but the slot is still not NULL,
	 *    pcq_put will advance by one, see that producer == consumer,
	 *    and fail.
	 * 4) Once the slot is set to NULL, the producer can fill the slot
	 *    and advance the producer.
	 *    
	 * and then we are back to 1.
	 */
	pcq->pcq_consumer = pcq_advance(pcq, consumer);
	membar_producer();

	*consumer = NULL;
	membar_producer();

	return item;
}

void *
pcq_peek(pcq_t *pcq)
{

	membar_consumer();	/* see updates to *pcq_consumer */ 
	return *pcq->pcq_consumer;
}

size_t
pcq_maxitems(pcq_t *pcq)
{

	return pcq->pcq_limit - pcq->pcq_base;
}

pcq_t *
pcq_create(size_t maxitems, km_flag_t kmflags)
{
	pcq_t *pcq;

	KASSERT(maxitems > 0);

	pcq = kmem_zalloc(offsetof(pcq_t, pcq_base[maxitems]), kmflags);
	if (__predict_false(pcq == NULL))
		return NULL;

	pcq->pcq_limit = pcq->pcq_base + maxitems;
	pcq->pcq_producer = pcq->pcq_base;
	pcq->pcq_consumer = pcq->pcq_producer;

	return pcq;
}

void
pcq_destroy(pcq_t *pcq)
{

	KASSERT(*pcq->pcq_consumer == NULL);

	kmem_free(pcq, (uintptr_t)pcq->pcq_limit - (uintptr_t)pcq);
}
