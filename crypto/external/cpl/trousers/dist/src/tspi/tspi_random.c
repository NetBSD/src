
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
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


TSS_RESULT
Tspi_TPM_GetRandom(TSS_HTPM hTPM,		/* in */
		   UINT32 ulRandomDataLength,	/* in */
		   BYTE ** prgbRandomData)	/* out */
{
	TSS_HCONTEXT tspContext;
	TSS_RESULT result;

	if (prgbRandomData == NULL || ulRandomDataLength > 4096)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (ulRandomDataLength == 0)
		return TSS_SUCCESS;

	if ((result = TCS_API(tspContext)->GetRandom(tspContext, ulRandomDataLength,
						     prgbRandomData)))
		return result;

	if ((result = __tspi_add_mem_entry(tspContext, *prgbRandomData))) {
		free(*prgbRandomData);
		*prgbRandomData = NULL;
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_StirRandom(TSS_HTPM hTPM,		/* in */
		    UINT32 ulEntropyDataLength,	/* in */
		    BYTE * rgbEntropyData)	/* in */
{
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;

	if (ulEntropyDataLength > 0 && rgbEntropyData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = TCS_API(tspContext)->StirRandom(tspContext, ulEntropyDataLength,
						      rgbEntropyData)))
		return result;

	return TSS_SUCCESS;
}
