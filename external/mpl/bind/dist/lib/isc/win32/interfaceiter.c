/*	$NetBSD: interfaceiter.c,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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


/*
 * Note that this code will need to be revisited to support IPv6 Interfaces.
 * For now we just iterate through IPv4 interfaces.
 */

#include <config.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <isc/interfaceiter.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/strerror.h>
#include <isc/types.h>
#include <isc/util.h>

void InitSockets(void);

/* Common utility functions */

/*
 * Extract the network address part from a "struct sockaddr".
 *
 * The address family is given explicitly
 * instead of using src->sa_family, because the latter does not work
 * for copying a network mask obtained by SIOCGIFNETMASK (it does
 * not have a valid address family).
 */


#define IFITER_MAGIC		0x49464954U	/* IFIT. */
#define VALID_IFITER(t)		((t) != NULL && (t)->magic == IFITER_MAGIC)

struct isc_interfaceiter {
	unsigned int		magic;		/* Magic number. */
	isc_mem_t		*mctx;
	SOCKET			socket;
	INTERFACE_INFO		IFData;		/* Current Interface Info. */
	int			numIF;		/* Current Interface count. */
	int			v4IF;		/* Number of IPv4 Interfaces */
	INTERFACE_INFO		*buf4;		/* Buffer for WSAIoctl data. */
	unsigned int		buf4size;	/* Bytes allocated. */
	INTERFACE_INFO		*pos4;		/* Current offset in IF List */
	SOCKET_ADDRESS_LIST	*buf6;		/* Buffer for WSAIoctl data. */
	unsigned int		buf6size;	/* Bytes allocated. */
	unsigned int		pos6;		/* Which entry to process. */
	isc_boolean_t		v6loop;		/* See IPv6 loop address. */
	isc_boolean_t		pos6zero;	/* Done pos6 == 0. */
	isc_interface_t		current;	/* Current interface data. */
	isc_result_t		result;		/* Last result code. */
};


/*
 * Size of buffer for SIO_GET_INTERFACE_LIST, in number of interfaces.
 * We assume no sane system will have more than than 1K of IP addresses on
 * all of its adapters.
 */
#define IFCONF_SIZE_INITIAL	  16
#define IFCONF_SIZE_INCREMENT	  64
#define IFCONF_SIZE_MAX		1040

static void
get_addr(unsigned int family, isc_netaddr_t *dst, struct sockaddr *src) {
	dst->family = family;
	switch (family) {
	case AF_INET:
		memmove(&dst->type.in,
			&((struct sockaddr_in *) src)->sin_addr,
			sizeof(struct in_addr));
		break;
	case	AF_INET6:
		memmove(&dst->type.in6,
			&((struct sockaddr_in6 *) src)->sin6_addr,
			sizeof(struct in6_addr));
		dst->zone = ((struct sockaddr_in6 *) src)->sin6_scope_id;
		break;
	default:
		INSIST(0);
		break;
	}
}

