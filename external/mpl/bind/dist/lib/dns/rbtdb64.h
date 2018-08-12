/*	$NetBSD: rbtdb64.h,v 1.1.1.1 2018/08/12 12:08:14 christos Exp $	*/

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

/* Id: rbtdb64.h,v 1.17 2007/06/19 23:47:16 tbox Exp  */

#ifndef DNS_RBTDB64_H
#define DNS_RBTDB64_H 1

#include <isc/lang.h>

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * DNS Red-Black Tree DB Implementation with 64-bit version numbers
 */

#include <dns/db.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rbtdb64_create(isc_mem_t *mctx, const dns_name_t *base, dns_dbtype_t type,
		   dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
		   void *driverarg, dns_db_t **dbp);

ISC_LANG_ENDDECLS

#endif /* DNS_RBTDB64_H */
