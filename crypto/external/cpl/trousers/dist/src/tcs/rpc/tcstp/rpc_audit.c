
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
tcs_wrap_SetOrdinalAuditStatus(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_AUTH ownerAuth;
	UINT32 ulOrdinal;
	TSS_BOOL bAuditState;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &ulOrdinal, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_BOOL, 2, &bAuditState, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (getData(TCSD_PACKET_TYPE_AUTH, 3, &ownerAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_SetOrdinalAuditStatus_Internal(hContext, &ownerAuth, ulOrdinal, bAuditState);

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
tcs_wrap_GetAuditDigest(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	UINT32 startOrdinal;
	TPM_DIGEST auditDigest;
	UINT32 counterValueSize;
	BYTE *counterValue;
	TSS_BOOL more;
	UINT32 ordSize;
	UINT32 *ordList;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &startOrdinal, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetAuditDigest_Internal(hContext, startOrdinal, &auditDigest, &counterValueSize, &counterValue,
						&more, &ordSize, &ordList);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 6);
		if (setData(TCSD_PACKET_TYPE_DIGEST, 0, &auditDigest, 0, &data->comm)) {
			free(counterValue);
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 1, &counterValueSize, 0, &data->comm)) {
			free(counterValue);
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 2, counterValue, counterValueSize, &data->comm)) {
			free(counterValue);
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(counterValue);
		if (setData(TCSD_PACKET_TYPE_BOOL, 3, &more, 0, &data->comm)) {
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 4, &ordSize, 0, &data->comm)) {
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 5, ordList, ordSize * sizeof(UINT32), &data->comm)) {
			free(ordList);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(ordList);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_GetAuditDigestSigned(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE keyHandle;
	TSS_BOOL closeAudit;
	TPM_NONCE antiReplay;
	TPM_AUTH privAuth, nullAuth, *pAuth;
	UINT32 counterValueSize;
	BYTE *counterValue;
	TPM_DIGEST auditDigest;
	TPM_DIGEST ordinalDigest;
	UINT32 sigSize;
	BYTE *sig;
	TSS_RESULT result;
	int i;

	memset(&privAuth, 0, sizeof(TPM_AUTH));
	memset(&nullAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &keyHandle, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_BOOL, 2, &closeAudit, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_NONCE, 3, &antiReplay, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &privAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (memcmp(&nullAuth, &privAuth, sizeof(TPM_AUTH)))
		pAuth = &privAuth;
	else
		pAuth = NULL;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_GetAuditDigestSigned_Internal(hContext, keyHandle, closeAudit, antiReplay,
							pAuth, &counterValueSize, &counterValue,
							&auditDigest, &ordinalDigest,
							&sigSize, &sig);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 7);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(counterValue);
				free(sig);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &counterValueSize, 0, &data->comm)) {
			free(counterValue);
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, counterValue, counterValueSize, &data->comm)) {
			free(counterValue);
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(counterValue);
		if (setData(TCSD_PACKET_TYPE_DIGEST, i++, &auditDigest, 0, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_DIGEST, i++, &ordinalDigest, 0, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &sigSize, 0, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, sig, sigSize, &data->comm)) {
			free(sig);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(sig);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
