/*	$NetBSD: openssleddsa_link.c,v 1.11 2025/01/26 16:25:23 christos Exp $	*/

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

#if HAVE_OPENSSL_ED25519 || HAVE_OPENSSL_ED448

#include <stdbool.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

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

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

#if HAVE_OPENSSL_ED25519
#ifndef NID_ED25519
#error "Ed25519 group is not known (NID_ED25519)"
#endif /* ifndef NID_ED25519 */
#endif /* HAVE_OPENSSL_ED25519 */

#if HAVE_OPENSSL_ED448
#ifndef NID_ED448
#error "Ed448 group is not known (NID_ED448)"
#endif /* ifndef NID_ED448 */
#endif /* HAVE_OPENSSL_ED448 */

typedef struct eddsa_alginfo {
	int pkey_type, nid;
	unsigned int key_size, sig_size;
} eddsa_alginfo_t;

static const eddsa_alginfo_t *
openssleddsa_alg_info(unsigned int key_alg) {
#if HAVE_OPENSSL_ED25519
	if (key_alg == DST_ALG_ED25519) {
		static const eddsa_alginfo_t ed25519_alginfo = {
			.pkey_type = EVP_PKEY_ED25519,
			.nid = NID_ED25519,
			.key_size = DNS_KEY_ED25519SIZE,
			.sig_size = DNS_SIG_ED25519SIZE,
		};
		return &ed25519_alginfo;
	}
#endif /* HAVE_OPENSSL_ED25519 */
#if HAVE_OPENSSL_ED448
	if (key_alg == DST_ALG_ED448) {
		static const eddsa_alginfo_t ed448_alginfo = {
			.pkey_type = EVP_PKEY_ED448,
			.nid = NID_ED448,
			.key_size = DNS_KEY_ED448SIZE,
			.sig_size = DNS_SIG_ED448SIZE,
		};
		return &ed448_alginfo;
	}
#endif /* HAVE_OPENSSL_ED448 */
	return NULL;
}

static isc_result_t
raw_key_to_ossl(const eddsa_alginfo_t *alginfo, int private,
		const unsigned char *key, size_t *key_len, EVP_PKEY **pkey) {
	isc_result_t ret;
	int pkey_type = alginfo->pkey_type;
	size_t len = alginfo->key_size;

	ret = (private ? DST_R_INVALIDPRIVATEKEY : DST_R_INVALIDPUBLICKEY);
	if (*key_len < len) {
		return ret;
	}

	if (private) {
		*pkey = EVP_PKEY_new_raw_private_key(pkey_type, NULL, key, len);
	} else {
		*pkey = EVP_PKEY_new_raw_public_key(pkey_type, NULL, key, len);
	}
	if (*pkey == NULL) {
		return dst__openssl_toresult(ret);
	}

	*key_len = len;
	return ISC_R_SUCCESS;
}

static isc_result_t
openssleddsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin);

static isc_result_t
openssleddsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	isc_buffer_t *buf = NULL;
	const eddsa_alginfo_t *alginfo =
		openssleddsa_alg_info(dctx->key->key_alg);

	UNUSED(key);
	REQUIRE(alginfo != NULL);

	isc_buffer_allocate(dctx->mctx, &buf, 64);
	dctx->ctxdata.generic = buf;

	return ISC_R_SUCCESS;
}

static void
openssleddsa_destroyctx(dst_context_t *dctx) {
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	const eddsa_alginfo_t *alginfo =
		openssleddsa_alg_info(dctx->key->key_alg);

	REQUIRE(alginfo != NULL);
	if (buf != NULL) {
		isc_buffer_free(&buf);
	}
	dctx->ctxdata.generic = NULL;
}

static isc_result_t
openssleddsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	isc_buffer_t *nbuf = NULL;
	isc_region_t r;
	unsigned int length;
	isc_result_t result;
	const eddsa_alginfo_t *alginfo =
		openssleddsa_alg_info(dctx->key->key_alg);

	REQUIRE(alginfo != NULL);

	result = isc_buffer_copyregion(buf, data);
	if (result == ISC_R_SUCCESS) {
		return ISC_R_SUCCESS;
	}

	length = isc_buffer_length(buf) + data->length + 64;
	isc_buffer_allocate(dctx->mctx, &nbuf, length);
	isc_buffer_usedregion(buf, &r);
	(void)isc_buffer_copyregion(nbuf, &r);
	(void)isc_buffer_copyregion(nbuf, data);
	isc_buffer_free(&buf);
	dctx->ctxdata.generic = nbuf;

	return ISC_R_SUCCESS;
}

