
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
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsplog.h"
#include "tsp_delegate.h"
#include "authsess.h"


TSS_RESULT
do_delegate_manage(TSS_HTPM hTpm, UINT32 familyID, UINT32 opFlag,
		   UINT32 opDataSize, BYTE *opData, UINT32 *outDataSize, BYTE **outData)
{
	TSS_HCONTEXT hContext;
	TSS_HPOLICY hPolicy;
	UINT32 secretMode = TSS_SECRET_MODE_NONE;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	TPM_AUTH ownerAuth, *pAuth;
	UINT32 retDataSize;
	BYTE *retData = NULL;
	TSS_RESULT result;

	if ((result = obj_tpm_get_tsp_context(hTpm, &hContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTpm, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	if (hPolicy != NULL_HPOLICY) {
		if ((result = obj_policy_get_mode(hPolicy, &secretMode)))
			return result;
	}

	if (secretMode != TSS_SECRET_MODE_NONE) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_Manage);
		result |= Trspi_Hash_UINT32(&hashCtx, familyID);
		result |= Trspi_Hash_UINT32(&hashCtx, opFlag);
		result |= Trspi_Hash_UINT32(&hashCtx, opDataSize);
		result |= Trspi_HashUpdate(&hashCtx, opDataSize, opData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			return result;

		pAuth = &ownerAuth;
		if ((result = secret_PerformAuth_OIAP(hTpm, TPM_ORD_Delegate_Manage, hPolicy, FALSE,
						      &digest, pAuth)))
			return result;
	} else
		pAuth = NULL;

	/* Perform the delegation operation */
	if ((result = TCS_API(hContext)->Delegate_Manage(hContext, familyID, opFlag, opDataSize,
							 opData, pAuth, &retDataSize, &retData)))
		return result;

	if (pAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_Manage);
		result |= Trspi_Hash_UINT32(&hashCtx, retDataSize);
		result |= Trspi_HashUpdate(&hashCtx, retDataSize, retData);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
			free(retData);
			goto done;
		}

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth))) {
			free(retData);
			goto done;
		}
	}

	*outDataSize = retDataSize;
	*outData = retData;

done:
	return result;
}

TSS_RESULT
create_owner_delegation(TSS_HTPM       hTpm,
			BYTE           bLabel,
			UINT32         ulFlags,
			TSS_HPCRS      hPcrs,
			TSS_HDELFAMILY hFamily,
			TSS_HPOLICY    hDelegation)
{
	TSS_HCONTEXT hContext;
	TSS_BOOL incrementCount = FALSE;
	UINT32 type;
	UINT32 publicInfoSize;
	BYTE *publicInfo = NULL;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	UINT32 blobSize;
	BYTE *blob = NULL;
	TSS_RESULT result;
	struct authsess *xsap = NULL;

	if ((result = obj_tpm_get_tsp_context(hTpm, &hContext)))
		return result;

	if ((ulFlags & ~TSS_DELEGATE_INCREMENTVERIFICATIONCOUNT) > 0)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (ulFlags & TSS_DELEGATE_INCREMENTVERIFICATIONCOUNT)
		incrementCount = TRUE;

	if ((result = obj_policy_get_delegation_type(hDelegation, &type)))
		return result;

	if (type != TSS_DELEGATIONTYPE_OWNER)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = __tspi_build_delegate_public_info(bLabel, hPcrs, hFamily, hDelegation,
			&publicInfoSize, &publicInfo)))
		return result;

	if ((result = authsess_xsap_init(hContext, hTpm, hDelegation, TSS_AUTH_POLICY_NOT_REQUIRED,
					 TPM_ORD_Delegate_CreateOwnerDelegation, TPM_ET_OWNER,
					 &xsap)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_CreateOwnerDelegation);
	result |= Trspi_Hash_BOOL(&hashCtx, incrementCount);
	result |= Trspi_HashUpdate(&hashCtx, publicInfoSize, publicInfo);
	result |= Trspi_Hash_DIGEST(&hashCtx, xsap->encAuthUse.authdata);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto done;

	/* Create the delegation */
	if ((result = TCS_API(hContext)->Delegate_CreateOwnerDelegation(hContext, incrementCount,
									publicInfoSize, publicInfo,
									&xsap->encAuthUse,
									xsap->pAuth, &blobSize,
									&blob)))
		goto done;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_CreateOwnerDelegation);
	result |= Trspi_Hash_UINT32(&hashCtx, blobSize);
	result |= Trspi_HashUpdate(&hashCtx, blobSize, blob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (authsess_xsap_verify(xsap, &digest)) {
		result = TSPERR(TSS_E_TSP_AUTHFAIL);
		goto done;
	}

	result = obj_policy_set_delegation_blob(hDelegation, TSS_DELEGATIONTYPE_OWNER,
			blobSize, blob);

done:
	authsess_free(xsap);
	free(publicInfo);
	free(blob);

	return result;
}

