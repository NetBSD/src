
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
tcs_common_Seal(UINT32 sealOrdinal,
		struct tcsd_thread_data *data)
{
	TSS_RESULT result;
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE keyHandle;
	TCPA_ENCAUTH KeyUsageAuth;
	UINT32 PCRInfoSize, inDataSize;
	BYTE *PCRInfo = NULL, *inData = NULL;
	TPM_AUTH emptyAuth, pubAuth, *pAuth;
	UINT32 outDataSize;
	BYTE *outData;

	int i = 0;

	memset(&emptyAuth, 0, sizeof(TPM_AUTH));
	memset(&pubAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &keyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, i++, &KeyUsageAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &PCRInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (PCRInfoSize > 0) {
		PCRInfo = calloc(1, PCRInfoSize);
		if (PCRInfo == NULL) {
			LogError("malloc of %u bytes failed.", PCRInfoSize);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, PCRInfo, PCRInfoSize, &data->comm)) {
			free(PCRInfo);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &inDataSize, 0, &data->comm)) {
		free(PCRInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (inDataSize > 0) {
		inData = calloc(1, inDataSize);
		if (inData == NULL) {
			LogError("malloc of %u bytes failed.", inDataSize);
			free(PCRInfo);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, inData, inDataSize, &data->comm)) {
			free(inData);
			free(PCRInfo);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	result = getData(TCSD_PACKET_TYPE_AUTH, i++, &pubAuth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pAuth = NULL;
	else if (result) {
		free(inData);
		free(PCRInfo);
		return result;
	} else
		pAuth = &pubAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Seal_Internal(sealOrdinal, hContext, keyHandle, KeyUsageAuth, PCRInfoSize,
				    PCRInfo, inDataSize, inData, pAuth, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(inData);
	free(PCRInfo);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (pAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, pAuth, 0, &data->comm)) {
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &outDataSize, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, outData, outDataSize, &data->comm)) {
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
tcs_wrap_Seal(struct tcsd_thread_data *data)
{
	return tcs_common_Seal(TPM_ORD_Seal, data);
}

#ifdef TSS_BUILD_SEALX
TSS_RESULT
tcs_wrap_Sealx(struct tcsd_thread_data *data)
{
	return tcs_common_Seal(TPM_ORD_Sealx, data);
}
#endif

TSS_RESULT
tcs_wrap_UnSeal(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE parentHandle;
	UINT32 inDataSize;
	BYTE *inData;

	TPM_AUTH parentAuth, dataAuth, emptyAuth;
	TPM_AUTH *pParentAuth, *pDataAuth;

	UINT32 outDataSize;
	BYTE *outData;
	TSS_RESULT result;

	memset(&emptyAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &inDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	inData = calloc(1, inDataSize);
	if (inData == NULL) {
		LogError("malloc of %d bytes failed.", inDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, inData, inDataSize, &data->comm)) {
		free(inData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = getData(TCSD_PACKET_TYPE_AUTH, 4, &parentAuth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pParentAuth = NULL;
	else if (result) {
		free(inData);
		return result;
	} else
		pParentAuth = &parentAuth;

	result = getData(TCSD_PACKET_TYPE_AUTH, 5, &dataAuth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE) {
		pDataAuth = pParentAuth;
		pParentAuth = NULL;
	} else if (result) {
		free(inData);
		return result;
	} else
		pDataAuth = &dataAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Unseal_Internal(hContext, parentHandle, inDataSize, inData, pParentAuth,
				      pDataAuth, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(inData);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 4);
		if (pParentAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, pParentAuth, 0, &data->comm)) {
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		} else {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, &emptyAuth, 0, &data->comm)) {
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_AUTH, 1, &dataAuth, 0, &data->comm)) {
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
