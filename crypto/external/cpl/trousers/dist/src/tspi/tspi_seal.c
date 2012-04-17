
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
#include "authsess.h"


TSS_RESULT
Tspi_Data_Seal(TSS_HENCDATA hEncData,	/* in */
	       TSS_HKEY hEncKey,	/* in */
	       UINT32 ulDataLength,	/* in */
	       BYTE * rgbDataToSeal,	/* in */
	       TSS_HPCRS hPcrComposite)	/* in */
{
	TPM_DIGEST digest;
	TSS_RESULT result;
	TSS_HPOLICY hPolicy, hEncPolicy;
	BYTE *encData = NULL;
	BYTE *pcrData = NULL;
	UINT32 encDataSize;
	UINT32 pcrDataSize;
	UINT32 pcrInfoType = TSS_PCRS_STRUCT_DEFAULT;
	UINT32 sealOrdinal = TPM_ORD_Seal;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;
	BYTE *sealData = NULL;
	struct authsess *xsap = NULL;
#ifdef TSS_BUILD_SEALX
	UINT32 protectMode;
#endif

	if (rgbDataToSeal == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_encdata_get_tsp_context(hEncData, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hEncKey, TSS_POLICY_USAGE,
					    &hPolicy, NULL)))
		return result;

	if ((result = obj_encdata_get_policy(hEncData, TSS_POLICY_USAGE,
					     &hEncPolicy)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hEncKey, &tcsKeyHandle)))
		return result;

#ifdef TSS_BUILD_SEALX
	/* Get the TSS_TSPATTRIB_ENCDATASEAL_PROTECT_MODE attribute
	   to determine the seal function to invoke */
	if ((result = obj_encdata_get_seal_protect_mode(hEncData, &protectMode)))
		return result;

	if (protectMode == TSS_TSPATTRIB_ENCDATASEAL_NO_PROTECT) {
		sealOrdinal = TPM_ORD_Seal;
		pcrInfoType = 0;
	} else if (protectMode == TSS_TSPATTRIB_ENCDATASEAL_PROTECT) {
		sealOrdinal = TPM_ORD_Sealx;
		pcrInfoType = TSS_PCRS_STRUCT_INFO_LONG;
	} else
		return TSPERR(TSS_E_INTERNAL_ERROR);
#endif

	/* If PCR's are of interest */
	pcrDataSize = 0;
	if (hPcrComposite) {
		if ((result = obj_pcrs_create_info_type(hPcrComposite, &pcrInfoType, &pcrDataSize,
							&pcrData)))
			return result;
	}

	if ((result = authsess_xsap_init(tspContext, hEncKey, hEncData, TSS_AUTH_POLICY_REQUIRED,
					 sealOrdinal, TPM_ET_KEYHANDLE, &xsap)))
		goto error;

#ifdef TSS_BUILD_SEALX
	if (sealOrdinal == TPM_ORD_Seal)
		sealData = rgbDataToSeal;
	else {
		if ((sealData = (BYTE *)calloc(1, ulDataLength)) == NULL) {
			LogError("malloc of %u bytes failed", ulDataLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}

		if ((result =
		     ((TSS_RESULT (*)(PVOID, TSS_HKEY, TSS_HENCDATA, TSS_ALGORITHM_ID,
		     UINT32, BYTE *, BYTE *, BYTE *, BYTE *, UINT32, BYTE *,
		     BYTE *))xsap->cb_sealx.callback)(xsap->cb_sealx.appData, hEncKey, hEncData,
						      xsap->cb_sealx.alg, sizeof(TPM_NONCE),
						      xsap->auth.NonceEven.nonce,
						      xsap->auth.NonceOdd.nonce,
						      xsap->nonceEvenxSAP.nonce,
						      xsap->nonceOddxSAP.nonce, ulDataLength,
						      rgbDataToSeal, sealData)))
			goto error;
	}
#else
	sealData = rgbDataToSeal;
#endif

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, sealOrdinal);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_Hash_UINT32(&hashCtx, pcrDataSize);
	result |= Trspi_HashUpdate(&hashCtx, pcrDataSize, pcrData);
	result |= Trspi_Hash_UINT32(&hashCtx, ulDataLength);
	result |= Trspi_HashUpdate(&hashCtx, ulDataLength, sealData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		goto error;
	}

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

#ifdef TSS_BUILD_SEALX
	if (sealOrdinal == TPM_ORD_Seal) {
		if ((result = TCS_API(tspContext)->Seal(tspContext, tcsKeyHandle, &xsap->encAuthUse,
							pcrDataSize, pcrData, ulDataLength,
							sealData, xsap->pAuth, &encDataSize,
							&encData))) {
			goto error;
		}
	} else if (sealOrdinal == TPM_ORD_Sealx) {
		if ((result = TCS_API(tspContext)->Sealx(tspContext, tcsKeyHandle, &xsap->encAuthUse,
						    pcrDataSize, pcrData, ulDataLength, sealData,
						    xsap->pAuth, &encDataSize, &encData))) {
			goto error;
		}
	} else {
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto error;
	}
#else
	if ((result = TCS_API(tspContext)->Seal(tspContext, tcsKeyHandle, &xsap->encAuthUse,
						pcrDataSize, pcrData, ulDataLength, sealData,
						xsap->pAuth, &encDataSize, &encData)))
		goto error;
