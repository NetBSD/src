
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
tcs_wrap_DaaJoin(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_HANDLE hDAA;
	BYTE stage;
	UINT32 inputSize0, inputSize1, outputSize, i;
	BYTE *inputData0 = NULL, *inputData1 = NULL,*outputData;
	TSS_RESULT result;
	TPM_AUTH ownerAuth, *pOwnerAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hDAA, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("thread %ld hDAA %x", THREAD_ID, hDAA);

	if (getData(TCSD_PACKET_TYPE_BYTE, 2, &stage, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebug("getData 2 (stage=%d)", (int)stage);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &inputSize0, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebug("getData 3  inputSize0=%d", inputSize0);

	inputData0 = calloc(1, inputSize0);
	if (inputData0 == NULL) {
		LogError("malloc of %d bytes failed.", inputSize0);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	LogDebug("getData 4 inputData0");
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, inputData0, inputSize0, &data->comm)) {
		free(inputData0);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	LogDebug("getData 5");
	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &inputSize1, 0, &data->comm)) {
		free( inputData0);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	LogDebug("getData 5  inputSize1=%d", inputSize1);

	if( inputSize1 > 0) {
		inputData1 = calloc(1, inputSize1);
		if (inputData1 == NULL) {
			LogError("malloc of %d bytes failed.", inputSize1);
			free( inputData0);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		LogDebug("getData 6 inputData1");
		if (getData(TCSD_PACKET_TYPE_PBYTE, 6, inputData1, inputSize1, &data->comm)) {
			free(inputData0);
			free(inputData1);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	LogDebug("getData 7");
	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &ownerAuth, 0, &data->comm)) {
		pOwnerAuth = NULL;
	} else {
		pOwnerAuth = &ownerAuth;
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_DaaJoin_internal(hContext, hDAA, stage, inputSize0, inputData0, inputSize1,
				       inputData1, pOwnerAuth, &outputSize, &outputData);

	MUTEX_UNLOCK(tcsp_lock);

	free(inputData0);
	if( inputData1 != NULL) free(inputData1);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if ( pOwnerAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &data->comm)) {
				free(outputData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outputSize, 0, &data->comm)) {
			free(outputData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outputData, outputSize, &data->comm)) {
			free(outputData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(outputData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_DaaSign(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_HANDLE hDAA;
	BYTE stage;
	UINT32 inputSize0, inputSize1, outputSize, i;
	BYTE *inputData0 = NULL, *inputData1 = NULL,*outputData;
	TSS_RESULT result;
	TPM_AUTH ownerAuth, *pOwnerAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hDAA, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("thread %ld hDAA %x", THREAD_ID, hDAA);

	if (getData(TCSD_PACKET_TYPE_BYTE, 2, &stage, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("getData 2 (stage=%d)", (int)stage);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &inputSize0, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebug("getData 3  inputSize0=%d", inputSize0);

	inputData0 = calloc(1, inputSize0);
	if (inputData0 == NULL) {
		LogError("malloc of %d bytes failed.", inputSize0);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	LogDebug("getData 4 inputData0");
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, inputData0, inputSize0, &data->comm)) {
		free(inputData0);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	LogDebug("getData 5");
	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &inputSize1, 0, &data->comm)) {
		free( inputData0);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	LogDebug("getData 5  inputSize1=%d", inputSize1);

	if( inputSize1 > 0) {
		inputData1 = calloc(1, inputSize1);
		if (inputData1 == NULL) {
			LogError("malloc of %d bytes failed.", inputSize1);
			free( inputData0);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		LogDebug("getData 6 inputData1");
		if (getData(TCSD_PACKET_TYPE_PBYTE, 6, inputData1, inputSize1, &data->comm)) {
			free(inputData0);
			free(inputData1);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	LogDebug("getData 7");
	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &ownerAuth, 0, &data->comm)) {
		pOwnerAuth = NULL;
	} else {
		pOwnerAuth = &ownerAuth;
	}

	LogDebugFn("-> TCSP_DaaSign_internal");

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_DaaSign_internal(hContext, hDAA, stage, inputSize0, inputData0, inputSize1,
				       inputData1, pOwnerAuth, &outputSize, &outputData);

	MUTEX_UNLOCK(tcsp_lock);

	LogDebugFn("<- TCSP_DaaSign_internal");

	free(inputData0);
	if( inputData1 != NULL) free(inputData1);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if ( pOwnerAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &data->comm)) {
				free(outputData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outputSize, 0, &data->comm)) {
			free(outputData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outputData, outputSize, &data->comm)) {
			free(outputData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(outputData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
