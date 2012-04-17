
/*
 * The Initial Developer of the Original Code is Intel Corporation.
 * Portions created by Intel Corporation are Copyright (C) 2007 Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 *
 * trousers - An open source TCG Software Stack
 *
 * Author: james.xu@intel.com Rossey.liu@intel.com
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
TCSP_NV_DefineOrReleaseSpace_Internal(TCS_CONTEXT_HANDLE hContext, /* in */
				      UINT32 cPubInfoSize,	/* in */
				      BYTE* pPubInfo,	/* in */
				      TPM_ENCAUTH encAuth,	/* in */
				      TPM_AUTH* pAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if (pAuth) {
		if ((result = auth_mgr_check(hContext, &pAuth->AuthHandle)))
			goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_NV_DefineSpace, &offset, txBlob, cPubInfoSize, pPubInfo,
				    TPM_ENCAUTH_SIZE, encAuth.authdata, pAuth)))
		return result;

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", offset);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%u) result=%u", paramSize, result);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_NV_DefineSpace, txBlob, paramSize, pAuth);
	}
done:
	LogDebug("Leaving DefineSpace with result:%u", result);
	auth_mgr_release_auth(pAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_NV_WriteValue_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    TSS_NV_INDEX hNVStore,	/* in */
			    UINT32 offset,		/* in */
			    UINT32 ulDataLength,	/* in */
			    BYTE * rgbDataToWrite,	/* in */
			    TPM_AUTH * privAuth)	/* in, out */
{
	UINT64 off_set = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ( (result = ctx_verify_context(hContext)))
		return result;
	if (privAuth) {
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_NV_WriteValue, &off_set, txBlob, hNVStore, offset,
				    ulDataLength, rgbDataToWrite, privAuth)))
		return result;

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", off_set);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%u) result=%u", paramSize, result);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_NV_WriteValue, txBlob, paramSize, privAuth);
	}
done:
	LogDebug("Leaving NVWriteValue with result:%u", result);
	auth_mgr_release_auth(privAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_NV_WriteValueAuth_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				TSS_NV_INDEX hNVStore,	/* in */
				UINT32 offset,		/* in */
				UINT32 ulDataLength,	/* in */
				BYTE * rgbDataToWrite,	/* in */
				TPM_AUTH * NVAuth)	/* in, out */
{
	UINT64 off_set = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ((result = ctx_verify_context(hContext)))
		return result;
	if ((result = auth_mgr_check(hContext, &NVAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_NV_WriteValueAuth, &off_set, txBlob, hNVStore, offset,
				    ulDataLength, rgbDataToWrite, NVAuth)))
		return result;

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", off_set);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%u) result=%u", paramSize, result);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_NV_WriteValueAuth, txBlob, paramSize, NVAuth);
	}
done:
	LogDebug("Leaving NVWriteValueAuth with result:%u", result);
	auth_mgr_release_auth(NVAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_NV_ReadValue_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			   TSS_NV_INDEX hNVStore,	/* in */
			   UINT32 offset,	/* in */
			   UINT32 * pulDataLength,	/* in, out */
			   TPM_AUTH * privAuth,	/* in, out */
			   BYTE ** rgbDataRead)	/* out */
{
	UINT64 off_set = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ((result = ctx_verify_context(hContext)))
		return result;

	if (privAuth) {
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_NV_ReadValue, &off_set, txBlob, hNVStore, offset,
				    *pulDataLength, privAuth)))
		return result;

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", off_set);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%u) result=%u", paramSize, result);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_NV_ReadValue, txBlob, paramSize, pulDataLength,
				       rgbDataRead, privAuth, NULL);
	}
done:
	LogDebug("Leaving NVReadValue with result:%u", result);
	auth_mgr_release_auth(privAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_NV_ReadValueAuth_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			       TSS_NV_INDEX hNVStore,	/* in */
			       UINT32 offset,		/* in */
			       UINT32 * pulDataLength,	/* in, out */
			       TPM_AUTH * NVAuth,	/* in, out */
			       BYTE ** rgbDataRead)	/* out */
{
	UINT64 off_set = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ((result = ctx_verify_context(hContext)))
		return result;
	if ((result = auth_mgr_check(hContext, &NVAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_NV_ReadValueAuth, &off_set, txBlob, hNVStore, offset,
				    *pulDataLength, NVAuth)))
		return result;

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", off_set);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%u) result=%u", paramSize, result);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_NV_ReadValueAuth, txBlob, paramSize, pulDataLength,
				       rgbDataRead, NVAuth, NULL);
	}
done:
	LogDebug("Leaving NVReadValueAuth with result:%u", result);
	auth_mgr_release_auth(NVAuth, NULL, hContext);
	return result;
}

