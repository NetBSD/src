
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
tcs_wrap_TCSGetCapability(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCPA_CAPABILITY_AREA capArea;
	UINT32 subCapSize;
	BYTE *subCap;
	UINT32 respSize;
	BYTE *resp;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &capArea, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &subCapSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	subCap = calloc(1, subCapSize);
	if (subCap == NULL) {
		LogError("malloc of %u bytes failed.", subCapSize);
		return TCSERR(TSS_E_OUTOFMEMORY);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 3, subCap, subCapSize, &data->comm)) {
		free(subCap);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	result = TCS_GetCapability_Internal(hContext, capArea, subCapSize, subCap,
				       &respSize, &resp);
	free(subCap);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 2);
		if (setData(TCSD_PACKET_TYPE_UINT32, 0, &respSize, 0, &data->comm)) {
			free(resp);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 1, resp, respSize, &data->comm)) {
			free(resp);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(resp);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
