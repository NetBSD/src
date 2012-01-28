
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
TCSP_DirWriteAuth_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			   TCPA_DIRINDEX dirIndex,	/* in */
			   TCPA_DIRVALUE newContents,	/* in */
			   TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering dirwriteauth");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if (dirIndex > tpm_metrics.num_dirs) {
		result = TCSERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_DirWriteAuth, &offset, txBlob, dirIndex,
				    TPM_DIGEST_SIZE, newContents.digest, ownerAuth, NULL)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_DirWriteAuth, txBlob, paramSize, ownerAuth);
	}
	LogResult("DirWriteAuth", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_DirRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		      TCPA_DIRINDEX dirIndex,	/* in */
		      TCPA_DIRVALUE * dirValue)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering DirRead");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if (dirValue == NULL)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if (dirIndex > tpm_metrics.num_dirs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = tpm_rqu_build(TPM_ORD_DirRead, &offset, txBlob, dirIndex, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_DirRead, txBlob, paramSize, NULL, dirValue->digest);
	}
	LogResult("DirRead", result);
	return result;
}

