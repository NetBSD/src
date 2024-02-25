/*	$NetBSD: order.h,v 1.5.2.1 2024/02/25 15:46:57 martin Exp $	*/

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

/*! \file dns/order.h */

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_order_create(isc_mem_t *mctx, dns_order_t **orderp);
/*%<
 * Create a order object.
 *
 * Requires:
 * \li	'orderp' to be non NULL and '*orderp == NULL'.
 *\li	'mctx' to be valid.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMEMORY
 */

isc_result_t
dns_order_add(dns_order_t *order, const dns_name_t *name,
	      dns_rdatatype_t rdtype, dns_rdataclass_t rdclass,
	      unsigned int mode);
/*%<
 * Add a entry to the end of the order list.
 *
 * Requires:
 * \li	'order' to be valid.
 *\li	'name' to be valid.
 *\li	'mode' to be one of #DNS_RDATASETATTR_RANDOMIZE,
 *		#DNS_RDATASETATTR_FIXEDORDER or zero (#DNS_RDATASETATTR_CYCLIC).
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

unsigned int
dns_order_find(dns_order_t *order, const dns_name_t *name,
	       dns_rdatatype_t rdtype, dns_rdataclass_t rdclass);
/*%<
 * Find the first matching entry on the list.
 *
 * Requires:
 *\li	'order' to be valid.
 *\li	'name' to be valid.
 *
 * Returns the mode set by dns_order_add() or zero.
 */

void
dns_order_attach(dns_order_t *source, dns_order_t **target);
/*%<
 * Attach to the 'source' object.
 *
 * Requires:
 * \li	'source' to be valid.
 *\li	'target' to be non NULL and '*target == NULL'.
 */

void
dns_order_detach(dns_order_t **orderp);
/*%<
 * Detach from the object.  Clean up if last this was the last
 * reference.
 *
 * Requires:
 *\li	'*orderp' to be valid.
 */

ISC_LANG_ENDDECLS
