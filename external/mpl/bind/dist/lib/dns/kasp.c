/*	$NetBSD: kasp.c,v 1.8 2025/01/26 16:25:23 christos Exp $	*/

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

/*! \file */

#include <string.h>

#include <isc/assertions.h>
#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/hex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/util.h>

#include <dns/kasp.h>
#include <dns/keyvalues.h>
#include <dns/log.h>

#include <dst/dst.h>

/* Default TTLsig (maximum zone ttl) */
#define DEFAULT_TTLSIG 604800 /* one week */

isc_result_t
dns_kasp_create(isc_mem_t *mctx, const char *name, dns_kasp_t **kaspp) {
	dns_kasp_t *kasp;
	dns_kasp_t k = {
		.magic = DNS_KASP_MAGIC,
		.digests = ISC_LIST_INITIALIZER,
		.keys = ISC_LIST_INITIALIZER,
		.link = ISC_LINK_INITIALIZER,
	};

	REQUIRE(name != NULL);
	REQUIRE(kaspp != NULL && *kaspp == NULL);

	kasp = isc_mem_get(mctx, sizeof(*kasp));
	*kasp = k;

	kasp->mctx = NULL;
	isc_mem_attach(mctx, &kasp->mctx);
	kasp->name = isc_mem_strdup(mctx, name);
	isc_mutex_init(&kasp->lock);
	isc_refcount_init(&kasp->references, 1);

	*kaspp = kasp;
	return ISC_R_SUCCESS;
}

void
dns_kasp_attach(dns_kasp_t *source, dns_kasp_t **targetp) {
	REQUIRE(DNS_KASP_VALID(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);
	*targetp = source;
}

static void
destroy(dns_kasp_t *kasp) {
	dns_kasp_key_t *key, *key_next;
	dns_kasp_digest_t *digest, *digest_next;

	REQUIRE(!ISC_LINK_LINKED(kasp, link));

	for (key = ISC_LIST_HEAD(kasp->keys); key != NULL; key = key_next) {
		key_next = ISC_LIST_NEXT(key, link);
		ISC_LIST_UNLINK(kasp->keys, key, link);
		dns_kasp_key_destroy(key);
	}
	INSIST(ISC_LIST_EMPTY(kasp->keys));

	for (digest = ISC_LIST_HEAD(kasp->digests); digest != NULL;
	     digest = digest_next)
	{
		digest_next = ISC_LIST_NEXT(digest, link);
		ISC_LIST_UNLINK(kasp->digests, digest, link);
		isc_mem_put(kasp->mctx, digest, sizeof(*digest));
	}
	INSIST(ISC_LIST_EMPTY(kasp->digests));

	isc_mutex_destroy(&kasp->lock);
	isc_mem_free(kasp->mctx, kasp->name);
	isc_mem_putanddetach(&kasp->mctx, kasp, sizeof(*kasp));
}

void
dns_kasp_detach(dns_kasp_t **kaspp) {
	REQUIRE(kaspp != NULL && DNS_KASP_VALID(*kaspp));

	dns_kasp_t *kasp = *kaspp;
	*kaspp = NULL;

	if (isc_refcount_decrement(&kasp->references) == 1) {
		destroy(kasp);
	}
}

const char *
dns_kasp_getname(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));

	return kasp->name;
}

void
dns_kasp_freeze(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->frozen = true;
}

void
dns_kasp_thaw(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	kasp->frozen = false;
}

uint32_t
dns_kasp_signdelay(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->signatures_validity - kasp->signatures_refresh;
}

uint32_t
dns_kasp_sigjitter(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->signatures_jitter;
}

void
dns_kasp_setsigjitter(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->signatures_jitter = value;
}

uint32_t
dns_kasp_sigrefresh(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->signatures_refresh;
}

void
dns_kasp_setsigrefresh(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->signatures_refresh = value;
}

uint32_t
dns_kasp_sigvalidity(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->signatures_validity;
}

void
dns_kasp_setsigvalidity(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->signatures_validity = value;
}

uint32_t
dns_kasp_sigvalidity_dnskey(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->signatures_validity_dnskey;
}

void
dns_kasp_setsigvalidity_dnskey(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->signatures_validity_dnskey = value;
}

dns_ttl_t
dns_kasp_dnskeyttl(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->dnskey_ttl;
}

void
dns_kasp_setdnskeyttl(dns_kasp_t *kasp, dns_ttl_t ttl) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->dnskey_ttl = ttl;
}

uint32_t
dns_kasp_purgekeys(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->purge_keys;
}

