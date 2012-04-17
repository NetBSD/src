
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef _BIOSEM_H_
#define _BIOSEM_H_

int bios_open(void *, FILE **);
TSS_RESULT bios_get_entries_by_pcr(FILE *, UINT32, UINT32, UINT32 *, TSS_PCR_EVENT **);
TSS_RESULT bios_get_entry(FILE *, UINT32, UINT32 *, TSS_PCR_EVENT **);
int bios_close(FILE *);

extern struct ext_log_source bios_source;

/* this should be large if we're reading out of /proc */
#define BIOS_READ_SIZE	4096

typedef struct {
	UINT32 pcrIndex;
	UINT32 eventType;
	BYTE   digest[20];
	UINT32 eventDataSize;
	BYTE   event[0];/* (eventSize) bytes of event data follows */
} TCG_PCClientPCREventStruc;

#define EVLOG_SOURCE_BIOS	1

#endif
