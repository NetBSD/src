/*	$NetBSD: rdatalist_p.h,v 1.1.1.1 2018/08/12 12:08:10 christos Exp $	*/

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


#ifndef DNS_RDATALIST_P_H
#define DNS_RDATALIST_P_H

/*! \file */

#include <isc/result.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

void
isc__rdatalist_disassociate(dns_rdataset_t *rdatasetp);

isc_result_t
isc__rdatalist_first(dns_rdataset_t *rdataset);

isc_result_t
isc__rdatalist_next(dns_rdataset_t *rdataset);

void
isc__rdatalist_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);

void
isc__rdatalist_clone(dns_rdataset_t *source, dns_rdataset_t *target);

unsigned int
isc__rdatalist_count(dns_rdataset_t *rdataset);

isc_result_t
isc__rdatalist_addnoqname(dns_rdataset_t *rdataset, const dns_name_t *name);

isc_result_t
isc__rdatalist_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig);

isc_result_t
isc__rdatalist_addclosest(dns_rdataset_t *rdataset, const dns_name_t *name);

isc_result_t
isc__rdatalist_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig);

void
isc__rdatalist_setownercase(dns_rdataset_t *rdataset, const dns_name_t *name);

void
isc__rdatalist_getownercase(const dns_rdataset_t *rdataset, dns_name_t *name);

ISC_LANG_ENDDECLS

#endif /* DNS_RDATALIST_P_H */
