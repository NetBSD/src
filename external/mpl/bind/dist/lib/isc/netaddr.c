/*	$NetBSD: netaddr.c,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

#include <isc/buffer.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/util.h>

bool
isc_netaddr_equal(const isc_netaddr_t *a, const isc_netaddr_t *b) {
	REQUIRE(a != NULL && b != NULL);

	if (a->family != b->family)
		return (false);

	if (a->zone != b->zone)
		return (false);

	switch (a->family) {
	case AF_INET:
		if (a->type.in.s_addr != b->type.in.s_addr)
			return (false);
		break;
	case AF_INET6:
		if (memcmp(&a->type.in6, &b->type.in6,
			   sizeof(a->type.in6)) != 0 ||
		    a->zone != b->zone)
			return (false);
		break;
#ifdef ISC_PLATFORM_HAVESYSUNH
	case AF_UNIX:
		if (strcmp(a->type.un, b->type.un) != 0)
			return (false);
		break;
#endif
	default:
		return (false);
	}
	return (true);
}

bool
isc_netaddr_eqprefix(const isc_netaddr_t *a, const isc_netaddr_t *b,
		     unsigned int prefixlen)
{
	const unsigned char *pa = NULL, *pb = NULL;
	unsigned int ipabytes = 0; /* Length of whole IP address in bytes */
	unsigned int nbytes;       /* Number of significant whole bytes */
	unsigned int nbits;        /* Number of significant leftover bits */

	REQUIRE(a != NULL && b != NULL);

	if (a->family != b->family)
		return (false);

	if (a->zone != b->zone && b->zone != 0)
		return (false);

	switch (a->family) {
	case AF_INET:
		pa = (const unsigned char *) &a->type.in;
		pb = (const unsigned char *) &b->type.in;
		ipabytes = 4;
		break;
	case AF_INET6:
		pa = (const unsigned char *) &a->type.in6;
		pb = (const unsigned char *) &b->type.in6;
		ipabytes = 16;
		break;
	default:
		return (false);
	}

	/*
	 * Don't crash if we get a pattern like 10.0.0.1/9999999.
	 */
	if (prefixlen > ipabytes * 8)
		prefixlen = ipabytes * 8;

	nbytes = prefixlen / 8;
	nbits = prefixlen % 8;

	if (nbytes > 0) {
		if (memcmp(pa, pb, nbytes) != 0)
			return (false);
	}
	if (nbits > 0) {
		unsigned int bytea, byteb, mask;
		INSIST(nbytes < ipabytes);
		INSIST(nbits < 8);
		bytea = pa[nbytes];
		byteb = pb[nbytes];
		mask = (0xFF << (8-nbits)) & 0xFF;
		if ((bytea & mask) != (byteb & mask))
			return (false);
	}
	return (true);
}

isc_result_t
isc_netaddr_totext(const isc_netaddr_t *netaddr, isc_buffer_t *target) {
	char abuf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255")];
	char zbuf[sizeof("%4294967295")];
	unsigned int alen;
	int zlen;
	const char *r;
	const void *type;

	REQUIRE(netaddr != NULL);

	switch (netaddr->family) {
	case AF_INET:
		type = &netaddr->type.in;
		break;
	case AF_INET6:
		type = &netaddr->type.in6;
		break;
#ifdef ISC_PLATFORM_HAVESYSUNH
	case AF_UNIX:
		alen = strlen(netaddr->type.un);
		if (alen > isc_buffer_availablelength(target))
			return (ISC_R_NOSPACE);
		isc_buffer_putmem(target,
				  (const unsigned char *)(netaddr->type.un),
				  alen);
		return (ISC_R_SUCCESS);
#endif
	default:
		return (ISC_R_FAILURE);
	}
	r = inet_ntop(netaddr->family, type, abuf, sizeof(abuf));
	if (r == NULL)
		return (ISC_R_FAILURE);

	alen = strlen(abuf);
	INSIST(alen < sizeof(abuf));

	zlen = 0;
	if (netaddr->family == AF_INET6 && netaddr->zone != 0) {
		zlen = snprintf(zbuf, sizeof(zbuf), "%%%u", netaddr->zone);
		if (zlen < 0)
			return (ISC_R_FAILURE);
		INSIST((unsigned int)zlen < sizeof(zbuf));
	}

	if (alen + zlen > isc_buffer_availablelength(target))
		return (ISC_R_NOSPACE);

	isc_buffer_putmem(target, (unsigned char *)abuf, alen);
	isc_buffer_putmem(target, (unsigned char *)zbuf, (unsigned int)zlen);

	return (ISC_R_SUCCESS);
}

