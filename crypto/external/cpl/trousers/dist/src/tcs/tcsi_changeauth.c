
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcsps.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "req_mgr.h"
#include "tcsd_wrap.h"
#include "tcsd.h"


TSS_RESULT
TCSP_ChangeAuth_Internal(TCS_CONTEXT_HANDLE contextHandle,	/* in */
			 TCS_KEY_HANDLE parentHandle,	/* in */
			 TCPA_PROTOCOL_ID protocolID,	/* in */
			 TCPA_ENCAUTH newAuth,	/* in */
			 TCPA_ENTITY_TYPE entityType,	/* in */
			 UINT32 encDataSize,	/* in */
			 BYTE *encData,	/* in */
			 TPM_AUTH *ownerAuth,	/* in, out */
			 TPM_AUTH *entityAuth,	/* in, out */
			 UINT32 *outDataSize,	/* out */
			 BYTE **outData	/* out */
    )
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	TCS_KEY_HANDLE tcsKeyHandleToEvict;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Changeauth");
	if ((result = ctx_verify_context(contextHandle)))
		goto done;

	if ((result = auth_mgr_check(contextHandle, &ownerAuth->AuthHandle)))
		goto done;
	if ((result = auth_mgr_check(contextHandle, &entityAuth->AuthHandle)))
		goto done;

	if ((result = ensureKeyIsLoaded(contextHandle, parentHandle, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_ChangeAuth, &offset, txBlob, keySlot, protocolID,
				    newAuth.authdata, entityType, encDataSize, encData, ownerAuth,
				    entityAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ChangeAuth, txBlob, paramSize, outDataSize, outData,
				       ownerAuth, entityAuth);

		/* if the malloc above failed, terminate the 2 new auth handles and exit */
		if (result)
			goto done;

		/*
		 * Check if ET is a key.  If it is, we need to
		 *	1 - Evict the key if loaded
		 *	2 - update the mem cache entry
		 */
		if (entityType == TCPA_ET_KEYHANDLE || entityType == TCPA_ET_KEY) {
			LogDebug("entity type is a key. Check if mem cache needs updating...");
			tcsKeyHandleToEvict = mc_get_handle_by_encdata(encData);
			LogDebug("tcsKeyHandle being evicted is %.8X", tcsKeyHandleToEvict);
			/*---	If it was found in knowledge, replace it */
			if (tcsKeyHandleToEvict != 0) {
				internal_EvictByKeySlot(keySlot);
				mc_update_encdata(encData, *outData);
			}

		}
	}
	LogResult("ChangeAuth", result);
done:
	auth_mgr_release_auth(ownerAuth, entityAuth, contextHandle);
	return result;
}

