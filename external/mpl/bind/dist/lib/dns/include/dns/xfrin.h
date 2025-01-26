/*	$NetBSD: xfrin.h,v 1.8 2025/01/26 16:25:29 christos Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file dns/xfrin.h
 * \brief
 * Incoming zone transfers (AXFR + IXFR).
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/refcount.h>
#include <isc/sockaddr.h>
#include <isc/tls.h>

#include <dns/transport.h>
#include <dns/types.h>

/* Add -DDNS_XFRIN_TRACE=1 to CFLAGS for detailed reference tracing */

/***
 *** Types
 ***/

/*%
 * A transfer in progress.  This is an opaque type.
 */
typedef struct dns_xfrin dns_xfrin_t;

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

void
dns_xfrin_create(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		 const isc_sockaddr_t *primaryaddr,
		 const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
		 dns_transport_type_t soa_transport_type,
		 dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
		 isc_mem_t *mctx, dns_xfrin_t **xfrp);
/*%<
 * Create an incoming zone transfer object of 'zone' from
 * 'primaryaddr'.  Attach '*xfrp' to the newly created object.
 *
 * Requires:
 *\li	'xfrp' != NULL and '*xfrp' == NULL.
 *
 *\li	'primaryaddr' has a non-zero port number.
 *
 *\li	'zone' is a valid zone and is associated with a view.
 *
 *\li	'xfrtype' is dns_rdatatype_axfr, dns_rdatatype_ixfr
 *	or dns_rdatatype_soa (soa query followed by axfr if
 *	serial is greater than current serial).
 *
 *\li	If 'xfrtype' is dns_rdatatype_ixfr or dns_rdatatype_soa,
 *	the zone has a database.
 *
 *\li	'soa_transport_type' is DNS_TRANSPORT_NONE if 'xfrtype'
 *	is dns_rdatatype_soa (because in that case the SOA request
 *	will use the same transport as the XFR), or when there is no
 *	preceding SOA request. Otherwise, it should indicate the
 *	transport type used for the SOA request performed by the
 *	caller itself.
 */

isc_result_t
dns_xfrin_start(dns_xfrin_t *xfr, dns_xfrindone_t done);
/*%<
 * Attempt to start an incoming zone transfer of 'xfr->zone'
 * using the previously created '*xfr' object.
 *
 * Iff ISC_R_SUCCESS is returned, '*done' is called with
 * 'zone' and a result code as arguments when the transfer finishes.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t object and is associated with a zone.
 *
 *\li	'done' != NULL.
 *
 */

isc_time_t
dns_xfrin_getstarttime(dns_xfrin_t *xfr);
/*%<
 * Get the start time of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	Transfer start time
 *
 */

void
dns_xfrin_getstate(const dns_xfrin_t *xfr, const char **statestr,
		   bool *is_first_data_received, bool *is_ixfr);
/*%<
 * Get the current state of the xfrin object as a character string, and whether
 * it's currently known to be an IXFR transfer as a boolean value.
 *
 * Notes:
 *\li	The 'is_ixfr' value is valid only if 'is_first_data_received' is true.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 */

uint32_t
dns_xfrin_getendserial(dns_xfrin_t *xfr);
/*%<
 * Get the 'end_serial' of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	Serial number of the new version zone (if it's already known), or 0.
 *
 */

void
dns_xfrin_getstats(dns_xfrin_t *xfr, unsigned int *nmsgp, unsigned int *nrecsp,
		   uint64_t *nbytesp);
/*%<
 * Get various statistics values of the xfrin object: number of the received
 * messages, number of the received records, number of the received bytes.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 */

const isc_sockaddr_t *
dns_xfrin_getsourceaddr(const dns_xfrin_t *xfr);
/*%<
 * Get the source socket address of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	const pointer to the zone transfer's source socket address
 */

const isc_sockaddr_t *
dns_xfrin_getprimaryaddr(const dns_xfrin_t *xfr);
/*%<
 * Get the socket address of the primary server of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	const pointer to the zone transfer's primary server's socket address
 */

dns_transport_type_t
dns_xfrin_gettransporttype(const dns_xfrin_t *xfr);
/*%<
 * Get the zone transfer's trnasport type of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	const pointer to the zone transfer's transport
 *
 */

dns_transport_type_t
dns_xfrin_getsoatransporttype(dns_xfrin_t *xfr);
/*%<
 * Get the SOA request's trnasport type of the xfrin object.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	const pointer to the zone transfer's transport
 *
 */

const dns_name_t *
dns_xfrin_gettsigkeyname(const dns_xfrin_t *xfr);
/*%<
 * Get the name of the xfrin object's TSIG key.
 *
 * Requires:
 *\li	'xfr' is a valid dns_xfrin_t.
 *
 * Returns:
 *\li	const pointer to the zone transfer's TSIG key's name or NULL
 *
 */

void
dns_xfrin_shutdown(dns_xfrin_t *xfr);
/*%<
 * If the zone transfer 'xfr' has already finished,
 * do nothing.  Otherwise, abort it and cause it to call
 * its done callback with a status of ISC_R_CANCELED.
 */

#if DNS_XFRIN_TRACE
#define dns_xfrin_ref(ptr)   dns_xfrin__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_xfrin_unref(ptr) dns_xfrin__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_xfrin_attach(ptr, ptrp) \
	dns_xfrin__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_xfrin_detach(ptrp) \
	dns_xfrin__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_xfrin);
#else
ISC_REFCOUNT_DECL(dns_xfrin);
#endif
ISC_LANG_ENDDECLS
