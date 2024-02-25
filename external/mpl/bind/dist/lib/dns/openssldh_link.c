/*	$NetBSD: openssldh_link.c,v 1.8.2.2 2024/02/25 15:46:51 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*! \file */

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#endif
#include <openssl/err.h>
#include <openssl/objects.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/param_build.h>
#endif
#include <openssl/dh.h>

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"
#include "openssl_shim.h"

#define PRIME2 "02"

#define PRIME768                                                               \
	"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088"            \
	"A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25"   \
	"F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A63A3620FFFFFFFFFFFFFFF" \
	"F"

#define PRIME1024                                                            \
	"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"           \
	"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF2" \
	"5F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406" \
	"B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381FFFFFFFFFFFFFFFF"

#define PRIME1536                                          \
	"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
	"29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
	"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
	"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
	"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D" \
	"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F" \
	"83655D23DCA3AD961C62F356208552BB9ED529077096966D" \
	"670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

static BIGNUM *bn2 = NULL, *bn768 = NULL, *bn1024 = NULL, *bn1536 = NULL;

static isc_result_t
openssldh_computesecret(const dst_key_t *pub, const dst_key_t *priv,
			isc_buffer_t *secret) {
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dhpub, *dhpriv;
	const BIGNUM *pub_key = NULL;
	int secret_len = 0;
#else
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *dhpub, *dhpriv;
	size_t secret_len = 0;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	isc_region_t r;
	unsigned int len;

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	REQUIRE(pub->keydata.dh != NULL);
	REQUIRE(priv->keydata.dh != NULL);

	dhpub = pub->keydata.dh;
	dhpriv = priv->keydata.dh;

	len = DH_size(dhpriv);
#else
	REQUIRE(pub->keydata.pkey != NULL);
	REQUIRE(priv->keydata.pkey != NULL);

	dhpub = pub->keydata.pkey;
	dhpriv = priv->keydata.pkey;

	len = EVP_PKEY_get_size(dhpriv);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	isc_buffer_availableregion(secret, &r);
	if (r.length < len) {
		return (ISC_R_NOSPACE);
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH_get0_key(dhpub, &pub_key, NULL);
	secret_len = DH_compute_key(r.base, pub_key, dhpriv);
	if (secret_len <= 0) {
		return (dst__openssl_toresult2("DH_compute_key",
					       DST_R_COMPUTESECRETFAILURE));
	}
#else
	ctx = EVP_PKEY_CTX_new_from_pkey(NULL, dhpriv, NULL);
	if (ctx == NULL) {
		return (dst__openssl_toresult2("EVP_PKEY_CTX_new_from_pkey",
					       DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_derive_init(ctx) != 1) {
		EVP_PKEY_CTX_free(ctx);
		return (dst__openssl_toresult2("EVP_PKEY_derive_init",
					       DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_derive_set_peer(ctx, dhpub) != 1) {
		EVP_PKEY_CTX_free(ctx);
		return (dst__openssl_toresult2("EVP_PKEY_derive_set_peer",
					       DST_R_OPENSSLFAILURE));
	}
	secret_len = r.length;
	if (EVP_PKEY_derive(ctx, r.base, &secret_len) != 1 || secret_len == 0) {
		EVP_PKEY_CTX_free(ctx);
		return (dst__openssl_toresult2("EVP_PKEY_derive",
					       DST_R_COMPUTESECRETFAILURE));
	}
	EVP_PKEY_CTX_free(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	isc_buffer_add(secret, (unsigned int)secret_len);

	return (ISC_R_SUCCESS);
}

static bool
openssldh_compare(const dst_key_t *key1, const dst_key_t *key2) {
	bool ret = true;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh1, *dh2;
	const BIGNUM *pub_key1 = NULL, *pub_key2 = NULL;
	const BIGNUM *priv_key1 = NULL, *priv_key2 = NULL;
	const BIGNUM *p1 = NULL, *g1 = NULL, *p2 = NULL, *g2 = NULL;
#else
	EVP_PKEY *pkey1, *pkey2;
	BIGNUM *pub_key1 = NULL, *pub_key2 = NULL;
	BIGNUM *priv_key1 = NULL, *priv_key2 = NULL;
	BIGNUM *p1 = NULL, *g1 = NULL, *p2 = NULL, *g2 = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	dh1 = key1->keydata.dh;
	dh2 = key2->keydata.dh;

	if (dh1 == NULL && dh2 == NULL) {
		return (true);
	} else if (dh1 == NULL || dh2 == NULL) {
		return (false);
	}

	DH_get0_key(dh1, &pub_key1, &priv_key1);
	DH_get0_key(dh2, &pub_key2, &priv_key2);
	DH_get0_pqg(dh1, &p1, NULL, &g1);
	DH_get0_pqg(dh2, &p2, NULL, &g2);
#else
	pkey1 = key1->keydata.pkey;
	pkey2 = key2->keydata.pkey;

	if (pkey1 == NULL && pkey2 == NULL) {
		return (true);
	} else if (pkey1 == NULL || pkey2 == NULL) {
		return (false);
	}

	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_FFC_P, &p1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_FFC_P, &p2);
	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_FFC_G, &g1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_FFC_G, &g2);
	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_PUB_KEY, &pub_key1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_PUB_KEY, &pub_key2);
	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_PRIV_KEY, &priv_key1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_PRIV_KEY, &priv_key2);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000*/

	if (BN_cmp(p1, p2) != 0 || BN_cmp(g1, g2) != 0 ||
	    BN_cmp(pub_key1, pub_key2) != 0)
	{
		DST_RET(false);
	}

	if (priv_key1 != NULL || priv_key2 != NULL) {
		if (priv_key1 == NULL || priv_key2 == NULL ||
		    BN_cmp(priv_key1, priv_key2) != 0)
		{
			DST_RET(false);
		}
	}

err:
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
	if (p1 != NULL) {
		BN_free(p1);
	}
	if (p2 != NULL) {
		BN_free(p2);
	}
	if (g1 != NULL) {
		BN_free(g1);
	}
	if (g2 != NULL) {
		BN_free(g2);
	}
	if (pub_key1 != NULL) {
		BN_free(pub_key1);
	}
	if (pub_key2 != NULL) {
		BN_free(pub_key2);
	}
	if (priv_key1 != NULL) {
		BN_clear_free(priv_key1);
	}
	if (priv_key2 != NULL) {
		BN_clear_free(priv_key2);
	}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000 \
	*/

	return (ret);
}

static bool
openssldh_paramcompare(const dst_key_t *key1, const dst_key_t *key2) {
	bool ret = true;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh1, *dh2;
	const BIGNUM *p1 = NULL, *g1 = NULL, *p2 = NULL, *g2 = NULL;
#else
	EVP_PKEY *pkey1, *pkey2;
	BIGNUM *p1 = NULL, *g1 = NULL, *p2 = NULL, *g2 = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	dh1 = key1->keydata.dh;
	dh2 = key2->keydata.dh;

	if (dh1 == NULL && dh2 == NULL) {
		return (true);
	} else if (dh1 == NULL || dh2 == NULL) {
		return (false);
	}

	DH_get0_pqg(dh1, &p1, NULL, &g1);
	DH_get0_pqg(dh2, &p2, NULL, &g2);
#else
	pkey1 = key1->keydata.pkey;
	pkey2 = key2->keydata.pkey;

	if (pkey1 == NULL && pkey2 == NULL) {
		return (true);
	} else if (pkey1 == NULL || pkey2 == NULL) {
		return (false);
	}

	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_FFC_P, &p1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_FFC_P, &p2);
	EVP_PKEY_get_bn_param(pkey1, OSSL_PKEY_PARAM_FFC_G, &g1);
	EVP_PKEY_get_bn_param(pkey2, OSSL_PKEY_PARAM_FFC_G, &g2);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	if (BN_cmp(p1, p2) != 0 || BN_cmp(g1, g2) != 0) {
		DST_RET(false);
	}

err:
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
	if (p1 != NULL) {
		BN_free(p1);
	}
	if (p2 != NULL) {
		BN_free(p2);
	}
	if (g1 != NULL) {
		BN_free(g1);
	}
	if (g2 != NULL) {
		BN_free(g2);
	}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000 \
	*/

	return (ret);
}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
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
#else
static int
progress_cb(EVP_PKEY_CTX *ctx) {
	union {
		void *dptr;
		void (*fptr)(int);
	} u;

	u.dptr = EVP_PKEY_CTX_get_app_data(ctx);
	if (u.fptr != NULL) {
		int p = EVP_PKEY_CTX_get_keygen_info(ctx, 0);
		u.fptr(p);
	}
	return (1);
}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

static isc_result_t
openssldh_generate(dst_key_t *key, int generator, void (*callback)(int)) {
	isc_result_t ret;
	union {
		void *dptr;
		void (*fptr)(int);
	} u;
	BIGNUM *p = NULL, *g = NULL;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh = NULL;
	BN_GENCB *cb = NULL;
#if !HAVE_BN_GENCB_NEW
	BN_GENCB _cb;
#endif /* !HAVE_BN_GENCB_NEW */
#else
	OSSL_PARAM_BLD *bld = NULL;
	OSSL_PARAM *params = NULL;
	EVP_PKEY_CTX *param_ctx = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *param_pkey = NULL;
	EVP_PKEY *pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	dh = DH_new();
	if (dh == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
#else
	bld = OSSL_PARAM_BLD_new();
	if (bld == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	param_ctx = EVP_PKEY_CTX_new_from_name(NULL, "DH", NULL);
	if (param_ctx == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	if (generator == 0) {
		/*
		 * When `generator` is 0, we have three pre-computed `p` and `g`
		 * static parameters which we can use.
		 */
		if (key->key_size == 768 || key->key_size == 1024 ||
		    key->key_size == 1536)
		{
			if (key->key_size == 768) {
				p = BN_dup(bn768);
			} else if (key->key_size == 1024) {
				p = BN_dup(bn1024);
			} else {
				p = BN_dup(bn1536);
			}
			g = BN_dup(bn2);
			if (p == NULL || g == NULL) {
				DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
			}
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
			if (DH_set0_pqg(dh, p, NULL, g) != 1) {
				DST_RET(dst__openssl_toresult2(
					"DH_set0_pqg", DST_R_OPENSSLFAILURE));
			}
#else
			if (OSSL_PARAM_BLD_push_uint(bld,
						     OSSL_PKEY_PARAM_FFC_PBITS,
						     key->key_size) != 1)
			{
				DST_RET(dst__openssl_toresult2(
					"OSSL_PARAM_BLD_push_uint",
					DST_R_OPENSSLFAILURE));
			}
			if (OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_P,
						   p) != 1 ||
			    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_G,
						   g) != 1)
			{
				DST_RET(dst__openssl_toresult2(
					"OSSL_PARAM_BLD_push_BN",
					DST_R_OPENSSLFAILURE));
			}
			params = OSSL_PARAM_BLD_to_param(bld);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

		} else {
			/*
			 * If the requested size is not present in our
			 * pre-computed set, we will use `generator` 2 to
			 * generate new parameters.
			 */
			generator = 2;
		}
	}

	if (generator != 0) {
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
		cb = BN_GENCB_new();
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
		if (cb == NULL) {
			DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
		}
#endif /* if OPENSSL_VERSION_NUMBER >= 0x10100000L && \
	* !defined(LIBRESSL_VERSION_NUMBER) */
		if (callback == NULL) {
			BN_GENCB_set_old(cb, NULL, NULL);
		} else {
			u.fptr = callback;
			BN_GENCB_set(cb, progress_cb, u.dptr);
		}

		if (!DH_generate_parameters_ex(dh, key->key_size, generator,
					       cb))
		{
			DST_RET(dst__openssl_toresult2("DH_generate_parameters_"
						       "ex",
						       DST_R_OPENSSLFAILURE));
		}
#else
		if (OSSL_PARAM_BLD_push_int(bld, OSSL_PKEY_PARAM_DH_GENERATOR,
					    generator) != 1)
		{
			DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_"
						       "int",
						       DST_R_OPENSSLFAILURE));
		}
		if (OSSL_PARAM_BLD_push_utf8_string(
			    bld, OSSL_PKEY_PARAM_FFC_TYPE, "generator", 0) != 1)
		{
			DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_"
						       "utf8_string",
						       DST_R_OPENSSLFAILURE));
		}
		if (OSSL_PARAM_BLD_push_uint(bld, OSSL_PKEY_PARAM_FFC_PBITS,
					     key->key_size) != 1)
		{
			DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_"
						       "uint",
						       DST_R_OPENSSLFAILURE));
		}
		params = OSSL_PARAM_BLD_to_param(bld);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (DH_generate_key(dh) == 0) {
		DST_RET(dst__openssl_toresult2("DH_generate_key",
					       DST_R_OPENSSLFAILURE));
	}
	DH_clear_flags(dh, DH_FLAG_CACHE_MONT_P);
	key->keydata.dh = dh;
	dh = NULL;
