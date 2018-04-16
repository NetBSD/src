/*	$NetBSD: hmacsha.c,v 1.11.4.1 2018/04/16 01:57:58 pgoyette Exp $	*/

/*
 * Copyright (C) 2005-2007, 2009, 2011-2018  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

/*
 * This code implements the HMAC-SHA1, HMAC-SHA224, HMAC-SHA256, HMAC-SHA384
 * and HMAC-SHA512 keyed hash algorithm described in RFC 2104 and
 * draft-ietf-dnsext-tsig-sha-01.txt.
 */

#include "config.h"

#include <isc/assertions.h>
#include <isc/hmacsha.h>
#include <isc/platform.h>
#include <isc/safe.h>
#include <isc/sha1.h>
#include <isc/sha2.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#if PKCS11CRYPTO
#include <pk11/internal.h>
#include <pk11/pk11.h>
#endif

#ifdef ISC_PLATFORM_OPENSSLHASH
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define HMAC_CTX_new() &(ctx->_ctx), HMAC_CTX_init(&(ctx->_ctx))
#define HMAC_CTX_free(ptr) HMAC_CTX_cleanup(ptr)
#endif

void
isc_hmacsha1_init(isc_hmacsha1_t *ctx, const unsigned char *key,
		  unsigned int len)
{
	ctx->ctx = HMAC_CTX_new();
	RUNTIME_CHECK(ctx->ctx != NULL);
	RUNTIME_CHECK(HMAC_Init_ex(ctx->ctx, (const void *) key,
				   (int) len, EVP_sha1(), NULL) == 1);
}

void
isc_hmacsha1_invalidate(isc_hmacsha1_t *ctx) {
	if (ctx->ctx == NULL)
		return;
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
}

void
isc_hmacsha1_update(isc_hmacsha1_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	RUNTIME_CHECK(HMAC_Update(ctx->ctx, buf, (int) len) == 1);
}

void
isc_hmacsha1_sign(isc_hmacsha1_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA1_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA1_DIGESTLENGTH);

	RUNTIME_CHECK(HMAC_Final(ctx->ctx, newdigest, NULL) == 1);
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

void
isc_hmacsha224_init(isc_hmacsha224_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	ctx->ctx = HMAC_CTX_new();
	RUNTIME_CHECK(ctx->ctx != NULL);
	RUNTIME_CHECK(HMAC_Init_ex(ctx->ctx, (const void *) key,
				   (int) len, EVP_sha224(), NULL) == 1);
}

void
isc_hmacsha224_invalidate(isc_hmacsha224_t *ctx) {
	if (ctx->ctx == NULL)
		return;
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
}

void
isc_hmacsha224_update(isc_hmacsha224_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	RUNTIME_CHECK(HMAC_Update(ctx->ctx, buf, (int) len) == 1);
}

void
isc_hmacsha224_sign(isc_hmacsha224_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA224_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA224_DIGESTLENGTH);

	RUNTIME_CHECK(HMAC_Final(ctx->ctx, newdigest, NULL) == 1);
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

void
isc_hmacsha256_init(isc_hmacsha256_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	ctx->ctx = HMAC_CTX_new();
	RUNTIME_CHECK(ctx->ctx != NULL);
	RUNTIME_CHECK(HMAC_Init_ex(ctx->ctx, (const void *) key,
				   (int) len, EVP_sha256(), NULL) == 1);
}

void
isc_hmacsha256_invalidate(isc_hmacsha256_t *ctx) {
	if (ctx->ctx == NULL)
		return;
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
}

void
isc_hmacsha256_update(isc_hmacsha256_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	RUNTIME_CHECK(HMAC_Update(ctx->ctx, buf, (int) len) == 1);
}

void
isc_hmacsha256_sign(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA256_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA256_DIGESTLENGTH);

	RUNTIME_CHECK(HMAC_Final(ctx->ctx, newdigest, NULL) == 1);
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

void
isc_hmacsha384_init(isc_hmacsha384_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	ctx->ctx = HMAC_CTX_new();
	RUNTIME_CHECK(ctx->ctx != NULL);
	RUNTIME_CHECK(HMAC_Init_ex(ctx->ctx, (const void *) key,
				   (int) len, EVP_sha384(), NULL) == 1);
}

void
isc_hmacsha384_invalidate(isc_hmacsha384_t *ctx) {
	if (ctx->ctx == NULL)
		return;
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
}

void
isc_hmacsha384_update(isc_hmacsha384_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	RUNTIME_CHECK(HMAC_Update(ctx->ctx, buf, (int) len) == 1);
}

void
isc_hmacsha384_sign(isc_hmacsha384_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA384_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA384_DIGESTLENGTH);

	RUNTIME_CHECK(HMAC_Final(ctx->ctx, newdigest, NULL) == 1);
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

void
isc_hmacsha512_init(isc_hmacsha512_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	ctx->ctx = HMAC_CTX_new();
	RUNTIME_CHECK(ctx->ctx != NULL);
	RUNTIME_CHECK(HMAC_Init_ex(ctx->ctx, (const void *) key,
				   (int) len, EVP_sha512(), NULL) == 1);
}

void
isc_hmacsha512_invalidate(isc_hmacsha512_t *ctx) {
	if (ctx->ctx == NULL)
		return;
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
}

