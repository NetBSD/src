
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
#include <time.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_SetOperatorAuth(TSS_HCONTEXT tspContext,	/* in */
			  TCPA_SECRET *operatorAuth)		/* in */
{
	TSS_RESULT result;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE data[sizeof(TCPA_SECRET)];

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob(&offset, TPM_SHA1_160_HASH_LEN, data, operatorAuth->authdata);

	return obj_context_transport_execute(tspContext, TPM_ORD_SetOperatorAuth, sizeof(data),
					     data, NULL, &handlesLen, NULL, NULL, NULL, NULL, NULL);
}
#endif