#else
	if (params == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}

	if (generator == 0) {
		if (EVP_PKEY_fromdata_init(param_ctx) != 1) {
			DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata_init",
						       DST_R_OPENSSLFAILURE));
		}
		if (EVP_PKEY_fromdata(param_ctx, &param_pkey,
				      OSSL_KEYMGMT_SELECT_ALL, params) != 1 ||
		    param_pkey == NULL)
		{
			DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata",
						       DST_R_OPENSSLFAILURE));
		}
	} else {
		if (EVP_PKEY_paramgen_init(param_ctx) != 1) {
			DST_RET(dst__openssl_toresult2("EVP_PKEY_paramgen_init",
						       DST_R_OPENSSLFAILURE));
		}
		if (EVP_PKEY_CTX_set_params(param_ctx, params) != 1) {
			DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_set_"
						       "params",
						       DST_R_OPENSSLFAILURE));
		}
		if (EVP_PKEY_paramgen(param_ctx, &param_pkey) != 1 ||
		    param_pkey == NULL)
		{
			DST_RET(dst__openssl_toresult2("EVP_PKEY_paramgen",
						       DST_R_OPENSSLFAILURE));
		}
	}

	/*
	 * Now `param_pkey` holds the DH parameters (either pre-coumputed or
	 * newly generated) so we will generate a new public/private key-pair
	 * using those parameters and put it into `pkey`.
	 */
	ctx = EVP_PKEY_CTX_new_from_pkey(NULL, param_pkey, NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_CTX_new_from_pkey",
					       DST_R_OPENSSLFAILURE));
	}
	if (callback != NULL) {
		u.fptr = callback;
		EVP_PKEY_CTX_set_app_data(ctx, u.dptr);
		EVP_PKEY_CTX_set_cb(ctx, progress_cb);
	}
	if (EVP_PKEY_keygen_init(ctx) != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen_init",
					       DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_keygen(ctx, &pkey) != 1 || pkey == NULL) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_keygen",
					       DST_R_OPENSSLFAILURE));
	}

	key->keydata.pkey = pkey;
	pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	ret = ISC_R_SUCCESS;

