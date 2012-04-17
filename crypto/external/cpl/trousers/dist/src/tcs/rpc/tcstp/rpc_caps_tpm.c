
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
tcs_wrap_GetCapability(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_CAPABILITY_AREA capArea;
	UINT32 subCapSize;
	BYTE *subCap;
	UINT32 respSize;
	BYTE *resp;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ldd context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &capArea, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &subCapSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (subCapSize == 0)
		subCap = NULL;
	else {
		subCap = calloc(1, subCapSize);
		if (subCap == NULL) {
			LogError("malloc of %u bytes failed.", subCapSize);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 3, subCap, subCapSize, &data->comm)) {
			free(subCap);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetCapability_Internal(hContext, capArea, subCapSize, subCap, &respSize,
					     &resp);

	MUTEX_UNLOCK(tcsp_lock);
	free(subCap);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &respSize, 0, &data->comm)) {
			free(resp);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, resp, respSize, &data->comm)) {
			free(resp);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(resp);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetCapabilityOwner(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_AUTH ownerAuth;
	TCPA_VERSION version;
	UINT32 nonVol;
	UINT32 vol;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_AUTH, 1, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetCapabilityOwner_Internal(hContext, &ownerAuth, &version, &nonVol, &vol);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 4);
		if (setData(TCSD_PACKET_TYPE_VERSION, 0, &version, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &nonVol, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 2, &vol, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, 3, &ownerAuth, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_SetCapability(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_CAPABILITY_AREA capArea;
	UINT32 subCapSize;
	BYTE *subCap;
	UINT32 valueSize;
	BYTE *value;
	TSS_RESULT result;
	TPM_AUTH ownerAuth, *pOwnerAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &capArea, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &subCapSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (subCapSize == 0)
		subCap = NULL;
	else {
		subCap = calloc(1, subCapSize);
		if (subCap == NULL) {
			LogError("malloc of %u bytes failed.", subCapSize);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 3, subCap, subCapSize, &data->comm)) {
			free(subCap);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &valueSize, 0, &data->comm)) {
		free(subCap);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (valueSize == 0)
		value = NULL;
	else {
		value = calloc(1, valueSize);
		if (value == NULL) {
			free(subCap);
			LogError("malloc of %u bytes failed.", valueSize);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 5, value, valueSize, &data->comm)) {
			free(subCap);
			free(value);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 6, &ownerAuth, 0, &data->comm))
		pOwnerAuth = NULL;
	else
		pOwnerAuth = &ownerAuth;


	MUTEX_LOCK(tcsp_lock);

	result = TCSP_SetCapability_Internal(hContext, capArea, subCapSize, subCap, valueSize,
					     value, pOwnerAuth);

	MUTEX_UNLOCK(tcsp_lock);
	free(subCap);
	free(value);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (pOwnerAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, pOwnerAuth, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
