
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "authsess.h"


TSS_RESULT
Tspi_TPM_CollateIdentityRequest(TSS_HTPM hTPM,				/* in */
				TSS_HKEY hKeySRK,			/* in */
				TSS_HKEY hCAPubKey,			/* in */
				UINT32 ulIdentityLabelLength,		/* in */
				BYTE * rgbIdentityLabelData,		/* in */
				TSS_HKEY hIdentityKey,			/* in */
				TSS_ALGORITHM_ID algID,			/* in */
				UINT32 * pulTcpaIdentityReqLength,	/* out */
				BYTE ** prgbTcpaIdentityReq)		/* out */
{
#ifdef TSS_BUILD_TRANSPORT
	UINT32 transport;
#endif
	TPM_AUTH srkAuth;
	TCPA_RESULT result;
	UINT64 offset;
	BYTE hashblob[USHRT_MAX], idReqBlob[USHRT_MAX], testblob[USHRT_MAX];
	TCPA_DIGEST digest;
	TSS_HPOLICY hSRKPolicy, hIDPolicy, hCAPolicy;
	UINT32 caKeyBlobSize, idKeySize, idPubSize;
	BYTE *caKeyBlob, *idKey, *newIdKey, *idPub;
	TSS_KEY caKey;
	TCPA_CHOSENID_HASH chosenIDHash = { { 0, } };
	UINT32 pcIdentityBindingSize;
	BYTE *prgbIdentityBinding = NULL;
	UINT32 pcEndorsementCredentialSize;
	BYTE *prgbEndorsementCredential = NULL;
	UINT32 pcPlatformCredentialSize;
	BYTE *prgbPlatformCredential = NULL;
	UINT32 pcConformanceCredentialSize;
	BYTE *prgbConformanceCredential = NULL;
#define CHOSENID_BLOB_SIZE 2048
	BYTE chosenIDBlob[CHOSENID_BLOB_SIZE];
	TSS_HCONTEXT tspContext;
	UINT32 encSymKeySize = 256, tmp;
	BYTE encSymKey[256], *cb_var;
	TSS_BOOL usesAuth;
	TPM_AUTH *pSrkAuth = &srkAuth;
	TCPA_IDENTITY_REQ rgbTcpaIdentityReq;
	TCPA_KEY_PARMS symParms, asymParms;
	TCPA_SYMMETRIC_KEY symKey;
	int padding;
	TSS_CALLBACK *cb;
	Trspi_HashCtx hashCtx;
	UINT32 tempCredSize;
	BYTE *tempCred = NULL;
	struct authsess *xsap = NULL;

	if (pulTcpaIdentityReqLength == NULL || prgbTcpaIdentityReq == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_cb12(hTPM, TSS_TSPATTRIB_TPM_CALLBACK_COLLATEIDENTITY, &tmp,
				       &cb_var)))
		return result;

	cb = (TSS_CALLBACK *)cb_var;
	if (cb->callback == NULL) {
		free_tspi(tspContext, cb);
		cb = NULL;
	}

	/* Get Policies */
	if ((result = obj_rsakey_get_policy(hKeySRK, TSS_POLICY_USAGE, &hSRKPolicy, &usesAuth)))
		return result;

	if ((result = obj_rsakey_get_policy(hCAPubKey, TSS_POLICY_USAGE,
					    &hCAPolicy, NULL)))
		return result;

	if ((result = obj_rsakey_get_policy(hIdentityKey, TSS_POLICY_USAGE,
					   &hIDPolicy, NULL)))
		return result;

	/* setup the symmetric key's parms. */
	memset(&symParms, 0, sizeof(TCPA_KEY_PARMS));
	switch (algID) {
		case TSS_ALG_AES:
			symParms.algorithmID = TCPA_ALG_AES;
			symKey.algId = TCPA_ALG_AES;
			symKey.size = 128/8;
			break;
		case TSS_ALG_DES:
			symParms.algorithmID = TCPA_ALG_DES;
			symKey.algId = TCPA_ALG_DES;
			symKey.size = 64/8;
			break;
		case TSS_ALG_3DES:
			symParms.algorithmID = TCPA_ALG_3DES;
			symKey.algId = TCPA_ALG_3DES;
			symKey.size = 192/8;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
			goto error;
			break;
	}

	/* No symmetric key encryption schemes existed in the 1.1 time frame */
	symParms.encScheme = TCPA_ES_NONE;

	/* get the CA Pubkey's encryption scheme */
	if ((result = obj_rsakey_get_es(hCAPubKey, &tmp)))
		return TSPERR(TSS_E_BAD_PARAMETER);

	switch (tmp) {
		case TSS_ES_RSAESPKCSV15:
			padding = TR_RSA_PKCS1_PADDING;
			break;
		case TSS_ES_RSAESOAEP_SHA1_MGF1:
			padding = TR_RSA_PKCS1_OAEP_PADDING;
			break;
		case TSS_ES_NONE:
			/* fall through */
		default:
			padding = TR_RSA_NO_PADDING;
			break;
	}

	/* Get Key blobs */
	if ((result = obj_rsakey_get_blob(hIdentityKey, &idKeySize, &idKey)))
		return result;

	if ((result = obj_rsakey_get_blob(hCAPubKey, &caKeyBlobSize, &caKeyBlob)))
		return result;

	offset = 0;
	memset(&caKey, 0, sizeof(TSS_KEY));
	if ((result = UnloadBlob_TSS_KEY(&offset, caKeyBlob, &caKey)))
		return result;

	/* ChosenID hash =  SHA1(label || TCPA_PUBKEY(CApub)) */
	offset = 0;
	Trspi_LoadBlob(&offset, ulIdentityLabelLength, chosenIDBlob, rgbIdentityLabelData);
	Trspi_LoadBlob_KEY_PARMS(&offset, chosenIDBlob, &caKey.algorithmParms);
	Trspi_LoadBlob_STORE_PUBKEY(&offset, chosenIDBlob, &caKey.pubKey);

	if (offset > CHOSENID_BLOB_SIZE)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if ((result = Trspi_Hash(TSS_HASH_SHA1, offset, chosenIDBlob, chosenIDHash.digest))) {
		free_key_refs(&caKey);
		return result;
	}

	/* use chosenIDBlob temporarily */
	offset = 0;
	Trspi_LoadBlob_KEY_PARMS(&offset, chosenIDBlob, &caKey.algorithmParms);

	offset = 0;
	if ((result = Trspi_UnloadBlob_KEY_PARMS(&offset, chosenIDBlob, &asymParms)))
		return result;

	if ((result = authsess_xsap_init(tspContext, hTPM, hIdentityKey, TSS_AUTH_POLICY_REQUIRED,
					 TPM_ORD_MakeIdentity, TPM_ET_OWNER, &xsap)))
		return result;

	/* Hash the Auth data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_MakeIdentity);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	result |= Trspi_HashUpdate(&hashCtx, TCPA_SHA1_160_HASH_LEN, chosenIDHash.digest);
	result |= Trspi_HashUpdate(&hashCtx, idKeySize, idKey);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto error;

	/* Do the Auth's */
	if (usesAuth) {
		if ((result = secret_PerformAuth_OIAP(hKeySRK, TPM_ORD_MakeIdentity, hSRKPolicy,
						      FALSE, &digest, &srkAuth)))
			goto error;
		pSrkAuth = &srkAuth;
	} else {
		pSrkAuth = NULL;
	}

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto error;

