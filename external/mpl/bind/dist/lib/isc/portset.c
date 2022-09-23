/*	$NetBSD: portset.c,v 1.1.1.5 2022/09/23 12:09:21 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/mem.h>
#include <isc/portset.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#define ISC_PORTSET_BUFSIZE (65536 / (sizeof(uint32_t) * 8))

/*%
 * Internal representation of portset.  It's an array of 32-bit integers, each
 * bit corresponding to a single port in the ascending order.  For example,
 * the second most significant bit of buf[0] corresponds to port 1.
 */
struct isc_portset {
	unsigned int nports; /*%< number of ports in the set */
	uint32_t buf[ISC_PORTSET_BUFSIZE];
};

static bool
portset_isset(isc_portset_t *portset, in_port_t port) {
	return ((portset->buf[port >> 5] & ((uint32_t)1 << (port & 31))) != 0);
}

static void
portset_add(isc_portset_t *portset, in_port_t port) {
	if (!portset_isset(portset, port)) {
		portset->nports++;
		portset->buf[port >> 5] |= ((uint32_t)1 << (port & 31));
	}
}

static void
portset_remove(isc_portset_t *portset, in_port_t port) {
	if (portset_isset(portset, port)) {
		portset->nports--;
		portset->buf[port >> 5] &= ~((uint32_t)1 << (port & 31));
	}
}

isc_result_t
isc_portset_create(isc_mem_t *mctx, isc_portset_t **portsetp) {
	isc_portset_t *portset;

	REQUIRE(portsetp != NULL && *portsetp == NULL);

	portset = isc_mem_get(mctx, sizeof(*portset));

	/* Make the set 'empty' by default */
	memset(portset, 0, sizeof(*portset));
	*portsetp = portset;

	return (ISC_R_SUCCESS);
}

void
isc_portset_destroy(isc_mem_t *mctx, isc_portset_t **portsetp) {
	isc_portset_t *portset;

	REQUIRE(portsetp != NULL);
	portset = *portsetp;

	isc_mem_put(mctx, portset, sizeof(*portset));
}

bool
isc_portset_isset(isc_portset_t *portset, in_port_t port) {
	REQUIRE(portset != NULL);

	return (portset_isset(portset, port));
}

unsigned int
isc_portset_nports(isc_portset_t *portset) {
	REQUIRE(portset != NULL);

	return (portset->nports);
}

void
isc_portset_add(isc_portset_t *portset, in_port_t port) {
	REQUIRE(portset != NULL);

	portset_add(portset, port);
}

void
isc_portset_remove(isc_portset_t *portset, in_port_t port) {
	portset_remove(portset, port);
}

void
isc_portset_addrange(isc_portset_t *portset, in_port_t port_lo,
		     in_port_t port_hi) {
	in_port_t p;

	REQUIRE(portset != NULL);
	REQUIRE(port_lo <= port_hi);

	p = port_lo;
	do {
		portset_add(portset, p);
	} while (p++ < port_hi);
}

void
isc_portset_removerange(isc_portset_t *portset, in_port_t port_lo,
			in_port_t port_hi) {
	in_port_t p;

	REQUIRE(portset != NULL);
	REQUIRE(port_lo <= port_hi);

	p = port_lo;
	do {
		portset_remove(portset, p);
	} while (p++ < port_hi);
}
