
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
Tspi_TPM_CreateMaintenanceArchive(TSS_HTPM hTPM,			/* in */
				  TSS_BOOL fGenerateRndNumber,		/* in */
				  UINT32 * pulRndNumberLength,		/* out */
				  BYTE ** prgbRndNumber,		/* out */
				  UINT32 * pulArchiveDataLength,	/* out */
				  BYTE ** prgbArchiveData)		/* out */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hOwnerPolicy;
	TPM_AUTH ownerAuth;
	TCPA_DIGEST digest;
	Trspi_HashCtx hashCtx;

	if (pulArchiveDataLength == NULL || prgbArchiveData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (fGenerateRndNumber &&
	    (pulRndNumberLength == NULL || prgbRndNumber == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hOwnerPolicy)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CreateMaintenanceArchive);
	result |= Trspi_Hash_BYTE(&hashCtx, fGenerateRndNumber);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_CreateMaintenanceArchive, hOwnerPolicy,
					      FALSE, &digest, &ownerAuth)))
		return result;

	if ((result = TCS_API(tspContext)->CreateMaintenanceArchive(tspContext, fGenerateRndNumber,
								    &ownerAuth, pulRndNumberLength,
								    prgbRndNumber,
								    pulArchiveDataLength,
								    prgbArchiveData)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CreateMaintenanceArchive);
	result |= Trspi_Hash_UINT32(&hashCtx, *pulRndNumberLength);
	result |= Trspi_HashUpdate(&hashCtx, *pulRndNumberLength, *prgbRndNumber);
	result |= Trspi_Hash_UINT32(&hashCtx, *pulArchiveDataLength);
	result |= Trspi_HashUpdate(&hashCtx, *pulArchiveDataLength, *prgbArchiveData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error1;

	if ((result = obj_policy_validate_auth_oiap(hOwnerPolicy, &digest, &ownerAuth)))
		goto error1;

	if ((result = __tspi_add_mem_entry(tspContext, *prgbRndNumber)))
		goto error1;

	if ((result = __tspi_add_mem_entry(tspContext, *prgbArchiveData))) {
		free_tspi(tspContext, *prgbRndNumber);
		goto error2;
	}

	return TSS_SUCCESS;
error1:
	free(*prgbRndNumber);
error2:
	free(*prgbArchiveData);
	return result;
}

TSS_RESULT
Tspi_TPM_KillMaintenanceFeature(TSS_HTPM hTPM)	/*  in */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hOwnerPolicy;
	TPM_AUTH ownerAuth;
	TCPA_DIGEST digest;
	Trspi_HashCtx hashCtx;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hOwnerPolicy)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_KillMaintenanceFeature);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_KillMaintenanceFeature, hOwnerPolicy,
					      FALSE, &digest, &ownerAuth)))
		return result;

	if ((result = TCS_API(tspContext)->KillMaintenanceFeature(tspContext, &ownerAuth)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_KillMaintenanceFeature);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = obj_policy_validate_auth_oiap(hOwnerPolicy, &digest, &ownerAuth)))
		return result;

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_LoadMaintenancePubKey(TSS_HTPM hTPM,				/* in */
			       TSS_HKEY hMaintenanceKey,		/* in */
			       TSS_VALIDATION * pValidationData)	/* in, out */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TCPA_DIGEST checkSum, digest;
	TCPA_NONCE nonce;
	UINT64 offset;
	UINT32 pubBlobSize;
	BYTE hashBlob[512], *pubBlob;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TCPA_NONCE),
					       (BYTE **)nonce.nonce)))
			return result;
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(nonce.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(&nonce.nonce, pValidationData->rgbExternalData, sizeof(nonce.nonce));
	}

	if ((result = obj_rsakey_get_pub_blob(hMaintenanceKey, &pubBlobSize, &pubBlob)))
		return result;

	if ((result = TCS_API(tspContext)->LoadManuMaintPub(tspContext, nonce, pubBlobSize, pubBlob,
							    &checkSum)))
		return result;

	offset = 0;
	Trspi_LoadBlob(&offset, pubBlobSize, hashBlob, pubBlob);
	Trspi_LoadBlob(&offset, TCPA_SHA1_160_HASH_LEN, hashBlob, (BYTE *)&nonce.nonce);

	if (pValidationData == NULL) {
		if ((result = Trspi_Hash(TSS_HASH_SHA1, offset, hashBlob, digest.digest)))
			return result;

		if (memcmp(&digest.digest, &checkSum.digest, TCPA_SHA1_160_HASH_LEN))
			result = TSPERR(TSS_E_FAIL);
	} else {
		if ((pValidationData->rgbData = calloc_tspi(tspContext, offset)) == NULL)
			return TSPERR(TSS_E_OUTOFMEMORY);

		pValidationData->ulDataLength = offset;
		memcpy(pValidationData->rgbData, hashBlob, offset);

		if ((pValidationData->rgbValidationData = calloc_tspi(tspContext,
								      TPM_SHA1_160_HASH_LEN))
		     == NULL) {
			free_tspi(tspContext, pValidationData->rgbData);
			pValidationData->rgbData = NULL;
			pValidationData->ulDataLength = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		pValidationData->ulValidationDataLength = TCPA_SHA1_160_HASH_LEN;

		memcpy(pValidationData->rgbValidationData, checkSum.digest, TCPA_SHA1_160_HASH_LEN);
	}

	return result;
}

TSS_RESULT
Tspi_TPM_CheckMaintenancePubKey(TSS_HTPM hTPM,				/* in */
				TSS_HKEY hMaintenanceKey,		/* in */
				TSS_VALIDATION * pValidationData)	/* in, out */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	TCPA_DIGEST checkSum, digest;
	TCPA_NONCE nonce;
	UINT32 pubBlobSize;
	BYTE *pubBlob;
	Trspi_HashCtx hashCtx;

	if ((pValidationData && hMaintenanceKey) || (!pValidationData && !hMaintenanceKey))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TCPA_NONCE),
					       (BYTE **)nonce.nonce)))
			return result;
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(nonce.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(&nonce.nonce, pValidationData->rgbExternalData, sizeof(nonce.nonce));
	}

	if ((result = TCS_API(tspContext)->ReadManuMaintPub(tspContext, nonce, &checkSum)))
		return result;

	if (pValidationData == NULL) {
		if ((result = obj_rsakey_get_pub_blob(hMaintenanceKey, &pubBlobSize, &pubBlob)))
			return result;

		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_HashUpdate(&hashCtx, pubBlobSize, pubBlob);
		result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN, (BYTE *)&nonce.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if (memcmp(&digest.digest, &checkSum.digest, TCPA_SHA1_160_HASH_LEN))
			result = TSPERR(TSS_E_FAIL);

		free_tspi(tspContext, pubBlob);
	} else {
		/* Ignore Data and DataLength, the application must already have this data.
		 * Do, however, copy out the checksum so that the application can verify */
		if ((pValidationData->rgbValidationData = calloc_tspi(tspContext,
								      TCPA_SHA1_160_HASH_LEN))
		     == NULL) {
			free_tspi(tspContext, pubBlob);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		pValidationData->ulValidationDataLength = TCPA_SHA1_160_HASH_LEN;
		memcpy(pValidationData->rgbValidationData, checkSum.digest, TCPA_SHA1_160_HASH_LEN);
	}

	return result;
}

