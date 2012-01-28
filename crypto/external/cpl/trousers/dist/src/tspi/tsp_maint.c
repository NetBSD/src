
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
Transport_CreateMaintenanceArchive(TSS_HCONTEXT tspContext,	/* in */
				   TSS_BOOL generateRandom,	/* in */
				   TPM_AUTH * ownerAuth,	/* in, out */
				   UINT32 * randomSize,	/* out */
				   BYTE ** random,	/* out */
				   UINT32 * archiveSize,	/* out */
				   BYTE ** archive)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	BYTE *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_CreateMaintenanceArchive,
						    sizeof(TSS_BOOL), (BYTE *)&generateRandom, NULL,
						    &handlesLen, NULL, ownerAuth, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, randomSize, dec);
	if (*randomSize > 0) {
		if ((*random = malloc(*randomSize)) == NULL) {
			*randomSize = 0;
			free(dec);
			LogError("malloc of %u bytes failed", *randomSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(&offset, *randomSize, dec, *random);
	}

	Trspi_UnloadBlob_UINT32(&offset, archiveSize, dec);
	if ((*archive = malloc(*archiveSize)) == NULL) {
		free(*random);
		*random = NULL;
		*randomSize = 0;
		free(dec);
		LogError("malloc of %u bytes failed", *archiveSize);
		*archiveSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *archiveSize, dec, *archive);
	free(dec);

	return result;
}

TSS_RESULT
Transport_LoadMaintenanceArchive(TSS_HCONTEXT tspContext,	/* in */
				 UINT32 dataInSize,	/* in */
				 BYTE * dataIn, /* in */
				 TPM_AUTH * ownerAuth,	/* in, out */
				 UINT32 * dataOutSize,	/* out */
				 BYTE ** dataOut)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	BYTE *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_LoadMaintenanceArchive,
						    dataInSize, dataIn, NULL, &handlesLen, NULL,
						    ownerAuth, NULL, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, dataOutSize, dec);

	/* sacrifice 4 bytes */
	*dataOut = &dec[offset];

	return result;
}

TSS_RESULT
Transport_KillMaintenanceFeature(TSS_HCONTEXT tspContext,	/* in */
				 TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	return obj_context_transport_execute(tspContext, TPM_ORD_KillMaintenanceFeature, 0, NULL,
					     NULL, &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);
}

TSS_RESULT
Transport_LoadManuMaintPub(TSS_HCONTEXT tspContext,	/* in */
			   TCPA_NONCE antiReplay,	/* in */
			   UINT32 PubKeySize,	/* in */
			   BYTE * PubKey,	/* in */
			   TCPA_DIGEST * checksum)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, dataLen, decLen;
	BYTE *data, *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TCPA_NONCE) + PubKeySize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, TPM_SHA1_160_HASH_LEN, data, antiReplay.nonce);
	Trspi_LoadBlob(&offset, PubKeySize, data, PubKey);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_LoadManuMaintPub,
						    dataLen, data, NULL, &handlesLen, NULL, NULL,
						    NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_DIGEST(&offset, dec, checksum);
	free(dec);

	return result;
}

TSS_RESULT
Transport_ReadManuMaintPub(TSS_HCONTEXT tspContext,	/* in */
			   TCPA_NONCE antiReplay,	/* in */
			   TCPA_DIGEST * checksum)	/* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	BYTE *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_ReadManuMaintPub,
						    sizeof(TCPA_NONCE), antiReplay.nonce, NULL,
						    &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_DIGEST(&offset, dec, checksum);
	free(dec);

	return result;
}
#endif

