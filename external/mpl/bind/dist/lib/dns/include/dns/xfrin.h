/*	$NetBSD: xfrin.h,v 1.6.2.1 2024/02/25 15:46:59 martin Exp $	*/

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
#include <isc/tls.h>

#include <dns/transport.h>
#include <dns/types.h>

/***
 *** Types
 ***/

/*%
 * A transfer in progress.  This is an opaque type.
 */
typedef struct dns_xfrin_ctx dns_xfrin_ctx_t;

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_xfrin_create(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		 const isc_sockaddr_t *primaryaddr,
		 const isc_sockaddr_t *sourceaddr, dns_tsigkey_t *tsigkey,
		 dns_transport_t *transport, isc_tlsctx_cache_t *tlsctx_cache,
		 isc_mem_t *mctx, isc_nm_t *netmgr, dns_xfrindone_t done,
		 dns_xfrin_ctx_t **xfrp);
/*%<
 * Attempt to start an incoming zone transfer of 'zone'
 * from 'primaryaddr', creating a dns_xfrin_ctx_t object to
 * manage it.  Attach '*xfrp' to the newly created object.
 *
 * Iff ISC_R_SUCCESS is returned, '*done' is called with
 * 'zone' and a result code as arguments when the transfer finishes.
 *
 * Requires:
 *\li	'xfrtype' is dns_rdatatype_axfr, dns_rdatatype_ixfr
 *	or dns_rdatatype_soa (soa query followed by axfr if
 *	serial is greater than current serial).
 *
 *\li	If 'xfrtype' is dns_rdatatype_ixfr or dns_rdatatype_soa,
 *	the zone has a database.
 */

void
dns_xfrin_shutdown(dns_xfrin_ctx_t *xfr);
/*%<
 * If the zone transfer 'xfr' has already finished,
 * do nothing.  Otherwise, abort it and cause it to call
 * its done callback with a status of ISC_R_CANCELED.
 */

void
dns_xfrin_detach(dns_xfrin_ctx_t **xfrp);
/*%<
 * Detach a reference to a zone transfer object.
 * Caller to maintain external locking if required.
 */

void
dns_xfrin_attach(dns_xfrin_ctx_t *source, dns_xfrin_ctx_t **target);
/*%<
 * Caller to maintain external locking if required.
 */

ISC_LANG_ENDDECLS
