/*	$NetBSD: opensslecdsa_link.c,v 1.9 2025/01/26 16:25:23 christos Exp $	*/

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

#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#endif

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/keyvalues.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"
#include "openssl_shim.h"

#ifndef NID_X9_62_prime256v1
#error "P-256 group is not known (NID_X9_62_prime256v1)"
#endif /* ifndef NID_X9_62_prime256v1 */
#ifndef NID_secp384r1
#error "P-384 group is not known (NID_secp384r1)"
#endif /* ifndef NID_secp384r1 */

#define MAX_PUBKEY_SIZE DNS_KEY_ECDSA384SIZE

#define MAX_PRIVKEY_SIZE (MAX_PUBKEY_SIZE / 2)

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

static bool
opensslecdsa_valid_key_alg(unsigned int key_alg) {
	switch (key_alg) {
	case DST_ALG_ECDSA256:
	case DST_ALG_ECDSA384:
		return true;
	default:
		return false;
	}
}

static int
opensslecdsa_key_alg_to_group_nid(unsigned int key_alg) {
	switch (key_alg) {
	case DST_ALG_ECDSA256:
		return NID_X9_62_prime256v1;
	case DST_ALG_ECDSA384:
		return NID_secp384r1;
	default:
		UNREACHABLE();
	}
}

static size_t
opensslecdsa_key_alg_to_publickey_size(unsigned int key_alg) {
	switch (key_alg) {
	case DST_ALG_ECDSA256:
		return DNS_KEY_ECDSA256SIZE;
	case DST_ALG_ECDSA384:
		return DNS_KEY_ECDSA384SIZE;
	default:
		UNREACHABLE();
	}
}

/*
 * OpenSSL requires us to set the public key portion, but since our private key
 * file format does not contain it directly, we generate it as needed.
 */
static EC_POINT *
opensslecdsa_generate_public_key(const EC_GROUP *group, const BIGNUM *privkey) {
	EC_POINT *pubkey = EC_POINT_new(group);
	if (pubkey == NULL) {
		return NULL;
	}
	if (EC_POINT_mul(group, pubkey, privkey, NULL, NULL, NULL) != 1) {
		EC_POINT_free(pubkey);
		return NULL;
	}
	return pubkey;
}

static int
BN_bn2bin_fixed(const BIGNUM *bn, unsigned char *buf, int size) {
	int bytes = size - BN_num_bytes(bn);

	INSIST(bytes >= 0);

	while (bytes-- > 0) {
		*buf++ = 0;
	}
	BN_bn2bin(bn, buf);
	return size;
}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L

static const char *
opensslecdsa_key_alg_to_group_name(unsigned int key_alg) {
	switch (key_alg) {
	case DST_ALG_ECDSA256:
		return "prime256v1";
	case DST_ALG_ECDSA384:
		return "secp384r1";
	default:
		UNREACHABLE();
	}
}

