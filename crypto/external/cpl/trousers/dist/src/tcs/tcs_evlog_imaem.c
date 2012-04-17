
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

/*
 * imaem.c
 *
 * Routines for handling PCR events from the Integrity Measurement
 * Architecture.
 *
 * The external event source format used by IMA:
 *
 *     4 bytes PCR Index (bin)
 *    20 bytes SHA1 template (bin)
 *     4 bytes template name_len
 * 1-255 bytes template name
 *    20 bytes SHA1 IMA(bin)
 *     4 bytes IMA name len
 * 1-255 bytes eventname
 *     1 byte  separator = '\0'
 *
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

#ifdef EVLOG_SOURCE_IMA

struct ext_log_source ima_source = {
	ima_open,
	ima_get_entries_by_pcr,
	ima_get_entry,
	ima_close
};

int
ima_open(void *source, FILE **handle)
{
 	FILE *fd;

	if ((fd = fopen((char *)source, "r")) == NULL) {
		LogError("Error opening PCR log file %s: %s",
			(char *)source, strerror(errno));
		return -1;
	}

	*handle =  fd;
	return 0;
}

TSS_RESULT
ima_get_entries_by_pcr(FILE *handle, UINT32 pcr_index, UINT32 first,
			UINT32 *count, TSS_PCR_EVENT **events)
{
	int pcr_value;
	char page[IMA_READ_SIZE];
	int error_path = 1, ptr = 0;
	UINT32 copied_events = 0, i;
	struct event_wrapper *list = calloc(1, sizeof(struct event_wrapper));
	struct event_wrapper *cur = list;
	TSS_RESULT result = TCSERR(TSS_E_INTERNAL_ERROR);
	FILE *fp = (FILE *) handle;
	uint len;
	char name[255];

	if (list == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct event_wrapper));
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (*count == 0) {
		result = TSS_SUCCESS;
		goto free_list;
	}

	if (!fp) {
		perror("unable to open file\n");
		return 1;
	}
	rewind(fp);

        while (fread(page, 24, 1, fp)) {
		/* copy the initial 4 bytes (PCR index) XXX endianess ignored */
		ptr = 0;
		memcpy(&pcr_value, &page[ptr], sizeof(int));
		cur->event.ulPcrIndex = pcr_value;
		ptr += sizeof(int);

		/* grab this entry */
		cur->event.ulPcrValueLength = 20;
		cur->event.rgbPcrValue = malloc(cur->event.ulPcrValueLength);
		if (cur->event.rgbPcrValue == NULL) {
			LogError("malloc of %d bytes failed.", 20);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto free_list;
		}

		/* copy the template SHA1 XXX endianess ignored */
		memcpy(cur->event.rgbPcrValue, &page[ptr],
		       cur->event.ulPcrValueLength);

/* Get the template name size, template name */
{
		char digest[20];

		if (fread(&len, 1, sizeof(len), fp) != (sizeof(len))) {
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto free_list;
		}
		
		memset(name, 0, sizeof name);
		if (fread(name, 1, len, fp) != len) {
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto free_list;
		}
		if (fread(digest, 1, sizeof digest, fp) != (sizeof(digest))) {
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto free_list;
		}
}
		/* Get the template data namelen and data */
		if (fread(&cur->event.ulEventLength, 1, sizeof(int), fp) != sizeof(int)) {
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto free_list;
		}
		cur->event.rgbEvent = malloc(cur->event.ulEventLength + 1);
		if (cur->event.rgbEvent == NULL) {
			free(cur->event.rgbPcrValue);
			LogError("malloc of %u bytes failed.",
				 cur->event.ulEventLength);
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto free_list;
		}
		memset(cur->event.rgbEvent, 0, cur->event.ulEventLength);
		if (fread(cur->event.rgbEvent, 1, cur->event.ulEventLength, fp) != cur->event.ulEventLength) {
			free(cur->event.rgbPcrValue);
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto free_list;
		}

		copied_events++;
printf("%d %s ", copied_events, name);

printf("%s\n", cur->event.rgbEvent);
		if (copied_events == *count)
			goto copy_events;

		cur->next = calloc(1, sizeof(struct event_wrapper));
		if (cur->next == NULL) {
			LogError("malloc of %zd bytes failed.",
				 sizeof(struct event_wrapper));
			result = TCSERR(TSS_E_OUTOFMEMORY);
			goto free_list;
		}
		cur = cur->next;
	}

copy_events:
	/* we've copied all the events we need to from this PCR, now
	 * copy them all into one contiguous memory block
	 */
