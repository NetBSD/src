/*	$NetBSD: opensslrsa_link.c,v 1.10 2023/01/25 21:43:30 christos Exp $	*/

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

#if !USE_PKCS11

#include <inttypes.h>
#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/rsa.h>

#include <isc/mem.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"
#if !defined(OPENSSL_NO_ENGINE)
#include <openssl/engine.h>
#endif /* if !defined(OPENSSL_NO_ENGINE) */

/*
 * Limit the size of public exponents.
 */
#ifndef RSA_MAX_PUBEXP_BITS
#define RSA_MAX_PUBEXP_BITS 35
#endif /* ifndef RSA_MAX_PUBEXP_BITS */

/*
 * We don't use configure for windows so enforce the OpenSSL version
 * here.  Unlike with configure we don't support overriding this test.
 */
#if defined(WIN32) && (OPENSSL_VERSION_NUMBER < 0x10000000L)
#error Please upgrade OpenSSL to 1.0.0 or greater.
#endif /* if defined(WIN32) && (OPENSSL_VERSION_NUMBER < 0x10000000L) */

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

#if !HAVE_RSA_SET0_KEY
/* From OpenSSL 1.1.0 */
static int
RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d) {
	/*
	 * If the fields n and e in r are NULL, the corresponding input
	 * parameters MUST be non-NULL for n and e.  d may be
	 * left NULL (in case only the public key is used).
	 */
	if ((r->n == NULL && n == NULL) || (r->e == NULL && e == NULL)) {
		return (0);
	}

	if (n != NULL) {
		BN_free(r->n);
		r->n = n;
	}
	if (e != NULL) {
		BN_free(r->e);
		r->e = e;
	}
	if (d != NULL) {
		BN_free(r->d);
		r->d = d;
	}

	return (1);
}

static int
RSA_set0_factors(RSA *r, BIGNUM *p, BIGNUM *q) {
	/*
	 * If the fields p and q in r are NULL, the corresponding input
	 * parameters MUST be non-NULL.
	 */
	if ((r->p == NULL && p == NULL) || (r->q == NULL && q == NULL)) {
		return (0);
	}

	if (p != NULL) {
		BN_free(r->p);
		r->p = p;
	}
	if (q != NULL) {
		BN_free(r->q);
		r->q = q;
	}

	return (1);
}

static int
RSA_set0_crt_params(RSA *r, BIGNUM *dmp1, BIGNUM *dmq1, BIGNUM *iqmp) {
	/*
	 * If the fields dmp1, dmq1 and iqmp in r are NULL, the
	 * corresponding input parameters MUST be non-NULL.
	 */
	if ((r->dmp1 == NULL && dmp1 == NULL) ||
	    (r->dmq1 == NULL && dmq1 == NULL) ||
	    (r->iqmp == NULL && iqmp == NULL))
	{
		return (0);
	}

	if (dmp1 != NULL) {
		BN_free(r->dmp1);
		r->dmp1 = dmp1;
	}
	if (dmq1 != NULL) {
		BN_free(r->dmq1);
		r->dmq1 = dmq1;
	}
	if (iqmp != NULL) {
		BN_free(r->iqmp);
		r->iqmp = iqmp;
	}

	return (1);
}

static void
RSA_get0_key(const RSA *r, const BIGNUM **n, const BIGNUM **e,
	     const BIGNUM **d) {
	if (n != NULL) {
		*n = r->n;
	}
	if (e != NULL) {
		*e = r->e;
	}
	if (d != NULL) {
		*d = r->d;
	}
}

static void
RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q) {
	if (p != NULL) {
		*p = r->p;
	}
	if (q != NULL) {
		*q = r->q;
	}
}

static void
RSA_get0_crt_params(const RSA *r, const BIGNUM **dmp1, const BIGNUM **dmq1,
		    const BIGNUM **iqmp) {
	if (dmp1 != NULL) {
		*dmp1 = r->dmp1;
	}
	if (dmq1 != NULL) {
		*dmq1 = r->dmq1;
	}
	if (iqmp != NULL) {
		*iqmp = r->iqmp;
	}
}

static int
RSA_test_flags(const RSA *r, int flags) {
	return (r->flags & flags);
}

#endif /* !HAVE_RSA_SET0_KEY */

static isc_result_t
opensslrsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx;
	const EVP_MD *type = NULL;

	UNUSED(key);
	REQUIRE(dctx->key->key_alg == DST_ALG_RSASHA1 ||
		dctx->key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		dctx->key->key_alg == DST_ALG_RSASHA256 ||
		dctx->key->key_alg == DST_ALG_RSASHA512);

	/*
	 * Reject incorrect RSA key lengths.
	 */
	switch (dctx->key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		/* From RFC 3110 */
		if (dctx->key->key_size > 4096) {
			return (ISC_R_FAILURE);
		}
		break;
	case DST_ALG_RSASHA256:
		/* From RFC 5702 */
		if ((dctx->key->key_size < 512) || (dctx->key->key_size > 4096))
		{
			return (ISC_R_FAILURE);
		}
		break;
	case DST_ALG_RSASHA512:
		/* From RFC 5702 */
		if ((dctx->key->key_size < 1024) ||
		    (dctx->key->key_size > 4096))
		{
			return (ISC_R_FAILURE);
		}
		break;
	default:
		UNREACHABLE();
	}

	evp_md_ctx = EVP_MD_CTX_create();
	if (evp_md_ctx == NULL) {
		return (ISC_R_NOMEMORY);
	}

	switch (dctx->key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		type = EVP_sha1(); /* SHA1 + RSA */
		break;
	case DST_ALG_RSASHA256:
		type = EVP_sha256(); /* SHA256 + RSA */
		break;
	case DST_ALG_RSASHA512:
		type = EVP_sha512();
		break;
	default:
		UNREACHABLE();
	}

	if (!EVP_DigestInit_ex(evp_md_ctx, type, NULL)) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		return (dst__openssl_toresult3(
			dctx->category, "EVP_DigestInit_ex", ISC_R_FAILURE));
	}
	dctx->ctxdata.evp_md_ctx = evp_md_ctx;

	return (ISC_R_SUCCESS);
}

