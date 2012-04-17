
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <openssl/evp.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcslog.h"

/*
 * Hopefully this will make the code clearer since
 * OpenSSL returns 1 on success
 */
#define EVP_SUCCESS 1

TSS_RESULT
Hash(UINT32 HashType, UINT32 BufSize, BYTE* Buf, BYTE* Digest)
{
	EVP_MD_CTX md_ctx;
	unsigned int result_size;
	int rv;

	switch (HashType) {
		case TSS_HASH_SHA1:
			rv = EVP_DigestInit(&md_ctx, EVP_sha1());
			break;
		default:
			rv = TCSERR(TSS_E_BAD_PARAMETER);
			goto out;
			break;
	}

	if (rv != EVP_SUCCESS) {
		rv = TCSERR(TSS_E_INTERNAL_ERROR);
		goto out;
	}

	rv = EVP_DigestUpdate(&md_ctx, Buf, BufSize);
	if (rv != EVP_SUCCESS) {
		rv = TCSERR(TSS_E_INTERNAL_ERROR);
		goto out;
	}

	result_size = EVP_MD_CTX_size(&md_ctx);
	rv = EVP_DigestFinal(&md_ctx, Digest, &result_size);
	if (rv != EVP_SUCCESS) {
		rv = TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		rv = TSS_SUCCESS;

out:
	return rv;
}
