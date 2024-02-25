/*	$NetBSD: uv_wrap.h,v 1.2.2.2 2024/02/25 15:47:53 martin Exp $	*/

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

#if HAVE_CMOCKA
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include <isc/atomic.h>

#define UNIT_TESTING
#include <cmocka.h>

#include "../netmgr/uv-compat.h"

/* uv_udp_t */

int
__wrap_uv_udp_open(uv_udp_t *handle, uv_os_sock_t sock);
int
__wrap_uv_udp_bind(uv_udp_t *handle, const struct sockaddr *addr,
		   unsigned int flags);
#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
int
__wrap_uv_udp_connect(uv_udp_t *handle, const struct sockaddr *addr);
int
__wrap_uv_udp_getpeername(const uv_udp_t *handle, struct sockaddr *name,
			  int *namelen);
#endif /* UV_VERSION_HEX >= UV_VERSION(1, 27, 0) */
int
__wrap_uv_udp_getsockname(const uv_udp_t *handle, struct sockaddr *name,
			  int *namelen);
int
__wrap_uv_udp_send(uv_udp_send_t *req, uv_udp_t *handle, const uv_buf_t bufs[],
		   unsigned int nbufs, const struct sockaddr *addr,
		   uv_udp_send_cb send_cb);
int
__wrap_uv_udp_recv_start(uv_udp_t *handle, uv_alloc_cb alloc_cb,
			 uv_udp_recv_cb recv_cb);
int
__wrap_uv_udp_recv_stop(uv_udp_t *handle);

/* uv_tcp_t */
int
__wrap_uv_tcp_open(uv_tcp_t *handle, uv_os_sock_t sock);
int
__wrap_uv_tcp_bind(uv_tcp_t *handle, const struct sockaddr *addr,
		   unsigned int flags);
int
__wrap_uv_tcp_getsockname(const uv_tcp_t *handle, struct sockaddr *name,
			  int *namelen);
int
__wrap_uv_tcp_getpeername(const uv_tcp_t *handle, struct sockaddr *name,
			  int *namelen);
int
__wrap_uv_tcp_connect(uv_connect_t *req, uv_tcp_t *handle,
		      const struct sockaddr *addr, uv_connect_cb cb);

/* uv_stream_t */
int
__wrap_uv_listen(uv_stream_t *stream, int backlog, uv_connection_cb cb);
int
__wrap_uv_accept(uv_stream_t *server, uv_stream_t *client);

/* uv_handle_t */
int
__wrap_uv_send_buffer_size(uv_handle_t *handle, int *value);
int
__wrap_uv_recv_buffer_size(uv_handle_t *handle, int *value);
int
__wrap_uv_fileno(const uv_handle_t *handle, uv_os_fd_t *fd);

/* uv_timer_t */
/* FIXME */
/*
 * uv_timer_init
 * uv_timer_start
 */

static atomic_int __state_uv_udp_open = 0;

int
__wrap_uv_udp_open(uv_udp_t *handle, uv_os_sock_t sock) {
	if (atomic_load(&__state_uv_udp_open) == 0) {
		return (uv_udp_open(handle, sock));
	}
	return (atomic_load(&__state_uv_udp_open));
}

static atomic_int __state_uv_udp_bind = 0;

int
__wrap_uv_udp_bind(uv_udp_t *handle, const struct sockaddr *addr,
		   unsigned int flags) {
	if (atomic_load(&__state_uv_udp_bind) == 0) {
		return (uv_udp_bind(handle, addr, flags));
	}
	return (atomic_load(&__state_uv_udp_bind));
}

static atomic_int __state_uv_udp_connect __attribute__((unused)) = 0;

#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
int
__wrap_uv_udp_connect(uv_udp_t *handle, const struct sockaddr *addr) {
	if (atomic_load(&__state_uv_udp_connect) == 0) {
		return (uv_udp_connect(handle, addr));
	}
	return (atomic_load(&__state_uv_udp_connect));
}
#endif /* UV_VERSION_HEX >= UV_VERSION(1, 27, 0) */

static atomic_int __state_uv_udp_getpeername __attribute__((unused)) = 0;

#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
int
__wrap_uv_udp_getpeername(const uv_udp_t *handle, struct sockaddr *name,
			  int *namelen) {
	if (atomic_load(&__state_uv_udp_getpeername) == 0) {
		return (uv_udp_getpeername(handle, name, namelen));
	}
	return (atomic_load(&__state_uv_udp_getpeername));
}
#endif /* UV_VERSION_HEX >= UV_VERSION(1, 27, 0) */

