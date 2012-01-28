
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

/*
 * hash.c - openssl TSS crypto routines
 *
 * Kent Yoder <shpedoikal@gmail.com>
 *
 */

#include <string.h>

#include <openssl/opensslv.h>
#include <openssl/rsa.h>  // for some reason the MGF1 prototype is in here
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "tsplog.h"

#ifdef TSS_DEBUG
#define DEBUG_print_openssl_errors() \
	do { \
		ERR_load_crypto_strings(); \
		ERR_print_errors_fp(stderr); \
	} while (0)
#else
#define DEBUG_print_openssl_errors()
#endif

#if (OPENSSL_VERSION_NUMBER >= 0x0090800FL)
#define OpenSSL_MGF1(m,mlen,s,slen,md)	PKCS1_MGF1(m,mlen,s,slen,md)
#else
int MGF1(unsigned char *, long, const unsigned char *, long);
#define OpenSSL_MGF1(m,mlen,s,slen,md)	MGF1(m,mlen,s,slen)
#endif

/*
 * Hopefully this will make the code clearer since
 * OpenSSL returns 1 on success
 */
#define EVP_SUCCESS 1

TSS_RESULT
Trspi_Hash(UINT32 HashType, UINT32 BufSize, BYTE* Buf, BYTE* Digest)
{
	EVP_MD_CTX md_ctx;
	unsigned int result_size;
	int rv;

	switch (HashType) {
		case TSS_HASH_SHA1:
			rv = EVP_DigestInit(&md_ctx, EVP_sha1());
			break;
		default:
			rv = TSPERR(TSS_E_BAD_PARAMETER);
			goto out;
			break;
	}

	if (rv != EVP_SUCCESS) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	rv = EVP_DigestUpdate(&md_ctx, Buf, BufSize);
	if (rv != EVP_SUCCESS) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	result_size = EVP_MD_CTX_size(&md_ctx);
	rv = EVP_DigestFinal(&md_ctx, Digest, &result_size);
	if (rv != EVP_SUCCESS) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	} else
		rv = TSS_SUCCESS;

	goto out;

err:
	DEBUG_print_openssl_errors();
out:
        return rv;
}

TSS_RESULT
Trspi_HashInit(Trspi_HashCtx *ctx, UINT32 HashType)
{
	int rv;
	EVP_MD *md;

	switch (HashType) {
		case TSS_HASH_SHA1:
			md = (EVP_MD *)EVP_sha1();
			break;
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
			break;
	}

	if ((ctx->ctx = malloc(sizeof(EVP_MD_CTX))) == NULL)
		return TSPERR(TSS_E_OUTOFMEMORY);

	rv = EVP_DigestInit((EVP_MD_CTX *)ctx->ctx, (const EVP_MD *)md);

	if (rv != EVP_SUCCESS) {
		DEBUG_print_openssl_errors();
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_HashUpdate(Trspi_HashCtx *ctx, UINT32 size, BYTE *data)
{
	int rv;

	if (ctx == NULL || ctx->ctx == NULL)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (data == NULL && size)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!size)
		return TSS_SUCCESS;

	rv = EVP_DigestUpdate(ctx->ctx, data, size);
	if (rv != EVP_SUCCESS) {
		DEBUG_print_openssl_errors();
		free(ctx->ctx);
		ctx->ctx = NULL;
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_HashFinal(Trspi_HashCtx *ctx, BYTE *digest)
{
	int rv;
	UINT32 result_size;

	if (ctx == NULL || ctx->ctx == NULL)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result_size = EVP_MD_CTX_size((EVP_MD_CTX *)ctx->ctx);
	rv = EVP_DigestFinal(ctx->ctx, digest, &result_size);
	if (rv != EVP_SUCCESS)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	free(ctx->ctx);
	ctx->ctx = NULL;

	return TSS_SUCCESS;
}

UINT32
Trspi_HMAC(UINT32 HashType, UINT32 SecretSize, BYTE* Secret, UINT32 BufSize, BYTE* Buf, BYTE* hmacOut)
{
	/*HMAC_CTX hmac_ctx;*/
	const EVP_MD *md;
	unsigned int len;
	int rv = TSS_SUCCESS;

	switch (HashType) {
		case TSS_HASH_SHA1:
			md = EVP_sha1();
			break;
		default:
			rv = TSPERR(TSS_E_BAD_PARAMETER);
			goto out;
			break;
	}

	len = EVP_MD_size(md);

	HMAC(md, Secret, SecretSize, Buf, BufSize, hmacOut, &len);
out:
	return rv;
}

TSS_RESULT
Trspi_MGF1(UINT32 alg, UINT32 seedLen, BYTE *seed, UINT32 outLen, BYTE *out)
{
	const EVP_MD *md;
	long olen = outLen, slen = seedLen;
	int rv = TSS_SUCCESS;

	switch (alg) {
		case TSS_HASH_SHA1:
			md = EVP_sha1();
			break;
		default:
			rv = TSPERR(TSS_E_BAD_PARAMETER);
			goto out;
			break;
	}

	rv = OpenSSL_MGF1(out, olen, seed, slen, md);
out:
	return rv;
}
