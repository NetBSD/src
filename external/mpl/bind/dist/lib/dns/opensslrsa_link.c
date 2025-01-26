/*	$NetBSD: opensslrsa_link.c,v 1.12 2025/01/26 16:25:24 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/opensslv.h>
#include <openssl/rsa.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#endif

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"
#include "openssl_shim.h"

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

typedef struct rsa_components {
	bool bnfree;
	const BIGNUM *e, *n, *d, *p, *q, *dmp1, *dmq1, *iqmp;
} rsa_components_t;

static isc_result_t
opensslrsa_components_get(const dst_key_t *key, rsa_components_t *c,
			  bool private) {
	REQUIRE(c->e == NULL && c->n == NULL && c->d == NULL && c->p == NULL &&
		c->q == NULL && c->dmp1 == NULL && c->dmq1 == NULL &&
		c->iqmp == NULL);

	EVP_PKEY *pub = key->keydata.pkeypair.pub;
	EVP_PKEY *priv = key->keydata.pkeypair.priv;

	if (private && priv == NULL) {
		return DST_R_INVALIDPRIVATEKEY;
	}
	/*
	 * NOTE: Errors regarding private compoments are ignored.
	 *
	 * OpenSSL allows omitting the parameters for CRT based calculations
	 * (factors, exponents, coefficients). Only the 'd'  parameter is
	 * mandatory for software keys.
	 *
	 * However, for a label based keys, all private key component queries
	 * can fail if they key is e.g. on a hardware device.
	 */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (EVP_PKEY_get_bn_param(pub, OSSL_PKEY_PARAM_RSA_E,
				  (BIGNUM **)&c->e) == 1)
	{
		c->bnfree = true;
		if (EVP_PKEY_get_bn_param(pub, OSSL_PKEY_PARAM_RSA_N,
					  (BIGNUM **)&c->n) != 1)
		{
			return dst__openssl_toresult(DST_R_OPENSSLFAILURE);
		}
		if (!private) {
			return ISC_R_SUCCESS;
		}
		(void)EVP_PKEY_get_bn_param(priv, OSSL_PKEY_PARAM_RSA_D,
					    (BIGNUM **)&c->d);
		(void)EVP_PKEY_get_bn_param(priv, OSSL_PKEY_PARAM_RSA_FACTOR1,
					    (BIGNUM **)&c->p);
		(void)EVP_PKEY_get_bn_param(priv, OSSL_PKEY_PARAM_RSA_FACTOR2,
					    (BIGNUM **)&c->q);
		(void)EVP_PKEY_get_bn_param(priv, OSSL_PKEY_PARAM_RSA_EXPONENT1,
					    (BIGNUM **)&c->dmp1);
		(void)EVP_PKEY_get_bn_param(priv, OSSL_PKEY_PARAM_RSA_EXPONENT2,
					    (BIGNUM **)&c->dmq1);
		(void)EVP_PKEY_get_bn_param(priv,
					    OSSL_PKEY_PARAM_RSA_COEFFICIENT1,
					    (BIGNUM **)&c->iqmp);
		ERR_clear_error();
		return ISC_R_SUCCESS;
	} else {
		ERR_clear_error();
	}
#endif
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	const RSA *rsa = EVP_PKEY_get0_RSA(pub);
	if (rsa == NULL) {
		return dst__openssl_toresult(DST_R_OPENSSLFAILURE);
	}
	RSA_get0_key(rsa, &c->n, &c->e, &c->d);
	if (c->e == NULL || c->n == NULL) {
		return dst__openssl_toresult(DST_R_OPENSSLFAILURE);
	}
	if (!private) {
		return ISC_R_SUCCESS;
	}
	rsa = EVP_PKEY_get0_RSA(priv);
	if (rsa == NULL) {
		return dst__openssl_toresult(DST_R_OPENSSLFAILURE);
	}
	RSA_get0_factors(rsa, &c->p, &c->q);
	RSA_get0_crt_params(rsa, &c->dmp1, &c->dmq1, &c->iqmp);
	return ISC_R_SUCCESS;