void
isc_netaddr_format(const isc_netaddr_t *na, char *array, unsigned int size) {
	isc_result_t result;
	isc_buffer_t buf;

	isc_buffer_init(&buf, array, size);
	result = isc_netaddr_totext(na, &buf);

	if (size == 0)
		return;

	/*
	 * Null terminate.
	 */
	if (result == ISC_R_SUCCESS) {
		if (isc_buffer_availablelength(&buf) >= 1)
			isc_buffer_putuint8(&buf, 0);
		else
			result = ISC_R_NOSPACE;
	}

	if (result != ISC_R_SUCCESS) {
		snprintf(array, size,
			 "<unknown address, family %u>",
			 na->family);
		array[size - 1] = '\0';
	}
}


isc_result_t
isc_netaddr_prefixok(const isc_netaddr_t *na, unsigned int prefixlen) {
	static const unsigned char zeros[16];
	unsigned int nbits, nbytes, ipbytes = 0;
	const unsigned char *p;

	switch (na->family) {
	case AF_INET:
		p = (const unsigned char *) &na->type.in;
		ipbytes = 4;
		if (prefixlen > 32)
			return (ISC_R_RANGE);
		break;
	case AF_INET6:
		p = (const unsigned char *) &na->type.in6;
		ipbytes = 16;
		if (prefixlen > 128)
			return (ISC_R_RANGE);
		break;
	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
	nbytes = prefixlen / 8;
	nbits = prefixlen % 8;
	if (nbits != 0) {
		INSIST(nbytes < ipbytes);
		if ((p[nbytes] & (0xff>>nbits)) != 0U)
			return (ISC_R_FAILURE);
		nbytes++;
	}
	if (nbytes < ipbytes && memcmp(p + nbytes, zeros, ipbytes - nbytes) != 0)
		return (ISC_R_FAILURE);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_netaddr_masktoprefixlen(const isc_netaddr_t *s, unsigned int *lenp) {
	unsigned int nbits = 0, nbytes = 0, ipbytes = 0, i;
	const unsigned char *p;

	switch (s->family) {
	case AF_INET:
		p = (const unsigned char *) &s->type.in;
		ipbytes = 4;
		break;
	case AF_INET6:
		p = (const unsigned char *) &s->type.in6;
		ipbytes = 16;
		break;
	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
	for (i = 0; i < ipbytes; i++) {
		if (p[i] != 0xFF)
			break;
	}
	nbytes = i;
	if (i < ipbytes) {
		unsigned int c = p[nbytes];
		while ((c & 0x80) != 0 && nbits < 8) {
			c <<= 1; nbits++;
		}
		if ((c & 0xFF) != 0)
			return (ISC_R_MASKNONCONTIG);
		i++;
	}
	for (; i < ipbytes; i++) {
		if (p[i] != 0)
			return (ISC_R_MASKNONCONTIG);
	}
	*lenp = nbytes * 8 + nbits;
	return (ISC_R_SUCCESS);
}

void
isc_netaddr_fromin(isc_netaddr_t *netaddr, const struct in_addr *ina) {
	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_INET;
	netaddr->type.in = *ina;
}

void
isc_netaddr_fromin6(isc_netaddr_t *netaddr, const struct in6_addr *ina6) {
	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_INET6;
	netaddr->type.in6 = *ina6;
}

isc_result_t
isc_netaddr_frompath(isc_netaddr_t *netaddr, const char *path) {
#ifdef ISC_PLATFORM_HAVESYSUNH
	if (strlen(path) > sizeof(netaddr->type.un) - 1)
		return (ISC_R_NOSPACE);

	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_UNIX;
	strlcpy(netaddr->type.un, path, sizeof(netaddr->type.un));
	netaddr->zone = 0;
	return (ISC_R_SUCCESS);
#else
	UNUSED(netaddr);
	UNUSED(path);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}


void
isc_netaddr_setzone(isc_netaddr_t *netaddr, uint32_t zone) {
	/* we currently only support AF_INET6. */
	REQUIRE(netaddr->family == AF_INET6);

	netaddr->zone = zone;
}

uint32_t
isc_netaddr_getzone(const isc_netaddr_t *netaddr) {
	return (netaddr->zone);
}

void
isc_netaddr_fromsockaddr(isc_netaddr_t *t, const isc_sockaddr_t *s) {
	int family = s->type.sa.sa_family;
	t->family = family;
	switch (family) {
	case AF_INET:
		t->type.in = s->type.sin.sin_addr;
		t->zone = 0;
		break;
	case AF_INET6:
		memmove(&t->type.in6, &s->type.sin6.sin6_addr, 16);
		t->zone = s->type.sin6.sin6_scope_id;
		break;
#ifdef ISC_PLATFORM_HAVESYSUNH
	case AF_UNIX:
		memmove(t->type.un, s->type.sunix.sun_path, sizeof(t->type.un));
		t->zone = 0;
		break;
#endif
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
}

void
isc_netaddr_any(isc_netaddr_t *netaddr) {
	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_INET;
	netaddr->type.in.s_addr = INADDR_ANY;
}

void
isc_netaddr_any6(isc_netaddr_t *netaddr) {
	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_INET6;
	netaddr->type.in6 = in6addr_any;
}

void
isc_netaddr_unspec(isc_netaddr_t *netaddr) {
	memset(netaddr, 0, sizeof(*netaddr));
	netaddr->family = AF_UNSPEC;
}

bool
isc_netaddr_ismulticast(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (ISC_IPADDR_ISMULTICAST(na->type.in.s_addr));
	case AF_INET6:
		return (IN6_IS_ADDR_MULTICAST(&na->type.in6));
	default:
		return (false);  /* XXXMLG ? */
	}
}

bool
isc_netaddr_isexperimental(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (ISC_IPADDR_ISEXPERIMENTAL(na->type.in.s_addr));
	default:
		return (false);  /* XXXMLG ? */
	}
}

bool
isc_netaddr_islinklocal(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (false);
	case AF_INET6:
		return (IN6_IS_ADDR_LINKLOCAL(&na->type.in6));
	default:
		return (false);
	}
}

bool
isc_netaddr_issitelocal(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (false);
	case AF_INET6:
		return (IN6_IS_ADDR_SITELOCAL(&na->type.in6));
	default:
		return (false);
	}
}

