
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
tcs_wrap_CertifyKey(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE certHandle, keyHandle;
	TPM_AUTH *pCertAuth = NULL, *pKeyAuth = NULL, certAuth, keyAuth, nullAuth;
	UINT32 CertifyInfoSize, outDataSize;
	BYTE *CertifyInfo, *outData;
	TCPA_NONCE antiReplay;
	TSS_RESULT result;
	UINT32 i;

	memset(&nullAuth, 0, sizeof(TPM_AUTH));
	memset(&certAuth, 0, sizeof(TPM_AUTH));
	memset(&keyAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &certHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &keyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_NONCE, 3, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &certAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 5, &keyAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (memcmp(&nullAuth, &certAuth, sizeof(TPM_AUTH)))
		pCertAuth = &certAuth;

	if (memcmp(&nullAuth, &keyAuth, sizeof(TPM_AUTH)))
		pKeyAuth = &keyAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_CertifyKey_Internal(hContext, certHandle, keyHandle, antiReplay, pCertAuth,
					  pKeyAuth, &CertifyInfoSize, &CertifyInfo, &outDataSize,
					  &outData);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 6);
		if (pCertAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pCertAuth, 0, &data->comm)) {
				free(CertifyInfo);
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (pKeyAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pKeyAuth, 0, &data->comm)) {
				free(CertifyInfo);
				free(outData);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &CertifyInfoSize, 0, &data->comm)) {
			free(CertifyInfo);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, CertifyInfo, CertifyInfoSize, &data->comm)) {
			free(CertifyInfo);
			free(outData);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(CertifyInfo);
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
