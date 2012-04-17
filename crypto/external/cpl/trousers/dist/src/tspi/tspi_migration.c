
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
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
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_AuthorizeMigrationTicket(TSS_HTPM hTPM,			/* in */
				  TSS_HKEY hMigrationKey,		/* in */
				  TSS_MIGRATION_SCHEME migrationScheme,	/* in */
				  UINT32 * pulMigTicketLength,		/* out */
				  BYTE ** prgbMigTicket)		/* out */
{
	UINT64 offset;
	TCPA_DIGEST digest;
	TCPA_RESULT result;
	TSS_HPOLICY hOwnerPolicy;
	UINT32 migrationKeySize;
	BYTE *migrationKeyBlob;
	TSS_KEY tssKey;
	BYTE pubKeyBlob[0x1000];
	TPM_AUTH ownerAuth;
	UINT32 pubKeySize;
	TSS_HCONTEXT tspContext;
	UINT32 tpmMigrationScheme;
	Trspi_HashCtx hashCtx;

	if (pulMigTicketLength == NULL || prgbMigTicket == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	/*  get the tpm Policy */
	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hOwnerPolicy)))
		return result;

	switch (migrationScheme) {
		case TSS_MS_MIGRATE:
			tpmMigrationScheme = TCPA_MS_MIGRATE;
			break;
		case TSS_MS_REWRAP:
			tpmMigrationScheme = TCPA_MS_REWRAP;
			break;
		case TSS_MS_MAINT:
			tpmMigrationScheme = TCPA_MS_MAINT;
			break;
#ifdef TSS_BUILD_CMK
		case TSS_MS_RESTRICT_MIGRATE:
			tpmMigrationScheme = TPM_MS_RESTRICT_MIGRATE;
			break;

		case TSS_MS_RESTRICT_APPROVE_DOUBLE:
			tpmMigrationScheme = TPM_MS_RESTRICT_APPROVE_DOUBLE;
			break;
#endif
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
			break;
	}

	/*  Get the migration key blob */
	if ((result = obj_rsakey_get_blob(hMigrationKey, &migrationKeySize, &migrationKeyBlob)))
		return result;

	/* First, turn the keyBlob into a TSS_KEY structure */
	offset = 0;
	memset(&tssKey, 0, sizeof(TSS_KEY));
	if ((result = UnloadBlob_TSS_KEY(&offset, migrationKeyBlob, &tssKey))) {
		free_tspi(tspContext, migrationKeyBlob);
		return result;
	}
	free_tspi(tspContext, migrationKeyBlob);

	/* Then pull the _PUBKEY portion out of that struct into a blob */
	offset = 0;
	Trspi_LoadBlob_KEY_PARMS(&offset, pubKeyBlob, &tssKey.algorithmParms);
	Trspi_LoadBlob_STORE_PUBKEY(&offset, pubKeyBlob, &tssKey.pubKey);
	pubKeySize = offset;
	free_key_refs(&tssKey);

	/* Auth */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_AuthorizeMigrationKey);
	result |= Trspi_Hash_UINT16(&hashCtx, tpmMigrationScheme);
	result |= Trspi_HashUpdate(&hashCtx, pubKeySize, pubKeyBlob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_AuthorizeMigrationKey, hOwnerPolicy,
					      FALSE, &digest, &ownerAuth)))
		return result;

	/* Send command */
	if ((result = TCS_API(tspContext)->AuthorizeMigrationKey(tspContext, migrationScheme,
								 pubKeySize, pubKeyBlob, &ownerAuth,
								 pulMigTicketLength,
								 prgbMigTicket)))
		return result;

	/* Validate Auth */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_AuthorizeMigrationKey);
	result |= Trspi_HashUpdate(&hashCtx, *pulMigTicketLength, *prgbMigTicket);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest))) {
		*pulMigTicketLength = 0;
		free(*prgbMigTicket);
		return result;
	}

	if ((result = obj_policy_validate_auth_oiap(hOwnerPolicy, &digest, &ownerAuth))) {
		*pulMigTicketLength = 0;
		free(*prgbMigTicket);
		return result;
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbMigTicket))) {
		*pulMigTicketLength = 0;
		free(*prgbMigTicket);
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_Key_CreateMigrationBlob(TSS_HKEY hKeyToMigrate,		/* in */
			     TSS_HKEY hParentKey,		/* in */
			     UINT32 ulMigTicketLength,		/* in */
			     BYTE * rgbMigTicket,		/* in */
			     UINT32 * pulRandomLength,		/* out */
			     BYTE ** prgbRandom,		/* out */
			     UINT32 * pulMigrationBlobLength,	/* out */
			     BYTE ** prgbMigrationBlob)		/* out */
{
	TPM_AUTH parentAuth, entityAuth;
	TPM_AUTH *pParentAuth;
	TCPA_RESULT result;
	UINT64 offset;
	TCPA_DIGEST digest;
	UINT32 keyToMigrateSize;
	BYTE *keyToMigrateBlob = NULL;
	TSS_HPOLICY hParentPolicy;
	TSS_HPOLICY hMigratePolicy;
	TCPA_MIGRATIONKEYAUTH migAuth;
	TSS_KEY tssKey;
	TCS_KEY_HANDLE parentHandle;
	TSS_BOOL parentUsesAuth;
	UINT32 randomSize;
	BYTE *random = NULL;
	UINT32 blobSize;
	BYTE *blob = NULL;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;

	memset(&tssKey, 0, sizeof(TSS_KEY));

	if (pulRandomLength == NULL || prgbRandom == NULL || rgbMigTicket == NULL ||
	    pulMigrationBlobLength == NULL || prgbMigrationBlob == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_rsakey(hKeyToMigrate))
		return TSPERR(TSS_E_INVALID_HANDLE);

	if ((result = obj_rsakey_get_tsp_context(hKeyToMigrate, &tspContext)))
		return result;

	if ((result = obj_rsakey_get_blob(hKeyToMigrate, &keyToMigrateSize, &keyToMigrateBlob)))
		goto done;

	if ((result = obj_rsakey_get_policy(hParentKey, TSS_POLICY_USAGE, &hParentPolicy,
					    &parentUsesAuth)))
		goto done;

	if ((result = obj_rsakey_get_policy(hKeyToMigrate, TSS_POLICY_MIGRATION, &hMigratePolicy,
					    NULL)))
		goto done;

	/*  Parsing the migration scheme from the blob and key object */
	memset(&migAuth, 0, sizeof(TCPA_MIGRATIONKEYAUTH));

	offset = 0;
	if ((result = Trspi_UnloadBlob_MIGRATIONKEYAUTH(&offset, rgbMigTicket, &migAuth)))
		goto done;

	/* free these now, since none are used below */
	free(migAuth.migrationKey.algorithmParms.parms);
	migAuth.migrationKey.algorithmParms.parmSize = 0;
	free(migAuth.migrationKey.pubKey.key);
	migAuth.migrationKey.pubKey.keyLength = 0;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, keyToMigrateBlob, &tssKey)))
		goto done;

	/* Generate the Authorization data */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CreateMigrationBlob);
	result |= Trspi_Hash_UINT16(&hashCtx, migAuth.migrationScheme);
	result |= Trspi_HashUpdate(&hashCtx, ulMigTicketLength, rgbMigTicket);
	result |= Trspi_Hash_UINT32(&hashCtx, tssKey.encSize);
	result |= Trspi_HashUpdate(&hashCtx, tssKey.encSize, tssKey.encData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (parentUsesAuth) {
		if ((result = secret_PerformAuth_OIAP(hParentPolicy, TPM_ORD_CreateMigrationBlob,
						      hParentPolicy, FALSE, &digest,
						      &parentAuth)))
			goto done;
		pParentAuth = &parentAuth;
	} else {
		pParentAuth = NULL;
	}

	if ((result = secret_PerformAuth_OIAP(hKeyToMigrate, TPM_ORD_CreateMigrationBlob,
					      hMigratePolicy, FALSE, &digest, &entityAuth)))
		goto done;

	if ((result = obj_rsakey_get_tcs_handle(hParentKey, &parentHandle)))
		goto done;

	if ((result = TCS_API(tspContext)->CreateMigrationBlob(tspContext, parentHandle,
							       migAuth.migrationScheme,
							       ulMigTicketLength, rgbMigTicket,
							       tssKey.encSize, tssKey.encData,
							       pParentAuth, &entityAuth,
							       &randomSize, &random,
							       &blobSize, &blob)))
		goto done;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_CreateMigrationBlob);
	result |= Trspi_Hash_UINT32(&hashCtx, randomSize);
	result |= Trspi_HashUpdate(&hashCtx, randomSize, random);
	result |= Trspi_Hash_UINT32(&hashCtx, blobSize);
	result |= Trspi_HashUpdate(&hashCtx, blobSize, blob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (parentUsesAuth) {
		if ((result = obj_policy_validate_auth_oiap(hParentPolicy, &digest, &parentAuth)))
			goto done;
	}

	if ((result = obj_policy_validate_auth_oiap(hMigratePolicy, &digest, &entityAuth)))
		goto done;

	free(tssKey.encData);
	tssKey.encSize = blobSize;
	tssKey.encData = blob;
	/* Set blob to null since it will now be freed during key ref freeing */
	blob = NULL;

	offset = 0;
	LoadBlob_TSS_KEY(&offset, NULL, &tssKey);
	
	*pulMigrationBlobLength = offset;
	*prgbMigrationBlob = calloc_tspi(tspContext, *pulMigrationBlobLength);
	if (*prgbMigrationBlob == NULL) {
		LogError("malloc of %u bytes failed.", *pulMigrationBlobLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}
	offset = 0;
	LoadBlob_TSS_KEY(&offset, *prgbMigrationBlob, &tssKey);

	if (randomSize) {
		if ((result = __tspi_add_mem_entry(tspContext, random)))
			goto done;
	}
	*pulRandomLength = randomSize;
	*prgbRandom = random;

done:
	if (result)
		free(random);
	free_tspi(tspContext, keyToMigrateBlob);
	free_key_refs(&tssKey);
	free(blob);

	return result;
}

TSS_RESULT
Tspi_Key_ConvertMigrationBlob(TSS_HKEY hKeyToMigrate,		/* in */
			      TSS_HKEY hParentKey,		/* in */
			      UINT32 ulRandomLength,		/* in */
			      BYTE * rgbRandom,			/* in */
			      UINT32 ulMigrationBlobLength,	/* in */
			      BYTE * rgbMigrationBlob)		/* in */
{
	TCPA_RESULT result;
	TSS_KEY tssKey;
	UINT32 outDataSize;
	BYTE *outData = NULL;
	TCS_KEY_HANDLE parentHandle;
	TPM_AUTH parentAuth;
	TSS_HPOLICY hParentPolicy;
	TCPA_DIGEST digest;
	TSS_BOOL useAuth;
	TPM_AUTH *pParentAuth;
	TSS_HCONTEXT tspContext;
	Trspi_HashCtx hashCtx;
	UINT64 offset;

	memset(&tssKey, 0, sizeof(TSS_KEY));

	if ((result = obj_rsakey_get_tsp_context(hKeyToMigrate, &tspContext)))
		return result;

	if (!obj_is_rsakey(hParentKey))
		return TSPERR(TSS_E_INVALID_HANDLE);

	/* Get the parent key handle */
	if ((result = obj_rsakey_get_tcs_handle(hParentKey, &parentHandle)))
		return result;

	/* Get the policy */
	if ((result = obj_rsakey_get_policy(hParentKey, TSS_POLICY_USAGE,
					&hParentPolicy, &useAuth)))
		return result;

	offset = 0;
	if ((result = UnloadBlob_TSS_KEY(&offset, rgbMigrationBlob, &tssKey)))
		return result;

	/* Generate the authorization */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ConvertMigrationBlob);
	result |= Trspi_Hash_UINT32(&hashCtx, tssKey.encSize);
	result |= Trspi_HashUpdate(&hashCtx, tssKey.encSize, tssKey.encData);
	result |= Trspi_Hash_UINT32(&hashCtx, ulRandomLength);
	result |= Trspi_HashUpdate(&hashCtx, ulRandomLength, rgbRandom);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (useAuth) {
		if ((result = secret_PerformAuth_OIAP(hParentPolicy, TPM_ORD_ConvertMigrationBlob,
						      hParentPolicy, FALSE, &digest, &parentAuth)))
			goto done;
		pParentAuth = &parentAuth;
	} else {
		pParentAuth = NULL;
	}

	if ((result = TCS_API(tspContext)->ConvertMigrationBlob(tspContext, parentHandle,
								tssKey.encSize, tssKey.encData,
								ulRandomLength, rgbRandom,
								pParentAuth,
								&outDataSize, &outData)))
		goto done;

	/* add validation */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ConvertMigrationBlob);
	result |= Trspi_Hash_UINT32(&hashCtx, outDataSize);
	result |= Trspi_HashUpdate(&hashCtx, outDataSize, outData);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		goto done;

	if (useAuth) {
		if ((result = obj_policy_validate_auth_oiap(hParentPolicy, &digest, &parentAuth)))
			goto done;
	}

	/* Set the key object to the now migrated key */
	if ((result = obj_rsakey_set_tcpakey(hKeyToMigrate, ulMigrationBlobLength, rgbMigrationBlob)))
		goto done;
	if ((result = obj_rsakey_set_privkey(hKeyToMigrate, TRUE, outDataSize, outData)))
		goto done;
	result = obj_rsakey_set_tcs_handle(hKeyToMigrate, 0);

done:
	free_key_refs(&tssKey);
	free(outData);

	return result;
}
