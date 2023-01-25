/*	$NetBSD: opensslecdsa_link.c,v 1.7 2023/01/25 21:43:30 christos Exp $	*/

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

#if !USE_PKCS11

#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#if !defined(OPENSSL_NO_ENGINE)
#include <openssl/engine.h>
#endif

#include <isc/mem.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/keyvalues.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"

#ifndef NID_X9_62_prime256v1
#error "P-256 group is not known (NID_X9_62_prime256v1)"
#endif /* ifndef NID_X9_62_prime256v1 */
#ifndef NID_secp384r1
#error "P-384 group is not known (NID_secp384r1)"
#endif /* ifndef NID_secp384r1 */

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

#if !HAVE_ECDSA_SIG_GET0
/* From OpenSSL 1.1 */
static void
ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps) {
	if (pr != NULL) {
		*pr = sig->r;
	}
	if (ps != NULL) {
		*ps = sig->s;
	}
}

static int
ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s) {
	if (r == NULL || s == NULL) {
		return (0);
	}

	BN_clear_free(sig->r);
	BN_clear_free(sig->s);
	sig->r = r;
	sig->s = s;

	return (1);
}
#endif /* !HAVE_ECDSA_SIG_GET0 */

static isc_result_t
opensslecdsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx;
	const EVP_MD *type = NULL;

	UNUSED(key);
	REQUIRE(dctx->key->key_alg == DST_ALG_ECDSA256 ||
		dctx->key->key_alg == DST_ALG_ECDSA384);

	evp_md_ctx = EVP_MD_CTX_create();
	if (evp_md_ctx == NULL) {
		return (ISC_R_NOMEMORY);
	}
	if (dctx->key->key_alg == DST_ALG_ECDSA256) {
		type = EVP_sha256();
	} else {
		type = EVP_sha384();
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
opensslecdsa_destroyctx(dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(dctx->key->key_alg == DST_ALG_ECDSA256 ||
		dctx->key->key_alg == DST_ALG_ECDSA384);

	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		dctx->ctxdata.evp_md_ctx = NULL;
	}
}

static isc_result_t
opensslecdsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(dctx->key->key_alg == DST_ALG_ECDSA256 ||
		dctx->key->key_alg == DST_ALG_ECDSA384);

	if (!EVP_DigestUpdate(evp_md_ctx, data->base, data->length)) {
		return (dst__openssl_toresult3(
			dctx->category, "EVP_DigestUpdate", ISC_R_FAILURE));
	}

	return (ISC_R_SUCCESS);
}

static int
BN_bn2bin_fixed(const BIGNUM *bn, unsigned char *buf, int size) {
	int bytes = size - BN_num_bytes(bn);

	while (bytes-- > 0) {
		*buf++ = 0;
	}
	BN_bn2bin(bn, buf);
	return (size);
}

static isc_result_t
opensslecdsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	isc_region_t region;
	ECDSA_SIG *ecdsasig;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	EVP_PKEY *pkey = key->keydata.pkey;
	EC_KEY *eckey = EVP_PKEY_get1_EC_KEY(pkey);
	unsigned int dgstlen, siglen;
	unsigned char digest[EVP_MAX_MD_SIZE];
	const BIGNUM *r, *s;

	REQUIRE(key->key_alg == DST_ALG_ECDSA256 ||
		key->key_alg == DST_ALG_ECDSA384);

	if (eckey == NULL) {
		return (ISC_R_FAILURE);
	}

	if (key->key_alg == DST_ALG_ECDSA256) {
		siglen = DNS_SIG_ECDSA256SIZE;
	} else {
		siglen = DNS_SIG_ECDSA384SIZE;
	}

	isc_buffer_availableregion(sig, &region);
	if (region.length < siglen) {
		DST_RET(ISC_R_NOSPACE);
	}

	if (!EVP_DigestFinal_ex(evp_md_ctx, digest, &dgstlen)) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestFinal_ex", ISC_R_FAILURE));
	}

	ecdsasig = ECDSA_do_sign(digest, dgstlen, eckey);
	if (ecdsasig == NULL) {
		DST_RET(dst__openssl_toresult3(dctx->category, "ECDSA_do_sign",
					       DST_R_SIGNFAILURE));
	}
	ECDSA_SIG_get0(ecdsasig, &r, &s);
	BN_bn2bin_fixed(r, region.base, siglen / 2);
	isc_region_consume(&region, siglen / 2);
	BN_bn2bin_fixed(s, region.base, siglen / 2);
	isc_region_consume(&region, siglen / 2);
	ECDSA_SIG_free(ecdsasig);
	isc_buffer_add(sig, siglen);
	ret = ISC_R_SUCCESS;

