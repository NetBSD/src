
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
TCSP_CMK_SetRestrictions_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
				  TSS_CMK_DELEGATE	Restriction,	/* in */
				  TPM_AUTH*		ownerAuth)	/* in */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CMK_SetRestrictions, &offset, txBlob,
				    Restriction, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_SetRestrictions, txBlob, paramSize,
				       ownerAuth);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_CMK_ApproveMA_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
			    TPM_DIGEST		migAuthorityDigest,	/* in */
			    TPM_AUTH*		ownerAuth,		/* in, out */
			    TPM_HMAC*		HmacMigAuthDigest)	/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CMK_ApproveMA, &offset, txBlob,
				    &migAuthorityDigest, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_ApproveMA, txBlob, paramSize,
				       HmacMigAuthDigest, ownerAuth);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_CMK_CreateKey_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
			    TCS_KEY_HANDLE	hWrappingKey,		/* in */
			    TPM_ENCAUTH		KeyUsageAuth,		/* in */
			    TPM_HMAC		MigAuthApproval,	/* in */
			    TPM_DIGEST		MigAuthorityDigest,	/* in */
			    UINT32*		keyDataSize,		/* in, out */
			    BYTE**		prgbKeyData,		/* in, out */
			    TPM_AUTH*		pAuth)			/* in, out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	UINT32 parentSlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext))) {
		free(*prgbKeyData);
		return result;
	}

	if ((result = get_slot(hContext, hWrappingKey, &parentSlot))) {
		free(*prgbKeyData);
		return result;
	}

	if (pAuth) {
		if ((result = auth_mgr_check(hContext, &pAuth->AuthHandle))) {
			free(*prgbKeyData);
			return result;
		}
	}

	if ((result = tpm_rqu_build(TPM_ORD_CMK_CreateKey, &offset, txBlob,
				    parentSlot, &KeyUsageAuth, *keyDataSize, *prgbKeyData,
				    &MigAuthApproval, &MigAuthorityDigest, pAuth))) {
		free(*prgbKeyData);
		goto done;
	}
	free(*prgbKeyData);

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_CreateKey, txBlob, paramSize,
				       keyDataSize, prgbKeyData, pAuth);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(pAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_CMK_CreateTicket_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
			       UINT32			PublicVerifyKeySize,	/* in */
			       BYTE*			PublicVerifyKey,	/* in */
			       TPM_DIGEST		SignedData,		/* in */
			       UINT32			SigValueSize,		/* in */
			       BYTE*			SigValue,		/* in */
			       TPM_AUTH*		pOwnerAuth,		/* in, out */
			       TPM_HMAC*		SigTicket)		/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CMK_CreateTicket, &offset, txBlob,
				    PublicVerifyKeySize, PublicVerifyKey, &SignedData,
				    SigValueSize, SigValue, pOwnerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_CreateTicket, txBlob, paramSize,
				       SigTicket, pOwnerAuth);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(pOwnerAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_CMK_CreateBlob_Internal(TCS_CONTEXT_HANDLE	hContext,		/* in */
			     TCS_KEY_HANDLE	parentHandle,		/* in */
			     TSS_MIGRATE_SCHEME	migrationType,		/* in */
			     UINT32		MigrationKeyAuthSize,	/* in */
			     BYTE*		MigrationKeyAuth,	/* in */
			     TPM_DIGEST		PubSourceKeyDigest,	/* in */
			     UINT32		msaListSize,		/* in */
			     BYTE*		msaList,		/* in */
			     UINT32		restrictTicketSize,	/* in */
			     BYTE*		restrictTicket,		/* in */
			     UINT32		sigTicketSize,		/* in */
			     BYTE*		sigTicket,		/* in */
			     UINT32		encDataSize,		/* in */
			     BYTE*		encData,		/* in */
			     TPM_AUTH*		parentAuth,		/* in, out */
			     UINT32*		randomSize,		/* out */
			     BYTE**		random,			/* out */
			     UINT32*		outDataSize,		/* out */
			     BYTE**		outData)		/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	UINT32 parentSlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = get_slot(hContext, parentHandle, &parentSlot)))
		return result;

	if (parentAuth) {
		if ((result = auth_mgr_check(hContext, &parentAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_CMK_CreateBlob, &offset, txBlob,
				    parentSlot, migrationType, MigrationKeyAuthSize,
				    MigrationKeyAuth, &PubSourceKeyDigest, msaListSize, msaList,
				    restrictTicketSize, restrictTicket, sigTicketSize, sigTicket,
				    encDataSize, encData, parentAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_CreateBlob, txBlob, paramSize,
				       randomSize, random, outDataSize, outData, parentAuth, NULL);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(parentAuth, NULL, hContext);

	return result;
}

TSS_RESULT
TCSP_CMK_ConvertMigration_Internal(TCS_CONTEXT_HANDLE	hContext,	/* in */
				   TCS_KEY_HANDLE	parentHandle,	/* in */
				   TPM_CMK_AUTH		restrictTicket,	/* in */
				   TPM_HMAC		sigTicket,	/* in */
				   UINT32		keyDataSize,	/* in */
				   BYTE*		prgbKeyData,	/* in */
				   UINT32		msaListSize,	/* in */
				   BYTE*		msaList,	/* in */
				   UINT32		randomSize,	/* in */
				   BYTE*		random,		/* in */
				   TPM_AUTH*		parentAuth,	/* in, out */
				   UINT32*		outDataSize,	/* out */
				   BYTE**		outData)	/* out */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	UINT32 parentSlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = get_slot(hContext, parentHandle, &parentSlot)))
		return result;

	if (parentAuth) {
		if ((result = auth_mgr_check(hContext, &parentAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_CMK_ConvertMigration, &offset, txBlob,
				    parentSlot, &restrictTicket, &sigTicket,
				    keyDataSize, prgbKeyData, msaListSize, msaList,
				    randomSize, random, parentAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CMK_ConvertMigration, txBlob, paramSize,
				       outDataSize, outData, parentAuth, NULL);
	}

	LogResult("CMK_SetRestrictions", result);

done:
	auth_mgr_release_auth(parentAuth, NULL, hContext);

	return result;
}

