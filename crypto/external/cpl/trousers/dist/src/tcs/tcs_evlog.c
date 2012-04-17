
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


struct event_log *tcs_event_log = NULL;

TSS_RESULT
event_log_init()
{
	if (tcs_event_log != NULL)
		return TSS_SUCCESS;

	tcs_event_log = calloc(1, sizeof(struct event_log));
	if (tcs_event_log == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct event_log));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	MUTEX_INIT(tcs_event_log->lock);

	/* allocate as many event lists as there are PCR's */
	tcs_event_log->lists = calloc(tpm_metrics.num_pcrs, sizeof(struct event_wrapper *));
	if (tcs_event_log->lists == NULL) {
		LogError("malloc of %zd bytes failed.",
				tpm_metrics.num_pcrs * sizeof(struct event_wrapper *));
		free(tcs_event_log);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	/* assign external event log sources here */
	//tcs_event_log->firmware_source = EVLOG_IMA_SOURCE;
	tcs_event_log->firmware_source = EVLOG_BIOS_SOURCE;
	tcs_event_log->kernel_source = EVLOG_IMA_SOURCE;

	return TSS_SUCCESS;
}

TSS_RESULT
event_log_final()
{
	struct event_wrapper *cur, *next;
	UINT32 i;

	MUTEX_LOCK(tcs_event_log->lock);

	for (i = 0; i < tpm_metrics.num_pcrs; i++) {
		cur = tcs_event_log->lists[i];
		while (cur != NULL) {
			next = cur->next;
			free(cur->event.rgbPcrValue);
			free(cur->event.rgbEvent);
			free(cur);
			cur = next;
		}
	}

	MUTEX_UNLOCK(tcs_event_log->lock);

	free(tcs_event_log->lists);
	free(tcs_event_log);

	return TSS_SUCCESS;
}

TSS_RESULT
copy_pcr_event(TSS_PCR_EVENT *dest, TSS_PCR_EVENT *source)
{
	memcpy(dest, source, sizeof(TSS_PCR_EVENT));
	return TSS_SUCCESS;
}

TSS_RESULT
event_log_add(TSS_PCR_EVENT *event, UINT32 *pNumber)
{
	struct event_wrapper *new, *tmp;
	TSS_RESULT result;
	UINT32 i;

	MUTEX_LOCK(tcs_event_log->lock);

	new = calloc(1, sizeof(struct event_wrapper));
	if (new == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct event_wrapper));
		MUTEX_UNLOCK(tcs_event_log->lock);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if ((result = copy_pcr_event(&(new->event), event))) {
		free(new);
		MUTEX_UNLOCK(tcs_event_log->lock);
		return result;
	}

	/* go to the end of the list to add the element, so that they're in order */
	i = 0;
	if (tcs_event_log->lists[event->ulPcrIndex] == NULL) {
		tcs_event_log->lists[event->ulPcrIndex] = new;
	} else {
		i++;
		tmp = tcs_event_log->lists[event->ulPcrIndex];
		while (tmp->next != NULL) {
			i++;
			tmp = tmp->next;
		}
		tmp->next = new;
	}

	*pNumber = ++i;

	MUTEX_UNLOCK(tcs_event_log->lock);

	return TSS_SUCCESS;
}

TSS_PCR_EVENT *
get_pcr_event(UINT32 pcrIndex, UINT32 eventNumber)
{
	struct event_wrapper *tmp;
	UINT32 counter = 0;

	MUTEX_LOCK(tcs_event_log->lock);

	tmp = tcs_event_log->lists[pcrIndex];
	for (; tmp; tmp = tmp->next) {
		if (counter == eventNumber) {
			break;
		}
		counter++;
	}

	MUTEX_UNLOCK(tcs_event_log->lock);

	return (tmp ? &(tmp->event) : NULL);
}

/* the lock should be held before calling this function */
UINT32
get_num_events(UINT32 pcrIndex)
{
	struct event_wrapper *tmp;
	UINT32 counter = 0;

	tmp = tcs_event_log->lists[pcrIndex];
	for (; tmp; tmp = tmp->next) {
		counter++;
	}

	return counter;
}

TSS_PCR_EVENT *
concat_pcr_events(TSS_PCR_EVENT **list_so_far, UINT32 list_size, TSS_PCR_EVENT *addition,
		  UINT32 addition_size)
{
	TSS_PCR_EVENT *ret;

	ret = realloc(*list_so_far, (list_size + addition_size) * sizeof(TSS_PCR_EVENT));
	if (ret == NULL) {
		LogError("malloc of %zd bytes failed",
			 (list_size + addition_size) * sizeof(TSS_PCR_EVENT));
		return ret;
	}

	memcpy(&ret[list_size], addition, addition_size * sizeof(TSS_PCR_EVENT));
	return ret;
}

/* XXX make this a macro */
UINT32
get_pcr_event_size(TSS_PCR_EVENT *e)
{
	return (sizeof(TSS_PCR_EVENT) + e->ulEventLength + e->ulPcrValueLength);
}

void
free_external_events(UINT32 eventCount, TSS_PCR_EVENT *ppEvents)
{
	UINT32 j;

	if (!ppEvents)
		return;

	for (j = 0; j < eventCount; j++) {
		/* This is a fairly heinous hack, but PCR event logs can get really large
		 * and without it, there is a real potential to exhaust memory by leaks.
		 * The PCR event logs that we pull out of securityfs have had their
		 * rgbPcrValue and rgbEvent pointers malloc'd dynamically as the
		 * securityfs log was parsed. The other event log lists that are
		 * maintained by the TCSD don't need to have this data free'd, since that
		 * will happen at shutdown time only. So, for each PCR index that's
		 * read from securityfs, we need to free its pointers after that data has
		 * been set in the packet to send back to the TSP. */
		if ((tcsd_options.kernel_pcrs & (1 << ppEvents[j].ulPcrIndex)) ||
		    (tcsd_options.firmware_pcrs & (1 << ppEvents[j].ulPcrIndex))) {
			free(ppEvents[j].rgbPcrValue);
			free(ppEvents[j].rgbEvent);
		}
	}
}