static isc_result_t
openssleddsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	isc_region_t tbsreg;
	isc_region_t sigreg;
	EVP_PKEY *pkey = key->keydata.pkeypair.priv;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	size_t siglen;

	REQUIRE(alginfo != NULL);

	if (ctx == NULL) {
		return ISC_R_NOMEMORY;
	}

	siglen = alginfo->sig_size;
	isc_buffer_availableregion(sig, &sigreg);
	if (sigreg.length < (unsigned int)siglen) {
		DST_RET(ISC_R_NOSPACE);
	}

	isc_buffer_usedregion(buf, &tbsreg);

	if (EVP_DigestSignInit(ctx, NULL, NULL, NULL, pkey) != 1) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestSignInit", ISC_R_FAILURE));
	}
	if (EVP_DigestSign(ctx, sigreg.base, &siglen, tbsreg.base,
			   tbsreg.length) != 1)
	{
		DST_RET(dst__openssl_toresult3(dctx->category, "EVP_DigestSign",
					       DST_R_SIGNFAILURE));
	}
	isc_buffer_add(sig, (unsigned int)siglen);
	ret = ISC_R_SUCCESS;

err:
	EVP_MD_CTX_free(ctx);
	isc_buffer_free(&buf);
	dctx->ctxdata.generic = NULL;

	return ret;
}

static isc_result_t
openssleddsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	int status;
	isc_region_t tbsreg;
	EVP_PKEY *pkey = key->keydata.pkeypair.pub;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);

	REQUIRE(alginfo != NULL);

	if (ctx == NULL) {
		return dst__openssl_toresult(ISC_R_NOMEMORY);
	}

	if (sig->length != alginfo->sig_size) {
		DST_RET(DST_R_VERIFYFAILURE);
	}

	isc_buffer_usedregion(buf, &tbsreg);

	if (EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) != 1) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestVerifyInit", ISC_R_FAILURE));
	}

	status = EVP_DigestVerify(ctx, sig->base, sig->length, tbsreg.base,
				  tbsreg.length);

	switch (status) {
	case 1:
		ret = ISC_R_SUCCESS;
		break;
	case 0:
		ret = dst__openssl_toresult(DST_R_VERIFYFAILURE);
		break;
	default:
		ret = dst__openssl_toresult3(dctx->category, "EVP_DigestVerify",
					     DST_R_VERIFYFAILURE);
		break;
	}

err:
	EVP_MD_CTX_free(ctx);
	isc_buffer_free(&buf);
	dctx->ctxdata.generic = NULL;

	return ret;
}

static isc_result_t
openssleddsa_generate(dst_key_t *key, int unused, void (*callback)(int)) {
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	int status;

	REQUIRE(alginfo != NULL);
	UNUSED(unused);
	UNUSED(callback);

	ctx = EVP_PKEY_CTX_new_id(alginfo->nid, NULL);
	if (ctx == NULL) {
		return dst__openssl_toresult2("EVP_PKEY_CTX_new_id",
					      DST_R_OPENSSLFAILURE);
	}

	status = EVP_PKEY_keygen_init(ctx);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen_init",
					       DST_R_OPENSSLFAILURE));
	}

	status = EVP_PKEY_keygen(ctx, &pkey);
	if (status != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen",
					       DST_R_OPENSSLFAILURE));
	}

	key->key_size = alginfo->key_size * 8;
	key->keydata.pkeypair.priv = pkey;
	key->keydata.pkeypair.pub = pkey;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_CTX_free(ctx);
	return ret;
}

static isc_result_t
openssleddsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	EVP_PKEY *pkey = key->keydata.pkeypair.pub;
	isc_region_t r;
	size_t len;

	REQUIRE(pkey != NULL);
	REQUIRE(alginfo != NULL);

	len = alginfo->key_size;
	isc_buffer_availableregion(data, &r);
	if (r.length < len) {
		return ISC_R_NOSPACE;
	}

	if (EVP_PKEY_get_raw_public_key(pkey, r.base, &len) != 1) {
		return dst__openssl_toresult(ISC_R_FAILURE);
	}

	isc_buffer_add(data, len);
	return ISC_R_SUCCESS;
}

static isc_result_t
openssleddsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	isc_result_t ret;
	isc_region_t r;
	size_t len;
	EVP_PKEY *pkey = NULL;

	REQUIRE(alginfo != NULL);

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		return ISC_R_SUCCESS;
	}

	len = r.length;
	ret = raw_key_to_ossl(alginfo, 0, r.base, &len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		return ret;
	}

	isc_buffer_forward(data, len);
	key->keydata.pkeypair.pub = pkey;
	key->key_size = len * 8;
	return ISC_R_SUCCESS;
}

