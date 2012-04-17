
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "obj.h"
#include "tsp_delegate.h"
#include "tsplog.h"


TSS_RESULT
Tspi_TPM_Delegate_AddFamily(TSS_HTPM        hTpm,	/* in, must not be NULL */
			    BYTE            bLabel,	/* in */
			    TSS_HDELFAMILY* phFamily)	/* out */
{
	TPM_FAMILY_ID familyID = 0;
	UINT32 outDataSize;
	BYTE *outData = NULL;
	UINT64 offset;
	TSS_RESULT result;

	if (phFamily == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);
	*phFamily = NULL_HDELFAMILY;

	if ((result = do_delegate_manage(hTpm, familyID, TPM_FAMILY_CREATE, sizeof(bLabel), &bLabel,
					 &outDataSize, &outData)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, &familyID, outData);

	/* Create or update the delfamily object */
	if ((result = update_delfamily_object(hTpm, familyID)))
		goto done;

	obj_delfamily_find_by_familyid(hTpm, familyID, phFamily);
	if (*phFamily == NULL_HDELFAMILY)
		result = TSPERR(TSS_E_INTERNAL_ERROR);

done:
	free(outData);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_GetFamily(TSS_HTPM        hTpm,	/* in, must not NULL */
			    UINT32          ulFamilyID,	/* in */
			    TSS_HDELFAMILY* phFamily)	/* out */
{
	TSS_RESULT result;

	if (phFamily == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);
	*phFamily = NULL_HDELFAMILY;

	/* Update the delfamily object */
	if ((result = update_delfamily_object(hTpm, ulFamilyID)))
		return result;

	obj_delfamily_find_by_familyid(hTpm, ulFamilyID, phFamily);
	if (*phFamily == NULL_HDELFAMILY)
		result = TSPERR(TSS_E_BAD_PARAMETER);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_InvalidateFamily(TSS_HTPM       hTpm,		/* in, must not be NULL */
				   TSS_HDELFAMILY hFamily)	/* in */
{
	TPM_FAMILY_ID familyID;
	UINT32 outDataSize;
	BYTE *outData = NULL;
	TSS_RESULT result;

	if ((result = obj_delfamily_get_familyid(hFamily, &familyID)))
		return result;

	if ((result = do_delegate_manage(hTpm, familyID, TPM_FAMILY_INVALIDATE,	0, NULL,
					 &outDataSize, &outData)))
		return result;

	/* Delete the delfamily object */
	result = obj_delfamily_remove(hFamily, hTpm);

	free(outData);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_CreateDelegation(TSS_HOBJECT    hObject,	/* in */
				   BYTE           bLabel,	/* in */
				   UINT32         ulFlags,	/* in */
				   TSS_HPCRS      hPcrs,	/* in */
				   TSS_HDELFAMILY hFamily,	/* in */
				   TSS_HPOLICY    hDelegation)	/* in, out */
{
	TSS_RESULT result;

	if (obj_is_tpm(hObject)) {
		if ((result = create_owner_delegation(hObject, bLabel, ulFlags, hPcrs, hFamily,
						      hDelegation)))
			return result;
	} else if (obj_is_rsakey(hObject)) {
		if ((result = create_key_delegation(hObject, bLabel, ulFlags, hPcrs, hFamily,
						    hDelegation)))
			return result;
	} else
		return TSPERR(TSS_E_INVALID_HANDLE);

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_Delegate_CacheOwnerDelegation(TSS_HTPM    hTpm,	/* in */
				       TSS_HPOLICY hDelegation,	/* in */
				       UINT32      ulIndex,	/* in */
				       UINT32      ulFlags)	/* in */
{
	TSS_HCONTEXT hContext;
	TSS_HPOLICY hPolicy;
	UINT32 blobSize;
	BYTE *blob = NULL;
	UINT32 secretMode = TSS_SECRET_MODE_NONE;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	TPM_AUTH ownerAuth, *pAuth;
	TSS_RESULT result;

	if ((result = obj_tpm_get_tsp_context(hTpm, &hContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTpm, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	if ((result = obj_policy_get_delegation_blob(hDelegation, TSS_DELEGATIONTYPE_OWNER,
			&blobSize, &blob)))
		return result;

	if (ulFlags & ~TSS_DELEGATE_CACHEOWNERDELEGATION_OVERWRITEEXISTING) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	if ((ulFlags & TSS_DELEGATE_CACHEOWNERDELEGATION_OVERWRITEEXISTING) == 0) {
		TPM_DELEGATE_PUBLIC public;

		/* Verify there is nothing occupying the specified row */
		result = get_delegate_index(hContext, ulIndex, &public);
		if (result == TSS_SUCCESS) {
			free(public.pcrInfo.pcrSelection.pcrSelect);
			result = TSPERR(TSS_E_DELFAMILY_ROWEXISTS);
			goto done;
		}
	}

	if (hPolicy != NULL_HPOLICY) {
		if ((result = obj_policy_get_mode(hPolicy, &secretMode)))
			goto done;
	}

	if (secretMode != TSS_SECRET_MODE_NONE) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_LoadOwnerDelegation);
		result |= Trspi_Hash_UINT32(&hashCtx, ulIndex);
		result |= Trspi_Hash_UINT32(&hashCtx, blobSize);
		result |= Trspi_HashUpdate(&hashCtx, blobSize, blob);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		pAuth = &ownerAuth;
		if ((result = secret_PerformAuth_OIAP(hTpm, TPM_ORD_Delegate_LoadOwnerDelegation,
				hPolicy, FALSE, &digest, pAuth)))
			goto done;
	} else
		pAuth = NULL;

	if ((result = TCS_API(hContext)->Delegate_LoadOwnerDelegation(hContext, ulIndex, blobSize,
								      blob, pAuth)))
		goto done;

	if (pAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_LoadOwnerDelegation);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth)))
			goto done;
	}

	result = obj_policy_set_delegation_index(hDelegation, ulIndex);

done:
	free_tspi(hContext, blob);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_UpdateVerificationCount(TSS_HTPM    hTpm,		/* in */
					  TSS_HPOLICY hDelegation)	/* in, out */
{
	TSS_HCONTEXT hContext;
	TSS_HPOLICY hPolicy;
	UINT32 secretMode = TSS_SECRET_MODE_NONE;
	Trspi_HashCtx hashCtx;
	TCPA_DIGEST digest;
	TPM_AUTH ownerAuth, *pAuth;
	TSS_BOOL indexSet;
	UINT32 inputSize;
	BYTE *input = NULL;
	UINT32 outputSize;
	BYTE *output = NULL;
	UINT64 offset;
	TSS_RESULT result;

	if ((result = obj_tpm_get_tsp_context(hTpm, &hContext)))
		return result;

	if ((result = obj_tpm_get_policy(hTpm, TSS_POLICY_USAGE, &hPolicy)))
		return result;

	if (hPolicy != NULL_HPOLICY) {
		if ((result = obj_policy_get_mode(hPolicy, &secretMode)))
			goto done;
	}

	if ((result = obj_policy_is_delegation_index_set(hDelegation, &indexSet)))
		return result;
	if (indexSet) {
		UINT32 index;

		if ((result = obj_policy_get_delegation_index(hDelegation, &index)))
			return result;
		inputSize = sizeof(UINT32);
		input = calloc_tspi(hContext, inputSize);
		if (!input) {
			LogError("malloc of %zd bytes failed.", sizeof(UINT32));
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		offset = 0;
		Trspi_LoadBlob_UINT32(&offset, index, input);
	} else {
		if ((result = obj_policy_get_delegation_blob(hDelegation, 0,
				&inputSize, &input)))
			return result;
	}

	if (secretMode != TSS_SECRET_MODE_NONE) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_UpdateVerification);
		result |= Trspi_Hash_UINT32(&hashCtx, inputSize);
		result |= Trspi_HashUpdate(&hashCtx, inputSize, input);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		pAuth = &ownerAuth;
		if ((result = secret_PerformAuth_OIAP(hTpm, TPM_ORD_Delegate_UpdateVerification,
				hPolicy, FALSE, &digest, pAuth)))
			goto done;
	} else
		pAuth = NULL;

	if ((result = TCS_API(hContext)->Delegate_UpdateVerificationCount(hContext, inputSize,
									  input, pAuth, &outputSize,
									  &output)))
		goto done;

	if (pAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_Delegate_UpdateVerification);
		result |= Trspi_Hash_UINT32(&hashCtx, outputSize);
		result |= Trspi_HashUpdate(&hashCtx, outputSize, output);
		if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
			goto done;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth)))
			goto done;
	}

	result = obj_policy_set_delegation_blob(hDelegation, 0, outputSize, output);

