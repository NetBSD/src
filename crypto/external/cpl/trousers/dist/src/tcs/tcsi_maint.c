
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
TCSP_CreateMaintenanceArchive_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TSS_BOOL generateRandom,	/* in */
				       TPM_AUTH * ownerAuth,	/* in, out */
				       UINT32 * randomSize,	/* out */
				       BYTE ** random,	/* out */
				       UINT32 * archiveSize,	/* out */
				       BYTE ** archive)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Create Main Archive");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_CreateMaintenanceArchive, &offset, txBlob,
				    generateRandom, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CreateMaintenanceArchive, txBlob, paramSize,
				       randomSize, random, archiveSize, archive, ownerAuth);
	}
	LogResult("Create Main Archive", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_LoadMaintenanceArchive_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				     UINT32 dataInSize,	/* in */
				     BYTE * dataIn,	/* in */
				     TPM_AUTH * ownerAuth,	/* in, out */
				     UINT32 * dataOutSize,	/* out */
				     BYTE ** dataOut)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Load Maint Archive");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_LoadMaintenanceArchive, &offset, txBlob, dataInSize,
				    dataInSize, dataIn, ownerAuth, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_LoadMaintenanceArchive, txBlob, paramSize,
				       dataOutSize, dataOut, ownerAuth, NULL);
	}
	LogResult("Load Maint Archive", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_KillMaintenanceFeature_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				     TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_KillMaintenanceFeature, &offset, txBlob, ownerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_KillMaintenanceFeature, txBlob, paramSize,
				       ownerAuth);
	}
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_LoadManuMaintPub_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			       TCPA_NONCE antiReplay,	/* in */
			       UINT32 PubKeySize,	/* in */
			       BYTE * PubKey,	/* in */
			       TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Load Manu Maint Pub");

	if ((result = tpm_rqu_build(TPM_ORD_LoadManuMaintPub, &offset, txBlob, TPM_NONCE_SIZE,
				    antiReplay.nonce, PubKeySize, PubKey, NULL)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_LoadManuMaintPub, txBlob, paramSize, NULL,
				       checksum->digest);
	}
	LogResult("Load Manu Maint Pub", result);
	return result;
}

TSS_RESULT
TCSP_ReadManuMaintPub_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			       TCPA_NONCE antiReplay,	/* in */
			       TCPA_DIGEST * checksum)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering Read Manu Maint Pub");

	if ((result = tpm_rqu_build(TPM_ORD_ReadManuMaintPub, &offset, txBlob, TPM_NONCE_SIZE,
				    antiReplay.nonce)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ReadManuMaintPub, txBlob, paramSize, NULL,
				       checksum->digest);
	}
	LogResult("Read Manu Maint Pub", result);
	return result;
}

