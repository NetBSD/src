/*	$NetBSD: dnsconf.h,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


#ifndef IRS_DNSCONF_H
#define IRS_DNSCONF_H 1

/*! \file
 *
 * \brief
 * The IRS dnsconf module parses an "advanced" configuration file related to
 * the DNS library, such as trusted keys for DNSSEC validation, and creates
 * the corresponding configuration objects for the DNS library modules.
 *
 * Notes:
 * This module is very experimental and the configuration syntax or library
 * interfaces may change in future versions.  Currently, only the
 * 'trusted-keys' statement is supported, whose syntax is the same as the
 * same name of statement for named.conf.
 */

#include <irs/types.h>

/*%
 * A compound structure storing DNS key information mainly for DNSSEC
 * validation.  A dns_key_t object will be created using the 'keyname' and
 * 'keydatabuf' members with the dst_key_fromdns() function.
 */
typedef struct irs_dnsconf_dnskey {
	dns_name_t				*keyname;
	isc_buffer_t				*keydatabuf;
	ISC_LINK(struct irs_dnsconf_dnskey)	link;
} irs_dnsconf_dnskey_t;

typedef ISC_LIST(irs_dnsconf_dnskey_t) irs_dnsconf_dnskeylist_t;

ISC_LANG_BEGINDECLS

isc_result_t
irs_dnsconf_load(isc_mem_t *mctx, const char *filename, irs_dnsconf_t **confp);
/*%<
 * Load the "advanced" DNS configuration file 'filename' in the "dns.conf"
 * format, and create a new irs_dnsconf_t object from the configuration.
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
irs_dnsconf_destroy(irs_dnsconf_t **confp);
/*%<
 * Destroy the dnsconf object.
 *
 * Requires:
 *
 *\li	'*confp' is a valid dnsconf object.
 *
 * Ensures:
 *
 *\li	*confp == NULL
 */

irs_dnsconf_dnskeylist_t *
irs_dnsconf_gettrustedkeys(irs_dnsconf_t *conf);
/*%<
 * Return a list of key information stored in 'conf'.
 *
 * Requires:
 *
 *\li	'conf' is a valid dnsconf object.
 */

ISC_LANG_ENDDECLS

#endif /* IRS_DNSCONF_H */
