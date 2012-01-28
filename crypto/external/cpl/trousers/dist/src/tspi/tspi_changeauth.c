
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
#include <time.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "authsess.h"

TSS_RESULT
Tspi_ChangeAuth(TSS_HOBJECT hObjectToChange,	/* in */
		TSS_HOBJECT hParentObject,	/* in */
		TSS_HPOLICY hNewPolicy)		/* in */
{
	UINT32 keyToChangeHandle;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;

	if ((result = obj_policy_get_tsp_context(hNewPolicy, &tspContext)))
		return result;

	/* if the object to change is the TPM object, then the parent should
	 * be NULL.  If the object to change is not the TPM, then the parent
	 * object must be either an rsakey or the TPM */
	if (obj_is_tpm(hObjectToChange)) {
		if (hParentObject != NULL_HOBJECT)
			return TSPERR(TSS_E_BAD_PARAMETER);
	} else if (!obj_is_rsakey(hParentObject) && !obj_is_tpm(hParentObject)) {
		return TSPERR(TSS_E_INVALID_HANDLE);
	}

	if (obj_is_tpm(hObjectToChange)) {
		if ((result = changeauth_owner(tspContext, hObjectToChange, NULL_HTPM, hNewPolicy)))
			return result;
	} else if (obj_is_rsakey(hObjectToChange)) {
		if ((result = obj_rsakey_get_tcs_handle(hObjectToChange, &keyToChangeHandle)))
			return result;

		if (keyToChangeHandle == TPM_KEYHND_SRK) {
			if ((result = changeauth_srk(tspContext, hObjectToChange, hParentObject,
						     hNewPolicy)))
				return result;
		} else {
			if ((result = changeauth_key(tspContext, hObjectToChange, hParentObject,
						     hNewPolicy)))
				return result;
		}
	} else if (obj_is_encdata(hObjectToChange)) {
		if ((result = changeauth_encdata(tspContext, hObjectToChange, hParentObject,
						 hNewPolicy)))
			return result;
	} else if (obj_is_policy(hObjectToChange) || obj_is_hash(hObjectToChange) ||
		   obj_is_pcrs(hObjectToChange) || obj_is_context(hObjectToChange)) {
		return TSPERR(TSS_E_BAD_PARAMETER);
	} else {
		return TSPERR(TSS_E_INVALID_HANDLE);
	}

	if ((result = obj_policy_set_type(hNewPolicy, TSS_POLICY_USAGE)))
		return result;

	return Tspi_Policy_AssignToObject(hNewPolicy, hObjectToChange);

}

