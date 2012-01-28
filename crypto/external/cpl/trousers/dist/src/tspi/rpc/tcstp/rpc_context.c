
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
#include "tsplog.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "rpc_tcstp_tsp.h"


TSS_RESULT
RPC_OpenContext_TP(struct host_table_entry* hte,
		       UINT32*                  tpm_version,
		       TCS_CONTEXT_HANDLE*      tcsContext)
{
	TSS_RESULT result;

	initData(&hte->comm, 0);
	hte->comm.hdr.u.ordinal = TCSD_ORD_OPENCONTEXT;
	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, tcsContext, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);

		LogDebugFn("Received TCS Context: 0x%x", *tcsContext);

		if (getData(TCSD_PACKET_TYPE_UINT32, 1, tpm_version, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_CloseContext_TP(struct host_table_entry *hte)
{
	TSS_RESULT result;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_CLOSECONTEXT;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	return result;
}

TSS_RESULT
RPC_FreeMemory_TP(struct host_table_entry *hte,
		  BYTE * pMemory)		/*  in */
{
	free(pMemory);

	return TSS_SUCCESS;
}
