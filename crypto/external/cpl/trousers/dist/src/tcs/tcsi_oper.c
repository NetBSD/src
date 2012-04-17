
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
TCSP_SetOperatorAuth_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			      TCPA_SECRET *operatorAuth)	/* in */
{
	TSS_RESULT result;
	UINT64 offset = 0;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_SetOperatorAuth, &offset, txBlob, TPM_AUTHDATA_SIZE,
				    operatorAuth->authdata)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	LogResult("SetOperatorAuth", result);
done:
	return result;
}

