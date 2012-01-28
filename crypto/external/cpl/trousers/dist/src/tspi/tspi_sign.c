
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"

TSS_RESULT
Tspi_Hash_Sign(TSS_HHASH hHash,			/* in */
	       TSS_HKEY hKey,			/* in */
	       UINT32 * pulSignatureLength,	/* out */
	       BYTE ** prgbSignature)		/* out */
{
	TPM_AUTH privAuth;
	TPM_AUTH *pPrivAuth = &privAuth;
	TCPA_DIGEST digest;
	TCPA_RESULT result;
	TSS_HPOLICY hPolicy;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_BOOL usesAuth;
	TSS_HCONTEXT tspContext;
	UINT32 ulDataLen;
	BYTE *data;
	Trspi_HashCtx hashCtx;

	if (pulSignatureLength == NULL || prgbSignature == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_hash_get_tsp_context(hHash, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
		return result;

	if ((result = obj_hash_get_value(hHash, &ulDataLen, &data)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
		goto done;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Sign);
		result |= Trspi_Hash_UINT32(&hashCtx, ulDataLen);
		result |= Trspi_HashUpdate(&hashCtx, ulDataLen, data);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		pPrivAuth = &privAuth;

		if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_Sign, hPolicy, FALSE, &digest,
						      &privAuth)))
			goto done;
	} else {
		pPrivAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->Sign(tspContext, tcsKeyHandle, ulDataLen, data,
						pPrivAuth, pulSignatureLength, prgbSignature)))
		goto done;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Sign);
		result |= Trspi_Hash_UINT32(&hashCtx, *pulSignatureLength);
		result |= Trspi_HashUpdate(&hashCtx, *pulSignatureLength, *prgbSignature);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free(*prgbSignature);
			goto done;
		}

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &privAuth))) {
			free(*prgbSignature);
			goto done;
		}
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbSignature)))
		free(*prgbSignature);

done:
	free_tspi(tspContext, data);
	return result;
}

TSS_RESULT
Tspi_Hash_VerifySignature(TSS_HHASH hHash,		/* in */
			  TSS_HKEY hKey,		/* in */
			  UINT32 ulSignatureLength,	/* in */
			  BYTE * rgbSignature)		/* in */
{
	TCPA_RESULT result;
	BYTE *pubKey = NULL;
	UINT32 pubKeySize;
	BYTE *hashData = NULL;
	UINT32 hashDataSize;
	UINT32 sigScheme;
	TSS_HCONTEXT tspContext;

	if (ulSignatureLength > 0 && rgbSignature == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_modulus(hKey, &pubKeySize, &pubKey)))
		return result;

	if ((result = obj_rsakey_get_ss(hKey, &sigScheme))) {
		free_tspi(tspContext, pubKey);
		return result;
	}

	if ((result = obj_hash_get_value(hHash, &hashDataSize, &hashData))) {
		free_tspi(tspContext, pubKey);
		return result;
	}

	if (sigScheme == TSS_SS_RSASSAPKCS1V15_SHA1) {
		result = Trspi_Verify(TSS_HASH_SHA1, hashData, hashDataSize, pubKey, pubKeySize,
				      rgbSignature, ulSignatureLength);
	} else if (sigScheme == TSS_SS_RSASSAPKCS1V15_DER) {
		result = Trspi_Verify(TSS_HASH_OTHER, hashData, hashDataSize, pubKey, pubKeySize,
				      rgbSignature, ulSignatureLength);
	} else {
		result = TSPERR(TSS_E_INVALID_SIGSCHEME);
	}

	free_tspi(tspContext, pubKey);
	free_tspi(tspContext, hashData);

	return result;
}
