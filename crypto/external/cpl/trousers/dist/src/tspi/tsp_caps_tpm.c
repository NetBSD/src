
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
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


/*
 * This function provides a funnel through which all the TCSP_SetCapability requests can be
 * sent.  This will keep the owner auth code from being duplicated around the TSP.
 */
TSS_RESULT
TSP_SetCapability(TSS_HCONTEXT tspContext,
		  TSS_HTPM hTPM,
		  TSS_HPOLICY hTPMPolicy,
		  TPM_CAPABILITY_AREA tcsCapArea,
		  UINT32 subCap,
		  TSS_BOOL value)
{
	TSS_RESULT result;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST digest;
	TPM_AUTH auth;

	subCap = endian32(subCap);

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetCapability);
	result |= Trspi_Hash_UINT32(&hashCtx, tcsCapArea);
	result |= Trspi_Hash_UINT32(&hashCtx, (UINT32)sizeof(UINT32));
	result |= Trspi_HashUpdate(&hashCtx, (UINT32)sizeof(UINT32), (BYTE *)&subCap);
	result |= Trspi_Hash_UINT32(&hashCtx, (UINT32)sizeof(TSS_BOOL));
	result |= Trspi_Hash_BOOL(&hashCtx, value);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_SetCapability, hTPMPolicy, FALSE,
					      &digest, &auth)))
		return result;

	if ((result = TCS_API(tspContext)->SetCapability(tspContext, tcsCapArea, sizeof(UINT32),
							 (BYTE *)&subCap, sizeof(TSS_BOOL),
							 (BYTE *)&value, &auth)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_SetCapability);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	return obj_policy_validate_auth_oiap(hTPMPolicy, &digest, &auth);
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_GetTPMCapability(TSS_HCONTEXT        tspContext,	/* in */
			   TPM_CAPABILITY_AREA capArea,	/* in */
			   UINT32              subCapLen,	/* in */
			   BYTE*               subCap,	/* in */
			   UINT32*             respLen,	/* out */
			   BYTE**              resp)	/* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0, dataLen;
	BYTE *dec = NULL;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = (2 * sizeof(UINT32)) + subCapLen;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, capArea, data);
	Trspi_LoadBlob_UINT32(&offset, subCapLen, data);
	Trspi_LoadBlob(&offset, subCapLen, data, subCap);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetCapability, dataLen,
						    data, NULL, &handlesLen, NULL, NULL, NULL,
						    &decLen, &dec))) {
		free(data);
		return result;
	}
	free(data);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, respLen, dec);

	if ((*resp = malloc(*respLen)) == NULL) {
		free(dec);
		LogError("malloc of %u bytes failed", *respLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	Trspi_UnloadBlob(&offset, *respLen, dec, *resp);
	free(dec);

	return result;

}

TSS_RESULT
Transport_SetCapability(TSS_HCONTEXT tspContext,	/* in */
			TCPA_CAPABILITY_AREA capArea,	/* in */
			UINT32 subCapSize,	/* in */
			BYTE * subCap,	/* in */
			UINT32 valueSize,	/* in */
			BYTE * value,	/* in */
			TPM_AUTH *ownerAuth) /* in, out */
{
	TSS_RESULT result;
	UINT32 dataLen;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	dataLen = (3 * sizeof(UINT32)) + subCapSize + valueSize;
	if ((data = malloc(dataLen)) == NULL) {
		LogError("malloc of %u bytes failed", dataLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, capArea, data);
	Trspi_LoadBlob_UINT32(&offset, subCapSize, data);
	Trspi_LoadBlob(&offset, subCapSize, data, subCap);
	Trspi_LoadBlob_UINT32(&offset, valueSize, data);
	Trspi_LoadBlob(&offset, valueSize, data, value);

	result = obj_context_transport_execute(tspContext, TPM_ORD_SetCapability, dataLen, data,
					       NULL, &handlesLen, NULL, NULL, NULL, NULL, NULL);

	free(data);
	return result;
}
#endif