static void
opensslrsa_destroyctx(dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(dctx->key->key_alg == DST_ALG_RSASHA1 ||
		dctx->key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		dctx->key->key_alg == DST_ALG_RSASHA256 ||
		dctx->key->key_alg == DST_ALG_RSASHA512);

	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		dctx->ctxdata.evp_md_ctx = NULL;
	}
}

static isc_result_t
opensslrsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(dctx->key->key_alg == DST_ALG_RSASHA1 ||
		dctx->key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		dctx->key->key_alg == DST_ALG_RSASHA256 ||
		dctx->key->key_alg == DST_ALG_RSASHA512);

	if (!EVP_DigestUpdate(evp_md_ctx, data->base, data->length)) {
		return (dst__openssl_toresult3(
			dctx->category, "EVP_DigestUpdate", ISC_R_FAILURE));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
opensslrsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_key_t *key = dctx->key;
	isc_region_t r;
	unsigned int siglen = 0;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	EVP_PKEY *pkey = key->keydata.pkey;

	REQUIRE(dctx->key->key_alg == DST_ALG_RSASHA1 ||
		dctx->key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		dctx->key->key_alg == DST_ALG_RSASHA256 ||
		dctx->key->key_alg == DST_ALG_RSASHA512);

	isc_buffer_availableregion(sig, &r);

	if (r.length < (unsigned int)EVP_PKEY_size(pkey)) {
		return (ISC_R_NOSPACE);
	}

	if (!EVP_SignFinal(evp_md_ctx, r.base, &siglen, pkey)) {
		return (dst__openssl_toresult3(dctx->category, "EVP_SignFinal",
					       ISC_R_FAILURE));
	}

	isc_buffer_add(sig, siglen);

	return (ISC_R_SUCCESS);
}

static isc_result_t
opensslrsa_verify2(dst_context_t *dctx, int maxbits, const isc_region_t *sig) {
	dst_key_t *key = dctx->key;
	int status = 0;
	const BIGNUM *e = NULL;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	EVP_PKEY *pkey = key->keydata.pkey;
	RSA *rsa;
	int bits;

	REQUIRE(dctx->key->key_alg == DST_ALG_RSASHA1 ||
		dctx->key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		dctx->key->key_alg == DST_ALG_RSASHA256 ||
		dctx->key->key_alg == DST_ALG_RSASHA512);

	rsa = EVP_PKEY_get1_RSA(pkey);
	if (rsa == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	RSA_get0_key(rsa, NULL, &e, NULL);
	if (e == NULL) {
		RSA_free(rsa);
		return (dst__openssl_toresult(DST_R_VERIFYFAILURE));
	}
	bits = BN_num_bits(e);
	RSA_free(rsa);
	if (bits > maxbits && maxbits != 0) {
		return (DST_R_VERIFYFAILURE);
	}

	status = EVP_VerifyFinal(evp_md_ctx, sig->base, sig->length, pkey);
	switch (status) {
	case 1:
		return (ISC_R_SUCCESS);
	case 0:
		return (dst__openssl_toresult(DST_R_VERIFYFAILURE));
	default:
		return (dst__openssl_toresult3(dctx->category,
					       "EVP_VerifyFinal",
					       DST_R_VERIFYFAILURE));
	}
}

static isc_result_t
opensslrsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	return (opensslrsa_verify2(dctx, 0, sig));
}

static bool
opensslrsa_compare(const dst_key_t *key1, const dst_key_t *key2) {
	int status;
	RSA *rsa1 = NULL, *rsa2 = NULL;
	const BIGNUM *n1 = NULL, *n2 = NULL;
	const BIGNUM *e1 = NULL, *e2 = NULL;
	const BIGNUM *d1 = NULL, *d2 = NULL;
	const BIGNUM *p1 = NULL, *p2 = NULL;
	const BIGNUM *q1 = NULL, *q2 = NULL;
	EVP_PKEY *pkey1, *pkey2;

	pkey1 = key1->keydata.pkey;
	pkey2 = key2->keydata.pkey;
	/*
	 * The pkey reference will keep these around after
	 * the RSA_free() call.
	 */
	if (pkey1 != NULL) {
		rsa1 = EVP_PKEY_get1_RSA(pkey1);
		RSA_free(rsa1);
	}
	if (pkey2 != NULL) {
		rsa2 = EVP_PKEY_get1_RSA(pkey2);
		RSA_free(rsa2);
	}

	if (rsa1 == NULL && rsa2 == NULL) {
		return (true);
	} else if (rsa1 == NULL || rsa2 == NULL) {
		return (false);
	}

	RSA_get0_key(rsa1, &n1, &e1, &d1);
	RSA_get0_key(rsa2, &n2, &e2, &d2);
	status = BN_cmp(n1, n2) || BN_cmp(e1, e2);

	if (status != 0) {
		return (false);
	}

	if (RSA_test_flags(rsa1, RSA_FLAG_EXT_PKEY) != 0 ||
	    RSA_test_flags(rsa2, RSA_FLAG_EXT_PKEY) != 0)
	{
		if (RSA_test_flags(rsa1, RSA_FLAG_EXT_PKEY) == 0 ||
		    RSA_test_flags(rsa2, RSA_FLAG_EXT_PKEY) == 0)
		{
			return (false);
		}
		/*
		 * Can't compare private parameters, BTW does it make sense?
		 */
		return (true);
	}

	if (d1 != NULL || d2 != NULL) {
		if (d1 == NULL || d2 == NULL) {
			return (false);
		}
		RSA_get0_factors(rsa1, &p1, &q1);
		RSA_get0_factors(rsa2, &p2, &q2);
		status = BN_cmp(d1, d2) || BN_cmp(p1, p2) || BN_cmp(q1, q2);

		if (status != 0) {
			return (false);
		}
	}
	return (true);
}

