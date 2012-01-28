
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
Tspi_Data_Bind(TSS_HENCDATA hEncData,	/* in */
	       TSS_HKEY hEncKey,	/* in */
	       UINT32 ulDataLength,	/* in */
	       BYTE *rgbDataToBind)	/* in */
{
	UINT32 encDataLength;
	BYTE encData[256];
	BYTE *keyData;
	UINT32 keyDataLength;
	TCPA_BOUND_DATA boundData;
	UINT64 offset;
	BYTE bdblob[256];
	TCPA_RESULT result;
	TSS_KEY keyContainer;
	TSS_HCONTEXT tspContext;

	if (rgbDataToBind == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_encdata(hEncData))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = obj_rsakey_get_tsp_context(hEncKey, &tspContext)))
		return result;

	/* XXX Just get the pubkey here */
	if ((result = obj_rsakey_get_blob(hEncKey, &keyDataLength, &keyData)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, keyData, &keyContainer))) {
		free_tspi(tspContext, keyData);
		return result;
	}
	free_tspi(tspContext, keyData);

	if (keyContainer.keyUsage != TPM_KEY_BIND &&
	    keyContainer.keyUsage != TPM_KEY_LEGACY) {
		result = TSPERR(TSS_E_INVALID_KEYUSAGE);
		goto done;
	}

	if (keyContainer.pubKey.keyLength < ulDataLength) {
		result = TSPERR(TSS_E_ENC_INVALID_LENGTH);
		goto done;
	}

	if (keyContainer.algorithmParms.encScheme == TCPA_ES_RSAESPKCSv15 &&
	    keyContainer.keyUsage == TPM_KEY_LEGACY) {
		if ((result = Trspi_RSA_PKCS15_Encrypt(rgbDataToBind, ulDataLength, encData,
						       &encDataLength, keyContainer.pubKey.key,
						       keyContainer.pubKey.keyLength)))
			goto done;
	} else if (keyContainer.algorithmParms.encScheme == TCPA_ES_RSAESPKCSv15 &&
		   keyContainer.keyUsage == TPM_KEY_BIND) {
		boundData.payload = TCPA_PT_BIND;

		memcpy(&boundData.ver, &VERSION_1_1, sizeof(TCPA_VERSION));

		boundData.payloadData = malloc(ulDataLength);
		if (boundData.payloadData == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(boundData.payloadData, rgbDataToBind, ulDataLength);

		offset = 0;
		Trspi_LoadBlob_BOUND_DATA(&offset, boundData, ulDataLength, bdblob);

		if ((result = Trspi_RSA_PKCS15_Encrypt(bdblob, offset, encData,
						       &encDataLength, keyContainer.pubKey.key,
						       keyContainer.pubKey.keyLength))) {
			free(boundData.payloadData);
			goto done;
		}
		free(boundData.payloadData);
	} else {
		boundData.payload = TCPA_PT_BIND;

		memcpy(&boundData.ver, &VERSION_1_1, sizeof(TCPA_VERSION));

		boundData.payloadData = malloc(ulDataLength);
		if (boundData.payloadData == NULL) {
			LogError("malloc of %u bytes failed.", ulDataLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(boundData.payloadData, rgbDataToBind, ulDataLength);

		offset = 0;
		Trspi_LoadBlob_BOUND_DATA(&offset, boundData, ulDataLength, bdblob);

		if ((result = Trspi_RSA_Encrypt(bdblob, offset, encData, &encDataLength,
						keyContainer.pubKey.key,
						keyContainer.pubKey.keyLength))) {
			free(boundData.payloadData);
			goto done;
		}

		free(boundData.payloadData);
	}

	if ((result = obj_encdata_set_data(hEncData, encDataLength, encData))) {
		LogError("Error in calling SetAttribData on the encrypted data object.");
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}
done:
	free_key_refs(&keyContainer);
	return result;
}

TSS_RESULT
Tspi_Data_Unbind(TSS_HENCDATA hEncData,		/* in */
		 TSS_HKEY hKey,			/* in */
		 UINT32 * pulUnboundDataLength,	/* out */
		 BYTE ** prgbUnboundData)	/* out */
{
	TCPA_RESULT result;
	TPM_AUTH privAuth;
	TCPA_DIGEST digest;
	TSS_HPOLICY hPolicy;
	BYTE *encData;
	UINT32 encDataSize;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_BOOL usesAuth;
	TPM_AUTH *pPrivAuth;
        TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;

	if (pulUnboundDataLength == NULL || prgbUnboundData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_encdata_get_tsp_context(hEncData, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE, &hPolicy, &usesAuth)))
		return result;

	if ((result = obj_encdata_get_data(hEncData, &encDataSize, &encData)))
		return result == (TSS_E_INVALID_OBJ_ACCESS | TSS_LAYER_TSP) ?
		       TSPERR(TSS_E_ENC_NO_DATA) :
		       result;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
		return result;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_UnBind);
		result |= Trspi_Hash_UINT32(&hashCtx, encDataSize);
		result |= Trspi_HashUpdate(&hashCtx, encDataSize, encData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_UnBind, hPolicy, FALSE, &digest,
						      &privAuth)))
			return result;
		pPrivAuth = &privAuth;
	} else {
		pPrivAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->UnBind(tspContext, tcsKeyHandle, encDataSize, encData,
						  pPrivAuth, pulUnboundDataLength,
						  prgbUnboundData)))
		return result;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_UnBind);
		result |= Trspi_Hash_UINT32(&hashCtx, *pulUnboundDataLength);
		result |= Trspi_HashUpdate(&hashCtx, *pulUnboundDataLength, *prgbUnboundData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto error;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &privAuth)))
			goto error;
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbUnboundData)))
		goto error;

	return TSS_SUCCESS;
error:
	free(*prgbUnboundData);
	*prgbUnboundData = NULL;
	*pulUnboundDataLength = 0;
	return result;
}

