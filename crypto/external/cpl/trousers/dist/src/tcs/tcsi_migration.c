
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
TCSP_CreateMigrationBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				  TCS_KEY_HANDLE parentHandle,	/* in */
				  TSS_MIGRATE_SCHEME migrationType,	/* in */
				  UINT32 MigrationKeyAuthSize,	/* in */
				  BYTE * MigrationKeyAuth,	/* in */
				  UINT32 encDataSize,	/* in */
				  BYTE * encData,	/* in */
				  TPM_AUTH * parentAuth,	/* in, out */
				  TPM_AUTH * entityAuth,	/* in, out */
				  UINT32 * randomSize,	/* out */
				  BYTE ** random,	/* out */
				  UINT32 * outDataSize,	/* out */
				  BYTE ** outData)	/* out */
{
	UINT64 offset = 0;
	UINT32 paramSize;
	TSS_RESULT result;
	TCPA_KEY_HANDLE keyHandle;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("Entering TPM_CreateMigrationBlob");

	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (parentAuth != NULL) {
		if ((result = auth_mgr_check(hContext, &parentAuth->AuthHandle)))
			goto done;
	}

	if ((result = auth_mgr_check(hContext, &entityAuth->AuthHandle)))
		goto done;

	if ((result = ensureKeyIsLoaded(hContext, parentHandle, &keyHandle)))
		goto done;

	switch (migrationType) {
		case TSS_MS_MIGRATE:
			migrationType = TCPA_MS_MIGRATE;
			break;
		case TSS_MS_REWRAP:
			migrationType = TCPA_MS_REWRAP;
			break;
		case TSS_MS_MAINT:
			migrationType = TCPA_MS_MAINT;
			break;
		default:
			/* Let the TPM return an error */
			break;
	}

	if ((result = tpm_rqu_build(TPM_ORD_CreateMigrationBlob, &offset, txBlob, keyHandle,
				    migrationType, MigrationKeyAuthSize, MigrationKeyAuth,
				    encDataSize, encData, parentAuth, entityAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (result == TSS_SUCCESS) {
		result = tpm_rsp_parse(TPM_ORD_CreateMigrationBlob, txBlob, paramSize, randomSize,
				       random, outDataSize, outData, parentAuth, entityAuth);
	}
	LogResult("TPM_CreateMigrationBlob", result);

done:
	auth_mgr_release_auth(entityAuth, parentAuth, hContext);
	return result;
}

TSS_RESULT
TCSP_ConvertMigrationBlob_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				   TCS_KEY_HANDLE parentHandle,	/* in */
				   UINT32 inDataSize,	/* in */
				   BYTE * inData,	/* in */
				   UINT32 randomSize,	/* in */
				   BYTE * random,	/* in */
				   TPM_AUTH * parentAuth,	/* in, out */
				   UINT32 * outDataSize,	/* out */
				   BYTE ** outData)	/* out */
{
	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	TCPA_KEY_HANDLE keySlot;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("ConvertMigBlob");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if (parentAuth != NULL) {
		LogDebug("Auth Used");
		if ((result = auth_mgr_check(hContext, &parentAuth->AuthHandle)))
			goto done;
	} else {
		LogDebug("No Auth");
	}
	if ((result = ensureKeyIsLoaded(hContext, parentHandle, &keySlot)))
		goto done;

	if ((result = tpm_rqu_build(TPM_ORD_ConvertMigrationBlob, &offset, txBlob, keySlot,
				    inDataSize, inData, randomSize, random, parentAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	offset = 10;
	result = UnloadBlob_Header(txBlob, &paramSize);

	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_ConvertMigrationBlob, txBlob, paramSize, outDataSize,
				       outData, parentAuth, NULL);
	}
	LogResult("***Leaving ConvertMigrationBlob with result ", result);
done:
	auth_mgr_release_auth(parentAuth, NULL, hContext);
	return result;
}

TSS_RESULT
TCSP_AuthorizeMigrationKey_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				    TSS_MIGRATE_SCHEME migrateScheme,	/* in */
				    UINT32 MigrationKeySize,	/* in */
				    BYTE * MigrationKey,	/* in */
				    TPM_AUTH * ownerAuth,	/* in, out */
				    UINT32 * MigrationKeyAuthSize,	/* out */
				    BYTE ** MigrationKeyAuth)	/* out */
{

	TSS_RESULT result;
	UINT32 paramSize;
	UINT64 offset = 0;
	//TCPA_MIGRATIONKEYAUTH container;
	BYTE txBlob[TSS_TPM_TXBLOB_SIZE];

	LogDebug("TCSP_AuthorizeMigrationKey");
	if ((result = ctx_verify_context(hContext)))
		goto done;

	if ((result = auth_mgr_check(hContext, &ownerAuth->AuthHandle)))
		goto done;

	switch (migrateScheme) {
		case TSS_MS_MIGRATE:
			migrateScheme = TCPA_MS_MIGRATE;
			break;
		case TSS_MS_REWRAP:
			migrateScheme = TCPA_MS_REWRAP;
			break;
		case TSS_MS_MAINT:
			migrateScheme = TCPA_MS_MAINT;
			break;
#ifdef TSS_BUILD_CMK
		case TSS_MS_RESTRICT_MIGRATE:
			migrateScheme = TPM_MS_RESTRICT_MIGRATE;
			break;

		case TSS_MS_RESTRICT_APPROVE_DOUBLE:
			migrateScheme = TPM_MS_RESTRICT_APPROVE_DOUBLE;
			break;
#endif
		default:
			/* Let the TPM return an error */
			break;
	}

	if ((result = tpm_rqu_build(TPM_ORD_AuthorizeMigrationKey, &offset, txBlob, migrateScheme,
				    MigrationKeySize, MigrationKey, ownerAuth)))
		return result;

	if ((result = req_mgr_submit_req(txBlob)))
		goto done;

	result = UnloadBlob_Header(txBlob, &paramSize);
	if (!result) {
		result = tpm_rsp_parse(TPM_ORD_AuthorizeMigrationKey, txBlob, paramSize,
				       MigrationKeyAuthSize, MigrationKeyAuth, ownerAuth);
	}
	LogDebugFn("TPM_AuthorizeMigrationKey result: 0x%x", result);
done:
	auth_mgr_release_auth(ownerAuth, NULL, hContext);
	return result;

}