#else
	return DST_R_OPENSSLFAILURE;
#endif
}

static void
opensslrsa_components_free(rsa_components_t *c) {
	if (!c->bnfree) {
		return;
	}
	/*
	 * NOTE: BN_free() frees the components of the BIGNUM, and if it was
	 * created by BN_new(), also the structure itself. BN_clear_free()
	 * additionally overwrites the data before the memory is returned to the
	 * system. If a is NULL, nothing is done.
	 */
	BN_free((BIGNUM *)c->e);
	BN_free((BIGNUM *)c->n);
	BN_clear_free((BIGNUM *)c->d);
	BN_clear_free((BIGNUM *)c->p);
	BN_clear_free((BIGNUM *)c->q);
	BN_clear_free((BIGNUM *)c->dmp1);
	BN_clear_free((BIGNUM *)c->dmq1);
	BN_clear_free((BIGNUM *)c->iqmp);
	c->bnfree = false;
}

static bool
opensslrsa_valid_key_alg(unsigned int key_alg) {
	switch (key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
	case DST_ALG_RSASHA256:
	case DST_ALG_RSASHA512:
		return true;
	default:
		return false;
	}
}

static isc_result_t
opensslrsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx;
	const EVP_MD *type = NULL;

	UNUSED(key);
	REQUIRE(dctx != NULL && dctx->key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(dctx->key->key_alg));

	/*
	 * Reject incorrect RSA key lengths.
	 */
	switch (dctx->key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		/* From RFC 3110 */
		if (dctx->key->key_size > 4096) {
			return ISC_R_FAILURE;
		}
		break;
	case DST_ALG_RSASHA256:
		/* From RFC 5702 */
		if (dctx->key->key_size < 512 || dctx->key->key_size > 4096) {
			return ISC_R_FAILURE;
		}
		break;
	case DST_ALG_RSASHA512:
		/* From RFC 5702 */
		if (dctx->key->key_size < 1024 || dctx->key->key_size > 4096) {
			return ISC_R_FAILURE;
		}
		break;
	default:
		UNREACHABLE();
	}

	evp_md_ctx = EVP_MD_CTX_create();
	if (evp_md_ctx == NULL) {
		return dst__openssl_toresult(ISC_R_NOMEMORY);
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
		return dst__openssl_toresult3(
			dctx->category, "EVP_DigestInit_ex", ISC_R_FAILURE);
	}
	dctx->ctxdata.evp_md_ctx = evp_md_ctx;

	return ISC_R_SUCCESS;
}

static void
opensslrsa_destroyctx(dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx = NULL;

	REQUIRE(dctx != NULL && dctx->key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(dctx->key->key_alg));

	evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		dctx->ctxdata.evp_md_ctx = NULL;
	}
}

static isc_result_t
opensslrsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
	EVP_MD_CTX *evp_md_ctx = NULL;

	REQUIRE(dctx != NULL && dctx->key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(dctx->key->key_alg));

	evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	if (!EVP_DigestUpdate(evp_md_ctx, data->base, data->length)) {
		return dst__openssl_toresult3(
			dctx->category, "EVP_DigestUpdate", ISC_R_FAILURE);
	}
	return ISC_R_SUCCESS;
}

static isc_result_t
opensslrsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_key_t *key = NULL;
	isc_region_t r;
	unsigned int siglen = 0;
	EVP_MD_CTX *evp_md_ctx = NULL;
	EVP_PKEY *pkey = NULL;

	REQUIRE(dctx != NULL && dctx->key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(dctx->key->key_alg));

	key = dctx->key;
	evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	pkey = key->keydata.pkeypair.priv;

	isc_buffer_availableregion(sig, &r);

	if (r.length < (unsigned int)EVP_PKEY_size(pkey)) {
		return ISC_R_NOSPACE;
	}

	if (!EVP_SignFinal(evp_md_ctx, r.base, &siglen, pkey)) {
		return dst__openssl_toresult3(dctx->category, "EVP_SignFinal",
					      ISC_R_FAILURE);
	}

	isc_buffer_add(sig, siglen);

	return ISC_R_SUCCESS;
}

