/*	$NetBSD: rootns.h,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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


#ifndef DNS_ROOTNS_H
#define DNS_ROOTNS_H 1

/*! \file dns/rootns.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rootns_create(isc_mem_t *mctx, dns_rdataclass_t rdclass,
		  const char *filename, dns_db_t **target);

void
dns_root_checkhints(dns_view_t *view, dns_db_t *hints, dns_db_t *db);
/*
 * Reports differences between hints and the real roots.
 *
 * Requires view, hints and (cache) db to be valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ROOTNS_H */
