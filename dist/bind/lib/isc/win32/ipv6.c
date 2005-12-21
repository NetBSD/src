/*	$NetBSD: ipv6.c,v 1.1.1.2 2005/12/21 19:59:11 christos Exp $	*/

/*
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: ipv6.c,v 1.4.2.2 2003/07/22 04:03:50 marka Exp */

#define off_t _off_t

#include <isc/net.h>
#include <isc/platform.h>

#if _MSC_VER < 1300
LIBISC_EXTERNAL_DATA const struct in6_addr in6addr_any =
	IN6ADDR_ANY_INIT;

LIBISC_EXTERNAL_DATA const struct in6_addr in6addr_loopback =
	IN6ADDR_LOOPBACK_INIT;
#endif
