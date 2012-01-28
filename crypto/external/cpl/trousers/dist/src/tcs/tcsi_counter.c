
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
TCSP_ReadCounter_Internal(TCS_CONTEXT_HANDLE hContext,
			  TSS_COUNTER_ID     idCounter,
			  TPM_COUNTER_VALUE* counterValue)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_ReadCounter, &offset, txBlob, idCounter, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto out;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("TPM_ReadCounter failed: rc=0x%x", result);
		goto out;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ReadCounter, txBlob, paramSize, NULL, counterValue,
				       NULL);
	}

out:
	return result;
}

TSS_RESULT
TCSP_CreateCounter_Internal(TCS_CONTEXT_HANDLE hContext,
			    UINT32             LabelSize,
			    BYTE*              pLabel,
			    TPM_ENCAUTH        CounterAuth,
			    TPM_AUTH*          pOwnerAuth,
			    TSS_COUNTER_ID*    idCounter,
			    TPM_COUNTER_VALUE* counterValue)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if (LabelSize != 4) {
		LogDebugFn("BAD_PARAMETER: LabelSize != 4");
		return TCSERR(TSS_E_BAD_PARAMETER);
	}

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CreateCounter, &offset, txBlob, CounterAuth.authdata,
				    LabelSize, pLabel, pOwnerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto out;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("TPM_CreateCounter failed: rc=0x%x", result);
		goto out;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CreateCounter, txBlob, paramSize, idCounter,
				       counterValue, pOwnerAuth);
	}

out:
	return result;
}

TSS_RESULT
TCSP_IncrementCounter_Internal(TCS_CONTEXT_HANDLE hContext,
			       TSS_COUNTER_ID     idCounter,
			       TPM_AUTH*          pCounterAuth,
			       TPM_COUNTER_VALUE* counterValue)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &pCounterAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_IncrementCounter, &offset, txBlob, idCounter,
				    pCounterAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto out;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto out;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_IncrementCounter, txBlob, paramSize, NULL,
				       counterValue, pCounterAuth);
	}
out:
	return result;
}

TSS_RESULT
TCSP_ReleaseCounter_Internal(TCS_CONTEXT_HANDLE hContext,
			     TSS_COUNTER_ID     idCounter,
			     TPM_AUTH*          pCounterAuth)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &pCounterAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_ReleaseCounter, &offset, txBlob, idCounter,
				    pCounterAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto out;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto out;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ReleaseCounter, txBlob, paramSize, pCounterAuth);
	}
out:
	return result;
}

TSS_RESULT
TCSP_ReleaseCounterOwner_Internal(TCS_CONTEXT_HANDLE hContext,
				  TSS_COUNTER_ID     idCounter,
				  TPM_AUTH*          pOwnerAuth)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_ReleaseCounterOwner, &offset, txBlob, idCounter,
				    pOwnerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto out;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto out;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ReleaseCounterOwner, txBlob, paramSize, pOwnerAuth);
	}
out:
	return result;
}