#define ISC_IPADDR_ISNETZERO(i) \
	       (((uint32_t)(i) & ISC__IPADDR(0xff000000)) \
		== ISC__IPADDR(0x00000000))

bool
isc_netaddr_isnetzero(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (ISC_IPADDR_ISNETZERO(na->type.in.s_addr));
	case AF_INET6:
		return (false);
	default:
		return (false);
	}
}

void
isc_netaddr_fromv4mapped(isc_netaddr_t *t, const isc_netaddr_t *s) {
	isc_netaddr_t *src;

	DE_CONST(s, src);	/* Must come before IN6_IS_ADDR_V4MAPPED. */

	REQUIRE(s->family == AF_INET6);
	REQUIRE(IN6_IS_ADDR_V4MAPPED(&src->type.in6));

	memset(t, 0, sizeof(*t));
	t->family = AF_INET;
	memmove(&t->type.in, (char *)&src->type.in6 + 12, 4);
	return;
}

bool
isc_netaddr_isloopback(const isc_netaddr_t *na) {
	switch (na->family) {
	case AF_INET:
		return (((ntohl(na->type.in.s_addr) & 0xff000000U) ==
			       0x7f000000U));
	case AF_INET6:
		return (IN6_IS_ADDR_LOOPBACK(&na->type.in6));
	default:
		return (false);
	}
}