err:
	EC_KEY_free(eckey);
	return (ret);
}

static isc_result_t
opensslecdsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	int status;
	unsigned char *cp = sig->base;
	ECDSA_SIG *ecdsasig = NULL;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	EVP_PKEY *pkey = key->keydata.pkey;
	EC_KEY *eckey = EVP_PKEY_get1_EC_KEY(pkey);
	unsigned int dgstlen, siglen;
	unsigned char digest[EVP_MAX_MD_SIZE];
	BIGNUM *r = NULL, *s = NULL;

	REQUIRE(key->key_alg == DST_ALG_ECDSA256 ||
		key->key_alg == DST_ALG_ECDSA384);

	if (eckey == NULL) {
		return (ISC_R_FAILURE);
	}

	if (key->key_alg == DST_ALG_ECDSA256) {
		siglen = DNS_SIG_ECDSA256SIZE;
	} else {
		siglen = DNS_SIG_ECDSA384SIZE;
	}

	if (sig->length != siglen) {
		DST_RET(DST_R_VERIFYFAILURE);
	}

	if (!EVP_DigestFinal_ex(evp_md_ctx, digest, &dgstlen)) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestFinal_ex", ISC_R_FAILURE));
	}

	ecdsasig = ECDSA_SIG_new();
	if (ecdsasig == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	r = BN_bin2bn(cp, siglen / 2, NULL);
	cp += siglen / 2;
	s = BN_bin2bn(cp, siglen / 2, NULL);
	ECDSA_SIG_set0(ecdsasig, r, s);
	/* cp += siglen / 2; */

	status = ECDSA_do_verify(digest, dgstlen, ecdsasig, eckey);
	switch (status) {
	case 1:
		ret = ISC_R_SUCCESS;
		break;
	case 0:
		ret = dst__openssl_toresult(DST_R_VERIFYFAILURE);
		break;
	default:
		ret = dst__openssl_toresult3(dctx->category, "ECDSA_do_verify",
					     DST_R_VERIFYFAILURE);
		break;
	}

err:
	if (ecdsasig != NULL) {
		ECDSA_SIG_free(ecdsasig);
	}
	EC_KEY_free(eckey);
	return (ret);
}

static bool
opensslecdsa_compare(const dst_key_t *key1, const dst_key_t *key2) {
	bool ret;
	int status;
	EVP_PKEY *pkey1 = key1->keydata.pkey;
	EVP_PKEY *pkey2 = key2->keydata.pkey;
	EC_KEY *eckey1 = NULL;
	EC_KEY *eckey2 = NULL;
	const BIGNUM *priv1, *priv2;

	if (pkey1 == NULL && pkey2 == NULL) {
		return (true);
	} else if (pkey1 == NULL || pkey2 == NULL) {
		return (false);
	}

	eckey1 = EVP_PKEY_get1_EC_KEY(pkey1);
	eckey2 = EVP_PKEY_get1_EC_KEY(pkey2);
	if (eckey1 == NULL && eckey2 == NULL) {
		DST_RET(true);
	} else if (eckey1 == NULL || eckey2 == NULL) {
		DST_RET(false);
	}

	status = EVP_PKEY_cmp(pkey1, pkey2);
	if (status != 1) {
		DST_RET(false);
	}

	priv1 = EC_KEY_get0_private_key(eckey1);
	priv2 = EC_KEY_get0_private_key(eckey2);
	if (priv1 != NULL || priv2 != NULL) {
		if (priv1 == NULL || priv2 == NULL) {
			DST_RET(false);
		}
		if (BN_cmp(priv1, priv2) != 0) {
			DST_RET(false);
		}
	}
	ret = true;

err:
	if (eckey1 != NULL) {
		EC_KEY_free(eckey1);
	}
	if (eckey2 != NULL) {
		EC_KEY_free(eckey2);
	}
	return (ret);
}

