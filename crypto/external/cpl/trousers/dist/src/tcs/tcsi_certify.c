
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
TCSP_CertifyKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			 TCS_KEY_HANDLE certHandle,	/* in */
			 TCS_KEY_HANDLE keyHandle,	/* in */
			 TCPA_NONCE antiReplay,	/* in */
			 TPM_AUTH * certAuth,	/* in, out */
			 TPM_AUTH * keyAuth,	/* in, out */
			 UINT32 * CertifyInfoSize,	/* out */
			 BYTE ** CertifyInfo,	/* out */
			 UINT32 * outDataSize,	/* out */
			 BYTE ** outData)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE certKeySlot, keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Certify Key");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (certAuth != NULL) {
		LogDebug("Auth Used for Cert signing key");
		if ((result = auth_mgr_check(hContext, &certAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth used for Cert signing key");
	}

	if (keyAuth != NULL) {
		LogDebug("Auth Used for Key being signed");
		if ((result = auth_mgr_check(hContext, &keyAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth used for Key being signed");
	}

	if ((result = ensureKeyIsLoaded(hContext, certHandle, &certKeySlot)))
		goto done;

	if ((result = ensureKeyIsLoaded(hContext, keyHandle, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_CertifyKey, &offset, txBlob, certKeySlot, keySlot,
				    antiReplay.nonce, certAuth, keyAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CertifyKey, txBlob, paramSize, CertifyInfoSize,
				       CertifyInfo, outDataSize, outData, certAuth, keyAuth);
	}
	LogResult("Certify Key", result);
done:
	auth_mgr_release_auth(certAuth, keyAuth, hContext);
	return result;
}
