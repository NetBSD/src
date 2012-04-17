
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
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsps.h"
#include "req_mgr.h"
#include "tcs_aik.h"


TSS_RESULT
TCSP_MakeIdentity_Internal(TCS_CONTEXT_HANDLE hContext,			/* in  */
			   TCPA_ENCAUTH identityAuth,			/* in */
			   TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
			   UINT32 idKeyInfoSize,			/* in */
			   BYTE * idKeyInfo,				/* in */
			   TPM_AUTH * pSrkAuth,				/* in, out */
			   TPM_AUTH * pOwnerAuth,			/* in, out */
			   UINT32 * idKeySize,				/* out */
			   BYTE ** idKey,				/* out */
			   UINT32 * pcIdentityBindingSize,		/* out */
			   BYTE ** prgbIdentityBinding,			/* out */
			   UINT32 * pcEndorsementCredentialSize,	/* out */
			   BYTE ** prgbEndorsementCredential,		/* out */
			   UINT32 * pcPlatformCredentialSize,		/* out */
			   BYTE ** prgbPlatformCredential,		/* out */
			   UINT32 * pcConformanceCredentialSize,	/* out */
			   BYTE ** prgbConformanceCredential)		/* out */
{
	UINT64 offset;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (pSrkAuth != NULL) {
		LogDebug("SRK Auth Used");
		if ((result = auth_mgr_check(hContext, &pSrkAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No SRK Auth");
	}

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		goto done;

	offset = 0;
	if ((result = tpm_rqu_build(TPM_ORD_MakeIdentity, &offset, txBlob, identityAuth.authdata,
				    IDLabel_PrivCAHash.digest, idKeyInfoSize, idKeyInfo, pSrkAuth,
				    pOwnerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		if ((result = tpm_rsp_parse(TPM_ORD_MakeIdentity, txBlob, paramSize, idKeySize,
					    idKey, pcIdentityBindingSize, prgbIdentityBinding,
					    pSrkAuth, pOwnerAuth)))
			goto done;

		/* If an error occurs, these will return NULL */
		get_credential(TSS_TCS_CREDENTIAL_PLATFORMCERT, pcPlatformCredentialSize,
			       prgbPlatformCredential);
		get_credential(TSS_TCS_CREDENTIAL_TPM_CC, pcConformanceCredentialSize,
			       prgbConformanceCredential);
		get_credential(TSS_TCS_CREDENTIAL_EKCERT, pcEndorsementCredentialSize,
			       prgbEndorsementCredential);
	}
	LogResult("Make Identity", result);
done:
	auth_mgr_release_auth(pSrkAuth, pOwnerAuth, hContext);
	return result;
}

TSS_RESULT
TCSP_ActivateTPMIdentity_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				  TCS_KEY_HANDLE idKey,	/* in */
				  UINT32 blobSize,	/* in */
				  BYTE * blob,	/* in */
				  TPM_AUTH * idKeyAuth,	/* in, out */
				  TPM_AUTH * ownerAuth,	/* in, out */
				  UINT32 * SymmetricKeySize,	/* out */
				  BYTE ** SymmetricKey)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 paramSize;
	UINT32 keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("TCSP_ActivateTPMIdentity");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (idKeyAuth != NULL) {
		if ((result = auth_mgr_check(hContext, &idKeyAuth->AuthHandle)))
			goto done;
	}
	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	if ((result = ensureKeyIsLoaded(hContext, idKey, &keySlot)))
		goto done;

	offset = 0;
	if ((result = tpm_rqu_build(TPM_ORD_ActivateIdentity, &offset, txBlob, keySlot, blobSize,
				    blob, idKeyAuth, ownerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		if ((result = tpm_rsp_parse(TPM_ORD_ActivateIdentity, txBlob, paramSize,
					    SymmetricKeySize, SymmetricKey, idKeyAuth, ownerAuth)))
			goto done;
	}

done:
	auth_mgr_release_auth(idKeyAuth, ownerAuth, hContext);
	return result;
}

TSS_RESULT
TCS_GetCredential_Internal(TCS_CONTEXT_HANDLE hContext,		/* in  */
			   UINT32 ulCredentialType,		/* in */
			   UINT32 ulCredentialAccessMode,	/* in */
			   UINT32 * pulCredentialSize,		/* out */
			   BYTE ** prgbCredentialData)		/* out */
{
	TSS_RESULT result;

	if ((result = ctx_verify_context(hContext)))
		return result;

	if ((ulCredentialType != TSS_TCS_CREDENTIAL_EKCERT) &&
	    (ulCredentialType != TSS_TCS_CREDENTIAL_TPM_CC) &&
	    (ulCredentialType != TSS_TCS_CREDENTIAL_PLATFORMCERT)) {
		LogError("GetCredential - Unsupported Credential Type");
		return TCSERR(TSS_E_BAD_PARAMETER);
	}

	if (ulCredentialAccessMode == TSS_TCS_CERT_ACCESS_AUTO) {
		get_credential(ulCredentialType, pulCredentialSize, prgbCredentialData);
	} else {
		LogError("GetCredential - Unsupported Credential Access Mode");
		return TCSERR(TSS_E_FAIL);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
TCSP_MakeIdentity2_Internal(TCS_CONTEXT_HANDLE hContext,		/* in  */
			    TCPA_ENCAUTH identityAuth,			/* in */
			    TCPA_CHOSENID_HASH IDLabel_PrivCAHash,	/* in */
			    UINT32 idKeyInfoSize,			/* in */
			    BYTE * idKeyInfo,				/* in */
			    TPM_AUTH * pSrkAuth,			/* in, out */
			    TPM_AUTH * pOwnerAuth,			/* in, out */
			    UINT32 * idKeySize,				/* out */
			    BYTE ** idKey,				/* out */
			    UINT32 * pcIdentityBindingSize,		/* out */
			    BYTE ** prgbIdentityBinding)		/* out */
{
	UINT64 offset;
	UINT32 paramSize;
	TSS_RESULT result;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (pSrkAuth) {
		if ((result = auth_mgr_check(hContext, &pSrkAuth->AuthHandle)))
			goto done;
	}

	if ((result = auth_mgr_check(hContext, &pOwnerAuth->AuthHandle)))
		goto done;

	offset = 0;
	if ((result = tpm_rqu_build(TPM_ORD_MakeIdentity, &offset, txBlob, identityAuth.authdata,
				    IDLabel_PrivCAHash.digest, idKeyInfoSize, idKeyInfo, pSrkAuth,
				    pOwnerAuth)))
		goto done;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		if ((result = tpm_rsp_parse(TPM_ORD_MakeIdentity, txBlob, paramSize, idKeySize,
					    idKey, pcIdentityBindingSize, prgbIdentityBinding,
					    pSrkAuth, pOwnerAuth)))
			goto done;
	}
	LogResult("Make Identity", result);
done:
	auth_mgr_release_auth(pSrkAuth, pOwnerAuth, hContext);
	return result;
}

