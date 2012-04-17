
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

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_Hash_SetHashValue(TSS_HHASH hHash,			/* in */
		       UINT32 ulHashValueLength,	/* in */
		       BYTE * rgbHashValue)		/* in */
{
	if (ulHashValueLength == 0 || rgbHashValue == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	return obj_hash_set_value(hHash, ulHashValueLength, rgbHashValue);
}

TSS_RESULT
Tspi_Hash_GetHashValue(TSS_HHASH hHash,			/* in */
		       UINT32 * pulHashValueLength,	/* out */
		       BYTE ** prgbHashValue)		/* out */
{
	if (pulHashValueLength == NULL || prgbHashValue == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	return obj_hash_get_value(hHash, pulHashValueLength, prgbHashValue);
}

TSS_RESULT
Tspi_Hash_UpdateHashValue(TSS_HHASH hHash,	/* in */
			  UINT32 ulDataLength,	/* in */
			  BYTE *rgbData)	/* in */
{
	if (rgbData == NULL && ulDataLength != 0)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (ulDataLength == 0)
		return TSS_SUCCESS;

	return obj_hash_update_value(hHash, ulDataLength, rgbData);
}