static isc_result_t
opensslecdsa_create_pkey_params(unsigned int key_alg, bool private,
				const unsigned char *key, size_t key_len,
				EVP_PKEY **pkey) {
	isc_result_t ret;
	int status;
	int group_nid = opensslecdsa_key_alg_to_group_nid(key_alg);
	const char *groupname = opensslecdsa_key_alg_to_group_name(key_alg);
	OSSL_PARAM_BLD *bld = NULL;
	OSSL_PARAM *params = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EC_POINT *pubkey = NULL;
	EC_GROUP *group = NULL;
	BIGNUM *priv = NULL;
	unsigned char buf[MAX_PUBKEY_SIZE + 1];

	bld = OSSL_PARAM_BLD_new();
	if (bld == NULL) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_new",
					       DST_R_OPENSSLFAILURE));
	}
	status = OSSL_PARAM_BLD_push_utf8_string(
		bld, OSSL_PKEY_PARAM_GROUP_NAME, groupname, 0);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_"
					       "utf8_string",
					       DST_R_OPENSSLFAILURE));
	}

	if (private) {
		group = EC_GROUP_new_by_curve_name(group_nid);
		if (group == NULL) {
			DST_RET(dst__openssl_toresult2("EC_GROUP_new_by_"
						       "curve_name",
						       DST_R_OPENSSLFAILURE));
		}

		priv = BN_bin2bn(key, key_len, NULL);
		if (priv == NULL) {
			DST_RET(dst__openssl_toresult2("BN_bin2bn",
						       DST_R_OPENSSLFAILURE));
		}

		status = OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PRIV_KEY,
						priv);
		if (status != 1) {
			DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
						       DST_R_OPENSSLFAILURE));
		}

		pubkey = opensslecdsa_generate_public_key(group, priv);
		if (pubkey == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}

		key = buf;
		key_len = EC_POINT_point2oct(group, pubkey,
					     POINT_CONVERSION_UNCOMPRESSED, buf,
					     sizeof(buf), NULL);
		if (key_len == 0) {
			DST_RET(dst__openssl_toresult2("EC_POINT_point2oct",
						       DST_R_OPENSSLFAILURE));
		}
	} else {
		INSIST(key_len + 1 <= sizeof(buf));
		buf[0] = POINT_CONVERSION_UNCOMPRESSED;
		memmove(buf + 1, key, key_len);
		key = buf;
		key_len = key_len + 1;
	}

	status = OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY,
						  key, key_len);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_"
					       "octet_string",
					       DST_R_OPENSSLFAILURE));
	}

	params = OSSL_PARAM_BLD_to_param(bld);
	if (params == NULL) {
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_to_param",
					       DST_R_OPENSSLFAILURE));
	}
	ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new_from_name",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_fromdata_init(ctx);
	if (status != 1) {
		/* This will fail if the default provider is an engine.
		 * Return ISC_R_FAILURE to retry using the legacy API. */
		DST_RET(dst__openssl_toresult(ISC_R_FAILURE));
	}
	status = EVP_PKEY_fromdata(
		ctx, pkey, private ? EVP_PKEY_KEYPAIR : EVP_PKEY_PUBLIC_KEY,
		params);
	if (status != 1 || *pkey == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata",
					       DST_R_OPENSSLFAILURE));
	}

	ret = ISC_R_SUCCESS;

err:
	OSSL_PARAM_free(params);
	OSSL_PARAM_BLD_free(bld);
	EVP_PKEY_CTX_free(ctx);
	BN_clear_free(priv);
	EC_POINT_free(pubkey);
	EC_GROUP_free(group);

	return ret;
}

static bool
opensslecdsa_extract_public_key_params(const dst_key_t *key, unsigned char *dst,
				       size_t dstlen) {
	EVP_PKEY *pkey = key->keydata.pkeypair.pub;
	BIGNUM *x = NULL;
	BIGNUM *y = NULL;
	bool ret = false;

	if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_EC_PUB_X, &x) == 1 &&
	    EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_EC_PUB_Y, &y) == 1)
	{
		BN_bn2bin_fixed(x, &dst[0], dstlen / 2);
		BN_bn2bin_fixed(y, &dst[dstlen / 2], dstlen / 2);
		ret = true;
	}
	BN_clear_free(x);
	BN_clear_free(y);
	return ret;
}

#endif

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000

