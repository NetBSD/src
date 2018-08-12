/*	$NetBSD: resconf.h,v 1.1.1.1 2018/08/12 12:08:29 christos Exp $	*/

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


#ifndef IRS_RESCONF_H
#define IRS_RESCONF_H 1

/*! \file
 *
 * \brief
 * The IRS resconf module parses the legacy "/etc/resolv.conf" file and
 * creates the corresponding configuration objects for the DNS library
 * modules.
 */

#include <irs/types.h>

/*%
 * A DNS search list specified in the 'domain' or 'search' statements
 * in the "resolv.conf" file.
 */
typedef struct irs_resconf_search {
	char					*domain;
	ISC_LINK(struct irs_resconf_search)	link;
} irs_resconf_search_t;

typedef ISC_LIST(irs_resconf_search_t) irs_resconf_searchlist_t;

ISC_LANG_BEGINDECLS

isc_result_t
irs_resconf_load(isc_mem_t *mctx, const char *filename, irs_resconf_t **confp);
/*%<
 * Load the resolver configuration file 'filename' in the "resolv.conf" format,
 * and create a new irs_resconf_t object from the configuration.  If the file
 * is not found ISC_R_FILENOTFOUND is returned with the structure initialized
 * as if file contained only:
 *
 *	nameserver ::1
 *	nameserver 127.0.0.1
 *
 * Notes:
 *
 *\li	Currently, only the following options are supported:
 *	nameserver, domain, search, sortlist, ndots, and options.
 *	In addition, 'sortlist' is not actually effective; it's parsed, but
 *	the application cannot use the configuration.
 *
 * Returns:
 * \li	ISC_R_SUCCESS on success
 * \li  ISC_R_FILENOTFOUND if the file was not found. *confp will be valid.
 * \li  other on error.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	'filename' != NULL
 *
 *\li	'confp' != NULL && '*confp' == NULL
 */

void
irs_resconf_destroy(irs_resconf_t **confp);
/*%<
 * Destroy the resconf object.
 *
 * Requires:
 *
 *\li	'*confp' is a valid resconf object.
 *
 * Ensures:
 *
 *\li	*confp == NULL
 */

isc_sockaddrlist_t *
irs_resconf_getnameservers(irs_resconf_t *conf);
/*%<
 * Return a list of name server addresses stored in 'conf'.
 *
 * Requires:
 *
 *\li	'conf' is a valid resconf object.
 */

irs_resconf_searchlist_t *
irs_resconf_getsearchlist(irs_resconf_t *conf);
/*%<
 * Return the search list stored in 'conf'.
 *
 * Requires:
 *
 *\li	'conf' is a valid resconf object.
 */

unsigned int
irs_resconf_getndots(irs_resconf_t *conf);
/*%<
 * Return the 'ndots' value stored in 'conf'.
 *
 * Requires:
 *
 *\li	'conf' is a valid resconf object.
 */

ISC_LANG_ENDDECLS

#endif /* IRS_RESCONF_H */