#endif

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, sealOrdinal);
	result |= Trspi_HashUpdate(&hashCtx, encDataSize, encData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_verify(xsap, &digest)))
		goto error;

	/* Need to set the object with the blob and the pcr's */
	if ((result = obj_encdata_set_data(hEncData, encDataSize, encData)))
		goto error;

	if (pcrDataSize)
		result = obj_encdata_set_pcr_info(hEncData, pcrInfoType, pcrData);

error:
	authsess_free(xsap);
	free(encData);
	free(pcrData);
	if (sealData != rgbDataToSeal)
		free(sealData);
	return result;
}

TSS_RESULT
Tspi_Data_Unseal(TSS_HENCDATA hEncData,		/* in */
		 TSS_HKEY hKey,			/* in */
		 UINT32 * pulUnsealedDataLength,/* out */
		 BYTE ** prgbUnsealedData)	/* out */
{
	UINT64 offset;
	TPM_AUTH privAuth2;
	TPM_DIGEST digest;
	TPM_NONCE authLastNonceEven;
	TSS_RESULT result;
	TSS_HPOLICY hPolicy, hEncPolicy;
	TCS_KEY_HANDLE tcsKeyHandle;
        TSS_HCONTEXT tspContext;
	UINT32 ulDataLen, unSealedDataLen;
	BYTE *data = NULL, *unSealedData = NULL, *maskedData;
	UINT16 mask;
	Trspi_HashCtx hashCtx;
	struct authsess *xsap = NULL;

	if (pulUnsealedDataLength == NULL || prgbUnsealedData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_encdata_get_tsp_context(hEncData, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE, &hPolicy, NULL)))
		return result;

	if ((result = obj_encdata_get_policy(hEncData, TSS_POLICY_USAGE, &hEncPolicy)))
		return result;

	if ((result = obj_encdata_get_data(hEncData, &ulDataLen, &data)))
		return result == (TSS_E_INVALID_OBJ_ACCESS | TSS_LAYER_TSP) ?
		       TSPERR(TSS_E_ENC_NO_DATA) :
		       result;

	offset = 0;
	Trspi_UnloadBlob_UINT16(&offset, &mask, data);
	if (mask == TPM_TAG_STORED_DATA12) {
		/* The second UINT16 in a TPM_STORED_DATA12 is the entity type. If its non-zero
		 * then we must unmask the unsealed data after it returns from the TCS */
		Trspi_UnloadBlob_UINT16(&offset, &mask, data);
	} else
		mask = 0;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
		goto error;

	if ((result = authsess_xsap_init(tspContext, hKey, hEncData, TSS_AUTH_POLICY_REQUIRED,
					 TPM_ORD_Unseal, TPM_ET_KEYHANDLE, &xsap)))
		goto error;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Unseal);
	result |= Trspi_HashUpdate(&hashCtx, ulDataLen, data);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

	if ((result = secret_PerformAuth_OIAP(hEncData, TPM_ORD_Unseal, hEncPolicy, FALSE, &digest,
					      &privAuth2)))
		goto error;

	if (mask) {
		/* save off last nonce even to pass to sealx callback */
		   memcpy(authLastNonceEven.nonce, xsap->auth.NonceEven.nonce, sizeof(TPM_NONCE));
	}

	if ((result = TCS_API(tspContext)->Unseal(tspContext, tcsKeyHandle, ulDataLen, data,
						  xsap->pAuth, &privAuth2, &unSealedDataLen,
						  &unSealedData)))
		goto error;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TSS_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Unseal);
	result |= Trspi_Hash_UINT32(&hashCtx, unSealedDataLen);
	result |= Trspi_HashUpdate(&hashCtx, unSealedDataLen, unSealedData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		free(unSealedData);
		goto error;
	}

	if ((result = authsess_xsap_verify(xsap, &digest))) {
		free(unSealedData);
		goto error;
	}

	if ((result = obj_policy_validate_auth_oiap(hEncPolicy, &digest, &privAuth2))) {
		free(unSealedData);
		goto error;
	}

	/* If the data is masked, use the callback set up in authsess_xsap_init */
	if (mask) {
		maskedData = unSealedData;

		if ((unSealedData = calloc_tspi(tspContext, unSealedDataLen)) == NULL) {
			free(maskedData);
			LogError("malloc of %u bytes failed", unSealedDataLen);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}

		/* XXX pass in out saved-off authLastNonceEven. This conflicts with the
		 * description of the rgbNonceEven parameter in the spec, but without it, its not
		 * possible to compute the MGF1 key */
		if ((result =
		     ((TSS_RESULT (*)(PVOID, TSS_HKEY, TSS_HENCDATA, TSS_ALGORITHM_ID,
		       UINT32, BYTE *, BYTE *, BYTE *, BYTE *, UINT32, BYTE *,
		       BYTE *))xsap->cb_sealx.callback)(xsap->cb_sealx.appData, hKey, hEncData,
							 xsap->cb_sealx.alg, sizeof(TPM_NONCE),
							 authLastNonceEven.nonce,
							 xsap->auth.NonceOdd.nonce,
							 xsap->nonceEvenxSAP.nonce,
							 xsap->nonceOddxSAP.nonce,
							 unSealedDataLen, maskedData,
							 unSealedData))) {
			free(maskedData);
			goto error;
		}

		free(maskedData);
	} else {
		if ((result = __tspi_add_mem_entry(tspContext, unSealedData)))
			goto error;
	}

	*pulUnsealedDataLength = unSealedDataLen;
	*prgbUnsealedData = unSealedData;

error:
	authsess_free(xsap);
	if (data)
		free_tspi(tspContext, data);

	return result;
}
