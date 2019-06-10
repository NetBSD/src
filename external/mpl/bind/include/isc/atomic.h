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

#if HAVE_STDATOMIC_H
#include <stdatomic.h>
#else
#include <isc/stdatomic.h>
#endif

/*
 * We define a few additional macros to make things easier
 */

#define atomic_store_relaxed(o, v) atomic_store_explicit((o), \
							 (v), \
							 memory_order_relaxed)
#define atomic_load_relaxed(o) atomic_load_explicit((o), memory_order_relaxed)
