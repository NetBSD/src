/*	$NetBSD: mutexblock.h,v 1.6.2.1 2024/02/25 15:47:21 martin Exp $	*/

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

/*! \file isc/mutexblock.h */

#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

void
isc_mutexblock_init(isc_mutex_t *block, unsigned int count);
/*%<
 * Initialize a block of locks.  If an error occurs all initialized locks
 * will be destroyed, if possible.
 *
 * Requires:
 *
 *\li	block != NULL
 *
 *\li	count > 0
 *
 */

void
isc_mutexblock_destroy(isc_mutex_t *block, unsigned int count);
/*%<
 * Destroy a block of locks.
 *
 * Requires:
 *
 *\li	block != NULL
 *
 *\li	count > 0
 *
 *\li	Each lock in the block be initialized via isc_mutex_init() or
 * 	the whole block was initialized via isc_mutex_initblock().
 *
 */

ISC_LANG_ENDDECLS
