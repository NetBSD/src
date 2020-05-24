/*	$NetBSD: queue.h,v 1.4 2020/05/24 19:46:26 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once
#include <isc/mem.h>

typedef struct isc_queue isc_queue_t;

isc_queue_t *
isc_queue_new(isc_mem_t *mctx, int max_threads);
/*%<
 * Create a new fetch-and-add array queue.
 *
 * 'max_threads' is currently unused. In the future it can be used
 * to pass a maximum threads parameter when creating hazard pointers,
 * but currently `isc_hp_t` uses a hard-coded value.
 */

void
isc_queue_enqueue(isc_queue_t *queue, uintptr_t item);
/*%<
 * Enqueue an object pointer 'item' at the tail of the queue.
 *
 * Requires:
 * \li	'item' is not null.
 */

uintptr_t
isc_queue_dequeue(isc_queue_t *queue);
/*%<
 * Remove an object pointer from the head of the queue and return the
 * pointer. If the queue is empty, return `nulluintptr` (the uintptr_t
 * representation of NULL).
 *
 * Requires:
 * \li	'queue' is not null.
 */

void
isc_queue_destroy(isc_queue_t *queue);
/*%<
 * Destroy a queue.
 *
 * Requires:
 * \li	'queue' is not null.
 */
