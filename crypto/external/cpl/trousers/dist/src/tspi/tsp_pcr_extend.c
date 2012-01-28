
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
Transport_Extend(TSS_HCONTEXT tspContext,	/* in */
		 TCPA_PCRINDEX pcrNum,	/* in */
		 TCPA_DIGEST inDigest,	/* in */
		 TCPA_PCRVALUE * outDigest)	/* out */
{
	TSS_RESULT result;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	UINT32 decLen;
	BYTE data[sizeof(TCPA_PCRINDEX) + sizeof(TCPA_DIGEST)], *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, pcrNum, data);
	Trspi_LoadBlob(&offset, TPM_SHA1_160_HASH_LEN, data, inDigest.digest);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_Extend, sizeof(data), data,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob(&offset, decLen, dec, outDigest->digest);

	free(dec);

	return TSS_SUCCESS;
}

TSS_RESULT
Transport_PcrRead(TSS_HCONTEXT tspContext,	/* in */
		  TCPA_PCRINDEX pcrNum,	/* in */
		  TCPA_PCRVALUE * outDigest)	/* out */
{
	TSS_RESULT result;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	UINT32 decLen;
	BYTE data[sizeof(TCPA_PCRINDEX)], *dec;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT32(&offset, pcrNum, data);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_PcrRead, sizeof(data),
						    data, NULL, &handlesLen, NULL, NULL, NULL,
						    &decLen, &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob(&offset, decLen, dec, outDigest->digest);

	free(dec);

	return TSS_SUCCESS;
}


TSS_RESULT
Transport_PcrReset(TSS_HCONTEXT tspContext,	/* in */
		   UINT32 pcrDataSizeIn,	/* in */
		   BYTE * pcrDataIn)		/* in */
{
	TSS_RESULT result;
	TCS_HANDLE handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	return obj_context_transport_execute(tspContext, TPM_ORD_PCR_Reset, pcrDataSizeIn,
					     pcrDataIn, NULL, &handlesLen, NULL, NULL, NULL, NULL,
					     NULL);
}
#endif
