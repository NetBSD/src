
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
TCSP_SelfTestFull_Internal(TCS_CONTEXT_HANDLE hContext)	/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Self Test Full");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_SelfTestFull, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("Self Test Full", result);
	return result;
}

TSS_RESULT
TCSP_CertifySelfTest_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TCS_KEY_HANDLE keyHandle,	/* in */
			      TCPA_NONCE antiReplay,	/* in */
			      TPM_AUTH * privAuth,	/* in, out */
			      UINT32 * sigSize,	/* out */
			      BYTE ** sig)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Certify Self Test");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (privAuth != NULL) {
		LogDebug("Auth Used");
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}

	if ((result = ensureKeyIsLoaded(hContext, keyHandle, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_CertifySelfTest, &offset, txBlob, keySlot,
				    TPM_NONCE_SIZE, antiReplay.nonce, privAuth, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CertifySelfTest, txBlob, paramSize, sigSize, sig,
				       privAuth, NULL);
	}
	LogResult("Certify Self Test", result);
done:
	auth_mgr_release_auth(privAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_GetTestResult_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    UINT32 * outDataSize,	/* out */
			    BYTE ** outData)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Get Test Result");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_GetTestResult, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_GetTestResult, txBlob, paramSize, outDataSize,
				       outData, NULL, NULL);
	}
	LogResult("Get Test Result", result);
	return result;
}