void
isc_hmacsha512_update(isc_hmacsha512_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	RUNTIME_CHECK(HMAC_Update(ctx->ctx, buf, (int) len) == 1);
}

void
isc_hmacsha512_sign(isc_hmacsha512_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA512_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA512_DIGESTLENGTH);

	RUNTIME_CHECK(HMAC_Final(ctx->ctx, newdigest, NULL) == 1);
	HMAC_CTX_free(ctx->ctx);
	ctx->ctx = NULL;
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

#elif PKCS11CRYPTO

#if defined(PK11_SHA_1_HMAC_REPLACE) || \
    defined(PK11_SHA224_HMAC_REPLACE) || \
    defined(PK11_SHA256_HMAC_REPLACE) || \
    defined(PK11_SHA384_HMAC_REPLACE) || \
    defined(PK11_SHA512_HMAC_REPLACE)
#define IPAD 0x36
#define OPAD 0x5C
#endif

#if !defined(PK11_SHA_1_HMAC_REPLACE) && \
    !defined(PK11_SHA224_HMAC_REPLACE) && \
    !defined(PK11_SHA256_HMAC_REPLACE) && \
    !defined(PK11_SHA384_HMAC_REPLACE) && \
    !defined(PK11_SHA512_HMAC_REPLACE)
static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;
#endif

#ifndef PK11_SHA_1_HMAC_REPLACE
void
isc_hmacsha1_init(isc_hmacsha1_t *ctx, const unsigned char *key,
		 unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA_1_HMAC, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_SHA_1_HMAC;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, (CK_ULONG) len }
	};
#ifdef PK11_PAD_HMAC_KEYS
	CK_BYTE keypad[ISC_SHA1_DIGESTLENGTH];

	if (len < ISC_SHA1_DIGESTLENGTH) {
		memset(keypad, 0, ISC_SHA1_DIGESTLENGTH);
		memmove(keypad, key, len);
		keyTemplate[5].pValue = keypad;
		keyTemplate[5].ulValueLen = ISC_SHA1_DIGESTLENGTH;
	} else
		DE_CONST(key, keyTemplate[5].pValue);
#else
	DE_CONST(key, keyTemplate[5].pValue);
#endif
	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx->object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx->session, keyTemplate,
			 (CK_ULONG) 6, &ctx->object));
	INSIST(ctx->object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_SignInit, (ctx->session, &mech, ctx->object));
}

void
isc_hmacsha1_invalidate(isc_hmacsha1_t *ctx) {
	CK_BYTE garbage[ISC_SHA1_DIGESTLENGTH];
	CK_ULONG len = ISC_SHA1_DIGESTLENGTH;

	if (ctx->handle == NULL)
		return;
	(void) pkcs_C_SignFinal(ctx->session, garbage, &len);
	isc_safe_memwipe(garbage, sizeof(garbage));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
}

void
isc_hmacsha1_update(isc_hmacsha1_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_SignUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha1_sign(isc_hmacsha1_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA1_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA1_DIGESTLENGTH;

	REQUIRE(len <= ISC_SHA1_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_SignFinal, (ctx->session, newdigest, &psl));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#else
void
isc_hmacsha1_init(isc_hmacsha1_t *ctx, const unsigned char *key,
		  unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA_1, NULL, 0 };
	unsigned char ipad[ISC_SHA1_BLOCK_LENGTH];
	unsigned int i;

	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK((ctx->key = pk11_mem_get(ISC_SHA1_BLOCK_LENGTH))
		      != NULL);
	if (len > ISC_SHA1_BLOCK_LENGTH) {
		CK_BYTE_PTR kPart;
		CK_ULONG kl;

		PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
		DE_CONST(key, kPart);
		PK11_FATALCHECK(pkcs_C_DigestUpdate,
				(ctx->session, kPart, (CK_ULONG) len));
		kl = ISC_SHA1_DIGESTLENGTH;
		PK11_FATALCHECK(pkcs_C_DigestFinal,
				(ctx->session, (CK_BYTE_PTR) ctx->key, &kl));
	} else
		memmove(ctx->key, key, len);
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	memset(ipad, IPAD, ISC_SHA1_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA1_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, ipad,
			 (CK_ULONG) ISC_SHA1_BLOCK_LENGTH));
}

void
isc_hmacsha1_invalidate(isc_hmacsha1_t *ctx) {
	if (ctx->key != NULL)
		pk11_mem_put(ctx->key, ISC_SHA1_BLOCK_LENGTH);
	ctx->key = NULL;
	isc_sha1_invalidate(ctx);
}

void
isc_hmacsha1_update(isc_hmacsha1_t *ctx, const unsigned char *buf,
		    unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha1_sign(isc_hmacsha1_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA1_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA1_DIGESTLENGTH;
	CK_MECHANISM mech = { CKM_SHA_1, NULL, 0 };
	CK_BYTE opad[ISC_SHA1_BLOCK_LENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA1_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	memset(opad, OPAD, ISC_SHA1_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA1_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];
	pk11_mem_put(ctx->key, ISC_SHA1_BLOCK_LENGTH);
	ctx->key = NULL;
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, opad,
			 (CK_ULONG) ISC_SHA1_BLOCK_LENGTH));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, (CK_BYTE_PTR) newdigest, psl));
	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif

