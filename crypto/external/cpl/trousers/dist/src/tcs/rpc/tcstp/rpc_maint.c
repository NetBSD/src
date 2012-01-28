
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
tcs_wrap_KillMaintenanceFeature(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TPM_AUTH ownerAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_AUTH, 1, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_KillMaintenanceFeature_Internal(hContext, &ownerAuth);

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

TSS_RESULT
tcs_wrap_CreateMaintenanceArchive(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TPM_AUTH ownerAuth;
	TSS_BOOL generateRandom;
	UINT32 randomSize, archiveSize;
	BYTE *random, *archive;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_BOOL, 1, &generateRandom, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateMaintenanceArchive_Internal(hContext, generateRandom, &ownerAuth,
							&randomSize, &random, &archiveSize,
							&archive);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 5);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			free(random);
			free(archive);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &randomSize, 0, &data->comm)) {
			free(random);
			free(archive);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, random, randomSize, &data->comm)) {
			free(random);
			free(archive);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, 3, &archiveSize, 0, &data->comm)) {
			free(random);
			free(archive);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 4, archive, archiveSize, &data->comm)) {
			free(random);
			free(archive);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		free(random);
		free(archive);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_LoadMaintenanceArchive(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TPM_AUTH ownerAuth;
	UINT32 dataInSize, dataOutSize;
	BYTE *dataIn, *dataOut;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &dataInSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	dataIn = (BYTE *)malloc(dataInSize);
	if (dataIn == NULL) {
		LogError("malloc of %d bytes failed.", dataInSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, dataIn, dataInSize, &data->comm)) {
		free(dataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 3, &ownerAuth, 0, &data->comm)) {
		free(dataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_LoadMaintenanceArchive_Internal(hContext, dataInSize, dataIn, &ownerAuth,
							&dataOutSize, &dataOut);

	MUTEX_UNLOCK(tcsp_lock);
	free(dataIn);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			free(dataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &dataOutSize, 0, &data->comm)) {
			free(dataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, dataOut, dataOutSize, &data->comm)) {
			free(dataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		free(dataOut);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_LoadManuMaintPub(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	UINT32 pubKeySize;
	BYTE *pubKey;
	TCPA_NONCE antiReplay;
	TCPA_DIGEST checksum;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &pubKeySize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	pubKey = (BYTE *)malloc(pubKeySize);
	if (pubKey == NULL) {
		LogError("malloc of %d bytes failed.", pubKeySize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, pubKey, pubKeySize, &data->comm)) {
		free(pubKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_LoadManuMaintPub_Internal(hContext, antiReplay, pubKeySize, pubKey,
						&checksum);

	MUTEX_UNLOCK(tcsp_lock);
	free(pubKey);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &checksum, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ReadManuMaintPub(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TCPA_NONCE antiReplay;
	TCPA_DIGEST checksum;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_NONCE, 1, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReadManuMaintPub_Internal(hContext, antiReplay, &checksum);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &checksum, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
