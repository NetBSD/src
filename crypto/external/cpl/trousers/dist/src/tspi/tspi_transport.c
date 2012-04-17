
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
Tspi_Context_SetTransEncryptionKey(TSS_HCONTEXT hContext,	/* in */
				   TSS_HKEY     hIdentKey)	/* in */
{
	if (!obj_is_rsakey(hIdentKey))
		return TSPERR(TSS_E_INVALID_HANDLE);

	return obj_context_set_transport_key(hContext, hIdentKey);
}

TSS_RESULT
Tspi_Context_CloseSignTransport(TSS_HCONTEXT    hContext,		/* in */
				TSS_HKEY        hSigningKey,		/* in */
				TSS_VALIDATION* pValidationData)	/* in, out */
{
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	TSS_BOOL usesAuth;
	UINT32 sigLen;
	BYTE *sig;
	UINT64 offset;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST digest;
	TPM_SIGN_INFO signInfo;

	if (!obj_is_context(hContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = obj_rsakey_get_policy(hSigningKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
		return result;

	if (pValidationData) {
		if (pValidationData->ulExternalDataLength != sizeof(TPM_NONCE))
			return TSPERR(TSS_E_BAD_PARAMETER);

		memcpy(signInfo.replay.nonce, pValidationData->rgbExternalData, sizeof(TPM_NONCE));
	} else {
		if ((result = get_local_random(hContext, FALSE, sizeof(TPM_NONCE),
					       (BYTE **)&signInfo.replay.nonce)))
			return result;
	}

	/* the transport sessions properties are kept in the context object itself, so just pass
	 * in what this function provides and let it call ReleaseTransportSigned */
	if ((result = obj_context_transport_close(hContext, hSigningKey, hPolicy, usesAuth,
						  &signInfo, &sigLen, &sig)))
		return result;

	/* inside obj_context_transport_close we set up all the fields of the sign info structure
	 * other than the tag and 'fixed' */
	signInfo.tag = TPM_TAG_SIGNINFO;
	signInfo.fixed[0] = 'T';
	signInfo.fixed[1] = 'R';
	signInfo.fixed[2] = 'A';
	signInfo.fixed[3] = 'N';

	/* hash the sign info struct for use in verifying the TPM's signature */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_SIGN_INFO(&hashCtx, &signInfo);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		free(sig);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	offset = 0;
	if (pValidationData) {
		/* tag the returned allocated memory as alloc'd by the TSP */
		if ((result = __tspi_add_mem_entry(hContext, sig))) {
			free(sig);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		pValidationData->rgbValidationData = sig;
		pValidationData->ulValidationDataLength = sigLen;

		/* passing a NULL blob here puts the exact size of TPM_SIGN_INFO into offset */
		Trspi_LoadBlob_SIGN_INFO(&offset, NULL, &signInfo);
		pValidationData->rgbData = calloc_tspi(hContext, offset);
		if (pValidationData->rgbData == NULL) {
			LogError("malloc of %" PRIu64 " bytes failed.", offset);
			free_tspi(hContext, pValidationData->rgbValidationData);
			pValidationData->rgbValidationData = NULL;
			pValidationData->ulValidationDataLength = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		pValidationData->ulDataLength = (UINT32)offset;

		offset = 0;
		Trspi_LoadBlob_SIGN_INFO(&offset, pValidationData->rgbData, &signInfo);
	} else
		result = __tspi_rsa_verify(hSigningKey, TSS_HASH_SHA1, sizeof(TPM_DIGEST), digest.digest,
				    sigLen, sig);

	return result;
}
