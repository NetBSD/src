
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
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
tcs_wrap_Delegate_Manage(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_FAMILY_ID familyId;
	TPM_FAMILY_OPERATION opFlag;
	UINT32 opDataSize;
	BYTE *opData;
	TPM_AUTH ownerAuth, nullAuth, *pAuth;
	UINT32 retDataSize;
	BYTE *retData;
	TSS_RESULT result;
	int i;

	memset(&ownerAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &familyId, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &opFlag, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &opDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	opData = malloc(opDataSize);
	if (opData == NULL) {
		LogError("malloc of %u bytes failed.", opDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, opData, opDataSize, &data->comm)) {
		free(opData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &ownerAuth, 0, &data->comm)) {
		free(opData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &ownerAuth, sizeof(TPM_AUTH)))
		pAuth = &ownerAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_Manage_Internal(hContext, familyId, opFlag,
			opDataSize, opData, pAuth, &retDataSize, &retData);

	MUTEX_UNLOCK(tcsp_lock);
	free(opData);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(retData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &retDataSize, 0, &data->comm)) {
			free(retData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, retData, retDataSize, &data->comm)) {
			free(retData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(retData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_CreateKeyDelegation(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	UINT32 publicInfoSize;
	BYTE *publicInfo;
	TPM_ENCAUTH encDelAuth;
	TPM_AUTH keyAuth, nullAuth, *pAuth;
	UINT32 blobSize;
	BYTE *blob;
	TSS_RESULT result;
	int i;

	memset(&keyAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &publicInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	publicInfo = malloc(publicInfoSize);
	if (publicInfo == NULL) {
		LogError("malloc of %u bytes failed.", publicInfoSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, publicInfo, publicInfoSize, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 4, &encDelAuth, 0, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &keyAuth, 0, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &keyAuth, sizeof(TPM_AUTH)))
		pAuth = &keyAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_CreateKeyDelegation_Internal(hContext, hKey,
			publicInfoSize, publicInfo, &encDelAuth, pAuth, &blobSize, &blob);

	MUTEX_UNLOCK(tcsp_lock);
	free(publicInfo);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(blob);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &blobSize, 0, &data->comm)) {
			free(blob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, blob, blobSize, &data->comm)) {
			free(blob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(blob);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_CreateOwnerDelegation(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_BOOL increment;
	UINT32 publicInfoSize;
	BYTE *publicInfo;
	TPM_ENCAUTH encDelAuth;
	TPM_AUTH ownerAuth, nullAuth, *pAuth;
	UINT32 blobSize;
	BYTE *blob;
	TSS_RESULT result;
	int i;

	memset(&ownerAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_BOOL, 1, &increment, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &publicInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	publicInfo = malloc(publicInfoSize);
	if (publicInfo == NULL) {
		LogError("malloc of %u bytes failed.", publicInfoSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, publicInfo, publicInfoSize, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 4, &encDelAuth, 0, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &ownerAuth, 0, &data->comm)) {
		free(publicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &ownerAuth, sizeof(TPM_AUTH)))
		pAuth = &ownerAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_CreateOwnerDelegation_Internal(hContext, increment,
			publicInfoSize, publicInfo, &encDelAuth, pAuth, &blobSize, &blob);

	MUTEX_UNLOCK(tcsp_lock);
	free(publicInfo);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(blob);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &blobSize, 0, &data->comm)) {
			free(blob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, blob, blobSize, &data->comm)) {
			free(blob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(blob);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_LoadOwnerDelegation(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_DELEGATE_INDEX index;
	UINT32 blobSize;
	BYTE *blob;
	TPM_AUTH ownerAuth, nullAuth, *pAuth;
	TSS_RESULT result;

	memset(&ownerAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &index, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &blobSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	blob = malloc(blobSize);
	if (blob == NULL) {
		LogError("malloc of %u bytes failed.", blobSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, blob, blobSize, &data->comm)) {
		free(blob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &ownerAuth, 0, &data->comm)) {
		free(blob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &ownerAuth, sizeof(TPM_AUTH)))
		pAuth = &ownerAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_LoadOwnerDelegation_Internal(hContext, index, blobSize, blob,
			pAuth);

	MUTEX_UNLOCK(tcsp_lock);
	free(blob);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, pAuth, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_ReadTable(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 familyTableSize;
	BYTE *familyTable;
	UINT32 delegateTableSize;
	BYTE *delegateTable;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_ReadTable_Internal(hContext, &familyTableSize, &familyTable,
			&delegateTableSize, &delegateTable);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 4);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &familyTableSize, 0, &data->comm)) {
			free(familyTable);
			free(delegateTable);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, familyTable, familyTableSize, &data->comm)) {
			free(familyTable);
			free(delegateTable);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(familyTable);

		if (setData(TCSD_PACKET_TYPE_UINT32, 2, &delegateTableSize, 0, &data->comm)) {
			free(delegateTable);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 3, delegateTable, delegateTableSize, &data->comm)) {
			free(delegateTable);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(delegateTable);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_UpdateVerificationCount(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 inputSize;
	BYTE *input;
	TPM_AUTH ownerAuth, nullAuth, *pAuth;
	UINT32 outputSize;
	BYTE *output;
	TSS_RESULT result;
	int i;

	memset(&ownerAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &inputSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	input = malloc(inputSize);
	if (input == NULL) {
		LogError("malloc of %u bytes failed.", inputSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, input, inputSize, &data->comm)) {
		free(input);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 3, &ownerAuth, 0, &data->comm)) {
		free(input);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &ownerAuth, sizeof(TPM_AUTH)))
		pAuth = &ownerAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_UpdateVerificationCount_Internal(hContext, inputSize, input,
			pAuth, &outputSize, &output);

	MUTEX_UNLOCK(tcsp_lock);
	free(input);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(output);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outputSize, 0, &data->comm)) {
			free(output);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, output, outputSize, &data->comm)) {
			free(output);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(output);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_Delegate_VerifyDelegation(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 delegateSize;
	BYTE *delegate;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &delegateSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	delegate = malloc(delegateSize);
	if (delegate == NULL) {
		LogError("malloc of %u bytes failed.", delegateSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, delegate, delegateSize, &data->comm)) {
		free(delegate);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_Delegate_VerifyDelegation_Internal(hContext, delegateSize, delegate);

	MUTEX_UNLOCK(tcsp_lock);
	free(delegate);

	initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_DSAP(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT16 entityType;
	TCS_KEY_HANDLE keyHandle;
	TPM_NONCE nonceOddDSAP, nonceEven, nonceEvenDSAP;
	UINT32 entityValueSize;
	BYTE *entityValue;
	TCS_AUTHHANDLE authHandle;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT16, 1, &entityType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &keyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_NONCE, 3, &nonceOddDSAP, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &entityValueSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	entityValue = malloc(entityValueSize);
	if (entityValue == NULL) {
		LogError("malloc of %u bytes failed.", entityValueSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, entityValue, entityValueSize, &data->comm)) {
		free(entityValue);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_DSAP_Internal(hContext, entityType, keyHandle, &nonceOddDSAP, entityValueSize,
				    entityValue, &authHandle, &nonceEven, &nonceEvenDSAP);

	MUTEX_UNLOCK(tcsp_lock);
	free(entityValue);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);

		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &authHandle, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_NONCE, 1, &nonceEven, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_NONCE, 2, &nonceEvenDSAP, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