static isc_result_t
opensslecdsa_create_pkey_legacy(unsigned int key_alg, bool private,
				const unsigned char *key, size_t key_len,
				EVP_PKEY **retkey) {
	isc_result_t ret = ISC_R_SUCCESS;
	EC_KEY *eckey = NULL;
	EVP_PKEY *pkey = NULL;
	BIGNUM *privkey = NULL;
	EC_POINT *pubkey = NULL;
	unsigned char buf[MAX_PUBKEY_SIZE + 1];
	int group_nid = opensslecdsa_key_alg_to_group_nid(key_alg);

	eckey = EC_KEY_new_by_curve_name(group_nid);
	if (eckey == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (private) {
		const EC_GROUP *group = EC_KEY_get0_group(eckey);

		privkey = BN_bin2bn(key, key_len, NULL);
		if (privkey == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
		if (!EC_KEY_set_private_key(eckey, privkey)) {
			DST_RET(dst__openssl_toresult(DST_R_INVALIDPRIVATEKEY));
		}

		pubkey = opensslecdsa_generate_public_key(group, privkey);
		if (pubkey == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
		if (EC_KEY_set_public_key(eckey, pubkey) != 1) {
			DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
	} else {
		const unsigned char *cp = buf;
		INSIST(key_len + 1 <= sizeof(buf));
		buf[0] = POINT_CONVERSION_UNCOMPRESSED;
		memmove(buf + 1, key, key_len);
		if (o2i_ECPublicKey(&eckey, &cp, key_len + 1) == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_INVALIDPUBLICKEY));
		}
		if (EC_KEY_check_key(eckey) != 1) {
			DST_RET(dst__openssl_toresult(DST_R_INVALIDPUBLICKEY));
		}
	}

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
	if (!EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
		DST_RET(dst__openssl_toresult(ISC_R_FAILURE));
	}

	*retkey = pkey;
	pkey = NULL;

err:
	BN_clear_free(privkey);
	EC_POINT_free(pubkey);
	EC_KEY_free(eckey);
	EVP_PKEY_free(pkey);
	return ret;
}

static bool
opensslecdsa_extract_public_key_legacy(const dst_key_t *key, unsigned char *dst,
				       size_t dstlen) {
	EVP_PKEY *pkey = key->keydata.pkeypair.pub;
	const EC_KEY *eckey = EVP_PKEY_get0_EC_KEY(pkey);
	const EC_GROUP *group = (eckey == NULL) ? NULL
						: EC_KEY_get0_group(eckey);
	const EC_POINT *pub = (eckey == NULL) ? NULL
					      : EC_KEY_get0_public_key(eckey);
	unsigned char buf[MAX_PUBKEY_SIZE + 1];
	size_t len;

	if (group == NULL || pub == NULL) {
		return false;
	}

	len = EC_POINT_point2oct(group, pub, POINT_CONVERSION_UNCOMPRESSED, buf,
				 sizeof(buf), NULL);
	if (len == dstlen + 1) {
		memmove(dst, buf + 1, dstlen);
		return true;
	}
	return false;
}

#endif

static bool
opensslecdsa_extract_public_key(const dst_key_t *key, unsigned char *dst,
				size_t dstlen) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (opensslecdsa_extract_public_key_params(key, dst, dstlen)) {
		return true;
	}
#endif
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (opensslecdsa_extract_public_key_legacy(key, dst, dstlen)) {
		return true;
	}
#endif
	return false;
}

static isc_result_t
opensslecdsa_create_pkey(unsigned int key_alg, bool private,
			 const unsigned char *key, size_t key_len,
			 EVP_PKEY **retkey) {
	isc_result_t ret;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	ret = opensslecdsa_create_pkey_params(key_alg, private, key, key_len,
					      retkey);
	if (ret != ISC_R_FAILURE) {
		return ret;
	}
#endif
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	ret = opensslecdsa_create_pkey_legacy(key_alg, private, key, key_len,
					      retkey);
	if (ret == ISC_R_SUCCESS) {
		return ret;
	}
#endif
	return DST_R_OPENSSLFAILURE;
}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L

static isc_result_t
opensslecdsa_generate_pkey_with_uri(int group_nid, const char *label,
				    EVP_PKEY **retkey) {
	int status;
	isc_result_t ret;
	char *uri = UNCONST(label);
	EVP_PKEY_CTX *ctx = NULL;
	OSSL_PARAM params[3];

	/* Generate the key's parameters. */
	params[0] = OSSL_PARAM_construct_utf8_string("pkcs11_uri", uri, 0);
	params[1] = OSSL_PARAM_construct_utf8_string(
		"pkcs11_key_usage", (char *)"digitalSignature", 0);
	params[2] = OSSL_PARAM_construct_end();

	ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", "provider=pkcs11");
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
	/*
	 * Setting the P-384 curve doesn't work correctly when using:
	 * OSSL_PARAM_construct_utf8_string("ec_paramgen_curve", "P-384", 0);
	 *
	 * Instead use the OpenSSL function to set the curve nid param.
	 */
	status = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, group_nid);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_set_ec_paramgen_"
					       "curve_nid",
					       DST_R_OPENSSLFAILURE));
	}

	/* Generate the key. */
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
opensslecdsa_generate_pkey(unsigned int key_alg, const char *label,
			   EVP_PKEY **retkey) {
	isc_result_t ret;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *params_pkey = NULL;
	int group_nid = opensslecdsa_key_alg_to_group_nid(key_alg);
	int status;

	if (label != NULL) {
		return opensslecdsa_generate_pkey_with_uri(group_nid, label,
							   retkey);
	}

	/* Generate the key's parameters. */
	ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new_from_name",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_paramgen_init(ctx);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_paramgen_init",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, group_nid);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_set_ec_paramgen_"
					       "curve_nid",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_paramgen(ctx, &params_pkey);
	if (status != 1 || params_pkey == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_paramgen",
					       DST_R_OPENSSLFAILURE));
	}
	EVP_PKEY_CTX_free(ctx);

	/* Generate the key. */
	ctx = EVP_PKEY_CTX_new(params_pkey, NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new",
					       DST_R_OPENSSLFAILURE));
	}
	status = EVP_PKEY_keygen_init(ctx);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen_init",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_keygen(ctx, retkey);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen",
					       DST_R_OPENSSLFAILURE));
	}
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_free(params_pkey);
	EVP_PKEY_CTX_free(ctx);
	return ret;
}

