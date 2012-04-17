
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
tcs_wrap_CMK_SetRestrictions(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_CMK_DELEGATE restriction;
	TPM_AUTH ownerAuth;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &restriction, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_SetRestrictions_Internal(hContext, restriction, &ownerAuth);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CMK_ApproveMA(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_DIGEST migAuthorityDigest;
	TPM_AUTH ownerAuth;
	TPM_HMAC migAuthorityApproval;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_DIGEST, 1, &migAuthorityDigest, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 2, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_ApproveMA_Internal(hContext, migAuthorityDigest, &ownerAuth,
			&migAuthorityApproval);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);

		if (setData(TCSD_PACKET_TYPE_DIGEST, 1, &migAuthorityApproval, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CMK_CreateKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	TPM_ENCAUTH keyUsageAuth;
	TPM_HMAC migAuthorityApproval;
	TPM_DIGEST migAuthorityDigest;
	UINT32 keyDataSize;
	BYTE *keyData;
	TPM_AUTH parentAuth, nullAuth, *pAuth;
	TSS_RESULT result;

	memset(&parentAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 2, &keyUsageAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_DIGEST, 3, &migAuthorityApproval, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_DIGEST, 4, &migAuthorityDigest, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &keyDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	keyData = malloc(keyDataSize);
	if (keyData == NULL) {
		LogError("malloc of %u bytes failed.", keyDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 6, keyData, keyDataSize, &data->comm)) {
		free(keyData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &parentAuth, 0, &data->comm)) {
		free(keyData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &parentAuth, sizeof(TPM_AUTH)))
		pAuth = &parentAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_CreateKey_Internal(hContext, hKey, keyUsageAuth, migAuthorityApproval,
			migAuthorityDigest, &keyDataSize, &keyData, pAuth);

	MUTEX_UNLOCK(tcsp_lock);

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

TSS_RESULT
tcs_wrap_CMK_CreateTicket(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 publicVerifyKeySize;
	BYTE *publicVerifyKey;
	TPM_DIGEST signedData;
	UINT32 sigValueSize;
	BYTE *sigValue;
	TPM_AUTH ownerAuth;
	TPM_HMAC sigTicket;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &publicVerifyKeySize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	publicVerifyKey = malloc(publicVerifyKeySize);
	if (publicVerifyKey == NULL) {
		LogError("malloc of %u bytes failed.", publicVerifyKeySize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, publicVerifyKey, publicVerifyKeySize, &data->comm)) {
		free(publicVerifyKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_DIGEST, 3, &signedData, 0, &data->comm)) {
		free(publicVerifyKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &sigValueSize, 0, &data->comm)) {
		free(publicVerifyKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	sigValue = malloc(sigValueSize);
	if (sigValue == NULL) {
		LogError("malloc of %u bytes failed.", sigValueSize);
		free(publicVerifyKey);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, sigValue, sigValueSize, &data->comm)) {
		free(publicVerifyKey);
		free(sigValue);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 6, &ownerAuth, 0, &data->comm)) {
		free(publicVerifyKey);
		free(sigValue);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_CreateTicket_Internal(hContext, publicVerifyKeySize, publicVerifyKey,
			signedData, sigValueSize, sigValue, &ownerAuth, &sigTicket);

	MUTEX_UNLOCK(tcsp_lock);
	free(publicVerifyKey);
	free(sigValue);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);

		if (setData(TCSD_PACKET_TYPE_DIGEST, 1, &sigTicket, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_CMK_CreateBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	UINT16 migrationType;
	UINT32 migKeyAuthSize;
	BYTE *migKeyAuth;
	TPM_DIGEST pubSourceKeyDigest;
	UINT32 msaListSize, restrictTicketSize, sigTicketSize, encDataSize;
	BYTE *msaList, *restrictTicket, *sigTicket, *encData;
	TPM_AUTH parentAuth, nullAuth, *pAuth;
	UINT32 randomSize, outDataSize;
	BYTE *random, *outData;
	TSS_RESULT result;
	int i;

	memset(&parentAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT16, 2, &migrationType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &migKeyAuthSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	migKeyAuth = malloc(migKeyAuthSize);
	if (migKeyAuth == NULL) {
		LogError("malloc of %u bytes failed.", migKeyAuthSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, migKeyAuth, migKeyAuthSize, &data->comm)) {
		free(migKeyAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_DIGEST, 5, &pubSourceKeyDigest, 0, &data->comm)) {
		free(migKeyAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 6, &msaListSize, 0, &data->comm)) {
		free(migKeyAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	msaList = malloc(msaListSize);
	if (msaList == NULL) {
		LogError("malloc of %u bytes failed.", msaListSize);
		free(migKeyAuth);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 7, msaList, msaListSize, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 8, &restrictTicketSize, 0, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	restrictTicket = malloc(restrictTicketSize);
	if (restrictTicket == NULL) {
		LogError("malloc of %u bytes failed.", restrictTicketSize);
		free(migKeyAuth);
		free(msaList);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 9, restrictTicket, restrictTicketSize, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 10, &sigTicketSize, 0, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	sigTicket = malloc(sigTicketSize);
	if (sigTicket == NULL) {
		LogError("malloc of %u bytes failed.", sigTicketSize);
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 11, sigTicket, sigTicketSize, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		free(sigTicket);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 12, &encDataSize, 0, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		free(sigTicket);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	encData = malloc(encDataSize);
	if (encData == NULL) {
		LogError("malloc of %u bytes failed.", encDataSize);
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		free(sigTicket);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 13, encData, encDataSize, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		free(sigTicket);
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 14, &parentAuth, 0, &data->comm)) {
		free(migKeyAuth);
		free(msaList);
		free(restrictTicket);
		free(sigTicket);
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &parentAuth, sizeof(TPM_AUTH)))
		pAuth = &parentAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_CreateBlob_Internal(hContext, hKey, migrationType, migKeyAuthSize,
			migKeyAuth, pubSourceKeyDigest, msaListSize, msaList, restrictTicketSize,
			restrictTicket, sigTicketSize, sigTicket, encDataSize, encData, pAuth,
			&randomSize, &random, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(migKeyAuth);
	free(msaList);
	free(restrictTicket);
	free(sigTicket);
	free(encData);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 5);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(random);
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &randomSize, 0, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, random, randomSize, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(random);

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outDataSize, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outData, outDataSize, &data->comm)) {
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
tcs_wrap_CMK_ConvertMigration(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hKey;
	TPM_CMK_AUTH restrictTicket;
	TPM_HMAC sigTicket;
	UINT32 keyDataSize, msaListSize, randomSize;
	BYTE *keyData, *msaList, *random;
	TPM_AUTH parentAuth, nullAuth, *pAuth;
	UINT32 outDataSize;
	BYTE *outData;
	TSS_RESULT result;
	int i;

	memset(&parentAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, &restrictTicket, sizeof(restrictTicket), &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_DIGEST, 3, &sigTicket, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &keyDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	keyData = malloc(keyDataSize);
	if (keyData == NULL) {
		LogError("malloc of %u bytes failed.", keyDataSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, keyData, keyDataSize, &data->comm)) {
		free(keyData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 6, &msaListSize, 0, &data->comm)) {
		free(keyData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	msaList = malloc(msaListSize);
	if (msaList == NULL) {
		LogError("malloc of %u bytes failed.", msaListSize);
		free(keyData);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 7, msaList, msaListSize, &data->comm)) {
		free(keyData);
		free(msaList);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 8, &randomSize, 0, &data->comm)) {
		free(keyData);
		free(msaList);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	random = malloc(randomSize);
	if (random == NULL) {
		LogError("malloc of %u bytes failed.", randomSize);
		free(keyData);
		free(msaList);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 9, random, randomSize, &data->comm)) {
		free(keyData);
		free(msaList);
		free(random);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 10, &parentAuth, 0, &data->comm)) {
		free(keyData);
		free(msaList);
		free(random);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (memcmp(&nullAuth, &parentAuth, sizeof(TPM_AUTH)))
		pAuth = &parentAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CMK_ConvertMigration_Internal(hContext, hKey, restrictTicket, sigTicket,
			keyDataSize, keyData, msaListSize, msaList, randomSize, random,
			pAuth, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);
	free(keyData);
	free(msaList);
	free(random);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outDataSize, 0, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outData, outDataSize, &data->comm)) {
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(outData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