void
dns_kasp_setpurgekeys(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->purge_keys = value;
}

uint32_t
dns_kasp_publishsafety(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->publish_safety;
}

void
dns_kasp_setpublishsafety(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->publish_safety = value;
}

uint32_t
dns_kasp_retiresafety(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->retire_safety;
}

void
dns_kasp_setretiresafety(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->retire_safety = value;
}

bool
dns_kasp_inlinesigning(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->inline_signing;
}

void
dns_kasp_setinlinesigning(dns_kasp_t *kasp, bool value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->inline_signing = value;
}

dns_ttl_t
dns_kasp_zonemaxttl(dns_kasp_t *kasp, bool fallback) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	if (kasp->zone_max_ttl == 0 && fallback) {
		return DEFAULT_TTLSIG;
	}
	return kasp->zone_max_ttl;
}

void
dns_kasp_setzonemaxttl(dns_kasp_t *kasp, dns_ttl_t ttl) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->zone_max_ttl = ttl;
}

uint32_t
dns_kasp_zonepropagationdelay(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->zone_propagation_delay;
}

void
dns_kasp_setzonepropagationdelay(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->zone_propagation_delay = value;
}

dns_ttl_t
dns_kasp_dsttl(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->parent_ds_ttl;
}

void
dns_kasp_setdsttl(dns_kasp_t *kasp, dns_ttl_t ttl) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->parent_ds_ttl = ttl;
}

uint32_t
dns_kasp_parentpropagationdelay(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->parent_propagation_delay;
}

void
dns_kasp_setparentpropagationdelay(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->parent_propagation_delay = value;
}

isc_result_t
dns_kasplist_find(dns_kasplist_t *list, const char *name, dns_kasp_t **kaspp) {
	dns_kasp_t *kasp = NULL;

	REQUIRE(kaspp != NULL && *kaspp == NULL);

	if (list == NULL) {
		return ISC_R_NOTFOUND;
	}

	for (kasp = ISC_LIST_HEAD(*list); kasp != NULL;
	     kasp = ISC_LIST_NEXT(kasp, link))
	{
		if (strcmp(kasp->name, name) == 0) {
			break;
		}
	}

	if (kasp == NULL) {
		return ISC_R_NOTFOUND;
	}

	dns_kasp_attach(kasp, kaspp);
	return ISC_R_SUCCESS;
}

dns_kasp_keylist_t
dns_kasp_keys(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->keys;
}

bool
dns_kasp_keylist_empty(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));

	return ISC_LIST_EMPTY(kasp->keys);
}

void
dns_kasp_addkey(dns_kasp_t *kasp, dns_kasp_key_t *key) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);
	REQUIRE(key != NULL);

	ISC_LIST_APPEND(kasp->keys, key, link);
}

isc_result_t
dns_kasp_key_create(dns_kasp_t *kasp, dns_kasp_key_t **keyp) {
	dns_kasp_key_t *key = NULL;
	dns_kasp_key_t k = { .tag_max = 0xffff, .length = -1 };

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = isc_mem_get(kasp->mctx, sizeof(*key));
	*key = k;

	key->mctx = NULL;
	isc_mem_attach(kasp->mctx, &key->mctx);

	ISC_LINK_INIT(key, link);

	*keyp = key;
	return ISC_R_SUCCESS;
}

void
dns_kasp_key_destroy(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	if (key->keystore != NULL) {
		dns_keystore_detach(&key->keystore);
	}
	isc_mem_putanddetach(&key->mctx, key, sizeof(*key));
}

uint32_t
dns_kasp_key_algorithm(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return key->algorithm;
}

unsigned int
dns_kasp_key_size(dns_kasp_key_t *key) {
	unsigned int size = 0;
	unsigned int min = 0;

	REQUIRE(key != NULL);

	switch (key->algorithm) {
	case DNS_KEYALG_RSASHA1:
	case DNS_KEYALG_NSEC3RSASHA1:
	case DNS_KEYALG_RSASHA256:
	case DNS_KEYALG_RSASHA512:
		min = (key->algorithm == DNS_KEYALG_RSASHA512) ? 1024 : 512;
		if (key->length > -1) {
			size = (unsigned int)key->length;
			if (size < min) {
				size = min;
			}
			if (size > 4096) {
				size = 4096;
			}
		} else {
			size = 2048;
		}
		break;
	case DNS_KEYALG_ECDSA256:
		size = 256;
		break;
	case DNS_KEYALG_ECDSA384:
		size = 384;
		break;
	case DNS_KEYALG_ED25519:
		size = 256;
		break;
	case DNS_KEYALG_ED448:
		size = 456;
		break;
	default:
		/* unsupported */
		break;
	}
	return size;
}

