
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
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsps.h"
#include "req_mgr.h"


TSS_RESULT
TCSP_LoadKeyByBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TCS_KEY_HANDLE hUnwrappingKey,	/* in */
			    UINT32 cWrappedKeyBlobSize,		/* in */
			    BYTE * rgbWrappedKeyBlob,		/* in */
			    TPM_AUTH * pAuth,			/* in, out */
			    TCS_KEY_HANDLE * phKeyTCSI,		/* out */
			    TCS_KEY_HANDLE * phKeyHMAC)		/* out */
{
	return LoadKeyByBlob_Internal(TPM_ORD_LoadKey, hContext, hUnwrappingKey,
				      cWrappedKeyBlobSize, rgbWrappedKeyBlob, pAuth, phKeyTCSI,
				      phKeyHMAC);
}

TSS_RESULT
TCSP_LoadKey2ByBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			     TCS_KEY_HANDLE hUnwrappingKey,	/* in */
			     UINT32 cWrappedKeyBlobSize,	/* in */
			     BYTE * rgbWrappedKeyBlob,		/* in */
			     TPM_AUTH * pAuth,			/* in, out */
			     TCS_KEY_HANDLE * phKeyTCSI)	/* out */
{
	return LoadKeyByBlob_Internal(TPM_ORD_LoadKey2, hContext, hUnwrappingKey,
				      cWrappedKeyBlobSize, rgbWrappedKeyBlob, pAuth, phKeyTCSI,
				      NULL);
}

TSS_RESULT
LoadKeyByBlob_Internal(UINT32 ord,	/* The ordinal to use, LoadKey or LoadKey2 */
		       TCS_CONTEXT_HANDLE hContext,	/* in */
		       TCS_KEY_HANDLE hUnwrappingKey,	/* in */
		       UINT32 cWrappedKeyBlobSize,		/* in */
		       BYTE * rgbWrappedKeyBlob,		/* in */
		       TPM_AUTH * pAuth,			/* in, out */
		       TCS_KEY_HANDLE * phKeyTCSI,		/* out */
		       TCS_KEY_HANDLE * phKeyHMAC)		/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 paramSize;
	TPM_KEY_HANDLE parentSlot, newSlot;
	TCS_KEY_HANDLE newHandle = NULL_TCS_HANDLE;
	TSS_BOOL canLoad;
	TSS_KEY key;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	LogDebugFn("Enter");
	LogDebugUnrollKey(rgbWrappedKeyBlob);

	if ((result = get_slot(hContext, hUnwrappingKey, &parentSlot)))
		return result;

	offset = 0;
	memset(&key, 0, sizeof(TSS_KEY));
	if ((result = UnloadBlob_TSS_KEY(&offset, rgbWrappedKeyBlob, &key)))
		return result;

	if (!pAuth) {
		LogDebugFn("Checking if LoadKeyByBlob can be avoided by using existing key");

		if ((newHandle = mc_get_handle_by_pub(&key.pubKey, hUnwrappingKey))) {
			LogDebugFn("tcs key handle exists");

			newSlot = mc_get_slot_by_handle(newHandle);
			if (newSlot && (isKeyLoaded(newSlot) == TRUE)) {
				LogDebugFn("Don't need to reload this key.");
				*phKeyTCSI = newHandle;
				if (phKeyHMAC)
					*phKeyHMAC = newSlot;
				return TSS_SUCCESS;
			}
		}
	}

        LogDebugFn("calling canILoadThisKey");
	if ((result = canILoadThisKey(&(key.algorithmParms), &canLoad)))
		goto error;

	if (canLoad == FALSE) {
		LogDebugFn("calling evictFirstKey");
		/* Evict a key that isn't the parent */
		if ((result = evictFirstKey(hUnwrappingKey)))
			goto error;
	}

	offset = 0;
	if ((result = tpm_rqu_build(ord, &offset, txBlob, parentSlot, cWrappedKeyBlobSize,
				    rgbWrappedKeyBlob, pAuth, NULL)))
		goto error;

	LogDebugFn("Submitting request to the TPM");
	if ((result = req_mgr_submit_req(txBlob)))
		goto error;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto error;
	}

	if ((result = tpm_rsp_parse(ord, txBlob, paramSize, &newSlot, pAuth)))
		goto error;

	if ((result = load_key_final(hContext, hUnwrappingKey, &newHandle, rgbWrappedKeyBlob,
				     newSlot)))
		goto error;

	/* Setup the outHandles */
	*phKeyTCSI = newHandle;
	if (phKeyHMAC)
		*phKeyHMAC = newSlot;

	LogDebugFn("Key handles for loadKeyByBlob slot:%.8X tcshandle:%.8X", newSlot, newHandle);
error:
	auth_mgr_release_auth(pAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_EvictKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		       TCS_KEY_HANDLE hKey)		/* in */
{
	TSS_RESULT result;
	TCPA_KEY_HANDLE tpm_handle;

	if ((result = ctx_verify_context(hContext)))
		return result;

	tpm_handle = mc_get_slot_by_handle(hKey);
	if (tpm_handle == NULL_TPM_HANDLE)
		return TSS_SUCCESS;	/*let's call this success if the key is already evicted */

	if ((result = internal_EvictByKeySlot(tpm_handle)))
		return result;

	result = mc_set_slot_by_slot(tpm_handle, NULL_TPM_HANDLE);

	return result;
}