static bool
opensslrsa_check_exponent_bits(EVP_PKEY *pkey, int maxbits) {
	/* Always use the new API first with OpenSSL 3.x. */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	BIGNUM *e = NULL;
	if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &e) == 1) {
		int bits = BN_num_bits(e);
		BN_free(e);
		return bits < maxbits;
	}
#endif
	/* Use old API for the OpenSSL ENGINE support, even with OpenSSL 3.x */
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	const RSA *rsa = EVP_PKEY_get0_RSA(pkey);
	if (rsa != NULL) {
		const BIGNUM *ce = NULL;
		RSA_get0_key(rsa, NULL, &ce, NULL);
		if (ce != NULL) {
			return BN_num_bits(ce) < maxbits;
		}
	}
#endif
	return false;
}

static isc_result_t
opensslrsa_verify2(dst_context_t *dctx, int maxbits, const isc_region_t *sig) {
	dst_key_t *key = NULL;
	int status = 0;
	EVP_MD_CTX *evp_md_ctx = NULL;
	EVP_PKEY *pkey = NULL;

	REQUIRE(dctx != NULL && dctx->key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(dctx->key->key_alg));

	key = dctx->key;
	evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	pkey = key->keydata.pkeypair.pub;

	if (maxbits != 0 && !opensslrsa_check_exponent_bits(pkey, maxbits)) {
		return DST_R_VERIFYFAILURE;
	}

	status = EVP_VerifyFinal(evp_md_ctx, sig->base, sig->length, pkey);
	switch (status) {
	case 1:
		return ISC_R_SUCCESS;
	case 0:
		return dst__openssl_toresult(DST_R_VERIFYFAILURE);
	default:
		return dst__openssl_toresult3(dctx->category, "EVP_VerifyFinal",
					      DST_R_VERIFYFAILURE);
	}
}

static isc_result_t
opensslrsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	return opensslrsa_verify2(dctx, 0, sig);
}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
static int
progress_cb(int p, int n, BN_GENCB *cb) {
	void (*fptr)(int);

	UNUSED(n);

	fptr = BN_GENCB_get_arg(cb);
	if (fptr != NULL) {
		fptr(p);
	}
	return 1;
}