static isc_result_t
opensslecdsa_generate(dst_key_t *key, int unused, void (*callback)(int)) {
	isc_result_t ret;
	EVP_PKEY *pkey;
	EC_KEY *eckey = NULL;
	int group_nid;

	REQUIRE(key->key_alg == DST_ALG_ECDSA256 ||
		key->key_alg == DST_ALG_ECDSA384);
	UNUSED(unused);
	UNUSED(callback);

	if (key->key_alg == DST_ALG_ECDSA256) {
		group_nid = NID_X9_62_prime256v1;
		key->key_size = DNS_KEY_ECDSA256SIZE * 4;
	} else {
		group_nid = NID_secp384r1;
		key->key_size = DNS_KEY_ECDSA384SIZE * 4;
	}

	eckey = EC_KEY_new_by_curve_name(group_nid);
	if (eckey == NULL) {
		return (dst__openssl_toresult2("EC_KEY_new_by_curve_name",
					       DST_R_OPENSSLFAILURE));
	}

	if (EC_KEY_generate_key(eckey) != 1) {
		DST_RET(dst__openssl_toresult2("EC_KEY_generate_key",
					       DST_R_OPENSSLFAILURE));
	}

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	if (!EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
		EVP_PKEY_free(pkey);
		DST_RET(ISC_R_FAILURE);
	}
	key->keydata.pkey = pkey;
	ret = ISC_R_SUCCESS;

err:
	EC_KEY_free(eckey);
	return (ret);
}

static bool
opensslecdsa_isprivate(const dst_key_t *key) {
	bool ret;
	EVP_PKEY *pkey = key->keydata.pkey;
	EC_KEY *eckey = EVP_PKEY_get1_EC_KEY(pkey);

	ret = (eckey != NULL && EC_KEY_get0_private_key(eckey) != NULL);
	if (eckey != NULL) {
		EC_KEY_free(eckey);
	}
	return (ret);
}

static void
opensslecdsa_destroy(dst_key_t *key) {
	EVP_PKEY *pkey = key->keydata.pkey;

	EVP_PKEY_free(pkey);
	key->keydata.pkey = NULL;
}

static isc_result_t
opensslecdsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	EVP_PKEY *pkey;
	EC_KEY *eckey = NULL;
	isc_region_t r;
	int len;
	unsigned char *cp;
	unsigned char buf[DNS_KEY_ECDSA384SIZE + 1];

	REQUIRE(key->keydata.pkey != NULL);

	pkey = key->keydata.pkey;
	eckey = EVP_PKEY_get1_EC_KEY(pkey);
	if (eckey == NULL) {
		return (dst__openssl_toresult(ISC_R_FAILURE));
	}
	len = i2o_ECPublicKey(eckey, NULL);
	/* skip form */
	len--;

	isc_buffer_availableregion(data, &r);
	if (r.length < (unsigned int)len) {
		DST_RET(ISC_R_NOSPACE);
	}
	cp = buf;
	if (!i2o_ECPublicKey(eckey, &cp)) {
		DST_RET(dst__openssl_toresult(ISC_R_FAILURE));
	}
	memmove(r.base, buf + 1, len);
	isc_buffer_add(data, len);
	ret = ISC_R_SUCCESS;

err:
	EC_KEY_free(eckey);
	return (ret);
}

