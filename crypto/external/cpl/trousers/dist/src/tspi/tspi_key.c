
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
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "authsess.h"


TSS_RESULT
Tspi_Key_UnloadKey(TSS_HKEY hKey)	/* in */
{
	TSS_HCONTEXT tspContext;
	TCS_KEY_HANDLE hTcsKey;
	TSS_RESULT result;

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &hTcsKey)))
		return result;

	return __tspi_free_resource(tspContext, hTcsKey, TPM_RT_KEY);
}

TSS_RESULT
Tspi_Key_LoadKey(TSS_HKEY hKey,			/* in */
		 TSS_HKEY hUnwrappingKey)	/* in */
{
	TPM_AUTH auth;
	TCPA_DIGEST digest;
	TSS_RESULT result;
	UINT32 keyslot;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hPolicy;
	UINT32 keySize;
	BYTE *keyBlob;
	TCS_KEY_HANDLE tcsKey, tcsParentHandle;
	TSS_BOOL usesAuth;
	TPM_AUTH *pAuth;
	Trspi_HashCtx hashCtx;
	TPM_COMMAND_CODE ordinal;

	if (!obj_is_rsakey(hUnwrappingKey))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if ((result = obj_context_get_loadkey_ordinal(tspContext, &ordinal)))
		return result;

	if ((result = obj_rsakey_get_blob(hKey, &keySize, &keyBlob)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hUnwrappingKey, &tcsParentHandle)))
		return result;

	if ((result = obj_rsakey_get_policy(hUnwrappingKey, TSS_POLICY_USAGE, &hPolicy,
					    &usesAuth))) {
		free_tspi(tspContext, keyBlob);
		return result;
	}

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
		result |= Trspi_HashUpdate(&hashCtx, keySize, keyBlob);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free_tspi(tspContext, keyBlob);
			return result;
		}

		if ((result = secret_PerformAuth_OIAP(hUnwrappingKey, ordinal, hPolicy, FALSE,
						      &digest, &auth))) {
			free_tspi(tspContext, keyBlob);
			return result;
		}
		pAuth = &auth;
	} else {
		pAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->LoadKeyByBlob(tspContext, tcsParentHandle, keySize,
							 keyBlob, pAuth, &tcsKey, &keyslot))) {
		free_tspi(tspContext, keyBlob);
		return result;
	}

	free_tspi(tspContext, keyBlob);

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
		if (ordinal == TPM_ORD_LoadKey)
			result |= Trspi_Hash_UINT32(&hashCtx, keyslot);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth)))
			return result;
	}

	return obj_rsakey_set_tcs_handle(hKey, tcsKey);
}

TSS_RESULT
Tspi_Key_GetPubKey(TSS_HKEY hKey,		/* in */
		   UINT32 * pulPubKeyLength,	/* out */
		   BYTE ** prgbPubKey)		/* out */
{
	TPM_AUTH auth;
	TPM_AUTH *pAuth;
	TCPA_DIGEST digest;
	TCPA_RESULT result;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hPolicy;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_BOOL usesAuth;
	Trspi_HashCtx hashCtx;

	if (pulPubKeyLength == NULL || prgbPubKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE,
					    &hPolicy, &usesAuth)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
		return result;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetPubKey);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_GetPubKey, hPolicy, FALSE,
						      &digest, &auth)))
			return result;
		pAuth = &auth;
	} else {
		pAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->GetPubKey(tspContext, tcsKeyHandle, pAuth,
						     pulPubKeyLength, prgbPubKey)))
		return result;

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_GetPubKey);
		result |= Trspi_HashUpdate(&hashCtx, *pulPubKeyLength, *prgbPubKey);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto error;

		/* goto error here since prgbPubKey has been set */
		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth)))
			goto error;
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbPubKey)))
		goto error;

	if (tcsKeyHandle == TPM_KEYHND_SRK)
		obj_rsakey_set_pubkey(hKey, TRUE, *prgbPubKey);

	return TSS_SUCCESS;
error:
	free(*prgbPubKey);
	*prgbPubKey = NULL;
	*pulPubKeyLength = 0;
	return result;
}

