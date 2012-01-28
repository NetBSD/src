
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
tcs_wrap_RegisterKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID WrappingKeyUUID;
	TSS_UUID KeyUUID;
	UINT32 cKeySize;
	BYTE *rgbKey;
	UINT32 cVendorData;
	BYTE *gbVendorData;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UUID, 1, &WrappingKeyUUID, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UUID, 2, &KeyUUID, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &cKeySize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbKey = calloc(1, cKeySize);
	if (rgbKey == NULL) {
		LogError("malloc of %d bytes failed.", cKeySize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, rgbKey, cKeySize, &data->comm)) {
		free(rgbKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &cVendorData, 0, &data->comm)) {
		free(rgbKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (cVendorData == 0)
		gbVendorData = NULL;
	else {
		gbVendorData = calloc(1, cVendorData);
		if (gbVendorData == NULL) {
			LogError("malloc of %d bytes failed.", cVendorData);
			free(rgbKey);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, 6, gbVendorData, cVendorData, &data->comm)) {
			free(rgbKey);
			free(gbVendorData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}

	result = TCS_RegisterKey_Internal(hContext, &WrappingKeyUUID, &KeyUUID,
				     cKeySize, rgbKey, cVendorData,
				     gbVendorData);
	free(rgbKey);
	free(gbVendorData);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_UnregisterKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID uuid;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UUID, 1, &uuid, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = TCS_UnregisterKey_Internal(hContext, uuid);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetRegisteredKeyBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID uuid;
	UINT32 pcKeySize;
	BYTE *prgbKey;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UUID, 1, &uuid, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = TCS_GetRegisteredKeyBlob_Internal(hContext, &uuid, &pcKeySize,
					      &prgbKey);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &pcKeySize, 0, &data->comm)) {
			free(prgbKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, prgbKey, pcKeySize, &data->comm)) {
			free(prgbKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(prgbKey);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_LoadKeyByUUID(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID uuid;
	TCS_LOADKEY_INFO info, *pInfo;
	TCS_KEY_HANDLE phKeyTCSI;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UUID, 1, &uuid, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = getData(TCSD_PACKET_TYPE_LOADKEY_INFO, 2, &info, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pInfo = NULL;
	else if (result)
		return result;
	else
		pInfo = &info;

	result = key_mgr_load_by_uuid(hContext, &uuid, pInfo, &phKeyTCSI);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &phKeyTCSI, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (pInfo != NULL) {
			if (setData(TCSD_PACKET_TYPE_LOADKEY_INFO, 1, pInfo, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
	} else {
		if (result == TCSERR(TCS_E_KM_LOADFAILED) && pInfo != NULL) {
			initData(&data->comm, 1);
			if (setData(TCSD_PACKET_TYPE_LOADKEY_INFO, 0, pInfo, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		} else
			initData(&data->comm, 0);

	}
	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_EnumRegisteredKeys(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID uuid, *pUuid;
	UINT32 cKeyHierarchySize;
	TSS_KM_KEYINFO *pKeyHierarchy;
	unsigned int i, j;
	TSS_RESULT result;

	/* Receive */
	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	result = getData(TCSD_PACKET_TYPE_UUID , 1, &uuid, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pUuid = NULL;
	else if (result)
		return result;
	else
		pUuid = &uuid;

	result = TCS_EnumRegisteredKeys_Internal(
			hContext,
			pUuid,
			&cKeyHierarchySize,
			&pKeyHierarchy);

	if (result == TSS_SUCCESS) {
		i=0;
		initData(&data->comm, cKeyHierarchySize + 1);
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &cKeyHierarchySize, 0, &data->comm)) {
			free(pKeyHierarchy);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		for (j = 0; j < cKeyHierarchySize; j++) {
			if (setData(TCSD_PACKET_TYPE_KM_KEYINFO, i++, &pKeyHierarchy[j], 0, &data->comm)) {
				free(pKeyHierarchy);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		free(pKeyHierarchy);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_EnumRegisteredKeys2(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_UUID uuid, *pUuid;
	UINT32 cKeyHierarchySize;
	TSS_KM_KEYINFO2 *pKeyHierarchy;
	unsigned int i, j;
	TSS_RESULT result;

	/* Receive */
	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	result = getData(TCSD_PACKET_TYPE_UUID , 1, &uuid, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pUuid = NULL;
	else if (result)
		return result;
	else
		pUuid = &uuid;

	result = TCS_EnumRegisteredKeys_Internal2(
			hContext,
			pUuid,
			&cKeyHierarchySize,
			&pKeyHierarchy);

	if (result == TSS_SUCCESS) {
		i=0; 
		initData(&data->comm, cKeyHierarchySize + 1);
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &cKeyHierarchySize, 0, &data->comm)) {
			free(pKeyHierarchy);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		for (j = 0; j < cKeyHierarchySize; j++) {
			if (setData(TCSD_PACKET_TYPE_KM_KEYINFO2, i++, &pKeyHierarchy[j], 0, &data->comm)) {
				free(pKeyHierarchy);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		free(pKeyHierarchy);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetRegisteredKeyByPublicInfo(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	UINT32 algId, ulPublicInfoLength, keySize;
	BYTE *rgbPublicInfo, *keyBlob;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &algId, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &ulPublicInfoLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbPublicInfo = (BYTE *)calloc(1, ulPublicInfoLength);
	if (rgbPublicInfo == NULL) {
		LogError("malloc of %d bytes failed.", ulPublicInfoLength);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, rgbPublicInfo, ulPublicInfoLength, &data->comm)) {
		free(rgbPublicInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = TCSP_GetRegisteredKeyByPublicInfo_Internal(hContext, algId,
			ulPublicInfoLength, rgbPublicInfo, &keySize, &keyBlob);

	free(rgbPublicInfo);
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &keySize, 0, &data->comm)) {
			free(keyBlob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, keyBlob, keySize, &data->comm)) {
			free(keyBlob);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(keyBlob);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

void
LoadBlob_LOADKEY_INFO(UINT64 *offset, BYTE *blob, TCS_LOADKEY_INFO *info)
{
	LoadBlob_UUID(offset, blob, info->keyUUID);
	LoadBlob_UUID(offset, blob, info->parentKeyUUID);
	LoadBlob(offset, TCPA_DIGEST_SIZE, blob, info->paramDigest.digest);
	LoadBlob_UINT32(offset, info->authData.AuthHandle, blob);
	LoadBlob(offset, TCPA_NONCE_SIZE, blob, info->authData.NonceOdd.nonce);
	LoadBlob(offset, TCPA_NONCE_SIZE, blob, info->authData.NonceEven.nonce);
	LoadBlob_BOOL(offset, info->authData.fContinueAuthSession, blob);
	LoadBlob(offset, TCPA_AUTHDATA_SIZE, blob, (BYTE *)&info->authData.HMAC);
}

void
UnloadBlob_LOADKEY_INFO(UINT64 *offset, BYTE *blob, TCS_LOADKEY_INFO *info)
{
	if (!info) {
		UnloadBlob_UUID(offset, blob, NULL);
		UnloadBlob_UUID(offset, blob, NULL);
		UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, NULL);
		UnloadBlob_UINT32(offset, NULL, blob);
		UnloadBlob(offset, TCPA_NONCE_SIZE, blob, NULL);
		UnloadBlob(offset, TCPA_NONCE_SIZE, blob, NULL);
		UnloadBlob_BOOL(offset, NULL, blob);
		UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, NULL);

		return;
	}

	UnloadBlob_UUID(offset, blob, &info->keyUUID);
	UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, info->paramDigest.digest);
	UnloadBlob_UINT32(offset, &info->authData.AuthHandle, blob);
	UnloadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceOdd.nonce);
	UnloadBlob(offset, TCPA_NONCE_SIZE, blob, (BYTE *)&info->authData.NonceEven.nonce);
	UnloadBlob_BOOL(offset, &info->authData.fContinueAuthSession, blob);
	UnloadBlob(offset, TCPA_DIGEST_SIZE, blob, (BYTE *)&info->authData.HMAC);
}

void
LoadBlob_UUID(UINT64 *offset, BYTE * blob, TSS_UUID uuid)
{
        LoadBlob_UINT32(offset, uuid.ulTimeLow, blob);
        LoadBlob_UINT16(offset, uuid.usTimeMid, blob);
        LoadBlob_UINT16(offset, uuid.usTimeHigh, blob);
        LoadBlob_BYTE(offset, uuid.bClockSeqHigh, blob);
        LoadBlob_BYTE(offset, uuid.bClockSeqLow, blob);
        LoadBlob(offset, 6, blob, uuid.rgbNode);
}

void
UnloadBlob_UUID(UINT64 *offset, BYTE * blob, TSS_UUID *uuid)
{
        if (!uuid) {
                UnloadBlob_UINT32(offset, NULL, blob);
                UnloadBlob_UINT16(offset, NULL, blob);
                UnloadBlob_UINT16(offset, NULL, blob);
                UnloadBlob_BYTE(offset, NULL, blob);
                UnloadBlob_BYTE(offset, NULL, blob);
                UnloadBlob(offset, 6, blob, NULL);

                return;
        }

        memset(uuid, 0, sizeof(TSS_UUID));
        UnloadBlob_UINT32(offset, &uuid->ulTimeLow, blob);
        UnloadBlob_UINT16(offset, &uuid->usTimeMid, blob);
        UnloadBlob_UINT16(offset, &uuid->usTimeHigh, blob);
        UnloadBlob_BYTE(offset, &uuid->bClockSeqHigh, blob);
        UnloadBlob_BYTE(offset, &uuid->bClockSeqLow, blob);
        UnloadBlob(offset, 6, blob, uuid->rgbNode);
}

void
LoadBlob_KM_KEYINFO(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO *info)
{
	LoadBlob_VERSION(offset, blob, (TPM_VERSION *)&(info->versionInfo));
	LoadBlob_UUID(offset, blob, info->keyUUID);
	LoadBlob_UUID(offset, blob, info->parentKeyUUID);
	LoadBlob_BYTE(offset, info->bAuthDataUsage, blob);
	LoadBlob_BOOL(offset, info->fIsLoaded, blob);
	LoadBlob_UINT32(offset, info->ulVendorDataLength, blob);
	LoadBlob(offset, info->ulVendorDataLength, blob, info->rgbVendorData);
}

void
LoadBlob_KM_KEYINFO2(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO2 *info)
{
	LoadBlob_VERSION(offset, blob, (TPM_VERSION *)&(info->versionInfo));
	LoadBlob_UUID(offset, blob, info->keyUUID);
	LoadBlob_UUID(offset, blob, info->parentKeyUUID);
	LoadBlob_BYTE(offset, info->bAuthDataUsage, blob);
	/* Load the infos of the blob regarding the new data type TSS_KM_KEYINFO2 */
	LoadBlob_UINT32(offset,info->persistentStorageType,blob);
	LoadBlob_UINT32(offset, info->persistentStorageTypeParent,blob);

	LoadBlob_BOOL(offset, info->fIsLoaded, blob);
	LoadBlob_UINT32(offset, info->ulVendorDataLength, blob);
	LoadBlob(offset, info->ulVendorDataLength, blob, info->rgbVendorData);
}

void
UnloadBlob_KM_KEYINFO(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO *info)
{
	if (!info) {
		UINT32 ulVendorDataLength;

		UnloadBlob_VERSION(offset, blob, NULL);
		UnloadBlob_UUID(offset, blob, NULL);
		UnloadBlob_UUID(offset, blob, NULL);
		UnloadBlob_BYTE(offset, blob, NULL);
		UnloadBlob_BOOL(offset, NULL, blob);
		UnloadBlob_UINT32(offset, &ulVendorDataLength, blob);

		(*offset) += ulVendorDataLength;

		return;
	}

	UnloadBlob_VERSION(offset, blob, (TPM_VERSION *)&(info->versionInfo));
	UnloadBlob_UUID(offset, blob, &info->keyUUID);
	UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	UnloadBlob_BYTE(offset, blob, &info->bAuthDataUsage);
	UnloadBlob_BOOL(offset, &info->fIsLoaded, blob);
	UnloadBlob_UINT32(offset, &info->ulVendorDataLength, blob);
	UnloadBlob(offset, info->ulVendorDataLength, info->rgbVendorData, blob);
}

#if 0
void
UnloadBlob_KM_KEYINFO2(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO2 *info)
{
	UnloadBlob_VERSION(offset, blob, (TCPA_VERSION *)&(info->versionInfo));
	UnloadBlob_UUID(offset, blob, &info->keyUUID);
	UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	UnloadBlob_BYTE(offset, blob, &info->bAuthDataUsage);
	
	/* Extract the infos of the blob regarding the new data type TSS_KM_KEYINFO2 */
	UnloadBlob_UINT32(offset, &info->persistentStorageType, blob);
	UnloadBlob_UINT32(offset, &info->persistentStorageTypeParent, blob);
	
	UnloadBlob_BOOL(offset, &info->fIsLoaded, blob);
	UnloadBlob_UINT32(offset, &info->ulVendorDataLength, blob);
	UnloadBlob(offset, info->ulVendorDataLength, info->rgbVendorData, blob);
}
#endif