err:
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (dh != NULL) {
		DH_free(dh);
	}
	if (cb != NULL) {
		BN_GENCB_free(cb);
	}
#else
	if (param_pkey != NULL) {
		EVP_PKEY_free(param_pkey);
	}
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (param_ctx != NULL) {
		EVP_PKEY_CTX_free(param_ctx);
	}
	if (ctx != NULL) {
		EVP_PKEY_CTX_free(ctx);
	}
	if (params != NULL) {
		OSSL_PARAM_free(params);
	}
	if (bld != NULL) {
		OSSL_PARAM_BLD_free(bld);
	}
	if (p != NULL) {
		BN_free(p);
	}
	if (g != NULL) {
		BN_free(g);
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	return (ret);
}

static bool
openssldh_isprivate(const dst_key_t *key) {
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh = key->keydata.dh;
	const BIGNUM *priv_key = NULL;

	DH_get0_key(dh, NULL, &priv_key);

	return (dh != NULL && priv_key != NULL);
#else
	bool ret;
	EVP_PKEY *pkey;
	BIGNUM *priv_key = NULL;

	pkey = key->keydata.pkey;
	if (pkey == NULL) {
		return (false);
	}

	ret = (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PRIV_KEY,
				     &priv_key) == 1 &&
	       priv_key != NULL);
	if (priv_key != NULL) {
		BN_clear_free(priv_key);
	}

	return (ret);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
}

