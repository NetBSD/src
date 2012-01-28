
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
Transport_DirWriteAuth(TSS_HCONTEXT tspContext,    /* in */
		       TCPA_DIRINDEX dirIndex,     /* in */
		       TCPA_DIRVALUE *newContents,  /* in */
		       TPM_AUTH * ownerAuth)       /* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;
	UINT64 offset;
	BYTE data[sizeof(TCPA_DIRINDEX) + sizeof(TCPA_DIRVALUE)];


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, dirIndex, data);
	Trspi_LoadBlob_DIGEST(&offset, data, (TPM_DIGEST *)newContents);

	result = obj_context_transport_execute(tspContext, TPM_ORD_DirWriteAuth, sizeof(data), data,
					       NULL, &handlesLen, NULL, ownerAuth, NULL, NULL,
					       NULL);

	return result;
}

TSS_RESULT
Transport_DirRead(TSS_HCONTEXT tspContext, /* in */
		  TCPA_DIRINDEX dirIndex,  /* in */
		  TCPA_DIRVALUE * dirValue)        /* out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0, decLen;
	UINT64 offset;
	BYTE data[sizeof(TCPA_DIRINDEX)], *dec;


	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, dirIndex, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_DirRead, sizeof(data), data,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_DIGEST(&offset, dec, dirValue);

	return result;
}
#endif