static atomic_int __state_uv_udp_getsockname = 0;
int
__wrap_uv_udp_getsockname(const uv_udp_t *handle, struct sockaddr *name,
			  int *namelen) {
	if (atomic_load(&__state_uv_udp_getsockname) == 0) {
		return (uv_udp_getsockname(handle, name, namelen));
	}
	return (atomic_load(&__state_uv_udp_getsockname));
}

static atomic_int __state_uv_udp_send = 0;
int
__wrap_uv_udp_send(uv_udp_send_t *req, uv_udp_t *handle, const uv_buf_t bufs[],
		   unsigned int nbufs, const struct sockaddr *addr,
		   uv_udp_send_cb send_cb) {
	if (atomic_load(&__state_uv_udp_send) == 0) {
		return (uv_udp_send(req, handle, bufs, nbufs, addr, send_cb));
	}
	return (atomic_load(&__state_uv_udp_send));
}

static atomic_int __state_uv_udp_recv_start = 0;
int
__wrap_uv_udp_recv_start(uv_udp_t *handle, uv_alloc_cb alloc_cb,
			 uv_udp_recv_cb recv_cb) {
	if (atomic_load(&__state_uv_udp_recv_start) == 0) {
		return (uv_udp_recv_start(handle, alloc_cb, recv_cb));
	}
	return (atomic_load(&__state_uv_udp_recv_start));
}

static atomic_int __state_uv_udp_recv_stop = 0;
int
__wrap_uv_udp_recv_stop(uv_udp_t *handle) {
	if (atomic_load(&__state_uv_udp_recv_stop) == 0) {
		return (uv_udp_recv_stop(handle));
	}
	return (atomic_load(&__state_uv_udp_recv_stop));
}

static atomic_int __state_uv_tcp_open = 0;
int
__wrap_uv_tcp_open(uv_tcp_t *handle, uv_os_sock_t sock) {
	if (atomic_load(&__state_uv_tcp_open) == 0) {
		return (uv_tcp_open(handle, sock));
	}
	return (atomic_load(&__state_uv_tcp_open));
}

static atomic_int __state_uv_tcp_bind = 0;
int
__wrap_uv_tcp_bind(uv_tcp_t *handle, const struct sockaddr *addr,
		   unsigned int flags) {
	if (atomic_load(&__state_uv_tcp_bind) == 0) {
		return (uv_tcp_bind(handle, addr, flags));
	}
	return (atomic_load(&__state_uv_tcp_bind));
}

static atomic_int __state_uv_tcp_getsockname = 0;
int
__wrap_uv_tcp_getsockname(const uv_tcp_t *handle, struct sockaddr *name,
			  int *namelen) {
	if (atomic_load(&__state_uv_tcp_getsockname) == 0) {
		return (uv_tcp_getsockname(handle, name, namelen));
	}
	return (atomic_load(&__state_uv_tcp_getsockname));
}

static atomic_int __state_uv_tcp_getpeername = 0;
int
__wrap_uv_tcp_getpeername(const uv_tcp_t *handle, struct sockaddr *name,
			  int *namelen) {
	if (atomic_load(&__state_uv_tcp_getpeername) == 0) {
		return (uv_tcp_getpeername(handle, name, namelen));
	}
	return (atomic_load(&__state_uv_tcp_getpeername));
}

static atomic_int __state_uv_tcp_connect = 0;
int
__wrap_uv_tcp_connect(uv_connect_t *req, uv_tcp_t *handle,
		      const struct sockaddr *addr, uv_connect_cb cb) {
	if (atomic_load(&__state_uv_tcp_connect) == 0) {
		return (uv_tcp_connect(req, handle, addr, cb));
	}
	return (atomic_load(&__state_uv_tcp_connect));
}

static atomic_int __state_uv_listen = 0;
int
__wrap_uv_listen(uv_stream_t *stream, int backlog, uv_connection_cb cb) {
	if (atomic_load(&__state_uv_listen) == 0) {
		return (uv_listen(stream, backlog, cb));
	}
	return (atomic_load(&__state_uv_listen));
}

static atomic_int __state_uv_accept = 0;
int
__wrap_uv_accept(uv_stream_t *server, uv_stream_t *client) {
	if (atomic_load(&__state_uv_accept) == 0) {
		return (uv_accept(server, client));
	}
	return (atomic_load(&__state_uv_accept));
}

