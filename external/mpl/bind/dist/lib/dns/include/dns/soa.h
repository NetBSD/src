/*	$NetBSD: soa.h,v 1.6.2.1 2024/02/25 15:46:58 martin Exp $	*/

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

/*! \file dns/soa.h
 * \brief
 * SOA utilities.
 */

/***
 *** Imports
 ***/

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_SOA_BUFFERSIZE ((2 * DNS_NAME_MAXWIRE) + (4 * 5))

isc_result_t
dns_soa_buildrdata(const dns_name_t *origin, const dns_name_t *contact,
		   dns_rdataclass_t rdclass, uint32_t serial, uint32_t refresh,
		   uint32_t retry, uint32_t expire, uint32_t minimum,
		   unsigned char *buffer, dns_rdata_t *rdata);
/*%<
 * Build the rdata of an SOA record.
 *
 * Requires:
 *\li   buffer  Points to a temporary buffer of at least
 *              DNS_SOA_BUFFERSIZE bytes.
 *\li   rdata   Points to an initialized dns_rdata_t.
 *
 * Ensures:
 *  \li    *rdata       Contains a valid SOA rdata.  The 'data' member
 *  			refers to 'buffer'.
 */

uint32_t
dns_soa_getserial(dns_rdata_t *rdata);
uint32_t
dns_soa_getrefresh(dns_rdata_t *rdata);
uint32_t
dns_soa_getretry(dns_rdata_t *rdata);
uint32_t
dns_soa_getexpire(dns_rdata_t *rdata);
uint32_t
dns_soa_getminimum(dns_rdata_t *rdata);
/*
 * Extract an integer field from the rdata of a SOA record.
 *
 * Requires:
 *	rdata refers to the rdata of a well-formed SOA record.
 */

void
dns_soa_setserial(uint32_t val, dns_rdata_t *rdata);
void
dns_soa_setrefresh(uint32_t val, dns_rdata_t *rdata);
void
dns_soa_setretry(uint32_t val, dns_rdata_t *rdata);
void
dns_soa_setexpire(uint32_t val, dns_rdata_t *rdata);
void
dns_soa_setminimum(uint32_t val, dns_rdata_t *rdata);
/*
 * Change an integer field of a SOA record by modifying the
 * rdata in-place.
 *
 * Requires:
 *	rdata refers to the rdata of a well-formed SOA record.
 */

ISC_LANG_ENDDECLS
