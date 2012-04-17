
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
Transport_Quote2(TSS_HCONTEXT tspContext, /* in */
		 TCS_KEY_HANDLE keyHandle, /* in */
		 TCPA_NONCE *antiReplay, /* in */
		 UINT32 pcrDataSizeIn, /* in */
		 BYTE * pcrDataIn, /* in */
		 TSS_BOOL addVersion, /* in */
		 TPM_AUTH * privAuth, /* in,out */
		 UINT32 * pcrDataSizeOut, /* out */
		 BYTE ** pcrDataOut, /* out */
		 UINT32 * versionInfoSize, /* out */
		 BYTE ** versionInfo, /* out */
		 UINT32 * sigSize, /* out */
		 BYTE ** sig) /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, dataLen, decLen;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	UINT64 offset;
	BYTE *data;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	dataLen = sizeof(TCPA_NONCE) + pcrDataSizeIn + sizeof(TSS_BOOL);
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_NONCE(&offset, data, antiReplay);
	Trspi_LoadBlob(&offset, pcrDataSizeIn, data, pcrDataIn);
	Trspi_LoadBlob_BOOL(&offset, addVersion, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Quote2, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles,
						    privAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_PCR_INFO_SHORT(&offset, dec, NULL);
	*pcrDataSizeOut = offset;

	if ((*pcrDataOut = malloc(*pcrDataSizeOut)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *pcrDataSizeOut);
		*pcrDataSizeOut = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_UnloadBlob(&offset, *pcrDataSizeOut, dec, *pcrDataOut);
	Trspi_UnloadBlob_UINT32(&offset, versionInfoSize, dec);

	if ((*versionInfo = malloc(*versionInfoSize)) == NULL) {
		free(*pcrDataOut);
		*pcrDataOut = NULL;
		*pcrDataSizeOut = 0;
		free(dec);
		LogError("malloc of %u bytes failed", *versionInfoSize);
		*versionInfoSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *versionInfoSize, dec, *versionInfo);

	Trspi_UnloadBlob_UINT32(&offset, sigSize, dec);

	if ((*sig = malloc(*sigSize)) == NULL) {
		free(*versionInfo);
		*versionInfo = NULL;
		*versionInfoSize = 0;
		free(*pcrDataOut);
		*pcrDataOut = NULL;
		*pcrDataSizeOut = 0;
		free(dec);
		LogError("malloc of %u bytes failed", *sigSize);
		*sigSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *sigSize, dec, *sig);
	free(dec);

	return result;
}
#endif

