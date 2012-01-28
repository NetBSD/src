
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
tcs_wrap_OpenContext(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	UINT32 tpm_version = tpm_metrics.version.minor;

	LogDebugFn("thread %ld", THREAD_ID);

	result = TCS_OpenContext_Internal(&hContext);
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);

		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &tpm_version, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);

		/* Set the context in the thread's object. Later, if something goes wrong
		 * and the connection can't be closed cleanly, we'll still have a reference
		 * to what resources need to be freed. */
		data->context = hContext;
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CloseContext(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	result = TCS_CloseContext_Internal(hContext);

	/* This will signal the thread that the connection has been closed cleanly */
	if (result == TSS_SUCCESS)
		data->context = NULL_TCS_HANDLE;

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