static isc_result_t
openssleddsa_tofile(const dst_key_t *key, const char *directory) {
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	isc_result_t ret;
	dst_private_t priv;
	unsigned char *buf = NULL;
	size_t len;
	int i;

	REQUIRE(alginfo != NULL);

	if (key->keydata.pkeypair.pub == NULL) {
		return DST_R_NULLKEY;
	}

	if (key->external) {
		priv.nelements = 0;
		return dst__privstruct_writefile(key, &priv, directory);
	}

	i = 0;

	if (dst__openssl_keypair_isprivate(key)) {
		len = alginfo->key_size;
		buf = isc_mem_get(key->mctx, len);
		if (EVP_PKEY_get_raw_private_key(key->keydata.pkeypair.priv,
						 buf, &len) != 1)
		{
			DST_RET(dst__openssl_toresult(ISC_R_FAILURE));
		}
		priv.elements[i].tag = TAG_EDDSA_PRIVATEKEY;
		priv.elements[i].length = len;
		priv.elements[i].data = buf;
		i++;
	}
	if (key->engine != NULL) {
		priv.elements[i].tag = TAG_EDDSA_ENGINE;
		priv.elements[i].length = (unsigned short)strlen(key->engine) +
					  1;
		priv.elements[i].data = (unsigned char *)key->engine;
		i++;
	}
	if (key->label != NULL) {
		priv.elements[i].tag = TAG_EDDSA_LABEL;
		priv.elements[i].length = (unsigned short)strlen(key->label) +
					  1;
		priv.elements[i].data = (unsigned char *)key->label;
		i++;
	}

	priv.nelements = i;
	ret = dst__privstruct_writefile(key, &priv, directory);

err:
	if (buf != NULL) {
		isc_mem_put(key->mctx, buf, len);
	}
	return ret;
}

static isc_result_t
openssleddsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	dst_private_t priv;
	isc_result_t ret;
	int i, privkey_index = -1;
	const char *engine = NULL, *label = NULL;
	EVP_PKEY *pkey = NULL;
	size_t len;
	isc_mem_t *mctx = key->mctx;

	REQUIRE(alginfo != NULL);

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_ED25519, lexer, mctx, &priv);
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
		key->keydata.pkeypair.priv = pub->keydata.pkeypair.priv;
		key->keydata.pkeypair.pub = pub->keydata.pkeypair.pub;
		pub->keydata.pkeypair.priv = NULL;
		pub->keydata.pkeypair.pub = NULL;
		DST_RET(ISC_R_SUCCESS);
	}

	for (i = 0; i < priv.nelements; i++) {
		switch (priv.elements[i].tag) {
		case TAG_EDDSA_ENGINE:
			engine = (char *)priv.elements[i].data;
			break;
		case TAG_EDDSA_LABEL:
			label = (char *)priv.elements[i].data;
			break;
		case TAG_EDDSA_PRIVATEKEY:
			privkey_index = i;
			break;
		default:
			break;
		}
	}

	if (label != NULL) {
		ret = openssleddsa_fromlabel(key, engine, label, NULL);
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
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	len = priv.elements[privkey_index].length;
	ret = raw_key_to_ossl(alginfo, 1, priv.elements[privkey_index].data,
			      &len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}
	/* Check that the public component matches if given */
	if (pub != NULL && EVP_PKEY_eq(pkey, pub->keydata.pkeypair.pub) != 1) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	key->keydata.pkeypair.priv = pkey;
	key->keydata.pkeypair.pub = pkey;
	key->key_size = len * 8;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_free(pkey);
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return ret;
}