static void
openssldh_destroy(dst_key_t *key) {
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh = key->keydata.dh;

	if (dh == NULL) {
		return;
	}

	DH_free(dh);
	key->keydata.dh = NULL;
#else
	EVP_PKEY *pkey = key->keydata.pkey;

	if (pkey == NULL) {
		return;
	}

	EVP_PKEY_free(pkey);
	key->keydata.pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
}

static void
uint16_toregion(uint16_t val, isc_region_t *region) {
	*region->base = (val & 0xff00) >> 8;
	isc_region_consume(region, 1);
	*region->base = (val & 0x00ff);
	isc_region_consume(region, 1);
}

static uint16_t
uint16_fromregion(isc_region_t *region) {
	uint16_t val;
	unsigned char *cp = region->base;

	val = ((unsigned int)(cp[0])) << 8;
	val |= ((unsigned int)(cp[1]));

	isc_region_consume(region, 2);

	return (val);
}

static isc_result_t
openssldh_todns(const dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret = ISC_R_SUCCESS;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh;
	const BIGNUM *pub_key = NULL, *p = NULL, *g = NULL;
#else
	EVP_PKEY *pkey;
	BIGNUM *pub_key = NULL, *p = NULL, *g = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	isc_region_t r;
	uint16_t dnslen, plen, glen, publen;

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	REQUIRE(key->keydata.dh != NULL);

	dh = key->keydata.dh;
	DH_get0_pqg(dh, &p, NULL, &g);
	DH_get0_key(dh, &pub_key, NULL);
#else
	REQUIRE(key->keydata.pkey != NULL);

	pkey = key->keydata.pkey;
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_FFC_P, &p);
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_FFC_G, &g);
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, &pub_key);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	isc_buffer_availableregion(data, &r);

	if (BN_cmp(g, bn2) == 0 &&
	    (BN_cmp(p, bn768) == 0 || BN_cmp(p, bn1024) == 0 ||
	     BN_cmp(p, bn1536) == 0))
	{
		plen = 1;
		glen = 0;
	} else {
		plen = BN_num_bytes(p);
		glen = BN_num_bytes(g);
	}

	publen = BN_num_bytes(pub_key);
	dnslen = plen + glen + publen + 6;
	if (r.length < (unsigned int)dnslen) {
		DST_RET(ISC_R_NOSPACE);
	}

	uint16_toregion(plen, &r);
	if (plen == 1) {
		if (BN_cmp(p, bn768) == 0) {
			*r.base = 1;
		} else if (BN_cmp(p, bn1024) == 0) {
			*r.base = 2;
		} else {
			*r.base = 3;
		}
	} else {
		BN_bn2bin(p, r.base);
	}
	isc_region_consume(&r, plen);

	uint16_toregion(glen, &r);
	if (glen > 0) {
		BN_bn2bin(g, r.base);
	}
	isc_region_consume(&r, glen);

	uint16_toregion(publen, &r);
	BN_bn2bin(pub_key, r.base);
	isc_region_consume(&r, publen);

	isc_buffer_add(data, dnslen);

