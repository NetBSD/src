/*	$NetBSD: openssleddsa_link.c,v 1.8 2023/01/25 21:43:30 christos Exp $	*/

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

#if HAVE_OPENSSL_ED25519 || HAVE_OPENSSL_ED448

#include <stdbool.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#if !defined(OPENSSL_NO_ENGINE)
#include <openssl/engine.h>
#endif /* if !defined(OPENSSL_NO_ENGINE) */

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/keyvalues.h>

#include <dst/result.h>

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

static isc_result_t
raw_key_to_ossl(unsigned int key_alg, int private, const unsigned char *key,
		size_t *key_len, EVP_PKEY **pkey) {
	isc_result_t ret;
	int pkey_type = EVP_PKEY_NONE;
	size_t len = 0;

#if HAVE_OPENSSL_ED25519
	if (key_alg == DST_ALG_ED25519) {
		pkey_type = EVP_PKEY_ED25519;
		len = DNS_KEY_ED25519SIZE;
	}
#endif /* HAVE_OPENSSL_ED25519 */
#if HAVE_OPENSSL_ED448
	if (key_alg == DST_ALG_ED448) {
		pkey_type = EVP_PKEY_ED448;
		len = DNS_KEY_ED448SIZE;
	}
#endif /* HAVE_OPENSSL_ED448 */
	if (pkey_type == EVP_PKEY_NONE) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	ret = (private ? DST_R_INVALIDPRIVATEKEY : DST_R_INVALIDPUBLICKEY);
	if (*key_len < len) {
		return (ret);
	}

	if (private) {
		*pkey = EVP_PKEY_new_raw_private_key(pkey_type, NULL, key, len);
	} else {
		*pkey = EVP_PKEY_new_raw_public_key(pkey_type, NULL, key, len);
	}
	if (*pkey == NULL) {
		return (dst__openssl_toresult(ret));
	}

	*key_len = len;
	return (ISC_R_SUCCESS);
}

static isc_result_t
openssleddsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin);

static isc_result_t
openssleddsa_createctx(dst_key_t *key, dst_context_t *dctx) {
	isc_buffer_t *buf = NULL;

	UNUSED(key);
	REQUIRE(dctx->key->key_alg == DST_ALG_ED25519 ||
		dctx->key->key_alg == DST_ALG_ED448);

	isc_buffer_allocate(dctx->mctx, &buf, 64);
	dctx->ctxdata.generic = buf;

	return (ISC_R_SUCCESS);
}

static void
openssleddsa_destroyctx(dst_context_t *dctx) {
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;

	REQUIRE(dctx->key->key_alg == DST_ALG_ED25519 ||
		dctx->key->key_alg == DST_ALG_ED448);
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

	REQUIRE(dctx->key->key_alg == DST_ALG_ED25519 ||
		dctx->key->key_alg == DST_ALG_ED448);

	result = isc_buffer_copyregion(buf, data);
	if (result == ISC_R_SUCCESS) {
		return (ISC_R_SUCCESS);
	}

	length = isc_buffer_length(buf) + data->length + 64;
	isc_buffer_allocate(dctx->mctx, &nbuf, length);
	isc_buffer_usedregion(buf, &r);
	(void)isc_buffer_copyregion(nbuf, &r);
	(void)isc_buffer_copyregion(nbuf, data);
	isc_buffer_free(&buf);
	dctx->ctxdata.generic = nbuf;

	return (ISC_R_SUCCESS);
}

static isc_result_t
openssleddsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	isc_region_t tbsreg;
	isc_region_t sigreg;
	EVP_PKEY *pkey = key->keydata.pkey;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	size_t siglen;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

	if (ctx == NULL) {
		return (ISC_R_NOMEMORY);
	}

	if (key->key_alg == DST_ALG_ED25519) {
		siglen = DNS_SIG_ED25519SIZE;
	} else {
		siglen = DNS_SIG_ED448SIZE;
	}

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

	return (ret);
}

static isc_result_t
openssleddsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	isc_result_t ret;
	dst_key_t *key = dctx->key;
	int status;
	isc_region_t tbsreg;
	EVP_PKEY *pkey = key->keydata.pkey;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	isc_buffer_t *buf = (isc_buffer_t *)dctx->ctxdata.generic;
	unsigned int siglen = 0;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

	if (ctx == NULL) {
		return (ISC_R_NOMEMORY);
	}

#if HAVE_OPENSSL_ED25519
	if (key->key_alg == DST_ALG_ED25519) {
		siglen = DNS_SIG_ED25519SIZE;
	}
#endif /* if HAVE_OPENSSL_ED25519 */
#if HAVE_OPENSSL_ED448
	if (key->key_alg == DST_ALG_ED448) {
		siglen = DNS_SIG_ED448SIZE;
	}
