/*	$NetBSD: keytable.h,v 1.7.2.1 2024/02/25 15:46:57 martin Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * The keytable module provides services for storing and retrieving DNSSEC
 * trusted keys, as well as the ability to find the deepest matching key
 * for a given domain name.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 */

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>
#include <isc/stdtime.h>

#include <dns/rdatastruct.h>
#include <dns/types.h>

#include <dst/dst.h>

ISC_LANG_BEGINDECLS

typedef void (*dns_keytable_callback_t)(const dns_name_t *name, void *fn_arg);

isc_result_t
dns_keytable_create(isc_mem_t *mctx, dns_keytable_t **keytablep);
/*%<
 * Create a keytable.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	keytablep != NULL && *keytablep == NULL
 *
 * Ensures:
 *
 *\li	On success, *keytablep is a valid, empty key table.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

void
dns_keytable_attach(dns_keytable_t *source, dns_keytable_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 *\li	'source' is a valid keytable.
 *
 *\li	'targetp' points to a NULL dns_keytable_t *.
 *
 * Ensures:
 *
 *\li	*targetp is attached to source.
 */

void
dns_keytable_detach(dns_keytable_t **keytablep);
/*%<
 * Detach *keytablep from its keytable.
 *
 * Requires:
 *
 *\li	'keytablep' points to a valid keytable.
 *
 * Ensures:
 *
 *\li	*keytablep is NULL.
 *
 *\li	If '*keytablep' is the last reference to the keytable,
 *		all resources used by the keytable will be freed
 */

isc_result_t
dns_keytable_add(dns_keytable_t *keytable, bool managed, bool initial,
		 dns_name_t *name, dns_rdata_ds_t *ds,
		 dns_keytable_callback_t callback, void *callback_arg);
/*%<
 * Add a key to 'keytable'. The keynode associated with 'name'
 * is updated with the DS specified in 'ds'.
 *
 * The value of keynode->managed is set to 'managed', and the
 * value of keynode->initial is set to 'initial'. (Note: 'initial'
 * should only be used when adding managed-keys from configuration.
 * This indicates the key is in "initializing" state, and has not yet
 * been confirmed with a key refresh query.  Once a key refresh query
 * has validated, we update the keynode with initial == false.)
 *
 * Notes:
 *
 *\li   If the key already exists in the table, adding it again
 *      has no effect and ISC_R_SUCCESS is returned.
 *
 * Requires:
 *
 *\li	'keytable' points to a valid keytable.
 *\li	'ds' is not NULL.
 *\li	if 'initial' is true then 'managed' must also be true.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_EXISTS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_keytable_marksecure(dns_keytable_t *keytable, const dns_name_t *name);
/*%<
 * Add a null key to 'keytable' for name 'name'.  This marks the
 * name as a secure domain, but doesn't supply any key data to allow the
 * domain to be validated.  (Used when automated trust anchor management
 * has gotten broken by a zone misconfiguration; for example, when the
 * active key has been revoked but the stand-by key was still in its 30-day
 * waiting period for validity.)
 *
 * Notes:
 *
 *\li   If a key already exists in the table, ISC_R_EXISTS is
 *      returned and nothing is done.
 *
 * Requires:
 *
 *\li	'keytable' points to a valid keytable.
 *
 *\li	keyp != NULL && *keyp is a valid dst_key_t *.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_EXISTS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_keytable_delete(dns_keytable_t *keytable, const dns_name_t *keyname,
		    dns_keytable_callback_t callback, void *callback_arg);
/*%<
 * Delete all trust anchors from 'keytable' matching name 'keyname'
 *
 * Requires:
 *
 *\li	'keytable' points to a valid keytable.
 *
 *\li	'name' is not NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_keytable_deletekey(dns_keytable_t *keytable, const dns_name_t *keyname,
		       dns_rdata_dnskey_t *dnskey);
/*%<
 * Remove the trust anchor matching the name 'keyname' and the DNSKEY
 * rdata struct 'dnskey' from 'keytable'.
 *
 * Requires:
 *
 *\li	'keytable' points to a valid keytable.
 *\li	'dnskey' is not NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result indicates failure.
 */

isc_result_t
dns_keytable_find(dns_keytable_t *keytable, const dns_name_t *keyname,
		  dns_keynode_t **keynodep);
/*%<
 * Search for the first instance of a trust anchor named 'name' in
 * 'keytable', without regard to keyid and algorithm.
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	keynodep != NULL && *keynodep == NULL
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

isc_result_t
dns_keytable_finddeepestmatch(dns_keytable_t *keytable, const dns_name_t *name,
			      dns_name_t *foundname);
/*%<
 * Search for the deepest match of 'name' in 'keytable'.
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	'foundname' is a name with a dedicated buffer.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOTFOUND
 *
 *\li	Any other result indicates an error.
 */

void
dns_keytable_detachkeynode(dns_keytable_t *keytable, dns_keynode_t **keynodep);
/*%<
 * Detach a keynode found via dns_keytable_find().
 *
 * Requires:
 *
 *\li	*keynodep is a valid keynode returned by a call to dns_keytable_find().
 *
 * Ensures:
 *
 *\li	*keynodep == NULL
 */

isc_result_t
dns_keytable_issecuredomain(dns_keytable_t *keytable, const dns_name_t *name,
			    dns_name_t *foundname, bool *wantdnssecp);
/*%<
 * Is 'name' at or beneath a trusted key?
 *
 * Requires:
 *
 *\li	'keytable' is a valid keytable.
 *
 *\li	'name' is a valid absolute name.
 *
 *\li	'foundanme' is NULL or is a pointer to an initialized dns_name_t
 *
 *\li	'*wantsdnssecp' is a valid bool.
 *
 * Ensures:
 *
 *\li	On success, *wantsdnssecp will be true if and only if 'name'
 *	is at or beneath a trusted key.  If 'foundname' is not NULL, then
 *	it will be updated to contain the name of the closest enclosing
 *	trust anchor.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Any other result is an error.
 */

isc_result_t
dns_keytable_dump(dns_keytable_t *keytable, FILE *fp);
/*%<
 * Dump the keytable on fp.
 */

isc_result_t
dns_keytable_totext(dns_keytable_t *keytable, isc_buffer_t **buf);
/*%<
 * Dump the keytable to buffer at 'buf'
 */

bool
dns_keynode_dsset(dns_keynode_t *keynode, dns_rdataset_t *rdataset);
/*%<
 * Clone the DS RRset associated with 'keynode' into 'rdataset' if
 * it exists.  'dns_rdataset_disassociate(rdataset)' needs to be
 * called when done.
 *
 * Returns:
 *\li	true if there is a DS RRset.
 *\li	false if there isn't DS RRset.
 *
 * Requires:
 *\li	'keynode' is valid.
 *\li	'rdataset' is valid or NULL.
 */

bool
dns_keynode_managed(dns_keynode_t *keynode);
/*%<
 * Is this flagged as a managed key?
 */

bool
dns_keynode_initial(dns_keynode_t *keynode);
/*%<
 * Is this flagged as an initializing key?
 */

void
dns_keynode_trust(dns_keynode_t *keynode);
/*%<
 * Sets keynode->initial to false in order to mark the key as
 * trusted: no longer an initializing key.
 */

isc_result_t
dns_keytable_forall(dns_keytable_t *keytable,
		    void (*func)(dns_keytable_t *, dns_keynode_t *,
				 dns_name_t *, void *),
		    void *arg);
ISC_LANG_ENDDECLS
