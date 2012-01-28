
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
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_NV_DefineOrReleaseSpace(TSS_HCONTEXT tspContext,	/* in */
				  UINT32 cPubInfoSize,	/* in */
				  BYTE* pPubInfo,		/* in */
				  TCPA_ENCAUTH encAuth,	/* in */
				  TPM_AUTH* pAuth)		/* in, out */
{
	TSS_RESULT result;
	UINT32 dataLen;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TCPA_ENCAUTH) + cPubInfoSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob(&offset, cPubInfoSize, data, pPubInfo);
	Trspi_LoadBlob(&offset, TPM_SHA1_160_HASH_LEN, data, encAuth.authdata);

	result = obj_context_transport_execute(tspContext, TPM_ORD_NV_DefineSpace, dataLen, data,
					       NULL, &handlesLen, NULL, pAuth, NULL, NULL, NULL);
	free(data);

	return result;
}

TSS_RESULT
Transport_NV_WriteValue(TSS_HCONTEXT tspContext,	/* in */
			TSS_NV_INDEX hNVStore,	/* in */
			UINT32 offset,		/* in */
			UINT32 ulDataLength,		/* in */
			BYTE* rgbDataToWrite,	/* in */
			TPM_AUTH* privAuth)		/* in, out */
{
	TSS_RESULT result;
	UINT32 dataLen;
	UINT64 offset64;
	TCS_HANDLE handlesLen = 0;
	BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TSS_NV_INDEX) + (2 * sizeof(UINT32)) + ulDataLength;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset64 = 0;
	Trspi_LoadBlob_UINT32(&offset64, hNVStore, data);
	Trspi_LoadBlob_UINT32(&offset64, offset, data);
	Trspi_LoadBlob_UINT32(&offset64, ulDataLength, data);
	Trspi_LoadBlob(&offset64, ulDataLength, data, rgbDataToWrite);

	result = obj_context_transport_execute(tspContext, TPM_ORD_NV_WriteValue, dataLen, data,
					       NULL, &handlesLen, NULL, privAuth, NULL, NULL, NULL);
	free(data);

	return result;
}

TSS_RESULT
Transport_NV_WriteValueAuth(TSS_HCONTEXT tspContext,	/* in */
			    TSS_NV_INDEX hNVStore,		/* in */
			    UINT32 offset,			/* in */
			    UINT32 ulDataLength,		/* in */
			    BYTE* rgbDataToWrite,		/* in */
			    TPM_AUTH* NVAuth)		/* in, out */
{
	TSS_RESULT result;
	UINT32 dataLen;
	UINT64 offset64;
	TCS_HANDLE handlesLen = 0;
	BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TSS_NV_INDEX) + (2 * sizeof(UINT32)) + ulDataLength;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset64 = 0;
	Trspi_LoadBlob_UINT32(&offset64, hNVStore, data);
	Trspi_LoadBlob_UINT32(&offset64, offset, data);
	Trspi_LoadBlob_UINT32(&offset64, ulDataLength, data);
	Trspi_LoadBlob(&offset64, ulDataLength, data, rgbDataToWrite);

	result = obj_context_transport_execute(tspContext, TPM_ORD_NV_WriteValueAuth, dataLen, data,
					       NULL, &handlesLen, NULL, NVAuth, NULL, NULL, NULL);
	free(data);

	return result;
}


TSS_RESULT
Transport_NV_ReadValue(TSS_HCONTEXT tspContext,	/* in */
		       TSS_NV_INDEX hNVStore,	/* in */
		       UINT32 offset,		/* in */
		       UINT32* pulDataLength,	/* in, out */
		       TPM_AUTH* privAuth,		/* in, out */
		       BYTE** rgbDataRead)		/* out */
{
	TSS_RESULT result;
	UINT32 dataLen, decLen;
	UINT64 offset64;
	TCS_HANDLE handlesLen = 0;
	BYTE *data, *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TSS_NV_INDEX) + sizeof(UINT32) + *pulDataLength;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset64 = 0;
	Trspi_LoadBlob_UINT32(&offset64, hNVStore, data);
	Trspi_LoadBlob_UINT32(&offset64, offset, data);
	Trspi_LoadBlob_UINT32(&offset64, *pulDataLength, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_NV_ReadValue, dataLen, data,
						    NULL, &handlesLen, NULL, privAuth, NULL,
						    &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset64 = 0;
	Trspi_UnloadBlob_UINT32(&offset64, pulDataLength, dec);

	if ((*rgbDataRead = malloc(*pulDataLength)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *pulDataLength);
		*pulDataLength = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset64, *pulDataLength, dec, *rgbDataRead);
	free(dec);

	return result;
}


TSS_RESULT
Transport_NV_ReadValueAuth(TSS_HCONTEXT tspContext,	/* in */
			   TSS_NV_INDEX hNVStore,    /* in */
			   UINT32 offset,		/* in */
			   UINT32* pulDataLength,    /* in, out */
			   TPM_AUTH* NVAuth,		/* in, out */
			   BYTE** rgbDataRead)       /* out */
{
	TSS_RESULT result;
	UINT32 dataLen, decLen;
	UINT64 offset64;
	TCS_HANDLE handlesLen = 0;
	BYTE *data, *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = sizeof(TSS_NV_INDEX) + sizeof(UINT32) + *pulDataLength;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset64 = 0;
	Trspi_LoadBlob_UINT32(&offset64, hNVStore, data);
	Trspi_LoadBlob_UINT32(&offset64, offset, data);
	Trspi_LoadBlob_UINT32(&offset64, *pulDataLength, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_NV_ReadValueAuth, dataLen,
						    data, NULL, &handlesLen, NULL, NVAuth, NULL,
						    &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset64 = 0;
	Trspi_UnloadBlob_UINT32(&offset64, pulDataLength, dec);

	if ((*rgbDataRead = malloc(*pulDataLength)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *pulDataLength);
		*pulDataLength = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset64, *pulDataLength, dec, *rgbDataRead);
	free(dec);

	return result;
}

#endif
