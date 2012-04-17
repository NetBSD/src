
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsplog.h"


/* XXX Split into two functions */
TSS_RESULT
Tspi_TPM_GetAuditDigest(TSS_HTPM            hTpm,		/* in */
			TSS_HKEY            hKey,		/* in */
			TSS_BOOL            closeAudit,		/* in */
			UINT32*             pulAuditDigestSize,	/* out */
			BYTE**              prgbAuditDigest,	/* out */
			TPM_COUNTER_VALUE*  pCounterValue,	/* out */
			TSS_VALIDATION*     pValidationData,	/* out */
			UINT32*             ordSize,		/* out */
			UINT32**            ordList)		/* out */
{
	TSS_HCONTEXT tspContext;
	UINT32 counterValueSize;
	BYTE *counterValue = NULL;
	TPM_DIGEST auditDigest;
	TSS_RESULT result = TSS_SUCCESS;
	UINT64 offset;

	if ((pulAuditDigestSize == NULL) || (prgbAuditDigest == NULL) || (pCounterValue == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (hKey == NULL_HKEY)
		if ((ordSize == NULL) || (ordList == NULL))
			return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTpm, &tspContext)))
		return result;

	if (hKey == NULL_HKEY) {
		UINT32 startOrdinal = 0;
		TSS_BOOL more;
		UINT32 tcsOrdSize;
		UINT32 *tcsOrdList = NULL;
		UINT32 *pulTemp;

		*prgbAuditDigest = NULL;
		*pulAuditDigestSize = 0;
		*ordList = NULL;
		*ordSize = 0;
		do {
			if ((result = TCS_API(tspContext)->GetAuditDigest(tspContext, startOrdinal,
									  &auditDigest,
									  &counterValueSize,
									  &counterValue, &more,
									  &tcsOrdSize,
									  &tcsOrdList)))
				goto done1;

			if ((pulTemp =
			    calloc_tspi(tspContext,
				        (*ordSize + tcsOrdSize) * sizeof(UINT32))) == NULL) {
				LogError("malloc of %u bytes failed.", *ordSize + tcsOrdSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done1;
			}

			if (*ordList)
				memcpy(pulTemp, *ordList, *ordSize * sizeof(UINT32));
			memcpy(pulTemp + *ordSize, tcsOrdList, tcsOrdSize * sizeof(UINT32));

			free(tcsOrdList);
			tcsOrdList = NULL;

			if (*ordList)
				free_tspi(tspContext, *ordList);
			*ordList = pulTemp;
			*ordSize += tcsOrdSize;

			if (more == TRUE) {
				offset = 0;
				Trspi_UnloadBlob_UINT32(&offset, &startOrdinal,
							(BYTE *)(*ordList + (*ordSize - 1)));
				startOrdinal++;
				free(counterValue);
				counterValue = NULL;
			}
		} while (more == TRUE);

		*pulAuditDigestSize = sizeof(auditDigest.digest);
		if ((*prgbAuditDigest = calloc_tspi(tspContext, *pulAuditDigestSize)) == NULL) {
			LogError("malloc of %u bytes failed.", *pulAuditDigestSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done1;
		}
		offset = 0;
		Trspi_LoadBlob_DIGEST(&offset, *prgbAuditDigest, &auditDigest);

		offset = 0;
		Trspi_UnloadBlob_COUNTER_VALUE(&offset, counterValue, pCounterValue);

		result = TSS_SUCCESS;

done1:
		if (result != TSS_SUCCESS) {
			if (*prgbAuditDigest)
				free_tspi(tspContext, *prgbAuditDigest);
			if (*ordList)
				free_tspi(tspContext, *ordList);
			*prgbAuditDigest = NULL;
			*pulAuditDigestSize = 0;
			*ordList = NULL;
			*ordSize = 0;
		}
		free(counterValue);
		free(tcsOrdList);

		return result;
	}
	else {
		TSS_HPOLICY hPolicy;
		TSS_BOOL usesAuth;
		TCS_KEY_HANDLE tcsKeyHandle;
		TPM_AUTH keyAuth, *pAuth;
		Trspi_HashCtx hashCtx;
		TCPA_DIGEST digest;
		TPM_NONCE antiReplay;
		TPM_DIGEST auditDigest;
		TPM_DIGEST ordinalDigest;
		UINT32 sigSize;
		BYTE *sig = NULL;
		TPM_SIGN_INFO signInfo;
		UINT32 signInfoBlobSize;
		BYTE *signInfoBlob = NULL;

		if (pValidationData == NULL) {
			LogDebug("Internal Verify");
			if ((result = get_local_random(tspContext, FALSE, TPM_NONCE_SIZE,
						       (BYTE **)antiReplay.nonce)))
				return result;
		} else {
			LogDebug("External Verify");
			if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
				return TSPERR(TSS_E_BAD_PARAMETER);

			if (pValidationData->rgbExternalData == NULL)
				return TSPERR(TSS_E_BAD_PARAMETER);

			memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
			       sizeof(antiReplay.nonce));

			pValidationData->ulDataLength = 0;
			pValidationData->rgbData = NULL;
			pValidationData->ulValidationDataLength = 0;
			pValidationData->rgbValidationData = NULL;
		}

		if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
			return result;

		if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
			return result;

		if (usesAuth) {
			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetAuditDigestSigned);
			result |= Trspi_Hash_BOOL(&hashCtx, closeAudit);
			result |= Trspi_Hash_NONCE(&hashCtx, antiReplay.nonce);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				return result;

			pAuth = &keyAuth;
			if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_GetAuditDigestSigned,
							      hPolicy, FALSE, &digest, pAuth)))
				return result;
		}
		else
			pAuth = NULL;

		if ((result = TCS_API(tspContext)->GetAuditDigestSigned(tspContext, tcsKeyHandle,
									closeAudit, &antiReplay,
									pAuth, &counterValueSize,
									&counterValue, &auditDigest,
									&ordinalDigest, &sigSize,
									&sig)))
			return result;

		memset(&signInfo, 0, sizeof(signInfo));
		signInfo.tag = TPM_TAG_SIGNINFO;
		memcpy(signInfo.fixed, "ADIG", strlen("ADIG"));
		signInfo.replay = antiReplay;
		signInfo.dataLen = sizeof(auditDigest.digest) + counterValueSize +
				   sizeof(ordinalDigest.digest);
		if ((signInfo.data = malloc(signInfo.dataLen)) == NULL) {
			LogError("malloc of %u bytes failed.", signInfo.dataLen);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done2;
		}
		offset = 0;
		Trspi_LoadBlob_DIGEST(&offset, signInfo.data, &auditDigest);
		Trspi_LoadBlob(&offset, counterValueSize, signInfo.data, counterValue);
		Trspi_LoadBlob_DIGEST(&offset, signInfo.data, &ordinalDigest);

		if (usesAuth) {
			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, result);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetAuditDigestSigned);
			result |= Trspi_HashUpdate(&hashCtx, counterValueSize, counterValue);
			result |= Trspi_Hash_DIGEST(&hashCtx, auditDigest.digest);
			result |= Trspi_Hash_DIGEST(&hashCtx, ordinalDigest.digest);
			result |= Trspi_Hash_UINT32(&hashCtx, sigSize);
			result |= Trspi_HashUpdate(&hashCtx, sigSize, sig);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				goto done2;

			if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth)))
				goto done2;
		}

		offset = 0;
		Trspi_LoadBlob_SIGN_INFO(&offset, NULL, &signInfo);
		signInfoBlobSize = offset;
		signInfoBlob = malloc(signInfoBlobSize);
		if (signInfoBlob == NULL) {
			LogError("malloc of %u bytes failed.", signInfoBlobSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done2;
		}
		offset = 0;
		Trspi_LoadBlob_SIGN_INFO(&offset, signInfoBlob, &signInfo);

		if (pValidationData == NULL) {
			if ((result = Trspi_Hash(TSS_HASH_SHA1, signInfoBlobSize, signInfoBlob,
						 digest.digest)))
				goto done2;

			if ((result = __tspi_rsa_verify(hKey, TSS_HASH_SHA1, sizeof(digest.digest),
						 digest.digest, sigSize, sig))) {
				result = TSPERR(TSS_E_VERIFICATION_FAILED);
				goto done2;
			}
		} else {
			pValidationData->ulDataLength = signInfoBlobSize;
			pValidationData->rgbData = calloc_tspi(tspContext, signInfoBlobSize);
			if (pValidationData->rgbData == NULL) {
				LogError("malloc of %u bytes failed.", signInfoBlobSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done2;
			}
			memcpy(pValidationData->rgbData, signInfoBlob, signInfoBlobSize);

			pValidationData->ulValidationDataLength = sigSize;
			pValidationData->rgbValidationData = calloc_tspi(tspContext, sigSize);
			if (pValidationData->rgbValidationData == NULL) {
				LogError("malloc of %u bytes failed.", sigSize);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done2;
			}
			memcpy(pValidationData->rgbValidationData, sig, sigSize);
		}

		*pulAuditDigestSize = sizeof(auditDigest.digest);
		if ((*prgbAuditDigest = calloc_tspi(tspContext, *pulAuditDigestSize)) == NULL) {
			LogError("malloc of %u bytes failed.", *pulAuditDigestSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done2;
		}
		offset = 0;
		Trspi_LoadBlob_DIGEST(&offset, *prgbAuditDigest, &auditDigest);

		offset = 0;
		Trspi_UnloadBlob_COUNTER_VALUE(&offset, counterValue, pCounterValue);

		result = TSS_SUCCESS;

done2:
		if (result != TSS_SUCCESS) {
			if (*prgbAuditDigest)
				free_tspi(tspContext, *prgbAuditDigest);
			*prgbAuditDigest = NULL;
			*pulAuditDigestSize = 0;
			if (pValidationData != NULL) {
				if (pValidationData->rgbData)
					free_tspi(tspContext, pValidationData->rgbData);
				if (pValidationData->rgbValidationData)
					free_tspi(tspContext, pValidationData->rgbValidationData);
				pValidationData->ulDataLength = 0;
				pValidationData->rgbData = NULL;
				pValidationData->ulValidationDataLength = 0;
				pValidationData->rgbValidationData = NULL;
			}
		}
		free(counterValue);
		free(sig);
		free(signInfo.data);
		free(signInfoBlob);

		return result;
	}
}
