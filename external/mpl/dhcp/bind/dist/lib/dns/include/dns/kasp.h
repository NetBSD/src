/*	$NetBSD: kasp.h,v 1.1.2.2 2024/02/24 13:07:05 martin Exp $	*/

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

#ifndef DNS_KASP_H
#define DNS_KASP_H 1

/*****
***** Module Info
*****/

/*! \file dns/kasp.h
 * \brief
 * DNSSEC Key and Signing Policy (KASP)
 *
 * A "kasp" is a DNSSEC policy, that determines how a zone should be
 * signed and maintained.
 */

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/mutex.h>
#include <isc/refcount.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

/* Stores a KASP key */
struct dns_kasp_key {
	isc_mem_t *mctx;

	/* Locked by themselves. */
	isc_refcount_t references;

	/* Under owner's locking control. */
	ISC_LINK(struct dns_kasp_key) link;

	/* Configuration */
	uint32_t lifetime;
	uint8_t	 algorithm;
	int	 length;
	uint8_t	 role;
};

struct dns_kasp_nsec3param {
	uint8_t saltlen;
	uint8_t algorithm;
	uint8_t iterations;
	bool	optout;
};

/* Stores a DNSSEC policy */
struct dns_kasp {
	unsigned int magic;
	isc_mem_t   *mctx;
	char	    *name;

	/* Internals. */
	isc_mutex_t lock;
	bool	    frozen;

	/* Locked by themselves. */
	isc_refcount_t references;

	/* Under owner's locking control. */
	ISC_LINK(struct dns_kasp) link;

	/* Configuration: signatures */
	uint32_t signatures_refresh;
	uint32_t signatures_validity;
	uint32_t signatures_validity_dnskey;

	/* Configuration: Keys */
	dns_kasp_keylist_t keys;
	dns_ttl_t	   dnskey_ttl;

	/* Configuration: Denial of existence */
	bool		      nsec3;
	dns_kasp_nsec3param_t nsec3param;

	/* Configuration: Timings */
	uint32_t publish_safety;
	uint32_t retire_safety;
	uint32_t purge_keys;

	/* Zone settings */
	dns_ttl_t zone_max_ttl;
	uint32_t  zone_propagation_delay;

	/* Parent settings */
	dns_ttl_t parent_ds_ttl;
	uint32_t  parent_propagation_delay;
};

#define DNS_KASP_MAGIC	     ISC_MAGIC('K', 'A', 'S', 'P')
#define DNS_KASP_VALID(kasp) ISC_MAGIC_VALID(kasp, DNS_KASP_MAGIC)

/* Defaults */
#define DNS_KASP_SIG_REFRESH	     (86400 * 5)
#define DNS_KASP_SIG_VALIDITY	     (86400 * 14)
#define DNS_KASP_SIG_VALIDITY_DNSKEY (86400 * 14)
#define DNS_KASP_KEY_TTL	     (3600)
#define DNS_KASP_DS_TTL		     (86400)
#define DNS_KASP_PUBLISH_SAFETY	     (3600)
#define DNS_KASP_PURGE_KEYS	     (86400 * 90)
#define DNS_KASP_RETIRE_SAFETY	     (3600)
#define DNS_KASP_ZONE_MAXTTL	     (86400)
#define DNS_KASP_ZONE_PROPDELAY	     (300)
#define DNS_KASP_PARENT_PROPDELAY    (3600)

/* Key roles */
#define DNS_KASP_KEY_ROLE_KSK 0x01
#define DNS_KASP_KEY_ROLE_ZSK 0x02

isc_result_t
dns_kasp_create(isc_mem_t *mctx, const char *name, dns_kasp_t **kaspp);
/*%<
 * Create a KASP.
 *
 * Requires:
 *
 *\li  'mctx' is a valid memory context.
 *
 *\li  'name' is a valid C string.
 *
 *\li  kaspp != NULL && *kaspp == NULL
 *
 * Returns:
 *
 *\li  #ISC_R_SUCCESS
 *\li  #ISC_R_NOMEMORY
 *
 *\li  Other errors are possible.
 */

void
dns_kasp_attach(dns_kasp_t *source, dns_kasp_t **targetp);
/*%<
 * Attach '*targetp' to 'source'.
 *
 * Requires:
 *
 *\li   'source' is a valid, frozen kasp.
 *
 *\li   'targetp' points to a NULL dns_kasp_t *.
 *
 * Ensures:
 *
 *\li   *targetp is attached to source.
 *
 *\li   While *targetp is attached, the kasp will not shut down.
 */