static isc_result_t
opensslecdsa_validate_pkey_group(unsigned int key_alg, EVP_PKEY *pkey) {
	const char *groupname = opensslecdsa_key_alg_to_group_name(key_alg);
	char gname[64];

	if (EVP_PKEY_get_group_name(pkey, gname, sizeof(gname), NULL) != 1) {
		return DST_R_INVALIDPRIVATEKEY;
	}
	if (strcmp(gname, groupname) != 0) {
		return DST_R_INVALIDPRIVATEKEY;
	}
	return ISC_R_SUCCESS;
}

static bool
opensslecdsa_extract_private_key(const dst_key_t *key, unsigned char *buf,
				 size_t buflen) {
	EVP_PKEY *pkey = key->keydata.pkeypair.priv;
	BIGNUM *priv = NULL;

	if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PRIV_KEY, &priv) != 1) {
		return false;
	}

	BN_bn2bin_fixed(priv, buf, buflen);
	BN_clear_free(priv);
	return true;
}

#else

static isc_result_t
opensslecdsa_generate_pkey(unsigned int key_alg, const char *label,
			   EVP_PKEY **retkey) {
	isc_result_t ret;
	EC_KEY *eckey = NULL;
	EVP_PKEY *pkey = NULL;
	int group_nid;

	UNUSED(label);

	group_nid = opensslecdsa_key_alg_to_group_nid(key_alg);

	eckey = EC_KEY_new_by_curve_name(group_nid);
	if (eckey == NULL) {
		DST_RET(dst__openssl_toresult2("EC_KEY_new_by_curve_name",
					       DST_R_OPENSSLFAILURE));
	}

	if (EC_KEY_generate_key(eckey) != 1) {
		DST_RET(dst__openssl_toresult2("EC_KEY_generate_key",
					       DST_R_OPENSSLFAILURE));
	}

	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
	if (EVP_PKEY_set1_EC_KEY(pkey, eckey) != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_set1_EC_KEY",
					       DST_R_OPENSSLFAILURE));
	}
	*retkey = pkey;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	EC_KEY_free(eckey);
	EVP_PKEY_free(pkey);
	return ret;
}

static isc_result_t
opensslecdsa_validate_pkey_group(unsigned int key_alg, EVP_PKEY *pkey) {
	const EC_KEY *eckey = EVP_PKEY_get0_EC_KEY(pkey);
	int group_nid;

	if (eckey == NULL) {
		return dst__openssl_toresult(DST_R_INVALIDPRIVATEKEY);
	}

	group_nid = opensslecdsa_key_alg_to_group_nid(key_alg);

	if (EC_GROUP_get_curve_name(EC_KEY_get0_group(eckey)) != group_nid) {
		return DST_R_INVALIDPRIVATEKEY;
	}

	return ISC_R_SUCCESS;
}

static bool
opensslecdsa_extract_private_key(const dst_key_t *key, unsigned char *buf,
				 size_t buflen) {
	const EC_KEY *eckey = NULL;
	const BIGNUM *privkey = NULL;

	eckey = EVP_PKEY_get0_EC_KEY(key->keydata.pkeypair.priv);
	if (eckey == NULL) {
		ERR_clear_error();
		return false;
	}

	privkey = EC_KEY_get0_private_key(eckey);
	if (privkey == NULL) {
		ERR_clear_error();
		return false;
	}

	BN_bn2bin_fixed(privkey, buf, buflen);
	return true;
}

#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L */

