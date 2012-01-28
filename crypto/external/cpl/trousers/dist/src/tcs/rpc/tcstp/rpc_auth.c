
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
tcs_wrap_OIAP(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_AUTHHANDLE authHandle;
	TCPA_NONCE n0;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	MUTEX_LOCK(tcsp_lock);

	result = auth_mgr_oiap(hContext, &authHandle, &n0);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &authHandle, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_NONCE, 1, &n0, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_OSAP(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_ENTITY_TYPE entityType;
	UINT32 entityValue;
	TCPA_NONCE nonceOddOSAP;

	TCS_AUTHHANDLE authHandle;
	TCPA_NONCE nonceEven;
	TCPA_NONCE nonceEvenOSAP;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT16, 1, &entityType, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &entityValue, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_NONCE, 3, &nonceOddOSAP, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	MUTEX_LOCK(tcsp_lock);

	result = auth_mgr_osap(hContext, entityType, entityValue, nonceOddOSAP,
			       &authHandle, &nonceEven, &nonceEvenOSAP);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 3);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &authHandle, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_NONCE, 1, &nonceEven, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_NONCE, 2, &nonceEvenOSAP, 0, &data->comm)) {
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;

	return TSS_SUCCESS;
}
