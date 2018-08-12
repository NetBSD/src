/*	$NetBSD: portset.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


/*! \file isc/portset.h
 * \brief Transport Protocol Port Manipulation Module
 *
 * This module provides simple utilities to handle a set of transport protocol
 * (UDP or TCP) port numbers, e.g., for creating an ACL list.  An isc_portset_t
 * object is an opaque instance of a port set, for which the user can add or
 * remove a specific port or a range of consecutive ports.  This object is
 * expected to be used as a temporary work space only, and does not protect
 * simultaneous access from multiple threads.  Therefore it must not be stored
 * in a place that can be accessed from multiple threads.
 */

#ifndef ISC_PORTSET_H
#define ISC_PORTSET_H 1

/***
 ***	Imports
 ***/

#include <isc/net.h>

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
isc_portset_create(isc_mem_t *mctx, isc_portset_t **portsetp);
/*%<
 * Create a port set and initialize it as an empty set.
 *
 * Requires:
 *\li	'mctx' to be valid.
 *\li	'portsetp' to be non NULL and '*portsetp' to be NULL;
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
isc_portset_destroy(isc_mem_t *mctx, isc_portset_t **portsetp);
/*%<
 * Destroy a port set.
 *
 * Requires:
 *\li	'mctx' to be valid and must be the same context given when the port set
 *       was created.
 *\li	'*portsetp' to be a valid set.
 */

isc_boolean_t
isc_portset_isset(isc_portset_t *portset, in_port_t port);
/*%<
 * Test whether the given port is stored in the portset.
 *
 * Requires:
 *\li	'portset' to be a valid set.
 *
 * Returns
 * \li	#ISC_TRUE if the port is found, ISC_FALSE otherwise.
 */

unsigned int
isc_portset_nports(isc_portset_t *portset);
/*%<
 * Provides the number of ports stored in the given portset.
 *
 * Requires:
 *\li	'portset' to be a valid set.
 *
 * Returns
 * \li	the number of ports stored in portset.
 */

void
isc_portset_add(isc_portset_t *portset, in_port_t port);
/*%<
 * Add the given port to the portset.  The port may or may not be stored in
 * the portset.
 *
 * Requires:
 *\li	'portlist' to be valid.
 */

void
isc_portset_remove(isc_portset_t *portset, in_port_t port);
/*%<
 * Remove the given port to the portset.  The port may or may not be stored in
 * the portset.
 *
 * Requires:
 *\li	'portlist' to be valid.
 */

void
isc_portset_addrange(isc_portset_t *portset, in_port_t port_lo,
		     in_port_t port_hi);
/*%<
 * Add a subset of [port_lo, port_hi] (inclusive) to the portset.  Ports in the
 * subset may or may not be stored in portset.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	port_lo <= port_hi
 */

void
isc_portset_removerange(isc_portset_t *portset, in_port_t port_lo,
			in_port_t port_hi);
/*%<
 * Subtract a subset of [port_lo, port_hi] (inclusive) from the portset.  Ports
 * in the subset may or may not be stored in portset.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	port_lo <= port_hi
 */

ISC_LANG_ENDDECLS

#endif	/* ISC_PORTSET_H */