static isc_result_t
opensslrsa_generate_pkey(unsigned int key_size, const char *label, BIGNUM *e,
			 void (*callback)(int), EVP_PKEY **retkey) {
	RSA *rsa = NULL;
	EVP_PKEY *pkey = NULL;
	BN_GENCB *cb = NULL;
	isc_result_t ret;

	UNUSED(label);

	rsa = RSA_new();
	pkey = EVP_PKEY_new();
	if (rsa == NULL || pkey == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (EVP_PKEY_set1_RSA(pkey, rsa) != 1) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (callback != NULL) {
		cb = BN_GENCB_new();
		if (cb == NULL) {
			DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
		}
		BN_GENCB_set(cb, progress_cb, (void *)callback);
	}

	if (RSA_generate_key_ex(rsa, key_size, e, cb) != 1) {
		DST_RET(dst__openssl_toresult2("RSA_generate_key_ex",
					       DST_R_OPENSSLFAILURE));
	}
	*retkey = pkey;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_free(pkey);
	RSA_free(rsa);
	BN_GENCB_free(cb);
	return ret;
}

static isc_result_t
opensslrsa_build_pkey(bool private, rsa_components_t *c, EVP_PKEY **retpkey) {
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	RSA *rsa = RSA_new();
	int status;

	REQUIRE(c->bnfree);

	if (c->n == NULL || c->e == NULL) {
		if (private) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}

	if (rsa == NULL) {
		DST_RET(dst__openssl_toresult2("RSA_new",
					       DST_R_OPENSSLFAILURE));
	}

	if (RSA_set0_key(rsa, (BIGNUM *)c->n, (BIGNUM *)c->e, (BIGNUM *)c->d) !=
	    1)
	{
		DST_RET(dst__openssl_toresult2("RSA_set0_key",
					       DST_R_OPENSSLFAILURE));
	}
	c->n = NULL;
	c->e = NULL;
	c->d = NULL;

	if (c->p != NULL || c->q != NULL) {
		if (RSA_set0_factors(rsa, (BIGNUM *)c->p, (BIGNUM *)c->q) != 1)
		{
			DST_RET(dst__openssl_toresult2("RSA_set0_factors",
						       DST_R_OPENSSLFAILURE));
		}
		c->p = NULL;
		c->q = NULL;
	}

	if (c->dmp1 != NULL || c->dmq1 != NULL || c->iqmp != NULL) {
		if (RSA_set0_crt_params(rsa, (BIGNUM *)c->dmp1,
					(BIGNUM *)c->dmq1,
					(BIGNUM *)c->iqmp) == 0)
		{
			DST_RET(dst__openssl_toresult2("RSA_set0_crt_params",
						       DST_R_OPENSSLFAILURE));
		}
		c->dmp1 = NULL;
		c->dmq1 = NULL;
		c->iqmp = NULL;
	}

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

	*retpkey = pkey;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_free(pkey);
	RSA_free(rsa);
	opensslrsa_components_free(c);
	return ret;
}
#else
static int
progress_cb(EVP_PKEY_CTX *ctx) {
	void (*fptr)(int);

	fptr = EVP_PKEY_CTX_get_app_data(ctx);
	if (fptr != NULL) {
		int p = EVP_PKEY_CTX_get_keygen_info(ctx, 0);
		fptr(p);
	}
	return 1;
}

static isc_result_t
opensslrsa_generate_pkey_with_uri(size_t key_size, const char *label,
				  EVP_PKEY **retkey) {
	EVP_PKEY_CTX *ctx = NULL;
	OSSL_PARAM params[4];
	char *uri = UNCONST(label);
	isc_result_t ret;
	int status;

	params[0] = OSSL_PARAM_construct_utf8_string("pkcs11_uri", uri, 0);
	params[1] = OSSL_PARAM_construct_utf8_string(
		"pkcs11_key_usage", (char *)"digitalSignature", 0);
	params[2] = OSSL_PARAM_construct_size_t("rsa_keygen_bits", &key_size);
	params[3] = OSSL_PARAM_construct_end();

	ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", "provider=pkcs11");
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new_from_name",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_keygen_init(ctx);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen_init",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_CTX_set_params(ctx, params);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_set_params",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_generate(ctx, retkey);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_generate",
					       DST_R_OPENSSLFAILURE));
	}

	ret = ISC_R_SUCCESS;
err:
	EVP_PKEY_CTX_free(ctx);
	return ret;
}

static isc_result_t
opensslrsa_generate_pkey(unsigned int key_size, const char *label, BIGNUM *e,
			 void (*callback)(int), EVP_PKEY **retkey) {
	EVP_PKEY_CTX *ctx;
	isc_result_t ret;

	if (label != NULL) {
		return opensslrsa_generate_pkey_with_uri(key_size, label,
							 retkey);
	}

	ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (EVP_PKEY_keygen_init(ctx) != 1) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, (int)key_size) != 1) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (EVP_PKEY_CTX_set1_rsa_keygen_pubexp(ctx, e) != 1) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (callback != NULL) {
		EVP_PKEY_CTX_set_app_data(ctx, (void *)callback);
		EVP_PKEY_CTX_set_cb(ctx, progress_cb);
	}

	if (EVP_PKEY_keygen(ctx, retkey) != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen",
					       DST_R_OPENSSLFAILURE));
	}
	ret = ISC_R_SUCCESS;
err:
	EVP_PKEY_CTX_free(ctx);
	return ret;
}

