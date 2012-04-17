
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"

TSS_RESULT
Tspi_PcrComposite_SetPcrValue(TSS_HPCRS hPcrComposite,	/* in */
			      UINT32 ulPcrIndex,	/* in */
			      UINT32 ulPcrValueLength,	/* in */
			      BYTE * rgbPcrValue)	/* in */
{
	if (ulPcrValueLength == 0 || rgbPcrValue == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (ulPcrValueLength != TCPA_SHA1_160_HASH_LEN)
		return TSPERR(TSS_E_BAD_PARAMETER);

	return obj_pcrs_set_value(hPcrComposite, ulPcrIndex, ulPcrValueLength, rgbPcrValue);
}

TSS_RESULT
Tspi_PcrComposite_GetPcrValue(TSS_HPCRS hPcrComposite,		/* in */
			      UINT32 ulPcrIndex,		/* in */
			      UINT32 * pulPcrValueLength,	/* out */
			      BYTE ** prgbPcrValue)		/* out */
{
	if (pulPcrValueLength == NULL || prgbPcrValue == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	return obj_pcrs_get_value(hPcrComposite, ulPcrIndex, pulPcrValueLength,
					prgbPcrValue);

}

TSS_RESULT
Tspi_PcrComposite_SelectPcrIndex(TSS_HPCRS hPcrComposite,	/* in */
				 UINT32 ulPcrIndex)		/* in */
{
	return obj_pcrs_select_index(hPcrComposite, ulPcrIndex);
}
