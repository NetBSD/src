
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
RPC_UnBind_TP(struct host_table_entry *hte,
			   TCS_KEY_HANDLE keyHandle,	/* in */
			   UINT32 inDataSize,	/* in */
			   BYTE * inData,	/* in */
			   TPM_AUTH * privAuth,	/* in, out */
			   UINT32 * outDataSize,	/* out */
			   BYTE ** outData	/* out */
    ) {
	TSS_RESULT result;
	int i;

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_UNBIND;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &keyHandle, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &inDataSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 3, inData, inDataSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (privAuth != NULL) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 4, privAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (privAuth != NULL) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, privAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, outDataSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*outData = (BYTE *) malloc(*outDataSize);
		if (*outData == NULL) {
			LogError("malloc of %u bytes failed.", *outDataSize);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *outData, *outDataSize, &hte->comm)) {
			free(*outData);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

done:
	return result;
}
