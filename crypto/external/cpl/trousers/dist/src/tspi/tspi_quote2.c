
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
Tspi_TPM_Quote2(TSS_HTPM        hTPM,            // in
		TSS_HKEY        hIdentKey,       // in
		TSS_BOOL        fAddVersion,     // in
		TSS_HPCRS       hPcrComposite,   // in
		TSS_VALIDATION* pValidationData, // in, out
		UINT32*         versionInfoSize, // out
		BYTE**          versionInfo)     // out
{
	TPM_RESULT result;
	TSS_HCONTEXT tspContext;
	TPM_AUTH privAuth;
	TPM_AUTH *pPrivAuth = &privAuth;
	UINT64 offset;
	TPM_DIGEST digest;
	TSS_BOOL usesAuth;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_HPOLICY hPolicy;
	TPM_NONCE antiReplay;
	UINT32 pcrDataSize;
	BYTE pcrData[128];
	UINT32 sigSize = 0;
	BYTE *sig = NULL;
	UINT32 pcrDataOutSize;
	BYTE *pcrDataOut;
	BYTE quoteinfo[1024];
	Trspi_HashCtx hashCtx;

	LogDebug("Tspi_TPM_Quote2 Start:");

	/* Takes the context that this TPM handle is on */
	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	/* Test if the hPcrComposite is valid */
	if ((hPcrComposite) && !obj_is_pcrs(hPcrComposite))
		return TSPERR(TSS_E_INVALID_HANDLE);

	/*  get the identKey Policy */
	if ((result = obj_rsakey_get_policy(hIdentKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
		return result;

	/*  get the Identity TCS keyHandle */
	if ((result = obj_rsakey_get_tcs_handle(hIdentKey, &tcsKeyHandle)))
		return result;

	/* Sets the validation data - if NULL, TSS provides it's own random value. If
	 * not NULL, takes the validation external data and sets the antiReplay data
	 * with this */
	if (pValidationData == NULL) {
		LogDebug("Internal Verify:");
		if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
					       (BYTE **)antiReplay.nonce)))
			return result;
	} else {
		LogDebug("External Verify:");
		if (pValidationData->ulExternalDataLength < sizeof(antiReplay.nonce))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(antiReplay.nonce, pValidationData->rgbExternalData,
				sizeof(antiReplay.nonce));
	}

	/* Create the TPM_PCR_COMPOSITE object */
	pcrDataSize = 0;
	if (hPcrComposite) {
		/* Load the PCR Selection Object into the pcrData */
		if ((result = obj_pcrs_get_selection(hPcrComposite, &pcrDataSize, pcrData)))
			return result;
	}

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Quote2);
		result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN, antiReplay.nonce);
		result |= Trspi_HashUpdate(&hashCtx, pcrDataSize, pcrData);
		result |= Trspi_Hash_BOOL(&hashCtx,fAddVersion);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;
		if ((result = secret_PerformAuth_OIAP(hIdentKey, TPM_ORD_Quote2, hPolicy, FALSE,
						      &digest, &privAuth))) {
			return result;
		}
		pPrivAuth = &privAuth;
	} else {
		pPrivAuth = NULL;
	}

	/* Send to TCS */
	if ((result = TCS_API(tspContext)->Quote2(tspContext, tcsKeyHandle, &antiReplay,
						  pcrDataSize, pcrData, fAddVersion, pPrivAuth,
						  &pcrDataOutSize, &pcrDataOut, versionInfoSize,
						  versionInfo, &sigSize, &sig)))
		return result;

#ifdef TSS_DEBUG
	LogDebug("Got TCS Response:");
	LogDebug("		pcrDataOutSize: %u",pcrDataOutSize);
	LogDebug("		pcrDataOut:");
	LogDebugData(pcrDataOutSize,pcrDataOut);
	LogDebug("		versionInfoSize: %u",*versionInfoSize);
	LogDebug("		versionInfo:");
	if (*versionInfoSize >0) 
		LogDebugData(*versionInfoSize,*versionInfo);
	LogDebug("		sigSize: %u",sigSize);
	LogDebug("		sig:");
	LogDebugData(sigSize,sig);
