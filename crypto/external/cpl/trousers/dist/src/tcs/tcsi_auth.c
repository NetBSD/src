
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
TCSP_OIAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		   TCS_AUTHHANDLE *authHandle,	/* out */
		   TCPA_NONCE *nonce0)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering TCSI_OIAP");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_OIAP, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OIAP, txBlob, paramSize, authHandle, nonce0->nonce);
	}

	LogResult("OIAP", result);
	return result;
}

TSS_RESULT
TCSP_OSAP_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		   TCPA_ENTITY_TYPE entityType,	/* in */
		   UINT32 entityValue,	/* in */
		   TCPA_NONCE nonceOddOSAP,	/* in */
		   TCS_AUTHHANDLE * authHandle,	/* out */
		   TCPA_NONCE * nonceEven,	/* out */
		   TCPA_NONCE * nonceEvenOSAP)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering OSAP");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_OSAP, &offset, txBlob, entityType, entityValue,
				    nonceOddOSAP.nonce)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OSAP, txBlob, paramSize, authHandle,
				       nonceEven->nonce, nonceEvenOSAP->nonce);
	}
	LogResult("OSAP", result);

	return result;
}

TSS_RESULT
internal_TerminateHandle(TCS_AUTHHANDLE handle)
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = tpm_rqu_build(TPM_ORD_Terminate_Handle, &offset, txBlob, handle, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	return UnloadBlob_Header(txBlob, &paramSize);
}

