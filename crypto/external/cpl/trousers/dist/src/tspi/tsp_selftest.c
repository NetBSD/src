
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
Transport_SelfTestFull(TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;
	TCS_HANDLE handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	return obj_context_transport_execute(tspContext, TPM_ORD_SelfTestFull, 0, NULL, NULL,
					     &handlesLen, NULL, NULL, NULL, NULL, NULL);
}

TSS_RESULT
Transport_CertifySelfTest(TSS_HCONTEXT tspContext,	/* in */
			  TCS_KEY_HANDLE keyHandle,	/* in */
			  TCPA_NONCE antiReplay,	/* in */
			  TPM_AUTH * privAuth,	/* in, out */
			  UINT32 * sigSize,	/* out */
			  BYTE ** sig)	/* out */
{
	TSS_RESULT result;
	UINT32 handlesLen, decLen = 0;
	BYTE *dec = NULL;
	UINT64 offset;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;
	TCS_HANDLE *handles, handle;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	if ((result = obj_tcskey_get_pubkeyhash(keyHandle, pubKeyHash.digest)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	handlesLen = 1;
	handle = keyHandle;
	handles = &handle;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_CertifySelfTest,
						    sizeof(TCPA_NONCE), antiReplay.nonce,
						    &pubKeyHash, &handlesLen, &handles, privAuth,
						    NULL, &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, sigSize, dec);

	if ((*sig = malloc(*sigSize)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *sigSize);
		*sigSize = 0;
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(&offset, *sigSize, dec, *sig);

	free(dec);

	return result;
}

TSS_RESULT
Transport_GetTestResult(TSS_HCONTEXT tspContext,	/* in */
			UINT32 * outDataSize,	/* out */
			BYTE ** outData)	/* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0;
	BYTE *dec = NULL;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetTestResult, 0, NULL,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

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
#endif
