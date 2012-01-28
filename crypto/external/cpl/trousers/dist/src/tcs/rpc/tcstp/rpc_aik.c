
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
tcs_wrap_MakeIdentity(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_ENCAUTH identityAuth;
	TCPA_CHOSENID_HASH privCAHash;
	UINT32 idKeyInfoSize;
	BYTE *idKeyInfo = NULL;

	TPM_AUTH auth1, auth2;
	TPM_AUTH *pSRKAuth, *pOwnerAuth;

	UINT32 idKeySize;
	BYTE *idKey = NULL;
	UINT32 pcIDBindSize;
	BYTE *prgbIDBind = NULL;
	UINT32 pcECSize;
	BYTE *prgbEC = NULL;
	UINT32 pcPlatCredSize;
	BYTE *prgbPlatCred = NULL;
	UINT32 pcConfCredSize;
	BYTE *prgbConfCred = NULL;
	TSS_RESULT result;

	int i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 1, &identityAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_DIGEST, 2, &privCAHash, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &idKeyInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	idKeyInfo = (BYTE *) calloc(1, idKeyInfoSize);
	if (idKeyInfo == NULL) {
		LogError("malloc of %d bytes failed.", idKeyInfoSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, idKeyInfo, idKeyInfoSize, &data->comm)) {
		free(idKeyInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &auth1, 0, &data->comm)) {
		free(idKeyInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = getData(TCSD_PACKET_TYPE_AUTH, 6, &auth2, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE) {
		pOwnerAuth = &auth1;
		pSRKAuth = NULL;
	} else if (result) {
		free(idKeyInfo);
		return result;
	} else {
		pOwnerAuth = &auth2;
		pSRKAuth = &auth1;
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_MakeIdentity_Internal(hContext, identityAuth, privCAHash,
				       idKeyInfoSize, idKeyInfo, pSRKAuth,
				       pOwnerAuth, &idKeySize, &idKey,
				       &pcIDBindSize, &prgbIDBind, &pcECSize,
				       &prgbEC, &pcPlatCredSize, &prgbPlatCred,
				       &pcConfCredSize, &prgbConfCred);

	MUTEX_UNLOCK(tcsp_lock);
	free(idKeyInfo);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 12);
		if (pSRKAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pSRKAuth, 0, &data->comm))
				goto internal_error;
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &idKeySize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, idKey, idKeySize, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pcIDBindSize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, prgbIDBind, pcIDBindSize, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pcECSize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, prgbEC, pcECSize, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pcPlatCredSize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, prgbPlatCred, pcPlatCredSize, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pcConfCredSize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, prgbConfCred, pcConfCredSize, &data->comm))
			goto internal_error;

		free(idKey);
		free(prgbIDBind);
		free(prgbEC);
		free(prgbPlatCred);
		free(prgbConfCred);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;

internal_error:
	free(idKey);
	free(prgbIDBind);
	free(prgbEC);
	free(prgbPlatCred);
	free(prgbConfCred);
	return TCSERR(TSS_E_INTERNAL_ERROR);
}

TSS_RESULT
tcs_wrap_ActivateIdentity(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE idKeyHandle;
	TPM_AUTH *pIdKeyAuth = NULL, *pOwnerAuth = NULL, auth1, auth2;
	UINT32 SymmetricKeySize, blobSize;
	BYTE *SymmetricKey, *blob;
	TSS_RESULT result;
	UINT32 i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &idKeyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &blobSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if ((blob = malloc(blobSize)) == NULL)
		return TCSERR(TSS_E_OUTOFMEMORY);

	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, blob, blobSize, &data->comm)) {
		free(blob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &auth1, 0, &data->comm)) {
		free(blob);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = getData(TCSD_PACKET_TYPE_AUTH, 5, &auth2, 0, &data->comm);
	if (result == TSS_TCP_RPC_BAD_PACKET_TYPE)
		pOwnerAuth = &auth1;
	else if (result) {
		free(blob);
		return result;
	} else {
		pIdKeyAuth = &auth1;
		pOwnerAuth = &auth2;
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ActivateTPMIdentity_Internal(hContext, idKeyHandle, blobSize,
						   blob, pIdKeyAuth, pOwnerAuth,
						   &SymmetricKeySize,
						   &SymmetricKey);

	MUTEX_UNLOCK(tcsp_lock);
	free(blob);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 4);
		if (pIdKeyAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pIdKeyAuth, 0, &data->comm)) {
				free(SymmetricKey);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pOwnerAuth, 0, &data->comm)) {
			free(SymmetricKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &SymmetricKeySize, 0, &data->comm)) {
			free(SymmetricKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, SymmetricKey, SymmetricKeySize, &data->comm)) {
			free(SymmetricKey);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(SymmetricKey);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_TSS12
TSS_RESULT
tcs_wrap_GetCredential(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 CredType;
	UINT32 CredAccessMode;
	UINT32 CredSize;
	BYTE *CredData = NULL;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &CredType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &CredAccessMode, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	result = TCS_GetCredential_Internal(hContext, CredType, CredAccessMode, 
					    &CredSize, &CredData);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &CredSize, 0, &data->comm))
			goto internal_error;
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, CredData, CredSize, &data->comm))
			goto internal_error;

		free(CredData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;

internal_error:
	free(CredData);
	return TCSERR(TSS_E_INTERNAL_ERROR);
}
#endif