TSS_RESULT
create_key_delegation(TSS_HKEY       hKey,
		      BYTE           bLabel,
		      UINT32         ulFlags,
		      TSS_HPCRS      hPcrs,
		      TSS_HDELFAMILY hFamily,
		      TSS_HPOLICY    hDelegation)
{
	TSS_HCONTEXT hContext;
	UINT32 type;
	TCS_KEY_HANDLE tcsKeyHandle;
	UINT32 publicInfoSize;
	BYTE *publicInfo = NULL;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	UINT32 blobSize;
	BYTE *blob = NULL;
	TSS_RESULT result;
	struct authsess *xsap = NULL;

	if ((result = obj_rsakey_get_tsp_context(hKey, &hContext)))
		return result;

	if (ulFlags != 0)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_policy_get_delegation_type(hDelegation, &type)))
		return result;

	if (type != TSS_DELEGATIONTYPE_KEY)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKeyHandle)))
		return result;

	if ((result = __tspi_build_delegate_public_info(bLabel, hPcrs, hFamily, hDelegation,
			&publicInfoSize, &publicInfo)))
		return result;

	if ((result = authsess_xsap_init(hContext, hKey, hDelegation, TSS_AUTH_POLICY_REQUIRED,
					 TPM_ORD_Delegate_CreateKeyDelegation, TPM_ET_KEYHANDLE,
					 &xsap)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_CreateKeyDelegation);
	result |= Trspi_HashUpdate(&hashCtx, publicInfoSize, publicInfo);
	result |= Trspi_Hash_ENCAUTH(&hashCtx, xsap->encAuthUse.authdata);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if ((result = authsess_xsap_hmac(xsap, &digest)))
		goto done;

	/* Create the delegation */
	if ((result = TCS_API(hContext)->Delegate_CreateKeyDelegation(hContext, tcsKeyHandle,
								      publicInfoSize, publicInfo,
								      &xsap->encAuthUse,
								      xsap->pAuth, &blobSize,
								      &blob)))
		goto done;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_CreateKeyDelegation);
	result |= Trspi_Hash_UINT32(&hashCtx, blobSize);
	result |= Trspi_HashUpdate(&hashCtx, blobSize, blob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (authsess_xsap_verify(xsap, &digest)) {
		result = TSPERR(TSS_E_TSP_AUTHFAIL);
		goto done;
	}

	result = obj_policy_set_delegation_blob(hDelegation, TSS_DELEGATIONTYPE_KEY, blobSize,
						blob);

done:
	free(blob);
	authsess_free(xsap);
	free(publicInfo);

	return result;
}

TSS_RESULT
update_delfamily_object(TSS_HTPM hTpm, UINT32 familyID)
{
	TSS_HCONTEXT hContext;
	UINT32 familyTableSize, delegateTableSize;
	BYTE *familyTable = NULL, *delegateTable = NULL;
	UINT64 offset;
	TPM_FAMILY_TABLE_ENTRY familyTableEntry;
	TSS_BOOL familyState;
	TSS_HDELFAMILY hFamily;
	TSS_RESULT result;

	if ((result = obj_tpm_get_tsp_context(hTpm, &hContext)))
		return result;

	if ((result = TCS_API(hContext)->Delegate_ReadTable(hContext, &familyTableSize,
							    &familyTable, &delegateTableSize,
							    &delegateTable)))
		return result;

	for (offset = 0; offset < familyTableSize;) {
		Trspi_UnloadBlob_TPM_FAMILY_TABLE_ENTRY(&offset, familyTable, &familyTableEntry);
		if (familyTableEntry.familyID == familyID) {
			obj_delfamily_find_by_familyid(hContext, familyID, &hFamily);
			if (hFamily == NULL_HDELFAMILY) {
				if ((result = obj_delfamily_add(hContext, &hFamily)))
					goto done;
				if ((result = obj_delfamily_set_familyid(hFamily,
									 familyTableEntry.familyID)))
					goto done;
				if ((result = obj_delfamily_set_label(hFamily,
								      familyTableEntry.label.label)))
					goto done;
			}

			/* Set/Update the family attributes */
			familyState = (familyTableEntry.flags & TPM_FAMFLAG_DELEGATE_ADMIN_LOCK) ?
				      TRUE : FALSE;
			if ((result = obj_delfamily_set_locked(hFamily, familyState, FALSE)))
				goto done;
			familyState = (familyTableEntry.flags & TPM_FAMFLAG_ENABLE) ? TRUE : FALSE;
			if ((result = obj_delfamily_set_enabled(hFamily, familyState, FALSE)))
				goto done;
			if ((result = obj_delfamily_set_vercount(hFamily,
								 familyTableEntry.verificationCount)))
				goto done;

			break;
		}
	}

done:
	free(familyTable);
	free(delegateTable);

	return result;
}