err:
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
	if (p != NULL) {
		BN_free(p);
	}
	if (g != NULL) {
		BN_free(g);
	}
	if (pub_key != NULL) {
		BN_free(pub_key);
	}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000 \
	*/

	return (ret);
}

static isc_result_t
openssldh_fromdns(dst_key_t *key, isc_buffer_t *data) {
	isc_result_t ret;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh;
#else
	OSSL_PARAM_BLD *bld = NULL;
	OSSL_PARAM *params = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	BIGNUM *pub_key = NULL, *p = NULL, *g = NULL;
	int key_size;
	isc_region_t r;
	uint16_t plen, glen, publen;
	int special = 0;

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		return (ISC_R_SUCCESS);
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	dh = DH_new();
	if (dh == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}
	DH_clear_flags(dh, DH_FLAG_CACHE_MONT_P);
#else
	bld = OSSL_PARAM_BLD_new();
	if (bld == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	ctx = EVP_PKEY_CTX_new_from_name(NULL, "DH", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	/*
	 * Read the prime length.  1 & 2 are table entries, > 16 means a
	 * prime follows, otherwise an error.
	 */
	if (r.length < 2) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	plen = uint16_fromregion(&r);
	if (plen < 16 && plen != 1 && plen != 2) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	if (r.length < plen) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	if (plen == 1 || plen == 2) {
		if (plen == 1) {
			special = *r.base;
			isc_region_consume(&r, 1);
		} else {
			special = uint16_fromregion(&r);
		}
		switch (special) {
		case 1:
			p = BN_dup(bn768);
			break;
		case 2:
			p = BN_dup(bn1024);
			break;
		case 3:
			p = BN_dup(bn1536);
			break;
		default:
			DST_RET(DST_R_INVALIDPUBLICKEY);
		}
	} else {
		p = BN_bin2bn(r.base, plen, NULL);
		isc_region_consume(&r, plen);
	}

	/*
	 * Read the generator length.  This should be 0 if the prime was
	 * special, but it might not be.  If it's 0 and the prime is not
	 * special, we have a problem.
	 */
	if (r.length < 2) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	glen = uint16_fromregion(&r);
	if (r.length < glen) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	if (special != 0) {
		if (glen == 0) {
			g = BN_dup(bn2);
		} else {
			g = BN_bin2bn(r.base, glen, NULL);
			if (g != NULL && BN_cmp(g, bn2) != 0) {
				DST_RET(DST_R_INVALIDPUBLICKEY);
			}
		}
	} else {
		if (glen == 0) {
			DST_RET(DST_R_INVALIDPUBLICKEY);
		}
		g = BN_bin2bn(r.base, glen, NULL);
	}
	isc_region_consume(&r, glen);

	if (p == NULL || g == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}

	key_size = BN_num_bits(p);

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (DH_set0_pqg(dh, p, NULL, g) != 1) {
		DST_RET(dst__openssl_toresult2("DH_set0_pqg",
					       DST_R_OPENSSLFAILURE));
	}

	/* These are now managed by OpenSSL */
	p = NULL;
	g = NULL;
#else
	if (OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_P, p) != 1 ||
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_G, g) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	if (r.length < 2) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	publen = uint16_fromregion(&r);
	if (r.length < publen) {
		DST_RET(DST_R_INVALIDPUBLICKEY);
	}
	pub_key = BN_bin2bn(r.base, publen, NULL);
	if (pub_key == NULL) {
		DST_RET(dst__openssl_toresult(ISC_R_NOMEMORY));
	}

	isc_region_consume(&r, publen);

	isc_buffer_forward(data, plen + glen + publen + 6);

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
#if (LIBRESSL_VERSION_NUMBER >= 0x2070000fL) && \
	(LIBRESSL_VERSION_NUMBER <= 0x2070200fL)
	/*
	 * LibreSSL << 2.7.3 DH_get0_key requires priv_key to be set when
	 * DH structure is empty, hence we cannot use DH_get0_key().
	 */
	dh->pub_key = pub_key;
#else  /* LIBRESSL_VERSION_NUMBER */
	if (DH_set0_key(dh, pub_key, NULL) != 1) {
		DST_RET(dst__openssl_toresult2("DH_set0_key",
					       DST_R_OPENSSLFAILURE));
	}
#endif /* LIBRESSL_VERSION_NUMBER */

	/* This is now managed by OpenSSL */
	pub_key = NULL;

	key->keydata.dh = dh;
	dh = NULL;
#else
	if (OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PUB_KEY, pub_key) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	params = OSSL_PARAM_BLD_to_param(bld);
	if (params == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_fromdata_init(ctx) != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata_init",
					       DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_fromdata(ctx, &pkey, OSSL_KEYMGMT_SELECT_ALL, params) !=
		    1 ||
	    pkey == NULL)
	{
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata",
					       DST_R_OPENSSLFAILURE));
	}

	key->keydata.pkey = pkey;
	pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	key->key_size = (unsigned int)key_size;

	ret = ISC_R_SUCCESS;