#ifndef PK11_SHA224_HMAC_REPLACE
void
isc_hmacsha224_init(isc_hmacsha224_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA224_HMAC, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_SHA224_HMAC;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, (CK_ULONG) len }
	};
#ifdef PK11_PAD_HMAC_KEYS
	CK_BYTE keypad[ISC_SHA224_DIGESTLENGTH];

	if (len < ISC_SHA224_DIGESTLENGTH) {
		memset(keypad, 0, ISC_SHA224_DIGESTLENGTH);
		memmove(keypad, key, len);
		keyTemplate[5].pValue = keypad;
		keyTemplate[5].ulValueLen = ISC_SHA224_DIGESTLENGTH;
	} else
		DE_CONST(key, keyTemplate[5].pValue);
#else
	DE_CONST(key, keyTemplate[5].pValue);
#endif
	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx->object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx->session, keyTemplate,
			 (CK_ULONG) 6, &ctx->object));
	INSIST(ctx->object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_SignInit, (ctx->session, &mech, ctx->object));
}

void
isc_hmacsha224_invalidate(isc_hmacsha224_t *ctx) {
	CK_BYTE garbage[ISC_SHA224_DIGESTLENGTH];
	CK_ULONG len = ISC_SHA224_DIGESTLENGTH;

	if (ctx->handle == NULL)
		return;
	(void) pkcs_C_SignFinal(ctx->session, garbage, &len);
	isc_safe_memwipe(garbage, sizeof(garbage));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
}

void
isc_hmacsha224_update(isc_hmacsha224_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_SignUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha224_sign(isc_hmacsha224_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA224_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA224_DIGESTLENGTH;

	REQUIRE(len <= ISC_SHA224_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_SignFinal, (ctx->session, newdigest, &psl));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#else
void
isc_hmacsha224_init(isc_hmacsha224_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA224, NULL, 0 };
	unsigned char ipad[ISC_SHA224_BLOCK_LENGTH];
	unsigned int i;

	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK((ctx->key = pk11_mem_get(ISC_SHA224_BLOCK_LENGTH))
		      != NULL);
	if (len > ISC_SHA224_BLOCK_LENGTH) {
		CK_BYTE_PTR kPart;
		CK_ULONG kl;

		PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
		DE_CONST(key, kPart);
		PK11_FATALCHECK(pkcs_C_DigestUpdate,
				(ctx->session, kPart, (CK_ULONG) len));
		kl = ISC_SHA224_DIGESTLENGTH;
		PK11_FATALCHECK(pkcs_C_DigestFinal,
				(ctx->session, (CK_BYTE_PTR) ctx->key, &kl));
	} else
		memmove(ctx->key, key, len);
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	memset(ipad, IPAD, ISC_SHA224_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA224_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, ipad,
			 (CK_ULONG) ISC_SHA224_BLOCK_LENGTH));
}

void
isc_hmacsha224_invalidate(isc_hmacsha224_t *ctx) {
	if (ctx->key != NULL)
		pk11_mem_put(ctx->key, ISC_SHA224_BLOCK_LENGTH);
	ctx->key = NULL;
	isc_sha224_invalidate(ctx);
}

void
isc_hmacsha224_update(isc_hmacsha224_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha224_sign(isc_hmacsha224_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA224_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA224_DIGESTLENGTH;
	CK_MECHANISM mech = { CKM_SHA224, NULL, 0 };
	CK_BYTE opad[ISC_SHA224_BLOCK_LENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA224_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	memset(opad, OPAD, ISC_SHA224_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA224_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];
	pk11_mem_put(ctx->key, ISC_SHA224_BLOCK_LENGTH);
	ctx->key = NULL;
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, opad,
			 (CK_ULONG) ISC_SHA224_BLOCK_LENGTH));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, (CK_BYTE_PTR) newdigest, psl));
	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif

#ifndef PK11_SHA256_HMAC_REPLACE
void
isc_hmacsha256_init(isc_hmacsha256_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA256_HMAC, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_SHA256_HMAC;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, (CK_ULONG) len }
	};
#ifdef PK11_PAD_HMAC_KEYS
	CK_BYTE keypad[ISC_SHA256_DIGESTLENGTH];

	if (len < ISC_SHA256_DIGESTLENGTH) {
		memset(keypad, 0, ISC_SHA256_DIGESTLENGTH);
		memmove(keypad, key, len);
		keyTemplate[5].pValue = keypad;
		keyTemplate[5].ulValueLen = ISC_SHA256_DIGESTLENGTH;
	} else
		DE_CONST(key, keyTemplate[5].pValue);
#else
	DE_CONST(key, keyTemplate[5].pValue);
#endif
	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx->object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx->session, keyTemplate,
			 (CK_ULONG) 6, &ctx->object));
	INSIST(ctx->object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_SignInit, (ctx->session, &mech, ctx->object));
}

void
isc_hmacsha256_invalidate(isc_hmacsha256_t *ctx) {
	CK_BYTE garbage[ISC_SHA256_DIGESTLENGTH];
	CK_ULONG len = ISC_SHA256_DIGESTLENGTH;

	if (ctx->handle == NULL)
		return;
	(void) pkcs_C_SignFinal(ctx->session, garbage, &len);
	isc_safe_memwipe(garbage, sizeof(garbage));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
}

