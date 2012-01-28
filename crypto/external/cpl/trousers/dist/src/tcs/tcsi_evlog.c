
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
#include <limits.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "tcslog.h"
#include "tcsem.h"


TSS_RESULT
TCS_LogPcrEvent_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			 TSS_PCR_EVENT Event,		/* in */
			 UINT32 *pNumber)		/* out */
{
	TSS_RESULT result;

	if((result = ctx_verify_context(hContext)))
		return result;

	if(Event.ulPcrIndex >= tpm_metrics.num_pcrs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if (tcsd_options.kernel_pcrs & (1 << Event.ulPcrIndex)) {
		LogInfo("PCR %d is configured to be kernel controlled. Event logging denied.",
				Event.ulPcrIndex);
		return TCSERR(TSS_E_FAIL);
	}

	if (tcsd_options.firmware_pcrs & (1 << Event.ulPcrIndex)) {
		LogInfo("PCR %d is configured to be firmware controlled. Event logging denied.",
				Event.ulPcrIndex);
		return TCSERR(TSS_E_FAIL);
	}

	return event_log_add(&Event, pNumber);
}

/* This routine will handle creating the TSS_PCR_EVENT structures from log
 * data produced by an external source. The external source in mind here
 * is the log of PCR extends done by the kernel from beneath the TSS
 * (via direct calls to the device driver).
 */
TSS_RESULT
TCS_GetExternalPcrEvent(UINT32 PcrIndex,		/* in */
			UINT32 *pNumber,		/* in, out */
			TSS_PCR_EVENT **ppEvent)	/* out */
{
	FILE *log_handle;
	char *source;

	if (tcsd_options.kernel_pcrs & (1 << PcrIndex)) {
		source = tcsd_options.kernel_log_file;

		if (tcs_event_log->kernel_source != NULL) {
			if (tcs_event_log->kernel_source->open((void *)source,
							       (FILE **) &log_handle))
				return TCSERR(TSS_E_INTERNAL_ERROR);

			if (tcs_event_log->kernel_source->get_entry(log_handle, PcrIndex,
						pNumber, ppEvent)) {
				tcs_event_log->kernel_source->close(log_handle);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}

			tcs_event_log->kernel_source->close(log_handle);
		} else {
			LogError("No source for externel kernel events was compiled in, but "
					"the tcsd is configured to use one! (see %s)",
					tcsd_config_file);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else if (tcsd_options.firmware_pcrs & (1 << PcrIndex)) {
		source = tcsd_options.firmware_log_file;

		if (tcs_event_log->firmware_source != NULL) {
			if (tcs_event_log->firmware_source->open((void *)source, &log_handle))
				return TCSERR(TSS_E_INTERNAL_ERROR);

			if (tcs_event_log->firmware_source->get_entry(log_handle, PcrIndex,
						pNumber, ppEvent)) {
				tcs_event_log->firmware_source->close(log_handle);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}

			tcs_event_log->firmware_source->close(log_handle);
		} else {
			LogError("No source for externel firmware events was compiled in, but "
					"the tcsd is configured to use one! (see %s)",
					tcsd_config_file);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		LogError("PCR index %d not flagged as kernel or firmware controlled.", PcrIndex);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_GetPcrEvent_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			 UINT32 PcrIndex,		/* in */
			 UINT32 *pNumber,		/* in, out */
			 TSS_PCR_EVENT **ppEvent)	/* out */
{
	TSS_RESULT result;
	TSS_PCR_EVENT *event;

	if ((result = ctx_verify_context(hContext)))
		return result;

	if(PcrIndex >= tpm_metrics.num_pcrs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	/* if this is a kernel or firmware controlled PCR, call an external routine */
        if ((tcsd_options.kernel_pcrs & (1 << PcrIndex)) ||
	    (tcsd_options.firmware_pcrs & (1 << PcrIndex))) {
		MUTEX_LOCK(tcs_event_log->lock);
		result =  TCS_GetExternalPcrEvent(PcrIndex, pNumber, ppEvent);
		MUTEX_UNLOCK(tcs_event_log->lock);

		return result;
	}

	if (ppEvent == NULL) {
		MUTEX_LOCK(tcs_event_log->lock);

		*pNumber = get_num_events(PcrIndex);

		MUTEX_UNLOCK(tcs_event_log->lock);
	} else {
		*ppEvent = calloc(1, sizeof(TSS_PCR_EVENT));
		if (*ppEvent == NULL) {
			LogError("malloc of %zd bytes failed.", sizeof(TSS_PCR_EVENT));
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		event = get_pcr_event(PcrIndex, *pNumber);
		if (event == NULL) {
			free(*ppEvent);
			return TCSERR(TSS_E_BAD_PARAMETER);
		}

		if ((result = copy_pcr_event(*ppEvent, event))) {
			free(*ppEvent);
			return result;
		}
	}

	return TSS_SUCCESS;
}

/* This routine will handle creating the TSS_PCR_EVENT structures from log
 * data produced by an external source. The external source in mind here
 * is the log of PCR extends done by the kernel from beneath the TSS
 * (via direct calls to the device driver).
 */
TSS_RESULT
TCS_GetExternalPcrEventsByPcr(UINT32 PcrIndex,		/* in */
				UINT32 FirstEvent,		/* in */
				UINT32 *pEventCount,		/* in, out */
				TSS_PCR_EVENT **ppEvents)	/* out */
{
	FILE *log_handle;
	char *source;

	if (tcsd_options.kernel_pcrs & (1 << PcrIndex)) {
		source = tcsd_options.kernel_log_file;

		if (tcs_event_log->kernel_source != NULL) {
			if (tcs_event_log->kernel_source->open((void *)source, &log_handle))
				return TCSERR(TSS_E_INTERNAL_ERROR);

			if (tcs_event_log->kernel_source->get_entries_by_pcr(log_handle, PcrIndex,
						FirstEvent, pEventCount, ppEvents)) {
				tcs_event_log->kernel_source->close(log_handle);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}

			tcs_event_log->kernel_source->close(log_handle);
		} else {
			LogError("No source for externel kernel events was compiled in, but "
					"the tcsd is configured to use one! (see %s)",
					tcsd_config_file);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else if (tcsd_options.firmware_pcrs & (1 << PcrIndex)) {
		source = tcsd_options.firmware_log_file;

		if (tcs_event_log->firmware_source != NULL) {
			if (tcs_event_log->firmware_source->open((void *)source, &log_handle))
				return TCSERR(TSS_E_INTERNAL_ERROR);

			if (tcs_event_log->firmware_source->get_entries_by_pcr(log_handle, PcrIndex,
						FirstEvent, pEventCount, ppEvents)) {
				tcs_event_log->firmware_source->close(log_handle);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}

			tcs_event_log->firmware_source->close(log_handle);
		} else {
			LogError("No source for externel firmware events was compiled in, but "
					"the tcsd is configured to use one! (see %s)",
					tcsd_config_file);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else {
		LogError("PCR index %d not flagged as kernel or firmware controlled.", PcrIndex);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_GetPcrEventsByPcr_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
				UINT32 PcrIndex,		/* in */
				UINT32 FirstEvent,		/* in */
				UINT32 *pEventCount,		/* in, out */
				TSS_PCR_EVENT **ppEvents)	/* out */
{
	UINT32 lastEventNumber, i, eventIndex;
	TSS_RESULT result;
	struct event_wrapper *tmp;

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (PcrIndex >= tpm_metrics.num_pcrs)
		return TCSERR(TSS_E_BAD_PARAMETER);

	/* if this is a kernel or firmware controlled PCR, call an external routine */
        if ((tcsd_options.kernel_pcrs & (1 << PcrIndex)) ||
	    (tcsd_options.firmware_pcrs & (1 << PcrIndex))) {
		MUTEX_LOCK(tcs_event_log->lock);
		result = TCS_GetExternalPcrEventsByPcr(PcrIndex, FirstEvent,
							pEventCount, ppEvents);
		MUTEX_UNLOCK(tcs_event_log->lock);

		return result;
	}

	MUTEX_LOCK(tcs_event_log->lock);

	lastEventNumber = get_num_events(PcrIndex);

	MUTEX_UNLOCK(tcs_event_log->lock);

	/* if pEventCount is larger than the number of events to return, just return less.
	 * *pEventCount will be set to the number returned below.
	 */
	lastEventNumber = MIN(lastEventNumber, FirstEvent + *pEventCount);

	if (FirstEvent > lastEventNumber)
		return TCSERR(TSS_E_BAD_PARAMETER);

	if (lastEventNumber == 0) {
		*pEventCount = 0;
		*ppEvents = NULL;
		return TSS_SUCCESS;
	}

	/* FirstEvent is 0 indexed see TSS 1.1b spec section 4.7.2.2.3. That means that
	 * the following calculation is not off by one. :-)
	 */
	*ppEvents = calloc((lastEventNumber - FirstEvent), sizeof(TSS_PCR_EVENT));
	if (*ppEvents == NULL) {
		LogError("malloc of %zd bytes failed.",
			 sizeof(TSS_PCR_EVENT) * (lastEventNumber - FirstEvent));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	MUTEX_LOCK(tcs_event_log->lock);

	tmp = tcs_event_log->lists[PcrIndex];

	/* move through the list until we get to the first event requested */
	for (i = 0; i < FirstEvent; i++)
		tmp = tmp->next;

	/* copy events from the first requested to the last requested */
	for (eventIndex = 0; i < lastEventNumber; eventIndex++, i++) {
		copy_pcr_event(&((*ppEvents)[eventIndex]), &(tmp->event));
		tmp = tmp->next;
	}

	MUTEX_UNLOCK(tcs_event_log->lock);

	*pEventCount = eventIndex;

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_GetPcrEventLog_Internal(TCS_CONTEXT_HANDLE hContext,/* in  */
			    UINT32 *pEventCount,	/* out */
			    TSS_PCR_EVENT **ppEvents)	/* out */
{
	TSS_RESULT result;
	UINT32 i, j, event_count, aggregate_count = 0;
	struct event_wrapper *tmp;
	TSS_PCR_EVENT *event_list = NULL, *aggregate_list = NULL;

	if ((result = ctx_verify_context(hContext)))
		return result;

	MUTEX_LOCK(tcs_event_log->lock);

	/* for each PCR index, if its externally controlled, get the total number of events
	 * externally, else copy the events from the TCSD list. Then tack that list onto a
	 * master list to returned. */
	for (i = 0; i < tpm_metrics.num_pcrs; i++) {
		if ((tcsd_options.kernel_pcrs & (1 << i)) ||
		    (tcsd_options.firmware_pcrs & (1 << i))) {
			/* A kernel or firmware controlled PCR event list */
			event_count = UINT_MAX;
			if ((result = TCS_GetExternalPcrEventsByPcr(i, 0, &event_count,
								    &event_list))) {
				LogDebug("Getting External event list for PCR %u failed", i);
				free(aggregate_list);
				goto error;
			}
			LogDebug("Retrieved %u events from PCR %u (external)", event_count, i);
		} else {
			/* A TCSD controlled PCR event list */
			event_count = get_num_events(i);

			if (event_count == 0)
				continue;

			if ((event_list = calloc(event_count, sizeof(TSS_PCR_EVENT))) == NULL) {
				LogError("malloc of %zd bytes failed",
					 event_count * sizeof(TSS_PCR_EVENT));
				result = TCSERR(TSS_E_OUTOFMEMORY);
				free(aggregate_list);
				goto error;
			}

			tmp = tcs_event_log->lists[i];
			for (j = 0; j < event_count; j++) {
				copy_pcr_event(&event_list[j], &(tmp->event));
				tmp = tmp->next;
			}
		}

		if (event_count == 0)
			continue;

		/* Tack the list onto the aggregate_list */
		aggregate_list = concat_pcr_events(&aggregate_list, aggregate_count, event_list,
						   event_count);
		if (aggregate_list == NULL) {
			free(event_list);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		aggregate_count += event_count;
		free(event_list);
	}

	*ppEvents = aggregate_list;
	*pEventCount = aggregate_count;
	result = TSS_SUCCESS;
error:
	MUTEX_UNLOCK(tcs_event_log->lock);

	return result;
}