static isc_result_t
opensslecdsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	isc_result_t ret = ISC_R_SUCCESS;
	EVP_MD_CTX *evp_md_ctx;
	const EVP_MD *type = NULL;

	UNUSED(key);
	REQUIRE(opensslecdsa_valid_key_alg(dctx->key->key_alg));
	REQUIRE(dctx->use == DO_SIGN || dctx->use == DO_VERIFY);

	evp_md_ctx = EVP_MD_CTX_create();
	if (evp_md_ctx == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
	if (dctx->key->key_alg == DST_ALG_ECDSA256) {
		type = EVP_sha256();
	} else {
		type = EVP_sha384();
	}

	if (dctx->use == DO_SIGN) {
		if (EVP_DigestSignInit(evp_md_ctx, NULL, type, NULL,
				       dctx->key->keydata.pkeypair.priv) != 1)
		{
			EVP_MD_CTX_destroy(evp_md_ctx);
			DST_RET(dst__openssl_toresult3(dctx->category,
						       "EVP_DigestSignInit",
						       ISC_R_FAILURE));
		}
	} else {
		if (EVP_DigestVerifyInit(evp_md_ctx, NULL, type, NULL,
					 dctx->key->keydata.pkeypair.pub) != 1)
		{
			EVP_MD_CTX_destroy(evp_md_ctx);
			DST_RET(dst__openssl_toresult3(dctx->category,
						       "EVP_DigestVerifyInit",
						       ISC_R_FAILURE));
		}
	}

	dctx->ctxdata.evp_md_ctx = evp_md_ctx;

err:
	return ret;
}

static void
opensslecdsa_destroyctx(dst_context_t *dctx) {
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(opensslecdsa_valid_key_alg(dctx->key->key_alg));
	REQUIRE(dctx->use == DO_SIGN || dctx->use == DO_VERIFY);

	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		dctx->ctxdata.evp_md_ctx = NULL;
	}
}

static isc_result_t
opensslecdsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
	isc_result_t ret = ISC_R_SUCCESS;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	REQUIRE(opensslecdsa_valid_key_alg(dctx->key->key_alg));
	REQUIRE(dctx->use == DO_SIGN || dctx->use == DO_VERIFY);

	if (dctx->use == DO_SIGN) {
		if (EVP_DigestSignUpdate(evp_md_ctx, data->base,
					 data->length) != 1)
		{
			DST_RET(dst__openssl_toresult3(dctx->category,
						       "EVP_DigestSignUpdate",
						       ISC_R_FAILURE));
		}
	} else {
		if (EVP_DigestVerifyUpdate(evp_md_ctx, data->base,
					   data->length) != 1)
		{
			DST_RET(dst__openssl_toresult3(dctx->category,
						       "EVP_DigestVerifyUpdate",
						       ISC_R_FAILURE));
		}
	}

err:
	return ret;
}

static isc_result_t
opensslecdsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	isc_region_t region;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	ECDSA_SIG *ecdsasig = NULL;
	size_t siglen, sigder_len = 0, sigder_alloced = 0;
	unsigned char *sigder = NULL;
	const unsigned char *sigder_copy;
	const BIGNUM *r, *s;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	REQUIRE(dctx->use == DO_SIGN);

	if (key->key_alg == DST_ALG_ECDSA256) {
		siglen = DNS_SIG_ECDSA256SIZE;
	} else {
		siglen = DNS_SIG_ECDSA384SIZE;
	}

	isc_buffer_availableregion(sig, &region);
	if (region.length < siglen) {
		DST_RET(ISC_R_NOSPACE);
	}

	if (EVP_DigestSignFinal(evp_md_ctx, NULL, &sigder_len) != 1) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestSignFinal", ISC_R_FAILURE));
	}
	if (sigder_len == 0) {
		DST_RET(ISC_R_FAILURE);
	}
	sigder = isc_mem_get(dctx->mctx, sigder_len);
	sigder_alloced = sigder_len;
	if (EVP_DigestSignFinal(evp_md_ctx, sigder, &sigder_len) != 1) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestSignFinal", ISC_R_FAILURE));
	}
	sigder_copy = sigder;
	if (d2i_ECDSA_SIG(&ecdsasig, &sigder_copy, sigder_len) == NULL) {
		DST_RET(dst__openssl_toresult3(dctx->category, "d2i_ECDSA_SIG",
					       ISC_R_FAILURE));
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
	if (sigder != NULL && sigder_alloced != 0) {
		isc_mem_put(dctx->mctx, sigder, sigder_alloced);
	}

	return ret;
}

