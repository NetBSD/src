
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#ifndef _TCSEM_H_
#define _TCSEM_H_

struct ext_log_source {
        int (*open)(void *, FILE **);
        TSS_RESULT (*get_entries_by_pcr)(FILE *, UINT32, UINT32, UINT32 *, TSS_PCR_EVENT **);
        TSS_RESULT (*get_entry)(FILE *, UINT32, UINT32 *, TSS_PCR_EVENT **);
        int (*close)(FILE *);
};

struct event_wrapper {
	TSS_PCR_EVENT event;
	struct event_wrapper *next;
};

struct event_log {
	MUTEX_DECLARE(lock);
	struct ext_log_source *firmware_source;
	struct ext_log_source *kernel_source;
	struct event_wrapper **lists;
};

/* include the compiled-in log sources and struct references here */
#include "imaem.h"
#include "biosem.h"

#ifdef EVLOG_SOURCE_IMA
#define EVLOG_IMA_SOURCE	&ima_source
#else
#define EVLOG_IMA_SOURCE	NULL
#endif

#ifdef EVLOG_SOURCE_BIOS
#define EVLOG_BIOS_SOURCE	&bios_source
#else
#define EVLOG_BIOS_SOURCE	NULL
#endif


TSS_RESULT event_log_init();
TSS_RESULT event_log_final();
TSS_RESULT copy_pcr_event(TSS_PCR_EVENT *, TSS_PCR_EVENT *);
TSS_RESULT event_log_add(TSS_PCR_EVENT *, UINT32 *);
TSS_PCR_EVENT *get_pcr_event(UINT32, UINT32);
UINT32 get_num_events(UINT32);
TSS_PCR_EVENT *concat_pcr_events(TSS_PCR_EVENT **, UINT32, TSS_PCR_EVENT *, UINT32);
UINT32 get_pcr_event_size(TSS_PCR_EVENT *);
void free_external_events(UINT32, TSS_PCR_EVENT *);

extern struct event_log *tcs_event_log;

#endif
