/*	$NetBSD: uv.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#include <stdbool.h>
#include <uv.h>

#include <isc/result.h>
#include <isc/tid.h>

#define UV_VERSION(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/*
 * Copied verbatim from libuv/src/version.c
 */

#define UV_STRINGIFY(v)	       UV_STRINGIFY_HELPER(v)
#define UV_STRINGIFY_HELPER(v) #v

#define UV_VERSION_STRING_BASE         \
	UV_STRINGIFY(UV_VERSION_MAJOR) \
	"." UV_STRINGIFY(UV_VERSION_MINOR) "." UV_STRINGIFY(UV_VERSION_PATCH)

#if UV_VERSION_IS_RELEASE
#define UV_VERSION_STRING UV_VERSION_STRING_BASE
#else
#define UV_VERSION_STRING UV_VERSION_STRING_BASE "-" UV_VERSION_SUFFIX
#endif

#if !defined(UV__ERR)
#define UV__ERR(x) (-(x))
#endif

/*
 * These are used with all versions of libuv:
 */

#define UV_RUNTIME_CHECK(func, ret)                                      \
	if (ret != 0) {                                                  \
		FATAL_ERROR("%s failed: %s\n", #func, uv_strerror(ret)); \
	}

#define isc_uverr2result(x) \
	isc__uverr2result(x, true, __FILE__, __LINE__, __func__)
isc_result_t
isc__uverr2result(int uverr, bool dolog, const char *file, unsigned int line,
		  const char *func);
/*%<
 * Convert a libuv error value into an isc_result_t.  The
 * list of supported error values is not complete; new users
 * of this function should add any expected errors that are
 * not already there.
 */

/**
 * Type-casting helpers
 */

#define uv_handle_set_data(handle, data) \
	uv_handle_set_data((uv_handle_t *)(handle), (data))
#define uv_handle_get_data(handle) uv_handle_get_data((uv_handle_t *)(handle))
#define uv_close(handle, close_cb) uv_close((uv_handle_t *)handle, close_cb)

#if UV_TRACE_INIT

#define uv_idle_init(loop, idle)                                          \
	({                                                                \
		int __r = uv_idle_init(loop, idle);                       \
		fprintf(stderr, "%" PRIu32 ":%s_:uv_idle_init(%p, %p)\n", \
			isc_tid(), __func__, loop, idle);                 \
		__r;                                                      \
	})

#define uv_timer_init(loop, timer)                                         \
	({                                                                 \
		int __r = uv_timer_init(loop, timer);                      \
		fprintf(stderr, "%" PRIu32 ":%s_:uv_timer_init(%p, %p)\n", \
			isc_tid(), __func__, loop, timer);                 \
		__r;                                                       \
	})

#define uv_async_init(loop, async, async_cb)                                   \
	({                                                                     \
		int __r = uv_async_init(loop, async, async_cb);                \
		fprintf(stderr, "%" PRIu32 ":%s_:uv_timer_init(%p, %p, %p)\n", \
			isc_tid(), __func__, loop, async, async_cb);           \
		__r;                                                           \
	})

#define uv_close(handle, close_cb)                                    \
	({                                                            \
		uv_close(handle, close_cb);                           \
		fprintf(stderr, "%" PRIu32 ":%s_:uv_close(%p, %p)\n", \
			isc_tid(), __func__, handle, close_cb);       \
	})

#endif

/*
 * Internal
 */

void
isc__uv_initialize(void);
void
isc__uv_shutdown(void);
void
isc__uv_setdestroycheck(bool check);
