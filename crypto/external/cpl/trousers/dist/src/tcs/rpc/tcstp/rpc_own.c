
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
tcs_wrap_TakeOwnership(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT16 protocolID;
	UINT32 encOwnerAuthSize;
	BYTE *encOwnerAuth;
	UINT32 encSrkAuthSize;
	BYTE *encSrkAuth;
	UINT32 srkInfoSize;
	BYTE *srkInfo;
	TPM_AUTH ownerAuth;

	UINT32 srkKeySize;
	BYTE *srkKey;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT16, 1, &protocolID, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &encOwnerAuthSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	encOwnerAuth = calloc(1, encOwnerAuthSize);
	if (encOwnerAuth == NULL) {
		LogError("malloc of %d bytes failed.", encOwnerAuthSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, encOwnerAuth, encOwnerAuthSize, &data->comm)) {
		free(encOwnerAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &encSrkAuthSize, 0, &data->comm)) {
		free(encOwnerAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	encSrkAuth = calloc(1, encSrkAuthSize);
	if (encSrkAuth == NULL) {
		LogError("malloc of %d bytes failed.", encSrkAuthSize);
		free(encOwnerAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, encSrkAuth, encSrkAuthSize, &data->comm)) {
		free(encOwnerAuth);
		free(encSrkAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_UINT32, 6, &srkInfoSize, 0, &data->comm)) {
		free(encOwnerAuth);
		free(encSrkAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	srkInfo = calloc(1, srkInfoSize);
	if (srkInfo == NULL) {
		LogError("malloc of %d bytes failed.", srkInfoSize);
		free(encOwnerAuth);
		free(encSrkAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 7, srkInfo, srkInfoSize, &data->comm)) {
		free(encOwnerAuth);
		free(encSrkAuth);
		free(srkInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 8, &ownerAuth, 0, &data->comm)) {
		free(encOwnerAuth);
		free(encSrkAuth);
		free(srkInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_TakeOwnership_Internal(hContext, protocolID, encOwnerAuthSize, encOwnerAuth,
					     encSrkAuthSize, encSrkAuth, srkInfoSize, srkInfo,
					     &ownerAuth, &srkKeySize, &srkKey);

	MUTEX_UNLOCK(tcsp_lock);
	free(encOwnerAuth);
	free(encSrkAuth);
	free(srkInfo);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			free(srkKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &srkKeySize, 0, &data->comm)) {
			free(srkKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, srkKey, srkKeySize, &data->comm)) {
			free(srkKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(srkKey);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_OwnerClear(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TPM_AUTH auth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_AUTH, 1, &auth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_OwnerClear_Internal(hContext, &auth);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
