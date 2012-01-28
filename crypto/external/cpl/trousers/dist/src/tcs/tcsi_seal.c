
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
TCSP_Seal_Internal(UINT32 sealOrdinal,		/* in */
		   TCS_CONTEXT_HANDLE hContext,	/* in */
		   TCS_KEY_HANDLE keyHandle,	/* in */
		   TCPA_ENCAUTH encAuth,	/* in */
		   UINT32 pcrInfoSize,	/* in */
		   BYTE * PcrInfo,	/* in */
		   UINT32 inDataSize,	/* in */
		   BYTE * inData,	/* in */
		   TPM_AUTH * pubAuth,	/* in, out */
		   UINT32 * SealedDataSize,	/* out */
		   BYTE ** SealedData)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Seal");
	if (!pubAuth)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &pubAuth->AuthHandle)))
		goto done;

	if ((result = ensureKeyIsLoaded(hContext, keyHandle, &keySlot)))
		goto done;

	/* XXX What's this check for? */
	if (keySlot == 0) {
		result = TCSERR(TSS_E_FAIL);
		goto done;
	}

	if ((result = tpm_rqu_build(sealOrdinal, &offset, txBlob, keySlot, encAuth.authdata,
				    pcrInfoSize, PcrInfo, inDataSize, inData, pubAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(sealOrdinal, txBlob, paramSize, SealedDataSize,
				       SealedData, pubAuth);
	}
	LogResult("Seal", result);
done:
	auth_mgr_release_auth(pubAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_Unseal_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		     TCS_KEY_HANDLE parentHandle,	/* in */
		     UINT32 SealedDataSize,	/* in */
		     BYTE * SealedData,	/* in */
		     TPM_AUTH * parentAuth,	/* in, out */
		     TPM_AUTH * dataAuth,	/* in, out */
		     UINT32 * DataSize,	/* out */
		     BYTE ** Data)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Unseal");

	if (dataAuth == NULL)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (parentAuth != NULL) {
		LogDebug("Auth used");
		if ((result = auth_mgr_check(hContext, &parentAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}

	if ((result = auth_mgr_check(hContext, &dataAuth->AuthHandle)))
		goto done;

	if ((result = ensureKeyIsLoaded(hContext, parentHandle, &keySlot)))
		goto done;

	/* XXX What's this check for? */
	if (keySlot == 0) {
		result = TCSERR(TSS_E_FAIL);
		goto done;
	}

	if ((result = tpm_rqu_build(TPM_ORD_Unseal, &offset, txBlob, keySlot, SealedDataSize,
				    SealedData, parentAuth, dataAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Unseal, txBlob, paramSize, DataSize, Data,
				       parentAuth, dataAuth);
	}
	LogResult("Unseal", result);
done:
	auth_mgr_release_auth(parentAuth, dataAuth, hContext);
	return result;
}