static int
progress_cb(int p, int n, BN_GENCB *cb) {
	union {
		void *dptr;
		void (*fptr)(int);
	} u;

	UNUSED(n);

	u.dptr = BN_GENCB_get_arg(cb);
	if (u.fptr != NULL) {
		u.fptr(p);
	}
	return (1);
}

static isc_result_t
opensslrsa_generate(dst_key_t *key, int exp, void (*callback)(int)) {
	isc_result_t ret = DST_R_OPENSSLFAILURE;
	union {
		void *dptr;
		void (*fptr)(int);
	} u;
	RSA *rsa = RSA_new();
	BIGNUM *e = BN_new();
#if !HAVE_BN_GENCB_NEW
	BN_GENCB _cb;
#endif /* !HAVE_BN_GENCB_NEW */
	BN_GENCB *cb = BN_GENCB_new();
	EVP_PKEY *pkey = EVP_PKEY_new();

	/*
	 * Reject incorrect RSA key lengths.
	 */
	switch (key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		/* From RFC 3110 */
		if (key->key_size > 4096) {
			goto err;
		}
		break;
	case DST_ALG_RSASHA256:
		/* From RFC 5702 */
		if ((key->key_size < 512) || (key->key_size > 4096)) {
			goto err;
		}
		break;
	case DST_ALG_RSASHA512:
		/* From RFC 5702 */
		if ((key->key_size < 1024) || (key->key_size > 4096)) {
			goto err;
		}
		break;
	default:
		UNREACHABLE();
	}

	if (rsa == NULL || e == NULL || cb == NULL) {
		goto err;
	}
	if (pkey == NULL) {
		goto err;
	}
	if (!EVP_PKEY_set1_RSA(pkey, rsa)) {
		goto err;
	}

	if (exp == 0) {
		/* RSA_F4 0x10001 */
		BN_set_bit(e, 0);
		BN_set_bit(e, 16);
	} else {
		/* (phased-out) F5 0x100000001 */
		BN_set_bit(e, 0);
		BN_set_bit(e, 32);
	}

	if (callback == NULL) {
		BN_GENCB_set_old(cb, NULL, NULL);
	} else {
		u.fptr = callback;
		BN_GENCB_set(cb, progress_cb, u.dptr);
	}

	if (RSA_generate_key_ex(rsa, key->key_size, e, cb)) {
		BN_free(e);
		BN_GENCB_free(cb);
		cb = NULL;
		key->keydata.pkey = pkey;

		RSA_free(rsa);
		return (ISC_R_SUCCESS);
	}
	ret = dst__openssl_toresult2("RSA_generate_key_ex",
				     DST_R_OPENSSLFAILURE);

err:
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
		pkey = NULL;
	}
	if (e != NULL) {
		BN_free(e);
		e = NULL;
	}
	if (rsa != NULL) {
		RSA_free(rsa);
		rsa = NULL;
	}
	if (cb != NULL) {
		BN_GENCB_free(cb);
		cb = NULL;
	}
	return (dst__openssl_toresult(ret));
}

static bool
opensslrsa_isprivate(const dst_key_t *key) {
	const BIGNUM *d = NULL;
	RSA *rsa = EVP_PKEY_get1_RSA(key->keydata.pkey);
	INSIST(rsa != NULL);
	RSA_free(rsa);
	/* key->keydata.pkey still has a reference so rsa is still valid. */
	if (rsa != NULL && RSA_test_flags(rsa, RSA_FLAG_EXT_PKEY) != 0) {
		return (true);
	}
	RSA_get0_key(rsa, NULL, NULL, &d);
	return (rsa != NULL && d != NULL);
}

static void
opensslrsa_destroy(dst_key_t *key) {
	EVP_PKEY *pkey = key->keydata.pkey;
	EVP_PKEY_free(pkey);
	key->keydata.pkey = NULL;
}

