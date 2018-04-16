/*	$NetBSD: dhcp4o6.c,v 1.2.2.2 2018/04/16 01:59:46 pgoyette Exp $	*/

/* dhcp4o6.c

   DHCPv4 over DHCPv6 shared code... */

/*
 * Copyright (c) 2016-2017 by Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: dhcp4o6.c,v 1.2.2.2 2018/04/16 01:59:46 pgoyette Exp $");

#include "dhcpd.h"

#ifdef DHCP4o6

int dhcp4o6_fd = -1;
omapi_object_t *dhcp4o6_object = NULL;
omapi_object_type_t *dhcp4o6_type = NULL;

static int dhcp4o6_readsocket(omapi_object_t *);

/*
 * DHCPv4 over DHCPv6 Inter Process Communication setup
 *
 * A UDP socket is created between ::1 port and ::1 port + 1
 * (port is given in network order, the DHCPv6 side is bound to port,
 *  the DHCPv4 side to port + 1. The socket descriptor is stored into
 *  dhcp4o6_fd and an OMAPI handler is registered. Any failure is fatal.)
 */
void dhcp4o6_setup(u_int16_t port) {
	struct sockaddr_in6 local6, remote6;
	int flag;
	isc_result_t status;

	/* Register DHCPv4 over DHCPv6 forwarding. */
	memset(&local6, 0, sizeof(local6));
	local6.sin6_family = AF_INET6;
	if (local_family == AF_INET6)
		local6.sin6_port = port;
	else
		local6.sin6_port = htons(ntohs(port) + 1);
	local6.sin6_addr.s6_addr[15] = 1;
#ifdef HAVE_SA_LEN
	local6.sin6_len = sizeof(local6);
#endif
	memset(&remote6, 0, sizeof(remote6));
	remote6.sin6_family = AF_INET6;
	if (local_family == AF_INET6)
		remote6.sin6_port = htons(ntohs(port) + 1);
	else
		remote6.sin6_port = port;
	remote6.sin6_addr.s6_addr[15] = 1;
#ifdef HAVE_SA_LEN
	remote6.sin6_len = sizeof(remote6);
#endif

	dhcp4o6_fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (dhcp4o6_fd < 0)
		log_fatal("Can't create dhcp4o6 socket: %m");
	flag = 1;
	if (setsockopt(dhcp4o6_fd, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&flag, sizeof(flag)) < 0)
		log_fatal("Can't set SO_REUSEADDR option "
			  "on dhcp4o6 socket: %m");
	if (bind(dhcp4o6_fd,
		 (struct sockaddr *)&local6,
		 sizeof(local6)) < 0)
		log_fatal("Can't bind dhcp4o6 socket: %m");
	if (connect(dhcp4o6_fd,
		    (struct sockaddr *)&remote6,
		    sizeof(remote6)) < 0)
		log_fatal("Can't connect dhcp4o6 socket: %m");

	/* Omapi stuff. */
	/* TODO: add tracing support. */
	status = omapi_object_type_register(&dhcp4o6_type,
					    "dhcp4o6",
					    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					    sizeof(*dhcp4o6_object),
					    0, RC_MISC);
	if (status != ISC_R_SUCCESS)
		log_fatal("Can't register dhcp4o6 type: %s",
			  isc_result_totext(status));
	status = omapi_object_allocate(&dhcp4o6_object, dhcp4o6_type, 0, MDL);
	if (status != ISC_R_SUCCESS)
		log_fatal("Can't allocate dhcp4o6 object: %s",
			  isc_result_totext(status));
	status = omapi_register_io_object(dhcp4o6_object,
					  dhcp4o6_readsocket, 0,
					  dhcpv4o6_handler, 0, 0);
	if (status != ISC_R_SUCCESS)
		log_fatal("Can't register dhcp4o6 handle: %s",
			  isc_result_totext(status));
}

static int dhcp4o6_readsocket(omapi_object_t *h) {
	IGNORE_UNUSED(h);
	return dhcp4o6_fd;
}
#endif /* DHCP4o6 */
