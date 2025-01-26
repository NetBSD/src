/*	$NetBSD: key.c,v 1.9 2025/01/26 16:25:23 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <isc/mem.h>
#include <isc/region.h>
#include <isc/util.h>

#include <dns/keyvalues.h>

#include <dst/dst.h>

#include "dst_internal.h"

uint16_t
dst_region_computeid(const isc_region_t *source) {
	uint32_t ac;
	const unsigned char *p;
	int size;

	REQUIRE(source != NULL);
	REQUIRE(source->length >= 4);

	p = source->base;
	size = source->length;

	for (ac = 0; size > 1; size -= 2, p += 2) {
		ac += ((*p) << 8) + *(p + 1);
	}

	if (size > 0) {
		ac += ((*p) << 8);
	}
	ac += (ac >> 16) & 0xffff;

	return (uint16_t)(ac & 0xffff);
}

uint16_t
dst_region_computerid(const isc_region_t *source) {
	uint32_t ac;
	const unsigned char *p;
	int size;

	REQUIRE(source != NULL);
	REQUIRE(source->length >= 4);

	p = source->base;
	size = source->length;

	ac = ((*p) << 8) + *(p + 1);
	ac |= DNS_KEYFLAG_REVOKE;
	for (size -= 2, p += 2; size > 1; size -= 2, p += 2) {
		ac += ((*p) << 8) + *(p + 1);
	}

	if (size > 0) {
		ac += ((*p) << 8);
	}
	ac += (ac >> 16) & 0xffff;

	return (uint16_t)(ac & 0xffff);
}

dns_name_t *
dst_key_name(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_name;
}

unsigned int
dst_key_size(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_size;
}

unsigned int
dst_key_proto(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_proto;
}

unsigned int
dst_key_alg(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_alg;
}

uint32_t
dst_key_flags(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_flags;
}

dns_keytag_t
dst_key_id(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_id;
}

dns_keytag_t
dst_key_rid(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_rid;
}

dns_rdataclass_t
dst_key_class(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_class;
}

const char *
dst_key_directory(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->directory;
}

bool
dst_key_iszonekey(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));

	if ((key->key_flags & DNS_KEYTYPE_NOAUTH) != 0) {
		return false;
	}
	if ((key->key_flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE) {
		return false;
	}
	if (key->key_proto != DNS_KEYPROTO_DNSSEC &&
	    key->key_proto != DNS_KEYPROTO_ANY)
	{
		return false;
	}
	return true;
}

bool
dst_key_isnullkey(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));

	if ((key->key_flags & DNS_KEYFLAG_TYPEMASK) != DNS_KEYTYPE_NOKEY) {
		return false;
	}
	if ((key->key_flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE) {
		return false;
	}
	if (key->key_proto != DNS_KEYPROTO_DNSSEC &&
	    key->key_proto != DNS_KEYPROTO_ANY)
	{
		return false;
	}
	return true;
}

#define REVOKE(x) ((dst_key_flags(x) & DNS_KEYFLAG_REVOKE) != 0)
#define KSK(x)	  ((dst_key_flags(x) & DNS_KEYFLAG_KSK) != 0)
#define ID(x)	  dst_key_id(x)
#define ALG(x)	  dst_key_alg(x)

bool
dst_key_have_ksk_and_zsk(dst_key_t **keys, unsigned int nkeys, unsigned int i,
			 bool check_offline, bool ksk, bool zsk, bool *have_ksk,
			 bool *have_zsk) {
	bool hksk = ksk;
	bool hzsk = zsk;
	isc_result_t result;

	REQUIRE(keys != NULL);

	for (unsigned int j = 0; j < nkeys && !(hksk && hzsk); j++) {
		if (j == i || ALG(keys[i]) != ALG(keys[j])) {
			continue;
		}
		/*
		 * Don't consider inactive keys.
		 */
		if (dst_key_inactive(keys[j])) {
			continue;
		}
		/*
		 * Don't consider offline keys.
		 */
		if (check_offline && !dst_key_isprivate(keys[j])) {
			continue;
		}
		if (REVOKE(keys[j])) {
			continue;
		}

		if (!hksk) {
			result = dst_key_getbool(keys[j], DST_BOOL_KSK, &hksk);
			if (result != ISC_R_SUCCESS) {
				if (KSK(keys[j])) {
					hksk = true;
				}
			}
		}
		if (!hzsk) {
			result = dst_key_getbool(keys[j], DST_BOOL_ZSK, &hzsk);
			if (result != ISC_R_SUCCESS) {
				if (!KSK(keys[j])) {
					hzsk = dst_key_isprivate(keys[j]);
				}
			}
		}
	}

	SET_IF_NOT_NULL(have_ksk, hksk);
	SET_IF_NOT_NULL(have_zsk, hzsk);
	return hksk && hzsk;
}

void
dst_key_setbits(dst_key_t *key, uint16_t bits) {
	unsigned int maxbits;
	REQUIRE(VALID_KEY(key));
	if (bits != 0) {
		RUNTIME_CHECK(dst_key_sigsize(key, &maxbits) == ISC_R_SUCCESS);
		maxbits *= 8;
		REQUIRE(bits <= maxbits);
	}
	key->key_bits = bits;
}

uint16_t
dst_key_getbits(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_bits;
}

void
dst_key_setttl(dst_key_t *key, dns_ttl_t ttl) {
	REQUIRE(VALID_KEY(key));
	key->key_ttl = ttl;
}

dns_ttl_t
dst_key_getttl(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return key->key_ttl;
}

void
dst_key_setdirectory(dst_key_t *key, const char *dir) {
	if (key->directory != NULL) {
		isc_mem_free(key->mctx, key->directory);
	}
	key->directory = isc_mem_strdup(key->mctx, dir);
}

/*! \file */