static isc_result_t
opensslrsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	isc_region_t r;
	unsigned int e_bytes;
	unsigned int mod_bytes;
	isc_result_t ret;
	RSA *rsa;
	EVP_PKEY *pkey;
	const BIGNUM *e = NULL, *n = NULL;

	REQUIRE(key->keydata.pkey != NULL);

	pkey = key->keydata.pkey;
	rsa = EVP_PKEY_get1_RSA(pkey);
	if (rsa == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	RSA_get0_key(rsa, &n, &e, NULL);
	if (e == NULL || n == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	mod_bytes = BN_num_bytes(n);
	e_bytes = BN_num_bytes(e);

	isc_buffer_availableregion(data, &r);

	if (e_bytes < 256) { /*%< key exponent is <= 2040 bits */
		if (r.length < 1) {
			DST_RET(ISC_R_NOSPACE);
		}
		isc_buffer_putuint8(data, (uint8_t)e_bytes);
		isc_region_consume(&r, 1);
	} else {
		if (r.length < 3) {
			DST_RET(ISC_R_NOSPACE);
		}
		isc_buffer_putuint8(data, 0);
		isc_buffer_putuint16(data, (uint16_t)e_bytes);
		isc_region_consume(&r, 3);
	}

	if (r.length < e_bytes + mod_bytes) {
		DST_RET(ISC_R_NOSPACE);
	}

	RSA_get0_key(rsa, &n, &e, NULL);
	BN_bn2bin(e, r.base);
	isc_region_consume(&r, e_bytes);
	BN_bn2bin(n, r.base);

	isc_buffer_add(data, e_bytes + mod_bytes);

	ret = ISC_R_SUCCESS;
err:
	RSA_free(rsa);
	return (ret);
}

static isc_result_t
opensslrsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	RSA *rsa;
	isc_region_t r;
	unsigned int e_bytes;
	unsigned int length;
	EVP_PKEY *pkey;
	BIGNUM *e = NULL, *n = NULL;

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		return (ISC_R_SUCCESS);
	}
	length = r.length;

	rsa = RSA_new();
	if (rsa == NULL) {
		return (dst__openssl_toresult(ISC_R_NOMEMORY));
	}

	if (r.length < 1) {
		RSA_free(rsa);
		return (DST_R_INVALIDPUBLICKEY);
	}
	e_bytes = *r.base;
	isc_region_consume(&r, 1);

	if (e_bytes == 0) {
		if (r.length < 2) {
			RSA_free(rsa);
			return (DST_R_INVALIDPUBLICKEY);
		}
		e_bytes = (*r.base) << 8;
		isc_region_consume(&r, 1);
		e_bytes += *r.base;
		isc_region_consume(&r, 1);
	}

	if (r.length < e_bytes) {
		RSA_free(rsa);
		return (DST_R_INVALIDPUBLICKEY);
	}
	e = BN_bin2bn(r.base, e_bytes, NULL);
	isc_region_consume(&r, e_bytes);
	n = BN_bin2bn(r.base, r.length, NULL);
	if (e == NULL || n == NULL) {
		RSA_free(rsa);
		return (ISC_R_NOMEMORY);
	}

	if (RSA_set0_key(rsa, n, e, NULL) == 0) {
		if (n != NULL) {
			BN_free(n);
		}
		if (e != NULL) {
			BN_free(e);
		}
		RSA_free(rsa);
		return (ISC_R_NOMEMORY);
	}
	key->key_size = BN_num_bits(n);

	isc_buffer_forward(data, length);

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		RSA_free(rsa);
		return (ISC_R_NOMEMORY);
	}
	if (!EVP_PKEY_set1_RSA(pkey, rsa)) {
		EVP_PKEY_free(pkey);
		RSA_free(rsa);
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	key->keydata.pkey = pkey;
	RSA_free(rsa);

	return (ISC_R_SUCCESS);
}

