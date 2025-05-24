/*	$NetBSD: keyvalues.h,v 1.10 2025/05/21 14:48:04 christos Exp $	*/

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

/*! \file dns/keyvalues.h */

/*
 * Flags field of the KEY rdata. Also used by DNSKEY, CDNSKEY, RKEY,
 * KEYDATA. Some values are only defined for KEY and not the others,
 * and vice versa.
 */
enum {
	/* valid for KEY only. if both are set, there is no key data. */
	DNS_KEYTYPE_NOAUTH = 1 << 15, /* cannot be used for authentication. */
	DNS_KEYTYPE_NOCONF = 1 << 14, /* cannot be used for confidentiality. */

	DNS_KEYFLAG_RESERVED2 = 1 << 13, /* reserved: must be zero. */

	DNS_KEYFLAG_EXTENDED = 1 << 12, /* key has extended flags: if this is
					 * set, the first two octets of the
					 * key data are an additional flags
					 * field, at least one bit of which
					 * must be nonzero. (valid for KEY
					 * only.) */

	DNS_KEYFLAG_RESERVED4 = 1 << 11, /* reserved: must be zero. */
	DNS_KEYFLAG_RESERVED5 = 1 << 10, /* reserved: must be zero. */

	/* if nether of these is set, this is a user key (valid for KEY only) */
	DNS_KEYOWNER_ENTITY = 1 << 9, /* host key (valid for KEY only). */
	DNS_KEYOWNER_ZONE = 1 << 8,   /* zone key (mandatory for DNSKEY). */

	DNS_KEYFLAG_REVOKE = 1 << 7,	 /* key revoked (per rfc5011) */
	DNS_KEYFLAG_RESERVED9 = 1 << 6,	 /* reserved: must be zero. */
	DNS_KEYFLAG_RESERVED10 = 1 << 5, /* reserved: must be zero. */
	DNS_KEYFLAG_RESERVED11 = 1 << 4, /* reserved: must be zero. */

	DNS_KEYFLAG_RESERVED12 = 1 << 3, /* reserved: must be zero. */
	DNS_KEYFLAG_RESERVED13 = 1 << 4, /* reserved: must be zero. */
	DNS_KEYFLAG_RESERVED14 = 1 << 2, /* reserved: must be zero. */

	DNS_KEYFLAG_KSK = 1 << 0, /* key signing key */
};

#define DNS_KEYFLAG_OWNERMASK (DNS_KEYOWNER_ENTITY | DNS_KEYOWNER_ZONE)
#define DNS_KEYFLAG_TYPEMASK  (DNS_KEYTYPE_NOAUTH | DNS_KEYTYPE_NOCONF)
#define DNS_KEYTYPE_NOKEY     DNS_KEYFLAG_TYPEMASK

/* The Algorithm field of the KEY and SIG RR's is an integer, {1..254} */
enum {
	DNS_KEYALG_RSAMD5 = 1,	      /*%< RSA with MD5 */
	DNS_KEYALG_DH_DEPRECATED = 2, /*%< deprecated */
	DNS_KEYALG_DSA = 3,	      /*%< DSA KEY */
	DNS_KEYALG_RSASHA1 = 5,
	DNS_KEYALG_NSEC3DSA = 6,
	DNS_KEYALG_NSEC3RSASHA1 = 7,
	DNS_KEYALG_RSASHA256 = 8,
	DNS_KEYALG_RSASHA512 = 10,
	DNS_KEYALG_ECCGOST = 12,
	DNS_KEYALG_ECDSA256 = 13,
	DNS_KEYALG_ECDSA384 = 14,
	DNS_KEYALG_ED25519 = 15,
	DNS_KEYALG_ED448 = 16,
	DNS_KEYALG_INDIRECT = 252,
	DNS_KEYALG_PRIVATEDNS = 253,
	DNS_KEYALG_PRIVATEOID = 254, /*%< Key begins with OID giving alg */
	DNS_KEYALG_MAX = 255,
};

/* Protocol values  */
enum {
	DNS_KEYPROTO_RESERVED = 0,
	DNS_KEYPROTO_DNSSEC = 3,
	DNS_KEYPROTO_ANY = 255,
};

/* Key and signature sizes */
#define DNS_KEY_ECDSA256SIZE 64
#define DNS_SIG_ECDSA256SIZE 64

#define DNS_KEY_ECDSA384SIZE 96
#define DNS_SIG_ECDSA384SIZE 96

#define DNS_KEY_ED25519SIZE 32
#define DNS_SIG_ED25519SIZE 64

#define DNS_KEY_ED448SIZE 57
#define DNS_SIG_ED448SIZE 114
