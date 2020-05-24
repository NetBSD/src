/*	$NetBSD: uv-compat.h,v 1.2 2020/05/24 19:46:27 christos Exp $	*/

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
#include <uv.h>

/*
 * These functions were introduced in newer libuv, but we still
 * want BIND9 compile on older ones so we emulate them.
 * They're inline to avoid conflicts when running with a newer
 * library version.
 */

#ifndef HAVE_UV_HANDLE_GET_DATA
static inline void *
uv_handle_get_data(const uv_handle_t *handle) {
	return (handle->data);
}
#endif /* ifndef HAVE_UV_HANDLE_GET_DATA */

#ifndef HAVE_UV_HANDLE_SET_DATA
static inline void
uv_handle_set_data(uv_handle_t *handle, void *data) {
	handle->data = data;
}
#endif /* ifndef HAVE_UV_HANDLE_SET_DATA */

#ifdef HAVE_UV_IMPORT

#define isc_uv_stream_info_t uv_stream_info_t
#define isc_uv_export	     uv_export
#define isc_uv_import	     uv_import

#else

/*
 * These functions are not available in libuv, but they're very internal
 * to libuv. We should try to get them merged upstream.
 */

/*
 * A sane way to pass listening TCP socket to child threads, without using
 * IPC (as the libuv example shows) but a version of the uv_export() and
 * uv_import() functions that were unfortunately removed from libuv.
 * This is based on the original libuv code.
 */

typedef struct isc_uv_stream_info_s isc_uv_stream_info_t;

struct isc_uv_stream_info_s {
	uv_handle_type type;
#ifdef WIN32
	WSAPROTOCOL_INFOW socket_info;
#else  /* ifdef WIN32 */
	int fd;
#endif /* ifdef WIN32 */
};

int
isc_uv_export(uv_stream_t *stream, isc_uv_stream_info_t *info);
/*%<
 * Exports uv_stream_t as isc_uv_stream_info_t value, which could
 * be used to initialize shared streams within the same process.
 */

int
isc_uv_import(uv_stream_t *stream, isc_uv_stream_info_t *info);
/*%<
 * Imports uv_stream_info_t value into uv_stream_t to initialize a
 * shared stream.
 */

#endif
