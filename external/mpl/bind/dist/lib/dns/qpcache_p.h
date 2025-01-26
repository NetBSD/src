/*	$NetBSD: qpcache_p.h,v 1.2 2025/01/26 16:25:24 christos Exp $	*/

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

#include <isc/heap.h>
#include <isc/lang.h>
#include <isc/urcu.h>

#include <dns/nsec3.h>
#include <dns/qp.h>
#include <dns/rbt.h>
#include <dns/types.h>

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * DNS QPDB Implementation (minimally adapted from RBTDB)
 */

ISC_LANG_BEGINDECLS

isc_result_t
dns__qpcache_create(isc_mem_t *mctx, const dns_name_t *base, dns_dbtype_t type,
		    dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
		    void *driverarg, dns_db_t **dbp);
/*%<
 * Create a new database of type "qpcache". Called via dns_db_create();
 * see documentation for that function for more details.
 *
 * If argv[0] is set, it points to a valid memory context to be used for
 * allocation of heap memory.  Generally this is used for cache databases
 * only.
 *
 * Requires:
 *
 * \li argc == 0 or argv[0] is a valid memory context.
 */
ISC_LANG_ENDDECLS
