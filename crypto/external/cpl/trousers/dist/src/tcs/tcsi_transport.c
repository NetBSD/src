
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
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsps.h"
#include "req_mgr.h"


TSS_RESULT
TCSP_EstablishTransport_Internal(TCS_CONTEXT_HANDLE      hContext,
				 UINT32                  ulTransControlFlags,
				 TCS_KEY_HANDLE          hEncKey,
				 UINT32                  ulTransSessionInfoSize,
				 BYTE*                   rgbTransSessionInfo,
				 UINT32                  ulSecretSize,
				 BYTE*                   rgbSecret,
				 TPM_AUTH*               pEncKeyAuth,
				 TPM_MODIFIER_INDICATOR* pbLocality,
				 TCS_HANDLE*             hTransSession,
				 UINT32*                 ulCurrentTicks,
				 BYTE**                  prgbCurrentTicks,
				 TPM_NONCE*              pTransNonce)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset;
	TPM_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (ulTransControlFlags == TSS_TCSATTRIB_TRANSPORT_EXCLUSIVE) {
		if ((result = ctx_req_exclusive_transport(hContext)))
			return result;
	}

	if (pEncKeyAuth) {
		if ((result = auth_mgr_check(hContext, &pEncKeyAuth->AuthHandle)))
			return result;
	}

	/* if hEncKey is set to TPM_KH_TRANSPORT, that's the signal to the TPM that this will be
	 * an unencrypted transport session, so we don't need to check that its loaded */
	if (hEncKey != TPM_KH_TRANSPORT) {
		if ((result = ensureKeyIsLoaded(hContext, hEncKey, &keySlot)))
			return result;
	}

	offset = TSS_TPM_TXBLOB_HDR_LEN;
	LoadBlob_UINT32(&offset, keySlot, txBlob);
	LoadBlob(&offset, ulTransSessionInfoSize, txBlob, rgbTransSessionInfo);
	LoadBlob_UINT32(&offset, ulSecretSize, txBlob);
	LoadBlob(&offset, ulSecretSize, txBlob, rgbSecret);
	if (pEncKeyAuth) {
		LoadBlob_Auth(&offset, txBlob, pEncKeyAuth);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset, TPM_ORD_EstablishTransport,
				txBlob);
	} else
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset, TPM_ORD_EstablishTransport, txBlob);

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto done;
	}

	offset = TSS_TPM_TXBLOB_HDR_LEN;
	UnloadBlob_UINT32(&offset, hTransSession, txBlob);
	UnloadBlob_UINT32(&offset, pbLocality, txBlob);

	*ulCurrentTicks = sizeof(TPM_STRUCTURE_TAG)
			  + sizeof(UINT64)
			  + sizeof(UINT16)
			  + sizeof(TPM_NONCE);

	*prgbCurrentTicks = malloc(*ulCurrentTicks);
	if (*prgbCurrentTicks == NULL) {
		result = TCSERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	UnloadBlob(&offset, *ulCurrentTicks, txBlob, *prgbCurrentTicks);
	UnloadBlob(&offset, sizeof(TPM_NONCE), txBlob, (BYTE *)pTransNonce);
	if (pEncKeyAuth)
		UnloadBlob_Auth(&offset, txBlob, pEncKeyAuth);

	ctx_set_transport_enabled(hContext, *hTransSession);
done:
	auth_mgr_release_auth(pEncKeyAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_ExecuteTransport_Internal(TCS_CONTEXT_HANDLE      hContext,
			       TPM_COMMAND_CODE        unWrappedCommandOrdinal,
			       UINT32                  ulWrappedCmdParamInSize,
			       BYTE*                   rgbWrappedCmdParamIn,
			       UINT32*                 pulHandleListSize,	/* in, out */
			       TCS_HANDLE**            rghHandles,		/* in, out */
			       TPM_AUTH*               pWrappedCmdAuth1,	/* in, out */
			       TPM_AUTH*               pWrappedCmdAuth2,	/* in, out */
			       TPM_AUTH*               pTransAuth,		/* in, out */
			       UINT64*                 punCurrentTicks,
			       TPM_MODIFIER_INDICATOR* pbLocality,
			       TPM_RESULT*             pulWrappedCmdReturnCode,
			       UINT32*                 ulWrappedCmdParamOutSize,
			       BYTE**                  rgbWrappedCmdParamOut)
{
	TSS_RESULT result;
	UINT32 paramSize, wrappedSize, val1 = 0, val2 = 0, *pVal1 = NULL, *pVal2 = NULL;
	TCS_HANDLE handle1 = 0, handle2 = 0;
	UINT64 offset, wrappedOffset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];


	if (*pulHandleListSize > 2) {
		LogDebugFn("************ EXPAND KEYSLOT SIZE *********");
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (pWrappedCmdAuth1)
		if ((result = auth_mgr_check(hContext, &pWrappedCmdAuth1->AuthHandle)))
			goto done;

	if (pWrappedCmdAuth2)
		if ((result = auth_mgr_check(hContext, &pWrappedCmdAuth2->AuthHandle)))
			goto done;

	switch (unWrappedCommandOrdinal) {
	/* If the command is FlushSpecific, we get handle that needs to be freed, but we don't know
	 * what type it is. */
	case TPM_ORD_FlushSpecific:
		if (*pulHandleListSize == 0) { /* invalid */
			result = TCSERR(TSS_E_BAD_PARAMETER);
			goto done;
		}

		/* If this is a transport handle, remove the context's reference to the session */
		ctx_set_transport_disabled(hContext, rghHandles[0]);

		/* Is it a key? If so, jump to the get_slot_lite and key management calls below */
		if (ctx_has_key_loaded(hContext, *rghHandles[0]))
			goto map_key_handles;

		/* fall through */
	case TPM_ORD_Terminate_Handle:
		if (!auth_mgr_check(hContext, rghHandles[0]))
			auth_mgr_release_auth_handle(*rghHandles[0], hContext, FALSE);

		/* If the handle is an auth handle or any other kind of handle, there's no
		 * mapping done in the TCS, so pass its value straight to the TPM and jump over
		 * the switch statement below where we assume FLushSpecific is being done on a
		 * key */
		handle1 = val1 = *rghHandles[0];
		pVal1 = &val1;

		goto build_command;
	default:
		break;
	}

map_key_handles:
	if (*pulHandleListSize == 2) {
		handle2 = (*rghHandles)[1];

		if ((result = get_slot_lite(hContext, handle2, &val2))) {
			*pulHandleListSize = 0;
			goto done;
		}

		pVal2 = &val2;
	}

	if (*pulHandleListSize >= 1) {
		handle1 = *rghHandles[0];

		*pulHandleListSize = 0;

		if ((result = get_slot_lite(hContext, handle1, &val1)))
			goto done;

		pVal1 = &val1;
	}

	switch (unWrappedCommandOrdinal) {
	case TPM_ORD_EvictKey:
	case TPM_ORD_FlushSpecific:
	{
		if ((result = ctx_remove_key_loaded(hContext, handle1)))
			goto done;

		if ((result = key_mgr_dec_ref_count(handle1)))
			goto done;

		/* we can't call key_mgr_ref_cnt() here since it calls TPM_EvictKey directly */
		mc_set_slot_by_handle(handle1, NULL_TPM_HANDLE);
		break;
	}
	case TPM_ORD_OIAP:
	{
		/* are the maximum number of auth sessions open? */
		if (auth_mgr_req_new(hContext) == FALSE) {
			if ((result = auth_mgr_swap_out(hContext)))
				goto done;
		}

		break;
	}
	case TPM_ORD_OSAP:
	{
		UINT16 entityType;
		UINT32 entityValue, newEntValue;

		/* are the maximum number of auth sessions open? */
		if (auth_mgr_req_new(hContext) == FALSE) {
			if ((result = auth_mgr_swap_out(hContext)))
				goto done;
		}

		offset = 0;
		UnloadBlob_UINT16(&offset, &entityType, rgbWrappedCmdParamIn);
		UnloadBlob_UINT32(&offset, &entityValue, rgbWrappedCmdParamIn);

		if (entityType == TCPA_ET_KEYHANDLE || entityType == TCPA_ET_KEY) {
			if (ensureKeyIsLoaded(hContext, entityValue, &newEntValue))
				return TCSERR(TSS_E_KEY_NOT_LOADED);

			/* OSAP is never encrypted in a transport session, so changing
			 * rgbWrappedCmdParamIn is ok here */
			offset = sizeof(UINT16);
			LoadBlob_UINT32(&offset, newEntValue, rgbWrappedCmdParamIn);
		}

		break;
	}
	case TPM_ORD_DSAP:
	{
		UINT16 entityType;
		UINT32 keyHandle, tpmKeyHandle;

		/* are the maximum number of auth sessions open? */
		if (auth_mgr_req_new(hContext) == FALSE) {
			if ((result = auth_mgr_swap_out(hContext)))
				goto done;
		}

		offset = 0;
		UnloadBlob_UINT16(&offset, &entityType, rgbWrappedCmdParamIn);
		UnloadBlob_UINT32(&offset, &keyHandle, rgbWrappedCmdParamIn);

		if (ensureKeyIsLoaded(hContext, keyHandle, &tpmKeyHandle)) {
			result = TCSERR(TSS_E_KEY_NOT_LOADED);
			goto done;
		}

		/* DSAP's only encrypted paramter is entityValue, so replacing keyHandle inside
		 * rgbWrappedCmdParamIn is ok */
		offset = sizeof(UINT16);
		LoadBlob_UINT32(&offset, tpmKeyHandle, rgbWrappedCmdParamIn);
	}
	default:
		break;
	}

build_command:
	if ((result = tpm_rqu_build(TPM_ORD_ExecuteTransport, &wrappedOffset,
				    &txBlob[TSS_TXBLOB_WRAPPEDCMD_OFFSET], unWrappedCommandOrdinal,
				    pVal1, pVal2, ulWrappedCmdParamInSize, rgbWrappedCmdParamIn,
				    pWrappedCmdAuth1, pWrappedCmdAuth2)))
		goto done;

	/* The blob we'll load here looks like this:
	 *
	 * |TAGet|LENet|ORDet|wrappedCmdSize|wrappedCmd|AUTHet|
	 *
	 * wrappedCmd looks like this:
	 *
	 * |TAGw|LENw|ORDw|HANDLESw|DATAw|AUTH1w|AUTH2w|
	 *
	 * w = wrapped command info
	 * et = execute transport command info
	 *
	 * Note that the wrapped command was loaded into the blob by the tpm_rqu_build call
	 * above.
	 *
	 */
	offset = TSS_TPM_TXBLOB_HDR_LEN;
	/* Load wrapped command size: |wrappedCmdSize| */
	LoadBlob_UINT32(&offset, wrappedOffset, txBlob);

	/* offset + wrappedOffset is the position of the execute transport auth struct */
	offset += wrappedOffset;

	if (pTransAuth) {
		/* Load the auth for the execute transport command: |AUTHet| */
		LoadBlob_Auth(&offset, txBlob, pTransAuth);
		/* Load the outer header: |TAGet|LENet|ORDet| */
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset, TPM_ORD_ExecuteTransport,
				txBlob);
	} else {
		/* Load the outer header: |TAGet|LENet|ORDet| */
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, offset, TPM_ORD_ExecuteTransport, txBlob);
	}

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	/* Unload the Execute Transport (outer) header */
	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto done;
	}

	/* The response from the TPM looks like this:
	 *
	 * |TAGet|LENet|RCet|currentTicks|locality|wrappedRspSize|wrappedRsp|AUTHet|
	 *
	 * and wrappedRsp looks like:
	 *
	 * |TAGw|LENw|RCw|HANDLESw|DATAw|AUTH1w|AUTH2w|
	 */

	offset = TSS_TPM_TXBLOB_HDR_LEN;
	UnloadBlob_UINT64(&offset, punCurrentTicks, txBlob);
	UnloadBlob_UINT32(&offset, pbLocality, txBlob);

	/* Unload the wrapped response size: |wrappedRspSize| */
	UnloadBlob_UINT32(&offset, &wrappedSize, txBlob);

	/* We've parsed right up to wrappedRsp, so save off this offset for later */
	wrappedOffset = offset;

	/* The current offset + the response size will be the offset of |AUTHet| */
	offset += wrappedSize;
	if (pTransAuth)
		UnloadBlob_Auth(&offset, txBlob, pTransAuth);

	/* Now parse through the returned response @ wrappedOffset */
	if ((result = UnloadBlob_Header(&txBlob[wrappedOffset], &paramSize))) {
		LogDebugFn("Wrapped command (Ordinal 0x%x) failed: rc=0x%x",
			   unWrappedCommandOrdinal, result);

		/* This is the result of the wrapped command. If its not success, return its value
		 * in the pulWrappedCmdReturnCode variable and return indicating that the execute
		 * transport command was successful */
		*pulWrappedCmdReturnCode = result;
		*ulWrappedCmdParamOutSize = 0;
		*rgbWrappedCmdParamOut = NULL;
		auth_mgr_release_auth(pWrappedCmdAuth1, pWrappedCmdAuth2, hContext);

		return TSS_SUCCESS;
	}

	*pulWrappedCmdReturnCode = TSS_SUCCESS;

	pVal1 = pVal2 = NULL;
	switch (unWrappedCommandOrdinal) {
		/* The commands below have 1 outgoing handle */
		case TPM_ORD_LoadKey2:
			pVal1 = &val1;
			break;
		default:
			break;
	}

	result = tpm_rsp_parse(TPM_ORD_ExecuteTransport, &txBlob[wrappedOffset], paramSize,
			       pVal1, pVal2, ulWrappedCmdParamOutSize, rgbWrappedCmdParamOut,
			       pWrappedCmdAuth1, pWrappedCmdAuth2);

	offset = 0;
	switch (unWrappedCommandOrdinal) {
	case TPM_ORD_LoadKey2:
	{
		TCS_KEY_HANDLE tcs_handle = NULL_TCS_HANDLE;

		if ((result = load_key_final(hContext, handle1, &tcs_handle, NULL, val1)))
			goto done;

		*rghHandles[0] = tcs_handle;
		*pulHandleListSize = 1;
		break;
	}
	case TPM_ORD_DSAP:
	case TPM_ORD_OSAP:
	case TPM_ORD_OIAP:
	{
		UINT32 handle;

		UnloadBlob_UINT32(&offset, &handle, *rgbWrappedCmdParamOut);
		result = auth_mgr_add(hContext, handle);
		break;
	}
	default:
		break;
	}