static isc_result_t
opensslecdsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	int status;
	unsigned char *cp = sig->base;
	ECDSA_SIG *ecdsasig = NULL;
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	size_t siglen, sigder_len = 0, sigder_alloced = 0;
	unsigned char *sigder = NULL;
	unsigned char *sigder_copy;
	BIGNUM *r = NULL, *s = NULL;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	REQUIRE(dctx->use == DO_VERIFY);

	if (key->key_alg == DST_ALG_ECDSA256) {
		siglen = DNS_SIG_ECDSA256SIZE;
	} else {
		siglen = DNS_SIG_ECDSA384SIZE;
	}

	if (sig->length != siglen) {
		DST_RET(DST_R_VERIFYFAILURE);
	}

	ecdsasig = ECDSA_SIG_new();
	if (ecdsasig == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
	r = BN_bin2bn(cp, siglen / 2, NULL);
	cp += siglen / 2;
	s = BN_bin2bn(cp, siglen / 2, NULL);
	/* cp += siglen / 2; */
	ECDSA_SIG_set0(ecdsasig, r, s);

	status = i2d_ECDSA_SIG(ecdsasig, NULL);
	if (status < 0) {
		DST_RET(dst__openssl_toresult3(dctx->category, "i2d_ECDSA_SIG",
					       DST_R_VERIFYFAILURE));
	}

	sigder_len = (size_t)status;
	sigder = isc_mem_get(dctx->mctx, sigder_len);
	sigder_alloced = sigder_len;

	sigder_copy = sigder;
	status = i2d_ECDSA_SIG(ecdsasig, &sigder_copy);
	if (status < 0) {
		DST_RET(dst__openssl_toresult3(dctx->category, "i2d_ECDSA_SIG",
					       DST_R_VERIFYFAILURE));
	}

	status = EVP_DigestVerifyFinal(evp_md_ctx, sigder, sigder_len);

	switch (status) {
	case 1:
		ret = ISC_R_SUCCESS;
		break;
	case 0:
		ret = dst__openssl_toresult(DST_R_VERIFYFAILURE);
		break;
	default:
		ret = dst__openssl_toresult3(dctx->category,
					     "EVP_DigestVerifyFinal",
					     DST_R_VERIFYFAILURE);
		break;
	}

err:
	if (ecdsasig != NULL) {
		ECDSA_SIG_free(ecdsasig);
	}
	if (sigder != NULL && sigder_alloced != 0) {
		isc_mem_put(dctx->mctx, sigder, sigder_alloced);
	}

	return ret;
}

static isc_result_t
opensslecdsa_generate(dst_key_t *key, int unused, void (*callback)(int)) {
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	UNUSED(unused);
	UNUSED(callback);

	ret = opensslecdsa_generate_pkey(key->key_alg, key->label, &pkey);
	if (ret != ISC_R_SUCCESS) {
		return ret;
	}

	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkeypair.priv = pkey;
	key->keydata.pkeypair.pub = pkey;
	return ret;
}

static isc_result_t
opensslecdsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	isc_region_t r;
	size_t keysize;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	REQUIRE(key->keydata.pkeypair.pub != NULL);

	keysize = opensslecdsa_key_alg_to_publickey_size(key->key_alg);
	isc_buffer_availableregion(data, &r);
	if (r.length < keysize) {
		DST_RET(ISC_R_NOSPACE);
	}
	if (!opensslecdsa_extract_public_key(key, r.base, keysize)) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	isc_buffer_add(data, keysize);
	ret = ISC_R_SUCCESS;

err:
	return ret;
}

static isc_result_t
opensslecdsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	isc_region_t r;
	size_t len;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	len = opensslecdsa_key_alg_to_publickey_size(key->key_alg);

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		DST_RET(ISC_R_SUCCESS);
	}
	if (r.length != len) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}

	ret = opensslecdsa_create_pkey(key->key_alg, false, r.base, len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		DST_RET(ret);
	}

	isc_buffer_forward(data, len);
	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkeypair.pub = pkey;
	ret = ISC_R_SUCCESS;

err:
	return ret;
}