err:
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (dh != NULL) {
		DH_free(dh);
	}
#else
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (ctx != NULL) {
		EVP_PKEY_CTX_free(ctx);
	}
	if (params != NULL) {
		OSSL_PARAM_free(params);
	}
	if (bld != NULL) {
		OSSL_PARAM_BLD_free(bld);
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	if (p != NULL) {
		BN_free(p);
	}
	if (g != NULL) {
		BN_free(g);
	}
	if (pub_key != NULL) {
		BN_free(pub_key);
	}

	return (ret);
}

static isc_result_t
openssldh_tofile(const dst_key_t *key, const char *directory) {
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh;
	const BIGNUM *pub_key = NULL, *priv_key = NULL, *p = NULL, *g = NULL;
#else
	EVP_PKEY *pkey;
	BIGNUM *pub_key = NULL, *priv_key = NULL, *p = NULL, *g = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	dst_private_t priv;
	unsigned char *bufs[4] = { NULL };
	unsigned short i = 0;
	isc_result_t result;

	if (key->external) {
		return (DST_R_EXTERNALKEY);
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (key->keydata.dh == NULL) {
		return (DST_R_NULLKEY);
	}

	dh = key->keydata.dh;
	DH_get0_key(dh, &pub_key, &priv_key);
	DH_get0_pqg(dh, &p, NULL, &g);
#else
	if (key->keydata.pkey == NULL) {
		return (DST_R_NULLKEY);
	}

	pkey = key->keydata.pkey;
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_FFC_P, &p);
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_FFC_G, &g);
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, &pub_key);
	EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PRIV_KEY, &priv_key);
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	priv.elements[i].tag = TAG_DH_PRIME;
	priv.elements[i].length = BN_num_bytes(p);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(p, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_GENERATOR;
	priv.elements[i].length = BN_num_bytes(g);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(g, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_PRIVATE;
	priv.elements[i].length = BN_num_bytes(priv_key);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(priv_key, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_PUBLIC;
	priv.elements[i].length = BN_num_bytes(pub_key);
	bufs[i] = isc_mem_get(key->mctx, priv.elements[i].length);
	BN_bn2bin(pub_key, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.nelements = i;
	result = dst__privstruct_writefile(key, &priv, directory);

	while (i--) {
		if (bufs[i] != NULL) {
			isc_mem_put(key->mctx, bufs[i],
				    priv.elements[i].length);
		}
	}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000
	if (p != NULL) {
		BN_free(p);
	}
	if (g != NULL) {
		BN_free(g);
	}
	if (pub_key != NULL) {
		BN_free(pub_key);
	}
	if (priv_key != NULL) {
		BN_clear_free(priv_key);
	}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000000L && OPENSSL_API_LEVEL >= 30000 \
	*/

	return (result);
}

static isc_result_t
openssldh_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	DH *dh = NULL;
#else
	OSSL_PARAM_BLD *bld = NULL;
	OSSL_PARAM *params = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	BIGNUM *pub_key = NULL, *priv_key = NULL, *p = NULL, *g = NULL;
	int key_size = 0;
	isc_mem_t *mctx;

	UNUSED(pub);
	mctx = key->mctx;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_DH, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS) {
		return (ret);
	}

	if (key->external) {
		DST_RET(DST_R_EXTERNALKEY);
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	dh = DH_new();
	if (dh == NULL) {
		DST_RET(ISC_R_NOMEMORY);
	}
	DH_clear_flags(dh, DH_FLAG_CACHE_MONT_P);
#else
	bld = OSSL_PARAM_BLD_new();
	if (bld == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	ctx = EVP_PKEY_CTX_new_from_name(NULL, "DH", NULL);
	if (ctx == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	for (i = 0; i < priv.nelements; i++) {
		BIGNUM *bn;
		bn = BN_bin2bn(priv.elements[i].data, priv.elements[i].length,
			       NULL);
		if (bn == NULL) {
			DST_RET(ISC_R_NOMEMORY);
		}

		switch (priv.elements[i].tag) {
		case TAG_DH_PRIME:
			p = bn;
			key_size = BN_num_bits(p);
			break;
		case TAG_DH_GENERATOR:
			g = bn;
			break;
		case TAG_DH_PRIVATE:
			priv_key = bn;
			break;
		case TAG_DH_PUBLIC:
			pub_key = bn;
			break;
		}
	}

#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (DH_set0_key(dh, pub_key, priv_key) != 1) {
		DST_RET(dst__openssl_toresult2("DH_set0_key",
					       DST_R_OPENSSLFAILURE));
	}
	if (DH_set0_pqg(dh, p, NULL, g) != 1) {
		DST_RET(dst__openssl_toresult2("DH_set0_pqg",
					       DST_R_OPENSSLFAILURE));
	}

	/* These are now managed by OpenSSL */
	pub_key = NULL;
	priv_key = NULL;
	p = NULL;
	g = NULL;

	key->keydata.dh = dh;
	dh = NULL;
#else
	if (OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PUB_KEY, pub_key) !=
		    1 ||
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PRIV_KEY, priv_key) !=
		    1 ||
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_P, p) != 1 ||
	    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_FFC_G, g) != 1)
	{
		DST_RET(dst__openssl_toresult2("OSSL_PARAM_BLD_push_BN",
					       DST_R_OPENSSLFAILURE));
	}
	params = OSSL_PARAM_BLD_to_param(bld);
	if (params == NULL) {
		DST_RET(dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_fromdata_init(ctx) != 1) {
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata_init",
					       DST_R_OPENSSLFAILURE));
	}
	if (EVP_PKEY_fromdata(ctx, &pkey, OSSL_KEYMGMT_SELECT_ALL, params) !=
		    1 ||
	    pkey == NULL)
	{
		DST_RET(dst__openssl_toresult2("EVP_PKEY_fromdata",
					       DST_R_OPENSSLFAILURE));
	}

	key->keydata.pkey = pkey;
	pkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */

	key->key_size = (unsigned int)key_size;
	ret = ISC_R_SUCCESS;

