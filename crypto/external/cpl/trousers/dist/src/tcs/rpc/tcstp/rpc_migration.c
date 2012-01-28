
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
tcs_wrap_CreateMigrationBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TCS_KEY_HANDLE parentHandle;
	TSS_MIGRATE_SCHEME migrationType;
	UINT32 MigrationKeyAuthSize, encDataSize, randomSize, outDataSize;
	BYTE *MigrationKeyAuth, *encData, *random, *outData;
	TPM_AUTH auth1, auth2, *pParentAuth, *pEntityAuth;
	UINT32 i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT16, 2, &migrationType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &MigrationKeyAuthSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MigrationKeyAuth = (BYTE *)malloc(MigrationKeyAuthSize);
	if (MigrationKeyAuth == NULL) {
		LogError("malloc of %d bytes failed.", MigrationKeyAuthSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, MigrationKeyAuth, MigrationKeyAuthSize, &data->comm)) {
		free(MigrationKeyAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &encDataSize, 0, &data->comm)) {
		free(MigrationKeyAuth);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	encData = (BYTE *)malloc(encDataSize);
	if (encData == NULL) {
		free(MigrationKeyAuth);
		LogError("malloc of %d bytes failed.", encDataSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 6, encData, encDataSize, &data->comm)) {
		free(MigrationKeyAuth);
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &auth1, 0, &data->comm)) {
		free(MigrationKeyAuth);
		free(encData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 8, &auth2, 0, &data->comm)) {
		/* If loading the 2nd auth fails, the first one was entity auth */
		pParentAuth = NULL;
		pEntityAuth = &auth1;
	} else {
		/* If loading the 2nd auth succeeds, the first one was parent auth */
		pParentAuth = &auth1;
		pEntityAuth = &auth2;
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CreateMigrationBlob_Internal(hContext, parentHandle, migrationType,
						   MigrationKeyAuthSize, MigrationKeyAuth,
						   encDataSize, encData, pParentAuth, pEntityAuth,
						   &randomSize, &random, &outDataSize, &outData);

	MUTEX_UNLOCK(tcsp_lock);

	free(MigrationKeyAuth);
	free(encData);
	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 6);
		if (pParentAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pParentAuth, 0, &data->comm)) {
				free(random);
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pEntityAuth, 0, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &randomSize, 0, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (randomSize > 0) {
			if (setData(TCSD_PACKET_TYPE_PBYTE, i++, random, randomSize, &data->comm)) {
				free(random);
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}

		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &outDataSize, 0, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, outData, outDataSize, &data->comm)) {
			free(random);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		free(random);
		free(outData);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ConvertMigrationBlob(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TCS_KEY_HANDLE parentHandle;
	UINT32 outDataSize, randomSize, inDataSize;
	BYTE *outData, *random, *inData;
	TPM_AUTH parentAuth, *pParentAuth;
	UINT32 i;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &parentHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &inDataSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	inData = (BYTE *)malloc(inDataSize);
	if (inData == NULL) {
		LogError("malloc of %d bytes failed.", inDataSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, inData, inDataSize, &data->comm)) {
		free(inData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 4, &randomSize, 0, &data->comm)) {
		free(inData);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	random = (BYTE *)malloc(randomSize);
	if (random == NULL) {
		free(inData);
		LogError("malloc of %d bytes failed.", randomSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 5, random, randomSize, &data->comm)) {
		free(inData);
		free(random);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 6, &parentAuth, 0, &data->comm))
		pParentAuth = NULL;
	else
		pParentAuth = &parentAuth;


	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ConvertMigrationBlob_Internal(hContext, parentHandle, inDataSize, inData,
						    randomSize, random, pParentAuth, &outDataSize,
						    &outData);

	MUTEX_UNLOCK(tcsp_lock);

	free(inData);
	free(random);
	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if (pParentAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pParentAuth, 0, &data->comm)) {
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

TSS_RESULT
tcs_wrap_AuthorizeMigrationKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_RESULT result;
	TSS_MIGRATE_SCHEME migrateScheme;
	UINT32 MigrationKeySize, MigrationKeyAuthSize;
	BYTE *MigrationKey, *MigrationKeyAuth;
	TPM_AUTH ownerAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT16, 1, &migrateScheme, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &MigrationKeySize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MigrationKey = (BYTE *)malloc(MigrationKeySize);
	if (MigrationKey == NULL) {
		LogError("malloc of %d bytes failed.", MigrationKeySize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, MigrationKey, MigrationKeySize, &data->comm)) {
		free(MigrationKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &ownerAuth, 0, &data->comm)) {
		free(MigrationKey);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_AuthorizeMigrationKey_Internal(hContext, migrateScheme, MigrationKeySize,
						     MigrationKey, &ownerAuth,
						     &MigrationKeyAuthSize, &MigrationKeyAuth);

	MUTEX_UNLOCK(tcsp_lock);

	free(MigrationKey);
	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_AUTH, 0, &ownerAuth, 0, &data->comm)) {
			free(MigrationKeyAuth);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &MigrationKeyAuthSize, 0, &data->comm)) {
			free(MigrationKeyAuth);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, MigrationKeyAuth, MigrationKeyAuthSize,
			    &data->comm)) {
			free(MigrationKeyAuth);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		free(MigrationKeyAuth);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