void
dns_kasp_detach(dns_kasp_t **kaspp);
/*%<
 * Detach KASP.
 *
 * Requires:
 *
 *\li   'kaspp' points to a valid dns_kasp_t *
 *
 * Ensures:
 *
 *\li   *kaspp is NULL.
 */

void
dns_kasp_freeze(dns_kasp_t *kasp);
/*%<
 * Freeze kasp.  No changes can be made to kasp configuration while frozen.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, unfrozen kasp.
 *
 * Ensures:
 *
 *\li   'kasp' is frozen.
 */

void
dns_kasp_thaw(dns_kasp_t *kasp);
/*%<
 * Thaw kasp.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Ensures:
 *
 *\li   'kasp' is no longer frozen.
 */

const char *
dns_kasp_getname(dns_kasp_t *kasp);
/*%<
 * Get kasp name.
 *
 * Requires:
 *
 *\li   'kasp' is a valid kasp.
 *
 * Returns:
 *
 *\li   name of 'kasp'.
 */

uint32_t
dns_kasp_signdelay(dns_kasp_t *kasp);
/*%<
 * Get the delay that is needed to ensure that all existing RRsets have been
 * re-signed with a successor key.  This is the signature validity minus the
 * signature refresh time (that indicates how far before signature expiry an
 * RRSIG should be refreshed).
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   signature refresh interval.
 */

uint32_t
dns_kasp_sigrefresh(dns_kasp_t *kasp);
/*%<
 * Get signature refresh interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   signature refresh interval.
 */

