
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
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
Tspi_Hash_TickStampBlob(TSS_HHASH       hHash,			/* in */
			TSS_HKEY        hIdentKey,		/* in */
			TSS_VALIDATION* pValidationData)	/* in */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TCS_KEY_HANDLE tcsKey;
	TSS_HPOLICY hPolicy;
	TPM_DIGEST digest;
	TSS_BOOL usesAuth;
	TPM_AUTH auth, *pAuth;
	UINT32 hashLen, sigLen, tcLen, signInfoLen;
	BYTE *hash, *sig, *tc, *signInfo = NULL;
	UINT64 offset;
	Trspi_HashCtx hashCtx;

	if (pValidationData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_hash_get_tsp_context(hHash, &tspContext)))
		return result;

	if (pValidationData->ulExternalDataLength != sizeof(TPM_NONCE))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_hash_get_value(hHash, &hashLen, &hash)))
		return result;

	if (hashLen != sizeof(TPM_DIGEST)) {
		free_tspi(tspContext, hash);
		return TSPERR(TSS_E_HASH_INVALID_LENGTH);
	}

	if ((result = obj_rsakey_get_policy(hIdentKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hIdentKey, &tcsKey)))
		return result;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_TickStampBlob);
		result |= Trspi_HashUpdate(&hashCtx, sizeof(TPM_NONCE),
					   pValidationData->rgbExternalData);
		result |= Trspi_HashUpdate(&hashCtx, sizeof(TPM_DIGEST), hash);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free_tspi(tspContext, hash);
			return result;
		}

		if ((result = secret_PerformAuth_OIAP(hIdentKey, TPM_ORD_TickStampBlob, hPolicy,
						      FALSE, &digest, &auth))) {
			free_tspi(tspContext, hash);
			return result;
		}
		pAuth = &auth;
	} else
		pAuth = NULL;

	if ((result = TCS_API(tspContext)->TickStampBlob(tspContext, tcsKey,
						(TPM_NONCE *)pValidationData->rgbExternalData,
						(TPM_DIGEST *)hash, pAuth, &sigLen, &sig, &tcLen,
						&tc))) {
		free_tspi(tspContext, hash);
		return result;
	}

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_TickStampBlob);
		result |= Trspi_HashUpdate(&hashCtx, tcLen, tc);
		result |= Trspi_Hash_UINT32(&hashCtx, sigLen);
		result |= Trspi_HashUpdate(&hashCtx, sigLen, sig);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free_tspi(tspContext, hash);
			free(sig);
			free(tc);
			return result;
		}

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth))) {
			free_tspi(tspContext, hash);
			free(sig);
			free(tc);
			return result;
		}
	}
	
	/*                 tag       fixed      replay          length(Data)                 Data        */
	signInfoLen = sizeof(UINT16) + 4 + sizeof(TPM_NONCE) + sizeof(UINT32) + sizeof(TPM_DIGEST) + tcLen;
	if ((signInfo = calloc_tspi(tspContext, signInfoLen)) == NULL) {
		free_tspi(tspContext, hash);
		free(sig);
		free(tc);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, TPM_TAG_SIGNINFO, signInfo);
	Trspi_LoadBlob_BYTE(&offset, 'T', signInfo);
	Trspi_LoadBlob_BYTE(&offset, 'S', signInfo);
	Trspi_LoadBlob_BYTE(&offset, 'T', signInfo);
	Trspi_LoadBlob_BYTE(&offset, 'P', signInfo);
	Trspi_LoadBlob(&offset, sizeof(TPM_NONCE), signInfo, pValidationData->rgbExternalData);
	Trspi_LoadBlob_UINT32(&offset, sizeof(TPM_DIGEST) + tcLen, signInfo);
	Trspi_LoadBlob(&offset, sizeof(TPM_DIGEST), signInfo, hash);
	Trspi_LoadBlob(&offset, (size_t)tcLen, signInfo, tc);

	free(tc);
	free_tspi(tspContext, hash);

	pValidationData->rgbData = signInfo;
	pValidationData->ulDataLength = signInfoLen;
		
	/* tag sig so that it can be free'd by the app through Tspi_Context_FreeMemory */
	if ((result = __tspi_add_mem_entry(tspContext, sig))) {
		free_tspi(tspContext, signInfo);
		free(sig);
		return result;
	}

	pValidationData->rgbValidationData = sig;
	pValidationData->ulValidationDataLength = sigLen;

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_ReadCurrentTicks(TSS_HTPM           hTPM,	/* in */
			  TPM_CURRENT_TICKS* tickCount)	/* out */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	UINT32 tcLen;
	BYTE *tc;
	UINT64 offset;

	if (tickCount == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = TCS_API(tspContext)->ReadCurrentTicks(tspContext, &tcLen, &tc)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_CURRENT_TICKS(&offset, tc, tickCount);
	free(tc);

	return result;
}