TSS_RESULT
Tspi_Key_CreateKey(TSS_HKEY hKey,		/* in */
		   TSS_HKEY hWrappingKey,	/* in */
		   TSS_HPCRS hPcrComposite)	/* in, may be NULL */
{
#ifdef TSS_BUILD_CMK
	UINT32 blobSize;
	BYTE *blob;
	TSS_BOOL isCmk = FALSE;
	TPM_HMAC msaApproval;
	TPM_DIGEST msaDigest;
#endif
	TCPA_DIGEST digest;
	TCPA_RESULT result;
	TCS_KEY_HANDLE parentTCSKeyHandle;
	BYTE *keyBlob = NULL;
	UINT32 keySize;
	UINT32 newKeySize;
	BYTE *newKey = NULL;
	UINT32 ordinal = TPM_ORD_CreateWrapKey;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;
	struct authsess *xsap = NULL;

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if (hPcrComposite) {
		/* its possible that hPcrComposite could be a bad handle here,
		 * or that no indices of it are yet set, which would throw
		 * internal error. Blanket both those codes with bad
		 * parameter to help the user out */
		if ((result = obj_rsakey_set_pcr_data(hKey, hPcrComposite)))
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if ((result = obj_rsakey_get_tcs_handle(hWrappingKey, &parentTCSKeyHandle)))
		return result;

	if ((result = obj_rsakey_get_blob(hKey, &keySize, &keyBlob)))
		return result;

#ifdef TSS_BUILD_CMK
	isCmk = obj_rsakey_is_cmk(hKey);
	if (isCmk) {
		if ((result = obj_rsakey_get_msa_approval(hKey, &blobSize, &blob)))
			goto done;
		memcpy(msaApproval.digest, blob, sizeof(msaApproval.digest));
		free_tspi(tspContext, blob);

		if ((result = obj_rsakey_get_msa_digest(hKey, &blobSize, &blob)))
			goto done;
		memcpy(msaDigest.digest, blob, sizeof(msaDigest.digest));
		free_tspi(tspContext, blob);

		ordinal = TPM_ORD_CMK_CreateKey;
	}
#endif

	if ((result = authsess_xsap_init(tspContext, hWrappingKey, hKey, TSS_AUTH_POLICY_REQUIRED,
					 ordinal, TPM_ET_KEYHANDLE, &xsap)))
		return result;

	/* Setup the Hash Data for the HMAC */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
#ifdef TSS_BUILD_CMK
	if (isCmk) {
		result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
		result |= Trspi_HashUpdate(&hashCtx, keySize, keyBlob);
		result |= Trspi_Hash_HMAC(&hashCtx, msaApproval.digest);
		result |= Trspi_Hash_DIGEST(&hashCtx, msaDigest.digest);
	} else {
#endif
		result |= Trspi_Hash_DIGEST(&hashCtx, xsap->encAuthUse.authdata);
		result |= Trspi_Hash_DIGEST(&hashCtx, xsap->encAuthMig.authdata);
		result |= Trspi_HashUpdate(&hashCtx, keySize, keyBlob);
#ifdef TSS_BUILD_CMK
	}
#endif
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto done;

	/* Now call the function */
#ifdef TSS_BUILD_CMK
	if (isCmk) {
		if ((newKey = malloc(keySize)) == NULL) {
			LogError("malloc of %u bytes failed.", keySize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(newKey, keyBlob, keySize);
		newKeySize = keySize;

		if ((result = RPC_CMK_CreateKey(tspContext, parentTCSKeyHandle,
						(TPM_ENCAUTH *)&xsap->encAuthUse,
						&msaApproval, &msaDigest, &newKeySize, &newKey,
						xsap->pAuth)))
			goto done;
	} else {
#endif
		if ((result = TCS_API(tspContext)->CreateWrapKey(tspContext, parentTCSKeyHandle,
								 (TPM_ENCAUTH *)&xsap->encAuthUse,
								 (TPM_ENCAUTH *)&xsap->encAuthMig,
								 keySize, keyBlob, &newKeySize,
								 &newKey, xsap->pAuth)))
			goto done;
#ifdef TSS_BUILD_CMK
	}
#endif

	/* Validate the Authorization before using the new key */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
	result |= Trspi_HashUpdate(&hashCtx, newKeySize, newKey);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (authsess_xsap_verify(xsap, &digest)) {
		result = TSPERR(TSS_E_TSP_AUTHFAIL);
		goto done;
	}

	/* Push the new key into the existing object */
	result = obj_rsakey_set_tcpakey(hKey, newKeySize, newKey);

done:
	authsess_free(xsap);
	free_tspi(tspContext, keyBlob);
	free(newKey);

	return result;
}

TSS_RESULT
Tspi_Key_WrapKey(TSS_HKEY hKey,			/* in */
		 TSS_HKEY hWrappingKey,		/* in */
		 TSS_HPCRS hPcrComposite)	/* in, may be NULL */
{
	TSS_HPOLICY hUsePolicy, hMigPolicy;
	TCPA_SECRET usage, migration;
	TSS_RESULT result;
	BYTE *keyPrivBlob = NULL, *wrappingPubKey = NULL, *keyBlob = NULL;
	UINT32 keyPrivBlobLen, wrappingPubKeyLen, keyBlobLen;
	BYTE newPrivKey[214]; /* its not magic, see TPM 1.1b spec p.71 */
	BYTE encPrivKey[256];
	UINT32 newPrivKeyLen = 214, encPrivKeyLen = 256;
	UINT64 offset;
	TSS_KEY keyContainer;
	TCPA_DIGEST digest;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;

	if ((result = obj_rsakey_get_tsp_context(hKey, &tspContext)))
		return result;

	if (hPcrComposite) {
		if ((result = obj_rsakey_set_pcr_data(hKey, hPcrComposite)))
			return result;
	}

	/* get the key to be wrapped's private key */
	if ((result = obj_rsakey_get_priv_blob(hKey, &keyPrivBlobLen, &keyPrivBlob)))
		goto done;

	/* get the key to be wrapped's blob */
	if ((result = obj_rsakey_get_blob(hKey, &keyBlobLen, &keyBlob)))
		goto done;

	/* get the wrapping key's public key */
	if ((result = obj_rsakey_get_modulus(hWrappingKey, &wrappingPubKeyLen, &wrappingPubKey)))
		goto done;

	/* get the key to be wrapped's usage policy */
	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_USAGE, &hUsePolicy, NULL)))
		goto done;

	if ((result = obj_rsakey_get_policy(hKey, TSS_POLICY_MIGRATION, &hMigPolicy, NULL)))
		goto done;

	if ((result = obj_policy_get_secret(hUsePolicy, TR_SECRET_CTX_NEW, &usage)))
		goto done;

	if ((result = obj_policy_get_secret(hMigPolicy, TR_SECRET_CTX_NEW, &migration)))
		goto done;

	memset(&keyContainer, 0, sizeof(TSS_KEY));

	/* unload the key to be wrapped's blob */
	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, keyBlob, &keyContainer)))
		return result;

	/* load the key's attributes into an object and get its hash value */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Hash_TSS_PRIVKEY_DIGEST(&hashCtx, &keyContainer);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	free_key_refs(&keyContainer);

	/* create the plaintext private key blob */
	offset = 0;
	Trspi_LoadBlob_BYTE(&offset, TCPA_PT_ASYM, newPrivKey);
	Trspi_LoadBlob(&offset, 20, newPrivKey, usage.authdata);
	Trspi_LoadBlob(&offset, 20, newPrivKey, migration.authdata);
	Trspi_LoadBlob(&offset, 20, newPrivKey, digest.digest);
	Trspi_LoadBlob_UINT32(&offset, keyPrivBlobLen, newPrivKey);
	Trspi_LoadBlob(&offset, keyPrivBlobLen, newPrivKey, keyPrivBlob);
	newPrivKeyLen = offset;

	/* encrypt the private key blob */
	if ((result = Trspi_RSA_Encrypt(newPrivKey, newPrivKeyLen, encPrivKey,
					&encPrivKeyLen, wrappingPubKey,
					wrappingPubKeyLen)))
		goto done;

	/* set the new encrypted private key in the wrapped key object */
	if ((result = obj_rsakey_set_privkey(hKey, FALSE, encPrivKeyLen, encPrivKey)))
		goto done;

