
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
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
TCSP_GetCapability_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TCPA_CAPABILITY_AREA capArea,	/* in */
			    UINT32 subCapSize,	/* in */
			    BYTE * subCap,	/* in */
			    UINT32 * respSize,	/* out */
			    BYTE ** resp)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Get Cap");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_GetCapability, &offset, txBlob, capArea, subCapSize,
				    subCap, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_GetCapability, txBlob, paramSize, respSize, resp,
				       NULL, NULL);
	}
	LogResult("Get Cap", result);
	return result;
}

TSS_RESULT
TCSP_GetCapabilityOwner_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				 TPM_AUTH * pOwnerAuth,		/* in / out */
				 TCPA_VERSION * pVersion,	/* out */
				 UINT32 * pNonVolatileFlags,	/* out */
				 UINT32 * pVolatileFlags)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Getcap owner");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_GetCapabilityOwner, &offset, txBlob, pOwnerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_GetCapabilityOwner, txBlob, paramSize, pVersion,
				       pNonVolatileFlags, pVolatileFlags, pOwnerAuth);
	}

	LogResult("GetCapowner", result);
done:
	auth_mgr_release_auth(pOwnerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_SetCapability_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TCPA_CAPABILITY_AREA capArea,	/* in */
			    UINT32 subCapSize,	/* in */
			    BYTE * subCap,	/* in */
			    UINT32 valueSize,	/* in */
			    BYTE * value,	/* in */
			    TPM_AUTH * pOwnerAuth)	/* in, out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_SetCapability, &offset, txBlob, capArea, subCapSize,
				    subCap, valueSize, value, pOwnerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_SetCapability, txBlob, paramSize, pOwnerAuth);
	}

done:
	auth_mgr_release_auth(pOwnerAuth, NULL, hContext);
	return result;
}

