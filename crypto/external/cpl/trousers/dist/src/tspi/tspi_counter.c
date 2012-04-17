
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
Tspi_TPM_ReadCounter(TSS_HTPM hTPM,		/* in */
		     UINT32*  counterValue)	/* out */
{
	TSS_HCONTEXT tspContext;
	TCPA_RESULT result;
	TSS_COUNTER_ID counterID;
	TPM_COUNTER_VALUE counter_value;

	if (counterValue == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if ((result = obj_tpm_get_current_counter(hTPM, &counterID)))
		return result;

	if ((result = TCS_API(tspContext)->ReadCounter(tspContext, counterID, &counter_value)))
		return result;

	*counterValue = counter_value.counter;

	return TSS_SUCCESS;
}
