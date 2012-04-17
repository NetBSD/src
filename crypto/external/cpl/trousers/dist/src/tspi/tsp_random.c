
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2007
 *
 */


#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "tsplog.h"
#include "obj.h"


#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_GetRandom(TSS_HCONTEXT tspContext,	/* in */
		    UINT32 bytesRequested,	/* in */
		    BYTE ** randomBytes)	/* out */
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
        Trspi_LoadBlob_UINT32(&offset, bytesRequested, data);

        if ((result = obj_context_transport_execute(tspContext, TPM_ORD_GetRandom, sizeof(data),
                                                    data, NULL, &handlesLen, NULL, NULL, NULL,
						    &decLen, &dec)))
                return result;

	*randomBytes = dec;

        return result;

}

TSS_RESULT
Transport_StirRandom(TSS_HCONTEXT tspContext,	/* in */
		     UINT32 inDataSize,	/* in */
		     BYTE * inData)	/* in */
{
	TSS_RESULT result;
        UINT64 offset;
	UINT32 dataLen;
	TCS_HANDLE handlesLen = 0;
        BYTE *data;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

        LogDebugFn("Executing in a transport session");

	dataLen = sizeof(UINT32) + inDataSize;
	if ((data = malloc(dataLen)) == NULL) {
                LogError("malloc of %u bytes failed", dataLen);
                return TSPERR(TSS_E_OUTOFMEMORY);
	}

        offset = 0;
        Trspi_LoadBlob_UINT32(&offset, inDataSize, data);
        Trspi_LoadBlob(&offset, inDataSize, data, inData);

	result = obj_context_transport_execute(tspContext, TPM_ORD_StirRandom, dataLen, data, NULL,
					       &handlesLen, NULL, NULL, NULL, NULL, NULL);
	free(data);

	return result;
}
#endif

