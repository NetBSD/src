/*	$NetBSD: callbacks.h,v 1.7 2025/01/26 16:25:26 christos Exp $	*/

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

/*! \file dns/callbacks.h */

/***
 ***	Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Types
 ***/

#define DNS_CALLBACK_MAGIC     ISC_MAGIC('C', 'L', 'L', 'B')
#define DNS_CALLBACK_VALID(cb) ISC_MAGIC_VALID(cb, DNS_CALLBACK_MAGIC)

struct dns_rdatacallbacks {
	unsigned int magic;

	/*%
	 * dns_load_master calls 'add' when it has an rdataset to add
	 * to the database. If defined, it calls 'setup' before and
	 * 'commit' after adding rdatasets.
	 *
	 * Some database implementations will commit each rdataset as
	 * soon as it's added, in which case 'setup' and 'commit' need
	 * not be defined.  However, other implementations can be
	 * optimized by grouping rdatasets into a transaction; the
	 * setup and commit functions allow this transaction to be
	 * opened and committed.
	 */
	dns_addrdatasetfunc_t add;
	dns_transactionfunc_t setup;
	dns_transactionfunc_t commit;

	/*%
	 * dns_master_load*() call this when loading a raw zonefile,
	 * to pass back information obtained from the file header
	 */
	dns_rawdatafunc_t rawdata;
	dns_zone_t	 *zone;

	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a error.
	 */
	void (*error)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a warning.
	 */
	void (*warn)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * Private data handles for use by the above callback functions.
	 */
	void *add_private;
	void *error_private;
	void *warn_private;
};

/***
 ***	Initialization
 ***/

void
dns_rdatacallbacks_init(dns_rdatacallbacks_t *callbacks);
/*%<
 * Initialize 'callbacks'.
 *
 * \li	'magic' is set to DNS_CALLBACK_MAGIC
 *
 * \li	'error' and 'warn' are set to default callbacks that print the
 *	error message through the DNS library log context.
 *
 *\li	All other elements are initialized to NULL.
 *
 * Requires:
 *  \li    'callbacks' is a valid dns_rdatacallbacks_t,
 */

void
dns_rdatacallbacks_init_stdio(dns_rdatacallbacks_t *callbacks);
/*%<
 * Like dns_rdatacallbacks_init, but logs to stdio.
 */

ISC_LANG_ENDDECLS