#endif /* if HAVE_OPENSSL_ED448 */
	if (siglen == 0) {
		DST_RET(ISC_R_NOTIMPLEMENTED);
	}

	if (sig->length != siglen) {
		DST_RET(DST_R_VERIFYFAILURE);
	}

	isc_buffer_usedregion(buf, &tbsreg);

	if (EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) != 1) {
		DST_RET(dst__openssl_toresult3(
			dctx->category, "EVP_DigestVerifyInit", ISC_R_FAILURE));
	}

	status = EVP_DigestVerify(ctx, sig->base, siglen, tbsreg.base,
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

	return (ret);
}

static bool
openssleddsa_compare(const dst_key_t *key1, const dst_key_t *key2) {
	int status;
	EVP_PKEY *pkey1 = key1->keydata.pkey;
	EVP_PKEY *pkey2 = key2->keydata.pkey;

	if (pkey1 == NULL && pkey2 == NULL) {
		return (true);
	} else if (pkey1 == NULL || pkey2 == NULL) {
		return (false);
	}

	status = EVP_PKEY_cmp(pkey1, pkey2);
	if (status == 1) {
		return (true);
	}
	return (false);
}

static isc_result_t
openssleddsa_generate(dst_key_t *key, int unused, void (*callback)(int)) {
	isc_result_t ret;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	int nid = 0, status;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);
	UNUSED(unused);
	UNUSED(callback);

#if HAVE_OPENSSL_ED25519
	if (key->key_alg == DST_ALG_ED25519) {
		nid = NID_ED25519;
		key->key_size = DNS_KEY_ED25519SIZE * 8;
	}
#endif /* if HAVE_OPENSSL_ED25519 */
#if HAVE_OPENSSL_ED448
	if (key->key_alg == DST_ALG_ED448) {
		nid = NID_ED448;
		key->key_size = DNS_KEY_ED448SIZE * 8;
	}
#endif /* if HAVE_OPENSSL_ED448 */
	if (nid == 0) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	ctx = EVP_PKEY_CTX_new_id(nid, NULL);
	if (ctx == NULL) {
		return (dst__openssl_toresult2("EVP_PKEY_CTX_new_id",
					       DST_R_OPENSSLFAILURE));
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

	key->keydata.pkey = pkey;
	ret = ISC_R_SUCCESS;

err:
	EVP_PKEY_CTX_free(ctx);
	return (ret);
}

static bool
openssleddsa_isprivate(const dst_key_t *key) {
	EVP_PKEY *pkey = key->keydata.pkey;
	size_t len;

	if (pkey == NULL) {
		return (false);
	}

	if (EVP_PKEY_get_raw_private_key(pkey, NULL, &len) == 1 && len > 0) {
		return (true);
	}
	/* can check if first error is EC_R_INVALID_PRIVATE_KEY */
	while (ERR_get_error() != 0) {
		/**/
	}

	return (false);
}

static void
openssleddsa_destroy(dst_key_t *key) {
	EVP_PKEY *pkey = key->keydata.pkey;

	EVP_PKEY_free(pkey);
	key->keydata.pkey = NULL;
}

static isc_result_t
openssleddsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	EVP_PKEY *pkey = key->keydata.pkey;
	isc_region_t r;
	size_t len;

	REQUIRE(pkey != NULL);
	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

	if (key->key_alg == DST_ALG_ED25519) {
		len = DNS_KEY_ED25519SIZE;
	} else {
		len = DNS_KEY_ED448SIZE;
	}

	isc_buffer_availableregion(data, &r);
	if (r.length < len) {
		return (ISC_R_NOSPACE);
	}

	if (EVP_PKEY_get_raw_public_key(pkey, r.base, &len) != 1) {
		return (dst__openssl_toresult(ISC_R_FAILURE));
	}

	isc_buffer_add(data, len);
	return (ISC_R_SUCCESS);
}

static isc_result_t
openssleddsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
	isc_region_t r;
	size_t len;
	EVP_PKEY *pkey;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		return (ISC_R_SUCCESS);
	}

	len = r.length;
	ret = raw_key_to_ossl(key->key_alg, 0, r.base, &len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		return ret;
	}

	isc_buffer_forward(data, len);
	key->keydata.pkey = pkey;
	key->key_size = len * 8;
	return (ISC_R_SUCCESS);
}

