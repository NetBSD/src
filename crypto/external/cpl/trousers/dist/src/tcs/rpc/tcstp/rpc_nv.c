/*
 * The Initial Developer of the Original Code is Intel Corporation.
 * Portions created by Intel Corporation are Copyright (C) 2007 Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 *
 * trousers - An open source TCG Software Stack
 *
 * Author: james.xu@intel.com Rossey.liu@intel.com
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
tcs_wrap_NV_DefineOrReleaseSpace(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 cPubInfoSize;
	BYTE *pubInfo = NULL;
	TSS_RESULT result;
	TPM_ENCAUTH encAuth;
	TPM_AUTH Auth, *pAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &cPubInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	pubInfo = calloc(1, cPubInfoSize);
	if (pubInfo == NULL) {
		LogError("malloc of %u bytes failed.", cPubInfoSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 2, pubInfo, cPubInfoSize, &data->comm)) {
		free(pubInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_ENCAUTH, 3, &encAuth, 0, &data->comm)) {
		free(pubInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &Auth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &Auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_NV_DefineOrReleaseSpace_Internal(hContext,
						       cPubInfoSize, pubInfo, encAuth, pAuth);

	MUTEX_UNLOCK(tcsp_lock);

	free(pubInfo);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if ( pAuth) {
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
tcs_wrap_NV_WriteValue(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_NV_INDEX hNVStore;
	UINT32 offset,ulDataLength;
	BYTE *rgbDataToWrite = NULL;
	TSS_RESULT result;
	TPM_AUTH Auth, *pAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbDataToWrite = calloc(1, ulDataLength);
	if (rgbDataToWrite == NULL) {
		LogError("malloc of %u bytes failed.", ulDataLength);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, rgbDataToWrite, ulDataLength, &data->comm)) {
		free(rgbDataToWrite);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &Auth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &Auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_NV_WriteValue_Internal(hContext, hNVStore,
					     offset, ulDataLength, rgbDataToWrite, pAuth);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbDataToWrite);

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
tcs_wrap_NV_WriteValueAuth(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_NV_INDEX hNVStore;
	UINT32 offset,ulDataLength;
	BYTE *rgbDataToWrite = NULL;
	TSS_RESULT result;
	TPM_AUTH Auth, *pAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbDataToWrite = calloc(1, ulDataLength);
	if (rgbDataToWrite == NULL) {
		LogError("malloc of %u bytes failed.", ulDataLength);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}

	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, rgbDataToWrite, ulDataLength, &data->comm)) {
		free(rgbDataToWrite);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &Auth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &Auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_NV_WriteValueAuth_Internal(hContext, hNVStore,
						 offset, ulDataLength, rgbDataToWrite, pAuth);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbDataToWrite);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 1);
		if ( pAuth) {
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
tcs_wrap_NV_ReadValue(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_NV_INDEX hNVStore;
	UINT32 offset,ulDataLength, i;
	BYTE *rgbDataRead = NULL;
	TSS_RESULT result;
	TPM_AUTH Auth, *pAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &Auth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &Auth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_NV_ReadValue_Internal(hContext, hNVStore,
					    offset, &ulDataLength, pAuth, &rgbDataRead);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if ( pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(rgbDataRead);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &ulDataLength, 0, &data->comm)) {
			free(rgbDataRead);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, rgbDataRead, ulDataLength, &data->comm)) {
			free(rgbDataRead);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(rgbDataRead);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_NV_ReadValueAuth(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TSS_NV_INDEX hNVStore;
	UINT32 offset,ulDataLength, i;
	BYTE *rgbDataRead = NULL;
	TSS_RESULT result;
	TPM_AUTH NVAuth, *pNVAuth;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hNVStore, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &offset, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &ulDataLength, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &NVAuth, 0, &data->comm)) {
		pNVAuth = NULL;
	} else {
		pNVAuth = &NVAuth;
	}

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_NV_ReadValueAuth_Internal(hContext, hNVStore,
						offset, &ulDataLength, pNVAuth, &rgbDataRead);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 3);
		if ( pNVAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pNVAuth, 0, &data->comm)) {
				free(rgbDataRead);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &ulDataLength, 0, &data->comm)) {
			free(rgbDataRead);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, rgbDataRead, ulDataLength, &data->comm)) {
			free(rgbDataRead);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(rgbDataRead);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