TSS_RESULT
TCSP_ChangeAuthOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TCPA_PROTOCOL_ID protocolID,	/* in */
			      TCPA_ENCAUTH newAuth,	/* in */
			      TCPA_ENTITY_TYPE entityType,	/* in */
			      TPM_AUTH * ownerAuth	/* in, out */
    )
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering ChangeAuthOwner");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_ChangeAuthOwner, &offset, txBlob, protocolID,
				    newAuth.authdata, entityType, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ChangeAuthOwner, txBlob, paramSize, ownerAuth);
	}

	LogResult("ChangeAuthOwner", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_ChangeAuthAsymStart_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				  TCS_KEY_HANDLE idHandle,	/* in */
				  TCPA_NONCE antiReplay,	/* in */
				  UINT32 KeySizeIn,	/* in */
				  BYTE * KeyDataIn,	/* in */
				  TPM_AUTH * pAuth,	/* in, out */
				  UINT32 * KeySizeOut,	/* out */
				  BYTE ** KeyDataOut,	/* out */
				  UINT32 * CertifyInfoSize,	/* out */
				  BYTE ** CertifyInfo,	/* out */
				  UINT32 * sigSize,	/* out */
				  BYTE ** sig,	/* out */
				  TCS_KEY_HANDLE * ephHandle	/* out */
    )
{
#if 0
#warning Locking trouble in evictFirstKey
	UINT64 offset;
	UINT32 paramSize;
	TSS_RESULT result;
	UINT32 keySlot;
	TCPA_CERTIFY_INFO certifyInfo;
	TSS_KEY tempKey;
	UINT32 tempSize;
	TCPA_KEY_PARMS keyParmsContainer;
	TSS_BOOL canLoad;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering ChangeAuthAsymStart");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (pAuth != NULL) {
		LogDebug("Auth Command");
		if ((result = auth_mgr_check(hContext, pAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}

	if ((result = ensureKeyIsLoaded(hContext, idHandle, &keySlot)))
		goto done;

	LogDebug("Checking for room to load the eph key");
	offset = 0;
	if ((result = UnloadBlob_KEY_PARMS(&offset, KeyDataIn, &keyParmsContainer)))
		goto done;

	/* if we can't load the key, evict keys until we can */
	if ((result = canILoadThisKey(&keyParmsContainer, &canLoad)))
		goto done;

	while (canLoad == FALSE) {
		/* Evict a key that isn't the parent */
		if ((result = evictFirstKey(idHandle)))
			goto done;

		if ((result = canILoadThisKey(&keyParmsContainer, &canLoad)))
			goto done;
	}

	offset = 10;
	LoadBlob_UINT32(&offset, keySlot, txBlob);
	LoadBlob(&offset, TCPA_NONCE_SIZE, txBlob, antiReplay.nonce);
/*	LoadBlob_KEY_PARMS( &offset, txBlob, &tempKeyParms ); */
/*	LoadBlob_UINT32( &offset, KeySizeIn, txBlob ); */
	LoadBlob(&offset, KeySizeIn, txBlob, KeyDataIn);

	if (pAuth != NULL) {
		LoadBlob_Auth(&offset, txBlob, pAuth);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset,
				TPM_ORD_ChangeAuthAsymStart, txBlob);
	} else {
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset,
				TPM_ORD_ChangeAuthAsymStart, txBlob);
	}

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);
	if (result == 0) {
		UnloadBlob_CERTIFY_INFO(&offset, txBlob, &certifyInfo);
		*CertifyInfoSize = offset - 10;
		*CertifyInfo = malloc(*CertifyInfoSize);
		if (*CertifyInfo == NULL) {
			LogError("malloc of %u bytes failed.", *CertifyInfoSize);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(*CertifyInfo, &txBlob[offset - *CertifyInfoSize],
		       *CertifyInfoSize);
		UnloadBlob_UINT32(&offset, sigSize, txBlob);
		*sig = malloc(*sigSize);
		if (*sig == NULL) {
			LogError("malloc of %u bytes failed.", *sigSize);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		UnloadBlob(&offset, *sigSize, txBlob, *sig);
		UnloadBlob_UINT32(&offset, ephHandle, txBlob);
		tempSize = offset;
		UnloadBlob_TSS_KEY(&offset, txBlob, &tempKey);
		*KeySizeOut = offset - tempSize;
		*KeyDataOut = malloc(*KeySizeOut);
		if (*KeyDataOut == NULL) {
			LogError("malloc of %u bytes failed.", *KeySizeOut);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(*KeyDataOut, &txBlob[offset - *KeySizeOut], *KeySizeOut);
		if (pAuth != NULL)
			UnloadBlob_Auth(&offset, txBlob, pAuth);
	}

	LogResult("ChangeAuthAsymStart", result);
done:
	auth_mgr_release_auth(pAuth, NULL, hContext);
	return result;
#else
	return TCSERR(TSS_E_NOTIMPL);
#endif
}

TSS_RESULT
TCSP_ChangeAuthAsymFinish_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				   TCS_KEY_HANDLE parentHandle,	/* in */
				   TCS_KEY_HANDLE ephHandle,	/* in */
				   TCPA_ENTITY_TYPE entityType,	/* in */
				   TCPA_HMAC newAuthLink,	/* in */
				   UINT32 newAuthSize,	/* in */
				   BYTE * encNewAuth,	/* in */
				   UINT32 encDataSizeIn,	/* in */
				   BYTE * encDataIn,	/* in */
				   TPM_AUTH * ownerAuth,	/* in, out */
				   UINT32 * encDataSizeOut,	/* out */
				   BYTE ** encDataOut,	/* out */
				   TCPA_SALT_NONCE * saltNonce,	/* out */
				   TCPA_DIGEST * changeProof	/* out */
    )
{
#if 0
	UINT64 offset;
	UINT32 paramSize;
	TSS_RESULT result;
	UINT32 keySlot;
#if 0
	TCPA_CERTIFY_INFO certifyInfo;
	TSS_KEY tempKey;
	UINT32 tempSize;
	TSS_UUID *uuidKeyToEvict;
#endif
	TCS_KEY_HANDLE tcsKeyHandleToEvict;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering ChangeAuthAsymFinish");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (ownerAuth != NULL) {
		LogDebug("Auth used");
		if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}
	if ((result = ensureKeyIsLoaded(hContext, parentHandle, &keySlot)))
		goto done;

	offset = 10;
	LoadBlob_UINT32(&offset, keySlot, txBlob);
	LoadBlob_UINT32(&offset, ephHandle, txBlob);
	LoadBlob_UINT16(&offset, entityType, txBlob);
	LoadBlob(&offset, 20, txBlob, newAuthLink.digest);
	LoadBlob_UINT32(&offset, newAuthSize, txBlob);
	LoadBlob(&offset, newAuthSize, txBlob, encNewAuth);
	LoadBlob_UINT32(&offset, encDataSizeIn, txBlob);
	LoadBlob(&offset, encDataSizeIn, txBlob, encDataIn);

	if (ownerAuth != NULL) {
		LoadBlob_Auth(&offset, txBlob, ownerAuth);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset,
				TPM_ORD_ChangeAuthAsymFinish, txBlob);
	} else {
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset,
				TPM_ORD_ChangeAuthAsymFinish, txBlob);
	}

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		UnloadBlob_UINT32(&offset, encDataSizeOut, txBlob);
		*encDataOut = calloc(1, *encDataSizeOut);
		if (*encDataOut == NULL) {
			LogError("malloc of %u bytes failed.", *encDataSizeOut);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		UnloadBlob(&offset, *encDataSizeOut, txBlob, *encDataOut);
		UnloadBlob(&offset, 20, txBlob, saltNonce->nonce);
		UnloadBlob(&offset, 20, txBlob, changeProof->digest);
		if (ownerAuth != NULL)
			UnloadBlob_Auth(&offset, txBlob, ownerAuth);

		/* Check if ET is a key.  If it is, we need to
		 *	1 - Evict the key if loaded
		 *	2 - update the mem cache entry
		 */
		if (entityType == TCPA_ET_KEYHANDLE ||
		    entityType == TCPA_ET_KEY) {
			tcsKeyHandleToEvict = mc_get_handle_by_encdata(encDataIn);
			/* If it was found in mem cache, replace it */
			if (tcsKeyHandleToEvict != 0) {
				key_mgr_evict(hContext, tcsKeyHandleToEvict);
				mc_update_encdata(encDataIn, *encDataOut);
			}
		}
	}

	LogResult("ChangeAuthAsymFinish", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
#else
	return TCSERR(TSS_E_NOTIMPL);
#endif
}