done:
	free_tspi(tspContext, keyPrivBlob);
	free_tspi(tspContext, keyBlob);
	free_tspi(tspContext, wrappingPubKey);
	return result;
}

TSS_RESULT
Tspi_Context_LoadKeyByBlob(TSS_HCONTEXT tspContext,	/* in */
			   TSS_HKEY hUnwrappingKey,	/* in */
			   UINT32 ulBlobLength,		/* in */
			   BYTE * rgbBlobData,		/* in */
			   TSS_HKEY * phKey)		/* out */
{
	TPM_AUTH auth;
	UINT64 offset;
	TCPA_DIGEST digest;
	TSS_RESULT result;
	UINT32 keyslot;
	TSS_HPOLICY hPolicy;
	TCS_KEY_HANDLE tcsParentHandle, myTCSKeyHandle;
	TSS_KEY keyContainer;
	TSS_BOOL useAuth;
	TPM_AUTH *pAuth;
	TSS_FLAG initFlags;
	UINT16 realKeyBlobSize;
	TCPA_KEY_USAGE keyUsage;
	UINT32 pubLen;
	Trspi_HashCtx hashCtx;
	TPM_COMMAND_CODE ordinal;

	if (phKey == NULL || rgbBlobData == NULL )
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_rsakey(hUnwrappingKey))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = obj_context_get_loadkey_ordinal(tspContext, &ordinal)))
		return result;

	if ((result = obj_rsakey_get_tcs_handle(hUnwrappingKey, &tcsParentHandle)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, rgbBlobData, &keyContainer)))
		return result;
	realKeyBlobSize = offset;
	pubLen = keyContainer.pubKey.keyLength;
	keyUsage = keyContainer.keyUsage;
	/* free these now, since they're not used below */
	free_key_refs(&keyContainer);

	if ((result = obj_rsakey_get_policy(hUnwrappingKey, TSS_POLICY_USAGE, &hPolicy, &useAuth)))
		return result;

	if (useAuth) {
		/* Create the Authorization */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
		result |= Trspi_HashUpdate(&hashCtx, ulBlobLength, rgbBlobData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = secret_PerformAuth_OIAP(hUnwrappingKey, ordinal, hPolicy, FALSE,
						      &digest, &auth)))
			return result;

		pAuth = &auth;
	} else {
		pAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->LoadKeyByBlob(tspContext, tcsParentHandle, ulBlobLength,
							 rgbBlobData, pAuth, &myTCSKeyHandle,
							 &keyslot)))
		return result;

	if (useAuth) {
		/* ---  Validate return auth */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, ordinal);
		if (ordinal == TPM_ORD_LoadKey)
			result |= Trspi_Hash_UINT32(&hashCtx, keyslot);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth)))
			return result;
	}

	/* ---  Create a new Object */
	initFlags = 0;
	if (pubLen == 0x100)
		initFlags |= TSS_KEY_SIZE_2048;
	else if (pubLen == 0x80)
		initFlags |= TSS_KEY_SIZE_1024;
	else if (pubLen == 0x40)
		initFlags |= TSS_KEY_SIZE_512;

	/* clear the key type field */
	initFlags &= ~TSS_KEY_TYPE_MASK;

	if (keyUsage == TPM_KEY_STORAGE)
		initFlags |= TSS_KEY_TYPE_STORAGE;
	else
		initFlags |= TSS_KEY_TYPE_SIGNING;	/* loading the blob
							   will fix this
							   back to what it
							   should be. */

	if ((result = obj_rsakey_add(tspContext, initFlags, phKey))) {
		LogDebug("Failed create object");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if ((result = obj_rsakey_set_tcpakey(*phKey,realKeyBlobSize, rgbBlobData))) {
		LogDebug("Key loaded but failed to setup the key object"
			  "correctly");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return obj_rsakey_set_tcs_handle(*phKey, myTCSKeyHandle);
}

TSS_RESULT
Tspi_TPM_OwnerGetSRKPubKey(TSS_HTPM hTPM,		/* in */
			   UINT32 * pulPuKeyLength,	/* out */
			   BYTE ** prgbPubKey)		/* out */
{
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	TSS_HCONTEXT tspContext;
	TCS_KEY_HANDLE hKey;
	TPM_AUTH auth;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;

	if (pulPuKeyLength == NULL || prgbPubKey == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	hKey = TPM_KEYHND_SRK;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerReadInternalPub);
	result |= Trspi_Hash_UINT32(&hashCtx, hKey);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_OwnerReadInternalPub,
					      hPolicy, FALSE, &digest, &auth)))
		return result;

	if ((result = TCS_API(tspContext)->OwnerReadInternalPub(tspContext, hKey, &auth,
								pulPuKeyLength, prgbPubKey)))
		return result;

	/* Validate return auth */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TSS_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerReadInternalPub);
	result |= Trspi_HashUpdate(&hashCtx, *pulPuKeyLength, *prgbPubKey);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &auth)))
		goto error;

	/* Call a special SRK-seeking command to transparently add the public data to the object */
	if ((result = obj_rsakey_set_srk_pubkey(*prgbPubKey))) {
		LogError("Error setting SRK public data, SRK key object may not exist");
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbPubKey)))
		goto error;

	return result;

