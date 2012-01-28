
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
#include "trousers_types.h"
#include "tcs_utils.h"
#include "tcslog.h"
#include "req_mgr.h"

TSS_RESULT
TCSP_Delegate_Manage_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TPM_FAMILY_ID familyID,		/* in */
			      TPM_FAMILY_OPERATION opFlag,	/* in */
			      UINT32 opDataSize,		/* in */
			      BYTE *opData,			/* in */
			      TPM_AUTH *ownerAuth,		/* in/out */
			      UINT32 *retDataSize,		/* out */
			      BYTE **retData)			/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ownerAuth) {
		if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_Manage, &offset, txBlob, familyID, opFlag,
				    opDataSize, opData, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_Manage, txBlob, paramSize, retDataSize,
				       retData, ownerAuth, NULL);
	}

	LogResult("Delegate_Manage", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_Delegate_CreateKeyDelegation_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					   TCS_KEY_HANDLE hKey,		/* in */
					   UINT32 publicInfoSize,	/* in */
					   BYTE *publicInfo,		/* in */
					   TPM_ENCAUTH *encDelAuth,	/* in */
					   TPM_AUTH *keyAuth,		/* in, out */
					   UINT32 *blobSize,		/* out */
					   BYTE **blob)			/* out */
{
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (keyAuth) {
		if ((result = auth_mgr_check(hContext, &keyAuth->AuthHandle)))
			return result;
	}

	if ((result = ensureKeyIsLoaded(hContext, hKey, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_CreateKeyDelegation, &offset, txBlob, keySlot,
				    publicInfoSize, publicInfo, encDelAuth, keyAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_CreateKeyDelegation, txBlob, paramSize,
				       blobSize, blob, keyAuth, NULL);
	}

	LogResult("Delegate_CreateKeyDelegation", result);

done:
	auth_mgr_release_auth(keyAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_Delegate_CreateOwnerDelegation_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					     TSS_BOOL increment,		/* in */
					     UINT32 publicInfoSize,		/* in */
					     BYTE *publicInfo,			/* in */
					     TPM_ENCAUTH *encDelAuth,		/* in */
					     TPM_AUTH *ownerAuth,		/* in, out */
					     UINT32 *blobSize,			/* out */
					     BYTE **blob)			/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ownerAuth) {
		if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_CreateOwnerDelegation, &offset, txBlob,
				    increment, publicInfoSize, publicInfo, encDelAuth, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_CreateOwnerDelegation, txBlob, paramSize,
				       blobSize, blob, ownerAuth, NULL);
	}

	LogResult("Delegate_CreateOwnerDelegation", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_Delegate_LoadOwnerDelegation_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					   TPM_DELEGATE_INDEX index,	/* in */
					   UINT32 blobSize,		/* in */
					   BYTE *blob,			/* in */
					   TPM_AUTH *ownerAuth)		/* in, out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ownerAuth) {
		if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_LoadOwnerDelegation, &offset, txBlob, index,
				    blobSize, blob, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_LoadOwnerDelegation, txBlob, paramSize,
				       ownerAuth);
	}

	LogResult("Delegate_LoadOwnerDelegation", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_Delegate_ReadTable_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				 UINT32 *familyTableSize,	/* out */
				 BYTE **familyTable,		/* out */
				 UINT32 *delegateTableSize,	/* out */
				 BYTE **delegateTable)		/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_ReadTable, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_ReadTable, txBlob, paramSize,
				       familyTableSize, familyTable, delegateTableSize,
				       delegateTable, NULL, NULL);
	}

	LogResult("Delegate_ReadTable", result);

	return result;
}

TSS_RESULT
TCSP_Delegate_UpdateVerificationCount_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					       UINT32 inputSize,		/* in */
					       BYTE *input,			/* in */
					       TPM_AUTH *ownerAuth,		/* in, out */
					       UINT32 *outputSize,		/* out */
					       BYTE **output)			/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ownerAuth) {
		if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_UpdateVerification, &offset, txBlob, inputSize,
				    inputSize, input, ownerAuth, NULL)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Delegate_UpdateVerification, txBlob, paramSize,
				       outputSize, output, ownerAuth, NULL);
	}

	LogResult("Delegate_UpdateVerificationCount", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_Delegate_VerifyDelegation_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
					UINT32 delegateSize,		/* in */
					BYTE *delegate)			/* in */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_Delegate_VerifyDelegation, &offset, txBlob,
				    delegateSize, delegateSize, delegate, NULL, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);

	LogResult("Delegate_VerifyDelegation", result);

	return result;
}

TSS_RESULT
TCSP_DSAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		   TPM_ENTITY_TYPE entityType,	/* in */
		   TCS_KEY_HANDLE keyHandle,	/* in */
		   TPM_NONCE *nonceOddDSAP,	/* in */
		   UINT32 entityValueSize,	/* in */
		   BYTE *entityValue,		/* in */
		   TCS_AUTHHANDLE *authHandle,	/* out */
		   TPM_NONCE *nonceEven,	/* out */
		   TPM_NONCE *nonceEvenDSAP)	/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	TPM_KEY_HANDLE tpmKeyHandle;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ensureKeyIsLoaded(hContext, keyHandle, &tpmKeyHandle))
		return TCSERR(TSS_E_KEY_NOT_LOADED);

	/* are the maximum number of auth sessions open? */
	if (auth_mgr_req_new(hContext) == FALSE) {
		if ((result = auth_mgr_swap_out(hContext)))
			goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_DSAP, &offset, txBlob, entityType, tpmKeyHandle,
				    nonceOddDSAP, entityValueSize, entityValue)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		if ((result = tpm_rsp_parse(TPM_ORD_DSAP, txBlob, paramSize, authHandle,
					    nonceEven->nonce, nonceEvenDSAP->nonce)))
			goto done;

		/* success, add an entry to the table */
		result = auth_mgr_add(hContext, *authHandle);
	}
done:
	LogResult("DSAP", result);

	return result;
}