isc_result_t
isc_interfaceiter_create(isc_mem_t *mctx, isc_interfaceiter_t **iterp) {
	char strbuf[ISC_STRERRORSIZE];
	isc_interfaceiter_t *iter;
	isc_result_t result;
	int error;
	unsigned long bytesReturned = 0;

	REQUIRE(mctx != NULL);
	REQUIRE(iterp != NULL);
	REQUIRE(*iterp == NULL);

	iter = isc_mem_get(mctx, sizeof(*iter));
	if (iter == NULL)
		return (ISC_R_NOMEMORY);

	InitSockets();

	iter->mctx = mctx;
	iter->buf4 = NULL;
	iter->buf6 = NULL;
	iter->pos4 = NULL;
	iter->pos6 = 0;
	iter->v6loop = ISC_TRUE;
	iter->pos6zero = ISC_TRUE;
	iter->buf6size = 0;
	iter->buf4size = 0;
	iter->result = ISC_R_FAILURE;
	iter->numIF = 0;
	iter->v4IF = 0;

	/*
	 * Create an unbound datagram socket to do the
	 * SIO_GET_INTERFACE_LIST WSAIoctl on.
	 */
	iter->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (iter->socket == INVALID_SOCKET) {
		error = WSAGetLastError();
		if (error == WSAEAFNOSUPPORT)
			goto inet6_only;
		isc__strerror(error, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				"making interface scan socket: %s",
				strbuf);
		result = ISC_R_UNEXPECTED;
		goto socket_failure;
	}

	/*
	 * Get the interface configuration, allocating more memory if
	 * necessary.
	 */
	iter->buf4size = IFCONF_SIZE_INITIAL*sizeof(INTERFACE_INFO);

	for (;;) {
		iter->buf4 = isc_mem_get(mctx, iter->buf4size);
		if (iter->buf4 == NULL) {
			result = ISC_R_NOMEMORY;
			goto alloc_failure;
		}

		if (WSAIoctl(iter->socket, SIO_GET_INTERFACE_LIST,
			     0, 0, iter->buf4, iter->buf4size,
			     &bytesReturned, 0, 0) == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			if (error != WSAEFAULT && error != WSAENOBUFS) {
				errno = error;
				isc__strerror(error, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						"get interface configuration: %s",
						strbuf);
				result = ISC_R_UNEXPECTED;
				goto ioctl_failure;
			}
			/*
			 * EINVAL.  Retry with a bigger buffer.
			 */
		} else {
			/*
			 * The WSAIoctl succeeded.
			 * If the number of the returned bytes is the same
			 * as the buffer size, we will grow it just in
			 * case and retry.
			 */
			if (bytesReturned > 0 &&
			    (bytesReturned < iter->buf4size))
				break;
		}
		if (iter->buf4size >= IFCONF_SIZE_MAX*sizeof(INTERFACE_INFO)) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "get interface configuration: "
					 "maximum buffer size exceeded");
			result = ISC_R_UNEXPECTED;
			goto ioctl_failure;
		}
		isc_mem_put(mctx, iter->buf4, iter->buf4size);

		iter->buf4size += IFCONF_SIZE_INCREMENT *
			sizeof(INTERFACE_INFO);
	}

	/*
	 * A newly created iterator has an undefined position
	 * until isc_interfaceiter_first() is called.
	 */
	iter->v4IF = bytesReturned/sizeof(INTERFACE_INFO);

	/* We don't need the socket any more, so close it */
	closesocket(iter->socket);

 inet6_only:
	/*
	 * Create an unbound datagram socket to do the
	 * SIO_ADDRESS_LIST_QUERY WSAIoctl on.
	 */
	iter->socket = socket(AF_INET6, SOCK_DGRAM, 0);
	if (iter->socket == INVALID_SOCKET) {
		error = WSAGetLastError();
		if (error == WSAEAFNOSUPPORT)
			goto inet_only;
		isc__strerror(error, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				"making interface scan socket: %s",
				strbuf);
		result = ISC_R_UNEXPECTED;
		goto ioctl_failure;
	}

	/*
	 * Get the interface configuration, allocating more memory if
	 * necessary.
	 */
	iter->buf6size = sizeof(SOCKET_ADDRESS_LIST) +
			 IFCONF_SIZE_INITIAL*sizeof(SOCKET_ADDRESS);

	for (;;) {
		iter->buf6 = isc_mem_get(mctx, iter->buf6size);
		if (iter->buf6 == NULL) {
			result = ISC_R_NOMEMORY;
			goto ioctl_failure;
		}

		if (WSAIoctl(iter->socket, SIO_ADDRESS_LIST_QUERY,
			     0, 0, iter->buf6, iter->buf6size,
			     &bytesReturned, 0, 0) == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			if (error != WSAEFAULT && error != WSAENOBUFS) {
				errno = error;
				isc__strerror(error, strbuf, sizeof(strbuf));
				UNEXPECTED_ERROR(__FILE__, __LINE__,
						 "sio address list query: %s",
						 strbuf);
				result = ISC_R_UNEXPECTED;
				goto ioctl6_failure;
			}
			/*
			 * EINVAL.  Retry with a bigger buffer.
			 */
		} else
			break;

		if (iter->buf6size >= IFCONF_SIZE_MAX*sizeof(SOCKET_ADDRESS)) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "get interface configuration: "
					 "maximum buffer size exceeded");
			result = ISC_R_UNEXPECTED;
			goto ioctl6_failure;
		}
		isc_mem_put(mctx, iter->buf6, iter->buf6size);

		iter->buf6size += IFCONF_SIZE_INCREMENT *
			sizeof(SOCKET_ADDRESS);
	}

	closesocket(iter->socket);

 inet_only:
	iter->magic = IFITER_MAGIC;
	*iterp = iter;
	return (ISC_R_SUCCESS);

 ioctl6_failure:
	isc_mem_put(mctx, iter->buf6, iter->buf6size);

 ioctl_failure:
	if (iter->buf4 != NULL)
		isc_mem_put(mctx, iter->buf4, iter->buf4size);

 alloc_failure:
	if (iter->socket != INVALID_SOCKET)
		(void) closesocket(iter->socket);

 socket_failure:
	isc_mem_put(mctx, iter, sizeof(*iter));
	return (result);
}

