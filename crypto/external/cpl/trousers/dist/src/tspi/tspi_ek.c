
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_CreateEndorsementKey(TSS_HTPM hTPM,			/* in */
			      TSS_HKEY hKey,			/* in */
			      TSS_VALIDATION * pValidationData)	/* in, out */
{
	TCPA_NONCE antiReplay;
	TCPA_DIGEST digest;
	TSS_RESULT result;
	UINT32 ekSize;
	BYTE *ek;
	TSS_KEY dummyKey;
	UINT64 offset;
	TCPA_DIGEST hash;
	UINT32 newEKSize;
	BYTE *newEK;
	TSS_HCONTEXT tspContext;
	TCPA_PUBKEY pubEK;
	Trspi_HashCtx hashCtx;

	memset(&pubEK, 0, sizeof(TCPA_PUBKEY));
	memset(&dummyKey, 0, sizeof(TSS_KEY));

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_blob(hKey, &ekSize, &ek)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, ek, &dummyKey)))
		return result;

	offset = 0;
	Trspi_LoadBlob_KEY_PARMS(&offset, ek, &dummyKey.algorithmParms);
	free_key_refs(&dummyKey);
	ekSize = offset;

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TCPA_NONCE),
					       (BYTE **)antiReplay.nonce))) {
			LogError("Failed to create random nonce");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
		       sizeof(antiReplay.nonce));
	}

	if ((result = TCS_API(tspContext)->CreateEndorsementKeyPair(tspContext, antiReplay, ekSize,
								    ek, &newEKSize, &newEK,
								    &digest)))
		return result;

	if (pValidationData == NULL) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_HashUpdate(&hashCtx, newEKSize, newEK);
		result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN, antiReplay.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, hash.digest)))
			goto done;

		if (memcmp(hash.digest, digest.digest, TCPA_SHA1_160_HASH_LEN)) {
			LogError("Internal verification failed");
			result = TSPERR(TSS_E_EK_CHECKSUM);
			goto done;
		}
	} else {
		pValidationData->rgbData = calloc_tspi(tspContext, newEKSize);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %u bytes failed.", newEKSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		pValidationData->ulDataLength = newEKSize;
		memcpy(pValidationData->rgbData, newEK, newEKSize);
		memcpy(&pValidationData->rgbData[ekSize], antiReplay.nonce,
		       sizeof(antiReplay.nonce));

		pValidationData->rgbValidationData = calloc_tspi(tspContext,
								 TCPA_SHA1_160_HASH_LEN);
		if (pValidationData->rgbValidationData == NULL) {
			LogError("malloc of %d bytes failed.", TCPA_SHA1_160_HASH_LEN);
			free_tspi(tspContext, pValidationData->rgbData);
			pValidationData->rgbData = NULL;
			pValidationData->ulDataLength = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		pValidationData->ulValidationDataLength = TCPA_SHA1_160_HASH_LEN;
		memcpy(pValidationData->rgbValidationData, digest.digest, TCPA_SHA1_160_HASH_LEN);
	}

	if ((result = obj_rsakey_set_pubkey(hKey, FALSE, newEK)) && pValidationData) {
		free_tspi(tspContext, pValidationData->rgbValidationData);
		free_tspi(tspContext, pValidationData->rgbData);
		pValidationData->rgbData = NULL;
		pValidationData->ulDataLength = 0;
		pValidationData->rgbValidationData = NULL;
		pValidationData->ulValidationDataLength = 0;
	}

done:
	free(newEK);

	return result;
}

