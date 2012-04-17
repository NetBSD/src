
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
tcs_wrap_CreateEndorsementKeyPair(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_NONCE antiReplay;
	UINT32 eKPtrSize;
	BYTE *eKPtr;
	UINT32 eKSize;
	BYTE* eK;
	TCPA_DIGEST checksum;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &eKPtrSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	eKPtr = calloc(1, eKPtrSize);
	if (eKPtr == NULL) {
		LogError("malloc of %u bytes failed.", eKPtrSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, eKPtr, eKPtrSize, &data->comm)) {
		free(eKPtr);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateEndorsementKeyPair_Internal(hContext, antiReplay, eKPtrSize, eKPtr,
							&eKSize, &eK, &checksum);

	MUTEX_UNLOCK(tcsp_lock);

	free(eKPtr);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &eKSize, 0, &data->comm)) {
			free(eK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, eK, eKSize, &data->comm)) {
			free(eK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(eK);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 2, &checksum, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ReadPubek(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_NONCE antiReplay;
	UINT32 pubEKSize;
	BYTE *pubEK;
	TCPA_DIGEST checksum;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReadPubek_Internal(hContext, antiReplay, &pubEKSize, &pubEK, &checksum);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &pubEKSize, 0, &data->comm)) {
			free(pubEK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, pubEK, pubEKSize, &data->comm)) {
			free(pubEK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(pubEK);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 2, &checksum, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_OwnerReadPubek(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 pubEKSize;
	BYTE *pubEK;
	TSS_RESULT result;
	TPM_AUTH auth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_AUTH, 1, &auth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_OwnerReadPubek_Internal(hContext, &auth, &pubEKSize, &pubEK);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &auth, 0, &data->comm)) {
			free(pubEK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &pubEKSize, 0, &data->comm)) {
			free(pubEK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, pubEK, pubEKSize, &data->comm)) {
			free(pubEK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(pubEK);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_DisablePubekRead(struct tcsd_thread_data *data)
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

	result = TCSP_DisablePubekRead_Internal(hContext, &auth);

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

#ifdef TSS_BUILD_TSS12
TSS_RESULT
tcs_wrap_CreateRevocableEndorsementKeyPair(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_NONCE antiReplay;
	UINT32 eKPtrSize;
	BYTE *eKPtr;
	TSS_BOOL genResetAuth;
	TPM_DIGEST eKResetAuth;
	UINT32 eKSize;
	BYTE* eK;
	TPM_DIGEST checksum;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &eKPtrSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	eKPtr = calloc(1, eKPtrSize);
	if (eKPtr == NULL) {
		LogError("malloc of %d bytes failed.", eKPtrSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, eKPtr, eKPtrSize, &data->comm)) {
		free(eKPtr);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_BOOL, 4, &genResetAuth, 0, &data->comm)) {
		free(eKPtr);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_DIGEST, 5, &eKResetAuth, 0, &data->comm)) {
		free(eKPtr);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateRevocableEndorsementKeyPair_Internal(hContext, antiReplay,
			eKPtrSize, eKPtr, genResetAuth, &eKResetAuth, &eKSize, &eK, &checksum);

	MUTEX_UNLOCK(tcsp_lock);

	free(eKPtr);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 4);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &eKResetAuth, 0, &data->comm)) {
			free(eK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &eKSize, 0, &data->comm)) {
			free(eK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, eK, eKSize, &data->comm)) {
			free(eK);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(eK);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 3, &checksum, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_RevokeEndorsementKeyPair(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_DIGEST eKResetAuth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_DIGEST, 1, &eKResetAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_RevokeEndorsementKeyPair_Internal(hContext, eKResetAuth);

	MUTEX_UNLOCK(tcsp_lock);

	initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
#endif