TSS_RESULT
TCSP_CreateWrapKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TCS_KEY_HANDLE hWrappingKey,	/* in */
			    TCPA_ENCAUTH KeyUsageAuth,		/* in */
			    TCPA_ENCAUTH KeyMigrationAuth,	/* in */
			    UINT32 keyInfoSize,			/* in */
			    BYTE * keyInfo,			/* in */
			    UINT32 * keyDataSize,		/* out */
			    BYTE ** keyData,			/* out */
			    TPM_AUTH * pAuth)			/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE parentSlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Create Wrap Key");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (pAuth) {
		if ((result = auth_mgr_check(hContext, &pAuth->AuthHandle)))
			goto done;
	}

	/* Since hWrappingKey must already be loaded, we can fail immediately if
	 * mc_get_slot_by_handle_lock() fails.*/
	parentSlot = mc_get_slot_by_handle_lock(hWrappingKey);
	if (parentSlot == NULL_TPM_HANDLE) {
		result = TCSERR(TSS_E_FAIL);
		goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_CreateWrapKey, &offset, txBlob, parentSlot,
				    KeyUsageAuth.authdata, KeyMigrationAuth.authdata, keyInfoSize,
				    keyInfo, pAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CreateWrapKey, txBlob, paramSize, keyDataSize,
				       keyData, pAuth);
	}
	LogResult("Create Wrap Key", result);

done:
	auth_mgr_release_auth(pAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_GetPubKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			TCS_KEY_HANDLE hKey,		/* in */
			TPM_AUTH * pAuth,		/* in, out */
			UINT32 * pcPubKeySize,		/* out */
			BYTE ** prgbPubKey)		/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Get pub key");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (pAuth != NULL) {
		LogDebug("Auth Used");
		if ((result = auth_mgr_check(hContext, &pAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}

	if (ensureKeyIsLoaded(hContext, hKey, &keySlot)) {
		result = TCSERR(TCS_E_KM_LOADFAILED);
		goto done;
	}

	LogDebug("GetPubKey: handle: 0x%x, slot: 0x%x", hKey, keySlot);
	if ((result = tpm_rqu_build(TPM_ORD_GetPubKey, &offset, txBlob, keySlot, pAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_GetPubKey, txBlob, paramSize, pcPubKeySize,
				       prgbPubKey, pAuth);
	}
	LogResult("Get Public Key", result);
done:
	auth_mgr_release_auth(pAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_OwnerReadInternalPub_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				   TCS_KEY_HANDLE hKey,	/* in */
				   TPM_AUTH * pOwnerAuth,	/* in, out */
				   UINT32 * punPubKeySize,	/* out */
				   BYTE ** ppbPubKeyData)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering OwnerReadInternalPub");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	LogDebug("OwnerReadInternalPub: handle: 0x%x", hKey);
	if (hKey != TPM_KH_SRK && hKey != TPM_KH_EK) {
		result = TCSERR(TSS_E_FAIL);
		LogDebug("OwnerReadInternalPub - Unsupported Key Handle");
		goto done;
	}

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_OwnerReadInternalPub, &offset, txBlob, hKey,
				    pOwnerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OwnerReadInternalPub, txBlob, paramSize,
				       punPubKeySize, ppbPubKeyData, pOwnerAuth);
	}
	LogResult("OwnerReadInternalPub", result);
done:
	auth_mgr_release_auth(pOwnerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_KeyControlOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TCS_KEY_HANDLE hTcsKey,		/* in */
			      UINT32 ulPubKeyLength,		/* in */
			      BYTE* rgbPubKey,			/* in */
			      UINT32 attribName,		/* in */
			      TSS_BOOL attribValue,		/* in */
			      TPM_AUTH* pOwnerAuth,		/* in,out */
			      TSS_UUID* pUuidData)		/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TPM_KEY_HANDLE hTpmKey;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ((result = ctx_verify_context(hContext))) {
		LogDebug("Invalid TSS Context");
		goto done;
	}

	if ((result = get_slot_lite(hContext, hTcsKey, &hTpmKey))) {
		LogDebug("Can't get TPM Keyhandle for TCS key 0x%x", hTcsKey);
		goto done;
	}
	LogDebugFn("TCS hKey=0x%x, TPM hKey=0x%x", hTcsKey, hTpmKey);

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle))) {
		LogDebug("Owner Authentication failed");
		goto done;
	}

	if ((result = mc_find_next_ownerevict_uuid(pUuidData))) {
		LogDebugFn("mc_find_next_ownerevict_uuid failed: rc=0x%x", result);
		goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_KeyControlOwner, &offset, txBlob, hTpmKey,
				    ulPubKeyLength, rgbPubKey, attribName, attribValue,
				    pOwnerAuth))) {
		LogDebugFn("rqu build failed");
		goto done;
	}

	if ((result = req_mgr_submit_req(txBlob))) {
	        LogDebugFn("Request submission failed");
		goto done;
	}

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto done;
	}

	if ((result = tpm_rsp_parse(TPM_ORD_KeyControlOwner, txBlob, paramSize, pOwnerAuth))) {
		LogDebugFn("tpm_rsp_parse failed: rc=0x%x", result);
		goto done;
	}

	if ((result = mc_set_uuid(hTcsKey, pUuidData))){
		LogDebugFn("mc_set_uuid failed: rc=0x%x", result);
		goto done;
	}

	LogResult("KeyControlOwner", result);
done:
	auth_mgr_release_auth(pOwnerAuth, NULL, hContext);
	return result;
}