static isc_result_t
openssleddsa_tofile(const dst_key_t *key, const char *directory) {
	isc_result_t ret;
	dst_private_t priv;
	unsigned char *buf = NULL;
	size_t len;
	int i;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

	if (key->keydata.pkey == NULL) {
		return (DST_R_NULLKEY);
	}

	if (key->external) {
		priv.nelements = 0;
		return (dst__privstruct_writefile(key, &priv, directory));
	}

	i = 0;

	if (openssleddsa_isprivate(key)) {
		if (key->key_alg == DST_ALG_ED25519) {
			len = DNS_KEY_ED25519SIZE;
		} else {
			len = DNS_KEY_ED448SIZE;
		}
		buf = isc_mem_get(key->mctx, len);
		if (EVP_PKEY_get_raw_private_key(key->keydata.pkey, buf,
						 &len) != 1)
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
	return (ret);
}

static isc_result_t
eddsa_check(EVP_PKEY *pkey, EVP_PKEY *pubpkey) {
	if (pubpkey == NULL) {
		return (ISC_R_SUCCESS);
	}
	if (EVP_PKEY_cmp(pkey, pubpkey) == 1) {
		return (ISC_R_SUCCESS);
	}
	return (ISC_R_FAILURE);
}

static isc_result_t
openssleddsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i, privkey_index = -1;
	const char *engine = NULL, *label = NULL;
	EVP_PKEY *pkey = NULL, *pubpkey = NULL;
	size_t len;
	isc_mem_t *mctx = key->mctx;

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

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
		key->keydata.pkey = pub->keydata.pkey;
		pub->keydata.pkey = NULL;
		dst__privstruct_free(&priv, mctx);
		isc_safe_memwipe(&priv, sizeof(priv));
		return (ISC_R_SUCCESS);
	}

	if (pub != NULL) {
		pubpkey = pub->keydata.pkey;
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
		if (eddsa_check(key->keydata.pkey, pubpkey) != ISC_R_SUCCESS) {
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		}
		DST_RET(ISC_R_SUCCESS);
	}

	if (privkey_index < 0) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	len = priv.elements[privkey_index].length;
	ret = raw_key_to_ossl(key->key_alg, 1,
			      priv.elements[privkey_index].data, &len, &pkey);
	if (ret != ISC_R_SUCCESS) {
		goto err;
	}
	if (eddsa_check(pkey, pubpkey) != ISC_R_SUCCESS) {
		EVP_PKEY_free(pkey);
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}
	key->keydata.pkey = pkey;
	key->key_size = len * 8;
	ret = ISC_R_SUCCESS;

err:
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (ret);
}

static isc_result_t
openssleddsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		       const char *pin) {
#if !defined(OPENSSL_NO_ENGINE)
	isc_result_t ret;
	ENGINE *e;
	EVP_PKEY *pkey = NULL, *pubpkey = NULL;
	int baseid = EVP_PKEY_NONE;

	UNUSED(pin);

	REQUIRE(key->key_alg == DST_ALG_ED25519 ||
		key->key_alg == DST_ALG_ED448);

#if HAVE_OPENSSL_ED25519
	if (key->key_alg == DST_ALG_ED25519) {
		baseid = EVP_PKEY_ED25519;
	}
#endif /* if HAVE_OPENSSL_ED25519 */
#if HAVE_OPENSSL_ED448
	if (key->key_alg == DST_ALG_ED448) {
		baseid = EVP_PKEY_ED448;
	}
#endif /* if HAVE_OPENSSL_ED448 */
	if (baseid == EVP_PKEY_NONE) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	if (engine == NULL) {
		return (DST_R_NOENGINE);
	}
	e = dst__openssl_getengine(engine);
	if (e == NULL) {
		return (DST_R_NOENGINE);
	}
	pkey = ENGINE_load_private_key(e, label, NULL, NULL);
	if (pkey == NULL) {
		return (dst__openssl_toresult2("ENGINE_load_private_key",
					       ISC_R_NOTFOUND));
	}
	if (EVP_PKEY_base_id(pkey) != baseid) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	pubpkey = ENGINE_load_public_key(e, label, NULL, NULL);
	if (eddsa_check(pkey, pubpkey) != ISC_R_SUCCESS) {
		DST_RET(DST_R_INVALIDPRIVATEKEY);
	}

	key->engine = isc_mem_strdup(key->mctx, engine);
	key->label = isc_mem_strdup(key->mctx, label);
	key->key_size = EVP_PKEY_bits(pkey);
	key->keydata.pkey = pkey;
	pkey = NULL;
	ret = ISC_R_SUCCESS;

err:
	if (pubpkey != NULL) {
		EVP_PKEY_free(pubpkey);
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

static dst_func_t openssleddsa_functions = {
	openssleddsa_createctx,
	NULL, /*%< createctx2 */
	openssleddsa_destroyctx,
	openssleddsa_adddata,
	openssleddsa_sign,
	openssleddsa_verify,
	NULL, /*%< verify2 */
	NULL, /*%< computesecret */
	openssleddsa_compare,
	NULL, /*%< paramcompare */
	openssleddsa_generate,
	openssleddsa_isprivate,
	openssleddsa_destroy,
	openssleddsa_todns,
	openssleddsa_fromdns,
	openssleddsa_tofile,
	openssleddsa_parse,
	NULL, /*%< cleanup */
	openssleddsa_fromlabel,
	NULL, /*%< dump */
	NULL, /*%< restore */
};

isc_result_t
dst__openssleddsa_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL) {
		*funcp = &openssleddsa_functions;
	}
	return (ISC_R_SUCCESS);
}

#endif /* HAVE_OPENSSL_ED25519 || HAVE_OPENSSL_ED448 */

#endif /* !USE_PKCS11 */

/*! \file */
