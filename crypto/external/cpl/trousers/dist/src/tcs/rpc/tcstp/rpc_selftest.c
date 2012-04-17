
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
tcs_wrap_SelfTestFull(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_SelfTestFull_Internal(hContext);

	MUTEX_UNLOCK(tcsp_lock);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CertifySelfTest(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	UINT32 sigSize;
	BYTE *sigData = NULL;
	TCS_KEY_HANDLE hKey;
	TCPA_NONCE antiReplay;
	TPM_AUTH privAuth;
	TPM_AUTH *pPrivAuth;
	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

        if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
                return TCSERR(TSS_E_INTERNAL_ERROR);
        if (getData(TCSD_PACKET_TYPE_NONCE, 2, &antiReplay, 0, &data->comm))
                return TCSERR(TSS_E_INTERNAL_ERROR);

        result = getData(TCSD_PACKET_TYPE_AUTH, 3, &privAuth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
                pPrivAuth = NULL;
	else if (result)
		return result;
        else
                pPrivAuth = &privAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CertifySelfTest_Internal(hContext, hKey, antiReplay, pPrivAuth, &sigSize,
					       &sigData);

	MUTEX_UNLOCK(tcsp_lock);

	i = 0;
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
                if (pPrivAuth != NULL) {
                        if (setData(TCSD_PACKET_TYPE_AUTH, i++, pPrivAuth, 0, &data->comm)) {
                                free(sigData);
                                return TCSERR(TSS_E_INTERNAL_ERROR);
                        }
                }

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &sigSize, 0, &data->comm)) {
			free(sigData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, sigData, sigSize, &data->comm)) {
			free(sigData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(sigData);
	} else
		initData(&data->comm, 0);


	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetTestResult(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	UINT32 resultDataSize;
	BYTE *resultData = NULL;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetTestResult_Internal(hContext, &resultDataSize, &resultData);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &resultDataSize, 0, &data->comm)) {
			free(resultData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, resultData, resultDataSize, &data->comm)) {
			free(resultData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(resultData);
	} else
		initData(&data->comm, 0);


	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