void
dns_kasp_setsigrefresh(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set signature refresh interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_sigvalidity(dns_kasp_t *kasp);
uint32_t
dns_kasp_sigvalidity_dnskey(dns_kasp_t *kasp);
/*%<
 * Get signature validity.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   signature validity.
 */

void
dns_kasp_setsigvalidity(dns_kasp_t *kasp, uint32_t value);
void
dns_kasp_setsigvalidity_dnskey(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set signature validity.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

dns_ttl_t
dns_kasp_dnskeyttl(dns_kasp_t *kasp);
/*%<
 * Get DNSKEY TTL.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   DNSKEY TTL.
 */

void
dns_kasp_setdnskeyttl(dns_kasp_t *kasp, dns_ttl_t ttl);
/*%<
 * Set DNSKEY TTL.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_purgekeys(dns_kasp_t *kasp);
/*%<
 * Get purge keys interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Purge keys interval.
 */

void
dns_kasp_setpurgekeys(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set purge keys interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_publishsafety(dns_kasp_t *kasp);
/*%<
 * Get publish safety interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Publish safety interval.
 */

void
dns_kasp_setpublishsafety(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set publish safety interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_retiresafety(dns_kasp_t *kasp);
/*%<
 * Get retire safety interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Retire safety interval.
 */

void
dns_kasp_setretiresafety(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set retire safety interval.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

dns_ttl_t
dns_kasp_zonemaxttl(dns_kasp_t *kasp);
/*%<
 * Get maximum zone TTL.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Maximum zone TTL.
 */

void
dns_kasp_setzonemaxttl(dns_kasp_t *kasp, dns_ttl_t ttl);
/*%<
 * Set maximum zone TTL.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_zonepropagationdelay(dns_kasp_t *kasp);
/*%<
 * Get zone propagation delay.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Zone propagation delay.
 */

void
dns_kasp_setzonepropagationdelay(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set zone propagation delay.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

dns_ttl_t
dns_kasp_dsttl(dns_kasp_t *kasp);
/*%<
 * Get DS TTL (should match that of the parent DS record).
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Expected parent DS TTL.
 */

void
dns_kasp_setdsttl(dns_kasp_t *kasp, dns_ttl_t ttl);
/*%<
 * Set DS TTL.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

uint32_t
dns_kasp_parentpropagationdelay(dns_kasp_t *kasp);
/*%<
 * Get parent zone propagation delay.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li   Parent zone propagation delay.
 */

void
dns_kasp_setparentpropagationdelay(dns_kasp_t *kasp, uint32_t value);
/*%<
 * Set parent propagation delay.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 */

isc_result_t
dns_kasplist_find(dns_kasplist_t *list, const char *name, dns_kasp_t **kaspp);
/*%<
 * Search for a kasp with name 'name' in 'list'.
 * If found, '*kaspp' is (strongly) attached to it.
 *
 * Requires:
 *
 *\li   'kaspp' points to a NULL dns_kasp_t *.
 *
 * Returns:
 *
 *\li   #ISC_R_SUCCESS          A matching kasp was found.
 *\li   #ISC_R_NOTFOUND         No matching kasp was found.
 */

dns_kasp_keylist_t
dns_kasp_keys(dns_kasp_t *kasp);
/*%<
 * Get the list of kasp keys.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, frozen kasp.
 *
 * Returns:
 *
 *\li  #ISC_R_SUCCESS
 *\li  #ISC_R_NOMEMORY
 *
 *\li  Other errors are possible.
 */

bool
dns_kasp_keylist_empty(dns_kasp_t *kasp);
/*%<
 * Check if the keylist is empty.
 *
 * Requires:
 *
 *\li   'kasp' is a valid kasp.
 *
 * Returns:
 *
 *\li  true if the keylist is empty, false otherwise.
 */

void
dns_kasp_addkey(dns_kasp_t *kasp, dns_kasp_key_t *key);
/*%<
 * Add a key.
 *
 * Requires:
 *
 *\li   'kasp' is a valid, thawed kasp.
 *\li   'key' is not NULL.
 */

isc_result_t
dns_kasp_key_create(dns_kasp_t *kasp, dns_kasp_key_t **keyp);
/*%<
 * Create a key inside a KASP.
 *
 * Requires:
 *
 *\li   'kasp' is a valid kasp.
 *
 *\li  keyp != NULL && *keyp == NULL
 *
 * Returns:
 *
 *\li  #ISC_R_SUCCESS
 *\li  #ISC_R_NOMEMORY
 *
 *\li  Other errors are possible.
 */

void
dns_kasp_key_destroy(dns_kasp_key_t *key);
/*%<
 * Destroy a KASP key.
 *
 * Requires:
 *
 *\li  key != NULL
 */

uint32_t
dns_kasp_key_algorithm(dns_kasp_key_t *key);
/*%<
 * Get the key algorithm.
 *
 * Requires:
 *
 *\li  key != NULL
 *
 * Returns:
 *
 *\li  Key algorithm.
 */

unsigned int
dns_kasp_key_size(dns_kasp_key_t *key);
/*%<
 * Get the key size.
 *
 * Requires:
 *
 *\li  key != NULL
 *
 * Returns:
 *
 *\li  Configured key size, or default key size for key algorithm if no size
 *     configured.
 */

uint32_t
dns_kasp_key_lifetime(dns_kasp_key_t *key);
/*%<
 * The lifetime of this key (how long may this key be active?)
 *
 * Requires:
 *
 *\li  key != NULL
 *
 * Returns:
 *
 *\li  Lifetime of key.
 *
 */

bool
dns_kasp_key_ksk(dns_kasp_key_t *key);
/*%<
 * Does this key act as a KSK?
 *
 * Requires:
 *
 *\li  key != NULL
 *
 * Returns:
 *
 *\li  True, if the key role has DNS_KASP_KEY_ROLE_KSK set.
 *\li  False, otherwise.
 *
 */

bool
dns_kasp_key_zsk(dns_kasp_key_t *key);
/*%<
 * Does this key act as a ZSK?
 *
 * Requires:
 *
 *\li  key != NULL
 *
 * Returns:
 *
 *\li  True, if the key role has DNS_KASP_KEY_ROLE_ZSK set.
 *\li  False, otherwise.
 *
 */

bool
dns_kasp_nsec3(dns_kasp_t *kasp);
/*%<
 * Return true if NSEC3 chain should be used.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, frozen kasp.
 *
 */

uint8_t
dns_kasp_nsec3iter(dns_kasp_t *kasp);
/*%<
 * The number of NSEC3 iterations to use.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, frozen kasp.
 *\li  'kasp->nsec3' is true.
 *
 */

uint8_t
dns_kasp_nsec3flags(dns_kasp_t *kasp);
/*%<
 * The NSEC3 flags field value.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, frozen kasp.
 *\li  'kasp->nsec3' is true.
 *
 */

uint8_t
dns_kasp_nsec3saltlen(dns_kasp_t *kasp);
/*%<
 * The NSEC3 salt length.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, frozen kasp.
 *\li  'kasp->nsec3' is true.
 *
 */

void
dns_kasp_setnsec3(dns_kasp_t *kasp, bool nsec3);
/*%<
 * Set to use NSEC3 if 'nsec3' is 'true', otherwise policy will use NSEC.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, unfrozen kasp.
 *
 */

void
dns_kasp_setnsec3param(dns_kasp_t *kasp, uint8_t iter, bool optout,
		       uint8_t saltlen);
/*%<
 * Set the desired NSEC3 parameters.
 *
 * Requires:
 *
 *\li  'kasp' is a valid, unfrozen kasp.
 *\li  'kasp->nsec3' is true.
 *
 */

ISC_LANG_ENDDECLS

#endif /* DNS_KASP_H */
