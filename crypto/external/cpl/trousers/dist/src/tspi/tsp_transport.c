
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


TSS_RESULT
Transport_LoadKeyByBlob(TSS_HCONTEXT     hContext,
			TPM_COMMAND_CODE ordinal,
			TSS_HKEY         hParentKey,
			UINT32           ulBlobLength,
			BYTE*            rgbBlobData,
			TPM_AUTH*        pAuth,
			TCS_KEY_HANDLE*  phKey,
			TPM_KEY_HANDLE*  phSlot)
{
	TSS_RESULT result;
	UINT32 handleListSize, decLen;
	TCS_KEY_HANDLE hTCSParentKey;
	TCS_HANDLE *handleList;
	BYTE *dec = NULL;
	TPM_DIGEST pubKeyHash;
	Trspi_HashCtx hashCtx;


	if ((result = obj_context_transport_init(hContext)) == TSS_TSPATTRIB_DISABLE_TRANSPORT) {
		if ((result = obj_rsakey_get_tcs_handle(hParentKey, &hTCSParentKey)))
			return result;

		return TCSP_LoadKeyByBlob(hContext, hTCSParentKey, ulBlobLength, rgbBlobData, pAuth,
					  phKey, phSlot);
	} else if (result != TSS_TSPATTRIB_ENABLE_TRANSPORT)
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_rsakey_get_transport_attribs(hParentKey, &hTCSParentKey, &pubKeyHash)))
		return result;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_DIGEST(&hashCtx, pubKeyHash.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, pubKeyHash.digest)))
		return result;

	/* Call ExecuteTransport */
	handleListSize = 1;
	if ((handleList = malloc(sizeof(TCS_HANDLE))) == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(TCS_HANDLE));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	*handleList = hTCSParentKey;

	if ((result = obj_context_transport_execute(hContext, ordinal, ulBlobLength,
						    rgbBlobData, &pubKeyHash, &handleListSize,
						    &handleList, pAuth, NULL, &decLen, &dec))) {
		free(handleList);
		return result;
	}

	if (handleListSize == 1)
		*phKey = *(TCS_KEY_HANDLE *)handleList;
	else
		result = TSPERR(TSS_E_INTERNAL_ERROR);

	free(handleList);
	free(dec);

	return result;
}