static isc_result_t
opensslecdsa_tofile(const dst_key_t *key, const char *directory) {
	isc_result_t ret;
	dst_private_t priv;
	unsigned char buf[MAX_PRIVKEY_SIZE];
	size_t keylen = 0;
	unsigned short i;

	if (key->keydata.pkeypair.pub == NULL) {
		DST_RET(DST_R_NULLKEY);
	}

	if (key->external) {
		priv.nelements = 0;
		DST_RET(dst__privstruct_writefile(key, &priv, directory));
	}

	if (key->keydata.pkeypair.priv == NULL) {
		DST_RET(DST_R_NULLKEY);
	}

	keylen = opensslecdsa_key_alg_to_publickey_size(key->key_alg) / 2;
	INSIST(keylen <= sizeof(buf));

	i = 0;
	if (opensslecdsa_extract_private_key(key, buf, keylen)) {
		priv.elements[i].tag = TAG_ECDSA_PRIVATEKEY;
		priv.elements[i].length = keylen;
		priv.elements[i].data = buf;
		i++;
	}
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
	isc_safe_memwipe(buf, keylen);
	return ret;
}

static isc_result_t
opensslecdsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin);

static isc_result_t
opensslecdsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	const char *engine = NULL;
	const char *label = NULL;
	int i, privkey_index = -1;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_ECDSA256, lexer, key->mctx,
				    &priv);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	if (key->external) {
		if (priv.nelements != 0 || pub == NULL) {
			DST_RET(dst__openssl_toresult(DST_R_INVALIDPRIVATEKEY));
		}
		key->keydata.pkeypair.priv = pub->keydata.pkeypair.priv;
		key->keydata.pkeypair.pub = pub->keydata.pkeypair.pub;
		pub->keydata.pkeypair.priv = NULL;
		pub->keydata.pkeypair.pub = NULL;
		DST_RET(ISC_R_SUCCESS);
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

	if (label != NULL) {
		ret = opensslecdsa_fromlabel(key, engine, label, NULL);
		if (ret != ISC_R_SUCCESS) {
			goto err;
		}
		/* Check that the public component matches if given */
		if (pub != NULL && EVP_PKEY_eq(key->keydata.pkeypair.pub,
					       pub->keydata.pkeypair.pub) != 1)
		{
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		DST_RET(ISC_R_SUCCESS);
	}

	if (privkey_index < 0) {
		DST_RET(dst__openssl_toresult(DST_R_INVALIDPRIVATEKEY));
	}

	ret = opensslecdsa_create_pkey(
		key->key_alg, true, priv.elements[privkey_index].data,
		priv.elements[privkey_index].length, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	/* Check that the public component matches if given */
	if (pub != NULL && EVP_PKEY_eq(pkey, pub->keydata.pkeypair.pub) != 1) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkeypair.priv = pkey;
	key->keydata.pkeypair.pub = pkey;
	pkey = NULL;

err:
	EVP_PKEY_free(pkey);
	if (ret != ISC_R_SUCCESS) {
		key->keydata.generic = NULL;
	}
	dst__privstruct_free(&priv, key->mctx);
	isc_safe_memwipe(&priv, sizeof(priv));

	return ret;
}

static isc_result_t
opensslecdsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin) {
	EVP_PKEY *privpkey = NULL, *pubpkey = NULL;
	isc_result_t ret;

	REQUIRE(opensslecdsa_valid_key_alg(key->key_alg));
	UNUSED(pin);

	ret = dst__openssl_fromlabel(EVP_PKEY_EC, engine, label, pin, &pubpkey,
				     &privpkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	ret = opensslecdsa_validate_pkey_group(key->key_alg, privpkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}
	ret = opensslecdsa_validate_pkey_group(key->key_alg, pubpkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
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

static dst_func_t opensslecdsa_functions = {
	opensslecdsa_createctx,
	NULL, /*%< createctx2 */
	opensslecdsa_destroyctx,
	opensslecdsa_adddata,
	opensslecdsa_sign,
	opensslecdsa_verify,
	NULL, /*%< verify2 */
	NULL, /*%< computesecret */
	dst__openssl_keypair_compare,
	NULL, /*%< paramcompare */
	opensslecdsa_generate,
	dst__openssl_keypair_isprivate,
	dst__openssl_keypair_destroy,
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
	return ISC_R_SUCCESS;
}