static isc_result_t
opensslecdsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	EVP_PKEY *pkey;
	EC_KEY *eckey = NULL;
	isc_region_t r;
	int group_nid;
	unsigned int len;
	const unsigned char *cp;
	unsigned char buf[DNS_KEY_ECDSA384SIZE + 1];

	REQUIRE(key->key_alg == DST_ALG_ECDSA256 ||
		key->key_alg == DST_ALG_ECDSA384);

	if (key->key_alg == DST_ALG_ECDSA256) {
		len = DNS_KEY_ECDSA256SIZE;
		group_nid = NID_X9_62_prime256v1;
	} else {
		len = DNS_KEY_ECDSA384SIZE;
		group_nid = NID_secp384r1;
	}

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		return (ISC_R_SUCCESS);
	}
	if (r.length < len) {
		return (DST_R_INVALIDPUBLICKEY);
	}

	eckey = EC_KEY_new_by_curve_name(group_nid);
	if (eckey == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	buf[0] = POINT_CONVERSION_UNCOMPRESSED;
	memmove(buf + 1, r.base, len);
	cp = buf;
	if (o2i_ECPublicKey(&eckey, (const unsigned char **)&cp,
			    (long)len + 1) == NULL)
	{
		DST_RET(dst__openssl_toresult(DST_R_INVALIDPUBLICKEY));
	}
	if (EC_KEY_check_key(eckey) != 1) {
		DST_RET(dst__openssl_toresult(DST_R_INVALIDPUBLICKEY));
	}

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	if (!EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
		EVP_PKEY_free(pkey);
		DST_RET(dst__openssl_toresult(ISC_R_FAILURE));
	}

	isc_buffer_forward(data, len);
	key->keydata.pkey = pkey;
	key->key_size = len * 4;
	ret = ISC_R_SUCCESS;

err:
	if (eckey != NULL) {
		EC_KEY_free(eckey);
	}
	return (ret);
}

