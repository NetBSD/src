/*	$NetBSD: socket.c,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#include <isc/errno.h>
#include <isc/uv.h>

#include "netmgr-int.h"

#define setsockopt_on(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 1 }, sizeof(int))

#define setsockopt_off(socket, level, name) \
	setsockopt(socket, level, name, &(int){ 0 }, sizeof(int))

static isc_result_t
socket_freebind(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Set the IP_FREEBIND (or equivalent option) on the uv_handle.
	 */
#ifdef IP_FREEBIND
	UNUSED(sa_family);
	if (setsockopt_on(fd, IPPROTO_IP, IP_FREEBIND) == -1) {
		return ISC_R_FAILURE;
	}
	return ISC_R_SUCCESS;
#elif defined(IP_BINDANY) || defined(IPV6_BINDANY)
	if (sa_family == AF_INET) {
#if defined(IP_BINDANY)
		if (setsockopt_on(fd, IPPROTO_IP, IP_BINDANY) == -1) {
			return ISC_R_FAILURE;
		}
		return ISC_R_SUCCESS;
#endif
	} else if (sa_family == AF_INET6) {
#if defined(IPV6_BINDANY)
		if (setsockopt_on(fd, IPPROTO_IPV6, IPV6_BINDANY) == -1) {
			return ISC_R_FAILURE;
		}
		return ISC_R_SUCCESS;
#endif
	}
	return ISC_R_NOTIMPLEMENTED;
#elif defined(SO_BINDANY)
	UNUSED(sa_family);
	if (setsockopt_on(fd, SOL_SOCKET, SO_BINDANY) == -1) {
		return ISC_R_FAILURE;
	}
	return ISC_R_SUCCESS;
#else
	UNUSED(fd);
	UNUSED(sa_family);
	return ISC_R_NOTIMPLEMENTED;
#endif
}

int
isc__nm_udp_freebind(uv_udp_t *handle, const struct sockaddr *addr,
		     unsigned int flags) {
	int r;
	uv_os_sock_t fd = -1;

	r = uv_fileno((const uv_handle_t *)handle, (uv_os_fd_t *)&fd);
	if (r < 0) {
		return r;
	}

	r = uv_udp_bind(handle, addr, flags);
	if (r == UV_EADDRNOTAVAIL &&
	    socket_freebind(fd, addr->sa_family) == ISC_R_SUCCESS)
	{
		/*
		 * Retry binding with IP_FREEBIND (or equivalent option) if the
		 * address is not available. This helps with IPv6 tentative
		 * addresses which are reported by the route socket, although
		 * named is not yet able to properly bind to them.
		 */
		r = uv_udp_bind(handle, addr, flags);
	}

	return r;
}

static int
tcp_bind_now(uv_tcp_t *handle, const struct sockaddr *addr,
	     unsigned int flags) {
	int r;
	struct sockaddr_storage sname;
	int snamelen = sizeof(sname);

	r = uv_tcp_bind(handle, addr, flags);
	if (r < 0) {
		return r;
	}

	/*
	 * uv_tcp_bind() uses a delayed error, initially returning
	 * success even if bind() fails. By calling uv_tcp_getsockname()
	 * here we can find out whether the bind() call was successful.
	 */
	r = uv_tcp_getsockname(handle, (struct sockaddr *)&sname, &snamelen);
	if (r < 0) {
		return r;
	}

	return 0;
}

int
isc__nm_tcp_freebind(uv_tcp_t *handle, const struct sockaddr *addr,
		     unsigned int flags) {
	int r;
	uv_os_sock_t fd = -1;

	r = uv_fileno((const uv_handle_t *)handle, (uv_os_fd_t *)&fd);
	if (r < 0) {
		return r;
	}

	r = tcp_bind_now(handle, addr, flags);
	if (r == UV_EADDRNOTAVAIL &&
	    socket_freebind(fd, addr->sa_family) == ISC_R_SUCCESS)
	{
		/*
		 * Retry binding with IP_FREEBIND (or equivalent option) if the
		 * address is not available. This helps with IPv6 tentative
		 * addresses which are reported by the route socket, although
		 * named is not yet able to properly bind to them.
		 */
		r = tcp_bind_now(handle, addr, flags);
	}

	return r;
}

isc_result_t
isc__nm_socket(int domain, int type, int protocol, uv_os_sock_t *sockp) {
	int sock = socket(domain, type, protocol);
	if (sock < 0) {
		return isc_errno_toresult(errno);
	}

	*sockp = (uv_os_sock_t)sock;
	return ISC_R_SUCCESS;
}

void
isc__nm_closesocket(uv_os_sock_t sock) {
	close(sock);
}

isc_result_t
isc__nm_socket_reuse(uv_os_sock_t fd, int val) {
	/*
	 * Generally, the SO_REUSEADDR socket option allows reuse of
	 * local addresses.
	 *
	 * On the BSDs, SO_REUSEPORT implies SO_REUSEADDR but with some
	 * additional refinements for programs that use multicast.
	 *
	 * On Linux, SO_REUSEPORT has different semantics: it _shares_ the port
	 * rather than steal it from the current listener, so we don't use it
	 * here, but rather in isc__nm_socket_reuse_lb().
	 */

#if defined(SO_REUSEPORT) && !defined(__linux__)
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) == -1) {
		return ISC_R_FAILURE;
	}
	return ISC_R_SUCCESS;
#elif defined(SO_REUSEADDR)
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
		return ISC_R_FAILURE;
	}
	return ISC_R_SUCCESS;
#else
	UNUSED(fd);
	return ISC_R_NOTIMPLEMENTED;
