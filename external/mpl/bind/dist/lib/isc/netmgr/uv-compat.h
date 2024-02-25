/*	$NetBSD: uv-compat.h,v 1.5.2.1 2024/02/25 15:47:25 martin Exp $	*/

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
#include <uv.h>

/*
 * These functions were introduced in newer libuv, but we still
 * want BIND9 compile on older ones so we emulate them.
 * They're inline to avoid conflicts when running with a newer
 * library version.
 */

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

#if UV_VERSION_HEX < UV_VERSION(1, 19, 0)
static inline void *
uv_handle_get_data(const uv_handle_t *handle) {
	return (handle->data);
}

static inline void
uv_handle_set_data(uv_handle_t *handle, void *data) {
	handle->data = data;
}

static inline void *
uv_req_get_data(const uv_req_t *req) {
	return (req->data);
}

static inline void
uv_req_set_data(uv_req_t *req, void *data) {
	req->data = data;
}
#endif /* UV_VERSION_HEX < UV_VERSION(1, 19, 0) */

#if UV_VERSION_HEX < UV_VERSION(1, 32, 0)
int
uv_tcp_close_reset(uv_tcp_t *handle, uv_close_cb close_cb);
#endif

#if UV_VERSION_HEX < UV_VERSION(1, 34, 0)
#define uv_sleep(msec) usleep((msec) * 1000)
#endif /* UV_VERSION_HEX < UV_VERSION(1, 34, 0) */

#if UV_VERSION_HEX < UV_VERSION(1, 27, 0)
int
isc_uv_udp_connect(uv_udp_t *handle, const struct sockaddr *addr);
/*%<
 * Associate the UDP handle to a remote address and port, so every message sent
 * by this handle is automatically sent to that destination.
 *
 * NOTE: This is just a limited shim for uv_udp_connect() as it requires the
 * handle to be bound.
 */
#else /* UV_VERSION_HEX < UV_VERSION(1, 27, 0) */
#define isc_uv_udp_connect uv_udp_connect
#endif /* UV_VERSION_HEX < UV_VERSION(1, 27, 0) */

#if UV_VERSION_HEX < UV_VERSION(1, 12, 0)
#include <stdlib.h>
#include <string.h>

static inline int
uv_os_getenv(const char *name, char *buffer, size_t *size) {
	size_t len;
	char *buf = getenv(name);

	if (buf == NULL) {
		return (UV_ENOENT);
	}

	len = strlen(buf) + 1;
	if (len > *size) {
		*size = len;
		return (UV_ENOBUFS);
	}

	*size = len;
	memmove(buffer, buf, len);

	return (0);
}

#define uv_os_setenv(name, value) setenv(name, value, 0)
#endif /* UV_VERSION_HEX < UV_VERSION(1, 12, 0) */

int
isc_uv_udp_freebind(uv_udp_t *handle, const struct sockaddr *addr,
		    unsigned int flags);

int
isc_uv_tcp_freebind(uv_tcp_t *handle, const struct sockaddr *addr,
		    unsigned int flags);
