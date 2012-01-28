
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
TCSP_DaaJoin_internal(TCS_CONTEXT_HANDLE hContext, /* in */
		      TPM_HANDLE handle, /* in */
		      BYTE stage,               /* in */
		      UINT32 inputSize0,   /* in */
		      BYTE *inputData0,   /* in */
		      UINT32 inputSize1, /* in */
		      BYTE *inputData1, /* in */
		      TPM_AUTH * ownerAuth,	/* in, out */
		      UINT32 *outputSize, /* out */
		      BYTE **outputData)  /* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ( (result = ctx_verify_context(hContext)) != TSS_SUCCESS)
		return result;
	if( (result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)) != TSS_SUCCESS)
		goto done;

#if 0
	offset = 10;
	LoadBlob_UINT32( &offset, handle, txBlob);
	LogDebug("load BYTE: stage: %x", stage);
	LoadBlob( &offset, sizeof(BYTE), txBlob, &stage);

	LogDebug("load UNIT32: inputSize0: %x  (oldOffset=%" PRIu64 ")", inputSize0, offset);
	LoadBlob_UINT32(&offset, inputSize0, txBlob);
	LogDebug("load Data: inputData0: %X   (oldOffset=%" PRIu64 ")", (int)inputData0, offset);
	LoadBlob(&offset, inputSize0, txBlob, inputData0);
	LogDebug("load UINT32: inputSize1:%x  (oldOffset=%" PRIu64 ")", inputSize1, offset);
	LoadBlob_UINT32(&offset, inputSize1, txBlob);
	if( inputSize1>0) {
		LogDebug("load Data: inputData1: %X  (oldOffset=%" PRIu64 ")", (int)inputData1, offset);
		LoadBlob(&offset, inputSize1, txBlob, inputData1);
	}
	LogDebug("load Auth: ownerAuth: %X  (oldOffset=%" PRIu64 ")", (int)ownerAuth, offset);
	LoadBlob_Auth(&offset, txBlob, ownerAuth);

	LogDebug("load Header: ordinal: %X  (oldOffset=%" PRIu64 ")", TPM_ORD_DAA_Join, offset);
	LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset, TPM_ORD_DAA_Join, txBlob);
#else
	if ((result = tpm_rqu_build(TPM_ORD_DAA_Join, &offset, txBlob, handle, stage, inputSize0,
				    inputData0, inputSize1, inputData1, ownerAuth)))
		goto done;
#endif

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", offset);
	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%d) result=%d", paramSize, result);
	if (!result) {
#if 0
		offset = 10;
		UnloadBlob_UINT32( &offset, outputSize, txBlob);
		LogDebug("Unload outputSize=%d", *outputSize);
		*outputData = malloc(*outputSize);
		if( *outputData == NULL) {
			LogError("malloc of %u bytes failed.", *outputSize);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebug("Unload outputData");
		UnloadBlob( &offset, *outputSize, txBlob, *outputData);
		LogDebug("Unload Auth");
		UnloadBlob_Auth(&offset, txBlob, ownerAuth);
#else
		result = tpm_rsp_parse(TPM_ORD_DAA_Join, txBlob, paramSize, outputSize, outputData,
				       ownerAuth);
#endif
	}
done:
	LogDebug("Leaving DaaJoin with result:%d", result);
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT TCSP_DaaSign_internal(TCS_CONTEXT_HANDLE hContext, /* in */
				 TPM_HANDLE handle, /* in */
				 BYTE stage,               /* in */
				 UINT32 inputSize0,   /* in */
				 BYTE *inputData0,   /* in */
				 UINT32 inputSize1, /* in */
				 BYTE *inputData1, /* in */
				 TPM_AUTH * ownerAuth,	/* in, out */
				 UINT32 *outputSize, /* out */
				 BYTE **outputData)  /* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");
	if ( (result = ctx_verify_context(hContext)) != TSS_SUCCESS)
		return result;

	if( (result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)) != TSS_SUCCESS)
		goto done;

#if 0
	offset = 10;
	LoadBlob_UINT32( &offset, handle, txBlob);
	LogDebug("load BYTE: stage: %x", stage);
	LoadBlob( &offset, sizeof(BYTE), txBlob, &stage);

	LogDebug("load UNIT32: inputSize0: %x  (oldOffset=%" PRIu64 ")", inputSize0, offset);
	LoadBlob_UINT32(&offset, inputSize0, txBlob);
	LogDebug("load Data: inputData0: %X   (oldOffset=%" PRIu64 ")", (int)inputData0, offset);
	LoadBlob(&offset, inputSize0, txBlob, inputData0);
	LogDebug("load UINT32: inputSize1:%x  (oldOffset=%" PRIu64 ")", inputSize1, offset);
	LoadBlob_UINT32(&offset, inputSize1, txBlob);
	if( inputSize1>0) {
		LogDebug("load Data: inputData1: %X  (oldOffset=%" PRIu64 ")", (int)inputData1, offset);
		LoadBlob(&offset, inputSize1, txBlob, inputData1);
	}
	LogDebug("load Auth: ownerAuth: %X  (oldOffset=%" PRIu64 ")", (int)ownerAuth, offset);
	LoadBlob_Auth(&offset, txBlob, ownerAuth);

	LogDebug("load Header: ordinal: %X  (oldOffset=%" PRIu64 ")", TPM_ORD_DAA_Sign, offset);
	LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, offset, TPM_ORD_DAA_Sign, txBlob);
#else
	if ((result = tpm_rqu_build(TPM_ORD_DAA_Sign, &offset, txBlob, handle, stage, inputSize0,
				    inputData0, inputSize1, inputData1, ownerAuth)))
		goto done;
#endif

	LogDebug("req_mgr_submit_req  (oldOffset=%" PRIu64 ")", offset);
	if ((result = req_mgr_submit_req(txBlob))) goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	LogDebug("UnloadBlob  (paramSize=%d) result=%d", paramSize, result);
	if (!result) {
#if 0
		offset = 10;
		UnloadBlob_UINT32( &offset, outputSize, txBlob);
		LogDebug("Unload outputSize=%d", *outputSize);
		*outputData = malloc(*outputSize);
		if( *outputData == NULL) {
			LogError("malloc of %u bytes failed.", *outputSize);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebug("Unload outputData");
		UnloadBlob(&offset, *outputSize, txBlob, *outputData);
		LogDebug("Unload Auth");
		UnloadBlob_Auth(&offset, txBlob, ownerAuth);
#else
		result = tpm_rsp_parse(TPM_ORD_DAA_Sign, txBlob, paramSize, outputSize, outputData,
				       ownerAuth);
#endif
	}
done:
	LogDebug("Leaving DaaSign with result:%d", result);
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