#ifdef TSS_BUILD_TRANSPORT
	if ((result = obj_context_transport_get_control(tspContext, TSS_TSPATTRIB_ENABLE_TRANSPORT,
							&transport)))
		goto error;

	if (transport) {
		if ((result = Transport_MakeIdentity2(tspContext, xsap->encAuthUse, chosenIDHash,
						      idKeySize, idKey, pSrkAuth, xsap->pAuth,
						      &idKeySize, &newIdKey, &pcIdentityBindingSize,
						      &prgbIdentityBinding)))
			goto error;
	} else {
#endif
		if ((result = RPC_MakeIdentity(tspContext, xsap->encAuthUse, chosenIDHash,
					       idKeySize, idKey, pSrkAuth, xsap->pAuth, &idKeySize,
					       &newIdKey, &pcIdentityBindingSize,
					       &prgbIdentityBinding, &pcEndorsementCredentialSize,
					       &prgbEndorsementCredential,
					       &pcPlatformCredentialSize, &prgbPlatformCredential,
					       &pcConformanceCredentialSize,
					       &prgbConformanceCredential)))
			goto error;
#ifdef TSS_BUILD_TRANSPORT
	}
#endif

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_MakeIdentity);
	result |= Trspi_HashUpdate(&hashCtx, idKeySize, newIdKey);
	result |= Trspi_Hash_UINT32(&hashCtx, pcIdentityBindingSize);
	result |= Trspi_HashUpdate(&hashCtx, pcIdentityBindingSize, prgbIdentityBinding);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		free(newIdKey);
		goto error;
	}

	if ((result = authsess_xsap_verify(xsap, &digest))) {
		free(newIdKey);
		goto error;
	}

	if (usesAuth == TRUE) {
		if ((result = obj_policy_validate_auth_oiap(hSRKPolicy, &digest, &srkAuth))) {
			free(newIdKey);
			goto error;
		}
	}

	if ((result = obj_rsakey_set_tcpakey(hIdentityKey, idKeySize, newIdKey))) {
		free(newIdKey);
		goto error;
	}
	free(newIdKey);
	if ((result = obj_rsakey_set_tcs_handle(hIdentityKey, 0)))
		goto error;

	if ((result = obj_rsakey_get_pub_blob(hIdentityKey, &idPubSize, &idPub)))
		goto error;

	if ((result = obj_tpm_get_cred(hTPM, TSS_TPMATTRIB_EKCERT, &tempCredSize, &tempCred)))
		goto error;

	if (tempCred != NULL) {
		free(prgbEndorsementCredential);
		prgbEndorsementCredential = tempCred;
		pcEndorsementCredentialSize = tempCredSize;
	}

	if ((result = obj_tpm_get_cred(hTPM, TSS_TPMATTRIB_TPM_CC, &tempCredSize, &tempCred)))
		goto error;

	if (tempCred != NULL) {
		free(prgbConformanceCredential);
		prgbConformanceCredential = tempCred;
		pcConformanceCredentialSize = tempCredSize;
	}

	if ((result = obj_tpm_get_cred(hTPM, TSS_TPMATTRIB_PLATFORMCERT, &tempCredSize, &tempCred)))
		goto error;

	if (tempCred != NULL) {
		free(prgbPlatformCredential);
		prgbPlatformCredential = tempCred;
		pcPlatformCredentialSize = tempCredSize;
	}

	/* set up the TCPA_IDENTITY_PROOF structure */
	/* XXX This should be DER encoded first. TPM1.1b section 9.4 */
	/* XXX hash this incrementally using a Trspi_HashCtx */
	offset = 0;
	Trspi_LoadBlob_TSS_VERSION(&offset, hashblob, VERSION_1_1);
	Trspi_LoadBlob_UINT32(&offset, ulIdentityLabelLength, hashblob);
	Trspi_LoadBlob_UINT32(&offset, pcIdentityBindingSize, hashblob);
	Trspi_LoadBlob_UINT32(&offset, pcEndorsementCredentialSize, hashblob);
	Trspi_LoadBlob_UINT32(&offset, pcPlatformCredentialSize, hashblob);
	Trspi_LoadBlob_UINT32(&offset, pcConformanceCredentialSize, hashblob);
	Trspi_LoadBlob(&offset, idPubSize, hashblob, idPub);
	free_tspi(tspContext, idPub);
	Trspi_LoadBlob(&offset, ulIdentityLabelLength, hashblob, rgbIdentityLabelData);
	Trspi_LoadBlob(&offset, pcIdentityBindingSize, hashblob, prgbIdentityBinding);
	Trspi_LoadBlob(&offset, pcEndorsementCredentialSize, hashblob, prgbEndorsementCredential);
	Trspi_LoadBlob(&offset, pcPlatformCredentialSize, hashblob, prgbPlatformCredential);
	Trspi_LoadBlob(&offset, pcConformanceCredentialSize, hashblob, prgbConformanceCredential);

	if (cb && cb->callback) {
		/* Alloc the space for the callback to copy into. The additional 32 bytes will
		 * attempt to account for padding that the symmetric encryption will do. */
		rgbTcpaIdentityReq.asymBlob = calloc(1, (int)offset + 32);
		rgbTcpaIdentityReq.symBlob = calloc(1, (int)offset + 32);
		if (rgbTcpaIdentityReq.asymBlob == NULL ||
		    rgbTcpaIdentityReq.symBlob == NULL) {
			free(rgbTcpaIdentityReq.asymBlob);
			free(rgbTcpaIdentityReq.symBlob);
			LogError("malloc of %" PRIu64 " bytes failed", offset);
			free_tspi(tspContext, cb);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		rgbTcpaIdentityReq.asymSize = (UINT32)offset + 32;
		rgbTcpaIdentityReq.symSize = (UINT32)offset + 32;

		if ((result = ((TSS_RESULT (*)(PVOID, UINT32, BYTE *, UINT32, UINT32 *, BYTE *,
			       UINT32 *, BYTE *))cb->callback)(cb->appData, (UINT32)offset,
							       hashblob, algID,
							       &rgbTcpaIdentityReq.asymSize,
							       rgbTcpaIdentityReq.asymBlob,
							       &rgbTcpaIdentityReq.symSize,
							       rgbTcpaIdentityReq.symBlob))) {
			LogDebug("CollateIdentityRequest callback returned error 0x%x", result);
			free_tspi(tspContext, cb);
			goto error;
		}
	} else {
		/* generate the symmetric key. */
		if ((result = get_local_random(tspContext, TRUE, symKey.size, &symKey.data)))
			goto error;

		/* No symmetric key encryption schemes existed in the 1.1 time frame */
		symKey.encScheme = TCPA_ES_NONE;

		/* encrypt the proof */
		rgbTcpaIdentityReq.symSize = sizeof(testblob);
		if ((result = Trspi_SymEncrypt(algID, TR_SYM_MODE_CBC, symKey.data, NULL, hashblob,
					       offset, testblob, &rgbTcpaIdentityReq.symSize)))
			goto error;

		rgbTcpaIdentityReq.symBlob = testblob;

		/* XXX This should be DER encoded first. TPM1.1b section 9.4 */
		offset = 0;
		Trspi_LoadBlob_SYMMETRIC_KEY(&offset, hashblob, &symKey);

		if ((result = Trspi_RSA_Public_Encrypt(hashblob, offset, encSymKey, &encSymKeySize,
						       caKey.pubKey.key, caKey.pubKey.keyLength,
						       65537, padding)))
			goto error;

		rgbTcpaIdentityReq.asymSize = encSymKeySize;
		rgbTcpaIdentityReq.asymBlob = encSymKey;
	}

	rgbTcpaIdentityReq.asymAlgorithm = asymParms;
	rgbTcpaIdentityReq.symAlgorithm = symParms;

	/* XXX This should be DER encoded first. TPM1.1b section 9.4 */
	offset = 0;
	Trspi_LoadBlob_IDENTITY_REQ(&offset, idReqBlob, &rgbTcpaIdentityReq);

	if (cb && cb->callback) {
		free(rgbTcpaIdentityReq.symBlob);
		free(rgbTcpaIdentityReq.asymBlob);
		free_tspi(tspContext, cb);
	}

	if ((*prgbTcpaIdentityReq = calloc_tspi(tspContext, offset)) == NULL) {
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto error;
	}

	memcpy(*prgbTcpaIdentityReq, idReqBlob, offset);
	*pulTcpaIdentityReqLength = offset;
error:
	authsess_free(xsap);
	free_key_refs(&caKey);
	free(prgbIdentityBinding);
	free(prgbEndorsementCredential);
	free(prgbPlatformCredential);
	free(prgbConformanceCredential);

	return result;
}

