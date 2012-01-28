/*
 * The Initial Developer of the Original Code is Intel Corporation.
 * Portions created by Intel Corporation are Copyright (C) 2007 Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 *
 * trousers - An open source TCG Software Stack
 *
 * Author: james.xu@intel.com Rossey.liu@intel.com
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "obj.h"
#include "rpc_tcstp_tsp.h"

TSS_RESULT
RPC_NV_DefineOrReleaseSpace_TP(struct host_table_entry *hte,	/* in */
				UINT32 cPubInfoSize,	/* in */
				BYTE* pPubInfo,			/* in */
				TCPA_ENCAUTH encAuth,		/* in */
				TPM_AUTH* pAuth)		/* in,out */
{
	TSS_RESULT result;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_NVDEFINEORRELEASESPACE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &cPubInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 2, pPubInfo, cPubInfoSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_ENCAUTH, 3, &encAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( pAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, pAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		LogDebugFn("getData outputSize");

		if( pAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, pAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

TSS_RESULT
RPC_NV_WriteValue_TP(struct host_table_entry *hte,	/* in */
		      TSS_NV_INDEX hNVStore,		/* in */
		      UINT32 offset,			/* in */
		      UINT32 ulDataLength,		/* in */
		      BYTE* rgbDataToWrite,		/* in */
		      TPM_AUTH* privAuth)		/* in,out */
{
	TSS_RESULT result;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_NVWRITEVALUE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, rgbDataToWrite, ulDataLength, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( privAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, privAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		LogDebugFn("getData outputSize");

		if( privAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, privAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

TSS_RESULT
RPC_NV_WriteValueAuth_TP(struct host_table_entry *hte,	/* in */
			  TSS_NV_INDEX hNVStore,	/* in */
			  UINT32 offset,		/* in */
			  UINT32 ulDataLength,		/* in */
			  BYTE* rgbDataToWrite,		/* in */
			  TPM_AUTH* NVAuth)		/* in,out */
{
	TSS_RESULT result;

	initData(&hte->comm, 6);
	hte->comm.hdr.u.ordinal = TCSD_ORD_NVWRITEVALUEAUTH;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, rgbDataToWrite, ulDataLength, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( NVAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 5, NVAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		LogDebugFn("getData outputSize");

		if( NVAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, NVAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

TSS_RESULT
RPC_NV_ReadValue_TP(struct host_table_entry *hte,	/* in */
		     TSS_NV_INDEX hNVStore,		/* in */
		     UINT32 offset,			/* in */
		     UINT32* pulDataLength,		/* in,out */
		     TPM_AUTH* privAuth,		/* in,out */
		     BYTE** rgbDataRead)		/* out */
{
	TSS_RESULT result;
	UINT32 i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_NVREADVALUE;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, pulDataLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("SetData privAuth\n");
	if( privAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, privAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	LogDebugFn("Send data.\n");
	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	LogDebugFn("result=%u", result);
	if (result == TSS_SUCCESS) {
		i = 0;
		LogDebugFn("getData outputSize");

		if( privAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, privAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulDataLength, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*rgbDataRead = (BYTE *) malloc(*pulDataLength);
		if (*rgbDataRead == NULL) {
			LogError("malloc of %u bytes failed.", *pulDataLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebugFn("getData rgbDataRead (pulDataLength=%u)", *pulDataLength);
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *rgbDataRead, *pulDataLength, &hte->comm)) {
			free(*rgbDataRead);
			*rgbDataRead = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

TSS_RESULT
RPC_NV_ReadValueAuth_TP(struct host_table_entry *hte,	/* in */
			 TSS_NV_INDEX hNVStore,		/* in */
			 UINT32 offset,			/* in */
			 UINT32* pulDataLength,		/* in,out */
			 TPM_AUTH* NVAuth,		/* in,out */
			 BYTE** rgbDataRead)		/* out */
{
	TSS_RESULT result;
	UINT32 i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_NVREADVALUEAUTH;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);
	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, pulDataLength, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if( NVAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, NVAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);
	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		LogDebugFn("getData outputSize");

		if( NVAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, NVAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulDataLength, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		*rgbDataRead = (BYTE *) malloc(*pulDataLength);
		if (*rgbDataRead == NULL) {
			LogError("malloc of %u bytes failed.", *pulDataLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		LogDebugFn("getData rgbDataRead (pulDataLength=%u)", *pulDataLength);
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *rgbDataRead, *pulDataLength, &hte->comm)) {
			free(*rgbDataRead);
			*rgbDataRead = NULL;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}

done:
	LogDebugFn("result=%u", result);
	return result;
}