TSS_RESULT
get_delegate_index(TSS_HCONTEXT hContext, UINT32 index, TPM_DELEGATE_PUBLIC *public)
{
	UINT32 familyTableSize, delegateTableSize;
	BYTE *familyTable = NULL, *delegateTable = NULL;
	UINT64 offset;
	UINT32 tpmIndex;
	TPM_DELEGATE_PUBLIC tempPublic;
	TSS_RESULT result;

	if ((result = TCS_API(hContext)->Delegate_ReadTable(hContext, &familyTableSize,
							    &familyTable, &delegateTableSize,
							    &delegateTable)))
		goto done;

	for (offset = 0; offset < delegateTableSize;) {
		Trspi_UnloadBlob_UINT32(&offset, &tpmIndex, delegateTable);
		if (tpmIndex == index) {
			result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(&offset, delegateTable, public);
			goto done;
		} else {
			if ((result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(&offset, delegateTable, &tempPublic)))
				goto done;
		}

		free(tempPublic.pcrInfo.pcrSelection.pcrSelect);
	}

	/* Didn't find a matching index */
	result = TSPERR(TSS_E_BAD_PARAMETER);

done:
	free(familyTable);
	free(delegateTable);

	return result;
}

TSS_RESULT
__tspi_build_delegate_public_info(BYTE           bLabel,
			   TSS_HPCRS      hPcrs,
			   TSS_HDELFAMILY hFamily,
			   TSS_HPOLICY    hDelegation,
			   UINT32        *publicInfoSize,
			   BYTE         **publicInfo)
{
	TPM_DELEGATE_PUBLIC public;
	UINT32 delegateType;
	UINT32 pcrInfoSize;
	BYTE *pcrInfo = NULL;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if (hDelegation == NULL_HPOLICY)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_policy_get_delegation_type(hDelegation, &delegateType)))
		return result;

	/* This call will create a "null" PCR_INFO_SHORT if hPcrs is null */
	if ((result = obj_pcrs_create_info_short(hPcrs, &pcrInfoSize, &pcrInfo)))
		return result;

	memset(&public, 0, sizeof(public));
	public.tag = TPM_TAG_DELEGATE_PUBLIC;
	public.label.label = bLabel;
	offset = 0;
	if ((result = Trspi_UnloadBlob_PCR_INFO_SHORT(&offset, pcrInfo, &public.pcrInfo)))
		goto done;
	public.permissions.tag = TPM_TAG_DELEGATIONS;
	public.permissions.delegateType =
		(delegateType == TSS_DELEGATIONTYPE_OWNER) ? TPM_DEL_OWNER_BITS : TPM_DEL_KEY_BITS;
	if ((result = obj_policy_get_delegation_per1(hDelegation, &public.permissions.per1)))
		goto done;
	if ((result = obj_policy_get_delegation_per2(hDelegation, &public.permissions.per2)))
		goto done;
	if ((result = obj_delfamily_get_familyid(hFamily, &public.familyID)))
		goto done;
	if ((result = obj_delfamily_get_vercount(hFamily, &public.verificationCount)))
		goto done;

	offset = 0;
	Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(&offset, NULL, &public);
	*publicInfoSize = offset;
	*publicInfo = malloc(*publicInfoSize);
	if (*publicInfo == NULL) {
		LogError("malloc of %u bytes failed.", *publicInfoSize);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	offset = 0;
	Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(&offset, *publicInfo, &public);

done:
	free(pcrInfo);
	free(public.pcrInfo.pcrSelection.pcrSelect);

	return result;
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_Delegate_Manage(TSS_HCONTEXT tspContext,              /* in */
			  TPM_FAMILY_ID familyID,             /* in */
			  TPM_FAMILY_OPERATION opFlag,        /* in */
			  UINT32 opDataSize,                  /* in */
			  BYTE *opData,                       /* in */
			  TPM_AUTH *ownerAuth,                /* in, out */
			  UINT32 *retDataSize,                /* out */
			  BYTE **retData)                     /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen, dataLen;
	UINT64 offset;
	BYTE *data, *dec = NULL;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TPM_FAMILY_ID)
		  + sizeof(TPM_FAMILY_OPERATION)
		  + sizeof(UINT32)
		  + opDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, familyID, data);
	Trspi_LoadBlob_UINT32(&offset, opFlag, data);
	Trspi_LoadBlob_UINT32(&offset, opDataSize, data);
	Trspi_LoadBlob(&offset, opDataSize, data, opData);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Delegate_Manage, dataLen,
						    data, NULL, &handlesLen, NULL, ownerAuth,
						    NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, retDataSize, dec);

	if ((*retData = malloc(*retDataSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *retDataSize);
		*retDataSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *retDataSize, dec, *retData);

	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_CreateKeyDelegation(TSS_HCONTEXT tspContext,         /* in */
				       TCS_KEY_HANDLE hKey,           /* in */
				       UINT32 publicInfoSize,         /* in */
				       BYTE *publicInfo,              /* in */
				       TPM_ENCAUTH *encDelAuth,        /* in */
				       TPM_AUTH *keyAuth,             /* in, out */
				       UINT32 *blobSize,              /* out */
				       BYTE **blob)                   /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen, dataLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data, *dec = NULL;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(hKey, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = hKey;
	handles = &handle;

	dataLen = publicInfoSize + sizeof(TPM_ENCAUTH);
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, publicInfoSize, data, publicInfo);
	Trspi_LoadBlob(&offset, sizeof(TPM_ENCAUTH), data, encDelAuth->authdata);

	if ((result = obj_context_transport_execute(tspContext,
						    TPM_ORD_Delegate_CreateKeyDelegation, dataLen,
						    data, &pubKeyHash, &handlesLen, &handles,
						    keyAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, blobSize, dec);

	if ((*blob = malloc(*blobSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *blobSize);
		*blobSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *blobSize, dec, *blob);

	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_CreateOwnerDelegation(TSS_HCONTEXT tspContext,       /* in */
					 TSS_BOOL increment,          /* in */
					 UINT32 publicInfoSize,       /* in */
					 BYTE *publicInfo,            /* in */
					 TPM_ENCAUTH *encDelAuth,      /* in */
					 TPM_AUTH *ownerAuth,         /* in, out */
					 UINT32 *blobSize,            /* out */
					 BYTE **blob)                 /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen, dataLen;
	UINT64 offset;
	BYTE *data, *dec = NULL;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TSS_BOOL) + publicInfoSize + sizeof(TPM_ENCAUTH);
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_BOOL(&offset, increment, data);
	Trspi_LoadBlob(&offset, publicInfoSize, data, publicInfo);
	Trspi_LoadBlob(&offset, sizeof(TPM_ENCAUTH), data, encDelAuth->authdata);

	if ((result = obj_context_transport_execute(tspContext,
						    TPM_ORD_Delegate_CreateOwnerDelegation, dataLen,
						    data, NULL, &handlesLen, NULL, ownerAuth,
						    NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, blobSize, dec);

	if ((*blob = malloc(*blobSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *blobSize);
		*blobSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *blobSize, dec, *blob);

	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_LoadOwnerDelegation(TSS_HCONTEXT tspContext, /* in */
				       TPM_DELEGATE_INDEX index,      /* in */
				       UINT32 blobSize,               /* in */
				       BYTE *blob,                    /* in */
				       TPM_AUTH *ownerAuth)           /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, dataLen, decLen;
	UINT64 offset;
	BYTE *data, *dec = NULL;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TPM_DELEGATE_INDEX) + sizeof(UINT32) + blobSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, index, data);
	Trspi_LoadBlob_UINT32(&offset, blobSize, data);
	Trspi_LoadBlob(&offset, blobSize, data, blob);

	if ((result = obj_context_transport_execute(tspContext,
						    TPM_ORD_Delegate_LoadOwnerDelegation, dataLen,
						    data, NULL, &handlesLen, NULL, ownerAuth,
						    NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);
	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_ReadTable(TSS_HCONTEXT tspContext,           /* in */
			     UINT32 *familyTableSize,         /* out */
			     BYTE **familyTable,              /* out */
			     UINT32 *delegateTableSize,       /* out */
			     BYTE **delegateTable)            /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	UINT64 offset;
	BYTE *dec = NULL;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Delegate_ReadTable, 0, NULL,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, familyTableSize, dec);

	if ((*familyTable = malloc(*familyTableSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *familyTableSize);
		*familyTableSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *familyTableSize, dec, *familyTable);

	Trspi_UnloadBlob_UINT32(&offset, delegateTableSize, dec);

	if ((*delegateTable = malloc(*delegateTableSize)) == NULL) {
		free(dec);
		free(*familyTable);
		*familyTable = NULL;
		*familyTableSize = 0;
		LogError("malloc of %u bytes failed", *delegateTableSize);
		*delegateTableSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *delegateTableSize, dec, *delegateTable);

	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_UpdateVerificationCount(TSS_HCONTEXT tspContext,     /* in */
					   UINT32 inputSize,          /* in */
					   BYTE *input,               /* in */
					   TPM_AUTH *ownerAuth,       /* in, out */
					   UINT32 *outputSize,        /* out */
					   BYTE **output)             /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen, dataLen;
	UINT64 offset;
	BYTE *data, *dec = NULL;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(UINT32) + inputSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, inputSize, data);
	Trspi_LoadBlob(&offset, inputSize, data, input);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Delegate_UpdateVerification,
						    dataLen, data, NULL, &handlesLen, NULL,
						    ownerAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, outputSize, dec);

	if ((*output = malloc(*outputSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *outputSize);
		*outputSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *outputSize, dec, *output);

	free(dec);

	return result;
}

TSS_RESULT
Transport_Delegate_VerifyDelegation(TSS_HCONTEXT tspContext,    /* in */
				    UINT32 delegateSize,      /* in */
				    BYTE *delegate)           /* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, dataLen, decLen;
	UINT64 offset;
	BYTE *data, *dec = NULL;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = + sizeof(UINT32) + delegateSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, delegateSize, data);
	Trspi_LoadBlob(&offset, delegateSize, data, delegate);

	result = obj_context_transport_execute(tspContext, TPM_ORD_Delegate_VerifyDelegation,
					       dataLen, data, NULL, &handlesLen, NULL, NULL, NULL,
					       &decLen, &dec);
	free(data);
	free(dec);

	return result;
}

TSS_RESULT
Transport_DSAP(TSS_HCONTEXT tspContext,		/* in */
	       TPM_ENTITY_TYPE entityType,	/* in */
	       TCS_KEY_HANDLE keyHandle,	/* in */
	       TPM_NONCE *nonceOddDSAP,		/* in */
	       UINT32 entityValueSize,		/* in */
	       BYTE * entityValue,		/* in */
	       TCS_AUTHHANDLE *authHandle,	/* out */
	       TPM_NONCE *nonceEven,		/* out */
	       TPM_NONCE *nonceEvenDSAP)	/* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, dataLen, decLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data, *dec = NULL;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	dataLen = sizeof(TPM_ENTITY_TYPE) + sizeof(TPM_KEY_HANDLE)
					  + sizeof(TPM_NONCE)
					  + sizeof(UINT32)
					  + entityValueSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, entityType, data);
	Trspi_LoadBlob_UINT32(&offset, keyHandle, data);
	Trspi_LoadBlob(&offset, sizeof(TPM_NONCE), data, nonceEvenDSAP->nonce);
	Trspi_LoadBlob_UINT32(&offset, entityValueSize, data);
	Trspi_LoadBlob(&offset, entityValueSize, data, entityValue);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_DSAP, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles, NULL, NULL,
						    &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, authHandle, dec);

	Trspi_UnloadBlob(&offset, sizeof(TPM_NONCE), dec, nonceEven->nonce);
	Trspi_UnloadBlob(&offset, sizeof(TPM_NONCE), dec, nonceEvenDSAP->nonce);

	free(dec);

	return result;
}
#endif
