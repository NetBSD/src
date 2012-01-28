
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef _TCS_CONTEXT_H_
#define _TCS_CONTEXT_H_

#include "threads.h"

struct keys_loaded
{
	TCS_KEY_HANDLE key_handle;
	struct keys_loaded *next;
};

#define TSS_CONTEXT_FLAG_TRANSPORT_EXCLUSIVE	0x1
#define TSS_CONTEXT_FLAG_TRANSPORT_ENCRYPTED	0x2
#define TSS_CONTEXT_FLAG_TRANSPORT_ENABLED	0x4

struct tcs_context {
	TSS_FLAG flags;
	TPM_TRANSHANDLE transHandle;
	TCS_CONTEXT_HANDLE handle;
	COND_VAR cond; /* used in waiting for an auth ctx to become available */
	struct keys_loaded *keys;
	struct tcs_context *next;
};

#endif

