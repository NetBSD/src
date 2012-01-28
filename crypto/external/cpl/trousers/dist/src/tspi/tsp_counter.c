
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
Transport_ReadCounter(TSS_HCONTEXT       tspContext,          /* in */
		      TSS_COUNTER_ID     idCounter,           /* in */
		      TPM_COUNTER_VALUE* counterValue)        /* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0;
	BYTE *dec = NULL;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE data[sizeof(UINT32)];

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, idCounter, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_ReadCounter, sizeof(data),
						    data, NULL, &handlesLen, NULL, NULL, NULL,
						    &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_COUNTER_VALUE(&offset, dec, counterValue);

	free(dec);

	return TSS_SUCCESS;
}
#endif
