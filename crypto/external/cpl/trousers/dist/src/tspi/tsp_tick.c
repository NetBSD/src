
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

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_ReadCurrentTicks(TSS_HCONTEXT tspContext,           /* in */
			   UINT32*      pulCurrentTime,       /* out */
			   BYTE**       prgbCurrentTime)      /* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0;
	BYTE *dec = NULL;
	TCS_HANDLE handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetTicks, 0, NULL,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	*pulCurrentTime = decLen;
	*prgbCurrentTime = dec;

	return TSS_SUCCESS;
}

TSS_RESULT
Transport_TickStampBlob(TSS_HCONTEXT   tspContext,            /* in */
			TCS_KEY_HANDLE hKey,                  /* in */
			TPM_NONCE*     antiReplay,            /* in */
			TPM_DIGEST*    digestToStamp,	      /* in */
			TPM_AUTH*      privAuth,              /* in, out */
			UINT32*        pulSignatureLength,    /* out */
			BYTE**         prgbSignature,	      /* out */
			UINT32*        pulTickCountLength,    /* out */
			BYTE**         prgbTickCount)	      /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen = 0;
	TCS_HANDLE *handles, handle;
	BYTE *dec = NULL;
	UINT64 offset;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	BYTE data[sizeof(TPM_NONCE) + sizeof(TPM_DIGEST)];

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

	offset = 0;
	Trspi_LoadBlob_NONCE(&offset, data, antiReplay);
	Trspi_LoadBlob_DIGEST(&offset, data, digestToStamp);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_TickStampBlob, sizeof(data),
						    data, &pubKeyHash, &handlesLen, &handles,
						    privAuth, NULL, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_CURRENT_TICKS(&offset, dec, NULL);
	*pulTickCountLength = (UINT32)offset;
	if ((*prgbTickCount = malloc(*pulTickCountLength)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *pulTickCountLength);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	offset = 0;
	Trspi_UnloadBlob(&offset, *pulTickCountLength, dec, *prgbTickCount);

	Trspi_UnloadBlob_UINT32(&offset, pulSignatureLength, dec);
	if ((*prgbSignature = malloc(*pulSignatureLength)) == NULL) {
		free(dec);
		free(*prgbTickCount);
		*pulTickCountLength = 0;
		LogError("malloc of %u bytes failed", *pulSignatureLength);
		*pulSignatureLength = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *pulSignatureLength, dec, *prgbSignature);

	free(dec);

	return result;
}
#endif