#endif
}

isc_result_t
isc__nm_socket_reuse_lb(uv_os_sock_t fd) {
	/*
	 * On FreeBSD 12+, SO_REUSEPORT_LB socket option allows sockets to be
	 * bound to an identical socket address. For UDP sockets, the use of
	 * this option can provide better distribution of incoming datagrams to
	 * multiple processes (or threads) as compared to the traditional
	 * technique of having multiple processes compete to receive datagrams
	 * on the same socket.
	 *
	 * On Linux, the same thing is achieved simply with SO_REUSEPORT.
	 */
#if defined(SO_REUSEPORT_LB)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEPORT_LB) == -1) {
		return ISC_R_FAILURE;
	} else {
		return ISC_R_SUCCESS;
	}
#elif defined(SO_REUSEPORT) && defined(__linux__)
	if (setsockopt_on(fd, SOL_SOCKET, SO_REUSEPORT) == -1) {
		return ISC_R_FAILURE;
	} else {
		return ISC_R_SUCCESS;
	}
#else
	UNUSED(fd);
	return ISC_R_NOTIMPLEMENTED;
#endif
}

isc_result_t
isc__nm_socket_disable_pmtud(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Disable the Path MTU Discovery on IP packets
	 */
	if (sa_family == AF_INET6) {
#if defined(IPV6_DONTFRAG)
		if (setsockopt_off(fd, IPPROTO_IPV6, IPV6_DONTFRAG) == -1) {
			return ISC_R_FAILURE;
		} else {
			return ISC_R_SUCCESS;
		}
#elif defined(IPV6_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		if (setsockopt(fd, IPPROTO_IPV6, IPV6_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_OMIT }, sizeof(int)) == -1)
		{
			return ISC_R_FAILURE;
		} else {
			return ISC_R_SUCCESS;
		}
#else
		UNUSED(fd);
#endif
	} else if (sa_family == AF_INET) {
#if defined(IP_DONTFRAG)
		if (setsockopt_off(fd, IPPROTO_IP, IP_DONTFRAG) == -1) {
			return ISC_R_FAILURE;
		} else {
			return ISC_R_SUCCESS;
		}
#elif defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_OMIT)
		if (setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER,
			       &(int){ IP_PMTUDISC_OMIT }, sizeof(int)) == -1)
		{
			return ISC_R_FAILURE;
		} else {
			return ISC_R_SUCCESS;
		}
#else
		UNUSED(fd);
#endif
	} else {
		return ISC_R_FAMILYNOSUPPORT;
	}

	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
isc__nm_socket_v6only(uv_os_sock_t fd, sa_family_t sa_family) {
	/*
	 * Enable the IPv6-only option on IPv6 sockets
	 */
	if (sa_family == AF_INET6) {
#if defined(IPV6_V6ONLY)
		if (setsockopt_on(fd, IPPROTO_IPV6, IPV6_V6ONLY) == -1) {
			return ISC_R_FAILURE;
		} else {
			return ISC_R_SUCCESS;
		}
#else
		UNUSED(fd);
#endif
	}
	return ISC_R_NOTIMPLEMENTED;
}

isc_result_t
isc__nm_socket_connectiontimeout(uv_os_sock_t fd, int timeout_ms) {
#if defined(TIMEOUT_OPTNAME)
	TIMEOUT_TYPE timeout = timeout_ms / TIMEOUT_DIV;

	if (timeout == 0) {
		timeout = 1;
	}

	if (setsockopt(fd, IPPROTO_TCP, TIMEOUT_OPTNAME, &timeout,
		       sizeof(timeout)) == -1)
	{
		return ISC_R_FAILURE;
	}

	return ISC_R_SUCCESS;
#else
	UNUSED(fd);
	UNUSED(timeout_ms);

	return ISC_R_SUCCESS;
#endif
}

isc_result_t
isc__nm_socket_tcp_nodelay(uv_os_sock_t fd, bool value) {
#ifdef TCP_NODELAY
	int ret;

	if (value) {
		ret = setsockopt_on(fd, IPPROTO_TCP, TCP_NODELAY);
	} else {
		ret = setsockopt_off(fd, IPPROTO_TCP, TCP_NODELAY);
	}

	if (ret == -1) {
		return ISC_R_FAILURE;
	} else {
		return ISC_R_SUCCESS;
	}
#else
	UNUSED(fd);
	return ISC_R_SUCCESS;
#endif
}

isc_result_t
isc__nm_socket_tcp_maxseg(uv_os_sock_t fd, int size) {
#ifdef TCP_MAXSEG
	if (setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, (void *)&size,
		       sizeof(size)))
	{
		return ISC_R_FAILURE;
	} else {
		return ISC_R_SUCCESS;
	}
#else
	UNUSED(fd);
	UNUSED(size);
	return ISC_R_SUCCESS;
#endif
}

isc_result_t
isc__nm_socket_min_mtu(uv_os_sock_t fd, sa_family_t sa_family) {
	if (sa_family != AF_INET6) {
		return ISC_R_SUCCESS;
	}
#ifdef IPV6_USE_MIN_MTU
	if (setsockopt_on(fd, IPPROTO_IPV6, IPV6_USE_MIN_MTU) == -1) {
		return ISC_R_FAILURE;
	}
#elif defined(IPV6_MTU)
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_MTU, &(int){ 1280 },
		       sizeof(int)) == -1)
	{
		return ISC_R_FAILURE;
	}
#else
	UNUSED(fd);
#endif

	return ISC_R_SUCCESS;
}
