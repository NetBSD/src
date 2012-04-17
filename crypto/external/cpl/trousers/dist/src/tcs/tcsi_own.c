
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
TCSP_TakeOwnership_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			    UINT16 protocolID,	/* in */
			    UINT32 encOwnerAuthSize,	/* in  */
			    BYTE * encOwnerAuth,	/* in */
			    UINT32 encSrkAuthSize,	/* in */
			    BYTE * encSrkAuth,	/* in */
			    UINT32 srkInfoSize,	/*in */
			    BYTE * srkInfo,	/*in */
			    TPM_AUTH * ownerAuth,	/* in, out */
			    UINT32 * srkKeySize,	/*out */
			    BYTE ** srkKey)	/*out */
{
	UINT64 offset;
	UINT32 paramSize;
	TSS_RESULT result;
	TSS_KEY srkKeyContainer;
	BYTE fake_pubkey[256] = { 0, }, fake_srk[2048] = { 0, };
	BYTE oldAuthDataUsage;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	/* Check on the Atmel Bug Patch */
	offset = 0;
	UnloadBlob_TSS_KEY(&offset, srkInfo, &srkKeyContainer);
	oldAuthDataUsage = srkKeyContainer.authDataUsage;
	LogDebug("auth data usage is %.2X", oldAuthDataUsage);

	offset = 0;
	if ((result = tpm_rqu_build(TPM_ORD_TakeOwnership, &offset, txBlob, protocolID,
				    encOwnerAuthSize, encOwnerAuth, encSrkAuthSize, encSrkAuth,
				    srkInfoSize, srkInfo, ownerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		if ((result = tpm_rsp_parse(TPM_ORD_TakeOwnership, txBlob, paramSize, srkKeySize,
					    srkKey, ownerAuth)))
			goto done;

		offset = 0;
		if ((result = UnloadBlob_TSS_KEY(&offset, *srkKey, &srkKeyContainer))) {
			*srkKeySize = 0;
			free(*srkKey);
			goto done;
		}

		if (srkKeyContainer.authDataUsage != oldAuthDataUsage) {
			LogDebug("AuthDataUsage was changed by TPM.  Atmel Bug. Fixing it in PS");
			srkKeyContainer.authDataUsage = oldAuthDataUsage;
		}

#ifdef TSS_BUILD_PS
		{
			BYTE *save;

			/* Once the key file is created, it stays forever. There could be
			 * migratable keys in the hierarchy that are still useful to someone.
			 */
			result = ps_remove_key(&SRK_UUID);
			if (result != TSS_SUCCESS && result != TCSERR(TSS_E_PS_KEY_NOTFOUND)) {
				destroy_key_refs(&srkKeyContainer);
				LogError("Error removing SRK from key file.");
				*srkKeySize = 0;
				free(*srkKey);
				goto done;
			}

			/* Set the SRK pubkey to all 0's before writing the SRK to disk, this is for
			 * privacy reasons as outlined in the TSS spec */
			save = srkKeyContainer.pubKey.key;
			srkKeyContainer.pubKey.key = fake_pubkey;
			offset = 0;
			LoadBlob_TSS_KEY(&offset, fake_srk, &srkKeyContainer);

			if ((result = ps_write_key(&SRK_UUID, &NULL_UUID, NULL, 0, fake_srk,
						   offset))) {
				destroy_key_refs(&srkKeyContainer);
				LogError("Error writing SRK to disk");
				*srkKeySize = 0;
				free(*srkKey);
				goto done;
			}

			srkKeyContainer.pubKey.key = save;
		}
#endif
		if ((result = mc_add_entry_init(SRK_TPM_HANDLE, SRK_TPM_HANDLE, &srkKeyContainer,
					        &SRK_UUID))) {
			destroy_key_refs(&srkKeyContainer);
			LogError("Error creating SRK mem cache entry");
			*srkKeySize = 0;
			free(*srkKey);
		}
		destroy_key_refs(&srkKeyContainer);
	}
	LogResult("TakeOwnership", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_OwnerClear_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			 TPM_AUTH * ownerAuth)	/* in, out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering OwnerClear");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_OwnerClear, &offset, txBlob, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_OwnerClear, txBlob, paramSize, ownerAuth);
	}
	LogResult("Ownerclear", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;
}

