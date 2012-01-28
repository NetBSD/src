
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef _IMAEM_H_
#define _IMAEM_H_

int ima_open(void *, FILE **);
TSS_RESULT ima_get_entries_by_pcr(FILE *, UINT32, UINT32, UINT32 *, TSS_PCR_EVENT **);
TSS_RESULT ima_get_entry(FILE *, UINT32, UINT32 *, TSS_PCR_EVENT **);
int ima_close(FILE *);

extern struct ext_log_source ima_source;

/*  4  bytes binary         [PCR  value]
 * 20  bytes binary         [SHA1 value]
 *  4  bytes binary         [event type]
 * 255 bytes of ascii (MAX) [event name]
 * 1   byte -> '\0'         [separator ]
 */
#define IMA_MIN_EVENT_SIZE 29
#define IMA_MAX_EVENT_SIZE 284

/* this should be large if we're reading out of /proc */
#define IMA_READ_SIZE	(4096 + IMA_MAX_EVENT_SIZE)

#define EVLOG_SOURCE_IMA	1

#endif
