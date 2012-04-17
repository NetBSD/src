
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
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "tcs_tsp.h"
#include "tspps.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "obj.h"


TSS_RESULT
Tspi_Context_GetCapability(TSS_HCONTEXT tspContext,	/* in */
			   TSS_FLAG capArea,		/* in */
			   UINT32 ulSubCapLength,	/* in */
			   BYTE * rgbSubCap,		/* in */
			   UINT32 * pulRespDataLength,	/* out */
			   BYTE ** prgbRespData)	/* out */
{
	TSS_RESULT result;

	if (prgbRespData == NULL || pulRespDataLength == NULL )
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (rgbSubCap == NULL && ulSubCapLength != 0)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (ulSubCapLength > sizeof(UINT32))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if (!obj_is_context(tspContext))
		return TSPERR(TSS_E_INVALID_HANDLE);

	switch (capArea) {
		case TSS_TSPCAP_ALG:
		case TSS_TSPCAP_RETURNVALUE_INFO:
		case TSS_TSPCAP_PLATFORM_INFO:
		case TSS_TSPCAP_MANUFACTURER:
			if (ulSubCapLength != sizeof(UINT32) || !rgbSubCap)
				return TSPERR(TSS_E_BAD_PARAMETER);
			/* fall through */
		case TSS_TSPCAP_VERSION:
		case TSS_TSPCAP_PERSSTORAGE:
			result = internal_GetCap(tspContext, capArea,
						 rgbSubCap ? *(UINT32 *)rgbSubCap : 0,
						 pulRespDataLength,
						 prgbRespData);
			break;
		case TSS_TCSCAP_ALG:
			if (ulSubCapLength != sizeof(UINT32) || !rgbSubCap)
				return TSPERR(TSS_E_BAD_PARAMETER);
			/* fall through */
		case TSS_TCSCAP_VERSION:
		case TSS_TCSCAP_CACHING:
		case TSS_TCSCAP_PERSSTORAGE:
		case TSS_TCSCAP_MANUFACTURER:
		case TSS_TCSCAP_TRANSPORT:
		case TSS_TCSCAP_PLATFORM_CLASS:
			result = RPC_GetCapability(tspContext, capArea, ulSubCapLength, rgbSubCap,
						   pulRespDataLength, prgbRespData);
			break;
		default:
			LogDebug("Invalid capArea: 0x%x", capArea);
			result = TSPERR(TSS_E_BAD_PARAMETER);
			break;
	}

	return result;
}