static isc_result_t
opensslrsa_build_pkey(bool private, rsa_components_t *c, EVP_PKEY **retpkey) {
	isc_result_t ret;
	int status;
	OSSL_PARAM_BLD *bld = NULL;
	OSSL_PARAM *params = NULL;
	EVP_PKEY_CTX *ctx = NULL;

	bld = OSSL_PARAM_BLD_new();
	if (bld == NULL) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_new",
					       DST_R_OPENSSLFAILURE));
	}
	if (OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, c->n) != 1 ||
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, c->e) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}

	if (c->d != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_D, c->d) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	if (c->p != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_FACTOR1, c->p) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	if (c->q != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_FACTOR2, c->q) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	if (c->dmp1 != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_EXPONENT1,
				   c->dmp1) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	if (c->dmq1 != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_EXPONENT2,
				   c->dmq1) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	if (c->iqmp != NULL &&
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_COEFFICIENT1,
				   c->iqmp) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}

	params = OSSL_PARAM_BLD_to_param(bld);
	if (params == NULL) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_to_param",
					       DST_R_OPENSSLFAILURE));
	}
	ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new_from_name",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_fromdata_init(ctx);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata_init",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_fromdata(
		ctx, retpkey, private ? EVP_PKEY_KEYPAIR : EVP_PKEY_PUBLIC_KEY,
		params);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata",
					       DST_R_OPENSSLFAILURE));
	}
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_CTX_free(ctx);
	OSSL_PARAM_free(params);
	OSSL_PARAM_BLD_free(bld);
	return ret;
}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

static isc_result_t
opensslrsa_generate(dst_key_t *key, int exp, void (*callback)(int)) {
	isc_result_t ret;
	BIGNUM *e = BN_new();
	EVP_PKEY *pkey = NULL;

	if (e == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	/*
	 * Reject incorrect RSA key lengths.
	 */
	switch (key->key_alg) {
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		/* From RFC 3110 */
		if (key->key_size > 4096) {
			DST_RET(DST_R_INVALIDPARAM);
		}
		break;
	case DST_ALG_RSASHA256:
		/* From RFC 5702 */
		if (key->key_size < 512 || key->key_size > 4096) {
			DST_RET(DST_R_INVALIDPARAM);
		}
		break;
	case DST_ALG_RSASHA512:
		/* From RFC 5702 */
		if (key->key_size < 1024 || key->key_size > 4096) {
			DST_RET(DST_R_INVALIDPARAM);
		}
		break;
	default:
		UNREACHABLE();
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

	ret = opensslrsa_generate_pkey(key->key_size, key->label, e, callback,
				       &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	key->keydata.pkeypair.pub = pkey;
	key->keydata.pkeypair.priv = pkey;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_free(pkey);
	BN_free(e);
	return ret;
}

static isc_result_t
opensslrsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	isc_region_t r;
	unsigned int e_bytes;
	unsigned int mod_bytes;
	isc_result_t ret;
	rsa_components_t c = { 0 };

	REQUIRE(key->keydata.pkeypair.pub != NULL);

	isc_buffer_availableregion(data, &r);

	ret = opensslrsa_components_get(key, &c, false);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	mod_bytes = BN_num_bytes(c.n);
	e_bytes = BN_num_bytes(c.e);

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

	BN_bn2bin(c.e, r.base);
	isc_region_consume(&r, e_bytes);
	BN_bn2bin(c.n, r.base);
	isc_region_consume(&r, mod_bytes);

	isc_buffer_add(data, e_bytes + mod_bytes);

	ret = ISC_R_SUCCESS;
err:
	opensslrsa_components_free(&c);
	return ret;
}

static isc_result_t
opensslrsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	isc_region_t r;
	unsigned int e_bytes;
	unsigned int length;
	rsa_components_t c = { .bnfree = true };

	REQUIRE(opensslrsa_valid_key_alg(key->key_alg));

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		DST_RET(ISC_R_SUCCESS);
	}
	length = r.length;
	if (r.length < 1) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}

	e_bytes = *r.base;
	isc_region_consume(&r, 1);

	if (e_bytes == 0) {
		if (r.length < 2) {
			DST_RET(DST_R_INVALIDPUBLICKEY);
		}
		e_bytes = (*r.base) << 8;
		isc_region_consume(&r, 1);
		e_bytes += *r.base;
		isc_region_consume(&r, 1);
	}

	if (r.length < e_bytes) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	c.e = BN_bin2bn(r.base, e_bytes, NULL);
	isc_region_consume(&r, e_bytes);
	c.n = BN_bin2bn(r.base, r.length, NULL);
	if (c.e == NULL || c.n == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	isc_buffer_forward(data, length);

	key->key_size = BN_num_bits(c.n);
	ret = opensslrsa_build_pkey(false, &c, &key->keydata.pkeypair.pub);

err:
	opensslrsa_components_free(&c);
	return ret;
}

