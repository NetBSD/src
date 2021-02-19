/*	$NetBSD: uv-compat.h,v 1.3 2021/02/19 16:42:20 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
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

#ifdef HAVE_UV_UDP_CONNECT
#define isc_uv_udp_connect uv_udp_connect
#else

int
isc_uv_udp_connect(uv_udp_t *handle, const struct sockaddr *addr);
/*%<
 * Associate the UDP handle to a remote address and port, so every message sent
 * by this handle is automatically sent to that destination.
 *
 * NOTE: This is just a limited shim for uv_udp_connect() as it requires the
 * handle to be bound.
 */

#endif

int
isc_uv_udp_freebind(uv_udp_t *handle, const struct sockaddr *addr,
		    unsigned int flags);

int
isc_uv_tcp_freebind(uv_tcp_t *handle, const struct sockaddr *addr,
		    unsigned int flags);