done:
	auth_mgr_release_auth(pWrappedCmdAuth1, pWrappedCmdAuth2, hContext);
	return result;
}

TSS_RESULT
TCSP_ReleaseTransportSigned_Internal(TCS_CONTEXT_HANDLE      hContext,
				     TCS_KEY_HANDLE          hSignatureKey,
				     TPM_NONCE*              AntiReplayNonce,
				     TPM_AUTH*               pKeyAuth,		/* in, out */
				     TPM_AUTH*               pTransAuth,	/* in, out */
				     TPM_MODIFIER_INDICATOR* pbLocality,
				     UINT32*                 pulCurrentTicksSize,
				     BYTE**                  prgbCurrentTicks,
				     UINT32*                 pulSignatureSize,
				     BYTE**                  prgbSignature)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset;
	TPM_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (pKeyAuth) {
		if ((result = auth_mgr_check(hContext, &pKeyAuth->AuthHandle)))
			return result;
	}

	if ((result = ensureKeyIsLoaded(hContext, hSignatureKey, &keySlot)))
		return result;

	offset = TSS_TPM_TXBLOB_HDR_LEN;
	LoadBlob_UINT32(&offset, keySlot, txBlob);
	LoadBlob(&offset, sizeof(TPM_NONCE), txBlob, (BYTE *)AntiReplayNonce);
	if (pKeyAuth) {
		LoadBlob_Auth(&offset, txBlob, pKeyAuth);
		LoadBlob_Auth(&offset, txBlob, pTransAuth);
		LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, offset, TPM_ORD_ReleaseTransportSigned,
				txBlob);
	} else {
		LoadBlob_Auth(&offset, txBlob, pTransAuth);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset, TPM_ORD_ReleaseTransportSigned,
				txBlob);
	}

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto done;
	}

	/* unconditionally disable our accounting of the session */
	ctx_set_transport_disabled(hContext, NULL);

	offset = TSS_TPM_TXBLOB_HDR_LEN;
	UnloadBlob_UINT32(&offset, pbLocality, txBlob);

	*pulCurrentTicksSize = sizeof(TPM_STRUCTURE_TAG)
			       + sizeof(UINT64)
			       + sizeof(UINT16)
			       + sizeof(TPM_NONCE);

	*prgbCurrentTicks = malloc(*pulCurrentTicksSize);
	if (*prgbCurrentTicks == NULL) {
		result = TCSERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	UnloadBlob(&offset, *pulCurrentTicksSize, txBlob, *prgbCurrentTicks);
	UnloadBlob_UINT32(&offset, pulSignatureSize, txBlob);

	*prgbSignature = malloc(*pulSignatureSize);
	if (*prgbSignature == NULL) {
		free(*prgbCurrentTicks);
		result = TCSERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	UnloadBlob(&offset, *pulSignatureSize, txBlob, *prgbSignature);

	if (pKeyAuth)
		UnloadBlob_Auth(&offset, txBlob, pKeyAuth);
	UnloadBlob_Auth(&offset, txBlob, pTransAuth);

done:
	auth_mgr_release_auth(pKeyAuth, NULL, hContext);
	return result;
}
