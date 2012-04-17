
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
TCSP_Sign_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		   TCS_KEY_HANDLE keyHandle,	/* in */
		   UINT32 areaToSignSize,	/* in */
		   BYTE * areaToSign,	/* in */
		   TPM_AUTH * privAuth,	/* in, out */
		   UINT32 * sigSize,	/* out */
		   BYTE ** sig	/* out */
    )
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Sign");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if (privAuth != NULL) {
		LogDebug("Auth Used");
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}

	if ((result = ensureKeyIsLoaded(hContext, keyHandle, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_Sign, &offset, txBlob, keySlot, areaToSignSize,
				    areaToSign, privAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Sign, txBlob, paramSize, sigSize, sig, privAuth,
				       NULL);
	}
	LogResult("sign", result);
done:
	auth_mgr_release_auth(privAuth, NULL, hContext);
	return result;
}

