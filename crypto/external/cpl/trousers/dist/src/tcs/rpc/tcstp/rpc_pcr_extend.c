
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
tcs_wrap_Extend(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 pcrIndex;
	TCPA_DIGEST inDigest;
	TSS_RESULT result;
	TCPA_DIGEST outDigest;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &pcrIndex, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_DIGEST, 2, &inDigest, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Extend_Internal(hContext, pcrIndex, inDigest, &outDigest);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &outDigest, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_PcrRead(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 pcrIndex;
	TCPA_DIGEST digest;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &pcrIndex, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_PcrRead_Internal(hContext, pcrIndex, &digest);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &digest, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_PcrReset(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 pcrDataSizeIn;
	BYTE *pcrDataIn;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &pcrDataSizeIn, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	pcrDataIn = (BYTE *)malloc(pcrDataSizeIn);
	if (pcrDataIn == NULL) {
		LogError("malloc of %u bytes failed.", pcrDataSizeIn);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, pcrDataIn, pcrDataSizeIn, &data->comm)) {
		free(pcrDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_PcrReset_Internal(hContext, pcrDataSizeIn, pcrDataIn);

	MUTEX_UNLOCK(tcsp_lock);
	free(pcrDataIn);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return result;
}

