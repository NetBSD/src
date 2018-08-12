/*	$NetBSD: xfrin.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_XFRIN_H
#define DNS_XFRIN_H 1

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

/*% see dns_xfrin_create2() */
isc_result_t
dns_xfrin_create(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		 const isc_sockaddr_t *masteraddr, dns_tsigkey_t *tsigkey,
		 isc_mem_t *mctx, isc_timermgr_t *timermgr,
		 isc_socketmgr_t *socketmgr, isc_task_t *task,
		 dns_xfrindone_t done, dns_xfrin_ctx_t **xfrp);

isc_result_t
dns_xfrin_create2(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		  const isc_sockaddr_t *masteraddr,
		  const isc_sockaddr_t *sourceaddr,
		  dns_tsigkey_t *tsigkey, isc_mem_t *mctx,
		  isc_timermgr_t *timermgr, isc_socketmgr_t *socketmgr,
		  isc_task_t *task, dns_xfrindone_t done,
		  dns_xfrin_ctx_t **xfrp);

isc_result_t
dns_xfrin_create3(dns_zone_t *zone, dns_rdatatype_t xfrtype,
		  const isc_sockaddr_t *masteraddr,
		  const isc_sockaddr_t *sourceaddr,
		  isc_dscp_t dscp, dns_tsigkey_t *tsigkey, isc_mem_t *mctx,
		  isc_timermgr_t *timermgr, isc_socketmgr_t *socketmgr,
		  isc_task_t *task, dns_xfrindone_t done,
		  dns_xfrin_ctx_t **xfrp);
/*%<
 * Attempt to start an incoming zone transfer of 'zone'
 * from 'masteraddr', creating a dns_xfrin_ctx_t object to
 * manage it.  Attach '*xfrp' to the newly created object.
 *
 * Iff ISC_R_SUCCESS is returned, '*done' is guaranteed to be
 * called in the context of 'task', with 'zone' and a result
 * code as arguments when the transfer finishes.
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

#endif /* DNS_XFRIN_H */
