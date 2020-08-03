/*	$NetBSD: keymgr.h,v 1.3 2020/08/03 17:23:41 christos Exp $	*/

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

#ifndef DNS_KEYMGR_H
#define DNS_KEYMGR_H 1

/*! \file dns/keymgr.h */

#include <isc/lang.h>
#include <isc/stdtime.h>

#include <dns/types.h>

#include <dst/dst.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_keymgr_run(const dns_name_t *origin, dns_rdataclass_t rdclass,
	       const char *directory, isc_mem_t *mctx,
	       dns_dnsseckeylist_t *keyring, dns_kasp_t *kasp,
	       isc_stdtime_t now, isc_stdtime_t *nexttime);
/*%<
 * Manage keys in 'keylist' and update timing data according to 'kasp' policy.
 * Create new keys for 'origin' if necessary in 'directory'.  Append all such
 * keys, along with use hints gleaned from their metadata, onto 'keylist'.
 *
 * Update key states and store changes back to disk. Store when to run next
 * in 'nexttime'.
 *
 *	Requires:
 *\li		'origin' is a valid FQDN.
 *\li		'mctx' is a valid memory context.
 *\li		'keyring' is not NULL.
 *\li		'kasp' is not NULL.
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		any error returned by dst_key_generate(), isc_dir_open(),
 *		dst_key_to_file(), or dns_dnsseckey_create().
 *
 *	Ensures:
 *\li		On error, keypool is unchanged
 */

void
dns_keymgr_status(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
		  isc_stdtime_t now, char *out, size_t out_len);
/*%<
 * Retrieve the status of given 'kasp' policy and keys in the
 * 'keyring' and store the printable output in the 'out' buffer.
 *
 *	Requires:
 *\li		'kasp' is not NULL.
 *\li		'keyring' is not NULL.
 *\li		'out' is not NULL.
 *
 *	Returns:
 *\li		Printable status in 'out'.
 *
 */

ISC_LANG_ENDDECLS

#endif /* DNS_KEYMGR_H */