err:
#if OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000
	if (dh != NULL) {
		DH_free(dh);
	}
#else
	if (pkey != NULL) {
		EVP_PKEY_free(pkey);
	}
	if (ctx != NULL) {
		EVP_PKEY_CTX_free(ctx);
	}
	if (params != NULL) {
		OSSL_PARAM_free(params);
	}
	if (bld != NULL) {
		OSSL_PARAM_BLD_free(bld);
	}
#endif /* OPENSSL_VERSION_NUMBER < 0x30000000L || OPENSSL_API_LEVEL < 30000 */
	if (p != NULL) {
		BN_free(p);
	}
	if (g != NULL) {
		BN_free(g);
	}
	if (pub_key != NULL) {
		BN_free(pub_key);
	}
	if (priv_key != NULL) {
		BN_clear_free(priv_key);
	}
	if (ret != ISC_R_SUCCESS) {
		openssldh_destroy(key);
	}
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));

	return (ret);
}

static void
openssldh_cleanup(void) {
	BN_free(bn2);
	bn2 = NULL;

	BN_free(bn768);
	bn768 = NULL;

	BN_free(bn1024);
	bn1024 = NULL;

	BN_free(bn1536);
	bn1536 = NULL;
}

static dst_func_t openssldh_functions = {
	NULL, /*%< createctx */
	NULL, /*%< createctx2 */
	NULL, /*%< destroyctx */
	NULL, /*%< adddata */
	NULL, /*%< openssldh_sign */
	NULL, /*%< openssldh_verify */
	NULL, /*%< openssldh_verify2 */
	openssldh_computesecret,
	openssldh_compare,
	openssldh_paramcompare,
	openssldh_generate,
	openssldh_isprivate,
	openssldh_destroy,
	openssldh_todns,
	openssldh_fromdns,
	openssldh_tofile,
	openssldh_parse,
	openssldh_cleanup,
	NULL, /*%< fromlabel */
	NULL, /*%< dump */
	NULL, /*%< restore */
};

isc_result_t
dst__openssldh_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL) {
		if (BN_hex2bn(&bn2, PRIME2) == 0 || bn2 == NULL) {
			goto cleanup;
		}
		if (BN_hex2bn(&bn768, PRIME768) == 0 || bn768 == NULL) {
			goto cleanup;
		}
		if (BN_hex2bn(&bn1024, PRIME1024) == 0 || bn1024 == NULL) {
			goto cleanup;
		}
		if (BN_hex2bn(&bn1536, PRIME1536) == 0 || bn1536 == NULL) {
			goto cleanup;
		}
		*funcp = &openssldh_functions;
	}
	return (ISC_R_SUCCESS);

cleanup:
	if (bn2 != NULL) {
		BN_free(bn2);
	}
	if (bn768 != NULL) {
		BN_free(bn768);
	}
	if (bn1024 != NULL) {
		BN_free(bn1024);
	}
	if (bn1536 != NULL) {
		BN_free(bn1536);
	}
	return (ISC_R_NOMEMORY);
}