done:
	free_tspi(hContext, input);
	free(output);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_VerifyDelegation(TSS_HPOLICY hDelegation)	/* in, out */
{
	TSS_HCONTEXT hContext;
	UINT32 delegateSize;
	BYTE *delegate = NULL;
	TSS_RESULT result;

	if ((result = obj_policy_get_tsp_context(hDelegation, &hContext)))
		return result;

	if ((result = obj_policy_get_delegation_blob(hDelegation, 0, &delegateSize, &delegate)))
		return result;

	result = TCS_API(hContext)->Delegate_VerifyDelegation(hContext, delegateSize, delegate);

	free_tspi(hContext, delegate);

	return result;
}

TSS_RESULT
Tspi_TPM_Delegate_ReadTables(TSS_HCONTEXT                 hContext,		/* in */
			     UINT32*                      pulFamilyTableSize,	/* out */
			     TSS_FAMILY_TABLE_ENTRY**     ppFamilyTable,	/* out */
			     UINT32*                      pulDelegateTableSize,	/* out */
			     TSS_DELEGATION_TABLE_ENTRY** ppDelegateTable)	/* out */
{
	UINT32 tpmFamilyTableSize, tpmDelegateTableSize;
	BYTE *tpmFamilyTable = NULL, *tpmDelegateTable = NULL;
	TPM_FAMILY_TABLE_ENTRY tpmFamilyEntry;
	TSS_FAMILY_TABLE_ENTRY tssFamilyEntry, *tssFamilyTable = NULL;
	UINT32 tssFamilyTableSize = 0;
	TPM_DELEGATE_PUBLIC tpmDelegatePublic;
	TSS_DELEGATION_TABLE_ENTRY tssDelegateEntry, *tssDelegateTable = NULL;
	UINT32 tssDelegateTableSize = 0;
	UINT32 tableIndex;
	UINT64 tpmOffset;
	UINT64 tssOffset;
	TSS_RESULT result;

	if (!pulFamilyTableSize || !ppFamilyTable || !pulDelegateTableSize || !ppDelegateTable)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(hContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = TCS_API(hContext)->Delegate_ReadTable(hContext, &tpmFamilyTableSize,
							    &tpmFamilyTable, &tpmDelegateTableSize,
							    &tpmDelegateTable)))
		return result;

	if (tpmFamilyTableSize > 0) {
		/* Create the TSS_FAMILY_TABLE_ENTRY array */
		for (tpmOffset = 0, tssOffset = 0; tpmOffset < tpmFamilyTableSize;) {
			Trspi_UnloadBlob_TPM_FAMILY_TABLE_ENTRY(&tpmOffset, tpmFamilyTable,
				&tpmFamilyEntry);

			/* No pointers in the family table entries, so no
			   assignments required before doing LoadBlob */
			Trspi_LoadBlob_TSS_FAMILY_TABLE_ENTRY(&tssOffset, NULL, &tssFamilyEntry);
		}

		if ((tssFamilyTable = calloc_tspi(hContext, tssOffset)) == NULL) {
			LogError("malloc of %" PRIu64 " bytes failed.", tssOffset);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		for (tpmOffset = 0, tssOffset = 0; tpmOffset < tpmFamilyTableSize; tssFamilyTableSize++) {
			Trspi_UnloadBlob_TPM_FAMILY_TABLE_ENTRY(&tpmOffset, tpmFamilyTable,
				&tpmFamilyEntry);

			tssFamilyEntry.familyID = tpmFamilyEntry.familyID;
			tssFamilyEntry.label = tpmFamilyEntry.label.label;
			tssFamilyEntry.verificationCount = tpmFamilyEntry.verificationCount;
			tssFamilyEntry.enabled =
				(tpmFamilyEntry.flags & TPM_FAMFLAG_ENABLE) ? TRUE : FALSE;
			tssFamilyEntry.locked =
				(tpmFamilyEntry.flags & TPM_FAMFLAG_DELEGATE_ADMIN_LOCK) ? TRUE : FALSE;
			Trspi_LoadBlob_TSS_FAMILY_TABLE_ENTRY(&tssOffset, (BYTE *)tssFamilyTable,
				&tssFamilyEntry);
		}
	}

	if (tpmDelegateTableSize > 0) {
		/* Create the TSS_DELEGATION_TABLE_ENTRY array */
		for (tpmOffset = 0, tssOffset = 0; tpmOffset < tpmDelegateTableSize;) {
			Trspi_UnloadBlob_UINT32(&tpmOffset, &tableIndex, tpmDelegateTable);
			if ((result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(&tpmOffset, tpmDelegateTable,
					&tpmDelegatePublic))) {
				free_tspi(hContext, tssFamilyTable);
				goto done;
			}

			/* Some pointers in the delegate table entries, so
			   do some assignments before doing LoadBlob */
			tssDelegateEntry.pcrInfo.sizeOfSelect =
				tpmDelegatePublic.pcrInfo.pcrSelection.sizeOfSelect;
			tssDelegateEntry.pcrInfo.selection =
				tpmDelegatePublic.pcrInfo.pcrSelection.pcrSelect;
			tssDelegateEntry.pcrInfo.sizeOfDigestAtRelease =
				sizeof(tpmDelegatePublic.pcrInfo.digestAtRelease.digest);
			tssDelegateEntry.pcrInfo.digestAtRelease =
				tpmDelegatePublic.pcrInfo.digestAtRelease.digest;
			Trspi_LoadBlob_TSS_DELEGATION_TABLE_ENTRY(&tssOffset, NULL,
				&tssDelegateEntry);

			free(tpmDelegatePublic.pcrInfo.pcrSelection.pcrSelect);
		}

		if ((tssDelegateTable = calloc_tspi(hContext, tssOffset)) == NULL) {
			LogError("malloc of %" PRIu64 " bytes failed.", tssOffset);
			free_tspi(hContext, tssFamilyTable);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		for (tpmOffset = 0, tssOffset = 0; tpmOffset < tpmDelegateTableSize; tssDelegateTableSize++) {
			Trspi_UnloadBlob_UINT32(&tpmOffset, &tableIndex, tpmDelegateTable);
			if ((result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(&tpmOffset,
					tpmDelegateTable, &tpmDelegatePublic))) {
				free_tspi(hContext, tssFamilyTable);
				free_tspi(hContext, tssDelegateTable);
				goto done;
			}

			tssDelegateEntry.tableIndex = tableIndex;
			tssDelegateEntry.label = tpmDelegatePublic.label.label;
			tssDelegateEntry.pcrInfo.sizeOfSelect =
				tpmDelegatePublic.pcrInfo.pcrSelection.sizeOfSelect;
			tssDelegateEntry.pcrInfo.selection =
				tpmDelegatePublic.pcrInfo.pcrSelection.pcrSelect;
			tssDelegateEntry.pcrInfo.localityAtRelease =
				tpmDelegatePublic.pcrInfo.localityAtRelease;
			tssDelegateEntry.pcrInfo.sizeOfDigestAtRelease =
				sizeof(tpmDelegatePublic.pcrInfo.digestAtRelease.digest);
			tssDelegateEntry.pcrInfo.digestAtRelease =
				tpmDelegatePublic.pcrInfo.digestAtRelease.digest;
			tssDelegateEntry.per1 = tpmDelegatePublic.permissions.per1;
			tssDelegateEntry.per2 = tpmDelegatePublic.permissions.per2;
			tssDelegateEntry.familyID = tpmDelegatePublic.familyID;
			tssDelegateEntry.verificationCount = tpmDelegatePublic.verificationCount;
			Trspi_LoadBlob_TSS_DELEGATION_TABLE_ENTRY(&tssOffset,
				(BYTE *)tssDelegateTable, &tssDelegateEntry);

			free(tpmDelegatePublic.pcrInfo.pcrSelection.pcrSelect);
		}
	}

	*ppFamilyTable = tssFamilyTable;
	*pulFamilyTableSize = tssFamilyTableSize;
	*ppDelegateTable = tssDelegateTable;
	*pulDelegateTableSize = tssDelegateTableSize;

done:
	free(tpmFamilyTable);
	free(tpmDelegateTable);

	return result;
}

