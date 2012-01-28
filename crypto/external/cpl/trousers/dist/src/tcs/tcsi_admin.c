
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2007
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
TCSP_SetOwnerInstall_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TSS_BOOL state)	/* in  */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering SetOwnerInstall");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_SetOwnerInstall, &offset, txBlob, state, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("SetOwnerInstall", result);
	return result;
}

TSS_RESULT
TCSP_OwnerSetDisable_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TSS_BOOL disableState,	/* in */
			      TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_OwnerSetDisable, &offset, txBlob, disableState,
				    ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OwnerSetDisable, txBlob, paramSize, ownerAuth);
	}
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_DisableOwnerClear_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering DisableownerClear");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_DisableOwnerClear, &offset, txBlob, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_DisableOwnerClear, txBlob, paramSize, ownerAuth);
	}
	LogResult("DisableOwnerClear", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_ForceClear_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Force Clear");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_ForceClear, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("Force Clear", result);
	return result;
}

TSS_RESULT
TCSP_DisableForceClear_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Disable Force Clear");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_DisableForceClear, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("Disable Force Clear", result);
	return result;
}

TSS_RESULT
TCSP_PhysicalPresence_Internal(TCS_CONTEXT_HANDLE hContext, /* in */
			TCPA_PHYSICAL_PRESENCE fPhysicalPresence) /* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result = TCSERR(TSS_E_NOTIMPL);
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];
	char runlevel;

	runlevel = platform_get_runlevel();

	if (runlevel != 's' && runlevel != 'S' && runlevel != '1') {
		LogInfo("Physical Presence command denied: Must be in single"
				" user mode.");
		return TCSERR(TSS_E_NOTIMPL);
	}

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TSC_ORD_PhysicalPresence, &offset, txBlob, fPhysicalPresence)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	return UnloadBlob_Header(txBlob, &paramSize);
}

TSS_RESULT
TCSP_PhysicalDisable_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Physical Disable");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_PhysicalDisable, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("Physical Disable", result);

	return result;
}

TSS_RESULT
TCSP_PhysicalEnable_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Physical Enable");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_PhysicalEnable, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("Physical Enable", result);

	return result;
}

TSS_RESULT
TCSP_PhysicalSetDeactivated_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				     TSS_BOOL state)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Physical Set Deactivated");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_PhysicalSetDeactivated, &offset, txBlob, state, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("PhysicalSetDeactivated", result);
	return result;
}

TSS_RESULT
TCSP_SetTempDeactivated_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Set Temp Deactivated");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_SetTempDeactivated, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("SetTempDeactivated", result);

	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
TCSP_SetTempDeactivated2_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				  TPM_AUTH * operatorAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Set Temp Deactivated2");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if (operatorAuth) {
		if ((result = auth_mgr_check(hContext, &operatorAuth->AuthHandle)))
			return result;
	}

	if ((result = tpm_rqu_build(TPM_ORD_SetTempDeactivated, &offset, txBlob, operatorAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
			result = tpm_rsp_parse(TPM_ORD_SetTempDeactivated, txBlob, paramSize,
					       operatorAuth);
	}

	LogResult("SetTempDeactivated2", result);

done:
	auth_mgr_release_auth(operatorAuth, NULL, hContext);

	return result;
}
#endif

TSS_RESULT
TCSP_FieldUpgrade_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			   UINT32 dataInSize,	/* in */
			   BYTE * dataIn,	/* in */
			   UINT32 * dataOutSize,	/* out */
			   BYTE ** dataOut,	/* out */
			   TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Field Upgrade");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_FieldUpgrade, &offset, txBlob, dataInSize, dataInSize,
				    dataIn, ownerAuth, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_FieldUpgrade, txBlob, paramSize, dataOutSize,
				       dataOut, ownerAuth);
	}
	LogResult("Field Upgrade", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_SetRedirection_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			     TCS_KEY_HANDLE keyHandle,	/* in */
			     UINT32 c1,	/* in */
			     UINT32 c2,	/* in */
			     TPM_AUTH * privAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Set Redirection");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (privAuth != NULL) {
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	}

	if ((result = ensureKeyIsLoaded(hContext, keyHandle, &keySlot))) {
		result = TCSERR(TSS_E_FAIL);
		goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_SetRedirection, &offset, txBlob, keySlot, c1, c2,
				    privAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_SetRedirection, txBlob, paramSize, privAuth);
	}
	LogResult("Set Redirection", result);
done:
	auth_mgr_release_auth(privAuth, NULL, hContext);
	return result;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
TCSP_ResetLockValue_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			     TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_ResetLockValue, &offset, txBlob, ownerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ResetLockValue, txBlob, paramSize, ownerAuth);
	}

done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_FlushSpecific_Common(UINT32 tpmResHandle, TPM_RESOURCE_TYPE resourceType)
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = tpm_rqu_build(TPM_ORD_FlushSpecific, &offset, txBlob, tpmResHandle,
				    resourceType)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_FlushSpecific, txBlob, paramSize, NULL);
	}

	return result;
}

TSS_RESULT
TCSP_FlushSpecific_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TCS_HANDLE hResHandle,	/* in */
			    TPM_RESOURCE_TYPE resourceType) /* in */
{
	UINT32 tpmResHandle;
	TSS_RESULT result;

	if ((result = ctx_verify_context(hContext)))
		return result;

	switch (resourceType) {
		case TPM_RT_KEY:
			if ((result = get_slot_lite(hContext, hResHandle, &tpmResHandle)))
				return result;

			if ((result = ctx_remove_key_loaded(hContext, hResHandle)))
				return result;

			if ((result = key_mgr_dec_ref_count(hResHandle)))
				return result;
			break;
		case TPM_RT_AUTH:
			if ((result = auth_mgr_check(hContext, &hResHandle)))
				return result;

			auth_mgr_release_auth_handle(hResHandle, hContext, FALSE);
			/* fall through */
		case TPM_RT_TRANS:
		case TPM_RT_DAA_TPM:
			tpmResHandle = hResHandle;
			break;
		case TPM_RT_CONTEXT:
			result = TCSERR(TSS_E_NOTIMPL);
			goto done;
		default:
			LogDebugFn("Unknown resource type: 0x%x", resourceType);
			goto done;
	}

	result = TCSP_FlushSpecific_Common(tpmResHandle, resourceType);

done:
	return result;
}
#endif

