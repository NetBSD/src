
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
tcs_wrap_Sign(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	UINT32 areaToSignSize;
	BYTE *areaToSign;

	TPM_AUTH auth;
	TPM_AUTH *pAuth;

	UINT32 sigSize;
	BYTE *sig;
	TSS_RESULT result;

	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &areaToSignSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	areaToSign = calloc(1, areaToSignSize);
	if (areaToSign == NULL) {
		LogError("malloc of %d bytes failed.", areaToSignSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, areaToSign, areaToSignSize, &data->comm)) {
		free(areaToSign);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	result = getData(TCSD_PACKET_TYPE_AUTH, 4, &auth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pAuth = NULL;
	else if (result) {
		free(areaToSign);
		return result;
	} else
		pAuth = &auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Sign_Internal(hContext, hKey, areaToSignSize, areaToSign, pAuth, &sigSize,
				    &sig);

	MUTEX_UNLOCK(tcsp_lock);
	free(areaToSign);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, &auth, 0, &data->comm)) {
				free(sig);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &sigSize, 0, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, sig, sigSize, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(sig);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
