/*	$NetBSD: keydata.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_KEYDATA_H
#define DNS_KEYDATA_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/keydata.h
 * \brief
 * KEYDATA utilities.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/types.h>
#include <dns/rdatastruct.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_keydata_todnskey(dns_rdata_keydata_t *keydata,
		     dns_rdata_dnskey_t *dnskey, isc_mem_t *mctx);

isc_result_t
dns_keydata_fromdnskey(dns_rdata_keydata_t *keydata,
		       dns_rdata_dnskey_t *dnskey,
		       isc_uint32_t refresh, isc_uint32_t addhd,
		       isc_uint32_t removehd, isc_mem_t *mctx);

ISC_LANG_ENDDECLS

#endif /* DNS_KEYDATA_H */
