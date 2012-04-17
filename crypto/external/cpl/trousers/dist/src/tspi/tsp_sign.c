
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_Sign(TSS_HCONTEXT tspContext,    /* in */
	       TCS_KEY_HANDLE keyHandle,   /* in */
	       UINT32 areaToSignSize,      /* in */
	       BYTE * areaToSign,  /* in */
	       TPM_AUTH * privAuth,        /* in, out */
	       UINT32 * sigSize,   /* out */
	       BYTE ** sig)        /* out */
{
	UINT64 offset;
	TSS_RESULT result;
	UINT32 handlesLen, decLen, dataLen;
	TCS_HANDLE *handles, handle;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE *dec, *data;


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

	dataLen = sizeof(UINT32) + areaToSignSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, areaToSignSize, data);
	Trspi_LoadBlob(&offset, areaToSignSize, data, areaToSign);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Sign, dataLen, data,
						    &pubKeyHash, &handlesLen, &handles,
						    privAuth, NULL, &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, sigSize, dec);

	if ((*sig = malloc(*sigSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *sigSize);
		*sigSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *sigSize, dec, *sig);

	return result;
}
#endif

