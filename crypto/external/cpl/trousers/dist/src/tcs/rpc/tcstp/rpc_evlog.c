
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
#include <syslog.h>
#include <string.h>
#include <netdb.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "tcs_utils.h"
#include "rpc_tcstp_tcs.h"


TSS_RESULT
tcs_wrap_GetPcrEvent(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_PCR_EVENT *pEvent = NULL;
	TSS_RESULT result;
	UINT32 pcrIndex, number, totalSize;
	BYTE lengthOnly;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &pcrIndex, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &number, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_BYTE, 3, &lengthOnly, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (lengthOnly)
		result = TCS_GetPcrEvent_Internal(hContext, pcrIndex, &number, NULL);
	else
		result = TCS_GetPcrEvent_Internal(hContext, pcrIndex, &number, &pEvent);

	if (result == TSS_SUCCESS) {
		if (lengthOnly == FALSE)
			totalSize = get_pcr_event_size(pEvent);
		else
			totalSize = 0;

		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &number, 0, &data->comm)) {
			if (lengthOnly == FALSE)
				free_external_events(1, pEvent);
			free(pEvent);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (lengthOnly == FALSE) {
			if (setData(TCSD_PACKET_TYPE_PCR_EVENT, 1, pEvent, 0, &data->comm)) {
				free_external_events(1, pEvent);
				free(pEvent);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
			free_external_events(1, pEvent);
			free(pEvent);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}


TSS_RESULT
tcs_wrap_GetPcrEventsByPcr(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_PCR_EVENT *ppEvents = NULL;
	TSS_RESULT result;
	UINT32 firstEvent, eventCount, totalSize, pcrIndex, i, j;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &pcrIndex, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &firstEvent, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &eventCount, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = TCS_GetPcrEventsByPcr_Internal(hContext, pcrIndex, firstEvent, &eventCount, &ppEvents);

	if (result == TSS_SUCCESS) {
		/* XXX totalSize not used */
		for (i = 0, totalSize = 0; i < eventCount; i++)
			totalSize += get_pcr_event_size(&(ppEvents[i]));

		initData(&data->comm, eventCount + 1);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &eventCount, 0, &data->comm)) {
			free_external_events(eventCount, ppEvents);
			free(ppEvents);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		i = 1;
		for (j = 0; j < eventCount; j++) {
			if (setData(TCSD_PACKET_TYPE_PCR_EVENT, i++, &(ppEvents[j]), 0, &data->comm)) {
				free_external_events(eventCount, ppEvents);
				free(ppEvents);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		free_external_events(eventCount, ppEvents);
		free(ppEvents);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetPcrEventLog(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_PCR_EVENT *ppEvents;
	TSS_RESULT result;
	UINT32 eventCount, totalSize, i, j;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	result = TCS_GetPcrEventLog_Internal(hContext, &eventCount, &ppEvents);

	if (result == TSS_SUCCESS) {
		for (i = 0, totalSize = 0; i < eventCount; i++)
			totalSize += get_pcr_event_size(&(ppEvents[i]));

		initData(&data->comm, eventCount + 1);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &eventCount, 0, &data->comm)) {
			free_external_events(eventCount, ppEvents);
			free(ppEvents);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		i = 1;
		for (j = 0; j < eventCount; j++) {
			if (setData(TCSD_PACKET_TYPE_PCR_EVENT, i++, &(ppEvents[j]), 0, &data->comm)) {
				free_external_events(eventCount, ppEvents);
				free(ppEvents);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		free_external_events(eventCount, ppEvents);
		free(ppEvents);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_LogPcrEvent(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_PCR_EVENT event;
	TSS_RESULT result;
	UINT32 number;

	/* Receive */
	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_PCR_EVENT , 1, &event, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = TCS_LogPcrEvent_Internal(hContext, event, &number);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &number, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

void
LoadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event)
{
	LoadBlob_VERSION(offset, blob, (TPM_VERSION *)&(event->versionInfo));
	LoadBlob_UINT32(offset, event->ulPcrIndex, blob);
	LoadBlob_UINT32(offset, event->eventType, blob);

	LoadBlob_UINT32(offset, event->ulPcrValueLength, blob);
	if (event->ulPcrValueLength > 0)
		LoadBlob(offset, event->ulPcrValueLength, blob, event->rgbPcrValue);

	LoadBlob_UINT32(offset, event->ulEventLength, blob);
	if (event->ulEventLength > 0)
		LoadBlob(offset, event->ulEventLength, blob, event->rgbEvent);

}

TSS_RESULT
UnloadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event)
{
	if (!event) {
		UINT32 ulPcrValueLength, ulEventLength;

		UnloadBlob_VERSION(offset, blob, NULL);
		UnloadBlob_UINT32(offset, NULL, blob);
		UnloadBlob_UINT32(offset, NULL, blob);

		UnloadBlob_UINT32(offset, &ulPcrValueLength, blob);
		(*offset) += ulPcrValueLength;

		UnloadBlob_UINT32(offset, &ulEventLength, blob);
		(*offset) += ulEventLength;

		return TSS_SUCCESS;
	}

	UnloadBlob_VERSION(offset, blob, (TPM_VERSION *)&(event->versionInfo));
	UnloadBlob_UINT32(offset, &event->ulPcrIndex, blob);
	UnloadBlob_UINT32(offset, &event->eventType, blob);

	UnloadBlob_UINT32(offset, &event->ulPcrValueLength, blob);
	if (event->ulPcrValueLength > 0) {
		event->rgbPcrValue = malloc(event->ulPcrValueLength);
		if (event->rgbPcrValue == NULL) {
			LogError("malloc of %u bytes failed.", event->ulPcrValueLength);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(offset, event->ulPcrValueLength, blob, event->rgbPcrValue);
	} else {
		event->rgbPcrValue = NULL;
	}

	UnloadBlob_UINT32(offset, &event->ulEventLength, blob);
	if (event->ulEventLength > 0) {
		event->rgbEvent = malloc(event->ulEventLength);
		if (event->rgbEvent == NULL) {
			LogError("malloc of %u bytes failed.", event->ulEventLength);
			free(event->rgbPcrValue);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(offset, event->ulEventLength, blob, event->rgbEvent);
	} else {
		event->rgbEvent = NULL;
	}

	return TSS_SUCCESS;
}
