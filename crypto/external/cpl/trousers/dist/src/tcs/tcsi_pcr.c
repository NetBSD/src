
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 * (C) Christian Kummer 2007
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
TCSP_Extend_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		     TCPA_PCRINDEX pcrNum,	/* in */
		     TCPA_DIGEST inDigest,	/* in */
		     TCPA_PCRVALUE * outDigest)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Extend");
	if ((result = ctx_verify_context(hContext)))
		return result;

	/* PCRs are numbered 0 - (NUM_PCRS - 1), thus the >= */
	if (pcrNum >= tpm_metrics.num_pcrs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if (tcsd_options.kernel_pcrs & (1 << pcrNum)) {
		LogInfo("PCR %d is configured to be kernel controlled. Extend request denied.",
				pcrNum);
		return TCSERR(TSS_E_FAIL);
	}

	if (tcsd_options.firmware_pcrs & (1 << pcrNum)) {
		LogInfo("PCR %d is configured to be firmware controlled. Extend request denied.",
				pcrNum);
		return TCSERR(TSS_E_FAIL);
	}

	if ((result = tpm_rqu_build(TPM_ORD_Extend, &offset, txBlob, pcrNum, TPM_DIGEST_SIZE,
				    inDigest.digest, NULL, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_Extend, txBlob, paramSize, NULL, outDigest->digest);
	}
	LogResult("Extend", result);
	return result;
}

TSS_RESULT
TCSP_PcrRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
		      TCPA_PCRINDEX pcrNum,		/* in */
		      TCPA_PCRVALUE * outDigest)	/* out */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering PCRRead");

	if ((result = ctx_verify_context(hContext)))
		return result;

	/* PCRs are numbered 0 - (NUM_PCRS - 1), thus the >= */
	if (pcrNum >= tpm_metrics.num_pcrs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if ((result = tpm_rqu_build(TPM_ORD_PcrRead, &offset, txBlob, pcrNum, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_PcrRead, txBlob, paramSize, NULL, outDigest->digest);
	}
	LogResult("PCR Read", result);
	return result;
}

TSS_RESULT
TCSP_PcrReset_Internal(TCS_CONTEXT_HANDLE hContext,      /* in */
		       UINT32 pcrDataSizeIn,             /* in */
		       BYTE * pcrDataIn)                 /* in */
{
	UINT64 offset = 0;
	TSS_RESULT result;
	UINT32 paramSize;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering PCRReset");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_PCR_Reset, &offset, txBlob, pcrDataSizeIn, pcrDataIn)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogResult("PCR Reset", result);
	return result;
}
