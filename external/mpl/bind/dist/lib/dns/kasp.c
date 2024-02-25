/*	$NetBSD: kasp.c,v 1.5.2.1 2024/02/25 15:46:50 martin Exp $	*/

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

/* Default TTLsig (maximum zone ttl) */
#define DEFAULT_TTLSIG 604800 /* one week */

isc_result_t
dns_kasp_create(isc_mem_t *mctx, const char *name, dns_kasp_t **kaspp) {
	dns_kasp_t *kasp;
	dns_kasp_t k = {
		.magic = DNS_KASP_MAGIC,
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

	ISC_LINK_INIT(kasp, link);
	ISC_LIST_INIT(kasp->keys);

	*kaspp = kasp;
	return (ISC_R_SUCCESS);
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
	dns_kasp_key_t *key;
	dns_kasp_key_t *key_next;

	REQUIRE(!ISC_LINK_LINKED(kasp, link));

	for (key = ISC_LIST_HEAD(kasp->keys); key != NULL; key = key_next) {
		key_next = ISC_LIST_NEXT(key, link);
		ISC_LIST_UNLINK(kasp->keys, key, link);
		dns_kasp_key_destroy(key);
	}
	INSIST(ISC_LIST_EMPTY(kasp->keys));

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

	return (kasp->name);
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

	return (kasp->signatures_validity - kasp->signatures_refresh);
}

uint32_t
dns_kasp_sigrefresh(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return (kasp->signatures_refresh);
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

	return (kasp->signatures_validity);
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

	return (kasp->signatures_validity_dnskey);
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

	return (kasp->dnskey_ttl);
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

	return (kasp->purge_keys);
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

	return (kasp->publish_safety);
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

	return (kasp->retire_safety);
}

void
dns_kasp_setretiresafety(dns_kasp_t *kasp, uint32_t value) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(!kasp->frozen);

	kasp->retire_safety = value;
}

dns_ttl_t
dns_kasp_zonemaxttl(dns_kasp_t *kasp, bool fallback) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	if (kasp->zone_max_ttl == 0 && fallback) {
		return (DEFAULT_TTLSIG);
	}
	return (kasp->zone_max_ttl);
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

	return (kasp->zone_propagation_delay);
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

	return (kasp->parent_ds_ttl);
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

	return (kasp->parent_propagation_delay);
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
		return (ISC_R_NOTFOUND);
	}

	for (kasp = ISC_LIST_HEAD(*list); kasp != NULL;
	     kasp = ISC_LIST_NEXT(kasp, link))
	{
		if (strcmp(kasp->name, name) == 0) {
			break;
		}
	}

	if (kasp == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dns_kasp_attach(kasp, kaspp);
	return (ISC_R_SUCCESS);
}

dns_kasp_keylist_t
dns_kasp_keys(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(kasp->frozen);

	return (kasp->keys);
}

bool
dns_kasp_keylist_empty(dns_kasp_t *kasp) {
	REQUIRE(DNS_KASP_VALID(kasp));

	return (ISC_LIST_EMPTY(kasp->keys));
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
	dns_kasp_key_t *key;

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = isc_mem_get(kasp->mctx, sizeof(*key));
	key->mctx = NULL;
	isc_mem_attach(kasp->mctx, &key->mctx);

	ISC_LINK_INIT(key, link);

	key->lifetime = 0;
	key->algorithm = 0;
	key->length = -1;
	key->role = 0;
	*keyp = key;
	return (ISC_R_SUCCESS);
}

void
dns_kasp_key_destroy(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	isc_mem_putanddetach(&key->mctx, key, sizeof(*key));
}

uint32_t
dns_kasp_key_algorithm(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return (key->algorithm);
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
	return (size);
}

uint32_t
dns_kasp_key_lifetime(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return (key->lifetime);
}

bool
dns_kasp_key_ksk(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return (key->role & DNS_KASP_KEY_ROLE_KSK);
}

bool
dns_kasp_key_zsk(dns_kasp_key_t *key) {
	REQUIRE(key != NULL);

	return (key->role & DNS_KASP_KEY_ROLE_ZSK);
}

uint8_t
dns_kasp_nsec3iter(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	return (kasp->nsec3param.iterations);
}

uint8_t
dns_kasp_nsec3flags(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	if (kasp->nsec3param.optout) {
		return (0x01);
	}
	return (0x00);
}

uint8_t
dns_kasp_nsec3saltlen(dns_kasp_t *kasp) {
	REQUIRE(kasp != NULL);
	REQUIRE(kasp->frozen);
	REQUIRE(kasp->nsec3);

	return (kasp->nsec3param.saltlen);
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
