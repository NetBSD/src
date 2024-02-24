/*	$NetBSD: dnsconf.h,v 1.1.2.2 2024/02/24 13:07:18 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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
 * the DNS library, such as trust anchors for DNSSEC validation, and creates
 * the corresponding configuration objects for the DNS library modules.
 *
 * Notes:
 * This module is very experimental and the configuration syntax or library
 * interfaces may change in future versions.  Currently, only static
 * key configuration is supported; "trusted-keys" and "trust-anchors"/
 * "managed-keys" statements will be parsed exactly as they are in
 * named.conf, except that "trust-anchors" and "managed-keys" entries will
 * be treated as if they were configured with "static-key", even if they
 * were actually configured with "initial-key".
 */

#include <irs/types.h>

/*%
 * A compound structure storing DNS key information mainly for DNSSEC
 * validation.  A dns_key_t object will be created using the 'keyname' and
 * 'keydatabuf' members with the dst_key_fromdns() function.
 */
typedef struct irs_dnsconf_dnskey {
	dns_name_t   *keyname;
	isc_buffer_t *keydatabuf;
	ISC_LINK(struct irs_dnsconf_dnskey) link;
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