TSS_RESULT
Tspi_TPM_GetPubEndorsementKey(TSS_HTPM hTPM,			/* in */
			      TSS_BOOL fOwnerAuthorized,	/* in */
			      TSS_VALIDATION *pValidationData,	/* in, out */
			      TSS_HKEY *phEndorsementPubKey)	/* out */
{
	TCPA_DIGEST digest;
	TSS_RESULT result;
	UINT64 offset;
	UINT32 pubEKSize;
	BYTE *pubEK;
	TCPA_NONCE antiReplay;
	TCPA_DIGEST checkSum;
	TSS_HOBJECT retKey;
	TSS_HCONTEXT tspContext;
	TCPA_PUBKEY pubKey;
	Trspi_HashCtx hashCtx;

	memset(&pubKey, 0, sizeof(TCPA_PUBKEY));

	if (phEndorsementPubKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (fOwnerAuthorized)
		return owner_get_pubek(tspContext, hTPM, phEndorsementPubKey);

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TCPA_NONCE),
					       (BYTE **)antiReplay.nonce))) {
			LogDebug("Failed to generate random nonce");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
		       sizeof(antiReplay.nonce));
	}

	/* call down to the TPM */
	if ((result = TCS_API(tspContext)->ReadPubek(tspContext, antiReplay, &pubEKSize, &pubEK,
						     &checkSum)))
		return result;

	/* validate the returned hash, or set up the return so that the user can */
	if (pValidationData == NULL) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_HashUpdate(&hashCtx, pubEKSize, pubEK);
		result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN, antiReplay.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		/* check validation of the entire pubkey structure */
		if (memcmp(digest.digest, checkSum.digest, TPM_SHA1_160_HASH_LEN)) {
			/* validation failed, unload the pubEK in order to hash
			 * just the pubKey portion of the pubEK. This is done on
			 * Atmel chips specifically.
			 */
			offset = 0;
			memset(&pubKey, 0, sizeof(TCPA_PUBKEY));
			if ((result = Trspi_UnloadBlob_PUBKEY(&offset, pubEK, &pubKey)))
				goto done;

			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_HashUpdate(&hashCtx, pubKey.pubKey.keyLength,
						   pubKey.pubKey.key);
			result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN,
						   antiReplay.nonce);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				goto done;

			if (memcmp(digest.digest, checkSum.digest, TCPA_SHA1_160_HASH_LEN)) {
				result = TSPERR(TSS_E_EK_CHECKSUM);
				goto done;
			}
		}
	} else {
		/* validate the entire TCPA_PUBKEY structure */
		pValidationData->ulDataLength = pubEKSize + TCPA_SHA1_160_HASH_LEN;
		pValidationData->rgbData = calloc_tspi(tspContext,
				pValidationData->ulDataLength);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %u bytes failed.",
					pValidationData->ulDataLength);
			pValidationData->ulDataLength = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		memcpy(pValidationData->rgbData, pubEK, pubEKSize);
		memcpy(&pValidationData->rgbData[pubEKSize], antiReplay.nonce,
				TCPA_SHA1_160_HASH_LEN);

		pValidationData->ulValidationDataLength = TCPA_SHA1_160_HASH_LEN;
		pValidationData->rgbValidationData = calloc_tspi(tspContext,
				TCPA_SHA1_160_HASH_LEN);
		if (pValidationData->rgbValidationData == NULL) {
			LogError("malloc of %d bytes failed.", TCPA_SHA1_160_HASH_LEN);
			pValidationData->ulValidationDataLength = 0;
			pValidationData->ulDataLength = 0;
			free_tspi(tspContext,pValidationData->rgbData);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		memcpy(pValidationData->rgbValidationData, checkSum.digest,
				TPM_SHA1_160_HASH_LEN);
	}

	if ((result = obj_rsakey_add(tspContext, TSS_KEY_SIZE_2048|TSS_KEY_TYPE_LEGACY, &retKey)))
		goto done;

	if ((result = obj_rsakey_set_pubkey(retKey, TRUE, pubEK)))
		goto done;

	*phEndorsementPubKey = retKey;

done:
	free(pubEK);
	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
