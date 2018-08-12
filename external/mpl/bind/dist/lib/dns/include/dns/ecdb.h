/*	$NetBSD: ecdb.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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


#ifndef DNS_ECDB_H
#define DNS_ECDB_H 1

/*****
 ***** Module Info
 *****/

/* TBD */

/***
 *** Imports
 ***/

#include <dns/types.h>

/***
 *** Types
 ***/

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

/* TBD: describe those */

isc_result_t
dns_ecdb_register(isc_mem_t *mctx, dns_dbimplementation_t **dbimp);

void
dns_ecdb_unregister(dns_dbimplementation_t **dbimp);

ISC_LANG_ENDDECLS

#endif /* DNS_ECDB_H */
