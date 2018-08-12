/*	$NetBSD: site.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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

/* The documentation about this file is in README.site */

#ifndef PK11_SITE_H
#define PK11_SITE_H 1

/*! \file pk11/site.h */

/*\brief Put here specific PKCS#11 tweaks
 *
 *\li PK11_<mechanism>_SKIP:
 *	Don't consider the lack of this mechanism as a fatal error.
 *
 *\li PK11_<mechanism>_REPLACE:
 *      Same as SKIP, and implement the mechanism using lower-level steps.
 *
 *\li PK11_<algorithm>_DISABLE:
 *	Same as SKIP, and disable support for the algorithm.
 *
 *\li PK11_PAD_HMAC_KEYS:
 *	Extend HMAC keys shorter than digest length.
 */

/* current implemented flags are:
PK11_DH_PKCS_PARAMETER_GEN_SKIP
PK11_DSA_PARAMETER_GEN_SKIP
PK11_RSA_PKCS_REPLACE
PK11_MD5_HMAC_REPLACE
PK11_SHA_1_HMAC_REPLACE
PK11_SHA224_HMAC_REPLACE
PK11_SHA256_HMAC_REPLACE
PK11_SHA384_HMAC_REPLACE
PK11_SHA512_HMAC_REPLACE
PK11_MD5_DISABLE
PK11_DSA_DISABLE
PK11_DH_DISABLE
PK11_PAD_HMAC_KEYS
*/

/*
 * Predefined flavors
 */
/* Thales nCipher */
#define PK11_THALES_FLAVOR 0
/* SoftHSMv1 with SHA224 */
#define PK11_SOFTHSMV1_FLAVOR 1
/* SoftHSMv2 */
#define PK11_SOFTHSMV2_FLAVOR 2
/* Cryptech */
#define PK11_CRYPTECH_FLAVOR 3
/* AEP Keyper */
#define PK11_AEP_FLAVOR 4

/* Default is for Thales nCipher */
#ifndef PK11_FLAVOR
#define PK11_FLAVOR PK11_THALES_FLAVOR
#endif

#if PK11_FLAVOR == PK11_THALES_FLAVOR
#define PK11_DH_PKCS_PARAMETER_GEN_SKIP
/* doesn't work but supported #define PK11_DSA_PARAMETER_GEN_SKIP */
#define PK11_MD5_HMAC_REPLACE
#endif

#if PK11_FLAVOR == PK11_SOFTHSMV1_FLAVOR
#define PK11_PAD_HMAC_KEYS
#endif

#if PK11_FLAVOR == PK11_SOFTHSMV2_FLAVOR
/* SoftHSMv2 was updated to enforce minimal key sizes... argh! */
#define PK11_MD5_HMAC_REPLACE
#define PK11_SHA_1_HMAC_REPLACE
#define PK11_SHA224_HMAC_REPLACE
#define PK11_SHA256_HMAC_REPLACE
#define PK11_SHA384_HMAC_REPLACE
#define PK11_SHA512_HMAC_REPLACE
#endif

#if PK11_FLAVOR == PK11_CRYPTECH_FLAVOR
#define PK11_DH_DISABLE
#define PK11_DSA_DISABLE
#define PK11_MD5_DISABLE
#define PK11_SHA_1_HMAC_REPLACE
#define PK11_SHA224_HMAC_REPLACE
#define PK11_SHA256_HMAC_REPLACE
#define PK11_SHA384_HMAC_REPLACE
#define PK11_SHA512_HMAC_REPLACE
#endif

#if PK11_FLAVOR == PK11_AEP_FLAVOR
#define PK11_DH_DISABLE
#define PK11_DSA_DISABLE
#define PK11_RSA_PKCS_REPLACE
#define PK11_MD5_HMAC_REPLACE
#define PK11_SHA_1_HMAC_REPLACE
#define PK11_SHA224_HMAC_REPLACE
#define PK11_SHA256_HMAC_REPLACE
#define PK11_SHA384_HMAC_REPLACE
#define PK11_SHA512_HMAC_REPLACE
#endif

#endif /* PK11_SITE_H */