printf("copied_events: %d\n", copied_events);
	*events = calloc(copied_events, sizeof(TSS_PCR_EVENT));
	if (*events == NULL) {
		LogError("malloc of %zd bytes failed.", copied_events * sizeof(TSS_PCR_EVENT));
		result = TCSERR(TSS_E_OUTOFMEMORY);
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
ima_get_entry(FILE *handle, UINT32 pcr_index, UINT32 *num, TSS_PCR_EVENT **ppEvent)
{
	int pcr_value, ptr = 0;
	uint len;
	char page[IMA_READ_SIZE];
	UINT32 seen_indices = 0;
	TSS_RESULT result = TCSERR(TSS_E_INTERNAL_ERROR);
	TSS_PCR_EVENT *event = NULL;
	FILE *fp = (FILE *) handle;
	char name[255];

	rewind(fp);
	while (fread(page, 24, 1, fp)) {
		/* copy the initial 4 bytes (PCR index) XXX endianess ignored */
		ptr = 0;
		memcpy(&pcr_value, &page[ptr], sizeof(int));

		if (pcr_index == (UINT32)pcr_value) {
			event = calloc(1, sizeof(TSS_PCR_EVENT));
			event->ulPcrIndex = pcr_value;
			ptr += sizeof(int);
			/* This is the case where we're looking for a specific event number in a
			 * specific PCR index. When we've reached the correct event, malloc
			 * space for it, copy it in, then break out of the while loop */
			if (ppEvent && seen_indices == *num) {
				/* grab this entry */
				event->ulPcrValueLength = 20;
				event->rgbPcrValue = malloc(event->ulPcrValueLength);
				if (event->rgbPcrValue == NULL) {
					LogError("malloc of %d bytes failed.", 20);
					free(event);
					result = TCSERR(TSS_E_OUTOFMEMORY);
					goto done;
				}

				/* copy the template SHA1 XXX endianess ignored */
				memcpy(event->rgbPcrValue, &page[ptr],
						event->ulPcrValueLength);

				/* Get the template name size, template name */
				{
					char digest[20];

					if (fread(&len, 1, sizeof(len), fp) != sizeof(len)) {
						free(event);
						LogError("Failed to read event log file");
						result = TCSERR(TSS_E_INTERNAL_ERROR);
						goto done;
					}
					memset(name, 0, sizeof name);
					if (fread(name, 1, len, fp) != len) {
						free(event);
						LogError("Failed to read event log file");
						result = TCSERR(TSS_E_INTERNAL_ERROR);
						goto done;
					}
					if (fread(digest, 1, sizeof(digest), fp) != sizeof(digest)) {
						free(event);
						LogError("Failed to read event log file");
						result = TCSERR(TSS_E_INTERNAL_ERROR);
						goto done;
					}
				}
				/* Get the template data namelen and data */
				if (fread(&event->ulEventLength, 1, sizeof(int), fp) != sizeof(int)) {
					free(event);
					LogError("Failed to read event log file");
					result = TCSERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
				event->rgbEvent = malloc(event->ulEventLength + 1);
				if (event->rgbEvent == NULL) {
					free(event->rgbPcrValue);
					free(event);
					LogError("malloc of %u bytes failed.",
							event->ulEventLength);
					result = TCSERR(TSS_E_OUTOFMEMORY);
					goto done;
				}
				memset(event->rgbEvent, 0, event->ulEventLength);
				if (fread(event->rgbEvent, 1, event->ulEventLength, fp) != event->ulEventLength ) {
					free(event->rgbPcrValue);
					free(event);
					LogError("Failed to read event log file");
					result = TCSERR(TSS_E_INTERNAL_ERROR);
					goto done;
				}
				
				*ppEvent = event;
				result = TSS_SUCCESS;
				break;
			}
		}
		if (fread(&len, 1, sizeof(len), fp) != sizeof(len)) {
			free(event->rgbPcrValue);
			free(event);
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		fseek(fp, len + 20, SEEK_CUR);
		if (fread(&len, 1, sizeof(len), fp) != sizeof(len)) {
			free(event->rgbPcrValue);
			free(event);
			LogError("Failed to read event log file");
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		fseek(fp, len, SEEK_CUR);
		seen_indices++;
		printf("%d - index\n", seen_indices);
	}
done:
	if (ppEvent == NULL)
		*num = seen_indices;

	return result;
}

int
ima_close(FILE *handle)
{
	fclose((FILE *)handle);

	return 0;
}
#endif