uint32_t
dns_kasp_key_lifetime(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return key->lifetime;
}

dns_keystore_t *
dns_kasp_key_keystore(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return key->keystore;
}

bool
dns_kasp_key_ksk(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return key->role & DNS_KASP_KEY_ROLE_KSK;
}

bool
dns_kasp_key_zsk(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return key->role & DNS_KASP_KEY_ROLE_ZSK;
}

uint16_t
dns_kasp_key_tagmin(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);
	return key->tag_min;
}

uint16_t
dns_kasp_key_tagmax(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);
	return key->tag_min;
}

bool
dns_kasp_key_match(dns_kasp_key_t *key, dns_dnsseckey_t *dkey) {
	isc_result_t ret;
	bool role = false;

	REQUIRE(key != NULL);
	REQUIRE(dkey != NULL);

	/* Matching algorithms? */
	if (dst_key_alg(dkey->key) != dns_kasp_key_algorithm(key)) {
		return false;
	}
	/* Matching length? */
	if (dst_key_size(dkey->key) != dns_kasp_key_size(key)) {
		return false;
	}
	/* Matching role? */
	ret = dst_key_getbool(dkey->key, DST_BOOL_KSK, &role);
	if (ret != ISC_R_SUCCESS || role != dns_kasp_key_ksk(key)) {
		return false;
	}
	ret = dst_key_getbool(dkey->key, DST_BOOL_ZSK, &role);
	if (ret != ISC_R_SUCCESS || role != dns_kasp_key_zsk(key)) {
		return false;
	}
	/* Valid key tag range? */
	uint16_t id = dst_key_id(dkey->key);
	uint16_t rid = dst_key_rid(dkey->key);
	if (id < key->tag_min || id > key->tag_max) {
		return false;
	}
	if (rid < key->tag_min || rid > key->tag_max) {
		return false;
	}

	/* Found a match. */
	return true;
}

uint8_t
dns_kasp_nsec3iter(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	return kasp->nsec3param.iterations;
}

uint8_t
dns_kasp_nsec3flags(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	if (kasp->nsec3param.optout) {
		return 0x01;
	}
	return 0x00;
}

uint8_t
dns_kasp_nsec3saltlen(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	return kasp->nsec3param.saltlen;
}

bool
dns_kasp_nsec3(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);

	return kasp->nsec3;
}

void
dns_kasp_setnsec3(dns_kasp_t *kasp, bool nsec3) {
	REQUIRE(kasp != NULL);
	REQUIRE(!kasp->frozen);

	kasp->nsec3 = nsec3;
}

void
dns_kasp_setnsec3param(dns_kasp_t *kasp, uint8_t iter, bool optout,
		       uint8_t saltlen) {
	REQUIRE(kasp != NULL);
	REQUIRE(!kasp->frozen);
	REQUIRE(kasp->nsec3);

	kasp->nsec3param.iterations = iter;
	kasp->nsec3param.optout = optout;
	kasp->nsec3param.saltlen = saltlen;
}

bool
dns_kasp_offlineksk(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);

	return kasp->offlineksk;
}

void
dns_kasp_setofflineksk(dns_kasp_t *kasp, bool offlineksk) {
	REQUIRE(kasp != NULL);
	REQUIRE(!kasp->frozen);

	kasp->offlineksk = offlineksk;
}

bool
dns_kasp_cdnskey(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);

	return kasp->cdnskey;
}

void
dns_kasp_setcdnskey(dns_kasp_t *kasp, bool cdnskey) {
	REQUIRE(kasp != NULL);
	REQUIRE(!kasp->frozen);

	kasp->cdnskey = cdnskey;
}

dns_kasp_digestlist_t
dns_kasp_digests(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return kasp->digests;
}

void
dns_kasp_adddigest(dns_kasp_t *kasp, dns_dsdigest_t alg) {
	dns_kasp_digest_t *digest;

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	/* Suppress unsupported algorithms */
	if (!dst_ds_digest_supported(alg)) {
		return;
	}

	/* Suppress duplicates */
	for (dns_kasp_digest_t *d = ISC_LIST_HEAD(kasp->digests); d != NULL;
	     d = ISC_LIST_NEXT(d, link))
	{
		if (d->digest == alg) {
			return;
		}
	}

	digest = isc_mem_get(kasp->mctx, sizeof(*digest));
	digest->digest = alg;
	ISC_LINK_INIT(digest, link);
	ISC_LIST_APPEND(kasp->digests, digest, link);
}
