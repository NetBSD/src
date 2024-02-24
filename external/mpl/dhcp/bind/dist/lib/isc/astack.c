/*	$NetBSD: astack.c,v 1.1.2.2 2024/02/24 13:07:19 martin Exp $	*/

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

#include <inttypes.h>
#include <string.h>

#include <isc/astack.h>
#include <isc/atomic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/types.h>
#include <isc/util.h>

struct isc_astack {
	isc_mem_t *mctx;
	size_t size;
	size_t pos;
	isc_mutex_t lock;
	uintptr_t nodes[];
};

isc_astack_t *
isc_astack_new(isc_mem_t *mctx, size_t size) {
	isc_astack_t *stack = isc_mem_get(
		mctx, sizeof(isc_astack_t) + size * sizeof(uintptr_t));

	*stack = (isc_astack_t){
		.size = size,
	};
	isc_mem_attach(mctx, &stack->mctx);
	memset(stack->nodes, 0, size * sizeof(uintptr_t));
	isc_mutex_init(&stack->lock);
	return (stack);
}

bool
isc_astack_trypush(isc_astack_t *stack, void *obj) {
	if (!isc_mutex_trylock(&stack->lock)) {
		if (stack->pos >= stack->size) {
			UNLOCK(&stack->lock);
			return (false);
		}
		stack->nodes[stack->pos++] = (uintptr_t)obj;
		UNLOCK(&stack->lock);
		return (true);
	} else {
		return (false);
	}
}

void *
isc_astack_pop(isc_astack_t *stack) {
	LOCK(&stack->lock);
	uintptr_t rv;
	if (stack->pos == 0) {
		rv = 0;
	} else {
		rv = stack->nodes[--stack->pos];
	}
	UNLOCK(&stack->lock);
	return ((void *)rv);
}

void
isc_astack_destroy(isc_astack_t *stack) {
	LOCK(&stack->lock);
	REQUIRE(stack->pos == 0);
	UNLOCK(&stack->lock);

	isc_mutex_destroy(&stack->lock);

	isc_mem_putanddetach(&stack->mctx, stack,
			     sizeof(struct isc_astack) +
				     stack->size * sizeof(uintptr_t));
}