void
isc_hmacsha256_update(isc_hmacsha256_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_SignUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha256_sign(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA256_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA256_DIGESTLENGTH;

	REQUIRE(len <= ISC_SHA256_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_SignFinal, (ctx->session, newdigest, &psl));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#else
void
isc_hmacsha256_init(isc_hmacsha256_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA256, NULL, 0 };
	unsigned char ipad[ISC_SHA256_BLOCK_LENGTH];
	unsigned int i;

	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK((ctx->key = pk11_mem_get(ISC_SHA256_BLOCK_LENGTH))
		      != NULL);
	if (len > ISC_SHA256_BLOCK_LENGTH) {
		CK_BYTE_PTR kPart;
		CK_ULONG kl;

		PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
		DE_CONST(key, kPart);
		PK11_FATALCHECK(pkcs_C_DigestUpdate,
				(ctx->session, kPart, (CK_ULONG) len));
		kl = ISC_SHA256_DIGESTLENGTH;
		PK11_FATALCHECK(pkcs_C_DigestFinal,
				(ctx->session, (CK_BYTE_PTR) ctx->key, &kl));
	} else
		memmove(ctx->key, key, len);
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	memset(ipad, IPAD, ISC_SHA256_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA256_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, ipad,
			 (CK_ULONG) ISC_SHA256_BLOCK_LENGTH));
}

void
isc_hmacsha256_invalidate(isc_hmacsha256_t *ctx) {
	if (ctx->key != NULL)
		pk11_mem_put(ctx->key, ISC_SHA256_BLOCK_LENGTH);
	ctx->key = NULL;
	isc_sha256_invalidate(ctx);
}

void
isc_hmacsha256_update(isc_hmacsha256_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha256_sign(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA256_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA256_DIGESTLENGTH;
	CK_MECHANISM mech = { CKM_SHA256, NULL, 0 };
	CK_BYTE opad[ISC_SHA256_BLOCK_LENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA256_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	memset(opad, OPAD, ISC_SHA256_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA256_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];
	pk11_mem_put(ctx->key, ISC_SHA256_BLOCK_LENGTH);
	ctx->key = NULL;
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, opad,
			 (CK_ULONG) ISC_SHA256_BLOCK_LENGTH));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, (CK_BYTE_PTR) newdigest, psl));
	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif

#ifndef PK11_SHA384_HMAC_REPLACE
void
isc_hmacsha384_init(isc_hmacsha384_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA384_HMAC, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_SHA384_HMAC;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, (CK_ULONG) len }
	};
#ifdef PK11_PAD_HMAC_KEYS
	CK_BYTE keypad[ISC_SHA384_DIGESTLENGTH];

	if (len < ISC_SHA384_DIGESTLENGTH) {
		memset(keypad, 0, ISC_SHA384_DIGESTLENGTH);
		memmove(keypad, key, len);
		keyTemplate[5].pValue = keypad;
		keyTemplate[5].ulValueLen = ISC_SHA384_DIGESTLENGTH;
	} else
		DE_CONST(key, keyTemplate[5].pValue);
#else
	DE_CONST(key, keyTemplate[5].pValue);
#endif
	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx->object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx->session, keyTemplate,
			 (CK_ULONG) 6, &ctx->object));
	INSIST(ctx->object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_SignInit, (ctx->session, &mech, ctx->object));
}

void
isc_hmacsha384_invalidate(isc_hmacsha384_t *ctx) {
	CK_BYTE garbage[ISC_SHA384_DIGESTLENGTH];
	CK_ULONG len = ISC_SHA384_DIGESTLENGTH;

	if (ctx->handle == NULL)
		return;
	(void) pkcs_C_SignFinal(ctx->session, garbage, &len);
	isc_safe_memwipe(garbage, sizeof(garbage));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
}

void
isc_hmacsha384_update(isc_hmacsha384_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_SignUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha384_sign(isc_hmacsha384_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA384_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA384_DIGESTLENGTH;

	REQUIRE(len <= ISC_SHA384_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_SignFinal, (ctx->session, newdigest, &psl));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#else
void
isc_hmacsha384_init(isc_hmacsha384_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA384, NULL, 0 };
	unsigned char ipad[ISC_SHA384_BLOCK_LENGTH];
	unsigned int i;

	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK((ctx->key = pk11_mem_get(ISC_SHA384_BLOCK_LENGTH))
		      != NULL);
	if (len > ISC_SHA384_BLOCK_LENGTH) {
		CK_BYTE_PTR kPart;
		CK_ULONG kl;

		PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
		DE_CONST(key, kPart);
		PK11_FATALCHECK(pkcs_C_DigestUpdate,
				(ctx->session, kPart, (CK_ULONG) len));
		kl = ISC_SHA384_DIGESTLENGTH;
		PK11_FATALCHECK(pkcs_C_DigestFinal,
				(ctx->session, (CK_BYTE_PTR) ctx->key, &kl));
	} else
		memmove(ctx->key, key, len);
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	memset(ipad, IPAD, ISC_SHA384_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA384_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, ipad,
			 (CK_ULONG) ISC_SHA384_BLOCK_LENGTH));
}

void
isc_hmacsha384_invalidate(isc_hmacsha384_t *ctx) {
	if (ctx->key != NULL)
		pk11_mem_put(ctx->key, ISC_SHA384_BLOCK_LENGTH);
	ctx->key = NULL;
	isc_sha384_invalidate(ctx);
}

