/*	$NetBSD: queue.h,v 1.7 2025/01/26 16:30:19 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <isc/os.h>
#include <isc/urcu.h>

STATIC_ASSERT(sizeof(struct __cds_wfcq_head) <= ISC_OS_CACHELINE_SIZE,
	      "size of struct __cds_wfcq_head must be smaller than "
	      "ISC_OS_CACHELINE_SIZE");

typedef struct isc_queue {
	struct __cds_wfcq_head head;
	uint8_t		       __padding[ISC_OS_CACHELINE_SIZE -
				 sizeof(struct __cds_wfcq_head)];
	struct cds_wfcq_tail   tail;
} isc_queue_t;

typedef struct cds_wfcq_node isc_queue_node_t;

static inline void
isc_queue_node_init(isc_queue_node_t *node) {
	cds_wfcq_node_init(node);
}

static inline void
isc_queue_init(isc_queue_t *queue) {
	__cds_wfcq_init(&(queue)->head, &(queue)->tail);
}

static inline void
isc_queue_destroy(isc_queue_t *queue) {
	UNUSED(queue);
}

static inline bool
isc_queue_empty(isc_queue_t *queue) {
	return cds_wfcq_empty(&(queue)->head, &(queue)->tail);
}

static inline bool
isc_queue_enqueue(isc_queue_t *queue, isc_queue_node_t *node) {
	return cds_wfcq_enqueue(&(queue)->head, &(queue)->tail, node);
}

#define isc_queue_enqueue_entry(queue, entry, member) \
	cds_wfcq_enqueue(&(queue)->head, &(queue)->tail, &((entry)->member))

static inline isc_queue_node_t *
isc_queue_dequeue(isc_queue_t *queue) {
	return __cds_wfcq_dequeue_nonblocking(&(queue)->head, &(queue)->tail);
}

#define isc_queue_entry(ptr, type, member) \
	caa_container_of_check_null(ptr, type, member)

#define isc_queue_dequeue_entry(queue, type, member) \
	isc_queue_entry(isc_queue_dequeue(queue), type, member)

static inline bool
isc_queue_splice(isc_queue_t *dest, isc_queue_t *src) {
	enum cds_wfcq_ret ret = __cds_wfcq_splice_blocking(
		&dest->head, &dest->tail, &src->head, &src->tail);
	INSIST(ret != CDS_WFCQ_RET_WOULDBLOCK &&
	       ret != CDS_WFCQ_RET_DEST_NON_EMPTY);

	return ret != CDS_WFCQ_RET_SRC_EMPTY;
}

#define isc_queue_first_entry(queue, type, member)                         \
	isc_queue_entry(                                                   \
		__cds_wfcq_first_blocking(&(queue)->head, &(queue)->tail), \
		type, member)

#define isc_queue_next_entry(queue, node, type, member)                 \
	isc_queue_entry(__cds_wfcq_next_blocking(&(queue)->head,        \
						 &(queue)->tail, node), \
			type, member)

#define isc_queue_for_each_entry(queue, pos, member)                       \
	for (pos = isc_queue_first_entry(queue, __typeof__(*pos), member); \
	     pos != NULL;                                                  \
	     pos = isc_queue_next_entry(queue, &(pos)->member,             \
					__typeof__(*pos), member))

#define isc_queue_for_each_entry_safe(queue, pos, next, member)            \
	for (pos = isc_queue_first_entry(queue, __typeof__(*pos), member), \
	    next = (pos ? isc_queue_next_entry(queue, &(pos)->member,      \
					       __typeof__(*pos), member)   \
			: NULL);                                           \
	     pos != NULL; pos = next,                                      \
	    next = (pos ? isc_queue_next_entry(queue, &(pos)->member,      \
					       __typeof__(*pos), member)   \
			: NULL))