static isc_result_t
opensslrsa_tofile(const dst_key_t *key, const char *directory) {
	int i;
	RSA *rsa;
	dst_private_t priv;
	unsigned char *bufs[8];
	isc_result_t result;
	const BIGNUM *n = NULL, *e = NULL, *d = NULL;
	const BIGNUM *p = NULL, *q = NULL;
	const BIGNUM *dmp1 = NULL, *dmq1 = NULL, *iqmp = NULL;

	if (key->external) {
		priv.nelements = 0;
		return (dst__privstruct_writefile(key, &priv, directory));
	}

	if (key->keydata.pkey == NULL) {
		return (DST_R_NULLKEY);
	}
	rsa = EVP_PKEY_get1_RSA(key->keydata.pkey);
	if (rsa == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	memset(bufs, 0, sizeof(bufs));

	RSA_get0_key(rsa, &n, &e, &d);
	RSA_get0_factors(rsa, &p, &q);
	RSA_get0_crt_params(rsa, &dmp1, &dmq1, &iqmp);

	for (i = 0; i < 8; i++) {
		bufs[i] = isc_mem_get(key->mctx, BN_num_bytes(n));
	}

	i = 0;

	priv.elements[i].tag = TAG_RSA_MODULUS;
	priv.elements[i].length = BN_num_bytes(n);
	BN_bn2bin(n, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_RSA_PUBLICEXPONENT;
	priv.elements[i].length = BN_num_bytes(e);
	BN_bn2bin(e, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	if (d != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIVATEEXPONENT;
		priv.elements[i].length = BN_num_bytes(d);
		BN_bn2bin(d, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (p != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIME1;
		priv.elements[i].length = BN_num_bytes(p);
		BN_bn2bin(p, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (q != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIME2;
		priv.elements[i].length = BN_num_bytes(q);
		BN_bn2bin(q, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (dmp1 != NULL) {
		priv.elements[i].tag = TAG_RSA_EXPONENT1;
		priv.elements[i].length = BN_num_bytes(dmp1);
		BN_bn2bin(dmp1, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (dmq1 != NULL) {
		priv.elements[i].tag = TAG_RSA_EXPONENT2;
		priv.elements[i].length = BN_num_bytes(dmq1);
		BN_bn2bin(dmq1, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (iqmp != NULL) {
		priv.elements[i].tag = TAG_RSA_COEFFICIENT;
		priv.elements[i].length = BN_num_bytes(iqmp);
		BN_bn2bin(iqmp, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (key->engine != NULL) {
		priv.elements[i].tag = TAG_RSA_ENGINE;
		priv.elements[i].length = (unsigned short)strlen(key->engine) +
					  1;
		priv.elements[i].data = (unsigned char *)key->engine;
		i++;
	}

	if (key->label != NULL) {
		priv.elements[i].tag = TAG_RSA_LABEL;
		priv.elements[i].length = (unsigned short)strlen(key->label) +
					  1;
		priv.elements[i].data = (unsigned char *)key->label;
		i++;
	}

	priv.nelements = i;
	result = dst__privstruct_writefile(key, &priv, directory);

	RSA_free(rsa);
	for (i = 0; i < 8; i++) {
		if (bufs[i] == NULL) {
			break;
		}
		isc_mem_put(key->mctx, bufs[i], BN_num_bytes(n));
	}
	return (result);
}

static isc_result_t
rsa_check(RSA *rsa, RSA *pub) {
	const BIGNUM *n1 = NULL, *n2 = NULL;
	const BIGNUM *e1 = NULL, *e2 = NULL;
	BIGNUM *n = NULL, *e = NULL;

	/*
	 * Public parameters should be the same but if they are not set
	 * copy them from the public key.
	 */
	RSA_get0_key(rsa, &n1, &e1, NULL);
	if (pub != NULL) {
		RSA_get0_key(pub, &n2, &e2, NULL);
		if (n1 != NULL) {
			if (BN_cmp(n1, n2) != 0) {
				return (DST_R_INVALIDPRIVATEKEY);
			}
		} else {
			n = BN_dup(n2);
			if (n == NULL) {
				return (ISC_R_NOMEMORY);
			}
		}
		if (e1 != NULL) {
			if (BN_cmp(e1, e2) != 0) {
				if (n != NULL) {
					BN_free(n);
				}
				return (DST_R_INVALIDPRIVATEKEY);
			}
		} else {
			e = BN_dup(e2);
			if (e == NULL) {
				if (n != NULL) {
					BN_free(n);
				}
				return (ISC_R_NOMEMORY);
			}
		}
		if (RSA_set0_key(rsa, n, e, NULL) == 0) {
			if (n != NULL) {
				BN_free(n);
			}
			if (e != NULL) {
				BN_free(e);
			}
		}
	}
	RSA_get0_key(rsa, &n1, &e1, NULL);
	if (n1 == NULL || e1 == NULL) {
		return (DST_R_INVALIDPRIVATEKEY);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
opensslrsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	RSA *rsa = NULL, *pubrsa = NULL;
#if !defined(OPENSSL_NO_ENGINE)
	ENGINE *ep = NULL;
	const BIGNUM *ex = NULL;
#endif /* if !defined(OPENSSL_NO_ENGINE) */
	isc_mem_t *mctx = key->mctx;
	const char *engine = NULL, *label = NULL;
	EVP_PKEY *pkey = NULL;
	BIGNUM *n = NULL, *e = NULL, *d = NULL;
	BIGNUM *p = NULL, *q = NULL;
	BIGNUM *dmp1 = NULL, *dmq1 = NULL, *iqmp = NULL;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_RSA, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	if (key->external) {
		if (priv.nelements != 0) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		if (pub == NULL) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		key->keydata.pkey = pub->keydata.pkey;
		pub->keydata.pkey = NULL;
		key->key_size = pub->key_size;
		dst__privstruct_free(&priv, mctx);
		isc_safe_memwipe(&priv, sizeof(priv));
		return (ISC_R_SUCCESS);
	}

	if (pub != NULL && pub->keydata.pkey != NULL) {
		pubrsa = EVP_PKEY_get1_RSA(pub->keydata.pkey);
	}

	for (i = 0; i < priv.nelements; i++) {
		switch (priv.elements[i].tag) {
		case TAG_RSA_ENGINE:
			engine = (char *)priv.elements[i].data;
			break;
		case TAG_RSA_LABEL:
			label = (char *)priv.elements[i].data;
			break;
		default:
			break;
		}
	}

	/*
	 * Is this key is stored in a HSM?
	 * See if we can fetch it.
	 */
	if (label != NULL) {
#if !defined(OPENSSL_NO_ENGINE)
		if (engine == NULL) {
			DST_RET(DST_R_NOENGINE);
		}
		ep = dst__openssl_getengine(engine);
		if (ep == NULL) {
			DST_RET(DST_R_NOENGINE);
		}
		pkey = ENGINE_load_private_key(ep, label, NULL, NULL);
		if (pkey == NULL) {
			DST_RET(dst__openssl_toresult2("ENGINE_load_private_"
						       "key",
						       ISC_R_NOTFOUND));
		}
		key->engine = isc_mem_strdup(key->mctx, engine);
		key->label = isc_mem_strdup(key->mctx, label);
		rsa = EVP_PKEY_get1_RSA(pkey);
		if (rsa == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
		if (rsa_check(rsa, pubrsa) != ISC_R_SUCCESS) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		RSA_get0_key(rsa, NULL, &ex, NULL);
		if (BN_num_bits(ex) > RSA_MAX_PUBEXP_BITS) {
			DST_RET(ISC_R_RANGE);
		}
		if (pubrsa != NULL) {
			RSA_free(pubrsa);
		}
		key->key_size = EVP_PKEY_bits(pkey);
		key->keydata.pkey = pkey;
		RSA_free(rsa);
		dst__privstruct_free(&priv, mctx);
		isc_safe_memwipe(&priv, sizeof(priv));
		return (ISC_R_SUCCESS);
#else  /* if !defined(OPENSSL_NO_ENGINE) */
		DST_RET(DST_R_NOENGINE);
#endif /* if !defined(OPENSSL_NO_ENGINE) */
	}

	rsa = RSA_new();
	if (rsa == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	if (!EVP_PKEY_set1_RSA(pkey, rsa)) {
		DST_RET(ISC_R_FAILURE);
	}
	key->keydata.pkey = pkey;

	for (i = 0; i < priv.nelements; i++) {
		BIGNUM *bn;
		switch (priv.elements[i].tag) {
		case TAG_RSA_ENGINE:
			continue;
		case TAG_RSA_LABEL:
			continue;
		default:
			bn = BN_bin2bn(priv.elements[i].data,
				       priv.elements[i].length, NULL);
			if (bn == NULL) {
				DST_RET(ISC_R_NOMEMORY);
			}
			switch (priv.elements[i].tag) {
			case TAG_RSA_MODULUS:
				n = bn;
				break;
			case TAG_RSA_PUBLICEXPONENT:
				e = bn;
				break;
			case TAG_RSA_PRIVATEEXPONENT:
				d = bn;
				break;
			case TAG_RSA_PRIME1:
				p = bn;
				break;
			case TAG_RSA_PRIME2:
				q = bn;
				break;
			case TAG_RSA_EXPONENT1:
				dmp1 = bn;
				break;
			case TAG_RSA_EXPONENT2:
				dmq1 = bn;
				break;
			case TAG_RSA_COEFFICIENT:
				iqmp = bn;
				break;
			}
		}
	}
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));

	if (RSA_set0_key(rsa, n, e, d) == 0) {
		if (n != NULL) {
			BN_free(n);
		}
		if (e != NULL) {
			BN_free(e);
		}
		if (d != NULL) {
			BN_free(d);
		}
	}
	if (RSA_set0_factors(rsa, p, q) == 0) {
		if (p != NULL) {
			BN_free(p);
		}
		if (q != NULL) {
			BN_free(q);
		}
	}
	if (RSA_set0_crt_params(rsa, dmp1, dmq1, iqmp) == 0) {
		if (dmp1 != NULL) {
			BN_free(dmp1);
		}
		if (dmq1 != NULL) {
			BN_free(dmq1);
		}
		if (iqmp != NULL) {
			BN_free(iqmp);
		}
	}

	if (rsa_check(rsa, pubrsa) != ISC_R_SUCCESS) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}
	if (BN_num_bits(e) > RSA_MAX_PUBEXP_BITS) {
		DST_RET(ISC_R_RANGE);
	}
	key->key_size = BN_num_bits(n);
	if (pubrsa != NULL) {
		RSA_free(pubrsa);
	}
	RSA_free(rsa);

	return (ISC_R_SUCCESS);

err:
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (rsa != NULL) {
		RSA_free(rsa);
	}
	if (pubrsa != NULL) {
		RSA_free(pubrsa);
	}
	key->keydata.generic = NULL;
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (ret);
}

static isc_result_t
opensslrsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		     const char *pin) {
#if !defined(OPENSSL_NO_ENGINE)
	ENGINE *e = NULL;
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	RSA *rsa = NULL, *pubrsa = NULL;
	const BIGNUM *ex = NULL;

	UNUSED(pin);

	if (engine == NULL) {
		DST_RET(DST_R_NOENGINE);
	}
	e = dst__openssl_getengine(engine);
	if (e == NULL) {
		DST_RET(DST_R_NOENGINE);
	}
	pkey = ENGINE_load_public_key(e, label, NULL, NULL);
	if (pkey != NULL) {
		pubrsa = EVP_PKEY_get1_RSA(pkey);
		EVP_PKEY_free(pkey);
		if (pubrsa == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
	}
	pkey = ENGINE_load_private_key(e, label, NULL, NULL);
	if (pkey == NULL) {
		DST_RET(dst__openssl_toresult2("ENGINE_load_private_key",
					       ISC_R_NOTFOUND));
	}
	key->engine = isc_mem_strdup(key->mctx, engine);
	key->label = isc_mem_strdup(key->mctx, label);
	rsa = EVP_PKEY_get1_RSA(pkey);
	if (rsa == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	if (rsa_check(rsa, pubrsa) != ISC_R_SUCCESS) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}
	RSA_get0_key(rsa, NULL, &ex, NULL);
	if (BN_num_bits(ex) > RSA_MAX_PUBEXP_BITS) {
		DST_RET(ISC_R_RANGE);
	}
	if (pubrsa != NULL) {
		RSA_free(pubrsa);
	}
	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkey = pkey;
	RSA_free(rsa);
	return (ISC_R_SUCCESS);

err:
	if (rsa != NULL) {
		RSA_free(rsa);
	}
	if (pubrsa != NULL) {
		RSA_free(pubrsa);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	return (ret);
#else  /* if !defined(OPENSSL_NO_ENGINE) */
	UNUSED(key);
	UNUSED(engine);
	UNUSED(label);
	UNUSED(pin);
	return (DST_R_NOENGINE);
#endif /* if !defined(OPENSSL_NO_ENGINE) */
}

static dst_func_t opensslrsa_functions = {
	opensslrsa_createctx,
	NULL, /*%< createctx2 */
	opensslrsa_destroyctx,
	opensslrsa_adddata,
	opensslrsa_sign,
	opensslrsa_verify,
	opensslrsa_verify2,
	NULL, /*%< computesecret */
	opensslrsa_compare,
	NULL, /*%< paramcompare */
	opensslrsa_generate,
	opensslrsa_isprivate,
	opensslrsa_destroy,
	opensslrsa_todns,
	opensslrsa_fromdns,
	opensslrsa_tofile,
	opensslrsa_parse,
	NULL, /*%< cleanup */
	opensslrsa_fromlabel,
	NULL, /*%< dump */
	NULL, /*%< restore */
};

/*
 * An RSA public key with 2048 bits
 */
static const unsigned char e_bytes[] = "\x01\x00\x01";
static const unsigned char n_bytes[] =
	"\xc3\x90\x07\xbe\xf1\x85\xfc\x1a\x43\xb1\xa5\x15\xce\x71\x34\xfc\xc1"
	"\x87\x27\x28\x38\xa4\xcf\x7c\x1a\x82\xa8\xdc\x04\x14\xd0\x3f\xb4\xfe"
	"\x20\x4a\xdd\xd9\x0d\xd7\xcd\x61\x8c\xbd\x61\xa8\x10\xb5\x63\x1c\x29"
	"\x15\xcb\x41\xee\x43\x91\x7f\xeb\xa5\x2c\xab\x81\x75\x0d\xa3\x3d\xe4"
	"\xc8\x49\xb9\xca\x5a\x55\xa1\xbb\x09\xd1\xfb\xcd\xa2\xd2\x12\xa4\x85"
	"\xdf\xa5\x65\xc9\x27\x2d\x8b\xd7\x8b\xfe\x6d\xc4\xd1\xd9\x83\x1c\x91"
	"\x7d\x3d\xd0\xa4\xcd\xe1\xe7\xb9\x7a\x11\x38\xf9\x8b\x3c\xec\x30\xb6"
	"\x36\xb9\x92\x64\x81\x56\x3c\xbc\xf9\x49\xfb\xba\x82\xb7\xa0\xfa\x65"
	"\x79\x83\xb9\x4c\xa7\xfd\x53\x0b\x5a\xe4\xde\xf9\xfc\x38\x7e\xb5\x2c"
	"\xa0\xc3\xb2\xfc\x7c\x38\xb0\x63\x50\xaf\x00\xaa\xb2\xad\x49\x54\x1e"
	"\x8b\x11\x88\x9b\x6e\xae\x3b\x23\xa3\xdd\x53\x51\x80\x7a\x0b\x91\x4e"
	"\x6d\x32\x01\xbd\x17\x81\x12\x64\x9f\x84\xae\x76\x53\x1a\x63\xa0\xda"
	"\xcc\x45\x04\x72\xb0\xa7\xfb\xfa\x02\x39\x53\xc1\x83\x1f\x88\x54\x47"
	"\x88\x63\x20\x71\x5d\xe2\xaa\x7c\x53\x39\x5e\x35\x25\xee\xe6\x5c\x15"
	"\x5e\x14\xbe\x99\xde\x25\x19\xe7\x13\xdb\xce\xa3\xd3\x6c\x5c\xbb\x0e"
	"\x6b";

static const unsigned char sha1_sig[] =
	"\x69\x99\x89\x28\xe0\x38\x34\x91\x29\xb6\xac\x4b\xe9\x51\xbd\xbe\xc8"
	"\x1a\x2d\xb6\xca\x99\xa3\x9f\x6a\x8b\x94\x5a\x51\x37\xd5\x8d\xae\x87"
	"\xed\xbc\x8e\xb8\xa3\x60\x6b\xf6\xe6\x72\xfc\x26\x2a\x39\x2b\xfe\x88"
	"\x1a\xa9\xd1\x93\xc7\xb9\xf8\xb6\x45\xa1\xf9\xa1\x56\x78\x7b\x00\xec"
	"\x33\x83\xd4\x93\x25\x48\xb3\x50\x09\xd0\xbc\x7f\xac\x67\xc7\xa2\x7f"
	"\xfc\xf6\x5a\xef\xf8\x5a\xad\x52\x74\xf5\x71\x34\xd9\x3d\x33\x8b\x4d"
	"\x99\x64\x7e\x14\x59\xbe\xdf\x26\x8a\x67\x96\x6c\x1f\x79\x85\x10\x0d"
	"\x7f\xd6\xa4\xba\x57\x41\x03\x71\x4e\x8c\x17\xd5\xc4\xfb\x4a\xbe\x66"
	"\x45\x15\x45\x0c\x02\xe0\x10\xe1\xbb\x33\x8d\x90\x34\x3c\x94\xa4\x4c"
	"\x7c\xd0\x5e\x90\x76\x80\x59\xb2\xfa\x54\xbf\xa9\x86\xb8\x84\x1e\x28"
	"\x48\x60\x2f\x9e\xa4\xbc\xd4\x9c\x20\x27\x16\xac\x33\xcb\xcf\xab\x93"
	"\x7a\x3b\x74\xa0\x18\x92\xa1\x4f\xfc\x52\x19\xee\x7a\x13\x73\xba\x36"
	"\xaf\x78\x5d\xb6\x1f\x96\x76\x15\x73\xee\x04\xa8\x70\x27\xf7\xe7\xfa"
	"\xe8\xf6\xc8\x5f\x4a\x81\x56\x0a\x94\xf3\xc6\x98\xd2\x93\xc4\x0b\x49"
	"\x6b\x44\xd3\x73\xa2\xe3\xef\x5d\x9e\x68\xac\xa7\x42\xb1\xbb\x65\xbe"
	"\x59";

static const unsigned char sha256_sig[] =
	"\x0f\x8c\xdb\xe6\xb6\x21\xc8\xc5\x28\x76\x7d\xf6\xf2\x3b\x78\x47\x77"
	"\x03\x34\xc5\x5e\xc0\xda\x42\x41\xc0\x0f\x97\xd3\xd0\x53\xa1\xd6\x87"
	"\xe4\x16\x29\x9a\xa5\x59\xf4\x01\xad\xc9\x04\xe7\x61\xe2\xcb\x79\x73"
	"\xce\xe0\xa6\x85\xe5\x10\x8c\x4b\xc5\x68\x3b\x96\x42\x3f\x56\xb3\x6d"
	"\x89\xc4\xff\x72\x36\xf2\x3f\xed\xe9\xb8\xe3\xae\xab\x3c\xb7\xaa\xf7"
	"\x1f\x8f\x26\x6b\xee\xc1\xac\x72\x89\x23\x8b\x7a\xd7\x8c\x84\xf3\xf5"
	"\x97\xa8\x8d\xd3\xef\xb2\x5e\x06\x04\x21\xdd\x28\xa2\x28\x83\x68\x9b"
	"\xac\x34\xdd\x36\x33\xda\xdd\xa4\x59\xc7\x5a\x4d\xf3\x83\x06\xd5\xc0"
	"\x0d\x1f\x4f\x47\x2f\x9f\xcc\xc2\x0d\x21\x1e\x82\xb9\x3d\xf3\xa4\x1a"
	"\xa6\xd8\x0e\x72\x1d\x71\x17\x1c\x54\xad\x37\x3e\xa4\x0e\x70\x86\x53"
	"\xfb\x40\xad\xb9\x14\xf8\x8d\x93\xbb\xd7\xe7\x31\xce\xe0\x98\xda\x27"
	"\x1c\x18\x8e\xd8\x85\xcb\xa7\xb1\x18\xac\x8c\xa8\x9d\xa9\xe2\xf6\x30"
	"\x95\xa4\x81\xf4\x1c\xa0\x31\xd5\xc7\x9d\x28\x33\xee\x7f\x08\x4f\xcb"
	"\xd1\x14\x17\xdf\xd0\x88\x78\x47\x29\xaf\x6c\xb2\x62\xa6\x30\x87\x29"
	"\xaa\x80\x19\x7d\x2f\x05\xe3\x7e\x23\x73\x88\x08\xcc\xbd\x50\x46\x09"
	"\x2a";

static const unsigned char sha512_sig[] =
	"\x15\xda\x87\x87\x1f\x76\x08\xd3\x9d\x3a\xb9\xd2\x6a\x0e\x3b\x7d\xdd"
	"\xec\x7d\xc4\x6d\x26\xf5\x04\xd3\x76\xc7\x83\xc4\x81\x69\x35\xe9\x47"
	"\xbf\x49\xd1\xc0\xf9\x01\x4e\x0a\x34\x5b\xd0\xec\x6e\xe2\x2e\xe9\x2d"
	"\x00\xfd\xe0\xa0\x28\x54\x53\x19\x49\x6d\xd2\x58\xb9\x47\xfa\x45\xad"
	"\xd2\x1d\x52\xac\x80\xcb\xfc\x91\x97\x84\x58\x5f\xab\x21\x62\x60\x79"
	"\xb8\x8a\x83\xe1\xf1\xcb\x05\x4c\x92\x56\x62\xd9\xbf\xa7\x81\x34\x23"
	"\xdf\xd7\xa7\xc4\xdf\xde\x96\x00\x57\x4b\x78\x85\xb9\x3b\xdd\x3f\x98"
	"\x88\x59\x1d\x48\xcf\x5a\xa8\xb7\x2a\x8b\x77\x93\x8e\x38\x3a\x0c\xa7"
	"\x8a\x5f\xe6\x9f\xcb\xf0\x9a\x6b\xb6\x91\x04\x8b\x69\x6a\x37\xee\xa2"
	"\xad\x5f\x31\x20\x96\xd6\x51\x80\xbf\x62\x48\xb8\xe4\x94\x10\x86\x4e"
	"\xf2\x22\x1e\xa4\xd5\x54\xfe\xe1\x35\x49\xaf\xf8\x62\xfc\x11\xeb\xf7"
	"\x3d\xd5\x5e\xaf\x11\xbd\x3d\xa9\x3a\x9f\x7f\xe8\xb4\x0d\xa2\xbb\x1c"
	"\xbd\x4c\xed\x9e\x81\xb1\xec\xd3\xea\xaa\x03\xe3\x14\xdf\x8c\xb3\x78"
	"\x85\x5e\x87\xad\xec\x41\x1a\xa9\x4f\xd2\xe6\xc6\xbe\xfa\xb8\x10\xea"
	"\x74\x25\x36\x0c\x23\xe2\x24\xb7\x21\xb7\x0d\xaf\xf6\xb4\x31\xf5\x75"
	"\xf1";

static isc_result_t
check_algorithm(unsigned char algorithm) {
	BIGNUM *n = NULL, *e = NULL;
	EVP_MD_CTX *evp_md_ctx = EVP_MD_CTX_create();
	EVP_PKEY *pkey = NULL;
	const EVP_MD *type = NULL;
	const unsigned char *sig = NULL;
	int status;
	isc_result_t ret = ISC_R_SUCCESS;
	size_t len;
	RSA *rsa = NULL;

	if (evp_md_ctx == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}

	switch (algorithm) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		type = EVP_sha1(); /* SHA1 + RSA */
		sig = sha1_sig;
		len = sizeof(sha1_sig) - 1;
		break;
	case DST_ALG_RSASHA256:
		type = EVP_sha256(); /* SHA256 + RSA */
		sig = sha256_sig;
		len = sizeof(sha256_sig) - 1;
		break;
	case DST_ALG_RSASHA512:
		type = EVP_sha512();
		sig = sha512_sig;
		len = sizeof(sha512_sig) - 1;
		break;
	default:
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

	if (type == NULL) {
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

	/*
	 * Construct pkey.
	 */
	e = BN_bin2bn(e_bytes, sizeof(e_bytes) - 1, NULL);
	n = BN_bin2bn(n_bytes, sizeof(n_bytes) - 1, NULL);
	if (e == NULL || n == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}

	rsa = RSA_new();
	if (rsa == NULL) {
		DST_RET(dst__openssl_toresult2("RSA_new",
					       DST_R_OPENSSLFAILURE));
	}
	status = RSA_set0_key(rsa, n, e, NULL);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("RSA_set0_key",
					       DST_R_OPENSSLFAILURE));
	}

	/* These are now managed by OpenSSL. */
	n = NULL;
	e = NULL;

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_new",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_set1_RSA(pkey, rsa);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_set1_RSA",
					       DST_R_OPENSSLFAILURE));
	}

	/*
	 * Check that we can verify the signature.
	 */
	if (EVP_DigestInit_ex(evp_md_ctx, type, NULL) != 1 ||
	    EVP_DigestUpdate(evp_md_ctx, "test", 4) != 1 ||
	    EVP_VerifyFinal(evp_md_ctx, sig, len, pkey) != 1)
	{
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

err:
	BN_free(e);
	BN_free(n);
	if (rsa != NULL) {
		RSA_free(rsa);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
	}
	ERR_clear_error();
	return (ret);
}

isc_result_t
dst__opensslrsa_init(dst_func_t **funcp, unsigned char algorithm) {
	isc_result_t result;

	REQUIRE(funcp != NULL);

	result = check_algorithm(algorithm);

	if (result == ISC_R_SUCCESS) {
		if (*funcp == NULL) {
			*funcp = &opensslrsa_functions;
		}
	} else if (result == ISC_R_NOTIMPLEMENTED) {
		result = ISC_R_SUCCESS;
	}

	return (result);
}

#endif /* !USE_PKCS11 */

/*! \file */
