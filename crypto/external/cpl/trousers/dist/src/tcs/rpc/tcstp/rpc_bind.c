
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
tcs_wrap_UnBind(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE keyHandle;
	UINT32 inDataSize;
	BYTE *inData;

	TPM_AUTH privAuth;
	TPM_AUTH *pPrivAuth;

	UINT32 outDataSize;
	BYTE *outData;
	TSS_RESULT result;

	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &keyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &inDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	inData = calloc(1, inDataSize);
	if (inData == NULL) {
		LogError("malloc of %u bytes failed.", inDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, inData, inDataSize, &data->comm)) {
		free(inData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = getData(TCSD_PACKET_TYPE_AUTH, 4, &privAuth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pPrivAuth = NULL;
	else if (result) {
		free(inData);
		return result;
	} else
		pPrivAuth = &privAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_UnBind_Internal(hContext, keyHandle, inDataSize, inData,
				 pPrivAuth, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(inData);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pPrivAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pPrivAuth, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outDataSize, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outData, outDataSize, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(outData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