TSS_RESULT
Tspi_TPM_ActivateIdentity(TSS_HTPM hTPM,			/* in */
			  TSS_HKEY hIdentKey,			/* in */
			  UINT32 ulAsymCAContentsBlobLength,	/* in */
			  BYTE * rgbAsymCAContentsBlob,		/* in */
			  UINT32 ulSymCAAttestationBlobLength,	/* in */
			  BYTE * rgbSymCAAttestationBlob,	/* in */
			  UINT32 * pulCredentialLength,		/* out */
			  BYTE ** prgbCredential)		/* out */
{
	TPM_AUTH idKeyAuth;
	TPM_AUTH ownerAuth;
	TSS_HCONTEXT tspContext;
	TSS_HPOLICY hIDPolicy, hTPMPolicy;
	UINT64 offset;
	BYTE credBlob[0x1000];
	TCPA_DIGEST digest;
	TSS_RESULT result;
	TCS_KEY_HANDLE tcsKeyHandle;
	TSS_BOOL usesAuth;
	TPM_AUTH *pIDKeyAuth;
	BYTE *symKeyBlob, *credCallback, *cb_var;
	UINT32 symKeyBlobLen, credLen, tmp;
	TCPA_SYMMETRIC_KEY symKey;
	TSS_CALLBACK *cb;
	Trspi_HashCtx hashCtx;
	TPM_SYM_CA_ATTESTATION symCAAttestation;

	if (pulCredentialLength == NULL || prgbCredential == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_cb12(hTPM, TSS_TSPATTRIB_TPM_CALLBACK_ACTIVATEIDENTITY, &tmp,
				       &cb_var)))
		return result;

	cb = (TSS_CALLBACK *)cb_var;
	if (cb->callback == NULL) {
		free_tspi(tspContext, cb);
		cb = NULL;
	}

	if ((result = obj_rsakey_get_tcs_handle(hIdentKey, &tcsKeyHandle)))
		return result;

	if ((result = obj_rsakey_get_policy(hIdentKey, TSS_POLICY_USAGE,
					    &hIDPolicy, &usesAuth)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hTPMPolicy)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ActivateIdentity);
	result |= Trspi_Hash_UINT32(&hashCtx, ulAsymCAContentsBlobLength);
	result |= Trspi_HashUpdate(&hashCtx, ulAsymCAContentsBlobLength, rgbAsymCAContentsBlob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if (usesAuth) {
		if ((result = secret_PerformAuth_OIAP(hIDPolicy, TPM_ORD_ActivateIdentity,
						      hIDPolicy, FALSE, &digest, &idKeyAuth)))
			return result;
		pIDKeyAuth = &idKeyAuth;
	} else {
		pIDKeyAuth = NULL;
	}

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_ActivateIdentity, hTPMPolicy, FALSE,
					      &digest, &ownerAuth)))
		return result;

	if ((result = TCS_API(tspContext)->ActivateTPMIdentity(tspContext, tcsKeyHandle,
							       ulAsymCAContentsBlobLength,
							       rgbAsymCAContentsBlob, pIDKeyAuth,
							       &ownerAuth, &symKeyBlobLen,
							       &symKeyBlob)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ActivateIdentity);
	result |= Trspi_HashUpdate(&hashCtx, symKeyBlobLen, symKeyBlob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if (usesAuth) {
		if ((result = obj_policy_validate_auth_oiap(hIDPolicy, &digest,
							    &idKeyAuth))) {
			LogDebugFn("Identity key auth validation of the symmetric key failed.");
			return result;
		}
	}

	if ((result = obj_policy_validate_auth_oiap(hTPMPolicy, &digest,
						    &ownerAuth))) {
		LogDebugFn("Owner auth validation of the symmetric key failed.");
		return result;
	}

	offset = 0;
	if ((result = Trspi_UnloadBlob_SYM_CA_ATTESTATION(&offset, rgbSymCAAttestationBlob,
							  &symCAAttestation))) {
		LogDebugFn("Error unloading CA's attestation blob.");
		return result;
	}

	if (cb && cb->callback) {
		/* alloc the space for the callback to copy into */
		credCallback = calloc(1, ulSymCAAttestationBlobLength);
		if (credCallback == NULL) {
			LogDebug("malloc of %u bytes failed", ulSymCAAttestationBlobLength);
			free(symKeyBlob);
			free_tspi(tspContext, cb);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		credLen = ulSymCAAttestationBlobLength;

		if ((result = ((TSS_RESULT (*)(PVOID, UINT32, BYTE *, UINT32, BYTE *, UINT32 *,
			       BYTE *))cb->callback)(cb->appData, symKeyBlobLen, symKeyBlob,
						     symCAAttestation.credSize,
						     symCAAttestation.credential,
						     &credLen, credCallback))) {
			LogDebug("ActivateIdentity callback returned error 0x%x", result);
			free(symCAAttestation.credential);
			free(symKeyBlob);
			free_tspi(tspContext, cb);
			free(credCallback);
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		free(symCAAttestation.credential);
		free_tspi(tspContext, cb);
		free(symKeyBlob);

		if ((*prgbCredential = calloc_tspi(tspContext, credLen)) == NULL) {
			free(credCallback);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		memcpy(*prgbCredential, credCallback, credLen);
		*pulCredentialLength = credLen;
		free(credCallback);

		return TSS_SUCCESS;
	}

	/* decrypt the symmetric blob using the recovered symmetric key */
	offset = 0;
	if ((result = Trspi_UnloadBlob_SYMMETRIC_KEY(&offset, symKeyBlob, &symKey))) {
		free(symCAAttestation.credential);
		free(symKeyBlob);
		return result;
	}
	free(symKeyBlob);

	if ((result = Trspi_SymDecrypt(symKey.algId, symKey.encScheme, symKey.data, NULL,
				       symCAAttestation.credential, symCAAttestation.credSize,
				       credBlob, &credLen))) {
		free(symCAAttestation.credential);
		free(symKey.data);
		return result;
	}
	free(symCAAttestation.credential);

	if ((*prgbCredential = calloc_tspi(tspContext, credLen)) == NULL) {
		free(symKey.data);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	free(symKey.data);
	memcpy(*prgbCredential, credBlob, credLen);
	*pulCredentialLength = credLen;

	return TSS_SUCCESS;
}