/*
 * Get information about the current interface to iter->current.
 * If successful, return ISC_R_SUCCESS.
 * If the interface has an unsupported address family, or if
 * some operation on it fails, return ISC_R_IGNORE to make
 * the higher-level iterator code ignore it.
 */

static isc_result_t
internal_current(isc_interfaceiter_t *iter) {
	BOOL ifNamed = FALSE;
	unsigned long flags;

	REQUIRE(VALID_IFITER(iter));
	REQUIRE(iter->numIF >= 0);

	memset(&iter->current, 0, sizeof(iter->current));
	iter->current.af = AF_INET;

	get_addr(AF_INET, &iter->current.address,
		 (struct sockaddr *)&(iter->IFData.iiAddress));

	/*
	 * Get interface flags.
	 */

	iter->current.flags = 0;
	flags = iter->IFData.iiFlags;

	if ((flags & IFF_UP) != 0)
		iter->current.flags |= INTERFACE_F_UP;

	if ((flags & IFF_POINTTOPOINT) != 0) {
		iter->current.flags |= INTERFACE_F_POINTTOPOINT;
		snprintf(iter->current.name, sizeof(iter->current.name),
			 "PPP Interface %d", iter->numIF);
		ifNamed = TRUE;
	}

	if ((flags & IFF_LOOPBACK) != 0) {
		iter->current.flags |= INTERFACE_F_LOOPBACK;
		snprintf(iter->current.name, sizeof(iter->current.name),
			 "Loopback Interface %d", iter->numIF);
		ifNamed = TRUE;
	}

	/*
	 * If the interface is point-to-point, get the destination address.
	 */
	if ((iter->current.flags & INTERFACE_F_POINTTOPOINT) != 0) {
		get_addr(AF_INET, &iter->current.dstaddress,
		(struct sockaddr *)&(iter->IFData.iiBroadcastAddress));
	}

	if (ifNamed == FALSE)
		snprintf(iter->current.name, sizeof(iter->current.name),
			"TCP/IP Interface %d", iter->numIF);

	/*
	 * Get the network mask.
	 */
	get_addr(AF_INET, &iter->current.netmask,
		 (struct sockaddr *)&(iter->IFData.iiNetmask));

	return (ISC_R_SUCCESS);
}

