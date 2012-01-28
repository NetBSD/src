
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
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
tcs_wrap_ReadCounter(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_COUNTER_ID idCounter;
	TPM_COUNTER_VALUE counterValue;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &idCounter, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReadCounter_Internal(hContext, idCounter, &counterValue);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_COUNTER_VALUE, 0, &counterValue, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CreateCounter(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_COUNTER_ID idCounter;
	TPM_COUNTER_VALUE counterValue;
	TPM_AUTH auth;
	TPM_ENCAUTH encauth;
	UINT32 LabelSize;
	BYTE *pLabel = NULL;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &LabelSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if ((pLabel = calloc(1, LabelSize)) == NULL) {
		LogError("malloc of %u bytes failed.", LabelSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, &pLabel, LabelSize, &data->comm)) {
		free(pLabel);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 3, &encauth, 0, &data->comm)) {
		free(pLabel);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &auth, 0, &data->comm)) {
		free(pLabel);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateCounter_Internal(hContext, LabelSize, pLabel, encauth, &auth,
					     &idCounter, &counterValue);

	MUTEX_UNLOCK(tcsp_lock);

	free(pLabel);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &idCounter, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_COUNTER_VALUE, 2, &counterValue, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_IncrementCounter(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_COUNTER_ID idCounter;
	TPM_COUNTER_VALUE counterValue;
	TPM_AUTH auth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &idCounter, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &auth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_IncrementCounter_Internal(hContext, idCounter, &auth, &counterValue);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_COUNTER_VALUE, 1, &counterValue, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ReleaseCounter(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_COUNTER_ID idCounter;
	TPM_AUTH auth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &idCounter, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &auth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReleaseCounter_Internal(hContext, idCounter, &auth);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ReleaseCounterOwner(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_COUNTER_ID idCounter;
	TPM_AUTH auth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &idCounter, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &auth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReleaseCounterOwner_Internal(hContext, idCounter, &auth);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
