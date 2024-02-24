/*	$NetBSD: astack.h,v 1.1.2.2 2024/02/24 13:07:23 martin Exp $	*/

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

#pragma once

#include <inttypes.h>

#include <isc/mem.h>
#include <isc/types.h>

isc_astack_t *
isc_astack_new(isc_mem_t *mctx, size_t size);
/*%<
 * Allocate and initialize a new array stack of size 'size'.
 */

void
isc_astack_destroy(isc_astack_t *stack);
/*%<
 * Free an array stack 'stack'.
 *
 * Requires:
 * \li	'stack' is empty.
 */

bool
isc_astack_trypush(isc_astack_t *stack, void *obj);
/*%<
 * Try to push 'obj' onto array stack 'astack'. On failure, either
 * because the stack size limit has been reached or because another
 * thread has already changed the stack pointer, return 'false'.
 */

void *
isc_astack_pop(isc_astack_t *stack);
/*%<
 * Pop an object off of array stack 'stack'. If the stack is empty,
 * return NULL.
 */