#endif

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Quote2);
		result |= Trspi_HashUpdate(&hashCtx, pcrDataOutSize, pcrDataOut);
		result |= Trspi_Hash_UINT32(&hashCtx,*versionInfoSize);
		if (*versionInfoSize > 0)
			result |= Trspi_HashUpdate(&hashCtx, *versionInfoSize,*versionInfo);
		result |= Trspi_Hash_UINT32(&hashCtx, sigSize);
		result |= Trspi_HashUpdate(&hashCtx, sigSize, sig);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free(pcrDataOut);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			free(sig);
			return result;
		}
		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &privAuth))) {
			free(pcrDataOut);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			free(sig);
			return result;
		}
	}

	/* Set the pcrDataOut back to the TSS */
	if (hPcrComposite){
		TPM_PCR_INFO_SHORT pcrInfo;

		offset = 0;
		if ((result = Trspi_UnloadBlob_PCR_INFO_SHORT(&offset, pcrDataOut, &pcrInfo))) {
			free(pcrDataOut);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			free(sig);
			return result;
		}

		/* Set both digestAtRelease and localityAtRelease */
		if ((result = obj_pcrs_set_locality(hPcrComposite, pcrInfo.localityAtRelease))) {
			free(pcrDataOut);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			free(sig);
			return result;
		}

		if ((result = obj_pcrs_set_digest_at_release(hPcrComposite,
							     pcrInfo.digestAtRelease))) {
			free(pcrDataOut);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			free(sig);
			return result;
		}
	}

	/* generate TPM_QUOTE_INFO2 struct */
	memset(&quoteinfo, 0, sizeof(quoteinfo));
	offset = 0;
	/* 1. Add Structure TAG */
	quoteinfo[offset++] = 0x00;
	quoteinfo[offset++] = (BYTE) TPM_TAG_QUOTE_INFO2;

	/* 2. add "QUT2" */
	quoteinfo[offset++]='Q';
	quoteinfo[offset++]='U';
	quoteinfo[offset++]='T';
	quoteinfo[offset++]='2';

	/* 3. AntiReplay Nonce - add the external data*/
	Trspi_LoadBlob(&offset, TCPA_SHA1_160_HASH_LEN, quoteinfo,
			antiReplay.nonce);
	/* 4. add the infoshort TPM_PCR_INFO_SHORT data */
	Trspi_LoadBlob(&offset,pcrDataOutSize,quoteinfo,pcrDataOut);
	free(pcrDataOut);

	if (fAddVersion)
		Trspi_LoadBlob(&offset,*versionInfoSize,quoteinfo,*versionInfo);
	else {
		/* versionInfo was not allocated and versionInfoSize has invalid value */
		*versionInfoSize = 0;
		*versionInfo = NULL;
	}

	LogDebug("TPM_QUOTE_INFO2 data: ");
	LogDebugData(offset,quoteinfo);

	if (pValidationData == NULL) {
		if ((result = Trspi_Hash(TSS_HASH_SHA1, offset, quoteinfo, digest.digest))) {
			free(sig);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			return result;
		}
		if ((result = __tspi_rsa_verify(hIdentKey,TSS_HASH_SHA1,sizeof(digest.digest),
					 digest.digest, sigSize, sig))) {
			free(sig);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			return TSPERR(TSS_E_VERIFICATION_FAILED);
		}
		free(sig);
	} else {
		pValidationData->rgbValidationData = calloc_tspi(tspContext, sigSize);
		if (pValidationData->rgbValidationData == NULL) {
			LogError("malloc of %u bytes failed.", sigSize);
			free(sig);
			if (*versionInfoSize > 0)
				free(*versionInfo);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		pValidationData->ulValidationDataLength = sigSize;
		memcpy(pValidationData->rgbValidationData, sig, sigSize);
		free(sig);

		pValidationData->rgbData = calloc_tspi(tspContext, offset);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %" PRIu64 " bytes failed.", offset);
			free_tspi(tspContext, pValidationData->rgbValidationData);
			pValidationData->rgbValidationData = NULL;
			pValidationData->ulValidationDataLength = 0;
			if (*versionInfoSize > 0)
				free(*versionInfo);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		pValidationData->ulDataLength = (UINT32)offset;
		memcpy(pValidationData->rgbData, quoteinfo, offset);
	}


	if(*versionInfoSize > 0) {
		if(fAddVersion) {
			/* tag versionInfo so that it can be free'd by the app through Tspi_Context_FreeMemory */
			if ((result = __tspi_add_mem_entry(tspContext, *versionInfo))) {
				free_tspi(tspContext, pValidationData->rgbValidationData);
				pValidationData->rgbValidationData = NULL;
				pValidationData->ulValidationDataLength = 0;
				free_tspi(tspContext, pValidationData->rgbData);
				pValidationData->rgbData = NULL;
				pValidationData->ulDataLength = 0;
				free(*versionInfo);
				return result;
			}
		}
		else {
			free(*versionInfo);
		}
        } 

	LogDebug("Tspi_TPM_Quote2 End");
	return TSS_SUCCESS;
}