void
isc_hmacsha384_update(isc_hmacsha384_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha384_sign(isc_hmacsha384_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA384_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA384_DIGESTLENGTH;
	CK_MECHANISM mech = { CKM_SHA384, NULL, 0 };
	CK_BYTE opad[ISC_SHA384_BLOCK_LENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA384_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	memset(opad, OPAD, ISC_SHA384_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA384_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];
	pk11_mem_put(ctx->key, ISC_SHA384_BLOCK_LENGTH);
	ctx->key = NULL;
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, opad,
			 (CK_ULONG) ISC_SHA384_BLOCK_LENGTH));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, (CK_BYTE_PTR) newdigest, psl));
	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif

#ifndef PK11_SHA512_HMAC_REPLACE
void
isc_hmacsha512_init(isc_hmacsha512_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA512_HMAC, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_SHA512_HMAC;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, (CK_ULONG) len }
	};
#ifdef PK11_PAD_HMAC_KEYS
	CK_BYTE keypad[ISC_SHA512_DIGESTLENGTH];

	if (len < ISC_SHA512_DIGESTLENGTH) {
		memset(keypad, 0, ISC_SHA512_DIGESTLENGTH);
		memmove(keypad, key, len);
		keyTemplate[5].pValue = keypad;
		keyTemplate[5].ulValueLen = ISC_SHA512_DIGESTLENGTH;
	} else
		DE_CONST(key, keyTemplate[5].pValue);
#else
	DE_CONST(key, keyTemplate[5].pValue);
#endif
	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx->object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx->session, keyTemplate,
			 (CK_ULONG) 6, &ctx->object));
	INSIST(ctx->object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_SignInit, (ctx->session, &mech, ctx->object));
}

void
isc_hmacsha512_invalidate(isc_hmacsha512_t *ctx) {
	CK_BYTE garbage[ISC_SHA512_DIGESTLENGTH];
	CK_ULONG len = ISC_SHA512_DIGESTLENGTH;

	if (ctx->handle == NULL)
		return;
	(void) pkcs_C_SignFinal(ctx->session, garbage, &len);
	isc_safe_memwipe(garbage, sizeof(garbage));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
}

