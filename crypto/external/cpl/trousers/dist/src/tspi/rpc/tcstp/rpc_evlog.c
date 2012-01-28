
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
RPC_LogPcrEvent_TP(struct host_table_entry *hte,
			       TSS_PCR_EVENT Event,	/*  in  */
			       UINT32 * pNumber	/*  out */
    ) {
	TSS_RESULT result;

	initData(&hte->comm, 2);
	hte->comm.hdr.u.ordinal = TCSD_ORD_LOGPCREVENT;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_PCR_EVENT, 1, &Event, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pNumber, 0, &hte->comm))
			result = TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return result;
}

TSS_RESULT
RPC_GetPcrEvent_TP(struct host_table_entry *hte,
			       UINT32 PcrIndex,	/* in */
			       UINT32 * pNumber,	/* in, out */
			       TSS_PCR_EVENT ** ppEvent	/* out */
    ) {
	TSS_RESULT result;
	BYTE lengthOnly = (ppEvent == NULL) ? TRUE : FALSE;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETPCREVENT;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &PcrIndex, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_UINT32, 2, pNumber, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_BYTE, 3, &lengthOnly, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pNumber, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (ppEvent) {
			*ppEvent = malloc(sizeof(TSS_PCR_EVENT));
			if (*ppEvent == NULL) {
				LogError("malloc of %zd bytes failed.",
					 sizeof(TSS_PCR_EVENT));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			if (getData(TCSD_PACKET_TYPE_PCR_EVENT, 1, *ppEvent, 0, &hte->comm)) {
				free(*ppEvent);
				*ppEvent = NULL;
				result = TSPERR(TSS_E_INTERNAL_ERROR);
			}
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetPcrEventsByPcr_TP(struct host_table_entry *hte,
				     UINT32 PcrIndex,	/* in */
				     UINT32 FirstEvent,	/* in */
				     UINT32 * pEventCount,	/* in, out */
				     TSS_PCR_EVENT ** ppEvents	/* out */
    ) {
	TSS_RESULT result;
	UINT32 i, j;

	initData(&hte->comm, 4);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETPCREVENTBYPCR;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &PcrIndex, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &FirstEvent, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	if (setData(TCSD_PACKET_TYPE_UINT32, 3, pEventCount, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pEventCount, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (*pEventCount > 0) {
			*ppEvents = calloc_tspi(hte->tspContext,
						sizeof(TSS_PCR_EVENT) * (*pEventCount));
			if (*ppEvents == NULL) {
				LogError("malloc of %zd bytes failed.", sizeof(TSS_PCR_EVENT) * (*pEventCount));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			i = 1;
			for (j = 0; j < (*pEventCount); j++) {
				if (getData(TCSD_PACKET_TYPE_PCR_EVENT, i++, &((*ppEvents)[j]), 0, &hte->comm)) {
					free(*ppEvents);
					*ppEvents = NULL;
					result = TSPERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
			}
		} else {
			*ppEvents = NULL;
		}
	}

done:
	return result;
}

TSS_RESULT
RPC_GetPcrEventLog_TP(struct host_table_entry *hte,
				  UINT32 * pEventCount,	/* out */
				  TSS_PCR_EVENT ** ppEvents	/* out */
    ) {
	TSS_RESULT result;
	int i, j;

	initData(&hte->comm, 1);
	hte->comm.hdr.u.ordinal = TCSD_ORD_GETPCREVENTLOG;
	LogDebugFn("TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (getData(TCSD_PACKET_TYPE_UINT32, 0, pEventCount, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		if (*pEventCount > 0) {
			*ppEvents = calloc_tspi(hte->tspContext,
						sizeof(TSS_PCR_EVENT) * (*pEventCount));
			if (*ppEvents == NULL) {
				LogError("malloc of %zd bytes failed.",
					 sizeof(TSS_PCR_EVENT) * (*pEventCount));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			i = 1;
			for (j = 0; (UINT32)j < (*pEventCount); j++) {
				if (getData(TCSD_PACKET_TYPE_PCR_EVENT, i++, &((*ppEvents)[j]), 0, &hte->comm)) {
					free(*ppEvents);
					*ppEvents = NULL;
					result = TSPERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
			}
		} else {
			*ppEvents = NULL;
		}
	}

done:
	return result;
}
