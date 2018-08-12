/*	$NetBSD: callbacks.h,v 1.1.1.1 2018/08/12 12:08:20 christos Exp $	*/

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


#ifndef DNS_CALLBACKS_H
#define DNS_CALLBACKS_H 1

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

#define DNS_CALLBACK_MAGIC	ISC_MAGIC('C','L','L','B')
#define DNS_CALLBACK_VALID(cb)	ISC_MAGIC_VALID(cb, DNS_CALLBACK_MAGIC)

struct dns_rdatacallbacks {
	unsigned int magic;

	/*%
	 * dns_load_master calls this when it has rdatasets to commit.
	 */
	dns_addrdatasetfunc_t add;

	/*%
	 * This is called when reading in a database image from a 'map'
	 * format zone file.
	 */
	dns_deserializefunc_t deserialize;

	/*%
	 * dns_master_load*() call this when loading a raw zonefile,
	 * to pass back information obtained from the file header
	 */
	dns_rawdatafunc_t rawdata;
	dns_zone_t *zone;

	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a error.
	 */
	void	(*error)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * dns_load_master / dns_rdata_fromtext call this to issue a warning.
	 */
	void	(*warn)(struct dns_rdatacallbacks *, const char *, ...);
	/*%
	 * Private data handles for use by the above callback functions.
	 */
	void	*add_private;
	void	*deserialize_private;
	void	*error_private;
	void	*warn_private;
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

#endif /* DNS_CALLBACKS_H */
