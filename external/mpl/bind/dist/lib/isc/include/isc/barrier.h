/*	$NetBSD: barrier.h,v 1.3.2.1 2024/02/25 15:47:20 martin Exp $	*/

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

#include <isc/util.h>

#if HAVE_PTHREAD_BARRIER_INIT

#include <pthread.h>

#define isc_barrier_t pthread_barrier_t

#define isc_barrier_init(barrier, count) \
	pthread_barrier_init(barrier, NULL, count)
#define isc_barrier_destroy(barrier) pthread_barrier_destroy(barrier)
#define isc_barrier_wait(barrier)    pthread_barrier_wait(barrier)

#else

#include <uv.h>

#define isc_barrier_t uv_barrier_t

#define isc_barrier_init(barrier, count) uv_barrier_init(barrier, count)
#define isc_barrier_destroy(barrier)	 uv_barrier_destroy(barrier)
#define isc_barrier_wait(barrier)	 uv_barrier_wait(barrier)

#endif /* __SANITIZE_THREAD__ */