static isc_result_t
internal_current6(isc_interfaceiter_t *iter) {
	SOCKET fd;
	int i;

	REQUIRE(VALID_IFITER(iter));
	REQUIRE(iter->buf6 != NULL);

	memset(&iter->current, 0, sizeof(iter->current));
	iter->current.af = AF_INET6;

	if (!iter->pos6zero) {
		if (iter->pos6 == 0U)
			iter->pos6zero = ISC_TRUE;
		get_addr(AF_INET6, &iter->current.address,
			 iter->buf6->Address[iter->pos6].lpSockaddr);

		/*
		 * Set interface flags.
		 */

		iter->current.flags = INTERFACE_F_UP;

		snprintf(iter->current.name, sizeof(iter->current.name),
			 "TCP/IPv6 Interface %u", iter->pos6 + 1);

		for (i = 0; i < 16; i++)
			iter->current.netmask.type.in6.s6_addr[i] = 0xff;
		iter->current.netmask.family = AF_INET6;
		if (IN6_IS_ADDR_LOOPBACK(&iter->current.address.type.in6))
			   iter->v6loop = ISC_TRUE;
	} else {
		/*
		 * See if we can bind to the ::1 and if so return ::1.
		 */
		struct sockaddr_in6 sin6;

		iter->v6loop = ISC_TRUE;	/* So we don't loop forever. */

		fd = socket(AF_INET6, SOCK_DGRAM, 0);
		if (fd == INVALID_SOCKET)
			return (ISC_R_IGNORE);
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_addr.s6_addr[15] = 1;
		if (bind(fd, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
			closesocket(fd);
			return (ISC_R_IGNORE);
		}
		closesocket(fd);

		iter->current.flags = INTERFACE_F_UP | INTERFACE_F_LOOPBACK;
		snprintf(iter->current.name, sizeof(iter->current.name),
			"TCP/IPv6 Loopback Interface");
		for (i = 0; i < 16; i++) {
			if (i != 15)
				iter->current.address.type.in6.s6_addr[i] = 0;
			else
				iter->current.address.type.in6.s6_addr[i] = 1;
			iter->current.netmask.type.in6.s6_addr[i] = 0xff;
		}
		iter->current.address.family = AF_INET6;
		iter->current.netmask.family = AF_INET6;
	}
	return (ISC_R_SUCCESS);
}

/*
 * Step the iterator to the next interface.  Unlike
 * isc_interfaceiter_next(), this may leave the iterator
 * positioned on an interface that will ultimately
 * be ignored.  Return ISC_R_NOMORE if there are no more
 * interfaces, otherwise ISC_R_SUCCESS.
 */
static isc_result_t
internal_next(isc_interfaceiter_t *iter) {
	if (iter->numIF >= iter->v4IF)
		return (ISC_R_NOMORE);

	/*
	 * The first one needs to be set up to point to the last
	 * Element of the array.  Go to the end and back up
	 * Microsoft's implementation is peculiar for returning
	 * the list in reverse order
	 */

	if (iter->numIF == 0)
		iter->pos4 = (INTERFACE_INFO *)(iter->buf4 + (iter->v4IF));

	iter->pos4--;
	if (&(iter->pos4) < &(iter->buf4))
		return (ISC_R_NOMORE);

	memset(&(iter->IFData), 0, sizeof(INTERFACE_INFO));
	memmove(&(iter->IFData), iter->pos4, sizeof(INTERFACE_INFO));
	iter->numIF++;

	return (ISC_R_SUCCESS);
}

static isc_result_t
internal_next6(isc_interfaceiter_t *iter) {
	if (iter->pos6 == 0U && iter->v6loop)
		return (ISC_R_NOMORE);
	if (iter->pos6 != 0U)
		iter->pos6--;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_interfaceiter_current(isc_interfaceiter_t *iter,
			  isc_interface_t *ifdata) {
	REQUIRE(iter->result == ISC_R_SUCCESS);
	memmove(ifdata, &iter->current, sizeof(*ifdata));
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_interfaceiter_first(isc_interfaceiter_t *iter) {

	REQUIRE(VALID_IFITER(iter));

	if (iter->buf6 != NULL) {
		iter->pos6 = iter->buf6->iAddressCount;
		iter->v6loop = ISC_FALSE;
		iter->pos6zero = ISC_TF(iter->pos6 == 0U);
	}
	iter->result = ISC_R_SUCCESS;
	return (isc_interfaceiter_next(iter));
}

isc_result_t
isc_interfaceiter_next(isc_interfaceiter_t *iter) {
	isc_result_t result;

	REQUIRE(VALID_IFITER(iter));
	REQUIRE(iter->result == ISC_R_SUCCESS);

	for (;;) {
		result = internal_next(iter);
		if (result == ISC_R_NOMORE) {
			result = internal_next6(iter);
			if (result != ISC_R_SUCCESS)
				break;
			result = internal_current6(iter);
			if (result == ISC_R_IGNORE)
				continue;
			break;
		} else if (result != ISC_R_SUCCESS)
			break;
		result = internal_current(iter);
		if (result != ISC_R_IGNORE)
			break;
	}
	iter->result = result;
	return (result);
}

void
isc_interfaceiter_destroy(isc_interfaceiter_t **iterp) {
	isc_interfaceiter_t *iter;
	REQUIRE(iterp != NULL);
	iter = *iterp;
	REQUIRE(VALID_IFITER(iter));

	if (iter->buf4 != NULL)
		isc_mem_put(iter->mctx, iter->buf4, iter->buf4size);
	if (iter->buf6 != NULL)
		isc_mem_put(iter->mctx, iter->buf6, iter->buf6size);

	iter->magic = 0;
	isc_mem_put(iter->mctx, iter, sizeof(*iter));
	*iterp = NULL;
}
