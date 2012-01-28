
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2007
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
TCSP_CreateEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TCPA_NONCE antiReplay,	/* in */
				       UINT32 endorsementKeyInfoSize,	/* in */
				       BYTE * endorsementKeyInfo,	/* in */
				       UINT32 * endorsementKeySize,	/* out */
				       BYTE ** endorsementKey,	/* out */
				       TCPA_DIGEST * checksum)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CreateEndorsementKeyPair, &offset, txBlob,
				    antiReplay.nonce, endorsementKeyInfoSize,
				    endorsementKeyInfo)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CreateEndorsementKeyPair, txBlob, paramSize,
				       endorsementKeySize, endorsementKey, checksum->digest);
	}
	LogDebug("Leaving CreateEKPair with result: 0x%x", result);
	return result;
}

TSS_RESULT
TCSP_ReadPubek_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			TCPA_NONCE antiReplay,	/* in */
			UINT32 * pubEndorsementKeySize,	/* out */
			BYTE ** pubEndorsementKey,	/* out */
			TCPA_DIGEST * checksum)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebugFn("Enter");

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_ReadPubek, &offset, txBlob, TPM_NONCE_SIZE,
				    antiReplay.nonce)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ReadPubek, txBlob, paramSize, pubEndorsementKeySize,
				       pubEndorsementKey, checksum->digest);
	}
	LogDebugFn("result: 0x%x", result);
	return result;
}

TSS_RESULT
TCSP_DisablePubekRead_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			       TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("DisablePubekRead");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_DisablePubekRead, &offset, txBlob, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_DisablePubekRead, txBlob, paramSize, ownerAuth);
	}
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_OwnerReadPubek_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			     TPM_AUTH * ownerAuth,	/* in, out */
			     UINT32 * pubEndorsementKeySize,	/* out */
			     BYTE ** pubEndorsementKey)	/* out */
{
	UINT32 paramSize;
	TSS_RESULT result;
	UINT64 offset = 0;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering OwnerReadPubek");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_OwnerReadPubek, &offset, txBlob, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OwnerReadPubek, txBlob, paramSize,
				       pubEndorsementKeySize, pubEndorsementKey, ownerAuth);
	}
	LogResult("Owner Read Pubek", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_CreateRevocableEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
						TPM_NONCE antiReplay,	/* in */
						UINT32 endorsementKeyInfoSize,	/* in */
						BYTE * endorsementKeyInfo,	/* in */
						TSS_BOOL genResetAuth,	/* in */
						TPM_DIGEST * eKResetAuth, /* in, out */
						UINT32 * endorsementKeySize,	/* out */
						BYTE ** endorsementKey,	/* out */
						TPM_DIGEST * checksum)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_CreateRevocableEK, &offset, txBlob,
				    antiReplay.nonce, endorsementKeyInfoSize,
				    endorsementKeyInfo, genResetAuth, eKResetAuth->digest)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_CreateRevocableEK, txBlob, paramSize,
				endorsementKeySize, endorsementKey, checksum->digest,
				eKResetAuth->digest);
	}

	LogDebug("Leaving CreateRevocableEKPair with result: 0x%x", result);
	return result;
}

TSS_RESULT
TCSP_RevokeEndorsementKeyPair_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				       TPM_DIGEST EKResetAuth)		/* in */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((result = tpm_rqu_build(TPM_ORD_RevokeTrust, &offset, txBlob, EKResetAuth.digest)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		return result;

	result = UnloadBlob_Header(txBlob, &paramSize);

	LogDebug("Leaving RevokeEKPair with result: 0x%x", result);
	return result;
}