static isc_result_t
opensslrsa_tofile(const dst_key_t *key, const char *directory) {
	isc_result_t ret;
	dst_private_t priv = { 0 };
	unsigned char *bufs[8] = { NULL };
	unsigned short i = 0;
	rsa_components_t c = { 0 };

	if (key->external) {
		return dst__privstruct_writefile(key, &priv, directory);
	}

	ret = opensslrsa_components_get(key, &c, true);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	priv.elements[i].tag = TAG_RSA_MODULUS;
	priv.elements[i].length = BN_num_bytes(c.n);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(c.n, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_RSA_PUBLICEXPONENT;
	priv.elements[i].length = BN_num_bytes(c.e);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(c.e, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	if (c.d != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIVATEEXPONENT;
		priv.elements[i].length = BN_num_bytes(c.d);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.d, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (c.p != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIME1;
		priv.elements[i].length = BN_num_bytes(c.p);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.p, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (c.q != NULL) {
		priv.elements[i].tag = TAG_RSA_PRIME2;
		priv.elements[i].length = BN_num_bytes(c.q);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.q, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (c.dmp1 != NULL) {
		priv.elements[i].tag = TAG_RSA_EXPONENT1;
		priv.elements[i].length = BN_num_bytes(c.dmp1);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.dmp1, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (c.dmq1 != NULL) {
		priv.elements[i].tag = TAG_RSA_EXPONENT2;
		priv.elements[i].length = BN_num_bytes(c.dmq1);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.dmq1, bufs[i]);
		priv.elements[i].data = bufs[i];
		i++;
	}

	if (c.iqmp != NULL) {
		priv.elements[i].tag = TAG_RSA_COEFFICIENT;
		priv.elements[i].length = BN_num_bytes(c.iqmp);
		INSIST(i < ARRAY_SIZE(bufs));
		bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
		BN_bn2bin(c.iqmp, bufs[i]);
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
	ret = dst__privstruct_writefile(key, &priv, directory);

err:
	for (i = 0; i < ARRAY_SIZE(bufs); i++) {
		if (bufs[i] != NULL) {
			isc_mem_put(key->mctx, bufs[i],
				    priv.elements[i].length);
		}
	}
	opensslrsa_components_free(&c);

	return ret;
}

static isc_result_t
opensslrsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		     const char *pin);

static isc_result_t
opensslrsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	isc_mem_t *mctx = NULL;
	const char *engine = NULL, *label = NULL;
	EVP_PKEY *pkey = NULL;
	rsa_components_t c = { .bnfree = true };

	REQUIRE(key != NULL);
	REQUIRE(opensslrsa_valid_key_alg(key->key_alg));

	mctx = key->mctx;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_RSA, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	if (key->external) {
		if (priv.nelements != 0 || pub == NULL) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		key->keydata.pkeypair.pub = pub->keydata.pkeypair.pub;
		key->keydata.pkeypair.priv = pub->keydata.pkeypair.priv;
		pub->keydata.pkeypair.pub = NULL;
		pub->keydata.pkeypair.priv = NULL;
		key->key_size = pub->key_size;
		DST_RET(ISC_R_SUCCESS);
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
	 * Is this key stored in a HSM?
	 * See if we can fetch it.
	 */
	if (label != NULL) {
		ret = opensslrsa_fromlabel(key, engine, label, NULL);
		if (ret != ISC_R_SUCCESS) {
			DST_RET(ret);
		}
		/* Check that the public component matches if given */
		if (pub != NULL && EVP_PKEY_eq(key->keydata.pkeypair.pub,
					       pub->keydata.pkeypair.pub) != 1)
		{
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		DST_RET(ISC_R_SUCCESS);
	}

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
				c.n = bn;
				break;
			case TAG_RSA_PUBLICEXPONENT:
				c.e = bn;
				break;
			case TAG_RSA_PRIVATEEXPONENT:
				c.d = bn;
				break;
			case TAG_RSA_PRIME1:
				c.p = bn;
				break;
			case TAG_RSA_PRIME2:
				c.q = bn;
				break;
			case TAG_RSA_EXPONENT1:
				c.dmp1 = bn;
				break;
			case TAG_RSA_EXPONENT2:
				c.dmq1 = bn;
				break;
			case TAG_RSA_COEFFICIENT:
				c.iqmp = bn;
				break;
			default:
				BN_clear_free(bn);
			}
		}
	}

	/* Basic sanity check for public key portion */
	if (c.n == NULL || c.e == NULL) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}
	if (BN_num_bits(c.e) > RSA_MAX_PUBEXP_BITS) {
		DST_RET(ISC_R_RANGE);
	}

	key->key_size = BN_num_bits(c.n);
	ret = opensslrsa_build_pkey(true, &c, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	/* Check that the public component matches if given */
	if (pub != NULL && EVP_PKEY_eq(pkey, pub->keydata.pkeypair.pub) != 1) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	key->keydata.pkeypair.pub = pkey;
	key->keydata.pkeypair.priv = pkey;
	pkey = NULL;

err:
	opensslrsa_components_free(&c);
	EVP_PKEY_free(pkey);
	if (ret != ISC_R_SUCCESS) {
		key->keydata.generic = NULL;
	}

	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));

	return ret;
}

static isc_result_t
opensslrsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		     const char *pin) {
	EVP_PKEY *privpkey = NULL, *pubpkey = NULL;
	isc_result_t ret;

	ret = dst__openssl_fromlabel(EVP_PKEY_RSA, engine, label, pin, &pubpkey,
				     &privpkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	if (!opensslrsa_check_exponent_bits(pubpkey, RSA_MAX_PUBEXP_BITS)) {
		DST_RET(ISC_R_RANGE);
	}

	if (engine != NULL) {
		key->engine = isc_mem_strdup(key->mctx, engine);
	}
	key->label = isc_mem_strdup(key->mctx, label);
	key->key_size = EVP_PKEY_bits(privpkey);
	key->keydata.pkeypair.priv = privpkey;
	key->keydata.pkeypair.pub = pubpkey;
	privpkey = NULL;
	pubpkey = NULL;

err:
	EVP_PKEY_free(privpkey);
	EVP_PKEY_free(pubpkey);
	return ret;
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
	dst__openssl_keypair_compare,
	NULL, /*%< paramcompare */
	opensslrsa_generate,
	dst__openssl_keypair_isprivate,
	dst__openssl_keypair_destroy,
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
	rsa_components_t c = { .bnfree = true };
	EVP_MD_CTX *evp_md_ctx = EVP_MD_CTX_create();
	EVP_PKEY *pkey = NULL;
	const EVP_MD *type = NULL;
	const unsigned char *sig = NULL;
	isc_result_t ret = ISC_R_SUCCESS;
	size_t len;

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
	c.e = BN_bin2bn(e_bytes, sizeof(e_bytes) - 1, NULL);
	c.n = BN_bin2bn(n_bytes, sizeof(n_bytes) - 1, NULL);
	if (c.e == NULL || c.n == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}

	ret = opensslrsa_build_pkey(false, &c, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
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
	opensslrsa_components_free(&c);
	EVP_PKEY_free(pkey);
	EVP_MD_CTX_destroy(evp_md_ctx);
	ERR_clear_error();
	return ret;
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

	return result;
}
