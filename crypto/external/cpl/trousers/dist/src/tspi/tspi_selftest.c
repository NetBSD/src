
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_SelfTestFull(TSS_HTPM hTPM)	/*  in */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	return TCS_API(tspContext)->SelfTestFull(tspContext);
}

TSS_RESULT
Tspi_TPM_CertifySelfTest(TSS_HTPM hTPM,				/* in */
			 TSS_HKEY hKey,				/* in */
			 TSS_VALIDATION *pValidationData)	/* in, out */
{
	TCPA_RESULT result;
	TPM_AUTH keyAuth;
	UINT64 offset = 0;
	TCPA_DIGEST digest;
	TCPA_NONCE antiReplay;
	UINT32 outDataSize;
	BYTE *outData;
	TSS_HPOLICY hPolicy;
	TCS_KEY_HANDLE keyTCSKeyHandle;
	BYTE *keyData = NULL;
	UINT32 keyDataSize;
	TSS_KEY keyContainer;
	TPM_AUTH *pKeyAuth;
	TSS_BOOL useAuth;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;


	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE,
					    &hPolicy, &useAuth)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &keyTCSKeyHandle)))
		return result;

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TCPA_NONCE),
					       (BYTE **)antiReplay.nonce))) {
			LogError("Failed creating random nonce");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
		       sizeof(antiReplay.nonce));
	}

	if (useAuth) {
		LogDebug("Uses Auth");

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CertifySelfTest);
		result |= Trspi_HashUpdate(&hashCtx, sizeof(TCPA_NONCE), antiReplay.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_CertifySelfTest, hPolicy, FALSE,
						      &digest, &keyAuth)))
			return result;
		pKeyAuth = &keyAuth;
	} else {
		LogDebug("No Auth");
		pKeyAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->CertifySelfTest(tspContext, keyTCSKeyHandle, antiReplay,
							   pKeyAuth, &outDataSize, &outData)))
		return result;

	/* validate auth */
	if (useAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CertifySelfTest);
		result |= Trspi_Hash_UINT32(&hashCtx, outDataSize);
		result |= Trspi_HashUpdate(&hashCtx, outDataSize, outData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &keyAuth)))
			return result;
	}

	if (pValidationData == NULL) {
		if ((result = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
				       TSS_TSPATTRIB_KEYBLOB_BLOB, &keyDataSize, &keyData))) {
			LogError("Failed call to GetAttribData to get key blob");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		offset = 0;
		memset(&keyContainer, 0, sizeof(TSS_KEY));
		if ((result = UnloadBlob_TSS_KEY(&offset, keyData, &keyContainer)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_HashUpdate(&hashCtx, strlen("Test Passed"), (BYTE *)"Test Passed");
		result |= Trspi_HashUpdate(&hashCtx, sizeof(TCPA_NONCE), antiReplay.nonce);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CertifySelfTest);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = Trspi_Verify(TSS_HASH_SHA1, digest.digest, 20,
					 keyContainer.pubKey.key, keyContainer.pubKey.keyLength,
					 outData, outDataSize))) {
			free(outData);
			free_key_refs(&keyContainer);
			return TSPERR(TSS_E_VERIFICATION_FAILED);
		}

	} else {
		pValidationData->ulDataLength = sizeof(TCPA_NONCE) + sizeof(UINT32) +
						strlen("Test Passed");
		pValidationData->rgbData = calloc_tspi(tspContext, pValidationData->ulDataLength);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %u bytes failed.", pValidationData->ulDataLength);
			pValidationData->ulDataLength = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		offset = 0;
		Trspi_LoadBlob(&offset, strlen("Test Passed"), pValidationData->rgbData,
			       (BYTE *)"Test Passed");
		Trspi_LoadBlob(&offset, sizeof(TCPA_NONCE), pValidationData->rgbData,
			       antiReplay.nonce);
		Trspi_LoadBlob_UINT32(&offset, TPM_ORD_CertifySelfTest, pValidationData->rgbData);
		pValidationData->ulValidationDataLength = outDataSize;
		pValidationData->rgbValidationData = calloc_tspi(tspContext, outDataSize);
		if (pValidationData->rgbValidationData == NULL) {
			free_tspi(tspContext, pValidationData->rgbData);
			pValidationData->rgbData = NULL;
			pValidationData->ulDataLength = 0;
			LogError("malloc of %u bytes failed.",
				 pValidationData->ulValidationDataLength);
			pValidationData->ulValidationDataLength = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		memcpy(pValidationData->rgbValidationData, outData, outDataSize);
		free(outData);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_GetTestResult(TSS_HTPM hTPM,			/* in */
		       UINT32 * pulTestResultLength,	/* out */
		       BYTE ** prgbTestResult)		/* out */
{
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if (pulTestResultLength == NULL || prgbTestResult == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = TCS_API(tspContext)->GetTestResult(tspContext, pulTestResultLength,
							 prgbTestResult)))
		return result;

	if ((result = __tspi_add_mem_entry(tspContext, *prgbTestResult))) {
		free(*prgbTestResult);
		*prgbTestResult = NULL;
		*pulTestResultLength = 0;
	}

	return TSS_SUCCESS;
}

