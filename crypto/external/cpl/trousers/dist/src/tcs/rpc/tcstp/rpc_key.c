
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
tcs_wrap_EvictKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = key_mgr_evict(hContext, hKey);

	MUTEX_UNLOCK(tcsp_lock);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetPubkey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	TPM_AUTH auth;
	TPM_AUTH *pAuth;
	UINT32 pubKeySize;
	BYTE *pubKey;
	TSS_RESULT result;
	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	result = getData(TCSD_PACKET_TYPE_AUTH, 2, &auth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pAuth = NULL;
	else if (result)
		return result;
	else
		pAuth = &auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetPubKey_Internal(hContext, hKey, pAuth, &pubKeySize, &pubKey);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth != NULL)
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(pubKey);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pubKeySize, 0, &data->comm)) {
			free(pubKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, pubKey, pubKeySize, &data->comm)) {
			free(pubKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(pubKey);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_TerminateHandle(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_AUTHHANDLE authHandle;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &authHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_TerminateHandle_Internal(hContext, authHandle);

	MUTEX_UNLOCK(tcsp_lock);

	initData(&data->comm, 0);
	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_LoadKeyByBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hUnwrappingKey;
	UINT32 cWrappedKeyBlob;
	BYTE *rgbWrappedKeyBlob;

	TPM_AUTH auth;

	TCS_KEY_HANDLE phKeyTCSI;
	TCS_KEY_HANDLE phKeyHMAC;

	TPM_AUTH *pAuth;
	TSS_RESULT result;
	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hUnwrappingKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &cWrappedKeyBlob, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbWrappedKeyBlob = calloc(1, cWrappedKeyBlob);
	if (rgbWrappedKeyBlob == NULL) {
		LogError("malloc of %d bytes failed.", cWrappedKeyBlob);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, rgbWrappedKeyBlob, cWrappedKeyBlob, &data->comm)) {
		free(rgbWrappedKeyBlob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	result = getData(TCSD_PACKET_TYPE_AUTH, 4, &auth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pAuth = NULL;
	else if (result) {
		free(rgbWrappedKeyBlob);
		return result;
	} else
		pAuth = &auth;

	MUTEX_LOCK(tcsp_lock);

	result = key_mgr_load_by_blob(hContext, hUnwrappingKey, cWrappedKeyBlob, rgbWrappedKeyBlob,
				      pAuth, &phKeyTCSI, &phKeyHMAC);

	if (!result)
		result = ctx_mark_key_loaded(hContext, phKeyTCSI);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbWrappedKeyBlob);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &phKeyTCSI, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &phKeyHMAC, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
tcs_wrap_LoadKey2ByBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hUnwrappingKey;
	UINT32 cWrappedKeyBlob;
	BYTE *rgbWrappedKeyBlob;
	TPM_AUTH auth;
	TCS_KEY_HANDLE phKeyTCSI;
	TPM_AUTH *pAuth;
	TSS_RESULT result;
	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hUnwrappingKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &cWrappedKeyBlob, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbWrappedKeyBlob = calloc(1, cWrappedKeyBlob);
	if (rgbWrappedKeyBlob == NULL) {
		LogError("malloc of %d bytes failed.", cWrappedKeyBlob);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, rgbWrappedKeyBlob, cWrappedKeyBlob, &data->comm)) {
		free(rgbWrappedKeyBlob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	result = getData(TCSD_PACKET_TYPE_AUTH, 4, &auth, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pAuth = NULL;
	else if (result) {
		free(rgbWrappedKeyBlob);
		return result;
	} else
		pAuth = &auth;

	MUTEX_LOCK(tcsp_lock);

	result = key_mgr_load_by_blob(hContext, hUnwrappingKey, cWrappedKeyBlob, rgbWrappedKeyBlob,
				      pAuth, &phKeyTCSI, NULL);

	if (!result)
		result = ctx_mark_key_loaded(hContext, phKeyTCSI);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbWrappedKeyBlob);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 2);
		if (pAuth != NULL) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &phKeyTCSI, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
#endif

TSS_RESULT
tcs_wrap_CreateWrapKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hWrappingKey;
	TCPA_ENCAUTH KeyUsageAuth;
	TCPA_ENCAUTH KeyMigrationAuth;
	UINT32 keyInfoSize;
	BYTE *keyInfo;

	TPM_AUTH *pAuth, auth;

	UINT32 keyDataSize;
	BYTE *keyData;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hWrappingKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 2, &KeyUsageAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 3, &KeyMigrationAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &keyInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	keyInfo = calloc(1, keyInfoSize);
	if (keyInfo == NULL) {
		LogError("malloc of %d bytes failed.", keyInfoSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, keyInfo, keyInfoSize, &data->comm)) {
		free(keyInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 6, &auth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateWrapKey_Internal(hContext, hWrappingKey, KeyUsageAuth, KeyMigrationAuth,
					     keyInfoSize, keyInfo, &keyDataSize, &keyData, pAuth);

	MUTEX_UNLOCK(tcsp_lock);

	free(keyInfo);
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &keyDataSize, 0, &data->comm)) {
			free(keyData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, keyData, keyDataSize, &data->comm)) {
			free(keyData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(keyData);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 2, pAuth, 0, &data->comm))
				return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
tcs_wrap_OwnerReadInternalPub(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	TPM_AUTH ownerAuth;
	UINT32 pubKeySize;
	BYTE *pubKeyData;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_OwnerReadInternalPub_Internal(hContext, hKey, &ownerAuth, &pubKeySize, &pubKeyData);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &pubKeySize, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, pubKeyData, pubKeySize, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_KeyControlOwner(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	UINT32 ulPublicKeyLength;
	BYTE* rgbPublicKey;
	UINT32 attribName;
	TSS_BOOL attribValue;
	TPM_AUTH ownerAuth;
	TSS_UUID uuidData;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &ulPublicKeyLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbPublicKey = (BYTE *) malloc(ulPublicKeyLength);
	if (rgbPublicKey == NULL) {
		LogError("malloc of %u bytes failed.", ulPublicKeyLength);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, rgbPublicKey, ulPublicKeyLength, &data->comm)) {
		free(rgbPublicKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &attribName, 0, &data->comm)) {
		free(rgbPublicKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_BOOL, 5, &attribValue, 0, &data->comm)) {
		free(rgbPublicKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 6, &ownerAuth, 0, &data->comm)) {
		free(rgbPublicKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_KeyControlOwner_Internal(hContext, hKey, ulPublicKeyLength, rgbPublicKey,
					       attribName, attribValue, &ownerAuth, &uuidData);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
		if (setData(TCSD_PACKET_TYPE_UUID, 1, &uuidData, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;

}
#endif