static isc_result_t
openssleddsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin) {
	const eddsa_alginfo_t *alginfo = openssleddsa_alg_info(key->key_alg);
	EVP_PKEY *privpkey = NULL, *pubpkey = NULL;
	isc_result_t ret;

	REQUIRE(alginfo != NULL);
	UNUSED(pin);

	ret = dst__openssl_fromlabel(alginfo->pkey_type, engine, label, pin,
				     &pubpkey, &privpkey);
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

static dst_func_t openssleddsa_functions = {
	openssleddsa_createctx,
	NULL, /*%< createctx2 */
	openssleddsa_destroyctx,
	openssleddsa_adddata,
	openssleddsa_sign,
	openssleddsa_verify,
	NULL, /*%< verify2 */
	NULL, /*%< computesecret */
	dst__openssl_keypair_compare,
	NULL, /*%< paramcompare */
	openssleddsa_generate,
	dst__openssl_keypair_isprivate,
	dst__openssl_keypair_destroy,
	openssleddsa_todns,
	openssleddsa_fromdns,
	openssleddsa_tofile,
	openssleddsa_parse,
	NULL, /*%< cleanup */
	openssleddsa_fromlabel,
	NULL, /*%< dump */
	NULL, /*%< restore */
};

/*
 * The test vectors below where generated by util/gen-eddsa-vectors.c
 */
#if HAVE_OPENSSL_ED448
static unsigned char ed448_pub[] =
	"\x0a\x19\x36\xf0\x4c\x2d\xc1\xfe\xbe\xdc\xfa\xf6\xeb\xd2\x8f\x3b\x04"
	"\x14\x2e\x88\xc6\xb5\xdc\xe8\x2a\xc6\xb9\x7c\xa8\x22\xe8\x36\xfb\x06"
	"\x55\xa3\x3c\xdb\x9d\x68\x59\x7e\xa9\x5f\x93\x96\x87\x83\x28\xce\xdd"
	"\x12\xc9\xb8\x78\x02\x80";
static unsigned char ed448_sig[] =
	"\x7e\xec\x4e\x11\xd9\x79\x89\xd2\xe2\x85\x7a\x1c\xd7\x36\xe8\x24\x1f"
	"\x90\xa0\x9c\x84\xfb\x51\xcd\xdc\xfd\x05\xcd\x8c\x08\x51\x05\x18\xc8"
	"\x85\xb2\x28\x00\xea\xfe\x10\x46\xad\x52\xe6\xe9\x62\x35\x3b\x2a\x14"
	"\x8b\xe7\xf0\x66\x5f\x00\x66\x3c\xa1\x4d\x03\x95\xcc\x73\xfc\xf2\x40"
	"\x4b\x67\x85\x5b\x9f\xa9\x87\xb6\xbb\xa3\x9d\x73\x9f\xcb\x4e\x2c\xd2"
	"\x46\xc7\x84\xd3\x7d\x94\x32\x30\x27\xb0\xa7\xf6\x6d\xf4\x77\xe8\xf5"
	"\xb4\xee\x3f\x0e\x2b\x35\xdd\x5a\x35\xfe\x35\x00";
#endif

#if HAVE_OPENSSL_ED25519
static unsigned char ed25519_pub[] =
	"\x66\x5c\x21\x59\xe3\xa0\x6e\xa3\x7d\x82\x7c\xf1\xe7\xa3\xdd\xaf\xd1"
	"\x6d\x92\x81\xfb\x09\x0c\x7c\xfe\x6d\xf8\x87\x24\x7e\x6e\x25";
static unsigned char ed25519_sig[] =
	"\x26\x70\x5c\xc1\x85\xb6\x5e\x65\xe5\xa7\xd5\x85\x63\xc9\x1d\x45\x56"
	"\x38\xa3\x9c\xa3\x42\x4d\xc8\x89\xff\x84\xea\x2c\xa8\x8b\xfa\x2f\xab"
	"\x75\x7c\x68\x95\xfd\xdf\x62\x60\x4e\x4d\x10\xf8\x3c\xae\xcf\x18\x93"
	"\x90\x05\xa4\x54\x38\x45\x2f\x81\x71\x1e\x0f\x46\x04";
#endif

static isc_result_t
check_algorithm(unsigned char algorithm) {
	EVP_MD_CTX *evp_md_ctx = EVP_MD_CTX_create();
	EVP_PKEY *pkey = NULL;
	const eddsa_alginfo_t *alginfo = NULL;
	const unsigned char *key = NULL;
	const unsigned char *sig = NULL;
	const unsigned char test[] = "test";
	isc_result_t ret = ISC_R_SUCCESS;
	size_t key_len, sig_len;

	if (evp_md_ctx == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}

	switch (algorithm) {
#if HAVE_OPENSSL_ED448
	case DST_ALG_ED448:
		sig = ed448_sig;
		sig_len = sizeof(ed448_sig) - 1;
		key = ed448_pub;
		key_len = sizeof(ed448_pub) - 1;
		alginfo = openssleddsa_alg_info(algorithm);
		break;
#endif
#if HAVE_OPENSSL_ED25519
	case DST_ALG_ED25519:
		sig = ed25519_sig;
		sig_len = sizeof(ed25519_sig) - 1;
		key = ed25519_pub;
		key_len = sizeof(ed25519_pub) - 1;
		alginfo = openssleddsa_alg_info(algorithm);
		break;
#endif
	default:
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

	INSIST(alginfo != NULL);
	ret = raw_key_to_ossl(alginfo, 0, key, &key_len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}

	/*
	 * Check that we can verify the signature.
	 */
	if (EVP_DigestVerifyInit(evp_md_ctx, NULL, NULL, NULL, pkey) != 1 ||
	    EVP_DigestVerify(evp_md_ctx, sig, sig_len, test,
			     sizeof(test) - 1) != 1)
	{
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

err:
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
	}
	ERR_clear_error();
	return ret;
}

isc_result_t
dst__openssleddsa_init(dst_func_t **funcp, unsigned char algorithm) {
	REQUIRE(funcp != NULL);

	if (*funcp == NULL) {
		if (check_algorithm(algorithm) == ISC_R_SUCCESS) {
			*funcp = &openssleddsa_functions;
		}
	}
	return ISC_R_SUCCESS;
}

#endif /* HAVE_OPENSSL_ED25519 || HAVE_OPENSSL_ED448 */