TSS_RESULT
Tspi_ChangeAuthAsym(TSS_HOBJECT hObjectToChange,	/* in */
		    TSS_HOBJECT hParentObject,		/* in */
		    TSS_HKEY hIdentKey,			/* in */
		    TSS_HPOLICY hNewPolicy)		/* in */
{
#if 0
	TPM_AUTH auth;
	UINT64 offset;
	BYTE hashBlob[0x1000];
	TCPA_DIGEST digest;
	TCPA_RESULT result;
	UINT32 keyHandle;
	UINT32 idHandle;
	TSS_HPOLICY hPolicy;
	TSS_HPOLICY hParentPolicy;
	UINT32 keyToChangeHandle;
	TCPA_NONCE antiReplay;
	UINT32 bytesRequested;
	UINT64 tempSize;
	BYTE tempKey[512];
	TCPA_KEY_PARMS keyParms;
	/* XXX Wow... */
	BYTE ephParms[] = { 0, 0, 0x08, 0, 0, 0, 0, 0x02, 0, 0, 0, 0 };
	UINT32 KeySizeOut;
	BYTE *KeyDataOut;
	UINT32 CertifyInfoSize;
	BYTE *CertifyInfo;
	UINT32 sigSize;
	BYTE *sig;
	UINT32 ephHandle;
	TPM_CHANGEAUTH_VALIDATE caValidate;
	TCPA_SECRET newSecret, oldSecret;
	BYTE seed[20];
	BYTE a1[256];
	UINT32 a1Size;
	TSS_KEY ephemeralKey;
	TCPA_DIGEST newAuthLink;
	UINT32 encObjectSize;
	BYTE *encObject = NULL;
	UINT32 encDataSizeOut;
	BYTE *encDataOut;
	TCPA_NONCE saltNonce;
	TCPA_DIGEST changeProof;
	TSS_HPOLICY hOldPolicy;
	UINT32 caValidSize;
	UINT32 keyObjectSize;
	BYTE *keyObject;
	TSS_KEY keyContainer;
	TCPA_STORED_DATA dataContainer;
	BYTE *dataObject;
	UINT32 dataObjectSize;
	UINT16 entityType;
	TSS_BOOL useAuth = TRUE; // XXX
	TPM_AUTH *pAuth;
	BYTE dataBlob[1024];
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;

	if ((result = obj_policy_get_tsp_context(hNewPolicy, &tspContext)))
		return result;

	/*  grab all of the needed handles */
	if ((result = obj_rsakey_get_tcs_handle(hIdentKey, &idHandle)))
		return result;

	/*  get the secret for the parent */
	if ((result = obj_rsakey_get_policy(hIdentKey, TSS_POLICY_USAGE, &hPolicy, &useAuth)))
		return result;

	/*  get the parent secret */
	if ((result = Tspi_GetPolicyObject(hParentObject, TSS_POLICY_USAGE, &hParentPolicy)))
		return result;

	if (!obj_is_rsakey(hParentObject) && !obj_is_tpm(hParentObject))
		return TSPERR(TSS_E_INVALID_HANDLE);

	/*  get the keyObject  */
	if ((result = obj_rsakey_get_tcs_handle(hParentObject, &keyHandle)))
		return result;

	if (obj_is_rsakey(hObjectToChange) ||
	    obj_is_encdata(hObjectToChange)) {

		if ((result = obj_rsakey_get_tcs_handle(hObjectToChange, &keyToChangeHandle)))
			return result;

		if (keyToChangeHandle == TPM_KEYHND_SRK) {
			return TSPERR(TSS_E_BAD_PARAMETER);
		} else {
			/*  generate container for ephemeral key */
			keyParms.algorithmID = 1;	/* rsa */
			keyParms.encScheme = 3;
			keyParms.sigScheme = 1;
			keyParms.parmSize = 12;
			keyParms.parms = malloc(12);
			if (keyParms.parms == NULL) {
				LogError("malloc of %d bytes failed.", 12);
				return TSPERR(TSS_E_OUTOFMEMORY);
			}
			memcpy(keyParms.parms, ephParms, 12);

			tempSize = 0;
			Trspi_LoadBlob_KEY_PARMS(&tempSize, tempKey, &keyParms);

			/*  generate antireplay nonce */
			bytesRequested = 20;
			if ((result = get_local_random(tspContext, FALSE, bytesRequested,
						       (BYTE **)antiReplay.nonce)))
				return result;

			/* caluculate auth data */
			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthAsymStart);
			result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN,
						   antiReplay.nonce);
			result |= Trspi_Hash_KEY_PARMS(&hashCtx, &keyParms);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				return result;

			if (useAuth) {
				if ((result = secret_PerformAuth_OIAP(hIdentKey,
								      TPM_ORD_ChangeAuthAsymStart,
								      hPolicy, FALSE, &digest,
								      &auth)))
					return result;

				pAuth = &auth;
			} else {
				pAuth = NULL;
			}

			if ((result = TCSP_ChangeAuthAsymStart(tspContext, idHandle, antiReplay,
							       tempSize, tempKey, pAuth,
							       &KeySizeOut, &KeyDataOut,
							       &CertifyInfoSize, &CertifyInfo,
							       &sigSize, &sig, &ephHandle)))
				return result;

			/* Validate the Auth's */
			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, result);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthAsymStart);
			result |= Trspi_HashUpdate(&hashCtx, CertifyInfoSize, CertifyInfo);
			result |= Trspi_Hash_UINT32(&hashCtx, sigSize);
			result |= Trspi_HashUpdate(&hashCtx, sigSize, sig);
			result |= Trspi_Hash_UINT32(&hashCtx, ephHandle);
			result |= Trspi_HashUpdate(&hashCtx, KeySizeOut, KeyDataOut);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				return result;

			if (useAuth) {
				if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest,
									    &auth)))
					return result;
			}

			/*  generate random data for asymfinish */
			if ((result = get_local_random(tspContext, FALSE, bytesRequested,
						       (BYTE **)&caValidate.n1.nonce)))
				return result;

			if ((result = get_local_random(tspContext, FALSE, bytesRequested,
						       (BYTE **)&antiReplay.nonce)))
				return result;

			if ((result = get_local_random(tspContext, FALSE, bytesRequested,
						       (BYTE **)&seed)))
				return result;

			if ((result = Tspi_GetPolicyObject(hObjectToChange, TSS_POLICY_USAGE,
							  &hOldPolicy)))
				return result;

			if ((result = obj_policy_get_secret(hNewPolicy, TR_SECRET_CTX_NEW,
							    &newSecret)))
				return result;
			if ((result = obj_policy_get_secret(hOldPolicy, TR_SECRET_CTX_NOT_NEW,
							    &oldSecret)))
				return result;

			/* Encrypt the ChangeAuthValidate structure with the
			 * ephemeral key */

			memcpy(caValidate.newAuthSecret.authdata, newSecret.authdata, 20);

			offset = 0;
			Trspi_LoadBlob_CHANGEAUTH_VALIDATE(&offset, hashBlob, &caValidate);
			caValidSize = offset;

			offset = 0;
			if ((result = UnloadBlob_TSS_KEY(&offset, KeyDataOut, &ephemeralKey)))
				return result;

			Trspi_RSA_Encrypt(hashBlob, caValidSize, a1, &a1Size,
				       ephemeralKey.pubKey.key,
				       ephemeralKey.pubKey.keyLength);

			free_key_refs(&ephemeralKey);

			Trspi_HMAC(TSS_HASH_SHA1, 20, oldSecret.authdata,
					 20, newSecret.authdata,
					 newAuthLink.digest);

			if (obj_is_rsakey(hObjectToChange)) {
				if ((result = obj_rsakey_get_blob(hObjectToChange,
						   &keyObjectSize, &keyObject)))
					return result;

				memset(&keyContainer, 0, sizeof(TSS_KEY));

				offset = 0;
				if ((result = UnloadBlob_TSS_KEY(&offset,
								 keyObject,
								 &keyContainer)))
					return result;

				encObjectSize = keyContainer.encSize;
				encObject = malloc(encObjectSize);
				if (encObject == NULL) {
					LogError("malloc of %d bytes failed.",
							encObjectSize);
					free_key_refs(&keyContainer);
					return TSPERR(TSS_E_OUTOFMEMORY);
				}
				memcpy(encObject, keyContainer.encData,
						encObjectSize);
				entityType = TCPA_ET_KEY;
			} else {
				if ((result = obj_encdata_get_data(hObjectToChange,
						   &dataObjectSize, &dataObject)))
					return result;

				offset = 0;
				if ((result = Trspi_UnloadBlob_STORED_DATA(&offset,
									   dataObject,
									   &dataContainer)))
					return result;

				encObjectSize = dataContainer.encDataSize;
				encObject = malloc(encObjectSize);
				if (encObject == NULL) {
					LogError("malloc of %d bytes failed.", encObjectSize);
					free(dataContainer.sealInfo);
					free(dataContainer.encData);
					return TSPERR(TSS_E_OUTOFMEMORY);
				}
				memcpy(encObject, dataContainer.encData,
						encObjectSize);
				entityType = TCPA_ET_DATA;
			}

			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthAsymFinish);
			result |= Trspi_Hash_UINT16(&hashCtx, entityType);
			result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN,
						   newAuthLink.digest);
			result |= Trspi_Hash_UINT32(&hashCtx, a1Size);
			result |= Trspi_HashUpdate(&hashCtx, a1Size, a1);
			result |= Trspi_Hash_UINT32(&hashCtx, encObjectSize);
			result |= Trspi_HashUpdate(&hashCtx, encObjectSize, encObject);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				return result;

			if (useAuth) {
				if ((result = secret_PerformAuth_OIAP(hParentObject,
								      TPM_ORD_ChangeAuthAsymFinish,
								      hParentPolicy, FALSE,
								      &digest, &auth))) {
					free(encObject);
					free_key_refs(&keyContainer);
					return result;
				}
				pAuth = &auth;
			} else {
				pAuth = NULL;
			}

			if ((result = TCSP_ChangeAuthAsymFinish(tspContext, keyHandle, ephHandle,
							       entityType, newAuthLink, a1Size, a1,
							       encObjectSize, encObject, pAuth,
							       &encDataSizeOut, &encDataOut,
							       &saltNonce, &changeProof))) {
				free_key_refs(&keyContainer);
				free(encObject);
				return result;
			}

			/* ---  Validate the Auth's */
			result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
			result |= Trspi_Hash_UINT32(&hashCtx, result);
			result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ChangeAuthAsymFinish);
			result |= Trspi_Hash_UINT32(&hashCtx, encDataSizeOut);
			result |= Trspi_HashUpdate(&hashCtx, encDataSizeOut, encDataOut);
			result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN,
						   saltNonce.nonce);
			result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN,
						   changeProof.digest);
			if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
				return result;

			if (useAuth) {
				if ((result = obj_policy_validate_auth_oiap(hParentPolicy,
									&digest,
									&auth))) {
					free_key_refs(&keyContainer);
					free(encObject);
					return result;
				}
			}

			if (entityType == TCPA_ET_KEY ||
			    entityType == TCPA_ET_KEYHANDLE) {
				memcpy(keyContainer.encData, encDataOut, encDataSizeOut);
				keyContainer.encSize = encDataSizeOut;

				offset = 0;
				LoadBlob_TSS_KEY(&offset, keyObject, &keyContainer);
				free_key_refs(&keyContainer);
				if ((result = obj_rsakey_set_tcpakey(hObjectToChange, offset,
								     keyObject))) {
					free(encObject);
					return result;
				}
			}

			if (entityType == TCPA_ET_DATA) {
				memcpy(dataContainer.encData, encDataOut,
						encDataSizeOut);
				dataContainer.encDataSize = encDataSizeOut;

				offset = 0;
				Trspi_LoadBlob_STORED_DATA(&offset, dataBlob,
							   &dataContainer);
				free(dataContainer.sealInfo);
				free(dataContainer.encData);
				obj_encdata_set_data(hObjectToChange,
						   offset, dataBlob);
			}
		}
	} else
		return TSPERR(TSS_E_BAD_PARAMETER);

	free(encObject);

	return Tspi_Policy_AssignToObject(hNewPolicy, hObjectToChange);
#else
	return TSPERR(TSS_E_NOTIMPL);
#endif
}

