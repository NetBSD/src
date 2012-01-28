
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
TCSP_ReadCurrentTicks_Internal(TCS_CONTEXT_HANDLE hContext,
			       UINT32*            pulCurrentTime,
			       BYTE**             prgbCurrentTime)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_GetTicks, &offset, txBlob, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result)
		result = tpm_rsp_parse(TPM_ORD_GetTicks, txBlob, paramSize, pulCurrentTime,
				       prgbCurrentTime, NULL);

done:
	return result;
}

TSS_RESULT
TCSP_TickStampBlob_Internal(TCS_CONTEXT_HANDLE hContext,
			    TCS_KEY_HANDLE     hKey,
			    TPM_NONCE*         antiReplay,
			    TPM_DIGEST*        digestToStamp,
			    TPM_AUTH*          privAuth,
			    UINT32*            pulSignatureLength,
			    BYTE**             prgbSignature,
			    UINT32*            pulTickCountLength,
			    BYTE**             prgbTickCount)
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	TPM_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (privAuth) {
		if ((result = auth_mgr_check(hContext, &privAuth->AuthHandle)))
			goto done;
	}

	if ((result = ensureKeyIsLoaded(hContext, hKey, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_TickStampBlob, &offset, txBlob, keySlot, antiReplay,
				    digestToStamp, privAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	if ((result = UnloadBlob_Header(txBlob, &paramSize))) {
		LogDebugFn("UnloadBlob_Header failed: rc=0x%x", result);
		goto done;
	}

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_TickStampBlob, txBlob, paramSize, pulTickCountLength,
				       prgbTickCount, pulSignatureLength, prgbSignature, privAuth);
	}
done:
	return result;
}

void
UnloadBlob_CURRENT_TICKS(UINT64 *offset, BYTE *b, TPM_CURRENT_TICKS *t)
{
	if (!t) {
		UnloadBlob_UINT16(offset, NULL, b);
		UnloadBlob_UINT64(offset, NULL, b);
		UnloadBlob_UINT16(offset, NULL, b);
		UnloadBlob(offset, sizeof(TPM_NONCE), b, NULL);

		return;
	}

	UnloadBlob_UINT16(offset, &t->tag, b);
	UnloadBlob_UINT64(offset, &t->currentTicks, b);
	UnloadBlob_UINT16(offset, &t->tickRate, b);
	UnloadBlob(offset, sizeof(TPM_NONCE), b, (BYTE *)&t->tickNonce);
}