static isc_result_t
opensslecdsa_tofile(const dst_key_t *key, const char *directory) {
	isc_result_t ret;
	EVP_PKEY *pkey;
	EC_KEY *eckey = NULL;
	const BIGNUM *privkey;
	dst_private_t priv;
	unsigned char *buf = NULL;
	unsigned short i;

	if (key->keydata.pkey == NULL) {
		return (DST_R_NULLKEY);
	}

	if (key->external) {
		priv.nelements = 0;
		return (dst__privstruct_writefile(key, &priv, directory));
	}

	pkey = key->keydata.pkey;
	eckey = EVP_PKEY_get1_EC_KEY(pkey);
	if (eckey == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	privkey = EC_KEY_get0_private_key(eckey);
	if (privkey == NULL) {
		ret = dst__openssl_toresult(DST_R_OPENSSLFAILURE);
		goto err;
	}

	buf = isc_mem_get(key->mctx, BN_num_bytes(privkey));

	i = 0;

	priv.elements[i].tag = TAG_ECDSA_PRIVATEKEY;
	priv.elements[i].length = BN_num_bytes(privkey);
	BN_bn2bin(privkey, buf);
	priv.elements[i].data = buf;
	i++;

	if (key->engine != NULL) {
		priv.elements[i].tag = TAG_ECDSA_ENGINE;
		priv.elements[i].length = (unsigned short)strlen(key->engine) +
					  1;
		priv.elements[i].data = (unsigned char *)key->engine;
		i++;
	}

	if (key->label != NULL) {
		priv.elements[i].tag = TAG_ECDSA_LABEL;
		priv.elements[i].length = (unsigned short)strlen(key->label) +
					  1;
		priv.elements[i].data = (unsigned char *)key->label;
		i++;
	}

	priv.nelements = i;
	ret = dst__privstruct_writefile(key, &priv, directory);

err:
	EC_KEY_free(eckey);
	if (buf != NULL) {
		isc_mem_put(key->mctx, buf, BN_num_bytes(privkey));
	}
	return (ret);
}

static isc_result_t
ecdsa_check(EC_KEY *eckey, EC_KEY *pubeckey) {
	const EC_POINT *pubkey;

	pubkey = EC_KEY_get0_public_key(eckey);
	if (pubkey != NULL) {
		return (ISC_R_SUCCESS);
	} else if (pubeckey != NULL) {
		pubkey = EC_KEY_get0_public_key(pubeckey);
		if (pubkey == NULL) {
			return (ISC_R_SUCCESS);
		}
		if (EC_KEY_set_public_key(eckey, pubkey) != 1) {
			return (ISC_R_SUCCESS);
		}
	}
	if (EC_KEY_check_key(eckey) == 1) {
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_FAILURE);
}

static isc_result_t
load_privkey_from_privstruct(EC_KEY *eckey, dst_private_t *priv) {
	BIGNUM *privkey = BN_bin2bn(priv->elements[0].data,
				    priv->elements[0].length, NULL);
	isc_result_t result = ISC_R_SUCCESS;

	if (privkey == NULL) {
		return (ISC_R_NOMEMORY);
	}

	if (!EC_KEY_set_private_key(eckey, privkey)) {
		result = ISC_R_NOMEMORY;
	}

	BN_clear_free(privkey);
	return (result);
}

static isc_result_t
eckey_to_pkey(EC_KEY *eckey, EVP_PKEY **pkey) {
	REQUIRE(pkey != NULL && *pkey == NULL);

	*pkey = EVP_PKEY_new();
	if (*pkey == NULL) {
		return (ISC_R_NOMEMORY);
	}
	if (!EVP_PKEY_set1_EC_KEY(*pkey, eckey)) {
		EVP_PKEY_free(*pkey);
		*pkey = NULL;
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
finalize_eckey(dst_key_t *key, EC_KEY *eckey, const char *engine,
	       const char *label) {
	isc_result_t result = ISC_R_SUCCESS;
	EVP_PKEY *pkey = NULL;

	result = eckey_to_pkey(eckey, &pkey);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	key->keydata.pkey = pkey;

	if (label != NULL) {
		key->label = isc_mem_strdup(key->mctx, label);
		key->engine = isc_mem_strdup(key->mctx, engine);
	}

	if (key->key_alg == DST_ALG_ECDSA256) {
		key->key_size = DNS_KEY_ECDSA256SIZE * 4;
	} else {
		key->key_size = DNS_KEY_ECDSA384SIZE * 4;
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
dst__key_to_eckey(dst_key_t *key, EC_KEY **eckey) {
	REQUIRE(eckey != NULL && *eckey == NULL);

	int group_nid;
	switch (key->key_alg) {
	case DST_ALG_ECDSA256:
		group_nid = NID_X9_62_prime256v1;
		break;
	case DST_ALG_ECDSA384:
		group_nid = NID_secp384r1;
		break;
	default:
		UNREACHABLE();
	}
	*eckey = EC_KEY_new_by_curve_name(group_nid);
	if (*eckey == NULL) {
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
opensslecdsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin);

static isc_result_t
opensslecdsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t result = ISC_R_SUCCESS;
	EC_KEY *eckey = NULL;
	EC_KEY *pubeckey = NULL;
	const char *engine = NULL;
	const char *label = NULL;
	int i, privkey_index = -1;
	bool finalize_key = false;

	/* read private key file */
	result = dst__privstruct_parse(key, DST_ALG_ECDSA256, lexer, key->mctx,
				       &priv);
	if (result != ISC_R_SUCCESS) {
		goto end;
	}

	if (key->external) {
		if (priv.nelements != 0 || pub == NULL) {
			result = DST_R_INVALIDPRIVATEKEY;
			goto end;
		}
		key->keydata.pkey = pub->keydata.pkey;
		pub->keydata.pkey = NULL;
		goto end;
	}

	for (i = 0; i < priv.nelements; i++) {
		switch (priv.elements[i].tag) {
		case TAG_ECDSA_ENGINE:
			engine = (char *)priv.elements[i].data;
			break;
		case TAG_ECDSA_LABEL:
			label = (char *)priv.elements[i].data;
			break;
		case TAG_ECDSA_PRIVATEKEY:
			privkey_index = i;
			break;
		default:
			break;
		}
	}

	if (privkey_index < 0) {
		result = DST_R_INVALIDPRIVATEKEY;
		goto end;
	}

	if (label != NULL) {
		result = opensslecdsa_fromlabel(key, engine, label, NULL);
		if (result != ISC_R_SUCCESS) {
			goto end;
		}

		eckey = EVP_PKEY_get1_EC_KEY(key->keydata.pkey);
		if (eckey == NULL) {
			result = dst__openssl_toresult(DST_R_OPENSSLFAILURE);
			goto end;
		}

	} else {
		result = dst__key_to_eckey(key, &eckey);
		if (result != ISC_R_SUCCESS) {
			goto end;
		}

		result = load_privkey_from_privstruct(eckey, &priv);
		if (result != ISC_R_SUCCESS) {
			goto end;
		}

		finalize_key = true;
	}

	if (pub != NULL && pub->keydata.pkey != NULL) {
		pubeckey = EVP_PKEY_get1_EC_KEY(pub->keydata.pkey);
	}

	if (ecdsa_check(eckey, pubeckey) != ISC_R_SUCCESS) {
		result = DST_R_INVALIDPRIVATEKEY;
		goto end;
	}

	if (finalize_key) {
		result = finalize_eckey(key, eckey, engine, label);
	}

end:
	if (pubeckey != NULL) {
		EC_KEY_free(pubeckey);
	}
	if (eckey != NULL) {
		EC_KEY_free(eckey);
	}
	dst__privstruct_free(&priv, key->mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (result);
}

static isc_result_t
opensslecdsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin) {
#if !defined(OPENSSL_NO_ENGINE)
	isc_result_t ret = ISC_R_SUCCESS;
	ENGINE *e;
	EC_KEY *eckey = NULL;
	EC_KEY *pubeckey = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY *pubkey = NULL;
	int group_nid = 0;

	UNUSED(pin);

	if (engine == NULL || label == NULL) {
		return (DST_R_NOENGINE);
	}
	e = dst__openssl_getengine(engine);
	if (e == NULL) {
		return (DST_R_NOENGINE);
	}

	if (key->key_alg == DST_ALG_ECDSA256) {
		group_nid = NID_X9_62_prime256v1;
	} else {
		group_nid = NID_secp384r1;
	}

	/* Load private key. */
	pkey = ENGINE_load_private_key(e, label, NULL, NULL);
	if (pkey == NULL) {
		return (dst__openssl_toresult2("ENGINE_load_private_key",
					       DST_R_OPENSSLFAILURE));
	}
	/* Check base id, group nid */
	if (EVP_PKEY_base_id(pkey) != EVP_PKEY_EC) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}
	eckey = EVP_PKEY_get1_EC_KEY(pkey);
	if (eckey == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	if (EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey)) != group_nid) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	/* Load public key. */
	pubkey = ENGINE_load_public_key(e, label, NULL, NULL);
	if (pubkey == NULL) {
		DST_RET(dst__openssl_toresult2("ENGINE_load_public_key",
					       DST_R_OPENSSLFAILURE));
	}
	/* Check base id, group nid */
	if (EVP_PKEY_base_id(pubkey) != EVP_PKEY_EC) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	pubeckey = EVP_PKEY_get1_EC_KEY(pubkey);
	if (pubeckey == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	if (EC_GROUP_get_curve_name(EC_KEY_get0_group(pubeckey)) != group_nid) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}

	if (ecdsa_check(eckey, pubeckey) != ISC_R_SUCCESS) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	key->label = isc_mem_strdup(key->mctx, label);
	key->engine = isc_mem_strdup(key->mctx, engine);
	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkey = pkey;
	pkey = NULL;

err:
	if (pubkey != NULL) {
		EVP_PKEY_free(pubkey);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (pubeckey != NULL) {
		EC_KEY_free(pubeckey);
	}
	if (eckey != NULL) {
		EC_KEY_free(eckey);
	}

	return (ret);
#else
	UNUSED(key);
	UNUSED(engine);
	UNUSED(label);
	UNUSED(pin);
	return (DST_R_NOENGINE);
#endif
}

static dst_func_t opensslecdsa_functions = {
	opensslecdsa_createctx,
	NULL, /*%< createctx2 */
	opensslecdsa_destroyctx,
	opensslecdsa_adddata,
	opensslecdsa_sign,
	opensslecdsa_verify,
	NULL, /*%< verify2 */
	NULL, /*%< computesecret */
	opensslecdsa_compare,
	NULL, /*%< paramcompare */
	opensslecdsa_generate,
	opensslecdsa_isprivate,
	opensslecdsa_destroy,
	opensslecdsa_todns,
	opensslecdsa_fromdns,
	opensslecdsa_tofile,
	opensslecdsa_parse,
	NULL,			/*%< cleanup */
	opensslecdsa_fromlabel, /*%< fromlabel */
	NULL,			/*%< dump */
	NULL,			/*%< restore */
};

isc_result_t
dst__opensslecdsa_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL) {
		*funcp = &opensslecdsa_functions;
	}
	return (ISC_R_SUCCESS);
}

#endif /* !USE_PKCS11 */