Tspi_TPM_CreateRevocableEndorsementKey(TSS_HTPM hTPM,			/* in */
				       TSS_HKEY hKey,			/* in */
				       TSS_VALIDATION * pValidationData,/* in, out */
				       UINT32 * pulEkResetDataLength,	/* in, out */
				       BYTE ** prgbEkResetData)		/* in, out */
{
	TPM_NONCE antiReplay;
	TPM_DIGEST digest;
	TSS_RESULT result;
	UINT32 ekSize;
	BYTE *ek;
	TSS_KEY dummyKey;
	UINT64 offset;
	TSS_BOOL genResetAuth;
	TPM_DIGEST eKResetAuth;
	TPM_DIGEST hash;
	UINT32 newEKSize;
	BYTE *newEK;
	TSS_HCONTEXT tspContext;
	TPM_PUBKEY pubEK;
	Trspi_HashCtx hashCtx;

	memset(&pubEK, 0, sizeof(TPM_PUBKEY));
	memset(&dummyKey, 0, sizeof(TSS_KEY));
	memset(&eKResetAuth, 0xff, sizeof(eKResetAuth));

	if (!pulEkResetDataLength || !prgbEkResetData)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (*pulEkResetDataLength != 0) {
		if (*prgbEkResetData == NULL)
			return TSPERR(TSS_E_BAD_PARAMETER);

		if (*pulEkResetDataLength < sizeof(eKResetAuth.digest))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(eKResetAuth.digest, *prgbEkResetData, sizeof(eKResetAuth.digest));
		genResetAuth = FALSE;
	} else {
		if (*prgbEkResetData != NULL)
			return TSPERR(TSS_E_BAD_PARAMETER);

		genResetAuth = TRUE;
	}

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_blob(hKey, &ekSize, &ek)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, ek, &dummyKey)))
		return result;

	offset = 0;
	Trspi_LoadBlob_KEY_PARMS(&offset, ek, &dummyKey.algorithmParms);
	free_key_refs(&dummyKey);
	ekSize = offset;

	if (pValidationData == NULL) {
		if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
					       (BYTE **)antiReplay.nonce))) {
			LogError("Failed to create random nonce");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
		       sizeof(antiReplay.nonce));
	}

	if ((result = RPC_CreateRevocableEndorsementKeyPair(tspContext, antiReplay, ekSize, ek,
							    genResetAuth, &eKResetAuth, &newEKSize,
							    &newEK, &digest)))
		return result;

	if (pValidationData == NULL) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_HashUpdate(&hashCtx, newEKSize, newEK);
		result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN, antiReplay.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, hash.digest)))
			goto done;

		if (memcmp(hash.digest, digest.digest, TPM_SHA1_160_HASH_LEN)) {
			LogError("Internal verification failed");
			result = TSPERR(TSS_E_EK_CHECKSUM);
			goto done;
		}
	} else {
		pValidationData->rgbData = calloc_tspi(tspContext, newEKSize);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %u bytes failed.", newEKSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		pValidationData->ulDataLength = newEKSize;
		memcpy(pValidationData->rgbData, newEK, newEKSize);
		memcpy(&pValidationData->rgbData[ekSize], antiReplay.nonce,
		       sizeof(antiReplay.nonce));

		pValidationData->rgbValidationData = calloc_tspi(tspContext,
								 TPM_SHA1_160_HASH_LEN);
		if (pValidationData->rgbValidationData == NULL) {
			LogError("malloc of %d bytes failed.", TPM_SHA1_160_HASH_LEN);
			free_tspi(tspContext, pValidationData->rgbData);
			pValidationData->rgbData = NULL;
			pValidationData->ulDataLength = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		pValidationData->ulValidationDataLength = TPM_SHA1_160_HASH_LEN;
		memcpy(pValidationData->rgbValidationData, digest.digest, TPM_SHA1_160_HASH_LEN);
	}

	if ((result = obj_rsakey_set_pubkey(hKey, FALSE, newEK))) {
		if (pValidationData) {
			free_tspi(tspContext, pValidationData->rgbValidationData);
			free_tspi(tspContext, pValidationData->rgbData);
			pValidationData->rgbData = NULL;
			pValidationData->ulDataLength = 0;
			pValidationData->rgbValidationData = NULL;
			pValidationData->ulValidationDataLength = 0;
		}
		goto done;
	}

	if (genResetAuth) {
		if ((*prgbEkResetData = calloc_tspi(tspContext, sizeof(eKResetAuth.digest))) == NULL) {
			LogError("malloc of %zd bytes failed.", sizeof(eKResetAuth.digest));
			if (pValidationData) {
				free_tspi(tspContext, pValidationData->rgbValidationData);
				free_tspi(tspContext, pValidationData->rgbData);
				pValidationData->rgbData = NULL;
				pValidationData->ulDataLength = 0;
				pValidationData->rgbValidationData = NULL;
				pValidationData->ulValidationDataLength = 0;
			}
			goto done;
		}

		memcpy(*prgbEkResetData, eKResetAuth.digest, sizeof(eKResetAuth.digest));
		*pulEkResetDataLength = sizeof(eKResetAuth.digest);
	}

done:
	free(newEK);

	return result;
}

TSS_RESULT
Tspi_TPM_RevokeEndorsementKey(TSS_HTPM hTPM,			/* in */
			      UINT32  ulEkResetDataLength,	/* in */
			      BYTE * rgbEkResetData)		/* in */
{
	TSS_HCONTEXT tspContext;
	TPM_DIGEST eKResetAuth;
	TSS_RESULT result;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (ulEkResetDataLength < sizeof(eKResetAuth.digest) || !rgbEkResetData)
		return TSPERR(TSS_E_BAD_PARAMETER);

	memcpy(eKResetAuth.digest, rgbEkResetData, sizeof(eKResetAuth.digest));

	if ((result = RPC_RevokeEndorsementKeyPair(tspContext, &eKResetAuth)))
		return result;

	return result;
}
#endif

