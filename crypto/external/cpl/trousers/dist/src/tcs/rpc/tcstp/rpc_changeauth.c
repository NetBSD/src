
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
tcs_wrap_ChangeAuth(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE parentHandle;
	TCPA_PROTOCOL_ID protocolID;
	TCPA_ENCAUTH newAuth;
	TCPA_ENTITY_TYPE entityType;
	UINT32 encDataSize;
	BYTE *encData;

	TPM_AUTH ownerAuth;
	TPM_AUTH entityAuth;

	UINT32 outDataSize;
	BYTE *outData;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT16, 2, &protocolID, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 3, &newAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT16, 4, &entityType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &encDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	encData = calloc(1, encDataSize);
	if (encData == NULL) {
		LogError("malloc of %d bytes failed.", encDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 6, encData, encDataSize, &data->comm)) {
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &ownerAuth, 0, &data->comm)) {
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 8, &entityAuth, 0, &data->comm)) {
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ChangeAuth_Internal(hContext, parentHandle, protocolID, newAuth, entityType,
					  encDataSize, encData, &ownerAuth, &entityAuth,
					  &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(encData);
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 4);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, 1, &entityAuth, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 2, &outDataSize, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 3, outData, outDataSize, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(outData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ChangeAuthOwner(struct tcsd_thread_data *data)
{

	TCS_CONTEXT_HANDLE hContext;
	TCPA_PROTOCOL_ID protocolID;
	TCPA_ENCAUTH newAuth;
	TCPA_ENTITY_TYPE entityType;

	TPM_AUTH ownerAuth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT16, 1, &protocolID, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 2, &newAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT16, 3, &entityType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ChangeAuthOwner_Internal(hContext, protocolID, newAuth, entityType,
					       &ownerAuth);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