void
isc_hmacsha512_update(isc_hmacsha512_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_SignUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha512_sign(isc_hmacsha512_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA512_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA512_DIGESTLENGTH;

	REQUIRE(len <= ISC_SHA512_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_SignFinal, (ctx->session, newdigest, &psl));
	if (ctx->object != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(ctx->session, ctx->object);
	ctx->object = CK_INVALID_HANDLE;
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#else
void
isc_hmacsha512_init(isc_hmacsha512_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_SHA512, NULL, 0 };
	unsigned char ipad[ISC_SHA512_BLOCK_LENGTH];
	unsigned int i;

	RUNTIME_CHECK(pk11_get_session(ctx, OP_DIGEST, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	RUNTIME_CHECK((ctx->key = pk11_mem_get(ISC_SHA512_BLOCK_LENGTH))
		      != NULL);
	if (len > ISC_SHA512_BLOCK_LENGTH) {
		CK_BYTE_PTR kPart;
		CK_ULONG kl;

		PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
		DE_CONST(key, kPart);
		PK11_FATALCHECK(pkcs_C_DigestUpdate,
				(ctx->session, kPart, (CK_ULONG) len));
		kl = ISC_SHA512_DIGESTLENGTH;
		PK11_FATALCHECK(pkcs_C_DigestFinal,
				(ctx->session, (CK_BYTE_PTR) ctx->key, &kl));
	} else
		memmove(ctx->key, key, len);
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	memset(ipad, IPAD, ISC_SHA512_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA512_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, ipad,
			 (CK_ULONG) ISC_SHA512_BLOCK_LENGTH));
}

void
isc_hmacsha512_invalidate(isc_hmacsha512_t *ctx) {
	if (ctx->key != NULL)
		pk11_mem_put(ctx->key, ISC_SHA512_BLOCK_LENGTH);
	ctx->key = NULL;
	isc_sha512_invalidate(ctx);
}

void
isc_hmacsha512_update(isc_hmacsha512_t *ctx, const unsigned char *buf,
		      unsigned int len)
{
	CK_RV rv;
	CK_BYTE_PTR pPart;

	DE_CONST(buf, pPart);
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, pPart, (CK_ULONG) len));
}

void
isc_hmacsha512_sign(isc_hmacsha512_t *ctx, unsigned char *digest, size_t len) {
	CK_RV rv;
	CK_BYTE newdigest[ISC_SHA512_DIGESTLENGTH];
	CK_ULONG psl = ISC_SHA512_DIGESTLENGTH;
	CK_MECHANISM mech = { CKM_SHA512, NULL, 0 };
	CK_BYTE opad[ISC_SHA512_BLOCK_LENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA512_DIGESTLENGTH);

	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	memset(opad, OPAD, ISC_SHA512_BLOCK_LENGTH);
	for (i = 0; i < ISC_SHA512_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];
	pk11_mem_put(ctx->key, ISC_SHA512_BLOCK_LENGTH);
	ctx->key = NULL;
	PK11_FATALCHECK(pkcs_C_DigestInit, (ctx->session, &mech));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, opad,
			 (CK_ULONG) ISC_SHA512_BLOCK_LENGTH));
	PK11_FATALCHECK(pkcs_C_DigestUpdate,
			(ctx->session, (CK_BYTE_PTR) newdigest, psl));
	PK11_FATALCHECK(pkcs_C_DigestFinal, (ctx->session, newdigest, &psl));
	pk11_return_session(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif

#else

#define IPAD 0x36
#define OPAD 0x5C

/*
 * Start HMAC-SHA1 process.  Initialize an sha1 context and digest the key.
 */
void
isc_hmacsha1_init(isc_hmacsha1_t *ctx, const unsigned char *key,
		  unsigned int len)
{
	unsigned char ipad[ISC_SHA1_BLOCK_LENGTH];
	unsigned int i;

	memset(ctx->key, 0, sizeof(ctx->key));
	if (len > sizeof(ctx->key)) {
		isc_sha1_t sha1ctx;
		isc_sha1_init(&sha1ctx);
		isc_sha1_update(&sha1ctx, key, len);
		isc_sha1_final(&sha1ctx, ctx->key);
	} else
		memmove(ctx->key, key, len);

	isc_sha1_init(&ctx->sha1ctx);
	memset(ipad, IPAD, sizeof(ipad));
	for (i = 0; i < ISC_SHA1_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	isc_sha1_update(&ctx->sha1ctx, ipad, sizeof(ipad));
}

void
isc_hmacsha1_invalidate(isc_hmacsha1_t *ctx) {
	isc_sha1_invalidate(&ctx->sha1ctx);
	isc_safe_memwipe(ctx, sizeof(*ctx));
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
isc_hmacsha1_update(isc_hmacsha1_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	isc_sha1_update(&ctx->sha1ctx, buf, len);
}

/*
 * Compute signature - finalize SHA1 operation and reapply SHA1.
 */
void
isc_hmacsha1_sign(isc_hmacsha1_t *ctx, unsigned char *digest, size_t len) {
	unsigned char opad[ISC_SHA1_BLOCK_LENGTH];
	unsigned char newdigest[ISC_SHA1_DIGESTLENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA1_DIGESTLENGTH);
	isc_sha1_final(&ctx->sha1ctx, newdigest);

	memset(opad, OPAD, sizeof(opad));
	for (i = 0; i < ISC_SHA1_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];

	isc_sha1_init(&ctx->sha1ctx);
	isc_sha1_update(&ctx->sha1ctx, opad, sizeof(opad));
	isc_sha1_update(&ctx->sha1ctx, newdigest, ISC_SHA1_DIGESTLENGTH);
	isc_sha1_final(&ctx->sha1ctx, newdigest);
	isc_hmacsha1_invalidate(ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

/*
 * Start HMAC-SHA224 process.  Initialize an sha224 context and digest the key.
 */
void
isc_hmacsha224_init(isc_hmacsha224_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	unsigned char ipad[ISC_SHA224_BLOCK_LENGTH];
	unsigned int i;

	memset(ctx->key, 0, sizeof(ctx->key));
	if (len > sizeof(ctx->key)) {
		isc_sha224_t sha224ctx;
		isc_sha224_init(&sha224ctx);
		isc_sha224_update(&sha224ctx, key, len);
		isc_sha224_final(ctx->key, &sha224ctx);
	} else
		memmove(ctx->key, key, len);

	isc_sha224_init(&ctx->sha224ctx);
	memset(ipad, IPAD, sizeof(ipad));
	for (i = 0; i < ISC_SHA224_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	isc_sha224_update(&ctx->sha224ctx, ipad, sizeof(ipad));
}

void
isc_hmacsha224_invalidate(isc_hmacsha224_t *ctx) {
	isc_safe_memwipe(ctx, sizeof(*ctx));
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
isc_hmacsha224_update(isc_hmacsha224_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	isc_sha224_update(&ctx->sha224ctx, buf, len);
}

/*
 * Compute signature - finalize SHA224 operation and reapply SHA224.
 */
void
isc_hmacsha224_sign(isc_hmacsha224_t *ctx, unsigned char *digest, size_t len) {
	unsigned char opad[ISC_SHA224_BLOCK_LENGTH];
	unsigned char newdigest[ISC_SHA224_DIGESTLENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA224_DIGESTLENGTH);
	isc_sha224_final(newdigest, &ctx->sha224ctx);

	memset(opad, OPAD, sizeof(opad));
	for (i = 0; i < ISC_SHA224_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];

	isc_sha224_init(&ctx->sha224ctx);
	isc_sha224_update(&ctx->sha224ctx, opad, sizeof(opad));
	isc_sha224_update(&ctx->sha224ctx, newdigest, ISC_SHA224_DIGESTLENGTH);
	isc_sha224_final(newdigest, &ctx->sha224ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

/*
 * Start HMAC-SHA256 process.  Initialize an sha256 context and digest the key.
 */
void
isc_hmacsha256_init(isc_hmacsha256_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	unsigned char ipad[ISC_SHA256_BLOCK_LENGTH];
	unsigned int i;

	memset(ctx->key, 0, sizeof(ctx->key));
	if (len > sizeof(ctx->key)) {
		isc_sha256_t sha256ctx;
		isc_sha256_init(&sha256ctx);
		isc_sha256_update(&sha256ctx, key, len);
		isc_sha256_final(ctx->key, &sha256ctx);
	} else
		memmove(ctx->key, key, len);

	isc_sha256_init(&ctx->sha256ctx);
	memset(ipad, IPAD, sizeof(ipad));
	for (i = 0; i < ISC_SHA256_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	isc_sha256_update(&ctx->sha256ctx, ipad, sizeof(ipad));
}

void
isc_hmacsha256_invalidate(isc_hmacsha256_t *ctx) {
	isc_safe_memwipe(ctx, sizeof(*ctx));
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
isc_hmacsha256_update(isc_hmacsha256_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	isc_sha256_update(&ctx->sha256ctx, buf, len);
}

/*
 * Compute signature - finalize SHA256 operation and reapply SHA256.
 */
void
isc_hmacsha256_sign(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len) {
	unsigned char opad[ISC_SHA256_BLOCK_LENGTH];
	unsigned char newdigest[ISC_SHA256_DIGESTLENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA256_DIGESTLENGTH);
	isc_sha256_final(newdigest, &ctx->sha256ctx);

	memset(opad, OPAD, sizeof(opad));
	for (i = 0; i < ISC_SHA256_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];

	isc_sha256_init(&ctx->sha256ctx);
	isc_sha256_update(&ctx->sha256ctx, opad, sizeof(opad));
	isc_sha256_update(&ctx->sha256ctx, newdigest, ISC_SHA256_DIGESTLENGTH);
	isc_sha256_final(newdigest, &ctx->sha256ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

/*
 * Start HMAC-SHA384 process.  Initialize an sha384 context and digest the key.
 */
void
isc_hmacsha384_init(isc_hmacsha384_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	unsigned char ipad[ISC_SHA384_BLOCK_LENGTH];
	unsigned int i;

	memset(ctx->key, 0, sizeof(ctx->key));
	if (len > sizeof(ctx->key)) {
		isc_sha384_t sha384ctx;
		isc_sha384_init(&sha384ctx);
		isc_sha384_update(&sha384ctx, key, len);
		isc_sha384_final(ctx->key, &sha384ctx);
	} else
		memmove(ctx->key, key, len);

	isc_sha384_init(&ctx->sha384ctx);
	memset(ipad, IPAD, sizeof(ipad));
	for (i = 0; i < ISC_SHA384_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	isc_sha384_update(&ctx->sha384ctx, ipad, sizeof(ipad));
}

void
isc_hmacsha384_invalidate(isc_hmacsha384_t *ctx) {
	isc_safe_memwipe(ctx, sizeof(*ctx));
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
isc_hmacsha384_update(isc_hmacsha384_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	isc_sha384_update(&ctx->sha384ctx, buf, len);
}

/*
 * Compute signature - finalize SHA384 operation and reapply SHA384.
 */
void
isc_hmacsha384_sign(isc_hmacsha384_t *ctx, unsigned char *digest, size_t len) {
	unsigned char opad[ISC_SHA384_BLOCK_LENGTH];
	unsigned char newdigest[ISC_SHA384_DIGESTLENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA384_DIGESTLENGTH);
	isc_sha384_final(newdigest, &ctx->sha384ctx);

	memset(opad, OPAD, sizeof(opad));
	for (i = 0; i < ISC_SHA384_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];

	isc_sha384_init(&ctx->sha384ctx);
	isc_sha384_update(&ctx->sha384ctx, opad, sizeof(opad));
	isc_sha384_update(&ctx->sha384ctx, newdigest, ISC_SHA384_DIGESTLENGTH);
	isc_sha384_final(newdigest, &ctx->sha384ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}

/*
 * Start HMAC-SHA512 process.  Initialize an sha512 context and digest the key.
 */
void
isc_hmacsha512_init(isc_hmacsha512_t *ctx, const unsigned char *key,
		    unsigned int len)
{
	unsigned char ipad[ISC_SHA512_BLOCK_LENGTH];
	unsigned int i;

	memset(ctx->key, 0, sizeof(ctx->key));
	if (len > sizeof(ctx->key)) {
		isc_sha512_t sha512ctx;
		isc_sha512_init(&sha512ctx);
		isc_sha512_update(&sha512ctx, key, len);
		isc_sha512_final(ctx->key, &sha512ctx);
	} else
		memmove(ctx->key, key, len);

	isc_sha512_init(&ctx->sha512ctx);
	memset(ipad, IPAD, sizeof(ipad));
	for (i = 0; i < ISC_SHA512_BLOCK_LENGTH; i++)
		ipad[i] ^= ctx->key[i];
	isc_sha512_update(&ctx->sha512ctx, ipad, sizeof(ipad));
}

void
isc_hmacsha512_invalidate(isc_hmacsha512_t *ctx) {
	isc_safe_memwipe(ctx, sizeof(*ctx));
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
isc_hmacsha512_update(isc_hmacsha512_t *ctx, const unsigned char *buf,
		   unsigned int len)
{
	isc_sha512_update(&ctx->sha512ctx, buf, len);
}

/*
 * Compute signature - finalize SHA512 operation and reapply SHA512.
 */
void
isc_hmacsha512_sign(isc_hmacsha512_t *ctx, unsigned char *digest, size_t len) {
	unsigned char opad[ISC_SHA512_BLOCK_LENGTH];
	unsigned char newdigest[ISC_SHA512_DIGESTLENGTH];
	unsigned int i;

	REQUIRE(len <= ISC_SHA512_DIGESTLENGTH);
	isc_sha512_final(newdigest, &ctx->sha512ctx);

	memset(opad, OPAD, sizeof(opad));
	for (i = 0; i < ISC_SHA512_BLOCK_LENGTH; i++)
		opad[i] ^= ctx->key[i];

	isc_sha512_init(&ctx->sha512ctx);
	isc_sha512_update(&ctx->sha512ctx, opad, sizeof(opad));
	isc_sha512_update(&ctx->sha512ctx, newdigest, ISC_SHA512_DIGESTLENGTH);
	isc_sha512_final(newdigest, &ctx->sha512ctx);
	memmove(digest, newdigest, len);
	isc_safe_memwipe(newdigest, sizeof(newdigest));
}
#endif /* !ISC_PLATFORM_OPENSSLHASH */

/*
 * Verify signature - finalize SHA1 operation and reapply SHA1, then
 * compare to the supplied digest.
 */
isc_boolean_t
isc_hmacsha1_verify(isc_hmacsha1_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA1_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA1_DIGESTLENGTH);
	isc_hmacsha1_sign(ctx, newdigest, ISC_SHA1_DIGESTLENGTH);
	return (isc_safe_memequal(digest, newdigest, len));
}

/*
 * Verify signature - finalize SHA224 operation and reapply SHA224, then
 * compare to the supplied digest.
 */
isc_boolean_t
isc_hmacsha224_verify(isc_hmacsha224_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA224_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA224_DIGESTLENGTH);
	isc_hmacsha224_sign(ctx, newdigest, ISC_SHA224_DIGESTLENGTH);
	return (isc_safe_memequal(digest, newdigest, len));
}

/*
 * Verify signature - finalize SHA256 operation and reapply SHA256, then
 * compare to the supplied digest.
 */
isc_boolean_t
isc_hmacsha256_verify(isc_hmacsha256_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA256_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA256_DIGESTLENGTH);
	isc_hmacsha256_sign(ctx, newdigest, ISC_SHA256_DIGESTLENGTH);
	return (isc_safe_memequal(digest, newdigest, len));
}

/*
 * Verify signature - finalize SHA384 operation and reapply SHA384, then
 * compare to the supplied digest.
 */
isc_boolean_t
isc_hmacsha384_verify(isc_hmacsha384_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA384_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA384_DIGESTLENGTH);
	isc_hmacsha384_sign(ctx, newdigest, ISC_SHA384_DIGESTLENGTH);
	return (isc_safe_memequal(digest, newdigest, len));
}

/*
 * Verify signature - finalize SHA512 operation and reapply SHA512, then
 * compare to the supplied digest.
 */
isc_boolean_t
isc_hmacsha512_verify(isc_hmacsha512_t *ctx, unsigned char *digest, size_t len) {
	unsigned char newdigest[ISC_SHA512_DIGESTLENGTH];

	REQUIRE(len <= ISC_SHA512_DIGESTLENGTH);
	isc_hmacsha512_sign(ctx, newdigest, ISC_SHA512_DIGESTLENGTH);
	return (isc_safe_memequal(digest, newdigest, len));
}

/*
 * Check for SHA-1 support; if it does not work, raise a fatal error.
 *
 * Use the first test vector from RFC 2104, with a second round using
 * a too-short key.
 *
 * Standard use is testing 0 and expecting result true.
 * Testing use is testing 1..4 and expecting result false.
 */
isc_boolean_t
isc_hmacsha1_check(int testing) {
	isc_hmacsha1_t ctx;
	unsigned char key[] = { /* 20*0x0b */
		0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
		0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
		0x0b, 0x0b, 0x0b, 0x0b
	};
	unsigned char input[] = { /* "Hi There" */
		0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65
	};
	unsigned char expected[] = {
		0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64,
		0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e,
		0xf1, 0x46, 0xbe, 0x00
	};
	unsigned char expected2[] = {
		0xa0, 0x75, 0xe0, 0x5f, 0x7f, 0x17, 0x9d, 0x34,
		0xb2, 0xab, 0xc5, 0x19, 0x8f, 0x38, 0x62, 0x36,
		0x42, 0xbd, 0xec, 0xde
	};
	isc_boolean_t result;

	/*
	 * Introduce a fault for testing.
	 */
	switch (testing) {
	case 0:
	default:
		break;
	case 1:
		key[0] ^= 0x01;
		break;
	case 2:
		input[0] ^= 0x01;
		break;
	case 3:
		expected[0] ^= 0x01;
		break;
	case 4:
		expected2[0] ^= 0x01;
		break;
	}

	/*
	 * These functions do not return anything; any failure will be fatal.
	 */
	isc_hmacsha1_init(&ctx, key, 20U);
	isc_hmacsha1_update(&ctx, input, 8U);
	result = isc_hmacsha1_verify(&ctx, expected, sizeof(expected));
	if (!result) {
		return (result);
	}

	/* Second round using a byte key */
	isc_hmacsha1_init(&ctx, key, 1U);
	isc_hmacsha1_update(&ctx, input, 8U);
	return (isc_hmacsha1_verify(&ctx, expected2, sizeof(expected2)));
}