static atomic_int __state_uv_send_buffer_size = 0;
int
__wrap_uv_send_buffer_size(uv_handle_t *handle, int *value) {
	if (atomic_load(&__state_uv_send_buffer_size) == 0) {
		return (uv_send_buffer_size(handle, value));
	}
	return (atomic_load(&__state_uv_send_buffer_size));
}

static atomic_int __state_uv_recv_buffer_size = 0;
int
__wrap_uv_recv_buffer_size(uv_handle_t *handle, int *value) {
	if (atomic_load(&__state_uv_recv_buffer_size) == 0) {
		return (uv_recv_buffer_size(handle, value));
	}
	return (atomic_load(&__state_uv_recv_buffer_size));
}

static atomic_int __state_uv_fileno = 0;
int
__wrap_uv_fileno(const uv_handle_t *handle, uv_os_fd_t *fd) {
	if (atomic_load(&__state_uv_fileno) == 0) {
		return (uv_fileno(handle, fd));
	}
	return (atomic_load(&__state_uv_fileno));
}

#define uv_udp_open(...) __wrap_uv_udp_open(__VA_ARGS__)
#define uv_udp_bind(...) __wrap_uv_udp_bind(__VA_ARGS__)
#if UV_VERSION_HEX >= UV_VERSION(1, 27, 0)
#define uv_udp_connect(...)	__wrap_uv_udp_connect(__VA_ARGS__)
#define uv_udp_getpeername(...) __wrap_uv_udp_getpeername(__VA_ARGS__)
#endif /* UV_VERSION_HEX >= UV_VERSION(1, 27, 0) */
#define uv_udp_getsockname(...) __wrap_uv_udp_getsockname(__VA_ARGS__)
#define uv_udp_send(...)	__wrap_uv_udp_send(__VA_ARGS__)
#define uv_udp_recv_start(...)	__wrap_uv_udp_recv_start(__VA_ARGS__)
#define uv_udp_recv_stop(...)	__wrap_uv_udp_recv_stop(__VA_ARGS__)

#define uv_tcp_open(...)	__wrap_uv_tcp_open(__VA_ARGS__)
#define uv_tcp_bind(...)	__wrap_uv_tcp_bind(__VA_ARGS__)
#define uv_tcp_getsockname(...) __wrap_uv_tcp_getsockname(__VA_ARGS__)
#define uv_tcp_getpeername(...) __wrap_uv_tcp_getpeername(__VA_ARGS__)
#define uv_tcp_connect(...)	__wrap_uv_tcp_connect(__VA_ARGS__)

#define uv_listen(...) __wrap_uv_listen(__VA_ARGS__)
#define uv_accept(...) __wrap_uv_accept(__VA_ARGS__)

#define uv_send_buffer_size(...) __wrap_uv_send_buffer_size(__VA_ARGS__)
#define uv_recv_buffer_size(...) __wrap_uv_recv_buffer_size(__VA_ARGS__)
#define uv_fileno(...)		 __wrap_uv_fileno(__VA_ARGS__)

#define RESET_RETURN                                           \
	{                                                      \
		atomic_store(&__state_uv_udp_open, 0);         \
		atomic_store(&__state_uv_udp_bind, 0);         \
		atomic_store(&__state_uv_udp_connect, 0);      \
		atomic_store(&__state_uv_udp_getpeername, 0);  \
		atomic_store(&__state_uv_udp_getsockname, 0);  \
		atomic_store(&__state_uv_udp_send, 0);         \
		atomic_store(&__state_uv_udp_recv_start, 0);   \
		atomic_store(&__state_uv_udp_recv_stop, 0);    \
		atomic_store(&__state_uv_tcp_open, 0);         \
		atomic_store(&__state_uv_tcp_bind, 0);         \
		atomic_store(&__state_uv_tcp_getpeername, 0);  \
		atomic_store(&__state_uv_tcp_getsockname, 0);  \
		atomic_store(&__state_uv_tcp_connect, 0);      \
		atomic_store(&__state_uv_listen, 0);           \
		atomic_store(&__state_uv_accept, 0);           \
		atomic_store(&__state_uv_send_buffer_size, 0); \
		atomic_store(&__state_uv_recv_buffer_size, 0); \
		atomic_store(&__state_uv_fileno, 0);           \
	}

#define WILL_RETURN(func, value) atomic_store(&__state_##func, value)

#endif /* HAVE_CMOCKA */
