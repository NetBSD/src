
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

/*
 * biosem.c
 *
 * Routines for handling PCR events from the TCG Compliant BIOS
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"
#include "tcsem.h"

#ifdef EVLOG_SOURCE_BIOS

struct ext_log_source bios_source = {
	bios_open,
	bios_get_entries_by_pcr,
	bios_get_entry,
	bios_close
};

int
bios_open(void *source, FILE **handle)
{
	FILE *fd;

	if ((fd = fopen((char *)source, "r")) == NULL ) {
		LogError("Error opening BIOS Eventlog file %s: %s", (char *)source,
			 strerror(errno));
		return -1;
	}

	*handle = fd;

	return 0;
}

TSS_RESULT
bios_get_entries_by_pcr(FILE *handle, UINT32 pcr_index, UINT32 first,
			UINT32 *count, TSS_PCR_EVENT **events)
{
	char page[BIOS_READ_SIZE];
	int error_path = 1;
	UINT32 seen_indices = 0, copied_events = 0, i;
	struct event_wrapper *list = calloc(1, sizeof(struct event_wrapper));
	struct event_wrapper *cur = list;
	TSS_RESULT result = TSS_E_INTERNAL_ERROR;
	TCG_PCClientPCREventStruc *event = NULL;
	int num=0;

	if (list == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct event_wrapper));
		return TSS_E_OUTOFMEMORY;
	}

	if (*count == 0) {
		result = TSS_SUCCESS;
		goto free_list;
	}

	while (1) {
		/* read event header from the file */
		if ((fread(page, 32, 1, handle)) <= 0) {
			goto copy_events;
		}

		event = (TCG_PCClientPCREventStruc *)page;

		/* if the index is the one we're looking for, grab the entry */
		if (pcr_index == event->pcrIndex) {
			if (seen_indices >= first) {
				/* grab this entry */
				cur->event.rgbPcrValue = malloc(20);
				if (cur->event.rgbPcrValue == NULL) {
					LogError("malloc of %d bytes failed.", 20);
					result = TSS_E_OUTOFMEMORY;
					goto free_list;
				}

				cur->event.ulPcrIndex = pcr_index;
				cur->event.eventType = event->eventType;
				cur->event.ulPcrValueLength = 20;

				/* copy the SHA1 XXX endianess ignored */
				memcpy(cur->event.rgbPcrValue, event->digest, 20);

				/* copy the event name XXX endianess ignored */
				cur->event.ulEventLength = event->eventDataSize;

				if (event->eventDataSize>0) {
					cur->event.rgbEvent = malloc(event->eventDataSize);
					if (cur->event.rgbEvent == NULL) {
						LogError("malloc of %d bytes failed.",
							 event->eventDataSize);
						free(cur->event.rgbPcrValue);
						result = TSS_E_OUTOFMEMORY;
						goto free_list;
					}
					if ((fread(cur->event.rgbEvent,
						   event->eventDataSize, 1, handle)) <= 0) {
						LogError("read from event source failed: %s",
							 strerror(errno));
						return result;
					}
				} else {
					cur->event.rgbEvent = NULL;
				}

				copied_events++;
				if (copied_events == *count)
					goto copy_events;

				cur->next = calloc(1, sizeof(struct event_wrapper));
				if (cur->next == NULL) {
					LogError("malloc of %zd bytes failed.",
						 sizeof(struct event_wrapper));
					result = TSS_E_OUTOFMEMORY;
					goto free_list;
				}
				cur = cur->next;
			} else {
				/* skip */
				if (event->eventDataSize > 0)
					fseek(handle,event->eventDataSize,SEEK_CUR);
			}
			seen_indices++;
		} else {
			if (event->eventDataSize > 0)
				fseek(handle,event->eventDataSize,SEEK_CUR);
			}
		num++;
	}

copy_events:

	/* we've copied all the events we need to from this PCR, now
	 * copy them all into one contiguous memory block
	 */
	*events = calloc(copied_events, sizeof(TSS_PCR_EVENT));
	if (*events == NULL) {
		LogError("malloc of %zd bytes failed.", copied_events * sizeof(TSS_PCR_EVENT));
		result = TSS_E_OUTOFMEMORY;
		goto free_list;
	}

	cur = list;
	for (i = 0; i < copied_events; i++) {
		memcpy(&((*events)[i]), &(cur->event), sizeof(TSS_PCR_EVENT));
		cur = cur->next;
	}

	*count = copied_events;
	/* assume we're in an error path until we get here */
	error_path = 0;
	result = TSS_SUCCESS;

free_list:
	cur = list->next;
	while (cur != NULL) {
		if (error_path) {
			free(cur->event.rgbEvent);
			free(cur->event.rgbPcrValue);
		}
		free(list);
		list = cur;
		cur = list->next;
	}
	free(list);
	return result;
}

TSS_RESULT
bios_get_entry(FILE *handle, UINT32 pcr_index, UINT32 *num, TSS_PCR_EVENT **ppEvent)
{
	char page[BIOS_READ_SIZE];
	UINT32 seen_indices = 0;
	TSS_RESULT result = TSS_E_INTERNAL_ERROR;
	TSS_PCR_EVENT *e = NULL;
	TCG_PCClientPCREventStruc *event = NULL;

	while (1) {
		/* read event header from the file */
		if ((fread(page, 32, 1, handle)) == 0) {
			goto done;
		}

		event = (TCG_PCClientPCREventStruc *)page;

		if (pcr_index == event->pcrIndex) {
			if (ppEvent && !*ppEvent && seen_indices == *num) {
				*ppEvent = calloc(1, sizeof(TSS_PCR_EVENT));
				if (*ppEvent == NULL) {
					LogError("malloc of %zd bytes failed.",
						 sizeof(TSS_PCR_EVENT));
					return TSS_E_INTERNAL_ERROR;
				}

				e = *ppEvent;

				e->rgbPcrValue = malloc(20);
				if (e->rgbPcrValue == NULL) {
					LogError("malloc of %d bytes failed.", 20);
					free(e);
					e = NULL;
					break;
				}

				e->ulPcrIndex = pcr_index;
				e->eventType = event->eventType;
				e->ulPcrValueLength = 20;

				/* copy the SHA1 XXX endianess ignored */
				memcpy(e->rgbPcrValue, event->digest, 20);

				e->ulEventLength = event->eventDataSize;

				if (event->eventDataSize>0) {
					e->rgbEvent = malloc(e->ulEventLength);
					if (e->rgbEvent == NULL) {
						LogError("malloc of %d bytes failed.",
							 e->ulEventLength);
						free(e->rgbPcrValue);
						free(e);
						e = NULL;
						break;
					}
					if ((fread(e->rgbEvent,
						   event->eventDataSize,
						   1, handle)) <= 0) {
						LogError("read from event source failed: %s",
							 strerror(errno));
						return result;
					}
				} else {
					e->rgbEvent = NULL;
				}
				result = TSS_SUCCESS;

				break;
			} else {
				/* skip */
				if (event->eventDataSize > 0) {
					fseek(handle,event->eventDataSize,SEEK_CUR);
				}
			}
			seen_indices++;
		} else {
			/* skip */
			if (event->eventDataSize > 0) {
				fseek(handle,event->eventDataSize,SEEK_CUR);
			}
		}
	}

done:
	if (!ppEvent) {
		*num = seen_indices;
		result = TSS_SUCCESS;
	} else if (e == NULL)
		*ppEvent = NULL;

	return result;
}

int
bios_close(FILE *handle)
{
	fclose(handle);

	return 0;
}

#endif