error:
	free(*prgbPubKey);
	pulPuKeyLength = 0;
	return result;
}

/* TSS 1.2-only interfaces */
#ifdef TSS_BUILD_TSS12
TSS_RESULT
Tspi_TPM_KeyControlOwner(TSS_HTPM hTPM,		/* in */
			 TSS_HKEY hTssKey,	/* in */
			 UINT32 attribName,	/* in */
			 TSS_BOOL attribValue,	/* in */
			 TSS_UUID* pUuidData)	/* out */
{
	TSS_RESULT result;
	TSS_HPOLICY hPolicy;
	TSS_HCONTEXT tspContext;
	TCS_KEY_HANDLE hTcsKey;
	BYTE *pubKey = NULL;
	UINT32 pubKeyLen;
	TPM_KEY_CONTROL tpmAttribName;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	TPM_AUTH ownerAuth;

	LogDebugFn("Enter");

	/* Check valid TPM context, get TSP context */
	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	/* Get Tcs KeyHandle */
	if ((result = obj_rsakey_get_tcs_handle(hTssKey, &hTcsKey)))
		return result;

	/* Validate/convert attribName */
	switch (attribName) {
		case TSS_TSPATTRIB_KEYCONTROL_OWNEREVICT:
			tpmAttribName = TPM_KEY_CONTROL_OWNER_EVICT;
			break;
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	/* Begin Auth - get TPM Policy Handler */
	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	/* Get associated pubKey */
	if ((result = obj_rsakey_get_pub_blob(hTssKey, &pubKeyLen, &pubKey)))
		return result;

	/* Create hash digest */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_KeyControlOwner);
	LogDebugData(pubKeyLen, pubKey);
	result |= Trspi_HashUpdate(&hashCtx, pubKeyLen, pubKey);
	result |= Trspi_Hash_UINT32(&hashCtx, tpmAttribName);
	result |= Trspi_Hash_BOOL(&hashCtx, attribValue);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		free_tspi(tspContext, pubKey);
		return result;
	}

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_KeyControlOwner, hPolicy, FALSE,
					      &digest, &ownerAuth))) {
		free_tspi(tspContext, pubKey);
		return result;
	}

	if ((result = RPC_KeyControlOwner(tspContext, hTcsKey, pubKeyLen, pubKey, tpmAttribName,
					  attribValue, &ownerAuth, pUuidData))) {
		free_tspi(tspContext, pubKey);
		return result;
	}

	/* Validate return auth */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TSS_SUCCESS);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_KeyControlOwner);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, &ownerAuth)))
		return result;

	/* change hKey internal flag, according to attrib[Name|Value] */
	switch (attribName) {
		case TSS_TSPATTRIB_KEYCONTROL_OWNEREVICT:
			result = obj_rsakey_set_ownerevict(hTssKey, attribValue);
			break;
		default:
			/* NOT-REACHED */
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	return result;
}
#endif
