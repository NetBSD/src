
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
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

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_CreateMigrationBlob(TSS_HCONTEXT tspContext,	/* in */
			      TCS_KEY_HANDLE parentHandle,	/* in */
			      TCPA_MIGRATE_SCHEME migrationType,	/* in */
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
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen, dataLen, decLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *data, *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(parentHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = parentHandle;
	handles = &handle;

	dataLen = sizeof(TCPA_MIGRATE_SCHEME)
		  + MigrationKeyAuthSize
		  + sizeof(UINT32)
		  + encDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, migrationType, data);
	Trspi_LoadBlob(&offset, MigrationKeyAuthSize, data, MigrationKeyAuth);
	Trspi_LoadBlob_UINT32(&offset, encDataSize, data);
	Trspi_LoadBlob(&offset, encDataSize, data, encData);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_CreateMigrationBlob,
						    dataLen, data, &pubKeyHash, &handlesLen,
						    &handles, parentAuth, entityAuth, &decLen,
						    &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, randomSize, dec);

	if ((*random = malloc(*randomSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *randomSize);
		*randomSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *randomSize, dec, *random);

	Trspi_UnloadBlob_UINT32(&offset, outDataSize, dec);

	if ((*outData = malloc(*outDataSize)) == NULL) {
		free(*random);
		*random = NULL;
		*randomSize = 0;
		free(dec);
		LogError("malloc of %u bytes failed", *outDataSize);
		*outDataSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *outDataSize, dec, *outData);
	free(dec);

	return result;
}

TSS_RESULT
Transport_ConvertMigrationBlob(TSS_HCONTEXT tspContext,	/* in */
			       TCS_KEY_HANDLE parentHandle,	/* in */
			       UINT32 inDataSize,	/* in */
			       BYTE * inData,	/* in */
			       UINT32 randomSize,	/* in */
			       BYTE * random,	/* in */
			       TPM_AUTH * parentAuth,	/* in, out */
			       UINT32 * outDataSize,	/* out */
			       BYTE ** outData)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen, dataLen, decLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *data, *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(parentHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = parentHandle;
	handles = &handle;

	dataLen = (2 * sizeof(UINT32)) + randomSize + inDataSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, inDataSize, data);
	Trspi_LoadBlob(&offset, inDataSize, data, inData);
	Trspi_LoadBlob_UINT32(&offset, randomSize, data);
	Trspi_LoadBlob(&offset, randomSize, data, random);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_ConvertMigrationBlob,
						    dataLen, data, &pubKeyHash, &handlesLen,
						    &handles, parentAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, outDataSize, dec);

	if ((*outData = malloc(*outDataSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *outDataSize);
		*outDataSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *outDataSize, dec, *outData);
	free(dec);

	return result;
}

TSS_RESULT
Transport_AuthorizeMigrationKey(TSS_HCONTEXT tspContext,	/* in */
				TCPA_MIGRATE_SCHEME migrateScheme,	/* in */
				UINT32 MigrationKeySize,	/* in */
				BYTE * MigrationKey,	/* in */
				TPM_AUTH * ownerAuth,	/* in, out */
				UINT32 * MigrationKeyAuthSize,	/* out */
				BYTE ** MigrationKeyAuth)	/* out */
{
	UINT64 offset;
	UINT16 tpmMigrateScheme;
	TSS_RESULT result;
	UINT32 handlesLen = 0, dataLen, decLen;
	BYTE *data, *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	/* The TSS_MIGRATE_SCHEME must be changed to a TPM_MIGRATE_SCHEME here, since the TCS
	 * expects a TSS migrate scheme, but this could be wrapped by the TSP before it gets to the
	 * TCS. */
	switch (migrateScheme) {
		case TSS_MS_MIGRATE:
			tpmMigrateScheme = TCPA_MS_MIGRATE;
			break;
		case TSS_MS_REWRAP:
			tpmMigrateScheme = TCPA_MS_REWRAP;
			break;
		case TSS_MS_MAINT:
			tpmMigrateScheme = TCPA_MS_MAINT;
			break;
#ifdef TSS_BUILD_CMK
		case TSS_MS_RESTRICT_MIGRATE:
			tpmMigrateScheme = TPM_MS_RESTRICT_MIGRATE;
			break;

		case TSS_MS_RESTRICT_APPROVE_DOUBLE:
			tpmMigrateScheme = TPM_MS_RESTRICT_APPROVE_DOUBLE;
			break;
#endif
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
			break;
	}

	dataLen = sizeof(TCPA_MIGRATE_SCHEME) + MigrationKeySize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, tpmMigrateScheme, data);
	Trspi_LoadBlob(&offset, MigrationKeySize, data, MigrationKey);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_AuthorizeMigrationKey,
						    dataLen, data, NULL, &handlesLen, NULL,
						    ownerAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	*MigrationKeyAuthSize = decLen;
	*MigrationKeyAuth = dec;

	return result;
}

#endif

